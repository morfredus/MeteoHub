#pragma once
// Shim minimal d'Arduino.h pour compiler, sur l'hote, le code de REFERENCE du hub
// (history_manager de la 1.62.0). Ne fournit que ce que ce code utilise.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>

inline unsigned long millis() { return 0; }
inline void delay(uint32_t) {}

// getLocalTime : l'horloge de l'hote est toujours "synchronisee".
inline bool getLocalTime(struct tm* info, uint32_t = 5000) {
    const time_t t = time(nullptr);
    return localtime_r(&t, info) != nullptr;
}

// String Arduino : seules les methodes utilisees par history_manager.
class String {
public:
    String() {}
    String(const char* s) : s_(s ? s : "") {}
    String(const std::string& s) : s_(s) {}
    size_t length() const { return s_.size(); }
    int indexOf(char c, size_t from = 0) const {
        const size_t k = s_.find(c, from);
        return k == std::string::npos ? -1 : (int)k;
    }
    String substring(size_t a, size_t b) const { return String(s_.substr(a, b - a)); }
    String substring(size_t a) const { return String(s_.substr(a)); }
    long toInt() const { return atol(s_.c_str()); }
    float toFloat() const { return (float)atof(s_.c_str()); }
    const char* c_str() const { return s_.c_str(); }
    void trim() {
        size_t a = 0, b = s_.size();
        while (a < b && isspace((unsigned char)s_[a])) a++;
        while (b > a && isspace((unsigned char)s_[b - 1])) b--;
        s_ = s_.substr(a, b - a);
    }
private:
    std::string s_;
};
