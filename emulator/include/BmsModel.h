#pragma once
#include <Arduino.h>
#include <vector>
#include <algorithm>
#include "Config.h"

// =============================================================================
// Supported Pylontech BMS Models & Hierarchy Ranks
// =============================================================================
enum PylonModelType {
    MODEL_US2000C = 0,
    MODEL_US3000C = 1,
    MODEL_US3000D = 2,
    MODEL_US5000  = 3,
    MODEL_UP5000  = 4
};

struct ModelDescriptor {
    PylonModelType type;
    const char* name;
    uint8_t cell_count;
    float nominal_ah;
    uint8_t hierarchy_rank; // Higher = Must be placed higher in master/slave stack
    bool supports_soh;      // Supports 'soh' command (per-cell SOH table)
    bool supports_euro;     // Supports 'euro' command (European efficiency stats)
    bool supports_slave_stat; // Supports 'stat <n>' for slave modules
    uint16_t height_mm;     // Front panel faceplate height in mm (89mm=2U, 132mm=3U, 161mm=3.6U)
    const char* default_fw;
    const char* barcode_prefix;
};

static const ModelDescriptor MODEL_DESCRIPTORS[] = {
    { MODEL_US2000C, "US2000C", 15,  50.0f, 20, true,  false, true,   89, "V2.8", "PPTB" },
    { MODEL_US3000C, "US3000C", 15,  74.0f, 30, true,  false, true,  132, "V2.8", "PPYB" },
    { MODEL_US3000D, "US3000D", 15,  74.0f, 40, false, true,  false, 132, "V1.3", "PPYB" },
    { MODEL_US5000,  "US5000",  16, 100.0f, 50, true,  false, true,  161, "V1.7", "PP50" },
    { MODEL_UP5000,  "UP5000",  16, 100.0f, 45, true,  false, true,  161, "V1.5", "PUP5" }
};
#define MODEL_COUNT (sizeof(MODEL_DESCRIPTORS) / sizeof(MODEL_DESCRIPTORS[0]))

inline const ModelDescriptor& getModelDescriptor(PylonModelType type) {
    for (size_t i = 0; i < MODEL_COUNT; i++) {
        if (MODEL_DESCRIPTORS[i].type == type) return MODEL_DESCRIPTORS[i];
    }
    return MODEL_DESCRIPTORS[1]; // default US3000C
}

inline const ModelDescriptor& getModelDescriptorByName(const String& name) {
    for (size_t i = 0; i < MODEL_COUNT; i++) {
        if (name.equalsIgnoreCase(MODEL_DESCRIPTORS[i].name)) return MODEL_DESCRIPTORS[i];
    }
    return MODEL_DESCRIPTORS[1]; // default US3000C
}

// =============================================================================
// Status / Alarm String Constants
// =============================================================================
// Base.St: Idle, Charge, Dischg
// Volt.St / Temp.St / Curr.St: Normal, High, Low, Over, Under
// B.V.St / B.T.St: Normal, Alarm, Error
// BAL: - or Y/N

// =============================================================================
// Module & Stack Data Structures
// =============================================================================
struct CellData {
    uint16_t voltage_mv;   // e.g. 3328 mV
    uint16_t soh_count;    // SOH incident counter (for Model C/5000)
    uint8_t soh_status;    // 0 = Normal
    bool balancing;        // true = balancing active
};

struct EuroStats {
    float soh_pct;              // e.g. 99.0 %
    uint32_t life_expect_days;  // e.g. 5475 days (15 years)
    uint32_t chg_dsg_cycle;     // e.g. 248 cycles
    float energy_thro_kwh;      // e.g. 1423.5 kWh
    float capac_thro_kwh;       // e.g. 1450.2 kWh
    float remain_cap_ah;        // e.g. 73.20 Ah
    float round_trip_eff_pct;   // e.g. 96.5 %
};

struct ModuleData {
    uint8_t id;                 // 1-based index (1 = Master)
    PylonModelType model_type;
    String serial_number;
    String barcode;
    String fw_version;
    uint8_t cell_count;         // 15 or 16
    CellData cells[MAX_CELLS_PER_MODULE];

    // Temperatures (in °C, e.g. 24.5 °C)
    float temp_sensors[4];      // T1, T2, T3, T4 (or Tlow/Thigh)
    float mos_tempr;            // MOSFET Temperature

