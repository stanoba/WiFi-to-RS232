#pragma once
#include <Arduino.h>
#include "BmsModel.h"
#include "BmsPhysics.h"

// =============================================================================
// Helper string formatters for Pylontech CLI Protocol
// =============================================================================

inline String getBaseStateStr(int32_t currentMa) {
    if (currentMa > 50) return "Charge";
    if (currentMa < -50) return "Dischg";
    return "Idle";
}

inline String getTimestampStr() {
    // Return formatted timestamp or fixed simulated RTC
    return "2026-10-03 13:00:00";
}

class BmsProtocolFormatter {
public:
    static String formatHelp(const ModuleData &master) {
        String out = "@\n";
        out += "help\n";
        out += "pwr [c]\n";
        out += "bat [c]\n";
        out += "info [c]\n";
        out += "stat [c]\n";
        const ModelDescriptor &desc = getModelDescriptor(master.model_type);
        if (desc.supports_soh) {
            out += "soh [c]\n";
        }
        if (desc.supports_euro) {
            out += "euro\n";
        }
        out += "time\n";
        out += "unit\n";
        out += "log\n";
        out += "login\n";
        out += "logout\n";
        out += "$$\n";
        out += "pylon>";
        return out;
    }

    static String formatInfo(const StackData &stack, int specificModule = 1) {
        if (stack.module_count == 0) return "@\n$$\npylon>";
        int targetMod = (specificModule > 0) ? specificModule : 1;
        if (targetMod > stack.module_count) {
            return "@\nInvalid command or fail to excute!\n$$\npylon>";
        }

        const ModuleData &mod = stack.modules[targetMod - 1];
        const ModelDescriptor &desc = getModelDescriptor(mod.model_type);

        String out = "@\n\n";
        out += "Device address      : " + String(mod.id) + "\n";
        out += "Manufacturer        : Pylon\n";
        out += "Device name         : " + String(desc.name) + "\n";
        out += "Board version       : " + String(desc.type == MODEL_US3000D ? "" : "V10R04") + "\n";
        out += "Board               : " + String(desc.type == MODEL_US3000D ? "NF4.E3" : "NF4.E2") + "\n";
        out += "Main Soft version   : " + mod.fw_version + "\n";
        out += "Soft  version       : " + String(desc.type == MODEL_US3000D ? "V1.1" : "V1.4") + "\n";
        out += "Boot  version       : " + String(desc.type == MODEL_US3000D ? "V0.02" : "V1.0") + "\n";
        out += "Comm version        : V2.0\n";
        out += "Release Date        : " + String(desc.type == MODEL_US3000D ? "25-11-13" : "22-01-24") + "\n";
        out += "Barcode             : " + mod.barcode + "\n";
        out += "\n";
        out += "Specification       : 48V/" + String((int)desc.nominal_ah) + "AH\n";
        out += "Cell Number         : " + String(mod.cell_count) + "\n";
        out += "Max Dischg Curr     : -" + String((int)(desc.nominal_ah * 1000.0f)) + "mA\n";
        out += "Max Charge Curr     : " + String((int)(desc.nominal_ah * 1000.0f)) + "mA\n";
        out += "EPONPort rate       : 1200\n";
        out += "Console Port rate   : 115200\n";
        out += "Command completed successfully\n";
        out += "$$\n";
        out += "pylon>";
        return out;
    }

