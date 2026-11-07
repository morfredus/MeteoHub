#pragma once
// Tampon des dernieres mesures (RAM) adosse a un fichier LittleFS, pour un flux (IN ou OUT).
// Le tampon sert l'affichage et les calculs courts (24 h) ; l'archive longue vit sur la SD.
//
//  - capacite fixe : on ne garde que les `capacity` dernieres mesures en RAM ;
//  - le fichier est ecrit en AJOUT SEUL, puis reecrit (compaction) quand il depasse le seuil :
//    fichier temporaire complet + rename, de sorte qu'a tout instant le fichier definitif est
//    soit l'ancien complet, soit le nouveau complet (LittleFS : rename remplace la cible) ;
//  - l'ajout en RAM se fait sous le verrou partage `ramMutex` (les pages web lisent ces
//    tampons depuis async_tcp) ; l'ecriture du fichier et la compaction n'y touchent pas : loop()
//    est le seul ecrivain.
//
// Le FORMAT du fichier est celui du Codec : binaire brut pour l'IN, CSV texte pour l'OUT. Il
// est identique, octet pour octet, a celui des firmwares precedents (les fichiers presents sur
// la partition doivent rester lisibles apres mise a jour comme apres retour arriere).
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

#include "fs_port.h"
#include "history_platform.h"
#include "history_types.h"

namespace mhhist {

// --- Format IN : structure brute de 16 octets ----------------------------------------------
// Sur l'ESP32 time_t fait 32 bits : l'ancien code ecrivait sizeof(HistoryRecord) = 16 octets
// bruts. On fige cette disposition dans un type explicite plutot que de dependre de
// sizeof(HistoryRecord) (qui changerait avec un time_t 64 bits : relecture fausse de tous les
// fichiers existants).
struct __attribute__((packed)) InRecentDisk {
    int32_t ts;
    float t;
    float h;
    float p;
};
static_assert(sizeof(InRecentDisk) == 16, "fichier recent IN : 16 octets par mesure");
#ifdef ARDUINO
// Sur le firmware, la disposition historique (struct brute) DOIT etre celle de InRecentDisk.
static_assert(sizeof(HistoryRecord) == sizeof(InRecentDisk), "HistoryRecord doit rester a 16 octets");
static_assert(sizeof(time_t) == 4, "format recent IN : time_t 32 bits");
#endif

struct InRecentCodec {
    static constexpr const char* kName = "IN";
    static constexpr bool kFlushAfterAppend = true;  // l'ancien code vidait le tampon a chaque ajout IN
    static constexpr bool kAppendFailureIsError = true;

    static bool write(mhfs::FileHandle& f, const HistoryRecord& r) {
        InRecentDisk d;
        d.ts = static_cast<int32_t>(r.timestamp);
        d.t = r.t;
        d.h = r.h;
        d.p = r.p;
        return f.write(reinterpret_cast<const uint8_t*>(&d), sizeof(d)) == sizeof(d);
    }
    // Lit tous les enregistrements COMPLETS. fileUnits : enregistrements presents (= lus).
    static size_t load(mhfs::FileHandle& f, std::vector<HistoryRecord>& out, size_t& fileUnits) {
        size_t loaded = 0;
        InRecentDisk d;
        while (f.read(reinterpret_cast<uint8_t*>(&d), sizeof(d)) == (int)sizeof(d)) {
            HistoryRecord r;
            r.timestamp = static_cast<time_t>(d.ts);
            r.t = d.t;
            r.h = d.h;
            r.p = d.p;
            out.push_back(r);
            loaded++;
        }
        fileUnits = loaded;
        return loaded;
    }
};

// --- Format OUT : une ligne CSV "timestamp,t,h,p" par mesure -------------------------------
struct OutRecentCodec {
    static constexpr const char* kName = "OUT";
    static constexpr bool kFlushAfterAppend = false;
    static constexpr bool kAppendFailureIsError = false;

    static bool write(mhfs::FileHandle& f, const OutdoorHistoryRecord& r) {
        char buf[128];
        const int len = snprintf(buf, sizeof(buf), "%lu,%.2f,%.2f,%.2f\n",
                                 (unsigned long)r.timestamp, r.t, r.h, r.p);
        return len > 0 && f.write(reinterpret_cast<const uint8_t*>(buf), (size_t)len) == (size_t)len;
    }
    // fileUnits : lignes non vides presentes (meme illisibles) : base de la rotation.
    static size_t load(mhfs::FileHandle& f, std::vector<OutdoorHistoryRecord>& out, size_t& fileUnits) {
        size_t loaded = 0;
        fileUnits = 0;
        mhfs::LineReader lines(f);
        std::string line;
        while (lines.next(line)) {
            if (line.empty()) continue;
            fileUnits++;
            // Format CSV : timestamp,t,h,p. indexOf(',', from) renvoie -1 si absent.
            auto indexOf = [&](size_t from) -> int {
                const size_t k = line.find(',', from);
                return k == std::string::npos ? -1 : (int)k;
            };
            const int comma1 = indexOf(0);
            const int comma2 = indexOf((size_t)(comma1 + 1));
            const int comma3 = indexOf((size_t)(comma2 + 1));
            if (comma1 > 0 && comma2 > 0 && comma3 > 0) {
                OutdoorHistoryRecord r;
                r.timestamp = atol(line.substr(0, comma1).c_str());
                r.t = (float)atof(line.substr(comma1 + 1, comma2 - comma1 - 1).c_str());
                r.h = (float)atof(line.substr(comma2 + 1, comma3 - comma2 - 1).c_str());
                r.p = (float)atof(line.substr(comma3 + 1).c_str());
                // Extensions futures a 0 pour l'instant.
                r.wind_speed = 0; r.wind_gust = 0; r.wind_direction_deg = 0;
                r.rain_rate = 0; r.rain_accumulated = 0;
                r.solar_lux = 0; r.uv_index = 0;
                out.push_back(r);
                loaded++;
            }
        }
        return loaded;
    }
};

template <typename Record, typename Codec>
class RecentRing {
public:
    RecentRing(mhfs::Fs& fs, std::mutex& ramMutex, const char* path, const char* tmpPath,
               size_t capacity, size_t compactThreshold)
        : _fs(fs), _mutex(ramMutex), _path(path), _tmp(tmpPath),
          _capacity(capacity), _threshold(compactThreshold) {}

