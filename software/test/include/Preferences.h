#pragma once
#include "Arduino.h"
#include <map>
#include <vector>
#include <cstring>
#include <algorithm>

class Preferences {
private:
    std::map<std::string, std::string> _strMap;
    std::map<std::string, uint32_t> _uintMap;
    std::map<std::string, float> _floatMap;
    std::map<std::string, bool> _boolMap;
    std::map<std::string, std::vector<uint8_t>> _bytesMap;

public:
    bool begin(const char* name, bool readOnly = false) { return true; }
    void end() {}
    void clear() {
        _strMap.clear();
        _uintMap.clear();
        _floatMap.clear();
        _boolMap.clear();
        _bytesMap.clear();
    }

    bool isKey(const char* key) {
        return _bytesMap.count(key) || _strMap.count(key) || _uintMap.count(key) || _floatMap.count(key) || _boolMap.count(key);
    }

    bool remove(const char* key) {
        _bytesMap.erase(key);
        _strMap.erase(key);
        _uintMap.erase(key);
        _floatMap.erase(key);
        _boolMap.erase(key);
        return true;
    }

    size_t putBytes(const char* key, const void* value, size_t len) {
        const uint8_t* p = (const uint8_t*)value;
        _bytesMap[key] = std::vector<uint8_t>(p, p + len);
        return len;
    }

    size_t getBytes(const char* key, void* buf, size_t maxLen) {
        auto it = _bytesMap.find(key);
        if (it == _bytesMap.end()) return 0;
        size_t copyLen = (std::min)(maxLen, it->second.size());
        memcpy(buf, it->second.data(), copyLen);
        return copyLen;
    }

    String getString(const char* key, const String& defaultValue = "") {
        auto it = _strMap.find(key);
        return (it != _strMap.end()) ? String(it->second) : defaultValue;
    }

    size_t putString(const char* key, const String& value) {
        _strMap[key] = value.c_str();
        return value.length();
    }

    uint32_t getUInt(const char* key, uint32_t defaultValue = 0) {
        auto it = _uintMap.find(key);
        return (it != _uintMap.end()) ? it->second : defaultValue;
    }

    size_t putUInt(const char* key, uint32_t value) {
        _uintMap[key] = value;
        return 4;
    }

    uint8_t getUChar(const char* key, uint8_t defaultValue = 0) {
        return (uint8_t)getUInt(key, defaultValue);
    }

    size_t putUChar(const char* key, uint8_t value) {
        return putUInt(key, value);
    }

    float getFloat(const char* key, float defaultValue = 0.0f) {
        auto it = _floatMap.find(key);
        return (it != _floatMap.end()) ? it->second : defaultValue;
    }

    size_t putFloat(const char* key, float value) {
        _floatMap[key] = value;
        return 4;
    }

    bool getBool(const char* key, bool defaultValue = false) {
        auto it = _boolMap.find(key);
        return (it != _boolMap.end()) ? it->second : defaultValue;
    }

    size_t putBool(const char* key, bool value) {
        _boolMap[key] = value;
        return 1;
    }
};