    static String formatPwr(const StackData &stack) {
        String out = "@\n";
        if (stack.module_count == 0) {
            out += "$$\npylon>";
            return out;
        }

        const ModuleData &master = stack.modules[0];
        const ModelDescriptor &masterDesc = getModelDescriptor(master.model_type);

        if (masterDesc.type == MODEL_US3000D) {
            // 22 columns with sensor IDs
            out += "Power Volt   Curr   Tempr  Tlow   Tlow.Id  Thigh  Thigh.Id Vlow   Vlow.Id  Vhigh  Vhigh.Id Base.St  Volt.St  Curr.St  Temp.St  Coulomb  Time                 B.V.St   B.T.St   MosTempr M.T.St\n";
        } else {
            // 18 columns standard (US2000C, US3000C, US5000, UP5000)
            out += "Power Volt   Curr   Tempr  Tlow   Thigh  Vlow   Vhigh  Base.St  Volt.St  Curr.St  Temp.St  Coulomb  Time                 B.V.St   B.T.St   MosTempr M.T.St\n";
        }

        for (uint8_t i = 0; i < 16; i++) {
            if (i < stack.module_count) {
                const ModuleData &mod = stack.modules[i];

                // Find min/max cell voltages and indices
                uint16_t vMin = 9999, vMax = 0;
                uint8_t vMinId = 1, vMaxId = 1;
                for (uint8_t c = 0; c < mod.cell_count; c++) {
                    if (mod.cells[c].voltage_mv < vMin) { vMin = mod.cells[c].voltage_mv; vMinId = c + 1; }
                    if (mod.cells[c].voltage_mv > vMax) { vMax = mod.cells[c].voltage_mv; vMaxId = c + 1; }
                }

                // Find min/max temps
                int32_t tMin = (int32_t)(mod.temp_sensors[0] * 1000);
                int32_t tMax = tMin;
                uint8_t tMinId = 1, tMaxId = 1;
                for (uint8_t t = 1; t < 4; t++) {
                    int32_t tVal = (int32_t)(mod.temp_sensors[t] * 1000);
                    if (tVal < tMin) { tMin = tVal; tMinId = t + 1; }
                    if (tVal > tMax) { tMax = tVal; tMaxId = t + 1; }
                }
                int32_t tAvg = (int32_t)(((mod.temp_sensors[0] + mod.temp_sensors[1] + mod.temp_sensors[2] + mod.temp_sensors[3]) / 4.0f) * 1000);
                int32_t mosT = (int32_t)(mod.mos_tempr * 1000);

                String baseSt = getBaseStateStr(mod.current_ma);
                String socStr = String((int)round(mod.soc)) + "%";
                char lineBuf[256];

                if (masterDesc.type == MODEL_US3000D) {
                    snprintf(lineBuf, sizeof(lineBuf),
                        "%-5d %-6ld %-6ld %-6ld %-6ld %-8d %-6ld %-8d %-6u %-8d %-6u %-8d %-8s %-8s %-8s %-8s %-8s %-20s %-8s %-8s %-8ld %-8s\n",
                        (int)(i + 1), (long)mod.voltage_mv, (long)mod.current_ma, (long)tAvg,
                        (long)tMin, tMinId, (long)tMax, tMaxId,
                        vMin, vMinId, vMax, vMaxId,
                        baseSt.c_str(), mod.volt_st.c_str(), mod.curr_st.c_str(), mod.temp_st.c_str(),
                        socStr.c_str(), getTimestampStr().c_str(),
                        mod.b_v_st.c_str(), mod.b_t_st.c_str(), (long)mosT, mod.mos_temp_st.c_str()
                    );
                } else {
                    snprintf(lineBuf, sizeof(lineBuf),
                        "%-5d %-6ld %-6ld %-6ld %-6ld %-6ld %-6u %-6u %-8s %-8s %-8s %-8s %-8s %-20s %-8s %-8s %-8ld %-8s\n",
                        (int)(i + 1), (long)mod.voltage_mv, (long)mod.current_ma, (long)tAvg,
                        (long)tMin, (long)tMax, vMin, vMax,
                        baseSt.c_str(), mod.volt_st.c_str(), mod.curr_st.c_str(), mod.temp_st.c_str(),
                        socStr.c_str(), getTimestampStr().c_str(),
                        mod.b_v_st.c_str(), mod.b_t_st.c_str(), (long)mosT, mod.mos_temp_st.c_str()
                    );
                }
                out += lineBuf;
            } else {
                // Absent slot (empty row)
                char lineBuf[256];
                if (masterDesc.type == MODEL_US3000D) {
                    snprintf(lineBuf, sizeof(lineBuf),
                        "%-5d %-6s %-6s %-6s %-6s %-8s %-6s %-8s %-8s %-8s %-6s %-8s %-8s %-8s %-8s %-8s %-8s %-20s %-8s %-8s %-8s %-8s\n",
                        (int)(i + 1), "-", "-", "-", "-", "-", "-", "-", "Absent", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-"
                    );
                } else {
                    snprintf(lineBuf, sizeof(lineBuf),
                        "%-5d %-6s %-6s %-6s %-6s %-6s %-6s %-6s %-8s %-8s %-8s %-8s %-8s %-20s %-8s %-8s\n",
                        (int)(i + 1), "-", "-", "-", "-", "-", "-", "-", "Absent", "-", "-", "-", "-", "-", "-", "-"
                    );
                }
                out += lineBuf;
            }
        }
        out += "Command completed successfully\n";
        out += "$$\n";
        out += "pylon>";
        return out;
    }

