#include "history_jobs.h"

#include <climits>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace mhhist {

bool removeDirStep(Fs& fs, const char* path, int& budget, int* progress) {
    std::vector<mhfs::DirEntry> kids; // (chemin absolu, est un dossier)
    const mhfs::DirResult res = fs.listDir(path, mhfs::Want::All, 16, kids);
    if (res == mhfs::DirResult::Missing) return true;
    if (res == mhfs::DirResult::NotDir) {
        if (fs.remove(path) && progress) (*progress)++;
        budget--;
        return true;
    }
    if (kids.empty()) {
        if (fs.rmdir(path) && progress) (*progress)++;
        return true;
    }

    for (const auto& k : kids) {
        if (budget <= 0) return false;
        if (k.isDir) {
            removeDirStep(fs, k.path.c_str(), budget, progress);
        } else {
            if (fs.remove(k.path.c_str()) && progress) (*progress)++;
            budget--;
        }
    }
    return false; // il reste peut-etre des entrees : prochain passage
}

void removeDirRecursive(Fs& fs, const char* path) {
    int unlimited = INT_MAX;
    while (!removeDirStep(fs, path, unlimited)) {}
}

void createHistoryTree(Fs& fs, const char* baseDir, const char* inRoot, const char* outRoot) {
    // Arborescence symetrique : /history/indoor et /history/outdoor cote a cote.
    if (!fs.exists(baseDir) && !fs.mkdir(baseDir)) {
        MH_LOG_ERROR(std::string("Failed to create ") + baseDir + " directory on SD card.");
        return;
    }
    if (!fs.exists(inRoot)) fs.mkdir(inRoot);
    if (!fs.exists(outRoot)) fs.mkdir(outRoot);
    MH_LOG_INFO(std::string("SD history structure ready (") + inRoot + " + " + outRoot + ").");
}

void ClearJob::service(Fs& fs, const char* historyDir, bool sdAvailable,
                       const std::function<void()>& clearRam,
                       const std::function<void()>& onTreeRemoved) {
    if (!_pending) return;
    if (!_active) {
        clearRam();
        _active = true;
        _stuckPasses = 0;
        MH_LOG_INFO("Clearing history (batch mode)...");
    }
    if (!sdAvailable) {
        _active = _pending = false;
        MH_LOG_INFO("History cleared (IN + OUT, no SD)");
        return;
    }
    int budget = 20; // fichiers supprimes par passage de loop()
    int removed = 0; // suppressions reussies : le budget baisse aussi quand remove() echoue
    const bool done = removeDirStep(fs, historyDir, budget, &removed);
    if (done) {
        onTreeRemoved();
        _active = _pending = false;
        MH_LOG_INFO("History cleared (IN + OUT)");
    } else if (removed > 0) {
        _stuckPasses = 0;
    } else if (++_stuckPasses > 20) {
        // Rien ne part (carte en lecture seule, entree verrouillee) : abandonner plutot que de
        // boucler a chaque tour de loop(). Un nouvel request() repart de zero (_active=false).
        _active = _pending = false;
        MH_LOG_ERROR("History clear aborted after 20 passes without any successful removal "
                     "(SD read-only or entries locked)");
    }
}

