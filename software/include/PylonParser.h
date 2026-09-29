#pragma once
#include <Arduino.h>
#include <vector>
#include "BatteryData.h"

// =============================================================================
// Column Definitions for Tabular 'pwr' Output
// =============================================================================
enum PwrCol {
    PWR_COL_MOD = 0,
    PWR_COL_VOLT,
    PWR_COL_CURR,
    PWR_COL_TEMP,
    PWR_COL_TLOW,
    PWR_COL_THIGH,
    PWR_COL_VLOW,
    PWR_COL_VHIGH,
    PWR_COL_BASE_ST,
    PWR_COL_VOLT_ST,
    PWR_COL_CURR_ST,
    PWR_COL_TEMP_ST,
    PWR_COL_SOC,
    PWR_COL_TIME,
    PWR_COL_BV_ST,
    PWR_COL_BT_ST,
    PWR_COL_MOS_TEMP,
    PWR_COL_MT_ST,
    PWR_COL_MAX
};

// =============================================================================
// Key Definitions for Key-Value 'pwr' Output (Single Unit / Slave)
// =============================================================================
enum PwrKey {
    KEY_PWR_VOLTAGE,
    KEY_PWR_CURRENT,
    KEY_PWR_TEMP,
    KEY_PWR_TEMP_LOW,
    KEY_PWR_TEMP_HIGH,
    KEY_PWR_VOLT_LOW,
    KEY_PWR_VOLT_HIGH,
    KEY_PWR_MOS_TEMP,
    KEY_PWR_COULOMB,
    KEY_PWR_BASE_STATE,
    KEY_PWR_VOLT_STATE,
    KEY_PWR_CURR_STATE,
    KEY_PWR_TEMP_STATE,
    KEY_PWR_MOS_TEMP_STATE,
    KEY_PWR_SOH_STATE,
    KEY_PWR_UNKNOWN
};

class PylonParser {
public:
    static std::vector<String> splitTokens(const String &line) {
        std::vector<String> tokens;
        int len = line.length();
        int start = -1;
        for (int i = 0; i < len; ++i) {
            char c = line.charAt(i);
            if (c > ' ' && c != '\r' && c != '\n') {
                if (start == -1) start = i;
            } else {
                if (start != -1) {
                    tokens.push_back(line.substring(start, i));
                    start = -1;
                }
            }
        }
        if (start != -1) {
            tokens.push_back(line.substring(start, len));
        }
        return tokens;
    }

    // Tokenize data line and merge Date + Time tokens into a single token (e.g. "2022-04-24 14:23:58")
    // This ensures data row token count exactly matches header token count.
    static std::vector<String> splitDataTokens(const String &line) {
        std::vector<String> raw = splitTokens(line);
        std::vector<String> out;
        out.reserve(raw.size());
        for (size_t i = 0; i < raw.size(); ++i) {
            if (i + 1 < raw.size() && raw[i].length() == 10 && raw[i].charAt(4) == '-' && raw[i].charAt(7) == '-' && raw[i+1].indexOf(':') >= 0) {
                out.push_back(raw[i] + " " + raw[i+1]);
                i++; // Skip merged time token
            } else {
                out.push_back(raw[i]);
            }
        }
        return out;
    }

    static void trimStr(String &s) {
        s.trim();
        while (s.endsWith("\r") || s.endsWith("\n")) {
            s.remove(s.length() - 1);
        }
    }

