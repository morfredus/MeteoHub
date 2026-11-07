#pragma once
// Adaptateur Arduino de l'interface de fichiers du code portable (history/fs_port.h) :
// enveloppe un fs::FS (SD ou LittleFS). Aucune semantique ajoutee, chaque appel est transmis
// tel quel a l'API d'Arduino.
#include <FS.h>
#include <cstring>

#include "../history/fs_port.h"
#include "sd_manager.h"

class ArduinoFile : public mhfs::FileHandle {
public:
    explicit ArduinoFile(File f) : _f(f) {}
    size_t size() override { return _f.size(); }
    bool seek(uint32_t pos) override { return _f.seek(pos); }
    int read(uint8_t* buf, size_t len) override { return _f.read(buf, len); }
    size_t write(const uint8_t* buf, size_t len) override { return _f.write(buf, len); }
    void flush() override { _f.flush(); }
    void close() override { _f.close(); }
private:
    File _f;
};

class ArduinoFs : public mhfs::Fs {
public:
    explicit ArduinoFs(fs::FS& fs) : _fs(fs) {}

    bool exists(const char* path) override { return _fs.exists(path); }
    bool mkdir(const char* path) override { return _fs.mkdir(path); }
    bool rmdir(const char* path) override { return _fs.rmdir(path); }
    bool remove(const char* path) override { return _fs.remove(path); }
    bool rename(const char* from, const char* to) override { return _fs.rename(from, to); }

    mhfs::FilePtr open(const char* path, mhfs::Mode mode) override {
        const char* m = FILE_READ;
        switch (mode) {
            case mhfs::Mode::Read:      m = FILE_READ; break;
            case mhfs::Mode::Write:     m = FILE_WRITE; break;
            case mhfs::Mode::Append:    m = FILE_APPEND; break;
            case mhfs::Mode::ReadWrite: m = "r+"; break;
        }
        File f = _fs.open(path, m);
        if (!f) return nullptr;
        return mhfs::FilePtr(new ArduinoFile(f));
    }

    mhfs::DirResult listDir(const char* path, mhfs::Want want, size_t maxEntries,
                            std::vector<mhfs::DirEntry>& out) override {
        File dir = _fs.open(path);
        if (!dir) return mhfs::DirResult::Missing;
        if (!dir.isDirectory()) { dir.close(); return mhfs::DirResult::NotDir; }
        for (File e = dir.openNextFile(); e && out.size() < maxEntries; e = dir.openNextFile()) {
            const bool isDir = e.isDirectory();
            if (want == mhfs::Want::All || (want == mhfs::Want::Dirs) == isDir) {
                const char* nm = e.name();
                const char* base = strrchr(nm, '/');
                mhfs::DirEntry de;
                de.name = base ? base + 1 : nm;
                de.path = e.path();
                de.isDir = isDir;
                out.push_back(std::move(de));
            }
            e.close();
        }
        dir.close();
        return mhfs::DirResult::Ok;
    }

protected:
    fs::FS& _fs;
};

// Carte SD : en lecture, la carte est remontee au besoin par le SdManager (ensureMounted).
class SdCardFs : public ArduinoFs {
public:
    explicit SdCardFs(fs::FS& fs) : ArduinoFs(fs) {}
    void attach(SdManager* sd) { _sd = sd; }
    bool ensureMounted() override { return _sd && _sd->ensureMounted(); }
private:
    SdManager* _sd = nullptr;
};
