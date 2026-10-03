#include <unity.h>
#include <string>
#include <vector>

#include "BmsModel.h"
#include "BmsPhysics.h"
#include "BmsProtocol.h"

// Global stack instance for tests
StackData g_stack;

void setUp(void) {
    g_stack.module_count = 2;
    g_stack.global_soc = 85.0f;
    g_stack.global_current_a = 15.0f;
    g_stack.global_temp_c = 23.0f;
    g_stack.cell_spread_mv = 8;
    g_stack.auto_physics = true;
    g_stack.sim_inverter_enabled = false;

    for (uint8_t i = 0; i < 2; i++) {
        g_stack.modules[i].id = i + 1;
        g_stack.modules[i].model_type = MODEL_US3000C;
        g_stack.modules[i].fw_version = "V2.8";
        g_stack.modules[i].barcode = "PPYB20220418000" + String(i + 1);
        g_stack.modules[i].cell_count = 15;
        g_stack.modules[i].soc = 85.0f;
        g_stack.modules[i].soh_pct = 99.0f;
        g_stack.modules[i].volt_st = "Normal";
        g_stack.modules[i].curr_st = "Normal";
        g_stack.modules[i].temp_st = "Normal";
        g_stack.modules[i].cycle_times = 145;
    }
    recalculatePhysics();
}

void tearDown(void) {
}

// 1. Test CLI Help Output
void test_help_formatting(void) {
    String help = BmsProtocolFormatter::formatHelp(g_stack.modules[0]);
    TEST_ASSERT_TRUE(help.startsWith("@\nhelp\n"));
    TEST_ASSERT_TRUE(help.indexOf("pwr [c]") >= 0);
    TEST_ASSERT_TRUE(help.indexOf("bat [c]") >= 0);
    TEST_ASSERT_TRUE(help.indexOf("info [c]") >= 0);
    TEST_ASSERT_TRUE(help.indexOf("stat [c]") >= 0);
    TEST_ASSERT_TRUE(help.indexOf("soh [c]") >= 0); // US3000C supports soh
    TEST_ASSERT_TRUE(help.endsWith("$$\npylon>"));
}

// 2. Test CLI Info Output
void test_info_formatting(void) {
    String info = BmsProtocolFormatter::formatInfo(g_stack, 1);
    TEST_ASSERT_TRUE(info.startsWith("@\n\nDevice address      : 1"));
    TEST_ASSERT_TRUE(info.indexOf("Manufacturer        : Pylon") >= 0);
    TEST_ASSERT_TRUE(info.indexOf("Device name         : US3000C") >= 0);
    TEST_ASSERT_TRUE(info.indexOf("Cell Number         : 15") >= 0);
    TEST_ASSERT_TRUE(info.indexOf("Barcode             : PPYB202204180001") >= 0);
    TEST_ASSERT_TRUE(info.endsWith("$$\npylon>"));

    // Query non-existent module -> Error
    String errInfo = BmsProtocolFormatter::formatInfo(g_stack, 10);
    TEST_ASSERT_TRUE(errInfo.indexOf("Invalid command") >= 0);
}

// 3. Test CLI Pwr Output (18 columns for US3000C vs 22 for US3000D)
void test_pwr_formatting(void) {
    // Standard US3000C format
    String pwr = BmsProtocolFormatter::formatPwr(g_stack);
    TEST_ASSERT_TRUE(pwr.startsWith("@\nPower Volt"));
    TEST_ASSERT_TRUE(pwr.indexOf("Base.St") >= 0);
    TEST_ASSERT_TRUE(pwr.indexOf("Absent") >= 0); // Modules 3..16 are Absent
    TEST_ASSERT_TRUE(pwr.endsWith("$$\npylon>"));

    // Switch Master to US3000D (22 columns with sensor IDs)
    g_stack.modules[0].model_type = MODEL_US3000D;
    String pwrD = BmsProtocolFormatter::formatPwr(g_stack);
    TEST_ASSERT_TRUE(pwrD.indexOf("Tlow.Id") >= 0);
    TEST_ASSERT_TRUE(pwrD.indexOf("Vlow.Id") >= 0);
}

