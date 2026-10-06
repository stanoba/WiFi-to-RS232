#include "SystemStats.h"
#include <WiFi.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <esp_freertos_hooks.h>

volatile uint32_t SystemStatsManager::idleTicksCore0 = 0;
#if !CONFIG_FREERTOS_UNICORE
volatile uint32_t SystemStatsManager::idleTicksCore1 = 0;
#endif
uint32_t SystemStatsManager::lastCheckMillis = 0;
float    SystemStatsManager::lastCpuUsage = 0.0f;

bool SystemStatsManager::idleHook0(void) {
    idleTicksCore0++;
    return true;
}

#if !CONFIG_FREERTOS_UNICORE
bool SystemStatsManager::idleHook1(void) {
    idleTicksCore1++;
    return true;
}
#endif

void SystemStatsManager::init() {
    esp_register_freertos_idle_hook_for_cpu(idleHook0, 0);
#if !CONFIG_FREERTOS_UNICORE
    esp_register_freertos_idle_hook_for_cpu(idleHook1, 1);
#endif
    lastCheckMillis = millis();
}

float SystemStatsManager::getCpuUsage() {
    uint32_t now = millis();
    uint32_t elapsed = now - lastCheckMillis;
    if (elapsed >= 500) {
        lastCheckMillis = now;
        uint32_t t0 = idleTicksCore0;
        idleTicksCore0 = 0;
        float idleRatio0 = (float)t0 / (float)elapsed;
        if (idleRatio0 > 1.0f) idleRatio0 = 1.0f;
        float load0 = (1.0f - idleRatio0) * 100.0f;

#if !CONFIG_FREERTOS_UNICORE
        uint32_t t1 = idleTicksCore1;
        idleTicksCore1 = 0;
        float idleRatio1 = (float)t1 / (float)elapsed;
        if (idleRatio1 > 1.0f) idleRatio1 = 1.0f;
        float load1 = (1.0f - idleRatio1) * 100.0f;
        lastCpuUsage = (load0 + load1) / 2.0f;
#else
        lastCpuUsage = load0;
#endif
        if (lastCpuUsage < 0.0f) lastCpuUsage = 0.0f;
        if (lastCpuUsage > 100.0f) lastCpuUsage = 100.0f;
    }
    return lastCpuUsage;
}

String SystemStatsManager::getResetReasonString() {
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON:   return "Power-on Reset";
        case ESP_RST_EXT:       return "External Pin Reset";
        case ESP_RST_SW:        return "Software Reset";
        case ESP_RST_PANIC:     return "Exception / Panic Reset";
        case ESP_RST_INT_WDT:   return "Interrupt Watchdog Reset";
        case ESP_RST_TASK_WDT:  return "Task Watchdog Reset";
        case ESP_RST_WDT:       return "Other Watchdog Reset";
        case ESP_RST_DEEPSLEEP: return "Deep Sleep Wakeup";
        case ESP_RST_BROWNOUT:  return "Brownout Reset (Voltage Sag)";
        case ESP_RST_SDIO:      return "SDIO Reset";
        default:                return "Unknown";
    }
}

SystemDiagnostics SystemStatsManager::getDiagnostics() {
    SystemDiagnostics diag;
    diag.uptimeSec = millis() / 1000;
    diag.cpuLoadPct = getCpuUsage();
    diag.cpuFreqMhz = ESP.getCpuFreqMHz();
    diag.internalTempC = temperatureRead();
    diag.cpuCores = ESP.getChipCores();
    diag.chipModel = String(ESP.getChipModel());
    diag.chipRevision = ESP.getChipRevision();
    diag.resetReasonCode = (uint8_t)esp_reset_reason();
    diag.resetReason = getResetReasonString();

    diag.heapTotalBytes = ESP.getHeapSize();
    diag.freeHeapBytes = ESP.getFreeHeap();
    diag.minFreeHeapBytes = ESP.getMinFreeHeap();
    diag.heapMaxAllocBytes = ESP.getMaxAllocHeap();
    diag.heapFragmentationPct = (diag.freeHeapBytes > 0 && diag.heapMaxAllocBytes <= diag.freeHeapBytes)
        ? (uint8_t)(100 - (uint32_t)((uint64_t)diag.heapMaxAllocBytes * 100 / diag.freeHeapBytes))
        : 0;

    diag.psramTotalBytes = ESP.getPsramSize();
    diag.psramFreeBytes = ESP.getFreePsram();

    diag.flashSizeBytes = ESP.getFlashChipSize();
    diag.sketchSizeBytes = ESP.getSketchSize();
    diag.sketchFreeBytes = ESP.getFreeSketchSpace();

    if (WiFi.status() == WL_CONNECTED) {
        diag.wifiRssi = (int8_t)WiFi.RSSI();
        diag.wifiSignalPct = calcSignalQuality(diag.wifiRssi);
        diag.wifiSsid = WiFi.SSID();
        diag.wifiBssid = WiFi.BSSIDstr();
        diag.wifiChannel = WiFi.channel();
        diag.wifiIp = WiFi.localIP().toString();
        diag.wifiMac = WiFi.macAddress();
        diag.wifiStatus = "connected";
    } else {
        diag.wifiRssi = 0;
        diag.wifiSignalPct = 0;
        diag.wifiSsid = "";
        diag.wifiBssid = "";
        diag.wifiChannel = 0;
        diag.wifiIp = WiFi.softAPIP().toString();
        diag.wifiMac = WiFi.softAPmacAddress();
        diag.wifiStatus = "disconnected";
    }

    diag.totalCommandsReceived = 0;
    diag.totalBytesTransmitted = 0;

    return diag;
}
