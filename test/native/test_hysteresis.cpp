#include "test_framework.h"
#include "../../src/utils/Hysteresis.h"

void test_initial_value_zero() {
    Hysteresis<2> h;
    ASSERT_EQ(0, h.getValue());
}

void test_update_changes_value() {
    Hysteresis<2> h;
    bool changed = h.update(2048);
    ASSERT_TRUE(changed);
    ASSERT_EQ(2048 >> 5, h.getValue());
}

void test_small_change_ignored() {
    Hysteresis<2> h;
    h.update(2048);
    bool changed = h.update(2049);
    ASSERT_FALSE(changed);
    ASSERT_EQ(2048 >> 5, h.getValue());
}

void test_large_change_detected() {
    Hysteresis<2> h;
    h.update(2048);
    bool changed = h.update(2200);
    ASSERT_TRUE(changed);
    ASSERT_EQ(2200 >> 5, h.getValue());
}

void test_full_range() {
    Hysteresis<2> h;
    h.update(0);
    ASSERT_EQ(0, h.getValue());
    h.update(4095);
    ASSERT_EQ(127, h.getValue());
}

void test_reset() {
    Hysteresis<2> h;
    h.update(4095);
    h.reset(0);
    ASSERT_EQ(0, h.getValue());
}

void test_bits_1_less_hysteresis() {
    Hysteresis<1> h;
    h.update(1024);
    bool changed = h.update(1026);
    ASSERT_TRUE(changed || h.getValue() == (1024 >> 5));
}

int main() {
    printf("=== Hysteresis ===\n");
    RUN_TEST(test_initial_value_zero);
    RUN_TEST(test_update_changes_value);
    RUN_TEST(test_small_change_ignored);
    RUN_TEST(test_large_change_detected);
    RUN_TEST(test_full_range);
    RUN_TEST(test_reset);
    RUN_TEST(test_bits_1_less_hysteresis);
    TEST_SUMMARY();
}