// 4. Test CLI Bat Output (Per-cell telemetry)
void test_bat_formatting(void) {
    String bat = BmsProtocolFormatter::formatBat(g_stack, 1);
    TEST_ASSERT_TRUE(bat.startsWith("@\nBattery  Volt"));
    TEST_ASSERT_TRUE(bat.indexOf("BAL") >= 0);
    TEST_ASSERT_TRUE(bat.indexOf("mAH") >= 0);
    TEST_ASSERT_TRUE(bat.endsWith("$$\npylon>"));

    // Module 2 with 16 cells (US5000)
    g_stack.modules[1].model_type = MODEL_US5000;
    g_stack.modules[1].cell_count = 16;
    recalculatePhysics();

    String bat16 = BmsProtocolFormatter::formatBat(g_stack, 2);
    TEST_ASSERT_TRUE(bat16.indexOf("15       ") >= 0); // 0-indexed cell 15 (16th cell)
}

// 5. Test CLI Stat Output (Model C vs Model D)
void test_stat_formatting(void) {
    // Model C format
    String statC = BmsProtocolFormatter::formatStat(g_stack, 1);
    TEST_ASSERT_TRUE(statC.indexOf("Data Items      :        4") >= 0);
    TEST_ASSERT_TRUE(statC.indexOf("HisData Items   :     1799") >= 0);
    TEST_ASSERT_TRUE(statC.indexOf("CYCLE Times     :      145") >= 0);
    TEST_ASSERT_TRUE(statC.endsWith("$$\npylon>"));

    // Model D format
    g_stack.modules[0].model_type = MODEL_US3000D;
    String statD = BmsProtocolFormatter::formatStat(g_stack, 1);
    TEST_ASSERT_TRUE(statD.indexOf("Charge Secs.") >= 0);
    TEST_ASSERT_TRUE(statD.indexOf("Discharge Secs.") >= 0);
    TEST_ASSERT_TRUE(statD.endsWith("$$\npylon>"));
}

// 6. Test CLI SOH & Euro Output
void test_soh_and_euro_formatting(void) {
    // US3000C supports SOH table, rejects Euro
    g_stack.modules[0].model_type = MODEL_US3000C;
    String sohC = BmsProtocolFormatter::formatSoh(g_stack, 1);
    TEST_ASSERT_TRUE(sohC.indexOf("Battery    Voltage    SOHCount   SOHStatus") >= 0);

    String euroC = BmsProtocolFormatter::formatEuro(g_stack, 1);
    TEST_ASSERT_TRUE(euroC.indexOf("Unknown command") >= 0);

    // US3000D supports Euro stats, rejects SOH table
    g_stack.modules[0].model_type = MODEL_US3000D;
    g_stack.modules[0].euro.soh_pct = 98.5f;
    g_stack.modules[0].euro.energy_thro_kwh = 1520.0f;
    g_stack.modules[0].euro.chg_dsg_cycle = 210;

    String sohD = BmsProtocolFormatter::formatSoh(g_stack, 1);
    TEST_ASSERT_TRUE(sohD.indexOf("Unknown command") >= 0);

    String euroD = BmsProtocolFormatter::formatEuro(g_stack, 1);
    TEST_ASSERT_TRUE(euroD.indexOf("Round Trip Eff.") >= 0);
    TEST_ASSERT_TRUE(euroD.indexOf("Energy Thro.") >= 0);
    TEST_ASSERT_TRUE(euroD.endsWith("$$\npylon>"));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_help_formatting);
    RUN_TEST(test_info_formatting);
    RUN_TEST(test_pwr_formatting);
    RUN_TEST(test_bat_formatting);
    RUN_TEST(test_stat_formatting);
    RUN_TEST(test_soh_and_euro_formatting);
    return UNITY_END();
}
