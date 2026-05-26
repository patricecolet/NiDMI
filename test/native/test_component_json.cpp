#include "test_framework.h"
#include <cstring>

// Pull in real types through the stub Arduino.h
#include "../../src/components/ComponentDefinition.h"

void test_safeSnprintf_basic() {
    char buf[64];
    int r = ComponentDefinition::safeSnprintf(buf, sizeof(buf), "hello %d", 42);
    ASSERT_STR_EQ("hello 42", buf);
    ASSERT_EQ(8, r);
}

void test_safeSnprintf_truncation() {
    char buf[5];
    int r = ComponentDefinition::safeSnprintf(buf, sizeof(buf), "hello world");
    ASSERT_EQ(-1, r);
}

void test_safeSnprintf_exact_fit() {
    char buf[6];
    int r = ComponentDefinition::safeSnprintf(buf, sizeof(buf), "hello");
    ASSERT_EQ(5, r);
    ASSERT_STR_EQ("hello", buf);
}

void test_appendJsonString_basic() {
    char buf[64];
    int r = ComponentDefinition::appendJsonString(buf, sizeof(buf), "hello");
    ASSERT_STR_EQ("hello", buf);
    ASSERT_EQ(5, r);
}

void test_appendJsonString_escapes_quotes() {
    char buf[64];
    int r = ComponentDefinition::appendJsonString(buf, sizeof(buf), "say \"hi\"");
    ASSERT_STR_EQ("say \\\"hi\\\"", buf);
    ASSERT_EQ(10, r);
}

void test_appendJsonString_escapes_backslash() {
    char buf[64];
    int r = ComponentDefinition::appendJsonString(buf, sizeof(buf), "a\\b");
    ASSERT_STR_EQ("a\\\\b", buf);
    ASSERT_EQ(4, r);
}

void test_appendJsonString_escapes_control_chars() {
    char buf[64];
    int r = ComponentDefinition::appendJsonString(buf, sizeof(buf), "a\nb\tc");
    ASSERT_STR_EQ("a\\nb\\tc", buf);
    ASSERT_EQ(7, r); // a(1) \n(2) b(1) \t(2) c(1) = 7
}

void test_appendJsonString_null_src() {
    char buf[64];
    int r = ComponentDefinition::appendJsonString(buf, sizeof(buf), nullptr);
    ASSERT_EQ(0, r);
}

void test_appendJsonString_buffer_overflow() {
    char buf[4];
    int r = ComponentDefinition::appendJsonString(buf, sizeof(buf), "abcdef");
    ASSERT_STR_EQ("abc", buf);
    ASSERT_EQ(3, r);
}

void test_toJson_minimal() {
    ComponentDefinition def;
    def.id = "pot";
    def.displayName = "Potentiometer";
    def.cardId = "cardPot";
    def.family = ComponentFamily::BASIC;
    def.familyName = "Basic";
    def.type = ComponentType::POTENTIOMETER;
    def.pinType = PinType::PIN_ANALOG;
    def.altPinType = -1;
    def.implemented = true;
    def.supportsMidi = true;
    def.supportsOsc = true;
    def.additionalPinCount = 0;
    def.statusTextTemplate = nullptr;

    char buf[1024];
    int written = def.toJson(buf, sizeof(buf));
    ASSERT_TRUE(written > 0);

    // Verify key fields present in output
    ASSERT_TRUE(strstr(buf, "\"id\":\"pot\"") != nullptr);
    ASSERT_TRUE(strstr(buf, "\"dn\":\"Potentiometer\"") != nullptr);
    ASSERT_TRUE(strstr(buf, "\"impl\":true") != nullptr);
    ASSERT_TRUE(strstr(buf, "\"midi\":true") != nullptr);
    ASSERT_TRUE(strstr(buf, "\"osc\":true") != nullptr);
    // Must be valid JSON (starts with { ends with })
    ASSERT_EQ('{', buf[0]);
    ASSERT_EQ('}', buf[written - 1]);
}

void test_toJson_buffer_too_small() {
    ComponentDefinition def;
    def.id = "potentiometer";
    def.displayName = "Potentiometer";
    def.cardId = "cardPot";
    def.family = ComponentFamily::BASIC;
    def.familyName = "Basic";

    char buf[50]; // too small
    int written = def.toJson(buf, sizeof(buf));
    ASSERT_EQ(0, written);
}

void test_toJson_with_special_chars() {
    ComponentDefinition def;
    def.id = "test";
    def.displayName = "Test \"quoted\"";
    def.cardId = "card";
    def.family = ComponentFamily::BASIC;
    def.familyName = "Basic";
    def.pinType = PinType::PIN_DIGITAL;
    def.altPinType = -1;
    def.implemented = false;
    def.supportsMidi = false;
    def.supportsOsc = false;
    def.additionalPinCount = 0;

    char buf[1024];
    int written = def.toJson(buf, sizeof(buf));
    ASSERT_TRUE(written > 0);
    ASSERT_TRUE(strstr(buf, "Test \\\"quoted\\\"") != nullptr);
}

int main() {
    printf("=== ComponentDefinition JSON ===\n");
    RUN_TEST(test_safeSnprintf_basic);
    RUN_TEST(test_safeSnprintf_truncation);
    RUN_TEST(test_safeSnprintf_exact_fit);
    RUN_TEST(test_appendJsonString_basic);
    RUN_TEST(test_appendJsonString_escapes_quotes);
    RUN_TEST(test_appendJsonString_escapes_backslash);
    RUN_TEST(test_appendJsonString_escapes_control_chars);
    RUN_TEST(test_appendJsonString_null_src);
    RUN_TEST(test_appendJsonString_buffer_overflow);
    RUN_TEST(test_toJson_minimal);
    RUN_TEST(test_toJson_buffer_too_small);
    RUN_TEST(test_toJson_with_special_chars);
    TEST_SUMMARY();
}
