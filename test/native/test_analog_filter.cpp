#include "test_framework.h"
#include "../../src/utils/AnalogFilter.h"

void test_mapFilterIntensity_bounds() {
    float a1 = AnalogFilter::mapFilterIntensity(1);
    float a10 = AnalogFilter::mapFilterIntensity(10);
    ASSERT_NEAR(0.5, a1, 0.01);
    ASSERT_NEAR(0.01, a10, 0.005);
}

void test_mapFilterIntensity_clamp() {
    ASSERT_NEAR(AnalogFilter::mapFilterIntensity(1), AnalogFilter::mapFilterIntensity(0), 0.001);
    ASSERT_NEAR(AnalogFilter::mapFilterIntensity(10), AnalogFilter::mapFilterIntensity(11), 0.001);
}

void test_mapFilterIntensity_monotonic() {
    for (uint8_t i = 1; i < 10; i++) {
        ASSERT_TRUE(AnalogFilter::mapFilterIntensity(i) > AnalogFilter::mapFilterIntensity(i + 1));
    }
}

void test_process_first_value_passthrough() {
    AnalogFilter f = {};
    ASSERT_EQ(1000, f.process(1000));
    ASSERT_TRUE(f.initialized);
}

void test_process_lowpass_smoothing() {
    AnalogFilter f = {};
    f.alpha = 0.5f;
    f.process(1000);
    uint16_t v = f.process(2000);
    ASSERT_EQ(1500, v);
}

void test_process_lowpass_convergence() {
    AnalogFilter f = {};
    f.alpha = 0.1f;
    f.process(0);
    for (int i = 0; i < 100; i++) f.process(4095);
    uint16_t v = f.process(4095);
    ASSERT_TRUE(v > 4000);
}

void test_median_and_lowpass_first_value() {
    AnalogFilter f = {};
    ASSERT_EQ(2000, f.processMedianAndLowpass(2000));
    ASSERT_TRUE(f.median_initialized);
}

void test_median_rejects_spike() {
    AnalogFilter f = {};
    f.processMedianAndLowpass(100);
    f.processMedianAndLowpass(100);
    f.processMedianAndLowpass(100);
    f.processMedianAndLowpass(4000); // spike
    f.processMedianAndLowpass(100);
    uint16_t v = f.processMedianAndLowpass(100);
    ASSERT_TRUE(v < 500);
}

int main() {
    printf("=== AnalogFilter ===\n");
    RUN_TEST(test_mapFilterIntensity_bounds);
    RUN_TEST(test_mapFilterIntensity_clamp);
    RUN_TEST(test_mapFilterIntensity_monotonic);
    RUN_TEST(test_process_first_value_passthrough);
    RUN_TEST(test_process_lowpass_smoothing);
    RUN_TEST(test_process_lowpass_convergence);
    RUN_TEST(test_median_and_lowpass_first_value);
    RUN_TEST(test_median_rejects_spike);
    TEST_SUMMARY();
}
