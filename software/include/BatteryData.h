#pragma once
#include <Arduino.h>
#include "Config.h"

enum BatteryModel {
    MODEL_UNKNOWN  = 0,
    MODEL_US2000   = 1,
    MODEL_US2000C  = 2,
    MODEL_US3000   = 3,
    MODEL_US3000C  = 4,
    MODEL_US3000D  = 5,
    MODEL_UP5000   = 6,
    MODEL_FORCE_L1 = 7,
    MODEL_FORCE_L2 = 8
};

struct ModelProfile {
    BatteryModel model;
    const char*  name;                // Display model name (e.g. "US3000C")
    const char*  matchSubstrings[4];  // Match keywords in info response
    bool         supportsEuro;        // Supports 'euro' command (e.g. Model D)
    bool         supportsSohCmd;      // Supports 'soh' command (e.g. Model C, US2000C)
    bool         slaveSupportsStat;   // Whether slave modules accept 'stat <n>'
    bool         slaveSupportsInfo;   // Whether slave modules accept 'info <n>'
    bool         slaveSupportsSoh;    // Whether slave modules accept 'soh <n>'
    bool         hasMosfetTemp;       // Expected to have hardware MOSFET temp
    uint8_t      defaultCellCount;    // Default cell count (usually 15, UP5000 is 16)
};

static const ModelProfile MODEL_REGISTRY[] = {
    { MODEL_US3000C,  "US3000C",  {"US3000C",  "3000C",   nullptr, nullptr}, false, true,  true,  true,  true,  false, 15 },
    { MODEL_US3000D,  "US3000D",  {"US3000D",  "3000D",   nullptr, nullptr}, true,  false, false, false, false, true,  15 },
    { MODEL_US2000C,  "US2000C",  {"US2000C",  "2000C",   nullptr, nullptr}, false, true,  true,  true,  true,  false, 15 },
    { MODEL_US2000,   "US2000",   {"US2000",   "2000B",   "2000",  nullptr}, false, false, true,  true,  false, false, 15 },
    { MODEL_US3000,   "US3000",   {"US3000",   "3000B",   nullptr, nullptr}, false, false, true,  true,  false, false, 15 },
    { MODEL_UP5000,   "UP5000",   {"UP5000",   "5000",    nullptr, nullptr}, false, true,  true,  true,  true,  false, 16 },
    { MODEL_FORCE_L1, "Force-L1", {"Force-L1", "FORCEL1", nullptr, nullptr}, false, true,  false, false, false, false, 15 },
    { MODEL_FORCE_L2, "Force-L2", {"Force-L2", "FORCEL2", nullptr, nullptr}, false, true,  false, false, false, false, 15 },
};

inline const ModelProfile* getModelProfile(BatteryModel model) {
    for (size_t i = 0; i < sizeof(MODEL_REGISTRY) / sizeof(MODEL_REGISTRY[0]); ++i) {
        if (MODEL_REGISTRY[i].model == model) return &MODEL_REGISTRY[i];
    }
    static const ModelProfile UNKNOWN_PROFILE = { MODEL_UNKNOWN, "Unknown", {nullptr}, false, false, false, false, false, false, 15 };
    return &UNKNOWN_PROFILE;
}

inline const ModelProfile* detectModelProfile(const char* deviceNameOrInfo) {
    if (!deviceNameOrInfo || deviceNameOrInfo[0] == '\0') return getModelProfile(MODEL_UNKNOWN);
    String s(deviceNameOrInfo);
    s.toUpperCase();
    for (size_t i = 0; i < sizeof(MODEL_REGISTRY) / sizeof(MODEL_REGISTRY[0]); ++i) {
        for (int k = 0; k < 4; ++k) {
            const char* sub = MODEL_REGISTRY[i].matchSubstrings[k];
            if (sub != nullptr && s.indexOf(sub) >= 0) {
                return &MODEL_REGISTRY[i];
            }
        }
    }
    return getModelProfile(MODEL_UNKNOWN);
}

inline const char* modelToString(BatteryModel model) {
    return getModelProfile(model)->name;
}

struct CellInfo {
    uint16_t voltMv = 0;
    int32_t  currMa = 0;
    int32_t  tempMdeg = 0;
    uint8_t  socPercent = 0;
    uint32_t coulombMah = 0;
    bool     balance = false;
    char     state[12] = {0};
    uint16_t sohCount = 0;
    char     sohStatus[12] = {0};
    bool     sohValid = false;
};

struct EuroStats {
    uint32_t remainCapAh = 0;
    uint32_t remainPowerWatts = 0;
    uint32_t roundTripEff = 0;
    uint32_t selfDsgRate = 0;
    uint32_t resistanceMilliOhm = 0;
    uint64_t energyThroughputWh = 0;
    uint64_t capacityThroughputAh = 0;
    uint32_t deepDsgCount = 0;
    uint32_t chgDsgCycle = 0;
    bool     valid = false;
};

