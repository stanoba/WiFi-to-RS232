#pragma once
#include <Arduino.h>
#include "BmsModel.h"

// =============================================================================
// LiFePO4 OCV & Physics Engine
// =============================================================================
// Approximate Open Circuit Voltage (mV) of LiFePO4 cell based on SOC (0.0 to 100.0%)
inline uint16_t lifepo4SocToOcvMv(float soc) {
    if (soc <= 0.0f) return 2800;
    if (soc >= 100.0f) return 3450;

    // Piecewise linear LiFePO4 curve
    struct Point { float soc; uint16_t mv; };
    static const Point OCV_POINTS[] = {
        { 0.0f,   2800 },
        { 2.0f,   3050 },
        { 5.0f,   3150 },
        { 10.0f,  3210 },
        { 20.0f,  3250 },
        { 40.0f,  3285 },
        { 60.0f,  3305 },
        { 80.0f,  3330 },
        { 90.0f,  3355 },
        { 95.0f,  3380 },
        { 99.0f,  3430 },
        { 100.0f, 3450 }
    };
    const size_t NUM_POINTS = sizeof(OCV_POINTS) / sizeof(OCV_POINTS[0]);

    for (size_t i = 0; i < NUM_POINTS - 1; i++) {
        if (soc >= OCV_POINTS[i].soc && soc <= OCV_POINTS[i + 1].soc) {
            float frac = (soc - OCV_POINTS[i].soc) / (OCV_POINTS[i + 1].soc - OCV_POINTS[i].soc);
            return (uint16_t)(OCV_POINTS[i].mv + frac * (OCV_POINTS[i + 1].mv - OCV_POINTS[i].mv));
        }
    }
    return 3300;
}

// Deterministic cell spread factor for cell 0..15 in range [-0.5, +0.5]
inline float getCellSpreadFactor(uint8_t moduleIdx, uint8_t cellIdx) {
    static const float SPREAD_FACTORS[16] = {
        -0.35f,  0.42f, -0.15f,  0.28f, -0.48f,  0.10f,  0.33f, -0.22f,
         0.45f, -0.30f,  0.18f, -0.40f,  0.25f, -0.12f,  0.38f, -0.05f
    };
    uint8_t idx = (cellIdx + moduleIdx * 3) % 16;
    return SPREAD_FACTORS[idx];
}

inline void recalculatePhysics() {
    if (!g_stack.auto_physics) return;

    float stackCurrentA = g_stack.global_current_a;
    float currentPerModA = (g_stack.module_count > 0) ? (stackCurrentA / g_stack.module_count) : 0.0f;
    int32_t currentPerModMa = (int32_t)(currentPerModA * 1000.0f);

    // Internal resistance per cell approx 0.8 mOhm = 0.0008 Ohm -> mV shift = I (A) * 0.8
    float rIntMvShift = currentPerModA * 0.85f;

    for (uint8_t m = 0; m < g_stack.module_count; m++) {
        ModuleData &mod = g_stack.modules[m];
        mod.soc = g_stack.global_soc;
        mod.current_ma = currentPerModMa;

        uint16_t baseCellMv = lifepo4SocToOcvMv(mod.soc);
        int32_t loadedCellMv = (int32_t)(baseCellMv + rIntMvShift);
        if (loadedCellMv < 2500) loadedCellMv = 2500;
        if (loadedCellMv > 3650) loadedCellMv = 3650;

        int32_t totalPackMv = 0;
        for (uint8_t c = 0; c < mod.cell_count; c++) {
            float factor = getCellSpreadFactor(m, c);
            int32_t spreadMv = (int32_t)(factor * g_stack.cell_spread_mv);
            int32_t cellV = loadedCellMv + spreadMv;
            if (cellV < 2500) cellV = 2500;
            if (cellV > 3650) cellV = 3650;
            mod.cells[c].voltage_mv = (uint16_t)cellV;
            totalPackMv += cellV;

            // Auto-balancing active if charging and high voltage & imbalance
            if (currentPerModA > 1.0f && cellV > 3400 && spreadMv > 10) {
                mod.cells[c].balancing = true;
            } else {
                mod.cells[c].balancing = false;
            }
        }
        mod.voltage_mv = totalPackMv;

        // Temperatures
        float baseT = g_stack.global_temp_c;
        // Mosfet runs slightly warmer when current is flowing
        float currentHeating = (abs(currentPerModA) * 0.12f);
        mod.temp_sensors[0] = baseT - 0.3f;
        mod.temp_sensors[1] = baseT + 0.2f;
        mod.temp_sensors[2] = baseT + 0.1f;
        mod.temp_sensors[3] = baseT - 0.2f;
        mod.mos_tempr = baseT + 1.8f + currentHeating;

        // Auto determine default states if not explicitly set to alarm
        if (mod.volt_st.length() == 0 || mod.volt_st == "Normal" || mod.volt_st == "High" || mod.volt_st == "Low") {
            if (totalPackMv > (mod.cell_count * 3550)) mod.volt_st = "High";
            else if (totalPackMv < (mod.cell_count * 3000)) mod.volt_st = "Low";
            else mod.volt_st = "Normal";
        }

        if (mod.curr_st.length() == 0 || mod.curr_st == "Normal" || mod.curr_st == "High") {
            if (abs(currentPerModA) > 65.0f) mod.curr_st = "High";
            else mod.curr_st = "Normal";
        }

        if (mod.temp_st.length() == 0 || mod.temp_st == "Normal" || mod.temp_st == "High" || mod.temp_st == "Low") {
            if (baseT > 45.0f) mod.temp_st = "High";
            else if (baseT < 5.0f) mod.temp_st = "Low";
            else mod.temp_st = "Normal";
        }

        mod.dtemp_st = mod.temp_st;
        mod.ctemp_st = mod.temp_st;
        mod.mos_temp_st = (mod.mos_tempr > 60.0f) ? "High" : "Normal";
        mod.b_v_st = "Normal";
        mod.b_t_st = "Normal";

        // Coulomb counter: approx mAh remaining
        const ModelDescriptor& desc = getModelDescriptor(mod.model_type);
        mod.coulomb_pct = (uint32_t)(mod.soc * 1000.0f); // or mAh
    }
}

