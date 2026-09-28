#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <esp_system.h>

struct SystemStats {
    uint32_t uptimeSec;
    float    cpuLoadPct;
    uint32_t cpuFreqMHz;
    float    cpuTempC;
    uint8_t  cpuCores;
    String   chipModel;
    uint8_t  chipRevision;
    String   resetReason;

    // RAM Statistics
    uint32_t heapTotal;
    uint32_t heapFree;
    uint32_t heapMinFree;
    uint32_t heapMaxAlloc;
    uint8_t  heapFragPct;
    uint32_t psramTotal;
    uint32_t psramFree;

    // Flash Memory
    uint32_t flashSize;
    uint32_t sketchSize;
    uint32_t sketchFree;

    // WiFi Statistics
    int8_t   wifiRssi;
    uint8_t  wifiSignalPct;
    String   wifiSsid;
    String   wifiBssid;
    int32_t  wifiChannel;
    String   wifiIp;
    String   wifiMac;
    String   wifiStatus;
};

class SystemMonitor {
private:
    static volatile uint32_t idleTicksCore0;
#if !CONFIG_FREERTOS_UNICORE
    static volatile uint32_t idleTicksCore1;
#endif
    static uint32_t lastCheckMillis;
    static float    lastCpuUsage;

    static bool idleHook0(void);
#if !CONFIG_FREERTOS_UNICORE
    static bool idleHook1(void);
#endif

public:
    static void begin();
    static float getCpuUsage();
    static String getResetReasonStr();
    static SystemStats getSnapshot();
};