    // Contenu RAM (les `capacity` dernieres mesures, ordre d'arrivee). Lecture hors loop() :
    // prendre le verrou partage (ramMutex du gestionnaire).
    std::vector<Record>& items() { return _items; }
    const std::vector<Record>& items() const { return _items; }
    size_t fileRecords() const { return _fileRecords; }

    // Temporaire de compaction orphelin (coupure avant le rename) : le fichier definitif est
    // reste intact, on jette simplement le temporaire incomplet. A appeler avant load().
    void purgeTmp() {
        if (_fs.exists(_tmp)) _fs.remove(_tmp);
        _fileRecords = 0;
    }

    // Charge le fichier en RAM (sous reserve qu'il existe), borne a `capacity`, puis compacte
    // si le fichier a grossi sans borne (firmware anterieur a la rotation).
    void load() {
        if (!_fs.exists(_path)) return;
        mhfs::FilePtr f = _fs.open(_path, mhfs::Mode::Read);
        if (!f) return;
        size_t fileUnits = 0;
        const size_t loaded = Codec::load(*f, _items, fileUnits);
        f->close();

        // La RAM est un tampon recent ; le fichier a pu accumuler des semaines de mesures. On
        // ne garde que les dernieres, comme push() le fait au fil de l'eau, pour ne pas
        // immobiliser des Mo ni ralentir chaque calcul (et parce que la compaction reecrit la
        // RAM : elle doit etre bornee).
        if (_items.size() > _capacity) {
            _items.erase(_items.begin(), _items.end() - _capacity);
        }
        MH_LOG_INFO(std::string("History: Loaded ") + std::to_string(loaded) + " " + Codec::kName
                    + " records (kept " + std::to_string(_items.size()) + " in RAM)");

        _fileRecords = fileUnits;
        if (_fileRecords > _threshold) compact();
    }

    // Ajoute en RAM (sous verrou) en gardant les `capacity` dernieres.
    void push(const Record& r) {
        std::lock_guard<std::mutex> lock(_mutex);
        _items.push_back(r);
        if (_items.size() > _capacity) {
            _items.erase(_items.begin());
        }
    }

    // Ajoute au fichier (ajout seul) puis applique la rotation bornee. La mesure est deja en
    // RAM : la compaction reecrit le contenu RAM.
    void persist(const Record& r) {
        mhfs::FilePtr f = _fs.open(_path, mhfs::Mode::Append);
        if (!f) {
            const std::string msg = std::string("History: Failed to open ") + Codec::kName
                                    + " recent file for append";
            if (Codec::kAppendFailureIsError) MH_LOG_ERROR(msg);
            else MH_LOG_WARNING(msg);
        } else {
            Codec::write(*f, r);
            if (Codec::kFlushAfterAppend) f->flush();
            f->close();
            _fileRecords++;
        }
        if (_fileRecords > _threshold) compact();
    }

    // Reecrit uniquement le contenu RAM (= la fin utile du fichier, meme borne) via un
    // temporaire puis rename, sans relire le gros fichier.
    void compact() {
        mhfs::FilePtr f = _fs.open(_tmp, mhfs::Mode::Write);
        if (!f) {
            MH_LOG_ERROR(std::string("History: ") + Codec::kName + " compaction failed (cannot create tmp)");
            return;
        }
        size_t written = 0;
        for (const auto& r : _items) {
            if (!Codec::write(*f, r)) break;
            written++;
        }
        f->flush();
        f->close();

        // Ecriture incomplete (partition pleine...) : l'original reste valide, on jette le
        // temporaire et on retentera a la prochaine mesure.
        if (written != _items.size()) {
            _fs.remove(_tmp);
            MH_LOG_ERROR(std::string("History: ") + Codec::kName + " compaction failed (short write)");
            return;
        }
        // rename LittleFS = remplacement atomique de la cible.
        if (!_fs.rename(_tmp, _path)) {
            _fs.remove(_tmp);
            MH_LOG_ERROR(std::string("History: ") + Codec::kName + " compaction failed (rename)");
            return;
        }
        MH_LOG_INFO(std::string("History: ") + Codec::kName + " recent file compacted ("
                    + std::to_string(_fileRecords) + " -> " + std::to_string(written) + " records)");
        _fileRecords = written;
    }

    // Efface la RAM (appelant : verrou pris) et le fichier avec son temporaire.
    void clear() {
        _items.clear();
        _fs.remove(_path);
        _fs.remove(_tmp);
        _fileRecords = 0;
    }

private:
    mhfs::Fs& _fs;
    std::mutex& _mutex;
    const char* _path;
    const char* _tmp;
    size_t _capacity;
    size_t _threshold;
    std::vector<Record> _items;
    // Nombre d'enregistrements presents dans le fichier (compte au chargement, incremente a
    // chaque ajout) : declenche la compaction sans relire le fichier a chaque mesure.
    size_t _fileRecords = 0;
};

using InRecentRing = RecentRing<HistoryRecord, InRecentCodec>;
using OutRecentRing = RecentRing<OutdoorHistoryRecord, OutRecentCodec>;

} // namespace mhhist
