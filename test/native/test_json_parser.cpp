#include "test_framework.h"
#include "../../src/utils/JSONParser.h"

void test_extractInt_basic() {
    String json("{\"rtpCc\":42,\"ch\":1}");
    ASSERT_EQ(42, JSONParser::extractInt(json, "rtpCc", -1));
    ASSERT_EQ(1, JSONParser::extractInt(json, "ch", -1));
}

void test_extractInt_missing_key() {
    String json("{\"a\":1}");
    ASSERT_EQ(99, JSONParser::extractInt(json, "missing", 99));
}

void test_extractInt_negative() {
    String json("{\"val\":-200}");
    ASSERT_EQ(-200, JSONParser::extractInt(json, "val", 0));
}

void test_extractInt_quoted_value() {
    String json("{\"val\":\"123\"}");
    ASSERT_EQ(123, JSONParser::extractInt(json, "val", 0));
}

void test_extractBool_true() {
    String json("{\"enabled\":true}");
    ASSERT_TRUE(JSONParser::extractBool(json, "enabled", false));
}

void test_extractBool_false() {
    String json("{\"enabled\":false}");
    ASSERT_FALSE(JSONParser::extractBool(json, "enabled", true));
}

void test_extractBool_missing() {
    String json("{\"a\":1}");
    ASSERT_TRUE(JSONParser::extractBool(json, "missing", true));
}

void test_extractStr_basic() {
    String json("{\"name\":\"hello\"}");
    String result = JSONParser::extractStr(json, "name", "");
    ASSERT_STR_EQ("hello", result.c_str());
}

void test_extractStr_with_escapes() {
    String json("{\"msg\":\"line1\\nline2\"}");
    String result = JSONParser::extractStr(json, "msg", "");
    ASSERT_STR_EQ("line1\nline2", result.c_str());
}

void test_extractStr_escaped_quote() {
    String json("{\"val\":\"say \\\"hi\\\"\"}");
    String result = JSONParser::extractStr(json, "val", "");
    ASSERT_STR_EQ("say \"hi\"", result.c_str());
}

void test_extractStr_missing() {
    String json("{\"a\":\"b\"}");
    String result = JSONParser::extractStr(json, "missing", "default");
    ASSERT_STR_EQ("default", result.c_str());
}

void test_extractInt_with_spaces() {
    String json("{\"val\": 42}");
    ASSERT_EQ(42, JSONParser::extractInt(json, "val", -1));
}

int main() {
    printf("=== JSONParser ===\n");
    RUN_TEST(test_extractInt_basic);
    RUN_TEST(test_extractInt_missing_key);
    RUN_TEST(test_extractInt_negative);
    RUN_TEST(test_extractInt_quoted_value);
    RUN_TEST(test_extractBool_true);
    RUN_TEST(test_extractBool_false);
    RUN_TEST(test_extractBool_missing);
    RUN_TEST(test_extractStr_basic);
    RUN_TEST(test_extractStr_with_escapes);
    RUN_TEST(test_extractStr_escaped_quote);
    RUN_TEST(test_extractStr_missing);
    RUN_TEST(test_extractInt_with_spaces);
    TEST_SUMMARY();
}
