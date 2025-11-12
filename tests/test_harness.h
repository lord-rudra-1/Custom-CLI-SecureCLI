#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, message); \
            test_failures++; \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(expected, actual, message) \
    do { \
        if ((expected) != (actual)) { \
            fprintf(stderr, "FAIL: %s:%d: %s (expected %d, got %d)\n", \
                    __FILE__, __LINE__, message, (int)(expected), (int)(actual)); \
            test_failures++; \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_STR_EQ(expected, actual, message) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            fprintf(stderr, "FAIL: %s:%d: %s (expected '%s', got '%s')\n", \
                    __FILE__, __LINE__, message, (expected), (actual)); \
            test_failures++; \
            return; \
        } \
    } while(0)

#define RUN_TEST(test_func) \
    do { \
        printf("Running " #test_func "... "); \
        test_failures = 0; \
        test_func(); \
        if (test_failures == 0) { \
            printf("PASS\n"); \
            tests_passed++; \
        } else { \
            tests_failed++; \
        } \
        tests_total++; \
    } while(0)

extern int test_failures;
extern int tests_total;
extern int tests_passed;
extern int tests_failed;

#endif



