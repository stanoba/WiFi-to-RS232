#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <chrono>
#include <cctype>
#include <cstring>

// Standard types
using byte = uint8_t;
using boolean = bool;

#define F(str) (str)

// Lightweight Arduino-compatible String mock
class String {
private:
    std::string _str;

public:
    String() : _str("") {}
    String(const char* s) : _str(s ? s : "") {}
    String(const std::string& s) : _str(s) {}
    String(char c) : _str(1, c) {}
    String(int val) : _str(std::to_string(val)) {}
    String(unsigned int val) : _str(std::to_string(val)) {}
    String(long val) : _str(std::to_string(val)) {}
    String(unsigned long val) : _str(std::to_string(val)) {}
    String(long long val) : _str(std::to_string(val)) {}
    String(unsigned long long val) : _str(std::to_string(val)) {}
    String(float val, int decimals = 2) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.*f", decimals, val);
        _str = buf;
    }
    String(double val, int decimals = 2) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.*f", decimals, val);
        _str = buf;
    }

    size_t length() const { return _str.length(); }
    const char* c_str() const { return _str.c_str(); }
    bool isEmpty() const { return _str.empty(); }

    char charAt(size_t index) const { return (index < _str.length()) ? _str[index] : 0; }
    char operator[](size_t index) const { return _str[index]; }
    char& operator[](size_t index) { return _str[index]; }

    int compareTo(const String& other) const {
        return _str.compare(other._str);
    }

    bool equalsIgnoreCase(const String& other) const {
        if (_str.length() != other._str.length()) return false;
        for (size_t i = 0; i < _str.length(); ++i) {
            if (std::tolower(_str[i]) != std::tolower(other._str[i])) return false;
        }
        return true;
    }

    bool startsWith(const String& prefix) const {
        if (prefix.length() > _str.length()) return false;
        return _str.rfind(prefix._str, 0) == 0;
    }

    bool endsWith(const String& suffix) const {
        if (suffix.length() > _str.length()) return false;
        return _str.compare(_str.length() - suffix.length(), suffix.length(), suffix._str) == 0;
    }

    int indexOf(char ch, size_t fromIndex = 0) const {
        auto pos = _str.find(ch, fromIndex);
        return (pos == std::string::npos) ? -1 : (int)pos;
    }

    int indexOf(const String& val, size_t fromIndex = 0) const {
        auto pos = _str.find(val._str, fromIndex);
        return (pos == std::string::npos) ? -1 : (int)pos;
    }

    String substring(size_t beginIndex, size_t endIndex = 0) const {
        if (beginIndex >= _str.length()) return String("");
        if (endIndex == 0 || endIndex > _str.length()) {
            return String(_str.substr(beginIndex));
        }
        return String(_str.substr(beginIndex, endIndex - beginIndex));
    }

    void toLowerCase() {
        for (char &c : _str) {
            c = (char)std::tolower(c);
        }
    }

    void toUpperCase() {
        for (char &c : _str) {
            c = (char)std::toupper(c);
        }
    }

    void trim() {
        size_t start = _str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            _str = "";
            return;
        }
        size_t end = _str.find_last_not_of(" \t\r\n");
        _str = _str.substr(start, end - start + 1);
    }

    void replace(const String& from, const String& to) {
        if (from._str.empty()) return;
        size_t start_pos = 0;
        while((start_pos = _str.find(from._str, start_pos)) != std::string::npos) {
            _str.replace(start_pos, from._str.length(), to._str);
            start_pos += to._str.length();
        }
    }

    int toInt() const {
        try {
            return std::stoi(_str);
        } catch (...) {
            return 0;
        }
    }

    float toFloat() const {
        try {
            return std::stof(_str);
        } catch (...) {
            return 0.0f;
        }
    }

    String& operator+=(const String& rhs) {
        _str += rhs._str;
        return *this;
    }

    String& operator+=(const char* rhs) {
        if (rhs) _str += rhs;
        return *this;
    }

    String& operator+=(char c) {
        _str += c;
        return *this;
    }

    bool operator==(const String& rhs) const { return _str == rhs._str; }
    bool operator==(const char* rhs) const { return rhs && _str == rhs; }
    bool operator!=(const String& rhs) const { return _str != rhs._str; }
    bool operator!=(const char* rhs) const { return !rhs || _str != rhs; }
    bool operator<(const String& rhs) const { return _str < rhs._str; }
    bool operator>(const String& rhs) const { return _str > rhs._str; }

    operator const char*() const { return _str.c_str(); }
};

inline String operator+(const String& lhs, const String& rhs) {
    String res = lhs;
    res += rhs;
    return res;
}

inline String operator+(const String& lhs, const char* rhs) {
    String res = lhs;
    res += rhs;
    return res;
}

inline String operator+(const char* lhs, const String& rhs) {
    String res(lhs);
    res += rhs;
    return res;
}

inline String operator+(const String& lhs, char c) {
    String res = lhs;
    res += c;
    return res;
}

inline String operator+(const String& lhs, int val) {
    return lhs + String(val);
}

inline String operator+(const String& lhs, unsigned int val) {
    return lhs + String(val);
}

inline String operator+(const String& lhs, long val) {
    return lhs + String(val);
}

inline String operator+(const String& lhs, unsigned long val) {
    return lhs + String(val);
}

// Time and random mocks
inline uint32_t millis() {
    static auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
}

inline uint32_t micros() {
    static auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return (uint32_t)std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
}

inline void delay(uint32_t ms) {
    // No-op or sleep for unit tests
}

inline uint32_t esp_random() {
    static uint32_t seed = 123456789;
    seed = (seed * 1103515245 + 12345) & 0x7fffffff;
    return seed;
}
