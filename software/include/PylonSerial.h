#pragma once
#include <Arduino.h>
#include <queue>
#include <Preferences.h>
#include "Config.h"
#include "BatteryData.h"
#include "PylonParser.h"
#include "ConsoleLog.h"

extern void yieldSystemTasks();

class PylonSerialManager {
private:
    HardwareSerial *port = nullptr;
    uint8_t txPin = PIN_UART_TX;
    uint8_t rxPin = PIN_UART_RX;
    std::queue<String> userCmdQueue;
    bool portBusy = false;

    void setLedSerial(bool on) {
        digitalWrite(PIN_LED_SERIAL, on ? LED_ACTIVE_LEVEL : !LED_ACTIVE_LEVEL);
    }

public:
    uint8_t getTxPin() const { return txPin; }
    uint8_t getRxPin() const { return rxPin; }
    bool isBusy() const { return portBusy; }

    void enqueueUserCommand(const String &cmd) {
        if (userCmdQueue.size() < 16) {
            userCmdQueue.push(cmd);
        } else {
            consoleLog.logError("Command queue full, dropped: " + cmd);
        }
    }

    void processQueue() {
        if (portBusy || userCmdQueue.empty()) return;
        String cmd = userCmdQueue.front();
        userCmdQueue.pop();
        sendCommand(cmd);
    }

    void begin() {
        pinMode(PIN_LED_SERIAL, OUTPUT);
        setLedSerial(false);

#if defined(CONFIG_IDF_TARGET_ESP32S2) || (SOC_UART_NUM < 3)
        // ESP32-S2 has 2 hardware UARTs (Serial0 / Serial1). Route Serial1 to pins via GPIO matrix.
        port = &Serial1;
        port->setRxBufferSize(SERIAL_RX_BUFFER_SIZE);
        port->begin(SERIAL_BAUD_RATE, SERIAL_8N1, rxPin, txPin);
        gpio_pullup_en((gpio_num_t)rxPin);
        gpio_pulldown_dis((gpio_num_t)rxPin);
#else
        if (txPin == 1 && rxPin == 3) {
            port = &Serial;
            port->setRxBufferSize(SERIAL_RX_BUFFER_SIZE);
            port->begin(SERIAL_BAUD_RATE);
            gpio_pullup_en(GPIO_NUM_3);
            gpio_pulldown_dis(GPIO_NUM_3);
        } else {
            port = &Serial2;
            port->setRxBufferSize(SERIAL_RX_BUFFER_SIZE);
            port->begin(SERIAL_BAUD_RATE, SERIAL_8N1, rxPin, txPin);
            gpio_pullup_en((gpio_num_t)rxPin);
            gpio_pulldown_dis((gpio_num_t)rxPin);
        }
#endif

        // Wake up BMS & flush any bootloader debris
        delay(40);
        port->print("\r\n");
        port->flush();
        delay(80);
        while (port->available()) {
            port->read();
        }

        consoleLog.logInfo("Serial console configured on TX=" + String(txPin) + ", RX=" + String(rxPin) + " at " + String(SERIAL_BAUD_RATE) + " baud");
    }

    String sendCommand(const String &cmd, uint32_t timeoutMs = SERIAL_TIMEOUT_MS) {
        if (!port) {
            consoleLog.logError("Serial port not initialized");
            return "";
        }

        portBusy = true;

        // Flush stale input
        while (port->available()) {
            port->read();
        }

        setLedSerial(true);
        consoleLog.logTx(cmd);

        // Pylontech CLI requires Carriage Return (\r\n or \r) to execute commands
        port->print(cmd);
        port->print("\r\n");
        port->flush();

        String response;
        response.reserve(2048);

        uint32_t startTime = millis();
        uint32_t lastByteTime = millis();
        size_t bytesRead = 0;

        while (millis() - startTime < timeoutMs) {
            yieldSystemTasks();

            if (port->available()) {
                char c = port->read();
                response += c;
                bytesRead++;
                lastByteTime = millis();

                // Pylontech completion markers:
                if (response.endsWith("$$\n") || response.endsWith("$$\r\n") || 
                    response.endsWith("$$\r") || response.endsWith("$$") ||
                    response.endsWith("pylon>") || response.endsWith("pylon> ")) {
                    break;
                }
            } else {
                // If 0 bytes received after 1200ms, abort early (battery disconnected / not responding)
                if (bytesRead == 0 && (millis() - startTime > 1200)) {
                    break;
                }
                if (response.length() > 30 && (millis() - lastByteTime > 400)) {
                    break;
                }
                delay(2);
            }
        }

        setLedSerial(false);

        if (response.length() > 0) {
            consoleLog.logRx(response);
        } else {
            consoleLog.logError("No response received for command: " + cmd + " (timeout " + String(timeoutMs) + "ms, 0 bytes read on TX=" + String(txPin) + ", RX=" + String(rxPin) + ")");
        }

        uint32_t dStart = millis();
        while (millis() - dStart < SERIAL_COMMAND_DELAY_MS) {
            yieldSystemTasks();
            delay(5);
        }

        portBusy = false;
        return response;
    }