struct ModuleInfo {
    char manufacturer[32] = {0};
    char deviceName[32] = {0};
    char boardVersion[32] = {0};
    char board[32] = {0};
    char mainSoftVersion[32] = {0};
    char softVersion[32] = {0};
    char bootVersion[32] = {0};
    char commVersion[32] = {0};
    char releaseDate[32] = {0};
    char barcode[64] = {0};
    char specification[32] = {0};
    uint8_t cellCount = 15;
    int32_t maxDischargeCurrentMa = 0;
    int32_t maxChargeCurrentMa = 0;
    bool valid = false;
};

struct ModulePower {
    int32_t  voltMv = 0;
    int32_t  currMa = 0;
    int32_t  tempMdeg = 0;
    int32_t  tempLowMdeg = 0;
    int32_t  tempHighMdeg = 0;
    int32_t  voltLowMv = 0;
    int32_t  voltHighMv = 0;
    int32_t  mosTempMdeg = 0;
    int16_t  socPercent = 0;
    char     baseState[16] = {0};
    char     voltState[16] = {0};
    char     currState[16] = {0};
    char     tempState[16] = {0};
    char     mosTempState[16] = {0};
    char     batVoltState[16] = {0};   // B.V.St  (Battery Voltage Status — tabular pwr column)
    char     batTempState[16] = {0};   // B.T.St  (Battery Temperature Status — tabular pwr column)
    char     sohState[16] = {0};
    char     timestamp[32] = {0};
    bool     valid = false;
};

struct ModuleStats {
    uint32_t cycleTimes = 0;
    int16_t  sohPercent = 0;
    char     sohStatus[16] = {0};
    uint32_t sohTimes = 0;
    uint32_t dischargedCapMah = 0;
    uint64_t coulombMc = 0;          // Model C coulomb in mC
    uint32_t chargeTimes = 0;
    uint32_t dischargeTimes = 0;
    uint32_t idleTimes = 0;
    uint32_t shutTimes = 0;
    uint32_t resetTimes = 0;
    uint32_t powerOnTimes = 0;
    
    // Alarms / Warnings
    uint32_t cocTimes = 0;
    uint32_t docTimes = 0;
    uint32_t scTimes = 0;
    uint32_t batOvTimes = 0;
    uint32_t batLvTimes = 0;
    uint32_t batUvTimes = 0;
    uint32_t pwrOvTimes = 0;
    uint32_t pwrUvTimes = 0;
    uint32_t cotTimes = 0;
    uint32_t cutTimes = 0;
    uint32_t dotTimes = 0;
    uint32_t dutTimes = 0;

    // Detailed Alarms / Warnings / Diagnostics (US3000C & US3000D)
    uint32_t cocaTimes = 0;
    uint32_t docaTimes = 0;
    uint32_t batHvTimes = 0;
    uint32_t batSlpTimes = 0;
    uint32_t pwrHvTimes = 0;
    uint32_t pwrLvTimes = 0;
    uint32_t pwrSlpTimes = 0;
    uint32_t chtTimes = 0;
    uint32_t cltTimes = 0;
    uint32_t dhtTimes = 0;
    uint32_t dltTimes = 0;
    uint32_t rvTimes = 0;
    uint32_t inputOvTimes = 0;
    uint32_t bmicErrTimes = 0;
    uint32_t lifeWarnTimes = 0;
    uint32_t lifeAlarmTimes = 0;

    // Model D Operating Duration Counters (seconds)
    uint32_t chargeSecs = 0;
    uint32_t dischargeSecs = 0;
    bool     valid = false;
};

inline int16_t getEffectiveSoh(const ModuleStats &st) {
    if (st.sohPercent > 0) return st.sohPercent;
    return -1;
}

inline float getDischargedCapAh(const ModuleStats &st, BatteryModel stackModel = MODEL_UNKNOWN) {
    if (!st.valid) return 0.0f;
    const ModelProfile* prof = getModelProfile(stackModel);
    if (prof->supportsEuro || stackModel == MODEL_US3000D || st.chargeSecs > 0) {
        return (float)st.dischargedCapMah;
    }
    if (st.dischargedCapMah > 5000000 || st.coulombMc > 0) {
        return st.dischargedCapMah / 1000.0f;
    }
    return st.dischargedCapMah / 1000.0f;
}

struct BatteryModule {
    bool        present = false;
    uint8_t     index = 0;           // 1 to 16
    ModuleInfo  info;
    ModulePower power;
    ModuleStats stats;
    EuroStats   euro;
    CellInfo    cells[MAX_CELLS_PER_MODULE];
    uint8_t     cellCountParsed = 0;
};

struct BatteryStack {
    BatteryModel model = MODEL_UNKNOWN;
    char         modelName[32] = "Unknown";
    uint8_t      moduleCount = 0;
    uint8_t      activeModuleIndex = 1;
    bool         isMaster = false;
    bool         scrapeSuccess = false;
    uint32_t     scrapeDurationMs = 0;
    uint32_t     lastScrapeMillis = 0;
    time_t       lastScrapeTimestamp = 0;
    BatteryModule modules[MAX_MODULES + 1]; // 1-indexed (modules[1]..modules[16])
};
