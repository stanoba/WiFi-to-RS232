#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include "Config.h"
#include "BatteryData.h"
#include "ConsoleLog.h"

class MqttClientManager {
private:
    WiFiClient espClient;
    PubSubClient mqtt;
    Preferences &prefs;
    bool enabled = false;
    String serverIp;
    uint16_t port = 1883;
    String user;
    String pass;
    String topicPrefix = "homeassistant/sensor/pylontech";
    uint32_t lastReconnectAttempt = 0;
    bool discoveryPublished = false;
    const BatteryStack *stackRef = nullptr;  // set on first publishState(); used by loop() to publish discovery right after reconnect

    // Returns unique 4-character hex suffix from MAC address (e.g. "b016")
    String getDeviceSuffix() const {
        String mac = WiFi.macAddress();
        mac.replace(":", "");
        if (mac.length() >= 4) {
            String suf = mac.substring(mac.length() - 4);
            suf.toLowerCase();
            return suf;
        }
        return "0000";
    }

    // Returns effective base state topic prefix (e.g. "homeassistant/sensor/pylontech_b016")
    String getEffectiveTopicPrefix() const {
        String suf = getDeviceSuffix();
        if (topicPrefix == "homeassistant/sensor/pylontech") {
            return "homeassistant/sensor/pylontech_" + suf;
        }
        return topicPrefix;
    }

    // Returns the HA device JSON fragment with unique device ID, device name, and configuration URL
    String deviceJson(const BatteryStack &stack) {
        String suf = getDeviceSuffix();
        String devName = "Pylon Smart Monitor (" + suf + ")";

        String json = "\"dev\":{\"ids\":[\"pylon_smart_" + suf + "\"],"
                      "\"name\":\"" + devName + "\","
                      "\"mf\":\"Pylontech\","
                      "\"mdl\":\"" + String(stack.modelName) + "\","
                      "\"sw\":\"" + String(FIRMWARE_VERSION) + "\"";
        if (WiFi.status() == WL_CONNECTED) {
            String ip = WiFi.localIP().toString();
            if (ip.length() > 0 && ip != "0.0.0.0") {
                json += ",\"cu\":\"http://" + ip + "/\"";
            }
        }
        json += "}";
        return json;
    }

    // Publishes a single HA MQTT discovery config for a sensor
    void pubSensor(const String &sensorKey, const String &name,
                   const String &stateTopic, const String &valTpl,
                   const String &devClass, const String &unit,
                   const String &icon, const String &devJson,
                   int8_t precision = -1, const String &stateClass = "") {
        String suf = getDeviceSuffix();
        String topic = "homeassistant/sensor/pylontech_" + suf + "_" + sensorKey + "/config";
        String payload = "{\"name\":\"" + name +
                         "\",\"stat_t\":\"" + stateTopic +
                         "\",\"val_tpl\":\"" + valTpl +
                         "\",\"uniq_id\":\"pylon_" + suf + "_" + sensorKey + "\"";
        if (devClass.length() > 0)   payload += ",\"dev_cla\":\"" + devClass + "\"";
        if (unit.length() > 0)       payload += ",\"unit_of_meas\":\"" + unit + "\"";
        if (icon.length() > 0)       payload += ",\"icon\":\"" + icon + "\"";
        if (stateClass.length() > 0) payload += ",\"stat_cla\":\"" + stateClass + "\"";
        if (precision >= 0)          payload += ",\"sug_dsp_prc\":" + String(precision);
        payload += "," + devJson + "}";
        mqtt.publish(topic.c_str(), payload.c_str(), true);
    }

    // Overload without icon
    void pubSensor(const String &sensorKey, const String &name,
                   const String &stateTopic, const String &valTpl,
                   const String &devClass, const String &unit,
                   const String &devJson,
                   int8_t precision = -1, const String &stateClass = "") {
        pubSensor(sensorKey, name, stateTopic, valTpl, devClass, unit, "", devJson, precision, stateClass);
    }

public:
    MqttClientManager(Preferences &p) : mqtt(espClient), prefs(p) {}

