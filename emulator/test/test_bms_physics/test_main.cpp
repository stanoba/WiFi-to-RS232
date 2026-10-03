#include <unity.h>
#include <string>
#include <vector>

// Forward declarations & pure C++ test targets
#include "BmsModel.h"
#include "BmsPhysics.h"

// Global stack instance for tests
StackData g_stack;

// Test Setup & TearDown
void setUp(void) {
    g_stack.module_count = 4;
    g_stack.global_soc = 80.0f;
    g_stack.global_current_a = 0.0f;
    g_stack.global_temp_c = 25.0f;
    g_stack.cell_spread_mv = 10;
    g_stack.auto_physics = true;
    g_stack.sim_inverter_enabled = false;

    for (uint8_t i = 0; i < 4; i++) {
        g_stack.modules[i].id = i + 1;
        g_stack.modules[i].model_type = MODEL_US3000C;
        g_stack.modules[i].fw_version = "V2.8";
        g_stack.modules[i].cell_count = 15;
        g_stack.modules[i].soc = 80.0f;
        g_stack.modules[i].soh_pct = 100.0f;
        g_stack.modules[i].volt_st = "Normal";
        g_stack.modules[i].curr_st = "Normal";
        g_stack.modules[i].temp_st = "Normal";
    }
}

void tearDown(void) {
}

// 1. Test OCV (Open Circuit Voltage) Curve
void test_lifepo4_soc_to_ocv(void) {
    TEST_ASSERT_EQUAL_UINT16(2800, lifepo4SocToOcvMv(0.0f));
    TEST_ASSERT_EQUAL_UINT16(3450, lifepo4SocToOcvMv(100.0f));
    
    // Normal operating range (20% - 80% should be around 3.25V - 3.33V)
    uint16_t v50 = lifepo4SocToOcvMv(50.0f);
    TEST_ASSERT_GREATER_THAN(3250, v50);
    TEST_ASSERT_LESS_THAN(3350, v50);
}

// 2. Test Cell Spread Factors
void test_cell_spread_factors(void) {
    for (uint8_t m = 0; m < 4; m++) {
        for (uint8_t c = 0; c < 15; c++) {
            float f = getCellSpreadFactor(m, c);
            TEST_ASSERT_TRUE(f >= -0.5f && f <= 0.5f);
        }
    }
}

// 3. Test Physics Voltage Recalculation
void test_physics_recalculation(void) {
    g_stack.global_soc = 50.0f;
    g_stack.global_current_a = 0.0f;
    g_stack.cell_spread_mv = 12;
    recalculatePhysics();

    // Verify cell voltages are calculated
    for (uint8_t c = 0; c < 15; c++) {
        uint16_t v = g_stack.modules[0].cells[c].voltage_mv;
        TEST_ASSERT_GREATER_THAN(3000, v);
        TEST_ASSERT_LESS_THAN(3500, v);
    }

    // Verify total pack voltage equals sum of cells
    int32_t sumMv = 0;
    for (uint8_t c = 0; c < 15; c++) {
        sumMv += g_stack.modules[0].cells[c].voltage_mv;
    }
    TEST_ASSERT_EQUAL_INT32(sumMv, g_stack.modules[0].voltage_mv);
}

// 4. Test Hierarchy Validation (Master Rank vs Slave Rank)
void test_hierarchy_validation(void) {
    String warning;

    // Default: all US3000C (Rank 30) -> Should be VALID
    TEST_ASSERT_TRUE(validateMasterHierarchy(warning));

    // Master is US2000C (Rank 20), Slave 2 is US5000 (Rank 50) -> Must be INVALID!
    g_stack.modules[0].model_type = MODEL_US2000C;
    g_stack.modules[1].model_type = MODEL_US5000;
    TEST_ASSERT_FALSE(validateMasterHierarchy(warning));
    TEST_ASSERT_TRUE(warning.length() > 0);

    // Auto-sort hierarchy -> Should restore valid order
    autoSortHierarchy();
    TEST_ASSERT_TRUE(validateMasterHierarchy(warning));
    TEST_ASSERT_EQUAL(MODEL_US5000, g_stack.modules[0].model_type);
}

