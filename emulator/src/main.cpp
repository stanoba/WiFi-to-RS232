#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <esp_sntp.h>
#include <algorithm>

#include "Config.h"
#include "Timezones.h"
#include "BmsModel.h"
#include "BmsPhysics.h"
#include "BmsProtocol.h"
#include "BmsUart.h"
#include "ConsoleLog.h"
#include "SystemStats.h"
#include "PeerDiscovery.h"
#include "WebPortal.h"

// Global instances
Preferences prefs;
StackData g_stack;
ConsoleLogManager g_consoleLog;
PeerDiscoveryManager g_peerDiscovery;
DNSServer dnsServer;
bool isApMode = false;

// NTP Time Synchronization State
time_t lastNtpSyncTimestamp = 0;
volatile bool ntpJustSynced = false;

void timeSyncNotificationCallback(struct timeval *tv) {
    lastNtpSyncTimestamp = time(nullptr);
    ntpJustSynced = true;
}

void triggerNtpSync() {
    bool ntpEn = prefs.getBool(NVS_KEY_NTP_ENABLED, true);
    if (!ntpEn) {
        if (sntp_enabled()) {
            sntp_stop();
        }
        g_consoleLog.logInfo("NTP is disabled in configuration.");
        return;
    }
    String tzPosix = prefs.getString(NVS_KEY_TZ_POSIX, DEFAULT_TZ_POSIX);
    String ntpSrv = prefs.getString(NVS_KEY_NTP_SERVER, NTP_DEFAULT_SERVER);

    if (sntp_enabled()) {
        sntp_stop();
    }
    sntp_set_time_sync_notification_cb(timeSyncNotificationCallback);
    configTzTime(tzPosix.c_str(), ntpSrv.c_str(), "time.google.com", "time.cloudflare.com");
    g_consoleLog.logInfo("NTP client started: " + ntpSrv + " (TZ: " + tzPosix + ")");
}
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(ARDUINO_ESP32C3_DEV)
    HardwareSerial bmsSerial(1);
#elif defined(CONFIG_IDF_TARGET_ESP32S2) || defined(ARDUINO_LOLIN_S2_MINI)
    HardwareSerial bmsSerial(1);
#else
    HardwareSerial bmsSerial(2);
#endif

BmsUartHandler uartHandler(bmsSerial, PIN_UART_TX, PIN_UART_RX, PIN_LED_SERIAL);
BmsWebPortal webPortal(prefs, uartHandler);