    void loadConfig() {
        enabled = prefs.getBool(NVS_KEY_MQTT_ENABLED, false);
        serverIp = prefs.getString(NVS_KEY_MQTT_SERVER, "");
        port = prefs.getUShort(NVS_KEY_MQTT_PORT, 1883);
        user = prefs.getString(NVS_KEY_MQTT_USER, "");
        pass = prefs.getString(NVS_KEY_MQTT_PASS, "");
        topicPrefix = prefs.getString(NVS_KEY_MQTT_PREFIX, "homeassistant/sensor/pylontech");
        if (topicPrefix.endsWith("/")) {
            topicPrefix = topicPrefix.substring(0, topicPrefix.length() - 1);
        }

        if (enabled && serverIp.length() > 0) {
            mqtt.setServer(serverIp.c_str(), port);
            mqtt.setBufferSize(2048);
            discoveryPublished = false;
        } else {
            if (mqtt.connected()) {
                mqtt.disconnect();
            }
        }
    }

    bool isEnabled() const { return enabled; }
    bool isConnected() { return enabled && mqtt.connected(); }

    void loop() {
        if (!enabled || serverIp.length() == 0 || WiFi.status() != WL_CONNECTED) {
            return;
        }

        if (!mqtt.connected()) {
            uint32_t now = millis();
            if (now - lastReconnectAttempt > 15000) {
                lastReconnectAttempt = now;
                if (reconnect() && stackRef != nullptr) {
                    // Publish discovery immediately after connect so HA registers
                    // all entities well before the first state payload arrives
                    publishDiscovery(*stackRef);
                }
            }
        } else {
            mqtt.loop();
        }
    }

    bool reconnect() {
        String clientId = "PylonSmart-" + String((uint32_t)ESP.getEfuseMac(), HEX);
        bool ok = false;
        if (user.length() > 0) {
            ok = mqtt.connect(clientId.c_str(), user.c_str(), pass.c_str());
        } else {
            ok = mqtt.connect(clientId.c_str());
        }

        if (ok) {
            consoleLog.logInfo("MQTT connected to " + serverIp + ":" + String(port));
            discoveryPublished = false;
            return true;
        }
        return false;
    }

