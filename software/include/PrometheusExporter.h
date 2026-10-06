#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include "BatteryData.h"
#include "Config.h"
#include "SystemStats.h"

class PrometheusExporter {
public:
    static void generateMetrics(const BatteryStack &stack, WebServer &server) {
        server.setContentLength(CONTENT_LENGTH_UNKNOWN);
        server.send(200, "text/plain; version=0.0.4; charset=utf-8", "");

        String chunk;
        chunk.reserve(1024);

        auto flushChunk = [&]() {
            if (chunk.length() > 0) {
                server.sendContent(chunk);
                chunk = "";
            }
        };

        // --- Exporter Meta Telemetry ---
        chunk += "# HELP pylontech_scrape_success Whether the last serial scrape was successful\n";
        chunk += "# TYPE pylontech_scrape_success gauge\n";
        chunk += "pylontech_scrape_success " + String(stack.scrapeSuccess ? 1 : 0) + "\n\n";

        chunk += "# HELP pylontech_firmware_info Firmware version metadata\n";
        chunk += "# TYPE pylontech_firmware_info gauge\n";
        chunk += "pylontech_firmware_info{version=\"" + String(FIRMWARE_VERSION) + "\",build_date=\"" + String(FIRMWARE_BUILD_DATE) + "\",build_time=\"" + String(FIRMWARE_BUILD_TIME) + "\"} 1\n\n";

        chunk += "# HELP pylontech_scrape_duration_seconds Duration of serial scrape in seconds\n";
        chunk += "# TYPE pylontech_scrape_duration_seconds gauge\n";
        chunk += "pylontech_scrape_duration_seconds " + String(stack.scrapeDurationMs / 1000.0, 3) + "\n\n";

        chunk += "# HELP pylontech_modules_detected Number of active battery modules\n";
        chunk += "# TYPE pylontech_modules_detected gauge\n";
        chunk += "pylontech_modules_detected " + String(stack.moduleCount) + "\n\n";
        flushChunk();

        // --- Module Info (Label metrics with value 1) ---
        chunk += "# HELP pylontech_module_info Battery module metadata\n";
        chunk += "# TYPE pylontech_module_info gauge\n";
        for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
            const BatteryModule &mod = stack.modules[m];
            if (!mod.present) continue;

            const ModuleInfo &info = mod.info;
            String model;
            if (strlen(info.deviceName) > 0) {
                model = info.deviceName;
            } else if (stack.model != MODEL_UNKNOWN && m == stack.activeModuleIndex) {
                model = stack.modelName;
            } else {
                model = "Device " + String(m);
            }

            chunk += "pylontech_module_info{module=\"" + String(m) + "\"";
            chunk += ",model=\"" + model + "\"";
            if (strlen(info.barcode) > 0) chunk += ",barcode=\"" + String(info.barcode) + "\"";
            if (strlen(info.board) > 0) chunk += ",board=\"" + String(info.board) + "\"";
            if (strlen(info.mainSoftVersion) > 0) chunk += ",main_soft=\"" + String(info.mainSoftVersion) + "\"";
            if (strlen(info.softVersion) > 0) chunk += ",soft=\"" + String(info.softVersion) + "\"";
            chunk += "} 1\n";
        }
        chunk += "\n";
        flushChunk();