    bool pollStack(BatteryStack &stack, bool fullSlowPoll = false) {
        uint32_t startTime = millis();
        consoleLog.logInfo(String("--- Starting ") + (fullSlowPoll ? "full (fast+slow)" : "fast") + " poll cycle ---");

        auto drainUserQueue = [this]() {
            while (!userCmdQueue.empty()) {
                String uCmd = userCmdQueue.front();
                userCmdQueue.pop();
                sendCommand(uCmd);
            }
        };

        // Drain any pending user commands before starting
        drainUserQueue();

        // Step 1: Detect Model if unknown
        if (stack.model == MODEL_UNKNOWN) {
            String respInfo = sendCommand("info");
            if (respInfo.length() > 0) {
                PylonParser::parseInfo(respInfo, stack, 0);
                consoleLog.logInfo("Detected battery model: " + String(stack.modelName));
            } else {
                consoleLog.logError("No response on 'info' command");
            }
            drainUserQueue();
        }

        // Step 2: Read Power table (pwr) -> dynamic values: V, I, SOC, Status (ALWAYS)
        String respPwr = sendCommand("pwr");
        if (respPwr.length() < 10) {
            stack.scrapeSuccess = false;
            consoleLog.logError("Failed to read 'pwr' table - skipping module polling");
            return false;
        }

        PylonParser::parsePwr(respPwr, stack);
        consoleLog.logInfo("Modules detected: " + String(stack.moduleCount) + (stack.isMaster ? " (Master Stack)" : " (Single Unit)"));
        drainUserQueue();

        uint8_t targetMod = stack.activeModuleIndex;
        if (targetMod < 1 || targetMod > MAX_MODULES) targetMod = 1;

        // Step 3: Dynamic cell telemetry (bat) -> cell voltages, temps, balancing (ALWAYS in fast poll)
        String respBat = sendCommand("bat");
        PylonParser::parseBat(respBat, stack, targetMod);
        drainUserQueue();

        if (stack.isMaster && stack.moduleCount > 1) {
            for (uint8_t n = 1; n <= stack.moduleCount && n <= MAX_MODULES; ++n) {
                if (!stack.modules[n].present || n == targetMod) continue;
                String rBatN = sendCommand("bat " + String(n));
                PylonParser::parseBat(rBatN, stack, n);
                drainUserQueue();
            }
        }

        // Step 4: Slow telemetry (stat, info, soh/euro) -> ONLY when fullSlowPoll is true (every 5 min)
        if (fullSlowPoll) {
            if (stack.model == MODEL_US3000C) {
                String respStat = sendCommand("stat");
                PylonParser::parseStat(respStat, stack, targetMod);
                drainUserQueue();

                String respInfo = sendCommand("info");
                PylonParser::parseInfo(respInfo, stack, targetMod);
                drainUserQueue();

                String respSoh = sendCommand("soh");
                PylonParser::parseSoh(respSoh, stack, targetMod);
                drainUserQueue();

                if (stack.isMaster && stack.moduleCount > 1) {
                    for (uint8_t n = 1; n <= stack.moduleCount && n <= MAX_MODULES; ++n) {
                        if (!stack.modules[n].present || n == targetMod) continue;

                        String rStatN = sendCommand("stat " + String(n));
                        PylonParser::parseStat(rStatN, stack, n);
                        drainUserQueue();

                        String rInfoN = sendCommand("info " + String(n));
                        PylonParser::parseInfo(rInfoN, stack, n);
                        drainUserQueue();

                        String rSohN = sendCommand("soh " + String(n));
                        PylonParser::parseSoh(rSohN, stack, n);
                        drainUserQueue();
                    }
                }
            } else {
                // Primary battery is model US3000D:
                // Commands 'stat', 'info', and 'euro' only exist on the master battery (Module 1).
                // They can ONLY be executed for the first battery (Module 1).
                String respStat = sendCommand("stat");
                PylonParser::parseStat(respStat, stack, 1);
                drainUserQueue();

                String respInfo = sendCommand("info");
                PylonParser::parseInfo(respInfo, stack, 1);
                drainUserQueue();

                String respEuro = sendCommand("euro");
                PylonParser::parseEuro(respEuro, stack, 1);
                drainUserQueue();

                // Slave modules (n > 1) connected to US3000D master do not support 'stat', 'info', or 'euro'.
                // Do NOT query them with index, and do NOT copy master's info/stats/euro to them.
            }
        }

        stack.scrapeDurationMs = millis() - startTime;
        stack.lastScrapeMillis = millis();
        stack.lastScrapeTimestamp = time(nullptr);
        stack.scrapeSuccess = true;

        consoleLog.logInfo("Poll cycle completed successfully in " + String(stack.scrapeDurationMs) + " ms");
        return true;
    }

    bool pollModuleOnDemand(BatteryStack &stack, uint8_t modIndex) {
        if (modIndex < 1 || modIndex > MAX_MODULES) return false;
        consoleLog.logInfo("Executing on-demand refresh for Module #" + String(modIndex));

        String modArg = (modIndex == stack.activeModuleIndex && !stack.isMaster) ? "" : (" " + String(modIndex));

        // 1. bat - always supports module index
        String rBat = sendCommand("bat" + modArg);
        PylonParser::parseBat(rBat, stack, modIndex);

        // 2. pwr - stack level
        String rPwr = sendCommand("pwr");
        PylonParser::parsePwr(rPwr, stack);

        // 3. stat, 4. info & 5. euro / soh
        if (stack.model == MODEL_US3000D) {
            // US3000D: commands 'stat', 'info', and 'euro' can ONLY be executed for Module 1
            if (modIndex == 1) {
                String rStat = sendCommand("stat");
                PylonParser::parseStat(rStat, stack, 1);

                String rInfo = sendCommand("info");
                PylonParser::parseInfo(rInfo, stack, 1);

                String rEuro = sendCommand("euro");
                PylonParser::parseEuro(rEuro, stack, 1);
            }
        } else {
            String rStat = sendCommand("stat" + modArg);
            PylonParser::parseStat(rStat, stack, modIndex);

            String rInfo = sendCommand("info" + modArg);
            PylonParser::parseInfo(rInfo, stack, modIndex);

            String rSoh = sendCommand("soh" + modArg);
            PylonParser::parseSoh(rSoh, stack, modIndex);
        }

        consoleLog.logInfo("On-demand refresh finished for Module #" + String(modIndex));
        return true;
    }
};
