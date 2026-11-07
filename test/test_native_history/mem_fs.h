#pragma once
// Implementation en memoire de mhfs::Fs au-dessus de MemVolume : le code refondu
// (src/history/) travaille sur la MEME structure que le shim Arduino du code de reference.
#include <memory>
#include <string>
#include <vector>
#include "history/fs_port.h"
#include "mem_volume.h"

class MemHandle : public mhfs::FileHandle {
public:
    explicit MemHandle(std::shared_ptr<MemFileState> s) : _s(std::move(s)) {}
    size_t size() override { return _s ? _s->size() : 0; }
    bool seek(uint32_t pos) override { return _s && _s->seek(pos); }
    int read(uint8_t* b, size_t n) override { return _s ? _s->read(b, n) : -1; }
    size_t write(const uint8_t* b, size_t n) override { return _s ? _s->write(b, n) : 0; }
    void flush() override {}
    void close() override { _s.reset(); }
private:
    std::shared_ptr<MemFileState> _s;
};

class MemFs : public mhfs::Fs {
public:
    explicit MemFs(MemVolume& v) : _v(v) {}
    bool mounted = true;   // false : la carte est "retiree" (ensureMounted echoue)

    bool ensureMounted() override { return mounted; }
    bool exists(const char* p) override { return _v.exists(p); }
    bool mkdir(const char* p) override { return _v.mkdir(p); }
    bool rmdir(const char* p) override { return _v.rmdir(p); }
    bool remove(const char* p) override { return _v.remove(p); }
    bool rename(const char* a, const char* b) override { return _v.rename(a, b); }
    mhfs::FilePtr open(const char* p, mhfs::Mode m) override {
        MemMode mm = MemMode::Read;
        switch (m) {
            case mhfs::Mode::Read: mm = MemMode::Read; break;
            case mhfs::Mode::Write: mm = MemMode::Write; break;
            case mhfs::Mode::Append: mm = MemMode::Append; break;
            case mhfs::Mode::ReadWrite: mm = MemMode::ReadWrite; break;
        }
        auto st = MemFileState::open(_v, p, mm);
        if (!st) return nullptr;
        return mhfs::FilePtr(new MemHandle(st));
    }
    mhfs::DirResult listDir(const char* path, mhfs::Want want, size_t maxEntries,
                            std::vector<mhfs::DirEntry>& out) override {
        if (_v.files.count(path)) return mhfs::DirResult::NotDir;
        if (!_v.dirExists(path)) return mhfs::DirResult::Missing;
        for (const auto& k : _v.children(path)) {
            if (out.size() >= maxEntries) break;
            if (want == mhfs::Want::All || (want == mhfs::Want::Dirs) == k.second) {
                mhfs::DirEntry e;
                e.path = k.first;
                e.name = k.first.substr(k.first.find_last_of('/') + 1);
                e.isDir = k.second;
                out.push_back(e);
            }
        }
        return mhfs::DirResult::Ok;
    }
private:
    MemVolume& _v;
};