// =============================================================================
// Real Inverter Load & Solar Dynamic Physics Loop
// =============================================================================
inline void updateInverterSimulation() {
    if (!g_stack.sim_inverter_enabled) return;

    static uint32_t lastTickMs = 0;
    uint32_t now = millis();
    if (lastTickMs == 0) {
        lastTickMs = now;
        return;
    }

    uint32_t dtMs = now - lastTickMs;
    if (dtMs < 1000) return; // 1s simulation tick
    lastTickMs = now;

    float dtSec = dtMs / 1000.0f;
    float tSec = now / 1000.0f;

    // Realistic Solar PV + Household Inverter Load dynamic model:
    // Macro wave (~75s cycle) simulates cycling between solar charging (+16A) and household load (-16A)
    // Micro wave (~22s cycle) simulates cloud passings and compressor/appliance cycling
    float macroWave = sin(tSec * 0.084f) * 16.5f;       // -16.5A to +16.5A
    float microWave = cos(tSec * 0.285f) * 3.8f;        // -3.8A to +3.8A
    float dcBias = sin(tSec * 0.018f) * 2.5f + 1.2f;    // Dynamic net flow bias

    // Micro-jitter / decimal variations (+/- 0.65A with realistic floating precision)
    float jitter = ((float)((int)(esp_random() % 130) - 65)) / 100.0f;

    float targetCurrent = macroWave + microWave + dcBias + jitter;

    // Soft taper near boundaries:
    if (g_stack.global_soc >= 98.5f && targetCurrent > 0.0f) {
        float taper = (100.0f - g_stack.global_soc) / 1.5f;
        if (taper < 0.05f) taper = 0.05f;
        targetCurrent = targetCurrent * taper + 0.35f; // Trickle/float charge
    }
    if (g_stack.global_soc <= 10.0f && targetCurrent < 0.0f) {
        float taper = (g_stack.global_soc - 5.0f) / 5.0f;
        if (taper < 0.0f) taper = 0.0f;
        targetCurrent *= taper;
    }

    // Smooth filtering
    g_stack.global_current_a = g_stack.global_current_a * 0.65f + targetCurrent * 0.35f;

    // Total nominal capacity in Ah
    float totalAh = 0.0f;
    for (uint8_t m = 0; m < g_stack.module_count; m++) {
        totalAh += getModelDescriptor(g_stack.modules[m].model_type).nominal_ah;
    }
    if (totalAh < 10.0f) totalAh = 74.0f * (g_stack.module_count > 0 ? g_stack.module_count : 1);

    // Integrate Coulomb: dSOC = (I * dt / 3600) / totalAh * 100
    float dSoc = (g_stack.global_current_a * (dtSec / 3600.0f) / totalAh) * 100.0f;
    g_stack.global_soc += dSoc;
    if (g_stack.global_soc > 100.0f) g_stack.global_soc = 100.0f;
    if (g_stack.global_soc < 0.0f) g_stack.global_soc = 0.0f;

    // Ambient & thermal drift
    float tempDrift = ((float)((int)(esp_random() % 60) - 30)) / 1000.0f;
    g_stack.global_temp_c += tempDrift;
    if (g_stack.global_temp_c < 21.0f) g_stack.global_temp_c = 21.0f;
    if (g_stack.global_temp_c > 31.0f) g_stack.global_temp_c = 31.0f;

    // Cell spread micro-variation
    if ((esp_random() % 8) == 0) {
        int8_t spreadDrift = (int8_t)(esp_random() % 3) - 1;
        int16_t newSpread = (int16_t)g_stack.cell_spread_mv + spreadDrift;
        if (newSpread >= 5 && newSpread <= 20) g_stack.cell_spread_mv = (uint16_t)newSpread;
    }

    recalculatePhysics();
}
