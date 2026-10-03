#pragma once
#include <Arduino.h>

struct SystemDiagnostics {
    uint32_t uptimeSec;
    float    cpuLoadPct;
    uint32_t cpuFreqMhz;
    float    internalTempC;
    uint8_t  cpuCores;
    String   chipModel;
    uint8_t  chipRevision;
    String   resetReason;

    // RAM Statistics
    uint32_t heapTotalBytes;
    uint32_t freeHeapBytes;
    uint32_t minFreeHeapBytes;
    uint32_t heapMaxAllocBytes;
    uint8_t  heapFragmentationPct;
    uint32_t psramTotalBytes;
    uint32_t psramFreeBytes;

    // Flash & Sketch Memory
    uint32_t flashSizeBytes;
    uint32_t sketchSizeBytes;
    uint32_t sketchFreeBytes;

    // WiFi Statistics
    int8_t   wifiRssi;
    uint8_t  wifiSignalPct;
    String   wifiSsid;
    String   wifiBssid;
    int32_t  wifiChannel;
    String   wifiIp;
    String   wifiMac;
    String   wifiStatus;

    // RS232 / Emulator Telemetry
    uint32_t totalCommandsReceived;
    uint32_t totalBytesTransmitted;
};

class SystemStatsManager {
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
    static void init();
    static float getCpuUsage();
    static String getResetReasonString();
    static uint8_t calcSignalQuality(int rssi) {
        return (rssi <= -100) ? 0 : ((rssi >= -50) ? 100 : (uint8_t)(2 * (rssi + 100)));
    }
    static SystemDiagnostics getDiagnostics();
};
