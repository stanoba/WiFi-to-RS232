#pragma once
#include <Arduino.h>
#include "Config.h"
#include "BmsModel.h"
#include "BmsProtocol.h"
#include "BmsPhysics.h"
#include "ConsoleLog.h"

class BmsUartHandler {
private:
    HardwareSerial &serialPort;
    int pinTx;
    int pinRx;
    int pinLedSerial;
    String rxBuffer;
    uint32_t lastActivityMs;
    bool ledActive;

public:
    BmsUartHandler(HardwareSerial &port, int tx, int rx, int ledSer)
        : serialPort(port), pinTx(tx), pinRx(rx), pinLedSerial(ledSer), lastActivityMs(0), ledActive(false) {}

    void begin(unsigned long baud = SERIAL_BAUD_RATE) {
        rxBuffer.reserve(128);
        #if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(ARDUINO_ESP32C3_DEV)
            serialPort.begin(baud, SERIAL_8N1, pinRx, pinTx);
        #elif defined(CONFIG_IDF_TARGET_ESP32S2) || defined(ARDUINO_LOLIN_S2_MINI)
            serialPort.begin(baud, SERIAL_8N1, pinRx, pinTx);
        #else
            serialPort.begin(baud, SERIAL_8N1, pinRx, pinTx);
        #endif

        if (pinLedSerial >= 0) {
            pinMode(pinLedSerial, OUTPUT);
            digitalWrite(pinLedSerial, !LED_ACTIVE_LEVEL); // OFF
        }
    }

    void pulseLed() {
        if (pinLedSerial >= 0) {
            digitalWrite(pinLedSerial, LED_ACTIVE_LEVEL); // ON
            ledActive = true;
            lastActivityMs = millis();
        }
    }

    void handleLedTimeout() {
        if (ledActive && (millis() - lastActivityMs > 35)) {
            if (pinLedSerial >= 0) {
                digitalWrite(pinLedSerial, !LED_ACTIVE_LEVEL); // OFF
            }
            ledActive = false;
        }
    }

    String processCommand(const String &cmdRaw) {
        String cmd = cmdRaw;
        cmd.trim();
        if (cmd.length() == 0) return "";

        pulseLed();
        g_consoleLog.logRx(cmd);
        DEBUG_PRINT("[RS232 RX] >> ");
        DEBUG_PRINTLN(cmd);

        String cmdLower = cmd;
        cmdLower.toLowerCase();

        String response = "";

        if (cmdLower == "help" || cmdLower == "?") {
            response = BmsProtocolFormatter::formatHelp(g_stack.modules[0]);
        } else if (cmdLower == "info") {
            response = BmsProtocolFormatter::formatInfo(g_stack, 1);
        } else if (cmdLower.startsWith("info ")) {
            int modNum = cmdLower.substring(5).toInt();
            response = BmsProtocolFormatter::formatInfo(g_stack, modNum);
        } else if (cmdLower == "pwr") {
            response = BmsProtocolFormatter::formatPwr(g_stack);
        } else if (cmdLower == "bat") {
            response = BmsProtocolFormatter::formatBat(g_stack, 1);
        } else if (cmdLower.startsWith("bat ")) {
            int modNum = cmdLower.substring(4).toInt();
            response = BmsProtocolFormatter::formatBat(g_stack, modNum);
        } else if (cmdLower == "stat") {
            response = BmsProtocolFormatter::formatStat(g_stack, 1);
        } else if (cmdLower.startsWith("stat ")) {
            int modNum = cmdLower.substring(5).toInt();
            response = BmsProtocolFormatter::formatStat(g_stack, modNum);
        } else if (cmdLower == "soh") {
            response = BmsProtocolFormatter::formatSoh(g_stack, 1);
        } else if (cmdLower.startsWith("soh ")) {
            int modNum = cmdLower.substring(4).toInt();
            response = BmsProtocolFormatter::formatSoh(g_stack, modNum);
        } else if (cmdLower == "euro") {
            response = BmsProtocolFormatter::formatEuro(g_stack, 1);
        } else if (cmdLower.startsWith("euro ")) {
            int modNum = cmdLower.substring(5).toInt();
            response = BmsProtocolFormatter::formatEuro(g_stack, modNum);
        } else if (cmdLower == "unit") {
            response = "@\nUnit: 1\n$$\npylon>";
        } else if (cmdLower == "time") {
            response = "@\nTime: 2026-10-03 13:00:00\n$$\npylon>";
        } else if (cmdLower.startsWith("login")) {
            response = "@\nAuthentication Success\n$$\npylon>";
        } else if (cmdLower == "logout" || cmdLower == "exit") {
            response = "@\nLogged out\n$$\npylon>";
        } else {
            response = BmsProtocolFormatter::formatUnknown();
        }

        if (response.length() > 0) {
            pulseLed();
            g_consoleLog.logTx(response);
            DEBUG_PRINT("[RS232 TX] << Sent response (");
            DEBUG_PRINT(response.length());
            DEBUG_PRINTLN(" bytes)");

            serialPort.print("\r\n");
            serialPort.print(response);
            serialPort.print("\r\n");
            serialPort.flush();
        }

        return response;
    }

    void loop() {
        handleLedTimeout();

        while (serialPort.available() > 0) {
            char c = (char)serialPort.read();
            if (c == '\r' || c == '\n') {
                if (rxBuffer.length() > 0) {
                    processCommand(rxBuffer);
                    rxBuffer = "";
                }
            } else if (c >= 32 && c <= 126) {
                if (rxBuffer.length() < 128) {
                    rxBuffer += c;
                }
            }
        }
    }
};