    // Telemetry
    float soc;                  // State of Charge (0.0 to 100.0 %)
    float soh_pct;              // State of Health (0.0 to 100.0 %)
    int32_t voltage_mv;         // Pack Voltage in mV (e.g. 49920)
    int32_t current_ma;         // Pack Current in mA (+ Charge, - Discharge)
    uint32_t coulomb_pct;       // Coulomb capacity / percentage

    // Live Simulated Alarm / Fault States
    String volt_st;             // "Normal", "High", "Low", "Over", "Under"
    String curr_st;             // "Normal", "High", "Over"
    String temp_st;             // "Normal", "High", "Low", "Over"
    String dtemp_st;            // "Normal" (US3000D discharge temp state)
    String ctemp_st;            // "Normal" (US3000D charge temp state)
    String mos_temp_st;         // "Normal", "High", "Over"
    String b_v_st;              // "Normal", "Alarm", "Error"
    String b_t_st;              // "Normal", "Alarm", "Error"

    // Current & Hardware Protections ('stat' counters)
    uint32_t coc_times;         // Charge Over-Current Cut-off
    uint32_t coca_times;        // Charge Over-Current Alarm
    uint32_t doc_times;         // Discharge Over-Current Cut-off
    uint32_t doca_times;        // Discharge Over-Current Alarm
    uint32_t sc_times;          // Short Circuit Protection
    uint32_t rv_times;          // Reverse Voltage Protection
    uint32_t input_ov_times;    // Input Over-Voltage Protection
    uint32_t bmic_err_times;    // BMS AFE/IC Hardware Fault
    uint32_t life_alarm_times;  // Lifetime Critical Alarms
    uint32_t life_warn_times;   // Lifetime System Warnings
    uint32_t bat_slp_times;     // Battery Sleep Transitions
    uint32_t pwr_slp_times;     // Power Bus Sleep Transitions

    // Voltage & Thermal Protections ('stat' counters)
    uint32_t bat_ov_times;      // Battery Over-Voltage Cut-off
    uint32_t bat_hv_times;      // Battery High-Voltage Warning
    uint32_t bat_lv_times;      // Battery Low-Voltage Warning
    uint32_t bat_uv_times;      // Battery Under-Voltage Cut-off
    uint32_t pwr_ov_times;      // Power Bus Over-Voltage Cut-off
    uint32_t pwr_hv_times;      // Power Bus High-Voltage Warning
    uint32_t pwr_lv_times;      // Power Bus Low-Voltage Warning
    uint32_t pwr_uv_times;      // Power Bus Under-Voltage Cut-off
    uint32_t cot_times;         // Charge Over-Temperature
    uint32_t cut_times;         // Charge Under-Temperature
    uint32_t dot_times;         // Discharge Over-Temperature
    uint32_t dut_times;         // Discharge Under-Temperature
    uint32_t cht_times;         // Charge High-Temperature
    uint32_t clt_times;         // Charge Low-Temperature
    uint32_t dht_times;         // Discharge High-Temperature
    uint32_t dlt_times;         // Discharge Low-Temperature

    // Operational Duration & Diagnostic Counters ('stat')
    uint32_t soh_times;         // SOH degradation incidents counter
    uint32_t cycle_times;       // Battery full cycles
    uint32_t shut_times;        // Emergency shutdown events
    uint32_t rst_times;         // BMS reset events
    uint32_t power_on_times;    // Power on times (Model D)
    uint32_t idle_times;        // Idle times
    uint32_t chg_times_or_secs; // Model C: Charge Times; Model D: Charge Secs
    uint32_t dsg_times_or_secs; // Model C: Discharge Cnt; Model D: Discharge Secs
    uint64_t pwr_coulomb_mc;    // Power Coulomb in mC (Model C)
    uint32_t dsg_cap_total;     // Model C: mAh (e.g. 24534743), Model D: Ah (e.g. 27495)

    // European Efficiency Stats ('euro', for US3000D)
    EuroStats euro;
};

struct StackData {
    uint8_t module_count;
    ModuleData modules[MAX_MODULES];

    // Global Physics & Rack Controls
    float global_soc;           // 0 - 100%
    float global_current_a;     // + Charge, - Discharge, 0.0 Idle
    float global_temp_c;        // Base Ambient Temperature
    uint16_t cell_spread_mv;    // Deviation between cells (e.g. 8 mV)
    bool auto_physics;          // Automatically recompute voltages from SOC & current
    bool sim_inverter_enabled;  // Real Inverter Dynamic Load / Solar Simulation

    // Hierarchy Validation State
    bool hierarchy_valid;
    String hierarchy_warning;
};

// Global instance
extern StackData g_stack;
void initBmsDefaults();