void migrateCsvToBinary(DayStore& inStore) {
    Fs& fs = inStore.fs();
    const char* csvDir = inStore.policy().legacyCsvDir;
    if (!csvDir) return;

    // Collecte d'abord les CSV a migrer (on ne modifie pas le dossier en le parcourant).
    std::vector<mhfs::DirEntry> entries;
    if (fs.listDir(csvDir, mhfs::Want::Files, static_cast<size_t>(-1), entries) != mhfs::DirResult::Ok) return;
    std::vector<std::string> csvFiles;
    for (const auto& e : entries) {
        if (e.name.size() > 4 && e.name.compare(e.name.size() - 4, 4, ".csv") == 0) csvFiles.push_back(e.path);
    }

    if (csvFiles.empty()) return;
    MH_LOG_INFO("History migration: " + std::to_string(csvFiles.size()) + " CSV file(s) to convert");

    for (const auto& csvPath : csvFiles) {
        mhfs::FilePtr in = fs.open(csvPath.c_str(), mhfs::Mode::Read);
        if (!in) continue;

        DayStats stats;
        stats.magic = kDayStatsMagic;
        struct tm firstTm;
        bool haveDir = false;
        char binPath[64] = {0}, statsPath[64] = {0};
        mhfs::FilePtr out;
        FileHeader mhdr = makeHeader();

        bool first_line = true;
        bool failed = false; // echec : le CSV doit rester en place pour une nouvelle tentative
        size_t line_count = 0;
        mhfs::LineReader lines(*in);
        std::string line;
        while (lines.next(line)) {
            MH_YIELD_EVERY(line_count, 32);
            line_count++;
            if (first_line) { first_line = false; continue; } // en-tete CSV
            if (line.empty()) continue;

            long ts = 0; float t = 0, h = 0, p = 0;
            if (sscanf(line.c_str(), "%ld,%f,%f,%f", &ts, &t, &h, &p) != 4) continue;

            // Cree l'arborescence et ouvre le .bin au premier echantillon valide.
            if (!haveDir) {
                time_t tt = static_cast<time_t>(ts);
                if (!localtime_r(&tt, &firstTm)) { failed = true; break; }
                inStore.ensureDirs(firstTm);
                inStore.dayPaths(firstTm, binPath, statsPath, sizeof(binPath));
                // Ne jamais ecraser un .bin existant (mesures deja archivees ce jour-la).
                if (fs.exists(binPath)) {
                    MH_LOG_WARNING(std::string("History migration: .bin existant, CSV conserve ") + binPath);
                    failed = true; break;
                }
                out = fs.open(binPath, mhfs::Mode::Write);
                if (!out) { failed = true; break; }
                if (out->write(reinterpret_cast<const uint8_t*>(&mhdr), sizeof(mhdr)) != sizeof(mhdr)) {
                    failed = true; break; // en-tete provisoire
                }
                haveDir = true;
            }

            BinRecord br { static_cast<uint32_t>(ts), t, h, p };
            if (out->write(reinterpret_cast<const uint8_t*>(&br), sizeof(br)) != sizeof(br)) {
                failed = true; break;
            }

            if (stats.count == 0) {
                stats.first_ts = static_cast<uint32_t>(ts);
                stats.t_min = stats.t_max = stats.t_first = t;
                stats.h_min = stats.h_max = stats.h_first = h;
                stats.p_min = stats.p_max = stats.p_first = p;
            }
            if (t < stats.t_min) stats.t_min = t;
            if (t > stats.t_max) stats.t_max = t;
            if (h < stats.h_min) stats.h_min = h;
            if (h > stats.h_max) stats.h_max = h;
            if (p < stats.p_min) stats.p_min = p;
            if (p > stats.p_max) stats.p_max = p;
            stats.t_sum += t; stats.h_sum += h; stats.p_sum += p;
            stats.t_last = t; stats.h_last = h; stats.p_last = p;
            stats.last_ts = static_cast<uint32_t>(ts);
            stats.count++;
        }
        in->close();

        if (failed) {
            // Migration interrompue : on retire le .bin partiel que NOUS avons cree et on garde
            // le CSV intact (pas de .bak), il sera retente au prochain demarrage.
            if (haveDir && out) { out->close(); fs.remove(binPath); }
            MH_LOG_WARNING("Migration FAILED, CSV kept: " + csvPath);
            continue;
        }

        if (haveDir && out) {
            // Finalise l'en-tete (compteur + horodatages) au debut du .bin.
            mhdr.recordCount = stats.count;
            mhdr.firstTimestamp = stats.first_ts;
            mhdr.lastTimestamp = stats.last_ts;
            out->seek(0);
            out->write(reinterpret_cast<const uint8_t*>(&mhdr), sizeof(mhdr));
            out->flush();
            out->close();
            writeFileReplacing(fs, statsPath, &stats, sizeof(stats));
        }

        // Renomme le CSV source pour ne pas le retraiter au prochain demarrage.
        std::string bak = csvPath + ".bak";
        fs.rename(csvPath.c_str(), bak.c_str());
        MH_LOG_INFO("Migrated " + csvPath + " (" + std::to_string(stats.count) + " points)");
    }

    inStore.resetStatsCache(); // forcera un rechargement propre du .stats du jour courant
    MH_LOG_INFO("History migration complete");
}

} // namespace mhhist