        // --- Voltage ---
        chunk += "# HELP pylontech_voltage_millivolts Module overall voltage\n";
        chunk += "# TYPE pylontech_voltage_millivolts gauge\n";
        for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
            if (stack.modules[m].present && stack.modules[m].power.valid) {
                chunk += "pylontech_voltage_millivolts{module=\"" + String(m) + "\"} " + String(stack.modules[m].power.voltMv) + "\n";
            }
        }
        chunk += "\n";

        // --- Current ---
        chunk += "# HELP pylontech_current_milliamps Module overall current\n";
        chunk += "# TYPE pylontech_current_milliamps gauge\n";
        for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
            if (stack.modules[m].present && stack.modules[m].power.valid) {
                chunk += "pylontech_current_milliamps{module=\"" + String(m) + "\"} " + String(stack.modules[m].power.currMa) + "\n";
            }
        }
        chunk += "\n";
        flushChunk();

        // --- Temperature ---
        chunk += "# HELP pylontech_temperature_millicelsius Module temperature in millidegrees Celsius\n";
        chunk += "# TYPE pylontech_temperature_millicelsius gauge\n";
        for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
            if (stack.modules[m].present && stack.modules[m].power.valid) {
                chunk += "pylontech_temperature_millicelsius{module=\"" + String(m) + "\"} " + String(stack.modules[m].power.tempMdeg) + "\n";
            }
        }
        chunk += "\n";

        // --- MOS Temperature ---
        chunk += "# HELP pylontech_mosfet_temperature_millicelsius BMS MOSFET temperature in millidegrees Celsius\n";
        chunk += "# TYPE pylontech_mosfet_temperature_millicelsius gauge\n";
        for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
            if (stack.modules[m].present && stack.modules[m].power.valid && stack.modules[m].power.mosTempMdeg > 0) {
                chunk += "pylontech_mosfet_temperature_millicelsius{module=\"" + String(m) + "\"} " + String(stack.modules[m].power.mosTempMdeg) + "\n";
            }
        }
        chunk += "\n";
        flushChunk();

        // --- State of Charge (SOC) ---
        chunk += "# HELP pylontech_soc_percent Module State of Charge percent\n";
        chunk += "# TYPE pylontech_soc_percent gauge\n";
        for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
            if (stack.modules[m].present && stack.modules[m].power.valid) {
                chunk += "pylontech_soc_percent{module=\"" + String(m) + "\"} " + String(stack.modules[m].power.socPercent) + "\n";
            }
        }
        chunk += "\n";

        // --- State of Health (SOH) ---
        chunk += "# HELP pylontech_soh_percent Module State of Health percent\n";
        chunk += "# TYPE pylontech_soh_percent gauge\n";
        for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
            if (stack.modules[m].present && stack.modules[m].stats.valid && stack.modules[m].stats.sohPercent > 0) {
                chunk += "pylontech_soh_percent{module=\"" + String(m) + "\"} " + String(stack.modules[m].stats.sohPercent) + "\n";
            }
        }
        chunk += "\n";
        flushChunk();

        // --- Cycle Times ---
        chunk += "# HELP pylontech_cycle_times_total Total charge/discharge cycle count\n";
        chunk += "# TYPE pylontech_cycle_times_total counter\n";
        for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
            if (stack.modules[m].present && (stack.modules[m].stats.valid || stack.modules[m].euro.valid)) {
                chunk += "pylontech_cycle_times_total{module=\"" + String(m) + "\"} " + String(getEffectiveCycles(stack.modules[m])) + "\n";
            }
        }
        chunk += "\n";

        // --- SOH Times ---
        chunk += "# HELP pylontech_soh_times_total Total SOH event times counter\n";
        chunk += "# TYPE pylontech_soh_times_total counter\n";
        for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
            if (stack.modules[m].present && stack.modules[m].stats.valid) {
                chunk += "pylontech_soh_times_total{module=\"" + String(m) + "\"} " + String(stack.modules[m].stats.sohTimes) + "\n";
            }
        }
        chunk += "\n";

        // --- Discharged Capacity ---
        chunk += "# HELP pylontech_discharged_capacity_mah_total Discharged capacity in mAh\n";
        chunk += "# TYPE pylontech_discharged_capacity_mah_total counter\n";
        for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
            if (stack.modules[m].present && stack.modules[m].stats.valid && stack.modules[m].stats.dischargedCapMah > 0) {
                uint64_t dsgMah = (uint64_t)(getDischargedCapAh(stack.modules[m].stats, stack.model) * 1000.0f);
                chunk += "pylontech_discharged_capacity_mah_total{module=\"" + String(m) + "\"} " + String(dsgMah) + "\n";
            }
        }
        chunk += "\n";
        flushChunk();

        // --- Individual Cell Voltages & Balances ---
        chunk += "# HELP pylontech_cell_voltage_millivolts Individual cell voltage in millivolts\n";
        chunk += "# TYPE pylontech_cell_voltage_millivolts gauge\n";
        for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
            const BatteryModule &mod = stack.modules[m];
            if (!mod.present || mod.cellCountParsed == 0) continue;

            for (uint8_t c = 0; c < mod.cellCountParsed; ++c) {
                chunk += "pylontech_cell_voltage_millivolts{module=\"" + String(m) + "\",cell=\"" + String(c) + "\"} " + String(mod.cells[c].voltMv) + "\n";
                if (chunk.length() > 800) flushChunk();
            }
        }
        chunk += "\n";
        flushChunk();

        chunk += "# HELP pylontech_cell_balance Active cell balancing status\n";
        chunk += "# TYPE pylontech_cell_balance gauge\n";
        for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
            const BatteryModule &mod = stack.modules[m];
            if (!mod.present || mod.cellCountParsed == 0) continue;

            for (uint8_t c = 0; c < mod.cellCountParsed; ++c) {
                chunk += "pylontech_cell_balance{module=\"" + String(m) + "\",cell=\"" + String(c) + "\"} " + String(mod.cells[c].balance ? 1 : 0) + "\n";
                if (chunk.length() > 800) flushChunk();
            }
        }
        chunk += "\n";
        flushChunk();

        // --- Individual Cell SOH Count (Model C) ---
        bool hasCellSoh = false;
        for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
            if (stack.modules[m].present && stack.modules[m].cells[0].sohValid) {
                hasCellSoh = true;
                break;
            }
        }
        if (hasCellSoh) {
            chunk += "# HELP pylontech_cell_soh_count Individual cell SOH degradation count\n";
            chunk += "# TYPE pylontech_cell_soh_count gauge\n";
            for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
                const BatteryModule &mod = stack.modules[m];
                if (!mod.present) continue;
                for (uint8_t c = 0; c < mod.cellCountParsed; ++c) {
                    if (mod.cells[c].sohValid) {
                        chunk += "pylontech_cell_soh_count{module=\"" + String(m) + "\",cell=\"" + String(c) + "\"} " + String(mod.cells[c].sohCount) + "\n";
                        if (chunk.length() > 800) flushChunk();
                    }
                }
            }
            chunk += "\n";
            flushChunk();
        }

        // --- Euro Statistics (Model D) ---
        bool hasEuro = false;
        for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
            if (stack.modules[m].present && stack.modules[m].euro.valid) {
                hasEuro = true;
                break;
            }
        }

        if (hasEuro) {
            chunk += "# HELP pylontech_euro_energy_throughput_watthours_total Total energy throughput in Wh\n";
            chunk += "# TYPE pylontech_euro_energy_throughput_watthours_total counter\n";
            for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
                if (stack.modules[m].present && stack.modules[m].euro.valid) {
                    chunk += "pylontech_euro_energy_throughput_watthours_total{module=\"" + String(m) + "\"} " + String((uint32_t)stack.modules[m].euro.energyThroughputWh) + "\n";
                }
            }
            chunk += "\n";

            chunk += "# HELP pylontech_euro_capacity_throughput_amperehours_total Total capacity throughput in Ah\n";
            chunk += "# TYPE pylontech_euro_capacity_throughput_amperehours_total counter\n";
            for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
                if (stack.modules[m].present && stack.modules[m].euro.valid) {
                    chunk += "pylontech_euro_capacity_throughput_amperehours_total{module=\"" + String(m) + "\"} " + String((uint32_t)stack.modules[m].euro.capacityThroughputAh) + "\n";
                }
            }
            chunk += "\n";

            chunk += "# HELP pylontech_euro_round_trip_efficiency Round trip efficiency (basis 10000 = 100%)\n";
            chunk += "# TYPE pylontech_euro_round_trip_efficiency gauge\n";
            for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
                if (stack.modules[m].present && stack.modules[m].euro.valid) {
                    chunk += "pylontech_euro_round_trip_efficiency{module=\"" + String(m) + "\"} " + String(stack.modules[m].euro.roundTripEff) + "\n";
                }
            }
            chunk += "\n";
            flushChunk();
        }

        // --- Error & Alarm Counters ---
        chunk += "# HELP pylontech_alarm_events_total Cumulative alarm and protection event triggers\n";
        chunk += "# TYPE pylontech_alarm_events_total counter\n";
        for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
            const BatteryModule &mod = stack.modules[m];
            if (!mod.present || !mod.stats.valid) continue;

            chunk += "pylontech_alarm_events_total{module=\"" + String(m) + "\",event=\"coc\"} " + String(mod.stats.cocTimes) + "\n";
            chunk += "pylontech_alarm_events_total{module=\"" + String(m) + "\",event=\"doc\"} " + String(mod.stats.docTimes) + "\n";
            chunk += "pylontech_alarm_events_total{module=\"" + String(m) + "\",event=\"sc\"} " + String(mod.stats.scTimes) + "\n";
            chunk += "pylontech_alarm_events_total{module=\"" + String(m) + "\",event=\"rv\"} " + String(mod.stats.rvTimes) + "\n";
            chunk += "pylontech_alarm_events_total{module=\"" + String(m) + "\",event=\"bat_ov\"} " + String(mod.stats.batOvTimes) + "\n";
            chunk += "pylontech_alarm_events_total{module=\"" + String(m) + "\",event=\"bat_lv\"} " + String(mod.stats.batLvTimes) + "\n";
            chunk += "pylontech_alarm_events_total{module=\"" + String(m) + "\",event=\"bat_uv\"} " + String(mod.stats.batUvTimes) + "\n";
            chunk += "pylontech_alarm_events_total{module=\"" + String(m) + "\",event=\"pwr_lv\"} " + String(mod.stats.pwrLvTimes) + "\n";
            chunk += "pylontech_alarm_events_total{module=\"" + String(m) + "\",event=\"bmicerr\"} " + String(mod.stats.bmicErrTimes) + "\n";
            chunk += "pylontech_alarm_events_total{module=\"" + String(m) + "\",event=\"cot\"} " + String(mod.stats.cotTimes) + "\n";
            chunk += "pylontech_alarm_events_total{module=\"" + String(m) + "\",event=\"dot\"} " + String(mod.stats.dotTimes) + "\n";
            if (chunk.length() > 800) flushChunk();
        }
        chunk += "\n";
        // --- ESP32 System Diagnostics & Telemetry ---
        SystemStats sys = SystemMonitor::getSnapshot();

        chunk += "# HELP esp32_uptime_seconds Device uptime in seconds\n";
        chunk += "# TYPE esp32_uptime_seconds counter\n";
        chunk += "esp32_uptime_seconds " + String(sys.uptimeSec) + "\n\n";

        chunk += "# HELP esp32_cpu_usage_percent CPU utilization percentage (0-100)\n";
        chunk += "# TYPE esp32_cpu_usage_percent gauge\n";
        chunk += "esp32_cpu_usage_percent " + String(sys.cpuLoadPct, 1) + "\n\n";

        chunk += "# HELP esp32_cpu_temperature_celsius Internal chip temperature in degrees Celsius\n";
        chunk += "# TYPE esp32_cpu_temperature_celsius gauge\n";
        chunk += "esp32_cpu_temperature_celsius " + String(sys.cpuTempC, 1) + "\n\n";

        chunk += "# HELP esp32_cpu_frequency_mhz Current CPU clock frequency in MHz\n";
        chunk += "# TYPE esp32_cpu_frequency_mhz gauge\n";
        chunk += "esp32_cpu_frequency_mhz " + String(sys.cpuFreqMHz) + "\n\n";

        chunk += "# HELP esp32_reset_reason Last hardware/software reset reason\n";
        chunk += "# TYPE esp32_reset_reason gauge\n";
        chunk += "esp32_reset_reason{code=\"" + String(sys.resetReasonCode) + "\",reason=\"" + sys.resetReason + "\"} 1\n\n";

        chunk += "# HELP esp32_heap_free_bytes Current free heap memory in bytes\n";
        chunk += "# TYPE esp32_heap_free_bytes gauge\n";
        chunk += "esp32_heap_free_bytes " + String(sys.heapFree) + "\n\n";

        chunk += "# HELP esp32_heap_total_bytes Total heap memory in bytes\n";
        chunk += "# TYPE esp32_heap_total_bytes gauge\n";
        chunk += "esp32_heap_total_bytes " + String(sys.heapTotal) + "\n\n";

        chunk += "# HELP esp32_heap_min_free_bytes Lowest free heap recorded since boot\n";
        chunk += "# TYPE esp32_heap_min_free_bytes gauge\n";
        chunk += "esp32_heap_min_free_bytes " + String(sys.heapMinFree) + "\n\n";

        chunk += "# HELP esp32_heap_max_alloc_bytes Largest contiguous free block on heap\n";
        chunk += "# TYPE esp32_heap_max_alloc_bytes gauge\n";
        chunk += "esp32_heap_max_alloc_bytes " + String(sys.heapMaxAlloc) + "\n\n";

        chunk += "# HELP esp32_heap_fragmentation_percent Heap fragmentation percentage (0-100)\n";
        chunk += "# TYPE esp32_heap_fragmentation_percent gauge\n";
        chunk += "esp32_heap_fragmentation_percent " + String(sys.heapFragPct) + "\n\n";

        if (sys.psramTotal > 0) {
            chunk += "# HELP esp32_psram_total_bytes Total PSRAM memory in bytes\n";
            chunk += "# TYPE esp32_psram_total_bytes gauge\n";
            chunk += "esp32_psram_total_bytes " + String(sys.psramTotal) + "\n\n";

            chunk += "# HELP esp32_psram_free_bytes Free PSRAM memory in bytes\n";
            chunk += "# TYPE esp32_psram_free_bytes gauge\n";
            chunk += "esp32_psram_free_bytes " + String(sys.psramFree) + "\n\n";
            flushChunk();
        }

        chunk += "# HELP esp32_wifi_rssi_dbm WiFi signal strength in dBm\n";
        chunk += "# TYPE esp32_wifi_rssi_dbm gauge\n";
        chunk += "esp32_wifi_rssi_dbm " + String(sys.wifiRssi) + "\n\n";

        chunk += "# HELP esp32_wifi_signal_percent WiFi signal quality percentage (0-100)\n";
        chunk += "# TYPE esp32_wifi_signal_percent gauge\n";
        chunk += "esp32_wifi_signal_percent " + String(sys.wifiSignalPct) + "\n\n";

        chunk += "# HELP esp32_wifi_connected WiFi station connection state (1 = connected, 0 = disconnected)\n";
        chunk += "# TYPE esp32_wifi_connected gauge\n";
        chunk += "esp32_wifi_connected " + String((WiFi.status() == WL_CONNECTED) ? 1 : 0) + "\n\n";

        chunk += "# HELP esp32_wifi_ap_active Access Point state (1 = active, 0 = inactive)\n";
        chunk += "# TYPE esp32_wifi_ap_active gauge\n";
        chunk += "esp32_wifi_ap_active " + String((WiFi.getMode() & WIFI_AP) ? 1 : 0) + "\n\n";

        chunk += "# HELP esp32_wifi_ap_clients Number of connected wireless clients to Access Point\n";
        chunk += "# TYPE esp32_wifi_ap_clients gauge\n";
        chunk += "esp32_wifi_ap_clients " + String(WiFi.softAPgetStationNum()) + "\n\n";

        chunk += "# HELP esp32_wifi_channel WiFi channel\n";
        chunk += "# TYPE esp32_wifi_channel gauge\n";
        chunk += "esp32_wifi_channel " + String(sys.wifiChannel) + "\n\n";

        chunk += "# HELP esp32_flash_size_bytes Total flash chip size in bytes\n";
        chunk += "# TYPE esp32_flash_size_bytes gauge\n";
        chunk += "esp32_flash_size_bytes " + String(sys.flashSize) + "\n\n";

        chunk += "# HELP esp32_sketch_size_bytes Flash space consumed by application firmware\n";
        chunk += "# TYPE esp32_sketch_size_bytes gauge\n";
        chunk += "esp32_sketch_size_bytes " + String(sys.sketchSize) + "\n\n";

        chunk += "# HELP esp32_sketch_free_bytes Free flash space available for OTA updates\n";
        chunk += "# TYPE esp32_sketch_free_bytes gauge\n";
        chunk += "esp32_sketch_free_bytes " + String(sys.sketchFree) + "\n\n";

        extern time_t lastNtpSyncTimestamp;
        bool isSynced = (time(nullptr) > 1577836800);
        chunk += "# HELP esp32_ntp_synced Whether network time synchronization is active (1 = synced, 0 = uncalibrated)\n";
        chunk += "# TYPE esp32_ntp_synced gauge\n";
        chunk += "esp32_ntp_synced " + String(isSynced ? 1 : 0) + "\n\n";

        chunk += "# HELP esp32_ntp_last_sync_timestamp Timestamp of last successful SNTP synchronization\n";
        chunk += "# TYPE esp32_ntp_last_sync_timestamp gauge\n";
        chunk += "esp32_ntp_last_sync_timestamp " + String((long)lastNtpSyncTimestamp) + "\n\n";

        chunk += "# HELP esp32_system_info Device system metadata\n";
        chunk += "# TYPE esp32_system_info gauge\n";
        chunk += "esp32_system_info{chip=\"" + sys.chipModel + "\",revision=\"" + String(sys.chipRevision) + "\",cores=\"" + String(sys.cpuCores) + "\",reset_reason=\"" + sys.resetReason + "\",ssid=\"" + sys.wifiSsid + "\",ip=\"" + sys.wifiIp + "\",mac=\"" + sys.wifiMac + "\"} 1\n\n";
        flushChunk();

        // End of stream
        server.sendContent("");
    }
};