// 5. Test Active Cell Balancing under High-Voltage Charge
void test_cell_balancing(void) {
    g_stack.global_soc = 95.0f; // High voltage LiFePO4 > 3.38V
    g_stack.global_current_a = 40.0f; // Fast charging (10A per module)
    g_stack.cell_spread_mv = 50; // Significant imbalance to trigger balancing
    recalculatePhysics();

    bool hasBalancingCell = false;
    for (uint8_t c = 0; c < 15; c++) {
        if (g_stack.modules[0].cells[c].balancing) {
            hasBalancingCell = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(hasBalancingCell);

    // Turn off current -> Balancing should deactivate
    g_stack.global_current_a = 0.0f;
    recalculatePhysics();
    for (uint8_t c = 0; c < 15; c++) {
        TEST_ASSERT_FALSE(g_stack.modules[0].cells[c].balancing);
    }
}

// 6. Test Inverter & Solar Dynamic Physics Loop
void test_inverter_simulation(void) {
    g_stack.sim_inverter_enabled = true;
    g_stack.global_soc = 50.0f;
    g_stack.global_current_a = 0.0f;

    // Simulate several seconds of ticks
    for (int i = 0; i < 5; i++) {
        updateInverterSimulation();
    }
    TEST_ASSERT_TRUE(g_stack.global_soc >= 0.0f && g_stack.global_soc <= 100.0f);
}

// 7. Test NVS Binary Storage Persistence & Module Deletion
void test_rack_nvs_persistence(void) {
    Preferences testPrefs;
    testPrefs.begin("pylon_emu", false);

    // Save 4 modules
    RackNvsRecord rec;
    memset(&rec, 0, sizeof(rec));
    rec.magic = RACK_NVS_MAGIC;
    rec.version = RACK_NVS_VERSION;
    rec.module_count = 4;
    rec.global_soc = 75.0f;
    rec.global_current_a = -1.2f;
    rec.global_temp_c = 23.0f;
    rec.cell_spread_mv = 12;

    for (uint8_t i = 0; i < 4; i++) {
        rec.modules[i].model_type = (i == 0) ? MODEL_US3000D : MODEL_US3000C;
        strncpy(rec.modules[i].fw_version, "V2.8", sizeof(rec.modules[i].fw_version) - 1);
        strncpy(rec.modules[i].barcode, ("PPYB20220418000" + std::to_string(i + 1)).c_str(), sizeof(rec.modules[i].barcode) - 1);
        rec.modules[i].cycle_times = 150 + i;
    }

    size_t saveSize = sizeof(rec) - (sizeof(ModuleNvsRecord) * (MAX_MODULES - rec.module_count));
    testPrefs.putBytes("rack_cfg", &rec, saveSize);

    // Now delete 2 modules (stack down to 2 modules)
    rec.module_count = 2;
    saveSize = sizeof(rec) - (sizeof(ModuleNvsRecord) * (MAX_MODULES - rec.module_count));
    testPrefs.putBytes("rack_cfg", &rec, saveSize);

    // Read back and verify
    RackNvsRecord loaded;
    memset(&loaded, 0, sizeof(loaded));
    size_t readLen = testPrefs.getBytes("rack_cfg", &loaded, sizeof(loaded));

    TEST_ASSERT_GREATER_THAN(32, readLen);
    TEST_ASSERT_EQUAL_HEX32(RACK_NVS_MAGIC, loaded.magic);
    TEST_ASSERT_EQUAL_UINT8(2, loaded.module_count); // Must be 2, NOT 4!
    TEST_ASSERT_EQUAL_FLOAT(75.0f, loaded.global_soc);
    TEST_ASSERT_EQUAL_STRING("V2.8", loaded.modules[0].fw_version);
}

// Runner Entrypoint (Like Go Test Runner)
int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_lifepo4_soc_to_ocv);
    RUN_TEST(test_cell_spread_factors);
    RUN_TEST(test_physics_recalculation);
    RUN_TEST(test_hierarchy_validation);
    RUN_TEST(test_cell_balancing);
    RUN_TEST(test_inverter_simulation);
    RUN_TEST(test_rack_nvs_persistence);
    return UNITY_END();
}


