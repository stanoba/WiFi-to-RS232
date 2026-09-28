#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <esp_sntp.h>

#include "Config.h"
#include "Timezones.h"
#include "BatteryData.h"
#include "ConsoleLog.h"
#include "PylonSerial.h"
#include "MqttClientManager.h"
#include "WebPortal.h"
#include "BatteryHistory.h"
#include "SystemStats.h"
#include "PeerDiscovery.h"

// =============================================================================
// NTP Time Synchronization State & Callbacks
// =============================================================================
time_t lastNtpSyncTimestamp = 0;
volatile bool ntpJustSynced = false;

void timeSyncNotificationCallback(struct timeval *tv) {
    lastNtpSyncTimestamp = time(nullptr);
    ntpJustSynced = true;
}

// =============================================================================
// Global State & Instances
// =============================================================================
ConsoleLogger      consoleLog;
BatteryStack       batteryStack;
PylonSerialManager pylonSerial;
WebServer          webServer(HTTP_PORT);
DNSServer          dnsServer;
Preferences        preferences;
MqttClientManager  mqttManager(preferences);
BatteryHistoryManager batteryHistory;
PeerDiscoveryManager  peerDiscovery;

bool isApMode              = false;
bool triggerManualPoll     = false;
bool isPollingPaused       = false;
bool apModeInitialPollDone = false;
bool staWifiWasConnected   = false;

uint32_t fastPollIntervalMs = FAST_POLL_INTERVAL_MS;
uint32_t slowPollIntervalMs = SLOW_POLL_INTERVAL_MS;

uint32_t lastFastPollMillis   = 0;
uint32_t lastSlowPollMillis   = 0;
uint32_t lastLedBlinkMillis   = 0;
bool ledWifiState             = false;

WebPortal portal(webServer, batteryStack, preferences, isApMode, triggerManualPoll, isPollingPaused, pylonSerial, mqttManager, batteryHistory, peerDiscovery);

// =============================================================================
// Helper Functions
// =============================================================================
void updateMdnsModelTxt() {
    static String lastPublishedModel = "";
    if (WiFi.status() == WL_CONNECTED && batteryStack.modelName[0] != '\0' && strcmp(batteryStack.modelName, "Unknown") != 0) {
        if (lastPublishedModel != batteryStack.modelName) {
            lastPublishedModel = batteryStack.modelName;
            MDNS.addServiceTxt("pylon-smart", "tcp", "model", (const char*)batteryStack.modelName);
        }
    }
}

void recordHistorySample() {
    updateMdnsModelTxt();
    if (!batteryStack.scrapeSuccess || batteryStack.moduleCount == 0) return;
    float stackCurr = 0.0f;
    float avgSoc = 0.0f;
    uint8_t validPwr = 0;

    for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
        if (batteryStack.modules[m].present && batteryStack.modules[m].power.valid) {
            stackCurr += (batteryStack.modules[m].power.currMa / 1000.0f);
            avgSoc += batteryStack.modules[m].power.socPercent;
            validPwr++;
        }
    }
    if (validPwr == 0) return;
    avgSoc /= validPwr;

    uint32_t nowTime = time(nullptr);
    if (nowTime < 1577836800) {
        nowTime = millis() / 1000;
    }
    batteryHistory.addSample(nowTime, stackCurr, (uint8_t)round(avgSoc));
}
void loadIntervals() {
    uint32_t fastSec = preferences.getUInt(NVS_KEY_FAST_POLL_SEC, FAST_POLL_INTERVAL_MS / 1000);
    uint32_t slowSec = preferences.getUInt(NVS_KEY_SLOW_POLL_SEC, SLOW_POLL_INTERVAL_MS / 1000);
    if (fastSec < 10) fastSec = 10;
    if (slowSec < 20) slowSec = 20;
    fastPollIntervalMs = fastSec * 1000UL;
    slowPollIntervalMs = slowSec * 1000UL;
}

void yieldSystemTasks() {
    ArduinoOTA.handle();
    webServer.handleClient();
    mqttManager.loop();
    yield();
}

void setLedWifi(bool on) {
    digitalWrite(PIN_LED_WIFI, on ? LED_ACTIVE_LEVEL : !LED_ACTIVE_LEVEL);
}

