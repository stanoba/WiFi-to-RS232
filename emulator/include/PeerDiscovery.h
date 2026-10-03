#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

struct DiscoveredPeer {
    String hostname;
    IPAddress ip;
    uint16_t port = 80;
    String version;
    String model;
    uint8_t moduleCount = 0;
    uint32_t lastSeenSec = 0;
};

class PeerDiscoveryManager {
private:
    std::vector<DiscoveredPeer> peers;
    SemaphoreHandle_t mutex;
    TaskHandle_t taskHandle;
    String selfHostname;
    bool running;

    static void taskTrampoline(void *param) {
        PeerDiscoveryManager *mgr = static_cast<PeerDiscoveryManager*>(param);
        mgr->taskLoop();
    }

    void taskLoop() {
        vTaskDelay(pdMS_TO_TICKS(8000));

        while (running) {
            if (WiFi.status() == WL_CONNECTED) {
                scanNow();
            }
            vTaskDelay(pdMS_TO_TICKS(60000)); // Scan every 60s
        }
        vTaskDelete(NULL);
    }

public:
    PeerDiscoveryManager() : mutex(NULL), taskHandle(NULL), running(false) {
        mutex = xSemaphoreCreateMutex();
    }

    ~PeerDiscoveryManager() {
        running = false;
        if (mutex) {
            vSemaphoreDelete(mutex);
        }
    }

    void begin(const String &hostname) {
        selfHostname = hostname;
        selfHostname.toLowerCase();

        if (!running) {
            running = true;
            xTaskCreate(
                taskTrampoline,
                "mdns_peer_scan",
                4096,
                this,
                1,
                &taskHandle
            );
        }
    }

    void setSelfHostname(const String &hostname) {
        selfHostname = hostname;
        selfHostname.toLowerCase();
    }

    void scanNow() {
        if (WiFi.status() != WL_CONNECTED) return;

        int count = MDNS.queryService("pylon-smart", "tcp");
        if (count <= 0) {
            if (mutex && xSemaphoreTake(mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
                peers.clear();
                xSemaphoreGive(mutex);
            }
            return;
        }

        IPAddress myIp = WiFi.localIP();
        uint32_t nowSec = millis() / 1000;
        std::vector<DiscoveredPeer> found;

        for (int i = 0; i < count; ++i) {
            IPAddress pIp = MDNS.IP(i);
            String pHost = MDNS.hostname(i);
            uint16_t pPort = MDNS.port(i);
            String pVer = "";
            if (MDNS.hasTxt(i, "ver")) {
                pVer = MDNS.txt(i, "ver");
            }
            String pModel = "";
            if (MDNS.hasTxt(i, "model")) {
                pModel = MDNS.txt(i, "model");
            }
            uint8_t pModules = 0;
            if (MDNS.hasTxt(i, "modules")) {
                pModules = (uint8_t)MDNS.txt(i, "modules").toInt();
            } else if (MDNS.hasTxt(i, "mods")) {
                pModules = (uint8_t)MDNS.txt(i, "mods").toInt();
            }

            if (pIp == myIp) continue;
            String pHostLower = pHost;
            pHostLower.toLowerCase();
            if (pHostLower.length() > 0 && pHostLower == selfHostname) continue;

            DiscoveredPeer p;
            p.hostname = pHost;
            p.ip = pIp;
            p.port = (pPort > 0) ? pPort : 80;
            p.version = pVer;
            p.model = pModel;
            p.moduleCount = pModules;
            p.lastSeenSec = nowSec;

            found.push_back(p);
        }

        if (mutex && xSemaphoreTake(mutex, pdMS_TO_TICKS(500)) == pdTRUE) {
            peers = found;
            xSemaphoreGive(mutex);
        }
    }

    std::vector<DiscoveredPeer> getPeers() {
        std::vector<DiscoveredPeer> result;
        if (mutex && xSemaphoreTake(mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
            result = peers;
            xSemaphoreGive(mutex);
        }
        return result;
    }

    size_t getPeerCount() {
        size_t count = 0;
        if (mutex && xSemaphoreTake(mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
            count = peers.size();
            xSemaphoreGive(mutex);
        }
        return count;
    }
};

extern PeerDiscoveryManager g_peerDiscovery;
