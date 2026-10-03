#include <unity.h>
#include <string>
#include <vector>

#include "BatteryData.h"
#include "PylonParser.h"

BatteryStack stack;

void setUp(void) {
    stack = BatteryStack();
}

void tearDown(void) {
}

// 1. Test Token Splitting and DateTime Merge
void test_token_splitting(void) {
    String line = "1     49920  -50    24500  24200  24700  3325   3331   Dischg   Normal   Normal   Normal   85%      2026-10-03 13:00:00  Normal   Normal   26300    Normal";
    std::vector<String> tokens = PylonParser::splitDataTokens(line);
    
    TEST_ASSERT_EQUAL(18, tokens.size());
    TEST_ASSERT_EQUAL_STRING("1", tokens[0].c_str());
    TEST_ASSERT_EQUAL_STRING("49920", tokens[1].c_str());
    TEST_ASSERT_EQUAL_STRING("Dischg", tokens[8].c_str());
    TEST_ASSERT_EQUAL_STRING("85%", tokens[12].c_str());
    TEST_ASSERT_EQUAL_STRING("2026-10-03 13:00:00", tokens[13].c_str());
    TEST_ASSERT_EQUAL_STRING("Normal", tokens[17].c_str());
}

// 2. Test Battery Model Profile Detection
void test_model_detection(void) {
    const ModelProfile* p3000c = detectModelProfile("US3000C");
    TEST_ASSERT_EQUAL(MODEL_US3000C, p3000c->model);
    TEST_ASSERT_TRUE(p3000c->supportsSohCmd);
    TEST_ASSERT_FALSE(p3000c->supportsEuro);

    const ModelProfile* p3000d = detectModelProfile("US3000D");
    TEST_ASSERT_EQUAL(MODEL_US3000D, p3000d->model);
    TEST_ASSERT_TRUE(p3000d->supportsEuro);
    TEST_ASSERT_FALSE(p3000d->supportsSohCmd);

    const ModelProfile* p5000 = detectModelProfile("UP5000");
    TEST_ASSERT_EQUAL(MODEL_UP5000, p5000->model);
    TEST_ASSERT_EQUAL(16, p5000->defaultCellCount);
}

// 3. Test Tabular PWR Parser (18-column US3000C)
void test_parse_pwr_table(void) {
    String pwrRaw = 
        "@\n"
        "Power Volt   Curr   Tempr  Tlow   Thigh  Vlow   Vhigh  Base.St  Volt.St  Curr.St  Temp.St  Coulomb  Time                 B.V.St   B.T.St   MosTempr M.T.St\n"
        "1     49920  -50    24500  24200  24700  3325   3331   Dischg   Normal   Normal   Normal   85%      2026-10-03 13:00:00  Normal   Normal   26300    Normal\n"
        "2     49910  -50    24400  24100  24600  3324   3330   Dischg   Normal   Normal   Normal   85%      2026-10-03 13:00:00  Normal   Normal   26100    Normal\n"
        "3     -      -      -      -      -      -      -      Absent   -        -        -        -        -\n"
        "Command completed successfully\n"
        "$$\n"
        "pylon>";

    bool ok = PylonParser::parsePwr(pwrRaw, stack);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL(2, stack.moduleCount);
    TEST_ASSERT_TRUE(stack.modules[1].present);
    TEST_ASSERT_EQUAL(49920, stack.modules[1].power.voltMv);
    TEST_ASSERT_EQUAL(-50, stack.modules[1].power.currMa);
    TEST_ASSERT_EQUAL(85, stack.modules[1].power.socPercent);
    TEST_ASSERT_EQUAL(26300, stack.modules[1].power.mosTempMdeg);
    TEST_ASSERT_EQUAL_STRING("Dischg", stack.modules[1].power.baseState);
    TEST_ASSERT_EQUAL_STRING("Normal", stack.modules[1].power.voltState);
}

// 4. Test BAT Response Parser (Per-cell voltages & balancing)
void test_parse_bat_cells(void) {
    String batRaw = 
        "@\n"
        "Battery  Volt     Curr     Tempr    Base State   Volt. State  Curr. State  Temp. State  SOC          Coulomb      BAL\n"
        "0        3328     -50      24500    Dischg       Normal       Normal       Normal       85%          62900 mAH      N\n"
        "1        3331     -50      24500    Dischg       Normal       Normal       Normal       85%          62900 mAH      Y\n"
        "2        3325     -50      24500    Dischg       Normal       Normal       Normal       85%          62900 mAH      N\n"
        "Command completed successfully\n"
        "$$\n"
        "pylon>";

    bool ok = PylonParser::parseBat(batRaw, stack, 1);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL(3, stack.modules[1].cellCountParsed);
    TEST_ASSERT_EQUAL(3328, stack.modules[1].cells[0].voltMv);
    TEST_ASSERT_FALSE(stack.modules[1].cells[0].balance);
    TEST_ASSERT_EQUAL(3331, stack.modules[1].cells[1].voltMv);
    TEST_ASSERT_TRUE(stack.modules[1].cells[1].balance);
}

// 5. Test INFO Response Parser
void test_parse_info(void) {
    String infoRaw = 
        "@\n\n"
        "Device address      : 1\n"
        "Manufacturer        : Pylon\n"
        "Device name         : US3000C\n"
        "Board version       : V10R04\n"
        "Main Soft version   : V2.8\n"
        "Soft  version       : V1.4\n"
        "Barcode             : PPYB202204180001\n"
        "Specification       : 48V/74AH\n"
        "Cell Number         : 15\n"
        "Command completed successfully\n"
        "$$\n"
        "pylon>";

    bool ok = PylonParser::parseInfo(infoRaw, stack, 1);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL(MODEL_US3000C, stack.model);
    TEST_ASSERT_EQUAL_STRING("US3000C", stack.modules[1].info.deviceName);
    TEST_ASSERT_EQUAL_STRING("PPYB202204180001", stack.modules[1].info.barcode);
    TEST_ASSERT_EQUAL_STRING("V2.8", stack.modules[1].info.mainSoftVersion);
    TEST_ASSERT_EQUAL(15, stack.modules[1].info.cellCount);
}

// 6. Test STAT Response Parser (Protection & Operational Counters)
void test_parse_stat(void) {
    String statRaw = 
        "@\n"
        "Device address           1\n"
        "Data Items      :        4\n"
        "HisData Items   :     1799\n"
        "Charge Times    :      452\n"
        "Discharge Cnt.  :      450\n"
        "Idle Times      :     3690\n"
        "COC Times       :        0\n"
        "DOC Times       :        0\n"
        "Bat OV Times    :        0\n"
        "Bat UV Times    :        0\n"
        "CYCLE Times     :      120\n"
        "SOH             :       99\n"
        "Dsg Cap         : 24534743\n"
        "Command completed successfully\n"
        "$$\n"
        "pylon>";

    bool ok = PylonParser::parseStat(statRaw, stack, 1);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL(120, stack.modules[1].stats.cycleTimes);
    TEST_ASSERT_EQUAL(99, stack.modules[1].stats.sohPercent);
    TEST_ASSERT_EQUAL(452, stack.modules[1].stats.chargeTimes);
    TEST_ASSERT_EQUAL(450, stack.modules[1].stats.dischargeTimes);
    TEST_ASSERT_EQUAL(3690, stack.modules[1].stats.idleTimes);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_token_splitting);
    RUN_TEST(test_model_detection);
    RUN_TEST(test_parse_pwr_table);
    RUN_TEST(test_parse_bat_cells);
    RUN_TEST(test_parse_info);
    RUN_TEST(test_parse_stat);
    return UNITY_END();
}