// =============================================================================
// Hierarchy Rules & NVS Storage
// =============================================================================
void saveRackConfigToNvs() {
    RackNvsRecord rec;
    memset(&rec, 0, sizeof(rec));
    rec.magic = RACK_NVS_MAGIC;
    rec.version = RACK_NVS_VERSION;
    rec.module_count = (g_stack.module_count > MAX_MODULES) ? MAX_MODULES : g_stack.module_count;
    rec.global_soc = g_stack.global_soc;
    rec.global_current_a = g_stack.global_current_a;
    rec.global_temp_c = g_stack.global_temp_c;
    rec.cell_spread_mv = g_stack.cell_spread_mv;
    rec.auto_physics = g_stack.auto_physics ? 1 : 0;

    for (uint8_t i = 0; i < rec.module_count; i++) {
        const ModuleData &m = g_stack.modules[i];
        ModuleNvsRecord &mr = rec.modules[i];
        mr.model_type = (uint8_t)m.model_type;
        strncpy(mr.fw_version, m.fw_version.c_str(), sizeof(mr.fw_version) - 1);
        strncpy(mr.barcode, m.barcode.c_str(), sizeof(mr.barcode) - 1);
        mr.soh_pct = m.soh_pct;
        mr.cycle_times = m.cycle_times;
        mr.soh_times = m.soh_times;
        mr.shut_times = m.shut_times;
        mr.rst_times = m.rst_times;
        mr.power_on_times = m.power_on_times;
        mr.idle_times = m.idle_times;
        mr.chg_times_or_secs = m.chg_times_or_secs;
        mr.dsg_times_or_secs = m.dsg_times_or_secs;
        mr.dsg_cap_total = m.dsg_cap_total;

        // Current & Hardware Protections
        mr.coc_times = m.coc_times;
        mr.coca_times = m.coca_times;
        mr.doc_times = m.doc_times;
        mr.doca_times = m.doca_times;
        mr.sc_times = m.sc_times;
        mr.rv_times = m.rv_times;
        mr.input_ov_times = m.input_ov_times;
        mr.bmic_err_times = m.bmic_err_times;
        mr.life_alarm_times = m.life_alarm_times;
        mr.life_warn_times = m.life_warn_times;
        mr.bat_slp_times = m.bat_slp_times;
        mr.pwr_slp_times = m.pwr_slp_times;

        // Voltage & Thermal Protections
        mr.bat_ov_times = m.bat_ov_times;
        mr.bat_hv_times = m.bat_hv_times;
        mr.bat_lv_times = m.bat_lv_times;
        mr.bat_uv_times = m.bat_uv_times;
        mr.pwr_ov_times = m.pwr_ov_times;
        mr.pwr_hv_times = m.pwr_hv_times;
        mr.pwr_lv_times = m.pwr_lv_times;
        mr.puv_times = m.pwr_uv_times;
        mr.cot_times = m.cot_times;
        mr.cut_times = m.cut_times;
        mr.dot_times = m.dot_times;
        mr.dut_times = m.dut_times;
        mr.cht_times = m.cht_times;
        mr.clt_times = m.clt_times;
        mr.dht_times = m.dht_times;
        mr.dlt_times = m.dlt_times;

        // Euro Stats
        mr.euro_soh_pct = m.euro.soh_pct;
        mr.euro_life_expect_days = m.euro.life_expect_days;
        mr.euro_energy_thro_kwh = m.euro.energy_thro_kwh;
        mr.euro_round_trip_eff_pct = m.euro.round_trip_eff_pct;
    }

    size_t saveSize = sizeof(rec) - (sizeof(ModuleNvsRecord) * (MAX_MODULES - rec.module_count));
    prefs.putBytes("rack_cfg", &rec, saveSize);
}

