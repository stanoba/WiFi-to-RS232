#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>

// =============================================================================
// Firmware Version (SemVer: MAJOR.MINOR.PATCH)
// =============================================================================
#define FIRMWARE_VERSION            "1.2.0"
#define FIRMWARE_BUILD_DATE         __DATE__
#define FIRMWARE_BUILD_TIME         __TIME__

// =============================================================================
// UART Configuration & Pin Definitions (Pylontech Console & LEDs)
// =============================================================================
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(ARDUINO_ESP32C3_DEV)
// ESP32-C3 Super Mini pin mapping:
#define PIN_UART_TX                 4    // GPIO4 (MAX3232 T1IN -> Pylontech Console RX)
#define PIN_UART_RX                 5    // GPIO5 (MAX3232 R1OUT <- Pylontech Console TX)
#define PIN_LED_WIFI                8    // GPIO8 (On-board blue LED, Active-LOW)
#define PIN_LED_SERIAL              3    // GPIO3 (LED SER - Serial RX/TX activity)
#elif defined(CONFIG_IDF_TARGET_ESP32S2) || defined(ARDUINO_LOLIN_S2_MINI)
// LOLIN S2 Mini (ESP32-S2) outer socket pin mapping matching Wemos D1 Mini shield:
#define PIN_UART_TX                 12   // Outer Socket D8 = GPIO12 (MAX3232 T1IN -> Pylontech Console RX)
#define PIN_UART_RX                 11   // Outer Socket D7 = GPIO11 (MAX3232 R1OUT <- Pylontech Console TX)
#define PIN_LED_WIFI                18   // Outer Socket D3 = GPIO18 (LED CONN - WiFi status)
#define PIN_LED_SERIAL              16   // Outer Socket D4 = GPIO16 (LED SER - Serial RX/TX activity)
#else
// Standard Wemos D1 Mini ESP32 (classic ESP32):
#define PIN_UART_TX                 5    // Wemos D1 Mini D8 = GPIO5 (MAX3232 T1IN -> Pylontech Console RX)
#define PIN_UART_RX                 23   // Wemos D1 Mini D7 = GPIO23 (MAX3232 R1OUT <- Pylontech Console TX)
#define PIN_LED_WIFI                17   // Wemos D1 Mini D3 = GPIO17 (LED CONN - WiFi status)
#define PIN_LED_SERIAL              16   // Wemos D1 Mini D4 = GPIO16 (LED SER - Serial RX/TX activity)
#endif

#define SERIAL_BAUD_RATE            115200
#define SERIAL_TIMEOUT_MS           2500
#define SERIAL_RX_BUFFER_SIZE       2048

#define DEBUG_BAUD_RATE             115200  // USB UART debug console baud rate

#if (defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(ARDUINO_LOLIN_S2_MINI)) && defined(ARDUINO_USB_CDC_ON_BOOT) && ARDUINO_USB_CDC_ON_BOOT
    #define DEBUG_PRINT(x)          do { Serial.print(x); Serial0.print(x); } while(0)
    #define DEBUG_PRINTLN(x)        do { Serial.println(x); Serial0.println(x); } while(0)
    #define DEBUG_PRINTLN_EMPTY()   do { Serial.println(); Serial0.println(); } while(0)
#else
    #define DEBUG_PRINT(x)          Serial.print(x)
    #define DEBUG_PRINTLN(x)        Serial.println(x)
    #define DEBUG_PRINTLN_EMPTY()   Serial.println()
#endif

// Polarity: LEDs are connected in Active-LOW configuration (VCC -> Resistor -> LED -> GPIO).
// LOW turns the LED ON, HIGH turns the LED OFF.
#define LED_ACTIVE_LEVEL            LOW

