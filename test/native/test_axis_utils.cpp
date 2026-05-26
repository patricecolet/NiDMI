#include "test_framework.h"
#include "../../src/utils/AxisUtils.h"

void test_deadzone_center() {
    AxisRangeConfig cfg = {0, 1900, 2100, 4095, false};
    ASSERT_EQ(0, mapAxisValueGeneric(2000, cfg));
    ASSERT_EQ(0, mapAxisValueGeneric(1900, cfg));
    ASSERT_EQ(0, mapAxisValueGeneric(2100, cfg));
}

void test_max_positive() {
    AxisRangeConfig cfg = {0, 1900, 2100, 4095, false};
    ASSERT_EQ(127, mapAxisValueGeneric(4095, cfg));
}

void test_max_negative() {
    AxisRangeConfig cfg = {0, 1900, 2100, 4095, false};
    ASSERT_EQ(-127, mapAxisValueGeneric(0, cfg));
}

void test_mid_positive() {
    AxisRangeConfig cfg = {0, 1900, 2100, 4095, false};
    int8_t v = mapAxisValueGeneric(3100, cfg);
    ASSERT_TRUE(v > 0 && v < 127);
}

void test_mid_negative() {
    AxisRangeConfig cfg = {0, 1900, 2100, 4095, false};
    int8_t v = mapAxisValueGeneric(950, cfg);
    ASSERT_TRUE(v < 0 && v > -127);
}

void test_invert() {
    AxisRangeConfig cfg = {0, 1900, 2100, 4095, true};
    ASSERT_EQ(-127, mapAxisValueGeneric(4095, cfg));
    ASSERT_EQ(127, mapAxisValueGeneric(0, cfg));
}

void test_beyond_max_clamped() {
    AxisRangeConfig cfg = {0, 1900, 2100, 4095, false};
    ASSERT_EQ(127, mapAxisValueGeneric(5000, cfg));
}

void test_beyond_min_clamped() {
    AxisRangeConfig cfg = {100, 1900, 2100, 4095, false};
    ASSERT_EQ(-127, mapAxisValueGeneric(0, cfg));
}

int main() {
    printf("=== AxisUtils ===\n");
    RUN_TEST(test_deadzone_center);
    RUN_TEST(test_max_positive);
    RUN_TEST(test_max_negative);
    RUN_TEST(test_mid_positive);
    RUN_TEST(test_mid_negative);
    RUN_TEST(test_invert);
    RUN_TEST(test_beyond_max_clamped);
    RUN_TEST(test_beyond_min_clamped);
    TEST_SUMMARY();
}