bool loadRackConfigFromNvs() {
    RackNvsRecord rec;
    memset(&rec, 0, sizeof(rec));
    size_t len = prefs.getBytes("rack_cfg", &rec, sizeof(rec));
    if (len >= 32 && rec.magic == RACK_NVS_MAGIC && rec.module_count >= 1 && rec.module_count <= MAX_MODULES) {
        g_stack.module_count = rec.module_count;
        g_stack.global_soc = (rec.global_soc >= 0.0f && rec.global_soc <= 100.0f) ? rec.global_soc : 98.0f;
        g_stack.global_current_a = rec.global_current_a;
        g_stack.global_temp_c = (rec.global_temp_c >= -40.0f && rec.global_temp_c <= 100.0f) ? rec.global_temp_c : 24.5f;
        g_stack.cell_spread_mv = (rec.cell_spread_mv <= 200) ? rec.cell_spread_mv : 8;
        g_stack.auto_physics = (rec.auto_physics != 0);
        g_stack.sim_inverter_enabled = false;

        for (uint8_t i = 0; i < rec.module_count; i++) {
            ModuleData &m = g_stack.modules[i];
            const ModuleNvsRecord &mr = rec.modules[i];
            m.id = i + 1;
            m.model_type = (PylonModelType)(mr.model_type < MODEL_COUNT ? mr.model_type : MODEL_US3000C);
            const ModelDescriptor &desc = getModelDescriptor(m.model_type);
            m.cell_count = desc.cell_count;
            m.fw_version = (strlen(mr.fw_version) > 0) ? String(mr.fw_version) : desc.default_fw;
            m.barcode = (strlen(mr.barcode) > 0) ? String(mr.barcode) : (String(desc.barcode_prefix) + "2022041800" + String(m.id < 10 ? "0" : "") + String(m.id));
            m.soh_pct = (mr.soh_pct > 0.0f && mr.soh_pct <= 100.0f) ? mr.soh_pct : 99.0f;
            m.cycle_times = mr.cycle_times;
            m.soh_times = mr.soh_times;
            m.shut_times = mr.shut_times;
            m.rst_times = mr.rst_times;
            m.power_on_times = mr.power_on_times;
            m.idle_times = mr.idle_times;
            m.chg_times_or_secs = mr.chg_times_or_secs;
            m.dsg_times_or_secs = mr.dsg_times_or_secs;
            m.dsg_cap_total = mr.dsg_cap_total;

            // Current & Hardware Protections
            m.coc_times = mr.coc_times;
            m.coca_times = mr.coca_times;
            m.doc_times = mr.doc_times;
            m.doca_times = mr.doca_times;
            m.sc_times = mr.sc_times;
            m.rv_times = mr.rv_times;
            m.input_ov_times = mr.input_ov_times;
            m.bmic_err_times = mr.bmic_err_times;
            m.life_alarm_times = mr.life_alarm_times;
            m.life_warn_times = mr.life_warn_times;
            m.bat_slp_times = mr.bat_slp_times;
            m.pwr_slp_times = mr.pwr_slp_times;

            // Voltage & Thermal Protections
            m.bat_ov_times = mr.bat_ov_times;
            m.bat_hv_times = mr.bat_hv_times;
            m.bat_lv_times = mr.bat_lv_times;
            m.bat_uv_times = mr.bat_uv_times;
            m.pwr_ov_times = mr.pwr_ov_times;
            m.pwr_hv_times = mr.pwr_hv_times;
            m.pwr_lv_times = mr.pwr_lv_times;
            m.pwr_uv_times = mr.puv_times;
            m.cot_times = mr.cot_times;
            m.cut_times = mr.cut_times;
            m.dot_times = mr.dot_times;
            m.dut_times = mr.dut_times;
            m.cht_times = mr.cht_times;
            m.clt_times = mr.clt_times;
            m.dht_times = mr.dht_times;
            m.dlt_times = mr.dlt_times;

            m.volt_st = "Normal";
            m.curr_st = "Normal";
            m.temp_st = "Normal";
            m.dtemp_st = "Normal";
            m.ctemp_st = "Normal";
            m.mos_temp_st = "Normal";
            m.b_v_st = "Normal";
            m.b_t_st = "Normal";
            m.soc = g_stack.global_soc;

            m.euro.soh_pct = (mr.euro_soh_pct > 0.0f) ? mr.euro_soh_pct : m.soh_pct;
            m.euro.life_expect_days = mr.euro_life_expect_days ? mr.euro_life_expect_days : 5475;
            m.euro.energy_thro_kwh = mr.euro_energy_thro_kwh ? mr.euro_energy_thro_kwh : 1250.0f;
            m.euro.round_trip_eff_pct = mr.euro_round_trip_eff_pct ? mr.euro_round_trip_eff_pct : 96.5f;
        }
        recalculatePhysics();
        return true;
    }

    // Clean up any legacy multi-key storage if found
    if (prefs.isKey("mod_count")) {
        prefs.remove("mod_count");
        for (int i = 0; i < MAX_MODULES; i++) {
            String pfx = "m" + String(i) + "_";
            const char* legacySuffixes[] = {"typ","fw","bar","soh_p","cyc","soh_t","shut","rst","pon","idle","chg_t","dsg_t","dsg_c","coc","coca","doc","doca","sc","rv","iov","bmic","lalm","lwrn","bslp","pslp","bov","bhv","blv","buv","pov","phv","plv","puv","cot","cut","dot","dut","cht","clt","dht","dlt","e_soh","e_life","e_en","e_eff"};
            for (const char* sfx : legacySuffixes) {
                prefs.remove((pfx + sfx).c_str());
            }
        }
    }

    return false;
}