    static String formatBat(const StackData &stack, int specificModule = 1) {
        if (stack.module_count == 0) {
            return "@\n$$\npylon>";
        }
        int targetMod = (specificModule > 0) ? specificModule : 1;
        if (targetMod > stack.module_count) {
            return "@\nInvalid command or fail to excute!\n$$\npylon>";
        }

        const ModuleData &mod = stack.modules[targetMod - 1];
        const ModelDescriptor &desc = getModelDescriptor(mod.model_type);

        String out = "@\n";
        if (desc.type == MODEL_US3000D) {
            out += "\nBattery  Volt     Curr     Tempr    Base State   Volt. State  Curr. State  DTemp. State CTemp. State SOC          Coulomb          BAL\n";
        } else {
            out += "Battery  Volt     Curr     Tempr    Base State   Volt. State  Curr. State  Temp. State  SOC          Coulomb      BAL\n";
        }

        String baseSt = getBaseStateStr(mod.current_ma);
        String socStr = String((int)round(mod.soc)) + "%";
        uint32_t coulombMah = (uint32_t)(desc.nominal_ah * 1000.0f * (mod.soc / 100.0f));

        for (uint8_t c = 0; c < mod.cell_count; c++) {
            uint8_t tIdx = (c * 4) / mod.cell_count;
            if (tIdx > 3) tIdx = 3;
            int32_t cellTemp = (int32_t)(mod.temp_sensors[tIdx] * 1000);

            String balStr = mod.cells[c].balancing ? "Y" : "N";

            char lineBuf[256];
            if (desc.type == MODEL_US3000D) {
                snprintf(lineBuf, sizeof(lineBuf),
                    "%-8d %-8u %-8ld %-8ld %-12s %-12s %-12s %-12s %-12s %-12s %-12lu mAH %s\n",
                    c, mod.cells[c].voltage_mv, (long)mod.current_ma, (long)cellTemp,
                    baseSt.c_str(), mod.volt_st.c_str(), mod.curr_st.c_str(),
                    mod.dtemp_st.c_str(), mod.ctemp_st.c_str(),
                    socStr.c_str(), (unsigned long)coulombMah, balStr.c_str()
                );
            } else {
                snprintf(lineBuf, sizeof(lineBuf),
                    "%-8d %-8u %-8ld %-8ld %-12s %-12s %-12s %-12s %-12s %-10lu mAH      %s\n",
                    c, mod.cells[c].voltage_mv, (long)mod.current_ma, (long)cellTemp,
                    baseSt.c_str(), mod.volt_st.c_str(), mod.curr_st.c_str(),
                    mod.temp_st.c_str(),
                    socStr.c_str(), (unsigned long)coulombMah, balStr.c_str()
                );
            }
            out += lineBuf;
        }

        out += "Command completed successfully\n";
        out += "$$\n";
        out += "pylon>";
        return out;
    }