    static PwrCol matchPwrHeaderToken(const String &tok) {
        String t = tok;
        t.trim();
        if (t.equalsIgnoreCase("Power") || t.equalsIgnoreCase("Battery") || t.equalsIgnoreCase("ID") || t.equalsIgnoreCase("Index")) return PWR_COL_MOD;
        if (t.equalsIgnoreCase("Volt") || t.equalsIgnoreCase("Voltage")) return PWR_COL_VOLT;
        if (t.equalsIgnoreCase("Curr") || t.equalsIgnoreCase("Current")) return PWR_COL_CURR;
        if (t.equalsIgnoreCase("Tempr") || t.equalsIgnoreCase("Temp") || t.equalsIgnoreCase("Temperature")) return PWR_COL_TEMP;
        if (t.equalsIgnoreCase("Tlow") || t.equalsIgnoreCase("Tempr.Low") || t.equalsIgnoreCase("Temp.Low")) return PWR_COL_TLOW;
        if (t.equalsIgnoreCase("Thigh") || t.equalsIgnoreCase("Tempr.High") || t.equalsIgnoreCase("Temp.High")) return PWR_COL_THIGH;
        if (t.equalsIgnoreCase("Vlow") || t.equalsIgnoreCase("Volt.Low") || t.equalsIgnoreCase("Voltage.Low")) return PWR_COL_VLOW;
        if (t.equalsIgnoreCase("Vhigh") || t.equalsIgnoreCase("Volt.High") || t.equalsIgnoreCase("Voltage.High")) return PWR_COL_VHIGH;
        if (t.equalsIgnoreCase("Base.St") || t.equalsIgnoreCase("Base.State") || t.equalsIgnoreCase("Base State") || t.equalsIgnoreCase("Basic.Status")) return PWR_COL_BASE_ST;
        if (t.equalsIgnoreCase("Volt.St") || t.equalsIgnoreCase("Volt.State") || t.equalsIgnoreCase("Volt Status")) return PWR_COL_VOLT_ST;
        if (t.equalsIgnoreCase("Curr.St") || t.equalsIgnoreCase("Curr.State") || t.equalsIgnoreCase("Curr Status")) return PWR_COL_CURR_ST;
        if (t.equalsIgnoreCase("Temp.St") || t.equalsIgnoreCase("Temp.State") || t.equalsIgnoreCase("Tmpr.St") || t.equalsIgnoreCase("Tmpr.Status")) return PWR_COL_TEMP_ST;
        if (t.equalsIgnoreCase("Coulomb") || t.equalsIgnoreCase("SOC") || t.equalsIgnoreCase("Pwr.Percent")) return PWR_COL_SOC;
        if (t.equalsIgnoreCase("Time") || t.equalsIgnoreCase("Timestamp") || t.equalsIgnoreCase("Date")) return PWR_COL_TIME;
        if (t.equalsIgnoreCase("B.V.St") || t.equalsIgnoreCase("BV.St") || t.equalsIgnoreCase("Bat.Volt.St")) return PWR_COL_BV_ST;
        if (t.equalsIgnoreCase("B.T.St") || t.equalsIgnoreCase("BT.St") || t.equalsIgnoreCase("Bat.Temp.St")) return PWR_COL_BT_ST;
        if (t.equalsIgnoreCase("MosTempr") || t.equalsIgnoreCase("Mos.Temp") || t.equalsIgnoreCase("MosTemp") || t.equalsIgnoreCase("Mos.Tempr")) return PWR_COL_MOS_TEMP;
        if (t.equalsIgnoreCase("M.T.St") || t.equalsIgnoreCase("MT.St") || t.equalsIgnoreCase("Mos.Temp.St") || t.equalsIgnoreCase("Mos.Status")) return PWR_COL_MT_ST;
        return PWR_COL_MAX;
    }

    static PwrKey matchPwrKey(const String &k) {
        String key = k;
        key.trim();
        if (key.equalsIgnoreCase("Voltage")) return KEY_PWR_VOLTAGE;
        if (key.equalsIgnoreCase("Current")) return KEY_PWR_CURRENT;
        if (key.equalsIgnoreCase("Temperature")) return KEY_PWR_TEMP;
        if (key.indexOf("Mos") >= 0 && (key.indexOf("Status") >= 0 || key.indexOf("State") >= 0)) return KEY_PWR_MOS_TEMP_STATE;
        if (key.indexOf("Mos") >= 0 && key.indexOf("Temp") >= 0) return KEY_PWR_MOS_TEMP;
        if ((key.indexOf("Temp") >= 0 || key.indexOf("Tmpr") >= 0) && key.indexOf("High") >= 0) return KEY_PWR_TEMP_HIGH;
        if ((key.indexOf("Temp") >= 0 || key.indexOf("Tmpr") >= 0) && key.indexOf("Low") >= 0) return KEY_PWR_TEMP_LOW;
        if (key.indexOf("Volt") >= 0 && key.indexOf("High") >= 0) return KEY_PWR_VOLT_HIGH;
        if (key.indexOf("Volt") >= 0 && key.indexOf("Low") >= 0) return KEY_PWR_VOLT_LOW;
        if (key.equalsIgnoreCase("Coulomb")) return KEY_PWR_COULOMB;
        if (key.equalsIgnoreCase("Basic Status") || key.equalsIgnoreCase("Base State") || key.equalsIgnoreCase("Basic State")) return KEY_PWR_BASE_STATE;
        if (key.equalsIgnoreCase("Volt Status") || key.equalsIgnoreCase("Volt. State") || key.equalsIgnoreCase("Voltage Status") || key.equalsIgnoreCase("Volt.St")) return KEY_PWR_VOLT_STATE;
        if (key.equalsIgnoreCase("Current Status") || key.equalsIgnoreCase("Curr. State") || key.equalsIgnoreCase("Curr.St")) return KEY_PWR_CURR_STATE;
        if (key.equalsIgnoreCase("Tmpr. Status") || key.equalsIgnoreCase("Temp. State") || key.equalsIgnoreCase("Temperature Status") || key.equalsIgnoreCase("Tmpr Status") || key.equalsIgnoreCase("Temp Status") || key.equalsIgnoreCase("Temp.St")) return KEY_PWR_TEMP_STATE;
        if (key.equalsIgnoreCase("Soh. Status") || key.equalsIgnoreCase("SOH Status") || key.equalsIgnoreCase("Soh Status")) return KEY_PWR_SOH_STATE;
        return KEY_PWR_UNKNOWN;
    }

