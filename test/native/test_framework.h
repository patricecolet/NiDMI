#pragma once
// Minimal test framework — no external dependencies
#include <cstdio>
#include <cstdlib>
#include <cmath>

static int _tests_run = 0;
static int _tests_failed = 0;

#define ASSERT_EQ(expected, actual) do { \
    auto _e = (expected); auto _a = (actual); \
    _tests_run++; \
    if (_e != _a) { \
        fprintf(stderr, "  FAIL %s:%d: expected %d, got %d\n", __FILE__, __LINE__, (int)_e, (int)_a); \
        _tests_failed++; \
    } \
} while(0)

#define ASSERT_TRUE(cond) do { \
    _tests_run++; \
    if (!(cond)) { \
        fprintf(stderr, "  FAIL %s:%d: expected true\n", __FILE__, __LINE__); \
        _tests_failed++; \
    } \
} while(0)

#define ASSERT_FALSE(cond) do { \
    _tests_run++; \
    if ((cond)) { \
        fprintf(stderr, "  FAIL %s:%d: expected false\n", __FILE__, __LINE__); \
        _tests_failed++; \
    } \
} while(0)

#define ASSERT_NEAR(expected, actual, epsilon) do { \
    double _e = (expected); double _a = (actual); \
    _tests_run++; \
    if (fabs(_e - _a) > (epsilon)) { \
        fprintf(stderr, "  FAIL %s:%d: expected ~%.4f, got %.4f\n", __FILE__, __LINE__, _e, _a); \
        _tests_failed++; \
    } \
} while(0)

#define ASSERT_STR_EQ(expected, actual) do { \
    const char* _e = (expected); const char* _a = (actual); \
    _tests_run++; \
    if (strcmp(_e, _a) != 0) { \
        fprintf(stderr, "  FAIL %s:%d: expected \"%s\", got \"%s\"\n", __FILE__, __LINE__, _e, _a); \
        _tests_failed++; \
    } \
} while(0)

#define RUN_TEST(fn) do { \
    printf("  %-50s", #fn); \
    int _before = _tests_failed; \
    fn(); \
    printf("%s\n", _tests_failed == _before ? "OK" : "FAILED"); \
} while(0)

#define TEST_SUMMARY() do { \
    printf("\n%d assertions, %d failed\n", _tests_run, _tests_failed); \
    return _tests_failed > 0 ? 1 : 0; \
} while(0)