inline bool validateMasterHierarchy(String &outWarning) {
    if (g_stack.module_count <= 1) return true;

    const ModuleData &master = g_stack.modules[0];
    const ModelDescriptor &masterDesc = getModelDescriptor(master.model_type);

    for (uint8_t i = 1; i < g_stack.module_count; i++) {
        const ModuleData &slave = g_stack.modules[i];
        const ModelDescriptor &slaveDesc = getModelDescriptor(slave.model_type);

        // Check 1: Generation Rank Rule
        if (slaveDesc.hierarchy_rank > masterDesc.hierarchy_rank) {
            outWarning = "Slave #" + String(slave.id) + " (" + slaveDesc.name + ", Rank " + String(slaveDesc.hierarchy_rank) +
                         ") has higher priority than Master #" + String(master.id) + " (" + masterDesc.name + ", Rank " + String(masterDesc.hierarchy_rank) + ").";
            return false;
        }

        // Check 2: Firmware Version Rule (if same model)
        if (slaveDesc.type == masterDesc.type) {
            if (slave.fw_version.compareTo(master.fw_version) > 0) {
                outWarning = "Slave #" + String(slave.id) + " (" + slaveDesc.name + " FW " + slave.fw_version +
                             ") has newer firmware than Master (" + masterDesc.name + " FW " + master.fw_version + ").";
                return false;
            }
        }
    }
    return true;
}

inline void autoSortHierarchy() {
    if (g_stack.module_count <= 1) return;

    std::vector<ModuleData> mods;
    for (uint8_t i = 0; i < g_stack.module_count; i++) {
        mods.push_back(g_stack.modules[i]);
    }

    std::sort(mods.begin(), mods.end(), [](const ModuleData &a, const ModuleData &b) {
        const ModelDescriptor &da = getModelDescriptor(a.model_type);
        const ModelDescriptor &db = getModelDescriptor(b.model_type);
        if (da.hierarchy_rank != db.hierarchy_rank) {
            return da.hierarchy_rank > db.hierarchy_rank;
        }
        return a.fw_version.compareTo(b.fw_version) > 0;
    });

    for (size_t i = 0; i < mods.size(); i++) {
        mods[i].id = (uint8_t)(i + 1);
        g_stack.modules[i] = mods[i];
    }
}

// =============================================================================
// NVS Storage Structs (Atomic Single-Key Storage)
// =============================================================================
#define RACK_NVS_MAGIC 0x50594C4E // 'PYLN'
#define RACK_NVS_VERSION 1

#pragma pack(push, 1)
struct ModuleNvsRecord {
    uint8_t model_type;
    char fw_version[12];
    char barcode[32];
    float soh_pct;
    uint32_t cycle_times;
    uint32_t soh_times;
    uint32_t shut_times;
    uint32_t rst_times;
    uint32_t power_on_times;
    uint32_t idle_times;
    uint32_t chg_times_or_secs;
    uint32_t dsg_times_or_secs;
    uint32_t dsg_cap_total;

    // Current & Hardware Protections
    uint32_t coc_times;
    uint32_t coca_times;
    uint32_t doc_times;
    uint32_t doca_times;
    uint32_t sc_times;
    uint32_t rv_times;
    uint32_t input_ov_times;
    uint32_t bmic_err_times;
    uint32_t life_alarm_times;
    uint32_t life_warn_times;
    uint32_t bat_slp_times;
    uint32_t pwr_slp_times;

    // Voltage & Thermal Protections
    uint32_t bat_ov_times;
    uint32_t bat_hv_times;
    uint32_t bat_lv_times;
    uint32_t bat_uv_times;
    uint32_t pwr_ov_times;
    uint32_t pwr_hv_times;
    uint32_t pwr_lv_times;
    uint32_t puv_times;
    uint32_t cot_times;
    uint32_t cut_times;
    uint32_t dot_times;
    uint32_t dut_times;
    uint32_t cht_times;
    uint32_t clt_times;
    uint32_t dht_times;
    uint32_t dlt_times;

    // Euro Stats
    float euro_soh_pct;
    uint32_t euro_life_expect_days;
    float euro_energy_thro_kwh;
    float euro_round_trip_eff_pct;
};

struct RackNvsRecord {
    uint32_t magic;
    uint8_t version;
    uint8_t module_count;
    float global_soc;
    float global_current_a;
    float global_temp_c;
    uint16_t cell_spread_mv;
    uint8_t auto_physics;
    uint8_t reserved[15];
    ModuleNvsRecord modules[MAX_MODULES];
};
#pragma pack(pop)