    static String formatStat(const StackData &stack, int specificModule = 1) {
        if (stack.module_count == 0) return "@\n$$\npylon>";
        int targetMod = (specificModule > 0) ? specificModule : 1;
        if (targetMod > stack.module_count) {
            return "@\nInvalid command or fail to excute!\n$$\npylon>";
        }

        const ModuleData &mod = stack.modules[targetMod - 1];
        const ModelDescriptor &desc = getModelDescriptor(mod.model_type);

        if (!desc.supports_slave_stat && targetMod > 1) {
            return "@\nInvalid command or fail to excute!\n$$\npylon>";
        }

        String out = "@\n";
        char addrBuf[64];
        snprintf(addrBuf, sizeof(addrBuf), "Device address           %d\n", mod.id);
        out += addrBuf;

        if (desc.type == MODEL_US3000D) {
            char b[128];
            snprintf(b, sizeof(b), "Charge Secs.    : %8lu\n", (unsigned long)mod.chg_times_or_secs); out += b;
            snprintf(b, sizeof(b), "Discharge Secs. : %8lu\n", (unsigned long)mod.dsg_times_or_secs); out += b;
            snprintf(b, sizeof(b), "Charge Times    : %8lu\n", 0UL); out += b;
            snprintf(b, sizeof(b), "Discharge Times : %8lu\n", 0UL); out += b;
            snprintf(b, sizeof(b), "Idle Times      : %8lu\n", (unsigned long)mod.idle_times); out += b;
            snprintf(b, sizeof(b), "COC2 Times      : %8lu\n", 0UL); out += b;
            snprintf(b, sizeof(b), "DOC2 Times      : %8lu\n", 0UL); out += b;
            snprintf(b, sizeof(b), "COC Times       : %8lu\n", (unsigned long)mod.coc_times); out += b;
            snprintf(b, sizeof(b), "DOC Times       : %8lu\n", (unsigned long)mod.doc_times); out += b;
            snprintf(b, sizeof(b), "COCA Times      : %8lu\n", (unsigned long)mod.coca_times); out += b;
            snprintf(b, sizeof(b), "DOCA Times      : %8lu\n", (unsigned long)mod.doca_times); out += b;
            snprintf(b, sizeof(b), "SC Times        : %8lu\n", (unsigned long)mod.sc_times); out += b;
            snprintf(b, sizeof(b), "Bat OV Times    : %8lu\n", (unsigned long)mod.bat_ov_times); out += b;
            snprintf(b, sizeof(b), "Bat HV Times    : %8lu\n", (unsigned long)mod.bat_hv_times); out += b;
            snprintf(b, sizeof(b), "Bat LV Times    : %8lu\n", (unsigned long)mod.bat_lv_times); out += b;
            snprintf(b, sizeof(b), "Bat UV Times    : %8lu\n", (unsigned long)mod.bat_uv_times); out += b;
            snprintf(b, sizeof(b), "Bat SLP Times   : %8lu\n", (unsigned long)mod.bat_slp_times); out += b;
            snprintf(b, sizeof(b), "Pwr OV Times    : %8lu\n", (unsigned long)mod.pwr_ov_times); out += b;
            snprintf(b, sizeof(b), "Pwr HV Times    : %8lu\n", (unsigned long)mod.pwr_hv_times); out += b;
            snprintf(b, sizeof(b), "Pwr LV Times    : %8lu\n", (unsigned long)mod.pwr_lv_times); out += b;
            snprintf(b, sizeof(b), "Pwr UV Times    : %8lu\n", (unsigned long)mod.pwr_uv_times); out += b;
            snprintf(b, sizeof(b), "Pwr SLP Times   : %8lu\n", (unsigned long)mod.pwr_slp_times); out += b;
            snprintf(b, sizeof(b), "COT Times       : %8lu\n", (unsigned long)mod.cot_times); out += b;
            snprintf(b, sizeof(b), "CUT Times       : %8lu\n", (unsigned long)mod.cut_times); out += b;
            snprintf(b, sizeof(b), "DOT Times       : %8lu\n", (unsigned long)mod.dot_times); out += b;
            snprintf(b, sizeof(b), "DUT Times       : %8lu\n", (unsigned long)mod.dut_times); out += b;
            snprintf(b, sizeof(b), "CHT Times       : %8lu\n", (unsigned long)mod.cht_times); out += b;
            snprintf(b, sizeof(b), "CLT Times       : %8lu\n", (unsigned long)mod.clt_times); out += b;
            snprintf(b, sizeof(b), "DLT Times       : %8lu\n", (unsigned long)mod.dlt_times); out += b;
            snprintf(b, sizeof(b), "DHT Times       : %8lu\n", (unsigned long)mod.dht_times); out += b;
            snprintf(b, sizeof(b), "RV Times        : %8lu\n", (unsigned long)mod.rv_times); out += b;
            snprintf(b, sizeof(b), "Input OV Times  : %8lu\n", (unsigned long)mod.input_ov_times); out += b;
            snprintf(b, sizeof(b), "SOH Times       : %8lu\n", (unsigned long)mod.soh_times); out += b;
            snprintf(b, sizeof(b), "BMICERR Times   : %8lu\n", (unsigned long)mod.bmic_err_times); out += b;
            snprintf(b, sizeof(b), "CYCLE Times     : %8lu\n", (unsigned long)mod.cycle_times); out += b;
            snprintf(b, sizeof(b), "SOH             : %8lu\n", (unsigned long)round(mod.soh_pct)); out += b;
            snprintf(b, sizeof(b), "Dsg Cap         : %8lu\n", (unsigned long)mod.dsg_cap_total); out += b;
            snprintf(b, sizeof(b), "HT@0.5C Secs    : %8lu\n", 0UL); out += b;
            snprintf(b, sizeof(b), "LT@0.5C Secs    : %8lu\n", 0UL); out += b;
            snprintf(b, sizeof(b), "HT Secs         : %8lu\n", 0UL); out += b;
            snprintf(b, sizeof(b), "LT Secs         : %8lu\n", 0UL); out += b;
            snprintf(b, sizeof(b), "LV Secs         : %8lu\n", 0UL); out += b;
            snprintf(b, sizeof(b), "SOC LOW Secs    : %8lu\n", 0UL); out += b;
            snprintf(b, sizeof(b), "Shut Times      : %8lu\n", (unsigned long)mod.shut_times); out += b;
            snprintf(b, sizeof(b), "Reset Times     : %8lu\n", (unsigned long)mod.rst_times); out += b;
            snprintf(b, sizeof(b), "Power on Times  : %8lu\n", (unsigned long)mod.power_on_times); out += b;
            out += "Command completed successfully!\n";
        } else {
            // US3000C / US2000C / US5000 / UP5000
            char b[128];
            snprintf(b, sizeof(b), "Data Items      : %8d\n", 4); out += b;
            snprintf(b, sizeof(b), "HisData Items   : %8d\n", 1799); out += b;
            snprintf(b, sizeof(b), "Charge Cnt.     : %8lu\n", 0UL); out += b;
            snprintf(b, sizeof(b), "Discharge Cnt.  : %8lu\n", (unsigned long)mod.dsg_times_or_secs); out += b;
            snprintf(b, sizeof(b), "Charge Times    : %8lu\n", (unsigned long)mod.chg_times_or_secs); out += b;
            snprintf(b, sizeof(b), "Status Cnt.     : %8lu\n", 321UL); out += b;
            snprintf(b, sizeof(b), "Idle Times      : %8lu\n", (unsigned long)mod.idle_times); out += b;
            snprintf(b, sizeof(b), "COC Times       : %8lu\n", (unsigned long)mod.coc_times); out += b;
            snprintf(b, sizeof(b), "COC2 Times      : %8lu\n", 0UL); out += b;
            snprintf(b, sizeof(b), "DOC Times       : %8lu\n", (unsigned long)mod.doc_times); out += b;
            snprintf(b, sizeof(b), "DOC2 Times      : %8lu\n", 0UL); out += b;
            snprintf(b, sizeof(b), "COCA Times      : %8lu\n", (unsigned long)mod.coca_times); out += b;
            snprintf(b, sizeof(b), "DOCA Times      : %8lu\n", (unsigned long)mod.doca_times); out += b;
            snprintf(b, sizeof(b), "SC Times        : %8lu\n", (unsigned long)mod.sc_times); out += b;
            snprintf(b, sizeof(b), "Bat OV Times    : %8lu\n", (unsigned long)mod.bat_ov_times); out += b;
            snprintf(b, sizeof(b), "Bat HV Times    : %8lu\n", (unsigned long)mod.bat_hv_times); out += b;
            snprintf(b, sizeof(b), "Bat LV Times    : %8lu\n", (unsigned long)mod.bat_lv_times); out += b;
            snprintf(b, sizeof(b), "Bat UV Times    : %8lu\n", (unsigned long)mod.bat_uv_times); out += b;
            snprintf(b, sizeof(b), "Bat SLP Times   : %8lu\n", (unsigned long)mod.bat_slp_times); out += b;
            snprintf(b, sizeof(b), "Pwr OV Times    : %8lu\n", (unsigned long)mod.pwr_ov_times); out += b;
            snprintf(b, sizeof(b), "Pwr HV Times    : %8lu\n", (unsigned long)mod.pwr_hv_times); out += b;
            snprintf(b, sizeof(b), "Pwr LV Times    : %8lu\n", (unsigned long)mod.pwr_lv_times); out += b;
            snprintf(b, sizeof(b), "Pwr UV Times    : %8lu\n", (unsigned long)mod.pwr_uv_times); out += b;
            snprintf(b, sizeof(b), "Pwr SLP Times   : %8lu\n", (unsigned long)mod.pwr_slp_times); out += b;
            snprintf(b, sizeof(b), "COT Times       : %8lu\n", (unsigned long)mod.cot_times); out += b;
            snprintf(b, sizeof(b), "CUT Times       : %8lu\n", (unsigned long)mod.cut_times); out += b;
            snprintf(b, sizeof(b), "DOT Times       : %8lu\n", (unsigned long)mod.dot_times); out += b;
            snprintf(b, sizeof(b), "DUT Times       : %8lu\n", (unsigned long)mod.dut_times); out += b;
            snprintf(b, sizeof(b), "CHT Times       : %8lu\n", (unsigned long)mod.cht_times); out += b;
            snprintf(b, sizeof(b), "CLT Times       : %8lu\n", (unsigned long)mod.clt_times); out += b;
            snprintf(b, sizeof(b), "DHT Times       : %8lu\n", (unsigned long)mod.dht_times); out += b;
            snprintf(b, sizeof(b), "DLT Times       : %8lu\n", (unsigned long)mod.dlt_times); out += b;
            snprintf(b, sizeof(b), "Shut Times      : %8lu\n", (unsigned long)mod.shut_times); out += b;
            snprintf(b, sizeof(b), "Reset Times     : %8lu\n", (unsigned long)mod.rst_times); out += b;
            snprintf(b, sizeof(b), "RV Times        : %8lu\n", (unsigned long)mod.rv_times); out += b;
            snprintf(b, sizeof(b), "Input OV Times  : %8lu\n", (unsigned long)mod.input_ov_times); out += b;
            snprintf(b, sizeof(b), "SOH Times       : %8lu\n", (unsigned long)mod.soh_times); out += b;
            snprintf(b, sizeof(b), "BMICERR Times   : %8lu\n", (unsigned long)mod.bmic_err_times); out += b;
            snprintf(b, sizeof(b), "CYCLE Times     : %8lu\n", (unsigned long)mod.cycle_times); out += b;
            snprintf(b, sizeof(b), "SOH             : %8lu\n", (unsigned long)round(mod.soh_pct)); out += b;
            snprintf(b, sizeof(b), "Pwr Percent     : %8lu\n", (unsigned long)round(mod.soc)); out += b;
            char mcStr[32];
            snprintf(mcStr, sizeof(mcStr), "%llu", (unsigned long long)mod.pwr_coulomb_mc);
            snprintf(b, sizeof(b), "Pwr Coulomb     : %s\n", mcStr); out += b;
            snprintf(b, sizeof(b), "Dsg Cap         : %8lu\n", (unsigned long)mod.dsg_cap_total); out += b;
            snprintf(b, sizeof(b), "HT@0.5C Cnt     : %8lu\n", 0UL); out += b;
            snprintf(b, sizeof(b), "LT@0.5C Cnt     : %8lu\n", 0UL); out += b;
            snprintf(b, sizeof(b), "HT Cnt          : %8lu\n", 0UL); out += b;
            snprintf(b, sizeof(b), "LT Cnt          : %8lu\n", 0UL); out += b;
            snprintf(b, sizeof(b), "LV Cnt          : %8lu\n", 182UL); out += b;
            snprintf(b, sizeof(b), "LifeWarn Times  : %8lu\n", (unsigned long)mod.life_warn_times); out += b;
            snprintf(b, sizeof(b), "LifeAlarm Times : %8lu\n", (unsigned long)mod.life_alarm_times); out += b;
            out += "Command completed successfully\n";
        }
        out += "$$\n";
        out += "pylon>";
        return out;
    }

