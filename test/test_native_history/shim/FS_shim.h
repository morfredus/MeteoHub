#pragma once
// Faux systeme de fichiers Arduino (File + SD + LittleFS) au-dessus de MemVolume.
#include <memory>
#include <string>
#include "Arduino.h"
#include "../mem_volume.h"

#define FILE_READ "r"
#define FILE_WRITE "w"
#define FILE_APPEND "a"

class File {
public:
    File() {}
    explicit File(std::shared_ptr<MemFileState> s) : s_(std::move(s)) {}
    explicit operator bool() const { return (bool)s_; }
    size_t size() { return s_ ? s_->size() : 0; }
    bool seek(uint32_t p) { return s_ && s_->seek(p); }
    int read(uint8_t* b, size_t n) { return s_ ? s_->read(b, n) : -1; }
    size_t write(const uint8_t* b, size_t n) { return s_ ? s_->write(b, n) : 0; }
    size_t print(const char* c) { return write((const uint8_t*)c, strlen(c)); }
    void flush() {}
    void close() { s_.reset(); }
    int available() { return s_ && s_->pos < s_->size() ? 1 : 0; }
    bool isDirectory() { return s_ && s_->isDir; }
    File openNextFile() {
        if (!s_ || !s_->isDir || s_->kidPos >= s_->kids.size()) return File();
        const auto& k = s_->kids[s_->kidPos++];
        auto st = std::make_shared<MemFileState>();
        st->v = s_->v; st->path = k.first; st->isDir = k.second;
        return File(st);
    }
    // name() = nom de base ; path() = chemin absolu (comme le core ESP32 2.x).
    const char* name() {
        const size_t k = s_->path.find_last_of('/');
        base_ = k == std::string::npos ? s_->path : s_->path.substr(k + 1);
        return base_.c_str();
    }
    const char* path() { return s_->path.c_str(); }
    String readStringUntil(char term) {
        std::string out;
        uint8_t c;
        while (s_ && s_->read(&c, 1) == 1) {
            if ((char)c == term) break;
            out += (char)c;
        }
        return String(out);
    }
    String readString() {
        std::string out;
        uint8_t c;
        while (s_ && s_->read(&c, 1) == 1) out += (char)c;
        return String(out);
    }
private:
    std::shared_ptr<MemFileState> s_;
    std::string base_;
};

class ShimFs {
public:
    MemVolume* vol = nullptr;
    File open(const char* p, const char* mode = "r") {
        MemMode m = MemMode::Read;
        if (!strcmp(mode, "w")) m = MemMode::Write;
        else if (!strcmp(mode, "a")) m = MemMode::Append;
        else if (!strcmp(mode, "r+")) m = MemMode::ReadWrite;
        return File(MemFileState::open(*vol, p, m));
    }
    bool exists(const char* p) { return vol->exists(p); }
    bool mkdir(const char* p) { return vol->mkdir(p); }
    bool rmdir(const char* p) { return vol->rmdir(p); }
    bool remove(const char* p) { return vol->remove(p); }
    bool rename(const char* a, const char* b) { return vol->rename(a, b); }
};
