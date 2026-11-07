#pragma once
// Volume de fichiers EN MEMOIRE, partage par le shim Arduino (code de reference) et par
// l'implementation memoire de mhfs::Fs (code refondu). Les deux travaillent ainsi sur la
// MEME structure de donnees, ce qui permet de comparer octet pour octet ce que chacun
// ecrit pour une meme suite d'operations.
//
// Comportements imites de la carte SD (FAT) et de LittleFS :
//  - mkdir echoue si le parent manque ou si l'entree existe deja ;
//  - un fichier ne se cree que dans un dossier existant ;
//  - rename ne remplace PAS une cible existante en FAT, la remplace en LittleFS
//    (renameReplaces) ;
//  - quota d'octets optionnel pour simuler une partition pleine (ecriture partielle).
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

struct MemVolume {
    std::map<std::string, std::vector<uint8_t>> files;
    std::set<std::string> dirs;
    bool renameReplaces = false;
    long quota = -1;               // octets encore ecrivables (-1 = illimite)
    bool failOpenForWrite = false; // simule une carte en lecture seule / retiree
    bool removeFails = false;      // simule une entree verrouillee (remove / rmdir echouent)
    unsigned long opens = 0;       // nombre d'ouvertures (mesure du cout)

    static std::string parentOf(const std::string& p) {
        const size_t k = p.find_last_of('/');
        if (k == std::string::npos || k == 0) return "/";
        return p.substr(0, k);
    }
    bool dirExists(const std::string& p) const { return p == "/" || dirs.count(p) > 0; }
    bool exists(const std::string& p) const { return files.count(p) > 0 || dirExists(p); }
    bool mkdir(const std::string& p) {
        if (exists(p) || !dirExists(parentOf(p))) return false;
        dirs.insert(p);
        return true;
    }
    bool rmdir(const std::string& p) {
        if (removeFails || !dirs.count(p)) return false;
        for (const auto& kv : files) if (parentOf(kv.first) == p) return false;
        for (const auto& d : dirs) if (parentOf(d) == p) return false;
        dirs.erase(p);
        return true;
    }
    bool remove(const std::string& p) { return !removeFails && files.erase(p) > 0; }
    bool rename(const std::string& a, const std::string& b) {
        auto it = files.find(a);
        if (it == files.end()) return false;
        if (files.count(b)) {
            if (!renameReplaces) return false;
            files.erase(b);
        }
        files[b] = it->second;
        files.erase(a);
        return true;
    }
    // Enfants directs (chemin absolu, est un dossier), tries par nom (ordre stable).
    std::vector<std::pair<std::string, bool>> children(const std::string& dir) const {
        std::vector<std::pair<std::string, bool>> out;
        for (const auto& kv : files) if (parentOf(kv.first) == dir) out.emplace_back(kv.first, false);
        for (const auto& d : dirs) if (parentOf(d) == dir) out.emplace_back(d, true);
        std::sort(out.begin(), out.end());
        return out;
    }
};

enum class MemMode { Read, Write, Append, ReadWrite };

// Etat d'un fichier ouvert (ou d'un dossier ouvert).
struct MemFileState {
    MemVolume* v = nullptr;
    std::string path;
    MemMode mode = MemMode::Read;
    bool isDir = false;
    size_t pos = 0;
    std::vector<std::pair<std::string, bool>> kids; // dossier : instantane des enfants
    size_t kidPos = 0;

    static std::shared_ptr<MemFileState> open(MemVolume& v, const std::string& p, MemMode m) {
        v.opens++;
        auto st = std::make_shared<MemFileState>();
        st->v = &v; st->path = p; st->mode = m;
        if (m == MemMode::Read && v.dirExists(p)) {
            st->isDir = true;
            st->kids = v.children(p);
            return st;
        }
        if (m == MemMode::Read || m == MemMode::ReadWrite) {
            if (!v.files.count(p)) return nullptr;
            if (m == MemMode::ReadWrite && v.failOpenForWrite) return nullptr;
            return st;
        }
        if (v.failOpenForWrite) return nullptr;
        if (!v.dirExists(MemVolume::parentOf(p))) return nullptr;
        if (m == MemMode::Write) v.files[p].clear();
        else if (!v.files.count(p)) v.files[p];
        if (m == MemMode::Append) st->pos = v.files[p].size();
        return st;
    }
    size_t size() const {
        auto it = v->files.find(path);
        return it == v->files.end() ? 0 : it->second.size();
    }
    bool seek(size_t p) {
        if (p > size()) return false;
        pos = p;
        return true;
    }
    int read(uint8_t* buf, size_t len) {
        auto it = v->files.find(path);
        if (it == v->files.end()) return -1;
        const auto& d = it->second;
        if (pos >= d.size()) return 0;
        const size_t n = std::min(len, d.size() - pos);
        memcpy(buf, d.data() + pos, n);
        pos += n;
        return (int)n;
    }
    size_t write(const uint8_t* buf, size_t len) {
        if (mode == MemMode::Read) return 0;
        auto it = v->files.find(path);
        if (it == v->files.end()) return 0;
        auto& d = it->second;
        size_t n = len;
        if (v->quota >= 0) {
            if ((long)n > v->quota) n = (size_t)v->quota;
            v->quota -= (long)n;
        }
        if (mode == MemMode::Append) pos = d.size();
        if (pos + n > d.size()) d.resize(pos + n);
        memcpy(d.data() + pos, buf, n);
        pos += n;
        return n;
    }
};
