#pragma once
#include <Arduino.h>
#include "Config.h"

enum BatteryModel {
    MODEL_UNKNOWN = 0,
    MODEL_US3000C = 1,
    MODEL_US3000D = 2
};

inline const char* modelToString(BatteryModel model) {
    switch (model) {
        case MODEL_US3000C: return "US3000C";
        case MODEL_US3000D: return "US3000D";
        default: return "Unknown";
    }
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
    char     batVoltState[16] = {0};   // B.V.St  (Battery Voltage Status — tabular pwr column 15/19)
    char     batTempState[16] = {0};   // B.T.St  (Battery Temperature Status — tabular pwr column 16/20)
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
    // Model D reports Dsg Cap directly in Ah (e.g. 133958 Ah for 2013 cycles).
    // Model C reports Dsg Cap in mAh (e.g. 27670648 mAh -> 27670.6 Ah for 373 cycles).
    if (stackModel == MODEL_US3000D || st.chargeSecs > 0) {
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
