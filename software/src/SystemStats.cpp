#include "SystemStats.h"
#include <esp_freertos_hooks.h>

volatile uint32_t SystemMonitor::idleTicksCore0 = 0;
#if !CONFIG_FREERTOS_UNICORE
volatile uint32_t SystemMonitor::idleTicksCore1 = 0;
#endif
uint32_t SystemMonitor::lastCheckMillis = 0;
float    SystemMonitor::lastCpuUsage = 0.0f;

bool SystemMonitor::idleHook0(void) {
    idleTicksCore0++;
    return true;
}

#if !CONFIG_FREERTOS_UNICORE
bool SystemMonitor::idleHook1(void) {
    idleTicksCore1++;
    return true;
}
#endif

void SystemMonitor::begin() {
    esp_register_freertos_idle_hook_for_cpu(idleHook0, 0);
#if !CONFIG_FREERTOS_UNICORE
    esp_register_freertos_idle_hook_for_cpu(idleHook1, 1);
#endif
    lastCheckMillis = millis();
}

float SystemMonitor::getCpuUsage() {
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

String SystemMonitor::getResetReasonStr() {
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

SystemStats SystemMonitor::getSnapshot() {
    SystemStats s;
    s.uptimeSec = millis() / 1000;
    s.cpuLoadPct = getCpuUsage();
    s.cpuFreqMHz = ESP.getCpuFreqMHz();
    s.cpuTempC = temperatureRead();
    s.cpuCores = ESP.getChipCores();
    s.chipModel = String(ESP.getChipModel());
    s.chipRevision = ESP.getChipRevision();
    s.resetReasonCode = (uint8_t)esp_reset_reason();
    s.resetReason = getResetReasonStr();

    s.heapTotal = ESP.getHeapSize();
    s.heapFree = ESP.getFreeHeap();
    s.heapMinFree = ESP.getMinFreeHeap();
    s.heapMaxAlloc = ESP.getMaxAllocHeap();
    s.heapFragPct = (s.heapFree > 0 && s.heapMaxAlloc <= s.heapFree)
        ? (uint8_t)(100 - (uint32_t)((uint64_t)s.heapMaxAlloc * 100 / s.heapFree))
        : 0;

    s.psramTotal = ESP.getPsramSize();
    s.psramFree = ESP.getFreePsram();

    s.flashSize = ESP.getFlashChipSize();
    s.sketchSize = ESP.getSketchSize();
    s.sketchFree = ESP.getFreeSketchSpace();

    if (WiFi.status() == WL_CONNECTED) {
        s.wifiRssi = (int8_t)WiFi.RSSI();
        s.wifiSignalPct = (s.wifiRssi <= -100) ? 0 : ((s.wifiRssi >= -50) ? 100 : (uint8_t)(2 * (s.wifiRssi + 100)));
        s.wifiSsid = WiFi.SSID();
        s.wifiBssid = WiFi.BSSIDstr();
        s.wifiChannel = WiFi.channel();
        s.wifiIp = WiFi.localIP().toString();
        s.wifiMac = WiFi.macAddress();
        s.wifiStatus = "connected";
    } else {
        s.wifiRssi = 0;
        s.wifiSignalPct = 0;
        s.wifiSsid = "";
        s.wifiBssid = "";
        s.wifiChannel = 0;
        s.wifiIp = WiFi.softAPIP().toString();
        s.wifiMac = WiFi.softAPmacAddress();
        s.wifiStatus = "disconnected";
    }

    return s;
}
