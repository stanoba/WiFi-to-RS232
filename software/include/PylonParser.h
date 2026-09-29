#pragma once
#include <Arduino.h>
#include <vector>
#include "BatteryData.h"

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

    static void trimStr(String &s) {
        s.trim();
        while (s.endsWith("\r") || s.endsWith("\n")) {
            s.remove(s.length() - 1);
        }
    }

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
                    strncpy(stack.modelName, val.c_str(), sizeof(stack.modelName) - 1);
                    if (val.indexOf("US3000D") >= 0 || val.indexOf("3000D") >= 0) {
                        stack.model = MODEL_US3000D;
                    } else if (val.indexOf("US3000C") >= 0 || val.indexOf("3000C") >= 0) {
                        stack.model = MODEL_US3000C;
                    }
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

    static bool parsePwr(const String &response, BatteryStack &stack) {
        int startPos = 0;
        uint8_t detectedCount = 0;
        uint8_t currentModIdx = stack.activeModuleIndex;
        if (currentModIdx < 1 || currentModIdx > MAX_MODULES) currentModIdx = 1;

        bool isKeyValueFormat = false;
        // Key-value format (single-module) has lines like "Voltage : 49770"
        // Tabular format (multi-module / master) has "Battery  Volt  Curr ..." header
        // Distinguish by checking for ':' separator AND absence of tabular header keyword
        if (response.indexOf(':') >= 0 && response.indexOf("Battery") == -1) {
            isKeyValueFormat = true;
        }

        int mosColIndex = -1;

        while (startPos < response.length()) {
            int endPos = response.indexOf('\n', startPos);
            if (endPos == -1) endPos = response.length();
            String line = response.substring(startPos, endPos);
            startPos = endPos + 1;
            trimStr(line);

            if (line.length() == 0 || line.startsWith("@") || line.startsWith("$") || line.startsWith("-")) {
                continue;
            }

            // Header line detection for dynamic column index of MosTempr
            if (line.indexOf("Battery") >= 0 && line.indexOf("Volt") >= 0 && line.indexOf("Tempr") >= 0) {
                std::vector<String> hdr = splitTokens(line);
                for (size_t i = 0; i < hdr.size(); ++i) {
                    if (hdr[i].indexOf("Mos") >= 0) {
                        mosColIndex = (int)i;
                        break;
                    }
                }
                continue;
            }

            if (line.startsWith("Power") || line.startsWith("power")) {
                std::vector<String> t = splitTokens(line);
                if (t.size() >= 2) {
                    int pNum = t[1].toInt();
                    if (pNum >= 1 && pNum <= MAX_MODULES) {
                        currentModIdx = pNum;
                        stack.activeModuleIndex = pNum;
                    }
                }
                continue;
            }

            if (isKeyValueFormat) {
                int colonPos = line.indexOf(':');
                if (colonPos == -1) continue;

                String key = line.substring(0, colonPos);
                String val = line.substring(colonPos + 1);
                trimStr(key);
                trimStr(val);

                BatteryModule &mod = stack.modules[currentModIdx];
                mod.index = currentModIdx;
                ModulePower &p = mod.power;

                if (key.equalsIgnoreCase("Voltage")) {
                    p.voltMv = val.toInt();
                    p.valid = true;
                    mod.present = true;
                    detectedCount++;
                } else if (key.equalsIgnoreCase("Current")) {
                    p.currMa = val.toInt();
                } else if (key.equalsIgnoreCase("Temperature")) {
                    p.tempMdeg = val.toInt();
                } else if ((key.indexOf("Temp") >= 0 && key.indexOf("High") >= 0 && key.indexOf("Mos") == -1) ||
                           key.equalsIgnoreCase("Tempr High")) {
                    p.tempHighMdeg = val.toInt();
                } else if ((key.indexOf("Temp") >= 0 && key.indexOf("Low") >= 0 && key.indexOf("Mos") == -1) ||
                           key.equalsIgnoreCase("Tempr Low")) {
                    p.tempLowMdeg = val.toInt();
                } else if ((key.indexOf("Volt") >= 0 && key.indexOf("High") >= 0) ||
                           key.equalsIgnoreCase("Volt High")) {
                    p.voltHighMv = val.toInt();
                } else if ((key.indexOf("Volt") >= 0 && key.indexOf("Low") >= 0) ||
                           key.equalsIgnoreCase("Volt Low")) {
                    p.voltLowMv = val.toInt();
                } else if (key.indexOf("Mos") >= 0 && key.indexOf("Temp") >= 0 && key.indexOf("Status") == -1) {
                    p.mosTempMdeg = val.toInt();
                } else if (key.equalsIgnoreCase("Coulomb")) {
                    p.socPercent = val.toInt();
                } else if (key.equalsIgnoreCase("Basic Status") || key.equalsIgnoreCase("Base State") || key.equalsIgnoreCase("Basic State")) {
                    strncpy(p.baseState, val.c_str(), sizeof(p.baseState) - 1);
                } else if (key.indexOf("Mos") >= 0 && key.indexOf("Temp") >= 0 &&
                           (key.indexOf("Status") >= 0 || key.indexOf("State") >= 0)) {
                    strncpy(p.mosTempState, val.c_str(), sizeof(p.mosTempState) - 1);
                } else if ((key.indexOf("Volt") >= 0) &&
                           (key.indexOf("Status") >= 0 || key.indexOf("State") >= 0) &&
                           key.indexOf("Mos") == -1 && key.indexOf("Curr") == -1 &&
                           key.indexOf("Temp") == -1 && key.indexOf("Tmpr") == -1 &&
                           key.indexOf("High") == -1 && key.indexOf("Low") == -1 &&
                           key.indexOf("Soh") == -1 && key.indexOf("Coul") == -1 &&
                           key.indexOf("Pwr") == -1) {
                    strncpy(p.voltState, val.c_str(), sizeof(p.voltState) - 1);
                } else if (key.indexOf("Curr") >= 0 &&
                           (key.indexOf("Status") >= 0 || key.indexOf("State") >= 0)) {
                    strncpy(p.currState, val.c_str(), sizeof(p.currState) - 1);
                } else if ((key.indexOf("Temp") >= 0 || key.indexOf("Tmpr") >= 0) &&
                           key.indexOf("Mos") == -1 &&
                           (key.indexOf("Status") >= 0 || key.indexOf("State") >= 0) &&
                           key.indexOf("High") == -1 && key.indexOf("Low") == -1) {
                    strncpy(p.tempState, val.c_str(), sizeof(p.tempState) - 1);
                } else if ((key.indexOf("Soh") >= 0 || key.indexOf("SOH") >= 0) &&
                           (key.indexOf("Status") >= 0 || key.indexOf("State") >= 0)) {
                    strncpy(p.sohState, val.c_str(), sizeof(p.sohState) - 1);
                }
            } else {
                if (line.startsWith("pwr") || line.startsWith("Command")) continue;

                std::vector<String> t = splitTokens(line);
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
                p.voltMv = t[1].toInt();
                p.currMa = t[2].toInt();
                p.tempMdeg = t[3].toInt();

                bool isModelD = (t.size() >= 22);

                if (isModelD) {
                    p.tempLowMdeg  = t[4].toInt();
                    p.tempHighMdeg = t[6].toInt();
                    p.voltLowMv    = t[8].toInt();
                    p.voltHighMv   = t[10].toInt();
                    strncpy(p.baseState, t[12].c_str(), sizeof(p.baseState) - 1);
                    strncpy(p.voltState, t[13].c_str(), sizeof(p.voltState) - 1);
                    strncpy(p.currState, t[14].c_str(), sizeof(p.currState) - 1);
                    strncpy(p.tempState, t[15].c_str(), sizeof(p.tempState) - 1);
                    p.socPercent = t[16].toInt();
                    if (t.size() >= 19) {
                        String timeStr = t[17] + " " + t[18];
                        strncpy(p.timestamp, timeStr.c_str(), sizeof(p.timestamp) - 1);
                    }
                    if (mosColIndex >= 0 && (size_t)mosColIndex < t.size()) {
                        p.mosTempMdeg = t[mosColIndex].toInt();
                    } else if (t.size() >= 22) {
                        p.mosTempMdeg = t[21].toInt();
                    }
                } else {
                    p.tempLowMdeg  = t[4].toInt();
                    p.tempHighMdeg = t[5].toInt();
                    p.voltLowMv    = t[6].toInt();
                    p.voltHighMv   = t[7].toInt();
                    strncpy(p.baseState, t[8].c_str(), sizeof(p.baseState) - 1);
                    strncpy(p.voltState, t[9].c_str(), sizeof(p.voltState) - 1);
                    strncpy(p.currState, t[10].c_str(), sizeof(p.currState) - 1);
                    strncpy(p.tempState, t[11].c_str(), sizeof(p.tempState) - 1);
                    p.socPercent = t[12].toInt();
                    if (t.size() >= 15) {
                        String timeStr = t[13] + " " + t[14];
                        strncpy(p.timestamp, timeStr.c_str(), sizeof(p.timestamp) - 1);
                    }
                    if (mosColIndex >= 0 && (size_t)mosColIndex < t.size()) {
                        p.mosTempMdeg = t[mosColIndex].toInt();
                    } else if (t.size() >= 18) {
                        p.mosTempMdeg = t[17].toInt();
                    }
                }
                p.valid = true;
            }
        }

        if (detectedCount == 0 && stack.modules[currentModIdx].power.valid) {
            detectedCount = 1;
        }
        stack.moduleCount = detectedCount;
        stack.isMaster = (detectedCount > 1);
        return (detectedCount > 0);
    }

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
        // In Pylontech battery stacks, only master units report SOH in %.
        // Slave units report SOH : 0 (as alarm indicator). We do not synthesize artificial values.
        if (strlen(parsedSt.sohStatus) == 0) {
            strncpy(parsedSt.sohStatus, "Normal", sizeof(parsedSt.sohStatus) - 1);
        }
        mod.stats = parsedSt;
        return true;
    }

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

            // Check if first token is a valid cell number (0 to 14)
            char *endptr = nullptr;
            long cellIdx = strtol(t[0].c_str(), &endptr, 10);
            if (*endptr != '\0' || cellIdx < 0 || cellIdx >= MAX_CELLS_PER_MODULE) continue;

            CellInfo &c = mod.cells[cellIdx];
            c.voltMv = t[1].toInt();
            c.currMa = t[2].toInt();
            c.tempMdeg = t[3].toInt();
            strncpy(c.state, t[4].c_str(), sizeof(c.state) - 1);

            // Find SOC and BAL tokens from the line
            // In Model D: t[9]=SOC (e.g. "62%"), t[10]=Coulomb ("46100"), t[11]="mAH", t[12]="N"
            // In Model C: t[8]=SOC ("100%"), t[9]=Coulomb ("71572"), t[10]="mAH", t[11]="N"
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
            // bat columns: idx volt curr tempr baseState voltState currState tempState SOC% coulomb mAH BAL
            // Extract status strings (t[4..7]) as fallback for ModulePower when pwr didn't provide them.
            // All cells in a normal module report the same state — use the first cell parsed.
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