    // =========================================================================
    // 1. parseInfo — Device & Stack Metadata
    // =========================================================================
    static bool parseInfo(const String &response, BatteryStack &stack, uint8_t targetModule = 0) {
        if (targetModule == 0) {
            targetModule = stack.activeModuleIndex;
        }
        if (targetModule < 1 || targetModule > MAX_MODULES) targetModule = 1;

        // Scan first for Device address
        int daPos = response.indexOf("Device address");
        if (daPos != -1) {
            int endLine = response.indexOf('\n', daPos);
            if (endLine == -1) endLine = response.length();
            String l = response.substring(daPos, endLine);
            int colon = l.indexOf(':');
            if (colon != -1) l = l.substring(colon + 1);
            std::vector<String> t = splitTokens(l);
            for (size_t i = 0; i < t.size(); ++i) {
                int a = t[i].toInt();
                if (a >= 1 && a <= MAX_MODULES) {
                    stack.activeModuleIndex = a;
                    targetModule = a;
                    break;
                }
            }
        }

        BatteryModule &mod = stack.modules[targetModule];
        mod.index = targetModule;
        ModuleInfo &info = mod.info;
        
        int startPos = 0;
        while (startPos < response.length()) {
            int endPos = response.indexOf('\n', startPos);
            if (endPos == -1) endPos = response.length();
            String line = response.substring(startPos, endPos);
            startPos = endPos + 1;
            trimStr(line);

            int colonPos = line.indexOf(':');
            if (colonPos == -1) continue;

            String key = line.substring(0, colonPos);
            String val = line.substring(colonPos + 1);
            trimStr(key);
            trimStr(val);

            if (key.equalsIgnoreCase("Device address")) {
                int addr = val.toInt();
                if (addr >= 1 && addr <= MAX_MODULES) {
                    stack.activeModuleIndex = addr;
                }
            } else if (key.equalsIgnoreCase("Manufacturer")) {
                strncpy(info.manufacturer, val.c_str(), sizeof(info.manufacturer) - 1);
            } else if (key.equalsIgnoreCase("Device name")) {
                strncpy(info.deviceName, val.c_str(), sizeof(info.deviceName) - 1);
                if (targetModule == stack.activeModuleIndex || stack.model == MODEL_UNKNOWN) {
                    const ModelProfile *prof = detectModelProfile(val.c_str());
                    stack.model = prof->model;
                    strncpy(stack.modelName, (prof->model != MODEL_UNKNOWN) ? prof->name : val.c_str(), sizeof(stack.modelName) - 1);
                }
            } else if (key.equalsIgnoreCase("Board version") || key.equalsIgnoreCase("Board ver")) {
                strncpy(info.boardVersion, val.c_str(), sizeof(info.boardVersion) - 1);
                if (info.board[0] == '\0') {
                    strncpy(info.board, val.c_str(), sizeof(info.board) - 1);
                }
            } else if (key.equalsIgnoreCase("Board")) {
                strncpy(info.board, val.c_str(), sizeof(info.board) - 1);
            } else if (key.equalsIgnoreCase("Main Soft version")) {
                strncpy(info.mainSoftVersion, val.c_str(), sizeof(info.mainSoftVersion) - 1);
            } else if (key.equalsIgnoreCase("Soft  version") || key.equalsIgnoreCase("Soft version")) {
                strncpy(info.softVersion, val.c_str(), sizeof(info.softVersion) - 1);
            } else if (key.equalsIgnoreCase("Boot  version") || key.equalsIgnoreCase("Boot version")) {
                strncpy(info.bootVersion, val.c_str(), sizeof(info.bootVersion) - 1);
            } else if (key.equalsIgnoreCase("Comm version")) {
                strncpy(info.commVersion, val.c_str(), sizeof(info.commVersion) - 1);
            } else if (key.equalsIgnoreCase("Release Date") || (key.indexOf("Release") >= 0 && key.indexOf("Date") >= 0)) {
                strncpy(info.releaseDate, val.c_str(), sizeof(info.releaseDate) - 1);
            } else if (key.equalsIgnoreCase("Barcode")) {
                strncpy(info.barcode, val.c_str(), sizeof(info.barcode) - 1);
            } else if (key.equalsIgnoreCase("Specification")) {
                strncpy(info.specification, val.c_str(), sizeof(info.specification) - 1);
            } else if (key.equalsIgnoreCase("Cell Number")) {
                info.cellCount = val.toInt();
            } else if (key.equalsIgnoreCase("Max Dischg Curr")) {
                info.maxDischargeCurrentMa = val.toInt();
            } else if (key.equalsIgnoreCase("Max Charge Curr")) {
                info.maxChargeCurrentMa = val.toInt();
            }
        }
        info.valid = true;
        mod.present = true;
        return true;
    }