// =============================================================================
// BMS Initial State Setup (Default: Single US3000C Module)
// =============================================================================
void initBmsDefaults() {
    g_stack.module_count = 1;
    g_stack.global_soc = 98.0f;
    g_stack.global_current_a = -0.05f; // Slight idle discharge
    g_stack.global_temp_c = 24.5f;
    g_stack.cell_spread_mv = 8;
    g_stack.auto_physics = true;
    g_stack.sim_inverter_enabled = false;

    // Module 1: Master US3000C
    {
        ModuleData &m1 = g_stack.modules[0];
        m1.id = 1;
        m1.model_type = MODEL_US3000C;
        m1.fw_version = "V2.8";
        m1.barcode = "PPYB202204180012";
        m1.cell_count = 15;
        m1.soc = 98.0f;
        m1.soh_pct = 99.0f;
        m1.volt_st = "Normal";
        m1.curr_st = "Normal";
        m1.temp_st = "Normal";
        m1.dtemp_st = "Normal";
        m1.ctemp_st = "Normal";
        m1.mos_temp_st = "Normal";
        m1.b_v_st = "Normal";
        m1.b_t_st = "Normal";
        m1.cycle_times = 180;
        m1.shut_times = 1;
        m1.rst_times = 3;
        m1.power_on_times = 12;
        m1.idle_times = 3690;
        m1.soh_times = 0;
        m1.chg_times_or_secs = 452;
        m1.dsg_times_or_secs = 450;
        m1.pwr_coulomb_mc = 266400000ULL;
        m1.dsg_cap_total = 24534743; // mAh

        // Current & Hardware Protections
        m1.coc_times = 0;
        m1.coca_times = 0;
        m1.doc_times = 0;
        m1.doca_times = 0;
        m1.sc_times = 0;
        m1.rv_times = 0;
        m1.input_ov_times = 0;
        m1.bmic_err_times = 0;
        m1.life_alarm_times = 0;
        m1.life_warn_times = 0;
        m1.bat_slp_times = 0;
        m1.pwr_slp_times = 0;

        // Voltage & Thermal Protections
        m1.bat_ov_times = 0;
        m1.bat_hv_times = 1;
        m1.bat_lv_times = 0;
        m1.bat_uv_times = 0;
        m1.pwr_ov_times = 0;
        m1.pwr_hv_times = 0;
        m1.pwr_lv_times = 0;
        m1.pwr_uv_times = 0;
        m1.cot_times = 0;
        m1.cut_times = 0;
        m1.dot_times = 0;
        m1.dut_times = 0;
        m1.cht_times = 0;
        m1.clt_times = 0;
        m1.dht_times = 0;
        m1.dlt_times = 0;

        // Euro Stats
        m1.euro.soh_pct = 99.0f;
        m1.euro.life_expect_days = 5475;
        m1.euro.energy_thro_kwh = 1250.0f;
        m1.euro.round_trip_eff_pct = 96.5f;
    }

    recalculatePhysics();
}

// =============================================================================
// OTA Setup
// =============================================================================
void setupOta(const String &hostname) {
    ArduinoOTA.setHostname(hostname.c_str());
    ArduinoOTA.setPort(OTA_PORT);

    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        g_consoleLog.logInfo("ArduinoOTA start updating " + type);
        DEBUG_PRINTLN("[OTA] Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        g_consoleLog.logInfo("ArduinoOTA update complete");
        DEBUG_PRINTLN("[OTA] Update complete");
    });
    ArduinoOTA.onError([](ota_error_t error) {
        g_consoleLog.logInfo("ArduinoOTA error [" + String(error) + "]");
        DEBUG_PRINTLN("[OTA] Error [" + String(error) + "]");
    });

    ArduinoOTA.begin();
    g_consoleLog.logInfo("ArduinoOTA initialized (" + hostname + ") on port " + String(OTA_PORT));
    DEBUG_PRINTLN("[OTA] ArduinoOTA initialized (" + hostname + ") on port " + String(OTA_PORT));
}