    static String formatSoh(const StackData &stack, int specificModule = 1) {
        if (stack.module_count == 0) return "@\n$$\npylon>";
        int targetMod = (specificModule > 0) ? specificModule : 1;
        if (targetMod > stack.module_count) {
            return "@\nInvalid command or fail to excute!\n$$\npylon>";
        }

        const ModuleData &mod = stack.modules[targetMod - 1];
        const ModelDescriptor &desc = getModelDescriptor(mod.model_type);
        if (!desc.supports_soh) {
            return "@\nUnknown command\n$$\npylon>";
        }

        String out = "@\n";
        out += "Power   " + String(mod.id) + "\n";
        out += "Battery    Voltage    SOHCount   SOHStatus\n";
        for (uint8_t c = 0; c < mod.cell_count; c++) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%-10d %-10u %-10u %s\n", c, mod.cells[c].voltage_mv, mod.cells[c].soh_count, "Normal");
            out += buf;
        }
        out += "Command completed successfully\n";
        out += "$$\n";
        out += "pylon>";
        return out;
    }

    static String formatEuro(const StackData &stack, int specificModule = 1) {
        if (stack.module_count == 0) return "@\n$$\npylon>";
        int targetMod = (specificModule > 0) ? specificModule : 1;
        if (targetMod > stack.module_count) {
            return "@\nInvalid command or fail to excute!\n$$\npylon>";
        }

        const ModuleData &mod = stack.modules[targetMod - 1];
        const ModelDescriptor &desc = getModelDescriptor(mod.model_type);
        if (!desc.supports_euro) {
            return "@\nUnknown command\n$$\npylon>";
        }

        const EuroStats &e = mod.euro;
        String out = "@\n\n";
        out += "===========================================================\n";
        out += "Date of manufacture           : ---------- --:--:--\n";
        out += "Date of putting into service  : ---------- --:--:--\n";
        out += "Storage                       :         - days\n";
        out += "SOH:\n";
        char b[128];
        snprintf(b, sizeof(b), "Remain Cap.         : %9.0f Ah\n", e.remain_cap_ah); out += b;
        snprintf(b, sizeof(b), "Remain Power        : %9ld W\n", (long)(e.remain_cap_ah * (mod.voltage_mv / 1000.0f))); out += b;
        snprintf(b, sizeof(b), "Round Trip Eff.     : %9ld\n", (long)(e.round_trip_eff_pct * 100.0f)); out += b;
        snprintf(b, sizeof(b), "Self Dsg Rate       : %9d\n", 0); out += b;
        snprintf(b, sizeof(b), "Resistence          : %9d mOhm\n", 0); out += b;
        out += "===========================================================\n";
        out += "Life Expectance:\n";
        snprintf(b, sizeof(b), "Energy Thro.        : %9ld Wh  %lu mWs\n", (long)(e.energy_thro_kwh * 1000.0f), (unsigned long)(e.energy_thro_kwh * 3600000.0f)); out += b;
        snprintf(b, sizeof(b), "Capac. Thro.        : %9ld Ah  %lu mAs\n", (long)e.capac_thro_kwh, (unsigned long)(e.capac_thro_kwh * 36000.0f)); out += b;
        snprintf(b, sizeof(b), "Deep Dsg. Count     : %9d\n", 0); out += b;
        snprintf(b, sizeof(b), "Extr. Tempr Total   : %9d Sec.\n", 0); out += b;
        snprintf(b, sizeof(b), "Extr. Tempr Chg.    : %9d Sec.\n", 0); out += b;
        snprintf(b, sizeof(b), "Chg. Dsg. Cycle     : %9lu\n", (unsigned long)e.chg_dsg_cycle); out += b;
        out += "===========================================================\n";
        out += "Command completed successfully!\n";
        out += "$$\n";
        out += "pylon>";
        return out;
    }

    static String formatUnknown() {
        return "@\nUnknown command\n$$\npylon>";
    }
};
