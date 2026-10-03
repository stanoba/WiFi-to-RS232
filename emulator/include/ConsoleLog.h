#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <time.h>
#include "Config.h"

enum LogDirection {
    LOG_DIR_INFO,
    LOG_DIR_RX,
    LOG_DIR_TX
};

class ConsoleLogManager {
private:
    char *buffer = nullptr;
    size_t capacity = 0;
    size_t length = 0;
    bool inPsram = false;

    String formatTime() {
        time_t now = time(nullptr);
        // Valid epoch check (after year 2020: 1577836800)
        if (now > 1577836800) {
            struct tm timeinfo;
            localtime_r(&now, &timeinfo);
            char buf[40];
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
            return String(buf);
        }
        unsigned long s = millis() / 1000;
        unsigned long m = s / 60;
        unsigned long h = m / 60;
        char buf[16];
        snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", h % 24, m % 60, s % 60);
        return String(buf);
    }

public:
    ConsoleLogManager() {
        capacity = 4096;
        buffer = (char*)malloc(capacity);
        if (buffer) {
            buffer[0] = '\0';
        }
    }

    void begin() {
        size_t targetCap = 16384;
        bool usePsram = false;
#if defined(BOARD_HAS_PSRAM) || defined(CONFIG_SPIRAM_SUPPORT)
        if (psramFound()) {
            targetCap = 65536;
            usePsram = true;
        }
#endif
        char *newBuf = nullptr;
        if (usePsram) {
            newBuf = (char*)ps_malloc(targetCap);
        }
        if (!newBuf) {
            targetCap = 16384;
            newBuf = (char*)malloc(targetCap);
            usePsram = false;
        }
        if (newBuf) {
            if (buffer && length > 0) {
                size_t copyLen = (length < targetCap - 1) ? length : (targetCap - 1);
                memcpy(newBuf, buffer, copyLen);
                newBuf[copyLen] = '\0';
                length = copyLen;
            } else {
                newBuf[0] = '\0';
                length = 0;
            }
            if (buffer) free(buffer);
            buffer = newBuf;
            capacity = targetCap;
            inPsram = usePsram;
        }
    }

    bool isPsram() const { return inPsram; }
    size_t getCapacity() const { return capacity; }
    size_t getLength() const { return length; }

    void append(const String &text) {
        if (!buffer || capacity == 0) return;

        size_t textLen = text.length();
        if (textLen == 0) return;

        if (length + textLen >= capacity) {
            size_t excess = (length + textLen) - (capacity - 1024);
            char *nl = (char*)memchr(buffer + excess, '\n', length - excess);
            size_t dropLen = nl ? (nl - buffer + 1) : excess;
            if (dropLen > length) dropLen = length;
            size_t remaining = length - dropLen;
            memmove(buffer, buffer + dropLen, remaining);
            length = remaining;
            buffer[length] = '\0';
        }

        size_t toCopy = textLen;
        if (length + toCopy >= capacity) {
            toCopy = capacity - length - 1;
        }
        memcpy(buffer + length, text.c_str(), toCopy);
        length += toCopy;
        buffer[length] = '\0';
    }

    void logRx(const String &cmd) {
        String msg = "[" + formatTime() + "] RX >> " + cmd + "\n";
        append(msg);
    }

    void logTx(const String &data) {
        String clean = data;
        clean.replace("\r", "");
        if (!clean.endsWith("\n")) clean += "\n";
        append(clean);
    }

    void logInfo(const String &msg) {
        append("[" + formatTime() + "] [SYSTEM] " + msg + "\n");
    }

    void clear() {
        length = 0;
        if (buffer) buffer[0] = '\0';
        append("[" + formatTime() + "] [SYSTEM] Log cleared.\n");
    }

    void streamLog(WebServer &server) const {
        server.setContentLength(CONTENT_LENGTH_UNKNOWN);
        server.send(200, "text/plain; charset=utf-8", "");
        if (!buffer || length == 0) {
            server.sendContent("");
            return;
        }
        const size_t CHUNK_SIZE = 1024;
        size_t offset = 0;
        while (offset < length) {
            size_t chunkLen = (length - offset < CHUNK_SIZE) ? (length - offset) : CHUNK_SIZE;
            char oldChar = buffer[offset + chunkLen];
            const_cast<char*>(buffer)[offset + chunkLen] = '\0';
            server.sendContent(buffer + offset);
            const_cast<char*>(buffer)[offset + chunkLen] = oldChar;
            offset += chunkLen;
            yield();
        }
        server.sendContent("");
    }

    String getLog() const {
        if (!buffer || length == 0) return "";
        return String(buffer);
    }

    // Compatibility overload
    void add(LogDirection dir, const String &text) {
        if (dir == LOG_DIR_RX) logRx(text);
        else if (dir == LOG_DIR_TX) logTx(text);
        else logInfo(text);
    }
};

extern ConsoleLogManager g_consoleLog;
