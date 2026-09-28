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
                reconnect();
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

        String deviceJson = "\"dev\":{\"ids\":[\"pylon_smart_monitor\"],\"name\":\"Pylon Smart Monitor\",\"mf\":\"Pylontech\",\"mdl\":\"" + String(stack.modelName) + "\",\"sw\":\"" + String(FIRMWARE_VERSION) + "\"}";
        String stateTopic = topicPrefix + "/state";

        auto pubSensor = [&](const String &uniqueId, const String &name, const String &valTpl, const String &devClass, const String &unit, const String &icon = "") {
            String topic = "homeassistant/sensor/pylontech_" + uniqueId + "/config";
            String payload = "{\"name\":\"" + name + "\",\"stat_t\":\"" + stateTopic + "\",\"val_tpl\":\"" + valTpl + "\",\"uniq_id\":\"pylon_" + uniqueId + "\"";
            if (devClass.length() > 0) payload += ",\"dev_cla\":\"" + devClass + "\"";
            if (unit.length() > 0) payload += ",\"unit_of_meas\":\"" + unit + "\"";
            if (icon.length() > 0) payload += ",\"icon\":\"" + icon + "\"";
            payload += "," + deviceJson + "}";
            mqtt.publish(topic.c_str(), payload.c_str(), true);
        };

        // Stack-level sensors
        pubSensor("voltage", "Stack Voltage", "{{ value_json.voltage }}", "voltage", "V");
        pubSensor("current", "Stack Current", "{{ value_json.current }}", "current", "A");
        pubSensor("power", "Stack Power", "{{ value_json.power }}", "power", "W");
        pubSensor("soc", "Stack SOC", "{{ value_json.soc }}", "battery", "%");
        pubSensor("soh", "Stack SOH", "{{ value_json.soh }}", "", "%", "mdi:heart-pulse");
        pubSensor("modules", "Active Modules", "{{ value_json.modules }}", "", "", "mdi:battery-heart-variant");

        // Module-level sensors
        for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
            if (!stack.modules[m].present) continue;
            String sm = String(m);
            pubSensor("mod" + sm + "_voltage", "Module " + sm + " Voltage", "{{ value_json.mod" + sm + "_voltage }}", "voltage", "V");
            pubSensor("mod" + sm + "_current", "Module " + sm + " Current", "{{ value_json.mod" + sm + "_current }}", "current", "A");
            pubSensor("mod" + sm + "_soc", "Module " + sm + " SOC", "{{ value_json.mod" + sm + "_soc }}", "battery", "%");
            pubSensor("mod" + sm + "_temp", "Module " + sm + " Temperature", "{{ value_json.mod" + sm + "_temp }}", "temperature", "°C");
            pubSensor("mod" + sm + "_mos_temp", "Module " + sm + " MOSFET Temp", "{{ value_json.mod" + sm + "_mos_temp }}", "temperature", "°C");
            pubSensor("mod" + sm + "_vspread", "Module " + sm + " Volt Spread", "{{ value_json.mod" + sm + "_vspread }}", "", "mV", "mdi:swap-vertical");
        }

        discoveryPublished = true;
        consoleLog.logInfo("Home Assistant MQTT discovery published");
    }

    void publishState(const BatteryStack &stack) {
        if (!enabled || !mqtt.connected()) return;

        if (!discoveryPublished) {
            publishDiscovery(stack);
        }

        // Calculate stack metrics
        float stackVolt = 0.0f;
        float stackCurr = 0.0f;
        float avgSoc = 0.0f;
        int activeSoh = -1;
        uint8_t validPwr = 0;

        for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
            if (stack.modules[m].present && stack.modules[m].power.valid) {
                stackVolt = stack.modules[m].power.voltMv / 1000.0f;
                stackCurr += (stack.modules[m].power.currMa / 1000.0f);
                avgSoc += stack.modules[m].power.socPercent;
                validPwr++;
            }
            if (stack.modules[m].present && stack.modules[m].stats.valid && stack.modules[m].stats.sohPercent > 0) {
                if (activeSoh < 0 || m == stack.activeModuleIndex) {
                    activeSoh = stack.modules[m].stats.sohPercent;
                }
            }
        }
        if (validPwr > 0) avgSoc /= validPwr;
        float stackPower = stackVolt * stackCurr;

        String json = "{";
        json += "\"voltage\":" + String(stackVolt, 2) + ",";
        json += "\"current\":" + String(stackCurr, 2) + ",";
        json += "\"power\":" + String(stackPower, 1) + ",";
        json += "\"soc\":" + String((int)round(avgSoc)) + ",";
        json += "\"soh\":" + (activeSoh >= 0 ? String(activeSoh) : "null") + ",";
        json += "\"modules\":" + String(stack.moduleCount);

        for (uint8_t m = 1; m <= MAX_MODULES; ++m) {
            const BatteryModule &mod = stack.modules[m];
            if (!mod.present) continue;
            String sm = String(m);

            if (mod.power.valid) {
                json += ",\"mod" + sm + "_voltage\":" + String(mod.power.voltMv / 1000.0f, 3);
                json += ",\"mod" + sm + "_current\":" + String(mod.power.currMa / 1000.0f, 2);
                json += ",\"mod" + sm + "_soc\":" + String(mod.power.socPercent);
                json += ",\"mod" + sm + "_temp\":" + String(mod.power.tempMdeg / 1000.0f, 1);
                if (mod.power.mosTempMdeg > 0) {
                    json += ",\"mod" + sm + "_mos_temp\":" + String(mod.power.mosTempMdeg / 1000.0f, 1);
                }
                
                // Volt spread
                int vSpread = (mod.power.voltHighMv > mod.power.voltLowMv) ? (mod.power.voltHighMv - mod.power.voltLowMv) : 0;
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
                json += ",\"mod" + sm + "_vspread\":" + String(vSpread);
            }
        }

        json += "}";

        String stateTopic = topicPrefix + "/state";
        mqtt.publish(stateTopic.c_str(), json.c_str(), false);
    }
};
