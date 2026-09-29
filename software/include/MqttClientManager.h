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

    // Returns the HA device JSON fragment (shared across all sensors)
    String deviceJson(const BatteryStack &stack) {
        return "\"dev\":{\"ids\":[\"pylon_smart_monitor\"],\"name\":\"Pylon Smart Monitor\","
               "\"mf\":\"Pylontech\",\"mdl\":\"" + String(stack.modelName) + "\","
               "\"sw\":\"" + String(FIRMWARE_VERSION) + "\"}";
    }

    // Publishes a single HA MQTT discovery config for a sensor
    void pubSensor(const String &uniqueId, const String &name,
                   const String &stateTopic, const String &valTpl,
                   const String &devClass, const String &unit,
                   const String &icon, const String &devJson) {
        String topic = "homeassistant/sensor/pylontech_" + uniqueId + "/config";
        String payload = "{\"name\":\"" + name +
                         "\",\"stat_t\":\"" + stateTopic +
                         "\",\"val_tpl\":\"" + valTpl +
                         "\",\"uniq_id\":\"pylon_" + uniqueId + "\"";
        if (devClass.length() > 0) payload += ",\"dev_cla\":\"" + devClass + "\"";
        if (unit.length() > 0)     payload += ",\"unit_of_meas\":\"" + unit + "\"";
        if (icon.length() > 0)     payload += ",\"icon\":\"" + icon + "\"";
        payload += "," + devJson + "}";
        mqtt.publish(topic.c_str(), payload.c_str(), true);
    }

    // Overload without icon
    void pubSensor(const String &uniqueId, const String &name,
                   const String &stateTopic, const String &valTpl,
                   const String &devClass, const String &unit,
                   const String &devJson) {
        pubSensor(uniqueId, name, stateTopic, valTpl, devClass, unit, "", devJson);
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
        String stackTopic = topicPrefix + "/state";

        // ── Stack-level sensors (single shared state topic) ───────────────────
        pubSensor("voltage",  "Stack Voltage",       stackTopic, "{{ value_json.voltage }}", "voltage",  "V",  dJson);
        pubSensor("current",  "Stack Current",       stackTopic, "{{ value_json.current }}", "current",  "A",  dJson);
        pubSensor("power",    "Stack Power",         stackTopic, "{{ value_json.power }}",   "power",    "W",  dJson);
        pubSensor("soc",      "Stack SOC",           stackTopic, "{{ value_json.soc }}",     "battery",  "%",  dJson);
        pubSensor("soh",      "Stack SOH",           stackTopic, "{{ value_json.soh }}",     "",         "%",  "mdi:heart-pulse", dJson);
        pubSensor("modules",  "Active Modules",      stackTopic, "{{ value_json.modules }}", "",         "",   "mdi:battery-heart-variant", dJson);

        // ── Per-module sensors (each module has its own state topic) ──────────
        for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
            if (!stack.modules[m].present) continue;
            String sm = String(m);
            String modTopic = topicPrefix + "/mod" + sm + "/state";
            bool hasMaster = stack.isMaster && (m == stack.activeModuleIndex);

            // Numeric measurement sensors
            pubSensor("mod" + sm + "_voltage",      "Module " + sm + " Voltage",
                      modTopic, "{{ value_json.voltage }}",        "voltage",     "V",    dJson);
            pubSensor("mod" + sm + "_current",      "Module " + sm + " Current",
                      modTopic, "{{ value_json.current }}",        "current",     "A",    dJson);
            pubSensor("mod" + sm + "_soc",          "Module " + sm + " State of Charge",
                      modTopic, "{{ value_json.soc }}",            "battery",     "%",    dJson);
            pubSensor("mod" + sm + "_temp",         "Module " + sm + " Temperature",
                      modTopic, "{{ value_json.temp }}",           "temperature", "°C",   dJson);
            pubSensor("mod" + sm + "_mos_temp",     "Module " + sm + " MOSFET Temperature",
                      modTopic, "{{ value_json.mos_temp }}",       "temperature", "°C",   dJson);
            pubSensor("mod" + sm + "_cell_high_v",  "Module " + sm + " Cell High Voltage",
                      modTopic, "{{ value_json.cell_high_v }}",    "voltage",     "V",    dJson);
            pubSensor("mod" + sm + "_cell_low_v",   "Module " + sm + " Cell Low Voltage",
                      modTopic, "{{ value_json.cell_low_v }}",     "voltage",     "V",    dJson);
            pubSensor("mod" + sm + "_cell_high_t",  "Module " + sm + " Cell High Temperature",
                      modTopic, "{{ value_json.cell_high_t }}",    "temperature", "°C",   dJson);
            pubSensor("mod" + sm + "_cell_low_t",   "Module " + sm + " Cell Low Temperature",
                      modTopic, "{{ value_json.cell_low_t }}",     "temperature", "°C",   dJson);
            pubSensor("mod" + sm + "_vspread",      "Module " + sm + " Volt Spread",
                      modTopic, "{{ value_json.vspread }}",        "",            "mV",   "mdi:swap-vertical", dJson);

            // Status string sensors (enum)
            pubSensor("mod" + sm + "_status",       "Module " + sm + " Status",
                      modTopic, "{{ value_json.status }}",         "",            "",     "mdi:information-outline", dJson);
            pubSensor("mod" + sm + "_volt_status",  "Module " + sm + " Battery Voltage Status",
                      modTopic, "{{ value_json.volt_status }}",    "",            "",     "mdi:lightning-bolt", dJson);
            pubSensor("mod" + sm + "_curr_status",  "Module " + sm + " Current Status",
                      modTopic, "{{ value_json.curr_status }}",    "",            "",     "mdi:current-dc", dJson);
            pubSensor("mod" + sm + "_temp_status",  "Module " + sm + " Temperature Status",
                      modTopic, "{{ value_json.temp_status }}",    "",            "",     "mdi:thermometer", dJson);
            pubSensor("mod" + sm + "_bat_t_status", "Module " + sm + " Battery Temperature Status",
                      modTopic, "{{ value_json.bat_t_status }}",   "",            "",     "mdi:thermometer-alert", dJson);
            pubSensor("mod" + sm + "_mos_status",   "Module " + sm + " MOSFET Temperature Status",
                      modTopic, "{{ value_json.mos_status }}",     "",            "",     "mdi:chip", dJson);

            // SOH per module (all modules report it if stats are valid)
            pubSensor("mod" + sm + "_soh",          "Module " + sm + " State of Health",
                      modTopic, "{{ value_json.soh }}",            "",            "%",    "mdi:heart-pulse", dJson);

            // Capacity & Energy Throughput — only master module
            if (hasMaster) {
                pubSensor("mod" + sm + "_cap_ah",   "Module " + sm + " Capacity Throughput",
                          modTopic, "{{ value_json.cap_ah }}",     "energy_storage", "Ah", "mdi:battery-arrow-up", dJson);
                pubSensor("mod" + sm + "_energy_wh","Module " + sm + " Energy Throughput",
                          modTopic, "{{ value_json.energy_wh }}", "energy",       "Wh",  "mdi:flash", dJson);
            }
        }

        discoveryPublished = true;
        consoleLog.logInfo("Home Assistant MQTT discovery published");
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

        String stackTopic = topicPrefix + "/state";
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
                if (!first) json += ","; first = false;
                json += "\""; json += key; json += "\":"; json += String(val, dec);
            };
            auto addI = [&](const char *key, long val) {
                if (!first) json += ","; first = false;
                json += "\""; json += key; json += "\":"; json += String(val);
            };
            auto addS = [&](const char *key, const char *val) {
                if (!first) json += ","; first = false;
                json += "\""; json += key; json += "\":\""; json += val; json += "\"";
            };
            auto addNull = [&](const char *key) {
                if (!first) json += ","; first = false;
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

                // Cell high/low voltage (V)
                addF("cell_high_v", mod.power.voltHighMv / 1000.0f, 3);
                addF("cell_low_v",  mod.power.voltLowMv  / 1000.0f, 3);

                // Cell high/low temperature (°C)
                addF("cell_high_t", mod.power.tempHighMdeg / 1000.0f, 1);
                addF("cell_low_t",  mod.power.tempLowMdeg  / 1000.0f, 1);

                // Volt spread (mV) — prefer parsed direct values, fallback to cell array
                int vSpread = (mod.power.voltHighMv > mod.power.voltLowMv)
                              ? (mod.power.voltHighMv - mod.power.voltLowMv) : 0;
                if (vSpread == 0 && mod.cellCountParsed > 0) {
                    uint16_t vMin = 65535, vMax = 0;
                    for (uint8_t c = 0; c < mod.cellCountParsed; ++c) {
                        if (mod.cells[c].voltMv > 0) {
                            if (mod.cells[c].voltMv < vMin) vMin = mod.cells[c].voltMv;
                            if (mod.cells[c].voltMv > vMax) vMax = mod.cells[c].voltMv;
                        }
                    }
                    if (vMax >= vMin && vMin > 0) vSpread = vMax - vMin;
                }
                addI("vspread", vSpread);

                // Status strings
                addS("status",      mod.power.baseState[0]    ? mod.power.baseState    : "Unknown");
                addS("volt_status", mod.power.voltState[0]    ? mod.power.voltState    : "Unknown");
                addS("curr_status", mod.power.currState[0]    ? mod.power.currState    : "Unknown");
                addS("temp_status", mod.power.tempState[0]    ? mod.power.tempState    : "Unknown");
                // Battery Temperature Status: same as Temperature Status for Pylontech protocol
                addS("bat_t_status",mod.power.tempState[0]    ? mod.power.tempState    : "Unknown");
                addS("mos_status",  mod.power.mosTempState[0] ? mod.power.mosTempState : "Unknown");
            }

            // SOH (all modules, when stats are available)
            if (mod.stats.valid && mod.stats.sohPercent > 0)
                addI("soh", mod.stats.sohPercent);
            else
                addNull("soh");

            // Capacity & Energy Throughput — master module only
            if (hasMaster && mod.euro.valid) {
                addI("cap_ah",    (long)mod.euro.capacityThroughputAh);
                addI("energy_wh", (long)mod.euro.energyThroughputWh);
            }

            json += "}";

            String modTopic = topicPrefix + "/mod" + sm + "/state";
            mqtt.publish(modTopic.c_str(), json.c_str(), false);
        }
    }
};