void startApMode() {
    isApMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.setSleep(false);
    
    // Generate unique AP SSID based on MAC address suffix
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    String apSsid = String(AP_SSID_PREFIX) + "-" + mac.substring(mac.length() - 4);

    IPAddress apIP(AP_IP_ADDRESS);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

    if (strlen(AP_DEFAULT_PASSWORD) > 0) {
        WiFi.softAP(apSsid.c_str(), AP_DEFAULT_PASSWORD);
    } else {
        WiFi.softAP(apSsid.c_str()); // Open network without password
    }

    dnsServer.start(DNS_PORT, "*", apIP);
    consoleLog.logInfo("Started Standalone AP mode: " + apSsid + " (Open network)");
    consoleLog.logInfo("  -> AP IP Address: " + apIP.toString());
    consoleLog.logInfo("  -> Web Portal URL: http://" + apIP.toString() + "/");
}

void setupOta(const String &hostname) {
    ArduinoOTA.setHostname(hostname.c_str());
    ArduinoOTA.setPort(OTA_PORT);

    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        consoleLog.logInfo("ArduinoOTA start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        consoleLog.logInfo("ArduinoOTA update complete");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        // Optional progress
    });
    ArduinoOTA.onError([](ota_error_t error) {
        consoleLog.logError("ArduinoOTA error [" + String(error) + "]");
    });

    ArduinoOTA.begin();
    consoleLog.logInfo("ArduinoOTA initialized (" + hostname + ") on port " + String(OTA_PORT));
}

void triggerNtpSync() {
    bool ntpEn = preferences.getBool(NVS_KEY_NTP_ENABLED, true);
    if (!ntpEn) {
        if (sntp_enabled()) {
            sntp_stop();
        }
        consoleLog.logInfo("NTP is disabled in configuration.");
        return;
    }
    String tzPosix = preferences.getString(NVS_KEY_TZ_POSIX, DEFAULT_TZ_POSIX);
    String ntpSrv = preferences.getString(NVS_KEY_NTP_SERVER, NTP_DEFAULT_SERVER);

    if (sntp_enabled()) {
        sntp_stop();
    }
    sntp_set_time_sync_notification_cb(timeSyncNotificationCallback);
    configTzTime(tzPosix.c_str(), ntpSrv.c_str(), "time.google.com", "time.cloudflare.com");
    consoleLog.logInfo("NTP client started: " + ntpSrv + ", time.google.com, time.cloudflare.com (TZ: " + tzPosix + ")");
}

bool connectToSta(const String &ssid, const String &pass, uint32_t timeoutMs = 20000) {
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);

    String hostname = getDeviceHostname(preferences);
    WiFi.setHostname(hostname.c_str());

    bool staticEn = preferences.getBool(NVS_KEY_STATIC_IP_EN, false);
    if (staticEn) {
        String ipStr = preferences.getString(NVS_KEY_STATIC_IP, "");
        String maskStr = preferences.getString(NVS_KEY_STATIC_MASK, "255.255.255.0");
        String gwStr = preferences.getString(NVS_KEY_STATIC_GW, "");
        String dnsStr = preferences.getString(NVS_KEY_STATIC_DNS, "");
        IPAddress ip, mask, gw, dns;
        if (ip.fromString(ipStr) && mask.fromString(maskStr) && gw.fromString(gwStr)) {
            if (!dns.fromString(dnsStr)) dns = gw;
            WiFi.config(ip, gw, mask, dns, IPAddress(8, 8, 8, 8));
            consoleLog.logInfo("Static IP configured: " + ipStr + " (GW: " + gwStr + ")");
        }
    } else {
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    }

    WiFi.begin(ssid.c_str(), pass.c_str());

    consoleLog.logInfo("Connecting to WiFi SSID: " + ssid);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start < timeoutMs)) {
        setLedWifi(true);
        delay(100);
        setLedWifi(false);
        delay(100);
        DEBUG_PRINT(".");
    }
    DEBUG_PRINTLN_EMPTY();

    if (WiFi.status() == WL_CONNECTED) {
        staWifiWasConnected = true;
        setLedWifi(true);
        MDNS.begin(hostname.c_str());
        MDNS.addService("http", "tcp", 80);
        MDNS.addService("pylon-smart", "tcp", 80);
        MDNS.addServiceTxt("pylon-smart", "tcp", "ver", FIRMWARE_VERSION);
        if (batteryStack.modelName[0] != '\0' && strcmp(batteryStack.modelName, "Unknown") != 0) {
            MDNS.addServiceTxt("pylon-smart", "tcp", "model", (const char*)batteryStack.modelName);
        }
        setupOta(hostname);

        triggerNtpSync();
        peerDiscovery.begin(hostname);

        consoleLog.logInfo("WiFi connected successfully!");
        consoleLog.logInfo("  -> Hostname:     " + hostname + ".local");
        consoleLog.logInfo("  -> Assigned IP:  " + WiFi.localIP().toString());
        consoleLog.logInfo("  -> Gateway:      " + WiFi.gatewayIP().toString());
        consoleLog.logInfo("  -> Subnet Mask:  " + WiFi.subnetMask().toString());
        consoleLog.logInfo("  -> DNS Server:   " + WiFi.dnsIP().toString());
        consoleLog.logInfo("  -> Signal RSSI:  " + String(WiFi.RSSI()) + " dBm");
        consoleLog.logInfo("  -> Web Portal:   http://" + WiFi.localIP().toString() + "/ or http://" + hostname + ".local/");
        return true;
    }

    consoleLog.logError("WiFi connection timeout after " + String(timeoutMs / 1000) + "s! Falling back to AP mode...");
    return false;
}