// =============================================================================
// Polling & Timing Parameters
// =============================================================================
#define FAST_POLL_INTERVAL_MS       60000   // Default 1 minute: dynamic telemetry (pwr, bat)
#define SLOW_POLL_INTERVAL_MS       300000  // Default 5 minutes: static & slow telemetry (stat, info, soh/euro)
#define SERIAL_COMMAND_DELAY_MS     250    // Pause between consecutive console commands
#define MAX_MODULES                 16     // Maximum supported modules in Pylontech stack
#define MAX_CELLS_PER_MODULE        16     // Up to 16 cells per module (US3000: 15, US5000/UP5000: 16)

// =============================================================================
// Captive Portal & AP Mode Setup
// =============================================================================
#define AP_SSID_PREFIX              "Pylon-Smart"
#define AP_DEFAULT_PASSWORD         ""     // Empty = Open AP (no password required)
#define AP_IP_ADDRESS               192, 168, 4, 1
#define DNS_PORT                    53
#define HTTP_PORT                   80

// =============================================================================
// Over-The-Air (OTA) Configuration
// =============================================================================
#define OTA_HOSTNAME                "pylon-smart"
#define OTA_PORT                    3232

// =============================================================================
// Web Console Log Parameters
// =============================================================================
#define LOG_AUTO_REFRESH_INTERVAL_SEC 5    // Auto-refresh interval in seconds on /log page

// =============================================================================
// NTP Time Synchronization Configuration
// =============================================================================
#define NTP_ENABLED                 true           // Default NTP enabled
#define NTP_DEFAULT_SERVER          "pool.ntp.org" // Default NTP Server
#define DEFAULT_TZ_POSIX            "CET-1CEST,M3.5.0,M10.5.0/3" // Europe/Bratislava
#define DEFAULT_TZ_CITY             "Europe/Bratislava (UTC+1, CEST)"

// NVS Namespace for Preferences
#define NVS_NAMESPACE               "pylon_cfg"

// NVS Keys
#define NVS_KEY_AUTH_ENABLED        "auth_en"
#define NVS_KEY_AUTH_USER           "auth_usr"
#define NVS_KEY_AUTH_PASS           "auth_pwd"
#define NVS_KEY_API_AUTH_ENABLED    "api_en"
#define NVS_KEY_API_TOKEN           "api_tok"
#define NVS_KEY_FAST_POLL_SEC       "fast_sec"
#define NVS_KEY_SLOW_POLL_SEC       "slow_sec"
#define NVS_KEY_MQTT_ENABLED        "mq_en"
#define NVS_KEY_MQTT_SERVER         "mq_srv"
#define NVS_KEY_MQTT_PORT           "mq_prt"
#define NVS_KEY_MQTT_USER           "mq_usr"
#define NVS_KEY_MQTT_PASS           "mq_pwd"
#define NVS_KEY_MQTT_PREFIX         "mq_pfx"
#define NVS_KEY_NTP_ENABLED         "ntp_en"
#define NVS_KEY_NTP_SERVER          "ntp_srv"
#define NVS_KEY_TZ_POSIX            "tz_posix"
#define NVS_KEY_TZ_CITY             "tz_city"
#define NVS_KEY_STATIC_IP_EN        "ip_static"
#define NVS_KEY_STATIC_IP           "ip_addr"
#define NVS_KEY_STATIC_MASK         "ip_mask"
#define NVS_KEY_STATIC_GW           "ip_gw"
#define NVS_KEY_STATIC_DNS          "ip_dns"
#define NVS_KEY_TIME_FORMAT_24H     "time_24h"
#define NVS_KEY_HOSTNAME            "dev_host"

// =============================================================================
// Hostname Generation & Retrieval
// =============================================================================
inline String getDefaultHostname() {
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    String suffix = (mac.length() >= 4) ? mac.substring(mac.length() - 4) : "0000";
    suffix.toLowerCase();
    return "pylon-smart-" + suffix;
}

inline String getDeviceHostname(Preferences &prefs) {
    String host = prefs.getString(NVS_KEY_HOSTNAME, "");
    host.trim();
    if (host.length() == 0) {
        return getDefaultHostname();
    }
    host.toLowerCase();
    return host;
}



