#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <time.h>
#include <vector>
#include <algorithm>

struct __attribute__((packed)) HistorySample {
    uint32_t timestamp;  // Epoch seconds (or uptime seconds if NTP not synced)
    int16_t  currCentiA; // Current in cA (0.01A, e.g. -1425 for -14.25A)
    uint8_t  soc;        // 0-100%
};

class BatteryHistoryManager {
public:
    static const size_t MAX_SAMPLES = 1440; // 24 hours at 1-minute resolution

private:
    HistorySample *samples = nullptr;
    size_t head = 0;
    size_t count = 0;
    uint32_t lastSampleTime = 0;
    bool inPsram = false;

public:
    BatteryHistoryManager() = default;

    void begin() {
        if (samples != nullptr) return;
#if defined(BOARD_HAS_PSRAM) || defined(CONFIG_SPIRAM_SUPPORT)
        if (psramFound()) {
            samples = (HistorySample*)ps_malloc(sizeof(HistorySample) * MAX_SAMPLES);
            if (samples) inPsram = true;
        }
#endif
        if (!samples) {
            samples = (HistorySample*)malloc(sizeof(HistorySample) * MAX_SAMPLES);
        }
        if (samples) {
            memset(samples, 0, sizeof(HistorySample) * MAX_SAMPLES);
        }
    }

    bool isPsram() const { return inPsram; }

    void addSample(uint32_t epochOrSec, float currA, uint8_t soc) {
        if (!samples) begin();
        if (!samples) return;

        // Enforce minimum 45s cadence to avoid duplicate points within the same minute
        if (lastSampleTime > 0 && (epochOrSec > lastSampleTime) && (epochOrSec - lastSampleTime < 45)) {
            return;
        }
        lastSampleTime = epochOrSec;

        HistorySample &s = samples[head];
        s.timestamp = epochOrSec;
        s.currCentiA = (int16_t)round(currA * 100.0f);
        s.soc = (soc > 100) ? 100 : soc;

        head = (head + 1) % MAX_SAMPLES;
        if (count < MAX_SAMPLES) {
            count++;
        }
    }

    size_t getCount() const { return count; }

    void streamJson(WebServer &server, uint32_t rangeSeconds = 86400) const {
        server.setContentLength(CONTENT_LENGTH_UNKNOWN);
        server.send(200, "application/json; charset=utf-8", "");

        if (!samples || count == 0) {
            server.sendContent("{\"range\":" + String(rangeSeconds) + ",\"count\":0,\"samples\":[]}");
            return;
        }

        uint32_t nowTime = time(nullptr);
        if (nowTime < 1577836800) nowTime = millis() / 1000;
        uint32_t minTime = (nowTime > rangeSeconds) ? (nowTime - rangeSeconds) : 0;

        size_t validCount = 0;
        for (size_t i = 0; i < count; ++i) {
            size_t idx = (head + MAX_SAMPLES - count + i) % MAX_SAMPLES;
            if (samples[idx].timestamp >= minTime) {
                validCount++;
            }
        }

        String chunk;
        chunk.reserve(512);
        chunk = "{\"range\":" + String(rangeSeconds) + ",\"count\":" + String(validCount) + ",\"samples\":[";
        server.sendContent(chunk);
        chunk = "";

        bool first = true;
        for (size_t i = 0; i < count; ++i) {
            size_t idx = (head + MAX_SAMPLES - count + i) % MAX_SAMPLES;
            const HistorySample &s = samples[idx];
            if (s.timestamp < minTime) continue;

            if (!first) chunk += ",";
            first = false;

            chunk += "{\"t\":" + String(s.timestamp) + ",";
            chunk += "\"c\":" + String(s.currCentiA / 100.0f, 2) + ",";
            chunk += "\"s\":" + String(s.soc) + "}";

            if (chunk.length() > 400) {
                server.sendContent(chunk);
                chunk = "";
            }
        }

        chunk += "]}";
        server.sendContent(chunk);
    }
};