    // =========================================================================
    // 2. parsePwr — Dynamic Header-Driven Table & Key-Value Parser
    // =========================================================================
    static bool parsePwr(const String &response, BatteryStack &stack) {
        int startPos = 0;
        uint8_t detectedCount = 0;
        uint8_t currentModIdx = stack.activeModuleIndex;
        if (currentModIdx < 1 || currentModIdx > MAX_MODULES) currentModIdx = 1;

        // Column mapper for tabular mode: maps PwrCol enum -> token index in data row
        int colMap[PWR_COL_MAX];
        for (int i = 0; i < PWR_COL_MAX; ++i) colMap[i] = -1;
        bool hasTableColumns = false;

        while (startPos < response.length()) {
            int endPos = response.indexOf('\n', startPos);
            if (endPos == -1) endPos = response.length();
            String line = response.substring(startPos, endPos);
            startPos = endPos + 1;
            trimStr(line);

            if (line.length() == 0 || line.startsWith("@") || line.startsWith("$") || line.startsWith("Command") || line.startsWith("pwr")) {
                continue;
            }

            // Power N banner in key-value single-module mode (e.g. "Power  6")
            if (line.startsWith("Power ") || line.startsWith("Power\t")) {
                std::vector<String> t = splitTokens(line);
                if (t.size() >= 2) {
                    int pNum = t[1].toInt();
                    if (pNum >= 1 && pNum <= MAX_MODULES) {
                        currentModIdx = pNum;
                        stack.activeModuleIndex = pNum;
                    }
                }
                // Do not skip if this is also a table header (e.g. "Power Volt Curr...")
                if (line.indexOf("Volt") == -1) {
                    continue;
                }
            }

            // Detect Tabular Header Line: contains Volt AND (Curr or Tempr) AND (Base.St or Volt.St or Tlow or Coulomb)
            if (line.indexOf("Volt") >= 0 && (line.indexOf("Curr") >= 0 || line.indexOf("Tempr") >= 0 || line.indexOf("Temp") >= 0) &&
                (line.indexOf("Base") >= 0 || line.indexOf("Volt.St") >= 0 || line.indexOf("Tlow") >= 0 || line.indexOf("Coulomb") >= 0)) {
                
                std::vector<String> hdr = splitTokens(line);
                for (size_t i = 0; i < hdr.size(); ++i) {
                    PwrCol col = matchPwrHeaderToken(hdr[i]);
                    if (col < PWR_COL_MAX && colMap[col] == -1) {
                        colMap[col] = (int)i;
                    }
                }
                hasTableColumns = (colMap[PWR_COL_VOLT] >= 0);
                continue;
            }

            // Parse Tabular Row (starts with module number e.g. 1..16)
            if (hasTableColumns && line.charAt(0) >= '0' && line.charAt(0) <= '9') {
                std::vector<String> t = splitDataTokens(line);
                if (t.size() < 2) continue;

                int modIdx = t[0].toInt();
                if (modIdx < 1 || modIdx > MAX_MODULES) continue;

                BatteryModule &mod = stack.modules[modIdx];
                mod.index = modIdx;

                if (line.indexOf("Absent") >= 0 || t[1] == "-") {
                    mod.present = false;
                    continue;
                }

                mod.present = true;
                detectedCount++;
                ModulePower &p = mod.power;

                auto getTok = [&](PwrCol c) -> String {
                    int idx = colMap[c];
                    if (idx >= 0 && (size_t)idx < t.size()) return t[idx];
                    return "";
                };
                auto getInt = [&](PwrCol c, int def = 0) -> int {
                    String s = getTok(c);
                    if (s.length() == 0 || s == "-") return def;
                    return s.toInt();
                };
                auto copyStr = [&](char *dest, size_t destSize, PwrCol c) {
                    String s = getTok(c);
                    if (s.length() > 0 && s != "-") {
                        strncpy(dest, s.c_str(), destSize - 1);
                        dest[destSize - 1] = '\0';
                    }
                };

                p.voltMv       = getInt(PWR_COL_VOLT);
                p.currMa       = getInt(PWR_COL_CURR);
                p.tempMdeg     = getInt(PWR_COL_TEMP);
                p.tempLowMdeg  = getInt(PWR_COL_TLOW);
                p.tempHighMdeg = getInt(PWR_COL_THIGH);
                p.voltLowMv    = getInt(PWR_COL_VLOW);
                p.voltHighMv   = getInt(PWR_COL_VHIGH);
                p.socPercent   = getInt(PWR_COL_SOC);
                p.mosTempMdeg  = getInt(PWR_COL_MOS_TEMP);

                copyStr(p.baseState,    sizeof(p.baseState),    PWR_COL_BASE_ST);
                copyStr(p.voltState,    sizeof(p.voltState),    PWR_COL_VOLT_ST);
                copyStr(p.currState,    sizeof(p.currState),    PWR_COL_CURR_ST);
                copyStr(p.tempState,    sizeof(p.tempState),    PWR_COL_TEMP_ST);
                copyStr(p.batVoltState, sizeof(p.batVoltState), PWR_COL_BV_ST);
                copyStr(p.batTempState, sizeof(p.batTempState), PWR_COL_BT_ST);
                copyStr(p.mosTempState, sizeof(p.mosTempState), PWR_COL_MT_ST);
                copyStr(p.timestamp,    sizeof(p.timestamp),    PWR_COL_TIME);

                p.valid = true;
                continue;
            }

            // Parse Key-Value Row (e.g. "Voltage : 49818 mV")
            int colonPos = line.indexOf(':');
            if (colonPos != -1) {
                String key = line.substring(0, colonPos);
                String val = line.substring(colonPos + 1);
                trimStr(key);
                trimStr(val);

                // Strip unit suffix if present (e.g. "49818 mV" -> "49818")
                String valNum = val;
                int spaceIdx = valNum.indexOf(' ');
                if (spaceIdx > 0) valNum = valNum.substring(0, spaceIdx);

                BatteryModule &mod = stack.modules[currentModIdx];
                mod.index = currentModIdx;
                ModulePower &p = mod.power;

                PwrKey pk = matchPwrKey(key);
                switch (pk) {
                    case KEY_PWR_VOLTAGE:
                        p.voltMv = valNum.toInt();
                        p.valid = true;
                        mod.present = true;
                        detectedCount++;
                        break;
                    case KEY_PWR_CURRENT:
                        p.currMa = valNum.toInt();
                        break;
                    case KEY_PWR_TEMP:
                        p.tempMdeg = valNum.toInt();
                        break;
                    case KEY_PWR_TEMP_HIGH:
                        p.tempHighMdeg = valNum.toInt();
                        break;
                    case KEY_PWR_TEMP_LOW:
                        p.tempLowMdeg = valNum.toInt();
                        break;
                    case KEY_PWR_VOLT_HIGH:
                        p.voltHighMv = valNum.toInt();
                        break;
                    case KEY_PWR_VOLT_LOW:
                        p.voltLowMv = valNum.toInt();
                        break;
                    case KEY_PWR_MOS_TEMP:
                        p.mosTempMdeg = valNum.toInt();
                        break;
                    case KEY_PWR_COULOMB:
                        p.socPercent = valNum.toInt();
                        break;
                    case KEY_PWR_BASE_STATE:
                        strncpy(p.baseState, valNum.c_str(), sizeof(p.baseState) - 1);
                        break;
                    case KEY_PWR_VOLT_STATE:
                        strncpy(p.voltState, valNum.c_str(), sizeof(p.voltState) - 1);
                        break;
                    case KEY_PWR_CURR_STATE:
                        strncpy(p.currState, valNum.c_str(), sizeof(p.currState) - 1);
                        break;
                    case KEY_PWR_TEMP_STATE:
                        strncpy(p.tempState, valNum.c_str(), sizeof(p.tempState) - 1);
                        break;
                    case KEY_PWR_MOS_TEMP_STATE:
                        strncpy(p.mosTempState, valNum.c_str(), sizeof(p.mosTempState) - 1);
                        break;
                    case KEY_PWR_SOH_STATE:
                        strncpy(p.sohState, valNum.c_str(), sizeof(p.sohState) - 1);
                        break;
                    default:
                        break;
                }
            }
        }

        if (detectedCount == 0 && stack.modules[currentModIdx].power.valid) {
            detectedCount = 1;
        }
        stack.moduleCount = detectedCount;
        stack.isMaster = (detectedCount > 1);
        return (detectedCount > 0);
    }