    void publishDiscovery(const BatteryStack &stack) {
        if (!mqtt.connected() || discoveryPublished) return;

        String dJson = deviceJson(stack);
        String effPrefix = getEffectiveTopicPrefix();
        String stackTopic = effPrefix + "/state";

        // ── Stack-level sensors (single shared state topic) ───────────────────
        pubSensor("voltage", "Stack Voltage",  stackTopic, "{{ value_json.voltage }}", "voltage", "V", "", dJson, 2, "measurement");
        pubSensor("current", "Stack Current",  stackTopic, "{{ value_json.current }}", "current", "A", "", dJson, 2, "measurement");
        pubSensor("power",   "Stack Power",    stackTopic, "{{ value_json.power }}",   "power",   "W", "", dJson, 1, "measurement");
        pubSensor("soc",     "Stack SOC",      stackTopic, "{{ value_json.soc }}",     "battery", "%", "", dJson, 0, "measurement");
        pubSensor("soh",     "Stack SOH",      stackTopic, "{{ value_json.soh }}",     "",        "%", "mdi:heart-pulse", dJson, 0, "measurement");
        pubSensor("modules", "Active Modules", stackTopic, "{{ value_json.modules }}", "",        "",  "mdi:battery-heart-variant", dJson, 0, "measurement");

        // ── Per-module sensors (each module has its own state topic) ──────────
        for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
            if (!stack.modules[m].present) continue;
            String sm = String(m);
            String modTopic = effPrefix + "/mod" + sm + "/state";
            bool hasMaster = stack.isMaster && (m == stack.activeModuleIndex);

            // Numeric measurement sensors
            pubSensor("mod" + sm + "_voltage",     "Module " + sm + " Voltage",
                      modTopic, "{{ value_json.voltage }}",        "voltage",     "V",   "",                   dJson, 3, "measurement");
            pubSensor("mod" + sm + "_current",     "Module " + sm + " Current",
                      modTopic, "{{ value_json.current }}",        "current",     "A",   "",                   dJson, 2, "measurement");
            pubSensor("mod" + sm + "_soc",         "Module " + sm + " State of Charge",
                      modTopic, "{{ value_json.soc }}",            "battery",     "%",   "",                   dJson, 0, "measurement");
            pubSensor("mod" + sm + "_temp",        "Module " + sm + " Temperature",
                      modTopic, "{{ value_json.temp }}",           "temperature", "°C",  "",                   dJson, 1, "measurement");
            pubSensor("mod" + sm + "_mos_temp",    "Module " + sm + " MOSFET Temperature",
                      modTopic, "{{ value_json.mos_temp }}",       "temperature", "°C",  "",                   dJson, 1, "measurement");
            pubSensor("mod" + sm + "_cell_high_v", "Module " + sm + " Cell High Voltage",
                      modTopic, "{{ value_json.cell_high_v }}",    "voltage",     "V",   "",                   dJson, 3, "measurement");
            pubSensor("mod" + sm + "_cell_low_v",  "Module " + sm + " Cell Low Voltage",
                      modTopic, "{{ value_json.cell_low_v }}",     "voltage",     "V",   "",                   dJson, 3, "measurement");
            pubSensor("mod" + sm + "_cell_high_t", "Module " + sm + " Cell High Temperature",
                      modTopic, "{{ value_json.cell_high_t }}",    "temperature", "°C",  "",                   dJson, 1, "measurement");
            pubSensor("mod" + sm + "_cell_low_t",  "Module " + sm + " Cell Low Temperature",
                      modTopic, "{{ value_json.cell_low_t }}",     "temperature", "°C",  "",                   dJson, 1, "measurement");
            pubSensor("mod" + sm + "_vspread",     "Module " + sm + " Volt Spread",
                      modTopic, "{{ value_json.vspread }}",        "",            "mV",  "mdi:swap-vertical",  dJson, 0, "measurement");

            // Status string sensors (enum)
            pubSensor("mod" + sm + "_status",      "Module " + sm + " Status",
                      modTopic, "{{ value_json.status }}",         "",            "",    "mdi:information-outline", dJson);
            pubSensor("mod" + sm + "_volt_status", "Module " + sm + " Battery Voltage Status",
                      modTopic, "{{ value_json.volt_status }}",    "",            "",    "mdi:lightning-bolt", dJson);
            pubSensor("mod" + sm + "_curr_status", "Module " + sm + " Current Status",
                      modTopic, "{{ value_json.curr_status }}",    "",            "",    "mdi:current-dc",     dJson);
            pubSensor("mod" + sm + "_temp_status", "Module " + sm + " Temperature Status",
                      modTopic, "{{ value_json.temp_status }}",    "",            "",    "mdi:thermometer",    dJson);
            pubSensor("mod" + sm + "_bat_t_status","Module " + sm + " Battery Temperature Status",
                      modTopic, "{{ value_json.bat_t_status }}",   "",            "",    "mdi:thermometer-alert", dJson);
            pubSensor("mod" + sm + "_mos_status",  "Module " + sm + " MOSFET Temperature Status",
                      modTopic, "{{ value_json.mos_status }}",     "",            "",    "mdi:chip",           dJson);

            // SOH per module (all modules report it if stats are valid)
            pubSensor("mod" + sm + "_soh",         "Module " + sm + " State of Health",
                      modTopic, "{{ value_json.soh }}",            "",            "%",   "mdi:heart-pulse",    dJson, 0, "measurement");

            // Capacity & Energy Throughput — only master module
            if (hasMaster) {
                pubSensor("mod" + sm + "_cap_ah",   "Module " + sm + " Capacity Throughput",
                          modTopic, "{{ value_json.cap_ah }}",     "energy_storage", "Ah", "mdi:battery-arrow-up", dJson, 0, "total_increasing");
                pubSensor("mod" + sm + "_energy_wh","Module " + sm + " Energy Throughput",
                          modTopic, "{{ value_json.energy_wh }}", "energy",       "Wh",  "mdi:flash",          dJson, 0, "total_increasing");
            }
        }

