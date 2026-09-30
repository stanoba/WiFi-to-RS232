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
    bool pollingActive = false;
    bool debugModeUnlocked = false;

    struct PollingActiveGuard {
        bool &flag;
        PollingActiveGuard(bool &f) : flag(f) { flag = true; }
        ~PollingActiveGuard() { flag = false; }
    };

    void setLedSerial(bool on) {
        digitalWrite(PIN_LED_SERIAL, on ? LED_ACTIVE_LEVEL : !LED_ACTIVE_LEVEL);
    }

public:
    uint8_t getTxPin() const { return txPin; }
    uint8_t getRxPin() const { return rxPin; }
    bool isBusy() const { return portBusy; }
    bool isPolling() const { return pollingActive; }
    bool isDebugUnlocked() const { return debugModeUnlocked; }

    static bool isBlockedCommand(const String &cmd) {
        String cleanCmd = cmd;
        cleanCmd.trim();
        cleanCmd.toLowerCase();
        if (cleanCmd.length() == 0) return false;

        int spaceIdx = cleanCmd.indexOf(' ');
        int tabIdx = cleanCmd.indexOf('\t');
        int splitIdx = -1;
        if (spaceIdx >= 0 && tabIdx >= 0) splitIdx = min(spaceIdx, tabIdx);
        else if (spaceIdx >= 0) splitIdx = spaceIdx;
        else if (tabIdx >= 0) splitIdx = tabIdx;

        String baseToken = (splitIdx >= 0) ? cleanCmd.substring(0, splitIdx) : cleanCmd;
        baseToken.trim();

        for (size_t i = 0; i < BLOCKED_COMMANDS_COUNT; ++i) {
            if (baseToken.equals(BLOCKED_CONSOLE_COMMANDS[i])) {
                return true;
            }
        }
        return false;
    }

    void enqueueUserCommand(const String &rawCmd) {
        String cmd = rawCmd;
        cmd.trim();
        if (cmd.length() == 0) return;

        String lowerCmd = cmd;
        lowerCmd.toLowerCase();

        // Check for debug login unlock / lock
        if (lowerCmd.startsWith("login debug") || lowerCmd.equals("login")) {
            debugModeUnlocked = true;
            consoleLog.logWarn("[SECURITY] Debug mode unlocked: safety command filters disabled for this session.");
        } else if (lowerCmd.equals("logout") || lowerCmd.equals("exit") || lowerCmd.equals("quit")) {
            debugModeUnlocked = false;
            consoleLog.logInfo("[SECURITY] Debug mode locked: safety command filters enabled.");
        }

        // If not in debug mode, check against blocked commands
        if (!debugModeUnlocked && isBlockedCommand(cmd)) {
            consoleLog.logWarn("[SECURITY] Command '" + cmd + "' is blocked for battery safety. Type 'login debug' to unlock.");
            return;
        }

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
        response.reserve(4096);

        uint32_t startTime = millis();
        uint32_t lastByteTime = millis();
        size_t bytesRead = 0;

        while (millis() - startTime < timeoutMs) {
            bool readAny = false;

            // Fast drain of UART buffer to avoid RX FIFO overflow
            while (port->available()) {
                char c = port->read();
                response += c;
                bytesRead++;
                lastByteTime = millis();
                readAny = true;

                // Pylontech completion markers:
                if (response.endsWith("$$\n") || response.endsWith("$$\r\n") || 
                    response.endsWith("$$\r") || response.endsWith("$$") ||
                    response.endsWith("pylon>") || response.endsWith("pylon> ")) {
                    break;
                }
            }

            if (response.endsWith("$$\n") || response.endsWith("$$\r\n") || 
                response.endsWith("$$\r") || response.endsWith("$$") ||
                response.endsWith("pylon>") || response.endsWith("pylon> ")) {
                break;
            }

            if (readAny) {
                // If data is actively arriving, yield quickly to RTOS without running heavy web server tasks
                vTaskDelay(1);
            } else {
                // When UART is idle/waiting for next chunk, service background network tasks
                yieldSystemTasks();

                // If 0 bytes received after 1200ms, abort early (battery disconnected / not responding)
                if (bytesRead == 0 && (millis() - startTime > 1200)) {
                    break;
                }
                // If silence after receiving data exceeds 1500ms, abort (safety fallback if marker missing)
                if (bytesRead > 0 && (millis() - lastByteTime > 1500)) {
                    break;
                }
                delay(2);
            }
        }

        setLedSerial(false);

        if (response.length() > 0) {
            consoleLog.logRx(response);
            if (!PylonParser::isResponseComplete(response)) {
                consoleLog.logWarn("Command '" + cmd + "' response incomplete (missing '$$' completion marker)");
            }
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
        PollingActiveGuard guard(pollingActive);
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
            if (respInfo.length() > 0 && PylonParser::parseInfo(respInfo, stack, 0)) {
                consoleLog.logInfo("Detected battery model: " + String(stack.modelName));
            } else {
                consoleLog.logError("No valid response on 'info' command");
            }
            drainUserQueue();
        }

        // Step 2: Read Power table (pwr) -> dynamic values: V, I, SOC, Status (ALWAYS)
        String respPwr = sendCommand("pwr");
        if (respPwr.length() < 10 || !PylonParser::parsePwr(respPwr, stack)) {
            stack.scrapeSuccess = false;
            consoleLog.logError("Failed to read 'pwr' table - skipping module polling");
            return false;
        }

        consoleLog.logInfo("Modules detected: " + String(stack.moduleCount) + (stack.isMaster ? " (Master Stack)" : " (Single Unit)"));
        drainUserQueue();

        uint8_t targetMod = stack.activeModuleIndex;
        if (targetMod < 1 || targetMod > MAX_MODULES) targetMod = 1;

        bool fastPollOk = true;

        // Step 3: Dynamic cell telemetry (bat) -> cell voltages, temps, balancing (ALWAYS in fast poll)
        String respBat = sendCommand("bat");
        if (!PylonParser::parseBat(respBat, stack, targetMod)) {
            fastPollOk = false;
            consoleLog.logWarn("Failed or incomplete cell telemetry for module " + String(targetMod));
        }
        drainUserQueue();

        if (stack.isMaster && stack.moduleCount > 1) {
            for (uint8_t n = 1; n <= stack.moduleCount && n <= MAX_MODULES; ++n) {
                if (!stack.modules[n].present || n == targetMod) continue;
                String rBatN = sendCommand("bat " + String(n));
                if (!PylonParser::parseBat(rBatN, stack, n)) {
                    fastPollOk = false;
                    consoleLog.logWarn("Failed or incomplete cell telemetry for module " + String(n));
                }
                drainUserQueue();
            }
        }

        // Step 4: Slow telemetry (stat, info, soh/euro) -> ONLY when fullSlowPoll is true (every 5 min)
        if (fullSlowPoll) {
            const ModelProfile *prof = getModelProfile(stack.model);
            uint8_t primaryMod = stack.isMaster ? 1 : targetMod;

            // Primary module always receives plain 'stat' and 'info' without index
            // (US3000D syntax is strictly 'stat' / 'info'; US3000C also defaults to active unit)
            String respStat = sendCommand("stat");
            PylonParser::parseStat(respStat, stack, primaryMod);
            drainUserQueue();

            String respInfo = sendCommand("info");
            PylonParser::parseInfo(respInfo, stack, primaryMod);
            drainUserQueue();

            if (prof->supportsEuro) {
                String respEuro = sendCommand("euro");
                PylonParser::parseEuro(respEuro, stack, primaryMod);
                drainUserQueue();
            } else if (prof->supportsSohCmd) {
                String respSoh = sendCommand("soh");
                PylonParser::parseSoh(respSoh, stack, primaryMod);
                drainUserQueue();
            }

            // Slave modules in master stack (only for models that support indexed queries, e.g. US3000C)
            if (stack.isMaster && stack.moduleCount > 1) {
                for (uint8_t n = 1; n <= stack.moduleCount && n <= MAX_MODULES; ++n) {
                    if (!stack.modules[n].present || n == primaryMod) continue;

                    if (prof->slaveSupportsStat) {
                        String rStatN = sendCommand("stat " + String(n));
                        PylonParser::parseStat(rStatN, stack, n);
                        drainUserQueue();
                    }
                    if (prof->slaveSupportsInfo) {
                        String rInfoN = sendCommand("info " + String(n));
                        PylonParser::parseInfo(rInfoN, stack, n);
                        drainUserQueue();
                    }
                    if (prof->slaveSupportsSoh) {
                        String rSohN = sendCommand("soh " + String(n));
                        PylonParser::parseSoh(rSohN, stack, n);
                        drainUserQueue();
                    }
                }
            }
        }

        stack.scrapeDurationMs = millis() - startTime;
        stack.lastScrapeMillis = millis();
        stack.lastScrapeTimestamp = time(nullptr);
        stack.scrapeSuccess = fastPollOk;

        consoleLog.logInfo("Poll cycle completed " + String(fastPollOk ? "successfully" : "with warnings") + " in " + String(stack.scrapeDurationMs) + " ms");
        return fastPollOk;
    }

    bool pollModuleOnDemand(BatteryStack &stack, uint8_t modIndex) {
        if (modIndex < 1 || modIndex > MAX_MODULES) return false;
        PollingActiveGuard guard(pollingActive);
        consoleLog.logInfo("Executing on-demand refresh for Module #" + String(modIndex));

        const ModelProfile *prof = getModelProfile(stack.model);
        bool isPrimary = (modIndex == 1 || !stack.isMaster || modIndex == stack.activeModuleIndex);
        String modArg = isPrimary ? "" : (" " + String(modIndex));

        // 1. bat - always supports module index
        String rBat = sendCommand("bat" + (isPrimary && !stack.isMaster ? "" : (" " + String(modIndex))));
        PylonParser::parseBat(rBat, stack, modIndex);

        // 2. pwr - stack level
        String rPwr = sendCommand("pwr");
        PylonParser::parsePwr(rPwr, stack);

        // 3. stat, 4. info & 5. euro / soh
        if (isPrimary) {
            String rStat = sendCommand("stat");
            PylonParser::parseStat(rStat, stack, modIndex);

            String rInfo = sendCommand("info");
            PylonParser::parseInfo(rInfo, stack, modIndex);

            if (prof->supportsEuro) {
                String rEuro = sendCommand("euro");
                PylonParser::parseEuro(rEuro, stack, modIndex);
            } else if (prof->supportsSohCmd) {
                String rSoh = sendCommand("soh");
                PylonParser::parseSoh(rSoh, stack, modIndex);
            }
        } else {
            if (prof->slaveSupportsStat) {
                String rStat = sendCommand("stat" + modArg);
                PylonParser::parseStat(rStat, stack, modIndex);
            }
            if (prof->slaveSupportsInfo) {
                String rInfo = sendCommand("info" + modArg);
                PylonParser::parseInfo(rInfo, stack, modIndex);
            }
            if (prof->slaveSupportsSoh) {
                String rSoh = sendCommand("soh" + modArg);
                PylonParser::parseSoh(rSoh, stack, modIndex);
            }
        }

        consoleLog.logInfo("On-demand refresh finished for Module #" + String(modIndex));
        return true;
    }
};