// =============================================================================
// WiFi & Network Initialization
// =============================================================================
void setupWiFi() {
    String hostname = getDeviceHostname(prefs);
    WiFi.setHostname(hostname.c_str());

    bool staticEn = prefs.getBool(NVS_KEY_STATIC_IP_EN, false);
    if (staticEn) {
        String ipStr = prefs.getString(NVS_KEY_STATIC_IP, "");
        String maskStr = prefs.getString(NVS_KEY_STATIC_MASK, "255.255.255.0");
        String gwStr = prefs.getString(NVS_KEY_STATIC_GW, "");
        String dnsStr = prefs.getString(NVS_KEY_STATIC_DNS, "");
        IPAddress ip, mask, gw, dns;
        if (ip.fromString(ipStr) && mask.fromString(maskStr) && gw.fromString(gwStr)) {
            if (!dns.fromString(dnsStr)) dns = gw;
            WiFi.config(ip, gw, mask, dns, IPAddress(8, 8, 8, 8));
            g_consoleLog.logInfo("Static IP configured: " + ipStr + " (GW: " + gwStr + ")");
            DEBUG_PRINTLN("[WiFi] Static IP configured: " + ipStr + " (GW: " + gwStr + ")");
        }
    } else {
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    }

    String ssid = prefs.getString(NVS_KEY_WIFI_SSID, "");
    String pass = prefs.getString(NVS_KEY_WIFI_PASS, "");

    if (ssid.length() > 0) {
        g_consoleLog.logInfo("Connecting to WiFi SSID: " + ssid);
        DEBUG_PRINT("Connecting to WiFi: ");
        DEBUG_PRINTLN(ssid);
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), pass.c_str());

        uint32_t startMs = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startMs < 10000) {
            delay(250);
            DEBUG_PRINT(".");
        }
        DEBUG_PRINTLN("");
    }

    if (WiFi.status() == WL_CONNECTED) {
        isApMode = false;
        DEBUG_PRINT("WiFi Connected. IP: ");
        DEBUG_PRINTLN(WiFi.localIP());
        if (PIN_LED_WIFI >= 0) {
            pinMode(PIN_LED_WIFI, OUTPUT);
            digitalWrite(PIN_LED_WIFI, LED_ACTIVE_LEVEL); // Solid ON
        }
        g_consoleLog.logInfo("WiFi connected successfully!");
        g_consoleLog.logInfo("  -> Hostname:     " + hostname + ".local");
        g_consoleLog.logInfo("  -> Assigned IP:  " + WiFi.localIP().toString());
        g_consoleLog.logInfo("  -> Gateway:      " + WiFi.gatewayIP().toString());
        g_consoleLog.logInfo("  -> Subnet Mask:  " + WiFi.subnetMask().toString());
        g_consoleLog.logInfo("  -> DNS Server:   " + WiFi.dnsIP().toString());
        g_consoleLog.logInfo("  -> Signal RSSI:  " + String(WiFi.RSSI()) + " dBm (" + String(SystemStatsManager::calcSignalQuality(WiFi.RSSI())) + "%)");
        g_consoleLog.logInfo("  -> Web Portal:   http://" + WiFi.localIP().toString() + "/ or http://" + hostname + ".local/");

        setupOta(hostname);
        triggerNtpSync();
    } else {
        isApMode = true;
        DEBUG_PRINTLN("Starting Access Point mode...");
        WiFi.mode(WIFI_AP);
        String apSsid = AP_SSID_PREFIX "-" + WiFi.macAddress().substring(12, 14) + WiFi.macAddress().substring(15, 17);
        WiFi.softAP(apSsid.c_str(), AP_DEFAULT_PASSWORD);
        dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
        DEBUG_PRINT("AP SSID: ");
        DEBUG_PRINT(apSsid);
        DEBUG_PRINT(", IP: ");
        DEBUG_PRINTLN(WiFi.softAPIP());

        g_consoleLog.logInfo("WiFi not connected. Starting Access Point mode...");
        g_consoleLog.logInfo("  -> AP SSID:      " + apSsid);
        g_consoleLog.logInfo("  -> AP Password:  " + String(AP_DEFAULT_PASSWORD));
        g_consoleLog.logInfo("  -> AP Web Portal: http://" + WiFi.softAPIP().toString() + "/");
    }

    if (MDNS.begin(hostname.c_str())) {
        DEBUG_PRINT("mDNS responder started: http://");
        DEBUG_PRINT(hostname);
        DEBUG_PRINTLN(".local");
        MDNS.addService("pylon-emu", "tcp", 80);
    }

    g_peerDiscovery.begin(hostname);
}

// =============================================================================
// LED Status Handler
// =============================================================================
static uint32_t lastLedBlinkMs = 0;
static bool ledWifiState = false;