// =============================================================================
// Arduino Setup & Loop
// =============================================================================
void setup() {
#if (defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(ARDUINO_LOLIN_S2_MINI)) && defined(ARDUINO_USB_CDC_ON_BOOT) && ARDUINO_USB_CDC_ON_BOOT
    Serial.setTxTimeoutMs(0); // Non-blocking if USB CDC terminal is not open
    Serial.begin(DEBUG_BAUD_RATE);
    Serial0.begin(DEBUG_BAUD_RATE, SERIAL_8N1, RX, TX); // Hardware UART0 on pins RX and TX
    
    // Give USB CDC host stack up to 2500ms to attach if USB is connected to PC
    unsigned long startWait = millis();
    while (!Serial && (millis() - startWait < 2500)) {
        delay(10);
    }
#else
    Serial.begin(DEBUG_BAUD_RATE);
#endif

    SystemMonitor::begin();
    pinMode(PIN_LED_WIFI, OUTPUT);
    setLedWifi(false);

    DEBUG_PRINTLN_EMPTY();
    DEBUG_PRINTLN("==================================================");
    DEBUG_PRINTLN("   Pylon Smart Monitor - USB Debug Console       ");
    DEBUG_PRINTLN("==================================================");

    consoleLog.begin();
    batteryHistory.begin();
    if (psramFound()) {
        consoleLog.logInfo("PSRAM detected & active: " + String(ESP.getFreePsram() / 1024) + " KB free (Console Log & Battery History moved to PSRAM)");
    }

    // Initialize Serial console for Pylontech
    pylonSerial.begin();

    // Initialize NVS
    preferences.begin(NVS_NAMESPACE, false);
    loadIntervals();
    mqttManager.loadConfig();

    String tzPosix = preferences.getString(NVS_KEY_TZ_POSIX, DEFAULT_TZ_POSIX);
    setenv("TZ", tzPosix.c_str(), 1);
    tzset();
    sntp_set_time_sync_notification_cb(timeSyncNotificationCallback);

    String savedSsid = preferences.getString("ssid", "");
    String savedPass = preferences.getString("pass", "");

    if (savedSsid.length() > 0) {
        if (!connectToSta(savedSsid, savedPass)) {
            startApMode();
        }
    } else {
        startApMode();
    }

    portal.setupRoutes();
    webServer.begin();

    // First initial scrape if connected to STA: perform full (fast + slow) poll
    if (WiFi.status() == WL_CONNECTED) {
        pylonSerial.pollStack(batteryStack, true);
        lastFastPollMillis = millis();
        lastSlowPollMillis = millis();
        mqttManager.publishState(batteryStack);
        recordHistorySample();
    }
}

