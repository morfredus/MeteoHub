#pragma once
// Petite interface d'acces aux fichiers (SD ou LittleFS) pour le code portable de
// l'historique. Le firmware branche l'implementation Arduino (managers/arduino_fs.h) ; les
// tests natifs branchent un volume en memoire. Le code de src/history/ ne touche donc jamais
// SD ou LittleFS directement et se teste sur l'hote.
//
// Les operations reprennent a l'identique celles de l'API fs::FS d'Arduino (open/exists/
// mkdir/rmdir/remove/rename) : aucune semantique nouvelle, pour que le comportement sur la
// carte reste celui du code d'origine.
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mhfs {

enum class Mode { Read, Write, Append, ReadWrite };  // "r", "w", "a", "r+"

// Un fichier ouvert. Les signatures suivent File d'Arduino (read renvoie un int, etc.) pour
// que les modeles de day_file_format.h s'appliquent indifferemment aux deux.
class FileHandle {
public:
    virtual ~FileHandle() {}
    virtual size_t size() = 0;
    virtual bool seek(uint32_t pos) = 0;
    virtual int read(uint8_t* buf, size_t len) = 0;
    virtual size_t write(const uint8_t* buf, size_t len) = 0;
    virtual void flush() = 0;
    virtual void close() = 0;  // idempotent
};
using FilePtr = std::unique_ptr<FileHandle>;

struct DirEntry {
    std::string path;  // chemin absolu
    std::string name;  // nom de base
    bool isDir = false;
};

enum class Want { All, Dirs, Files };

// Resultat d'un listage : dossier absent, entree qui n'est pas un dossier, ou liste obtenue.
enum class DirResult { Missing, NotDir, Ok };

class Fs {
public:
    virtual ~Fs() {}
    // Monte la carte si besoin (lecture) : faux si elle est inaccessible. Sans objet pour
    // LittleFS (toujours vrai).
    virtual bool ensureMounted() { return true; }
    virtual bool exists(const char* path) = 0;
    virtual bool mkdir(const char* path) = 0;
    virtual bool rmdir(const char* path) = 0;
    virtual bool remove(const char* path) = 0;
    virtual bool rename(const char* from, const char* to) = 0;
    // nullptr si l'ouverture echoue.
    virtual FilePtr open(const char* path, Mode mode) = 0;
    // Liste les entrees d'UN dossier puis le referme aussitot. Ce decoupage n'est pas
    // cosmetique : garder ouvert l'iterateur d'un dossier pendant qu'on parcourt ses enfants
    // bloque la lecture de la carte SD sur ESP32 (la premiere version de listDays() ne
    // repondait jamais). `maxEntries` borne le nombre d'entrees RETENUES (apres filtre).
    virtual DirResult listDir(const char* path, Want want, size_t maxEntries,
                              std::vector<DirEntry>& out) = 0;
};

// Lecture d'un fichier texte ligne par ligne (equivalent de readStringUntil('\n') tant que
// available()) : une ligne par appel, '\n' exclu, '\r' conserve ; la derniere ligne peut ne
// pas etre terminee par '\n'. Faux a la fin du fichier.
class LineReader {
public:
    explicit LineReader(FileHandle& f) : _f(f) {}
    bool next(std::string& line) {
        line.clear();
        bool any = false;
        for (;;) {
            if (_pos == _len) {
                if (_eof) return any;
                const int n = _f.read(_buf, sizeof(_buf));
                if (n <= 0) { _eof = true; return any; }
                _len = static_cast<size_t>(n);
                _pos = 0;
            }
            const char c = static_cast<char>(_buf[_pos++]);
            any = true;
            if (c == '\n') return true;
            line += c;
        }
    }
private:
    FileHandle& _f;
    uint8_t _buf[128];
    size_t _len = 0, _pos = 0;
    bool _eof = false;
};

} // namespace mhfs