void updateWifiLed() {
    if (PIN_LED_WIFI < 0) return;

    if (WiFi.status() == WL_CONNECTED) {
        digitalWrite(PIN_LED_WIFI, LED_ACTIVE_LEVEL); // Solid ON when connected
    } else {
        // AP mode or connecting -> blink at 500ms intervals
        if (millis() - lastLedBlinkMs >= 500) {
            lastLedBlinkMs = millis();
            ledWifiState = !ledWifiState;
            digitalWrite(PIN_LED_WIFI, ledWifiState ? LED_ACTIVE_LEVEL : !LED_ACTIVE_LEVEL);
        }
    }
}

// =============================================================================
// Arduino Setup & Loop
// =============================================================================
void setup() {
    Serial.begin(DEBUG_BAUD_RATE);
    delay(500);
    DEBUG_PRINTLN("\r\n========================================");
    DEBUG_PRINTLN(" Pylontech BMS RS232 Emulator v" FIRMWARE_VERSION);
    DEBUG_PRINTLN("========================================");

    if (PIN_LED_WIFI >= 0) {
        pinMode(PIN_LED_WIFI, OUTPUT);
        digitalWrite(PIN_LED_WIFI, !LED_ACTIVE_LEVEL); // Start OFF
    }

    prefs.begin(NVS_NAMESPACE, false);
    g_consoleLog.begin();
    SystemStatsManager::init();

    g_consoleLog.logInfo("Pylon BMS Console Emulator v" + String(FIRMWARE_VERSION) + " starting...");
#if defined(BOARD_HAS_PSRAM) || defined(CONFIG_SPIRAM_SUPPORT)
    if (psramFound()) {
        g_consoleLog.logInfo("PSRAM detected: " + String(ESP.getFreePsram() / 1024) + " KB free (Console RingBuffer active in PSRAM)");
    } else {
        g_consoleLog.logInfo("System initialized in SRAM (320KB RAM, RingBuffer active)");
    }
#else
    g_consoleLog.logInfo("System initialized in SRAM (320KB RAM, RingBuffer active)");
#endif

    if (!loadRackConfigFromNvs()) {
        initBmsDefaults();
        saveRackConfigToNvs();
    }
    g_consoleLog.logInfo("Battery Stack initialized with " + String(g_stack.module_count) + " module(s).");
    g_consoleLog.logInfo("  -> Master Unit:  " + String(getModelDescriptor(g_stack.modules[0].model_type).name) + " (FW " + g_stack.modules[0].fw_version + ")");
    g_consoleLog.logInfo("  -> Stack State:  " + String(g_stack.modules[0].voltage_mv / 1000.0f, 2) + " V | SOC: " + String((int)g_stack.global_soc) + "%");

    DEBUG_PRINTLN("[INIT] Stack initialized with " + String(g_stack.module_count) + " module(s).");
    DEBUG_PRINTLN("[INIT] Master: " + String(getModelDescriptor(g_stack.modules[0].model_type).name) + " FW: " + g_stack.modules[0].fw_version);
    DEBUG_PRINTLN("[INIT] Voltage: " + String(g_stack.modules[0].voltage_mv / 1000.0f, 2) + " V | SOC: " + String((int)g_stack.global_soc) + "%");

    setupWiFi();

    uartHandler.begin(SERIAL_BAUD_RATE);
    g_consoleLog.logInfo("RS232 Hardware Serial active on TX (GPIO " + String(PIN_UART_TX) + ") & RX (GPIO " + String(PIN_UART_RX) + ") @ 115200 baud");
    DEBUG_PRINTLN("[UART] RS232 Hardware Serial active on TX (GPIO " + String(PIN_UART_TX) + ") & RX (GPIO " + String(PIN_UART_RX) + ") @ 115200 baud");

    webPortal.begin();
    g_consoleLog.logInfo("HTTP Web Portal listening on port 80");
    g_consoleLog.logInfo("Ready for RS232 requests from Smart Monitor.");

    DEBUG_PRINTLN("[HTTP] Web Portal listening on port 80");
    DEBUG_PRINTLN("========================================");
    DEBUG_PRINTLN("Ready for RS232 requests from Smart Monitor.\r\n");
}

void loop() {
    if (isApMode) {
        dnsServer.processNextRequest();
    }
    if (WiFi.status() == WL_CONNECTED) {
        ArduinoOTA.handle();
    }
    updateInverterSimulation();
    uartHandler.loop();
    webPortal.loop();
    updateWifiLed();
}