        discoveryPublished = true;
        consoleLog.logInfo("Home Assistant MQTT discovery published (Device: " + getDeviceHostname(prefs) + ", Suffix: " + getDeviceSuffix() + ")");
    }

    void publishState(const BatteryStack &stack) {
        if (!enabled || !mqtt.connected()) return;

        // Keep stackRef current so loop() can republish discovery after reconnect
        stackRef = &stack;

        if (!discoveryPublished) {
            publishDiscovery(stack);
        }

        // ── Stack-level state ─────────────────────────────────────────────────
        float stackVolt = 0.0f;
        float stackCurr = 0.0f;
        float avgSoc    = 0.0f;
        int   activeSoh = -1;
        uint8_t validPwr = 0;

        for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
            if (stack.modules[m].present && stack.modules[m].power.valid) {
                stackVolt  = stack.modules[m].power.voltMv / 1000.0f;
                stackCurr += (stack.modules[m].power.currMa / 1000.0f);
                avgSoc    += stack.modules[m].power.socPercent;
                validPwr++;
            }
            if (stack.modules[m].present && stack.modules[m].stats.valid &&
                stack.modules[m].stats.sohPercent > 0) {
                if (activeSoh < 0 || m == stack.activeModuleIndex) {
                    activeSoh = stack.modules[m].stats.sohPercent;
                }
            }
        }
        if (validPwr > 0) avgSoc /= validPwr;
        float stackPower = stackVolt * stackCurr;

        String stackJson = "{";
        stackJson += "\"voltage\":"  + String(stackVolt, 2) + ",";
        stackJson += "\"current\":"  + String(stackCurr, 2) + ",";
        stackJson += "\"power\":"    + String(stackPower, 1) + ",";
        stackJson += "\"soc\":"      + String((int)round(avgSoc)) + ",";
        stackJson += "\"soh\":"      + (activeSoh >= 0 ? String(activeSoh) : "null") + ",";
        stackJson += "\"modules\":"  + String(stack.moduleCount);
        stackJson += "}";

        String effPrefix = getEffectiveTopicPrefix();
        String stackTopic = effPrefix + "/state";
        mqtt.publish(stackTopic.c_str(), stackJson.c_str(), false);

        // ── Per-module state (one MQTT publish per module) ────────────────────
        for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
            const BatteryModule &mod = stack.modules[m];
            if (!mod.present) continue;
            String sm = String(m);
            bool hasMaster = stack.isMaster && (m == stack.activeModuleIndex);

            String json = "{";
            bool first = true;

            auto addF = [&](const char *key, float val, int dec) {
                if (!first) json += ",";
                first = false;
                json += "\""; json += key; json += "\":";
                json += String(val, dec);
            };
            auto addI = [&](const char *key, long val) {
                if (!first) json += ",";
                first = false;
                json += "\""; json += key; json += "\":";
                json += String(val);
            };
            auto addS = [&](const char *key, const char *val) {
                if (!first) json += ",";
                first = false;
                json += "\""; json += key; json += "\":\"";
                json += val; json += "\"";
            };
            auto addNull = [&](const char *key) {
                if (!first) json += ",";
                first = false;
                json += "\""; json += key; json += "\":null";
            };

            if (mod.power.valid) {
                addF("voltage",     mod.power.voltMv  / 1000.0f, 3);
                addF("current",     mod.power.currMa  / 1000.0f, 2);
                addI("soc",         mod.power.socPercent);
                addF("temp",        mod.power.tempMdeg / 1000.0f, 1);
                if (mod.power.mosTempMdeg > 0)
                    addF("mos_temp", mod.power.mosTempMdeg / 1000.0f, 1);
                else
                    addNull("mos_temp");

                // Cell high/low voltage (V) — prefer direct pwr fields, fallback to cell array
                int32_t cellHighV = mod.power.voltHighMv;
                int32_t cellLowV  = mod.power.voltLowMv;
                if ((cellHighV == 0 || cellLowV == 0) && mod.cellCountParsed > 0) {
                    uint16_t vMin = 65535, vMax = 0;
                    for (uint8_t c = 0; c < mod.cellCountParsed; ++c) {
                        if (mod.cells[c].voltMv > 0) {
                            if (mod.cells[c].voltMv < vMin) vMin = mod.cells[c].voltMv;
                            if (mod.cells[c].voltMv > vMax) vMax = mod.cells[c].voltMv;
                        }
                    }
                    if (vMin > 0 && vMax > 0) { cellHighV = vMax; cellLowV = vMin; }
                }
                if (cellHighV > 0) addF("cell_high_v", cellHighV / 1000.0f, 3);
                else addNull("cell_high_v");
                if (cellLowV > 0) addF("cell_low_v", cellLowV / 1000.0f, 3);
                else addNull("cell_low_v");

                // Cell high/low temperature (°C) — prefer direct pwr fields, fallback to cell array
                int32_t cellHighT = mod.power.tempHighMdeg;
                int32_t cellLowT  = mod.power.tempLowMdeg;
                if ((cellHighT == 0 || cellLowT == 0) && mod.cellCountParsed > 0) {
                    int32_t tMin = INT32_MAX, tMax = INT32_MIN;
                    for (uint8_t c = 0; c < mod.cellCountParsed; ++c) {
                        if (mod.cells[c].tempMdeg != 0) {
                            if (mod.cells[c].tempMdeg < tMin) tMin = mod.cells[c].tempMdeg;
                            if (mod.cells[c].tempMdeg > tMax) tMax = mod.cells[c].tempMdeg;
                        }
                    }
                    if (tMin != INT32_MAX) { cellHighT = tMax; cellLowT = tMin; }
                }
                if (cellHighT != 0) addF("cell_high_t", cellHighT / 1000.0f, 1);
                else addNull("cell_high_t");
                if (cellLowT != 0) addF("cell_low_t", cellLowT / 1000.0f, 1);
                else addNull("cell_low_t");

                // Volt spread (mV) — prefer parsed direct values, fallback to cell array
                int vSpread = (mod.power.voltHighMv > mod.power.voltLowMv)
                              ? (mod.power.voltHighMv - mod.power.voltLowMv) : 0;
                if (vSpread == 0 && cellHighV > 0 && cellLowV > 0) {
                    vSpread = (int)(cellHighV - cellLowV);
                }
                addI("vspread", vSpread);

                // Status strings
                const char *vSt  = mod.power.voltState[0]    ? mod.power.voltState    : (mod.power.batVoltState[0] ? mod.power.batVoltState : "Unknown");
                const char *tSt  = mod.power.tempState[0]    ? mod.power.tempState    : (mod.power.batTempState[0] ? mod.power.batTempState : "Unknown");
                const char *btSt = mod.power.batTempState[0] ? mod.power.batTempState : (mod.power.tempState[0]    ? mod.power.tempState    : "Unknown");

                addS("status",       mod.power.baseState[0]    ? mod.power.baseState    : "Unknown");
                addS("volt_status",  vSt);
                addS("curr_status",  mod.power.currState[0]    ? mod.power.currState    : "Unknown");
                addS("temp_status",  tSt);
                addS("bat_t_status", btSt);
                addS("mos_status",   mod.power.mosTempState[0] ? mod.power.mosTempState : "Unknown");
            }

            // SOH & Cycles (all modules, when stats are available)
            if (mod.stats.valid && mod.stats.sohPercent > 0)
                addI("soh", mod.stats.sohPercent);
            else
                addNull("soh");

            uint32_t effCycles = getEffectiveCycles(mod);
            if (effCycles > 0 || mod.stats.valid || mod.euro.valid)
                addI("cycles", (long)effCycles);
            else
                addNull("cycles");

            // Capacity & Energy Throughput — master module only
            if (hasMaster && mod.euro.valid) {
                addI("cap_ah",    (long)mod.euro.capacityThroughputAh);
                addI("energy_wh", (long)mod.euro.energyThroughputWh);
            }

            json += "}";

            String modTopic = effPrefix + "/mod" + sm + "/state";
            mqtt.publish(modTopic.c_str(), json.c_str(), false);
        }
    }
};