    // =========================================================================
    // 3. parseStat — Diagnostic Counters & Lifetime Statistics
    // =========================================================================
    static bool parseStat(const String &response, BatteryStack &stack, uint8_t targetModule = 0) {
        if (targetModule == 0) {
            targetModule = stack.activeModuleIndex;
        }
        if (targetModule < 1 || targetModule > MAX_MODULES) targetModule = 1;

        int daPos = response.indexOf("Device address");
        if (daPos != -1) {
            int endLine = response.indexOf('\n', daPos);
            if (endLine == -1) endLine = response.length();
            String l = response.substring(daPos, endLine);
            int colon = l.indexOf(':');
            if (colon != -1) l = l.substring(colon + 1);
            std::vector<String> t = splitTokens(l);
            for (size_t i = 0; i < t.size(); ++i) {
                int a = t[i].toInt();
                if (a >= 1 && a <= MAX_MODULES) {
                    stack.activeModuleIndex = a;
                    targetModule = a;
                    break;
                }
            }
        }

        BatteryModule &mod = stack.modules[targetModule];
        mod.index = targetModule;
        ModuleStats parsedSt;
        bool hasData = false;

        int startPos = 0;
        while (startPos < response.length()) {
            int endPos = response.indexOf('\n', startPos);
            if (endPos == -1) endPos = response.length();
            String line = response.substring(startPos, endPos);
            startPos = endPos + 1;
            trimStr(line);

            int colonPos = line.indexOf(':');
            if (colonPos == -1) continue;

            String key = line.substring(0, colonPos);
            String val = line.substring(colonPos + 1);
            trimStr(key);
            trimStr(val);

            if (key.equalsIgnoreCase("CYCLE Times")) {
                parsedSt.cycleTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("SOH") || (key.indexOf("SOH") >= 0 && key.indexOf("Percent") >= 0)) {
                parsedSt.sohPercent = val.toInt();
                hasData = true;
            } else if (key.equalsIgnoreCase("Dsg Cap")) {
                parsedSt.dischargedCapMah = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("Pwr Coulomb")) {
                parsedSt.coulombMc = strtoull(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("Charge Secs.") || key.equalsIgnoreCase("Charge Secs")) {
                parsedSt.chargeSecs = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("Discharge Secs.") || key.equalsIgnoreCase("Discharge Secs")) {
                parsedSt.dischargeSecs = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("Charge Times") || key.equalsIgnoreCase("Charge Cnt.") || key.equalsIgnoreCase("Charge Cnt")) {
                parsedSt.chargeTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("Discharge Times") || key.equalsIgnoreCase("Discharge Cnt.") || key.equalsIgnoreCase("Discharge Cnt")) {
                parsedSt.dischargeTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("Idle Times")) {
                parsedSt.idleTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("Shut Times")) {
                parsedSt.shutTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("Reset Times")) {
                parsedSt.resetTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("Power on Times") || key.equalsIgnoreCase("Power On Times")) {
                parsedSt.powerOnTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("COC Times") || key.equalsIgnoreCase("COC2 Times")) {
                parsedSt.cocTimes += strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("DOC Times") || key.equalsIgnoreCase("DOC2 Times")) {
                parsedSt.docTimes += strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("COCA Times")) {
                parsedSt.cocaTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("DOCA Times")) {
                parsedSt.docaTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("SC Times")) {
                parsedSt.scTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("Bat OV Times")) {
                parsedSt.batOvTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("Bat HV Times")) {
                parsedSt.batHvTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("Bat LV Times")) {
                parsedSt.batLvTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("Bat UV Times")) {
                parsedSt.batUvTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("Bat SLP Times")) {
                parsedSt.batSlpTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("Pwr OV Times")) {
                parsedSt.pwrOvTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("Pwr HV Times")) {
                parsedSt.pwrHvTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("Pwr LV Times")) {
                parsedSt.pwrLvTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("Pwr UV Times")) {
                parsedSt.pwrUvTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("Pwr SLP Times")) {
                parsedSt.pwrSlpTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("COT Times")) {
                parsedSt.cotTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("CUT Times")) {
                parsedSt.cutTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("DOT Times")) {
                parsedSt.dotTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("DUT Times")) {
                parsedSt.dutTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("CHT Times")) {
                parsedSt.chtTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("CLT Times")) {
                parsedSt.cltTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("DHT Times")) {
                parsedSt.dhtTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("DLT Times")) {
                parsedSt.dltTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("RV Times")) {
                parsedSt.rvTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("Input OV Times")) {
                parsedSt.inputOvTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("BMICERR Times") || key.equalsIgnoreCase("BMIC ERR Times")) {
                parsedSt.bmicErrTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("LifeWarn Times")) {
                parsedSt.lifeWarnTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("LifeAlarm Times")) {
                parsedSt.lifeAlarmTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            } else if (key.equalsIgnoreCase("SOH Status") || key.equalsIgnoreCase("Soh. Status")) {
                strncpy(parsedSt.sohStatus, val.c_str(), sizeof(parsedSt.sohStatus) - 1);
                hasData = true;
            } else if (key.equalsIgnoreCase("SOH Times") || key.equalsIgnoreCase("Soh Times") || key.equalsIgnoreCase("SOH count")) {
                parsedSt.sohTimes = strtoul(val.c_str(), nullptr, 10);
                hasData = true;
            }
        }

        if (!hasData) return false;

        parsedSt.valid = true;
        if (strlen(parsedSt.sohStatus) == 0) {
            strncpy(parsedSt.sohStatus, "Normal", sizeof(parsedSt.sohStatus) - 1);
        }
        mod.stats = parsedSt;
        return true;
    }

    // =========================================================================
    // 4. parseSoh — Cell SOH Status & Degradation Counters (Model C)
    // =========================================================================
    static bool parseSoh(const String &response, BatteryStack &stack, uint8_t targetModule = 0) {
        if (targetModule == 0) targetModule = stack.activeModuleIndex;

        int pwrPos = response.indexOf("Power");
        if (pwrPos != -1) {
            int endL = response.indexOf('\n', pwrPos);
            if (endL == -1) endL = response.length();
            String pwrLine = response.substring(pwrPos, endL);
            std::vector<String> pt = splitTokens(pwrLine);
            if (pt.size() >= 2) {
                int pm = pt[1].toInt();
                if (pm >= 1 && pm <= MAX_MODULES) {
                    targetModule = (uint8_t)pm;
                }
            }
        }

        if (targetModule < 1 || targetModule > MAX_MODULES) targetModule = 1;
        BatteryModule &mod = stack.modules[targetModule];
        mod.index = targetModule;
        ModuleStats &st = mod.stats;

        int startPos = 0;
        uint8_t cellsParsed = 0;
        while (startPos < response.length()) {
            int endPos = response.indexOf('\n', startPos);
            if (endPos == -1) endPos = response.length();
            String line = response.substring(startPos, endPos);
            startPos = endPos + 1;
            trimStr(line);

            if (line.length() == 0 || line.startsWith("@") || line.startsWith("$") || 
                line.startsWith("Power") || line.startsWith("Battery") || line.startsWith("soh") ||
                line.startsWith("Command")) {
                continue;
            }

            std::vector<String> t = splitTokens(line);
            if (t.size() >= 4) {
                char *endptr = nullptr;
                long cellIdx = strtol(t[0].c_str(), &endptr, 10);
                if (*endptr == '\0' && cellIdx >= 0 && cellIdx < MAX_CELLS_PER_MODULE) {
                    CellInfo &c = mod.cells[cellIdx];
                    if (c.voltMv == 0) {
                        c.voltMv = t[1].toInt();
                    }
                    c.sohCount = (uint16_t)t[2].toInt();
                    strncpy(c.sohStatus, t[3].c_str(), sizeof(c.sohStatus) - 1);
                    c.sohValid = true;
                    cellsParsed++;

                    if (strlen(st.sohStatus) == 0 || strcmp(st.sohStatus, "Normal") == 0) {
                        strncpy(st.sohStatus, t[3].c_str(), sizeof(st.sohStatus) - 1);
                        strncpy(mod.power.sohState, t[3].c_str(), sizeof(mod.power.sohState) - 1);
                    }
                }
            }
        }
        return (cellsParsed > 0);
    }

    // =========================================================================
    // 5. parseBat — Cell Voltages, Currents, Temps & Balancing
    // =========================================================================
    static bool parseBat(const String &response, BatteryStack &stack, uint8_t targetModule = 0) {
        if (targetModule == 0) {
            targetModule = stack.activeModuleIndex;
        }
        if (targetModule < 1 || targetModule > MAX_MODULES) targetModule = 1;
        BatteryModule &mod = stack.modules[targetModule];
        mod.index = targetModule;

        int startPos = 0;
        uint8_t cellsParsed = 0;

        while (startPos < response.length()) {
            int endPos = response.indexOf('\n', startPos);
            if (endPos == -1) endPos = response.length();
            String line = response.substring(startPos, endPos);
            startPos = endPos + 1;
            trimStr(line);

            if (line.length() == 0 || line.startsWith("Battery") || line.startsWith("bat") || line.startsWith("@") || line.startsWith("$")) {
                continue;
            }

            std::vector<String> t = splitTokens(line);
            if (t.size() < 6) continue;

            // Check if first token is a valid cell number (0 to 14/15)
            char *endptr = nullptr;
            long cellIdx = strtol(t[0].c_str(), &endptr, 10);
            if (*endptr != '\0' || cellIdx < 0 || cellIdx >= MAX_CELLS_PER_MODULE) continue;

            CellInfo &c = mod.cells[cellIdx];
            c.voltMv = t[1].toInt();
            c.currMa = t[2].toInt();
            c.tempMdeg = t[3].toInt();
            strncpy(c.state, t[4].c_str(), sizeof(c.state) - 1);

            // Find SOC, Coulomb and BAL tokens from the line
            for (size_t i = 5; i < t.size(); ++i) {
                if (t[i].endsWith("%")) {
                    c.socPercent = t[i].toInt();
                    if (i + 1 < t.size()) {
                        c.coulombMah = strtoul(t[i + 1].c_str(), nullptr, 10);
                    }
                }
                if (t[i] == "Y" || t[i] == "N") {
                    c.balance = (t[i] == "Y");
                }
            }

            // Copy status strings from first cell as fallback for ModulePower
            if (cellsParsed == 0 && t.size() >= 8) {
                ModulePower &p = mod.power;
                if (p.baseState[0] == '\0') strncpy(p.baseState, t[4].c_str(), sizeof(p.baseState) - 1);
                if (p.voltState[0] == '\0') strncpy(p.voltState, t[5].c_str(), sizeof(p.voltState) - 1);
                if (p.currState[0] == '\0') strncpy(p.currState, t[6].c_str(), sizeof(p.currState) - 1);
                if (p.tempState[0] == '\0') strncpy(p.tempState, t[7].c_str(), sizeof(p.tempState) - 1);
            }
            cellsParsed++;
        }
        mod.cellCountParsed = cellsParsed;
        return (cellsParsed > 0);
    }

    // =========================================================================
    // 6. parseEuro — European Efficiency & Energy Counters (Model D)
    // =========================================================================
    static bool parseEuro(const String &response, BatteryStack &stack, uint8_t targetModule = 1) {
        if (targetModule < 1 || targetModule > MAX_MODULES) targetModule = 1;
        BatteryModule &mod = stack.modules[targetModule];
        EuroStats &euro = mod.euro;

        int startPos = 0;
        while (startPos < response.length()) {
            int endPos = response.indexOf('\n', startPos);
            if (endPos == -1) endPos = response.length();
            String line = response.substring(startPos, endPos);
            startPos = endPos + 1;
            trimStr(line);

            int colonPos = line.indexOf(':');
            if (colonPos == -1) continue;

            String key = line.substring(0, colonPos);
            String val = line.substring(colonPos + 1);
            trimStr(key);
            trimStr(val);

            if (key.equalsIgnoreCase("Remain Cap.")) {
                euro.remainCapAh = val.toInt();
            } else if (key.equalsIgnoreCase("Remain Power")) {
                euro.remainPowerWatts = val.toInt();
            } else if (key.equalsIgnoreCase("Round Trip Eff.")) {
                euro.roundTripEff = val.toInt();
            } else if (key.equalsIgnoreCase("Self Dsg Rate")) {
                euro.selfDsgRate = val.toInt();
            } else if (key.equalsIgnoreCase("Resistence")) {
                euro.resistanceMilliOhm = val.toInt();
            } else if (key.equalsIgnoreCase("Energy Thro.")) {
                euro.energyThroughputWh = strtoull(val.c_str(), nullptr, 10);
            } else if (key.equalsIgnoreCase("Capac. Thro.")) {
                euro.capacityThroughputAh = strtoull(val.c_str(), nullptr, 10);
            } else if (key.equalsIgnoreCase("Deep Dsg. Count")) {
                euro.deepDsgCount = val.toInt();
            } else if (key.equalsIgnoreCase("Chg. Dsg. Cycle")) {
                euro.chgDsgCycle = val.toInt();
            }
        }
        euro.valid = true;
        return true;
    }
};