void loop() {
    // Process any queued user commands from the web console FIFO queue
    pylonSerial.processQueue();

    // Refresh dynamic polling intervals
    loadIntervals();

    if (isApMode) {
        dnsServer.processNextRequest();

        // Blink WiFi LED in AP mode (500ms cycle)
        if (millis() - lastLedBlinkMillis >= 500) {
            lastLedBlinkMillis = millis();
            ledWifiState = !ledWifiState;
            setLedWifi(ledWifiState);
        }

        int apClients = WiFi.softAPgetStationNum();
        if (apClients > 0) {
            if (!apModeInitialPollDone) {
                consoleLog.logInfo("AP client connected (" + String(apClients) + " client(s)). Starting initial battery scrape...");
                apModeInitialPollDone = true;
                lastSlowPollMillis = millis();
                lastFastPollMillis = millis();
                pylonSerial.pollStack(batteryStack, true);
                recordHistorySample();
            } else if (!isPollingPaused) {
                if (triggerManualPoll || (millis() - lastSlowPollMillis >= slowPollIntervalMs)) {
                    triggerManualPoll = false;
                    lastSlowPollMillis = millis();
                    lastFastPollMillis = millis();
                    pylonSerial.pollStack(batteryStack, true);
                    recordHistorySample();
                } else if (millis() - lastFastPollMillis >= fastPollIntervalMs) {
                    lastFastPollMillis = millis();
                    pylonSerial.pollStack(batteryStack, false);
                    recordHistorySample();
                }
            }
        } else {
            // No stations connected to AP
            apModeInitialPollDone = false; // Reset so next connection triggers fresh poll
            if (triggerManualPoll) {
                triggerManualPoll = false;
                pylonSerial.pollStack(batteryStack, true);
                recordHistorySample();
            }
        }
    } else {
        // STA Mode
        if (WiFi.status() == WL_CONNECTED) {
            if (!staWifiWasConnected) {
                staWifiWasConnected = true;
                consoleLog.logInfo("WiFi reconnected! Assigned IP: " + WiFi.localIP().toString());
                String hostname = getDeviceHostname(preferences);
                MDNS.begin(hostname.c_str());
                MDNS.addService("http", "tcp", 80);
                MDNS.addService("pylon-smart", "tcp", 80);
                MDNS.addServiceTxt("pylon-smart", "tcp", "ver", FIRMWARE_VERSION);
                if (batteryStack.modelName[0] != '\0' && strcmp(batteryStack.modelName, "Unknown") != 0) {
                    MDNS.addServiceTxt("pylon-smart", "tcp", "model", (const char*)batteryStack.modelName);
                }
            }
            setLedWifi(true);
            ArduinoOTA.handle();
            mqttManager.loop();

            if (!isPollingPaused) {
                if (triggerManualPoll || (millis() - lastSlowPollMillis >= slowPollIntervalMs)) {
                    triggerManualPoll = false;
                    lastSlowPollMillis = millis();
                    lastFastPollMillis = millis();
                    pylonSerial.pollStack(batteryStack, true); // Full slow+fast poll
                    mqttManager.publishState(batteryStack);
                    recordHistorySample();
                } else if (millis() - lastFastPollMillis >= fastPollIntervalMs) {
                    lastFastPollMillis = millis();
                    pylonSerial.pollStack(batteryStack, false); // Fast poll only: pwr + bat
                    mqttManager.publishState(batteryStack);
                    recordHistorySample();
                }
            } else if (triggerManualPoll) {
                // If paused but user explicitly clicks "Poll Now"
                triggerManualPoll = false;
                pylonSerial.pollStack(batteryStack, true);
                mqttManager.publishState(batteryStack);
                recordHistorySample();
            }
        } else {
            static uint32_t lastReconnectAttempt = 0;
            if (staWifiWasConnected) {
                staWifiWasConnected = false;
                consoleLog.logError("WiFi connection lost! Attempting auto-reconnect...");
                WiFi.reconnect();
                lastReconnectAttempt = millis();
            } else if (millis() - lastReconnectAttempt >= 10000) {
                lastReconnectAttempt = millis();
                consoleLog.logInfo("Retrying WiFi connection...");
                WiFi.reconnect();
            }

            // WiFi disconnected:
            // "ked vypadne wifi, prikazy sa prestanu posielat"
            if (millis() - lastLedBlinkMillis >= 250) {
                lastLedBlinkMillis = millis();
                ledWifiState = !ledWifiState;
                setLedWifi(ledWifiState);
            }
        }
    }

    if (ntpJustSynced) {
        ntpJustSynced = false;
        struct tm ti;
        localtime_r(&lastNtpSyncTimestamp, &ti);
        char buf[32];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &ti);
        consoleLog.logInfo("NTP synchronized successfully! Device time: " + String(buf));
    }
    if (lastNtpSyncTimestamp == 0 && time(nullptr) > 1577836800) {
        lastNtpSyncTimestamp = time(nullptr);
    }

    webServer.handleClient();
    delay(2);
}
