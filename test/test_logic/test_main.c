/* This file contains automated tests to check if our logic works correctly */
#include <unity.h>
#include "alarm.h"
#include "input.h"
#include "system_state.h"

// Include the C files directly so we can test them on a computer
#include "../../src/alarm.c"
#include "../../src/input.c"
#include "../../src/system_state.c"


/* Test what happens when it is too cold */
void test_alarm_below_lower_threshold(void) {
    TEST_ASSERT_EQUAL(ALARM_LOW_TEMPERATURE, evaluateTemperature(17.9f));
    TEST_ASSERT_EQUAL(ALARM_LOW_TEMPERATURE, evaluateTemperature(0.0f));
    TEST_ASSERT_EQUAL(ALARM_LOW_TEMPERATURE, evaluateTemperature(-5.0f));
}

/* Test exactly on the cold limit */
void test_alarm_exactly_lower_threshold(void) {
    TEST_ASSERT_EQUAL(ALARM_NORMAL, evaluateTemperature(18.0f));
}

/* Test normal room temperatures */
void test_alarm_normal_value(void) {
    TEST_ASSERT_EQUAL(ALARM_NORMAL, evaluateTemperature(22.5f));
    TEST_ASSERT_EQUAL(ALARM_NORMAL, evaluateTemperature(25.0f));
    TEST_ASSERT_EQUAL(ALARM_NORMAL, evaluateTemperature(29.9f));
}

/* Test exactly on the hot limit */
void test_alarm_exactly_upper_threshold(void) {
    TEST_ASSERT_EQUAL(ALARM_NORMAL, evaluateTemperature(30.0f));
}

/* Test what happens when it is too hot */
void test_alarm_above_upper_threshold(void) {
    TEST_ASSERT_EQUAL(ALARM_HIGH_TEMPERATURE, evaluateTemperature(30.1f));
    TEST_ASSERT_EQUAL(ALARM_HIGH_TEMPERATURE, evaluateTemperature(38.5f));
    TEST_ASSERT_EQUAL(ALARM_HIGH_TEMPERATURE, evaluateTemperature(50.0f));
}

/* Test turning the knob right goes to the next page */
void test_navigation_forward_sequence(void) {
    DisplayMode mode = DISPLAY_TEMPERATURE;
    mode = nextDisplayMode(mode);
    TEST_ASSERT_EQUAL(DISPLAY_HUMIDITY, mode);
    mode = nextDisplayMode(mode);
    TEST_ASSERT_EQUAL(DISPLAY_LIGHT, mode);
    mode = nextDisplayMode(mode);
    TEST_ASSERT_EQUAL(DISPLAY_MOTION, mode);
}

/* Test turning the knob right from the last page goes back to the first */
void test_navigation_forward_wraparound(void) {
    DisplayMode mode = DISPLAY_MOTION;
    mode = nextDisplayMode(mode);
    TEST_ASSERT_EQUAL(DISPLAY_TEMPERATURE, mode);
}

/* Test turning the knob left goes to the previous page */
void test_navigation_reverse_sequence(void) {
    DisplayMode mode = DISPLAY_MOTION;
    mode = previousDisplayMode(mode);
    TEST_ASSERT_EQUAL(DISPLAY_LIGHT, mode);
    mode = previousDisplayMode(mode);
    TEST_ASSERT_EQUAL(DISPLAY_HUMIDITY, mode);
    mode = previousDisplayMode(mode);
    TEST_ASSERT_EQUAL(DISPLAY_TEMPERATURE, mode);
}

/* Test turning the knob left from the first page goes to the last */
void test_navigation_reverse_wraparound(void) {
    DisplayMode mode = DISPLAY_TEMPERATURE;
    mode = previousDisplayMode(mode);
    TEST_ASSERT_EQUAL(DISPLAY_MOTION, mode);
}

/* Test that the system stays awake if time has not run out */
void test_state_active_no_timeout(void) {
    SystemState next = evaluateSystemState(SYSTEM_ACTIVE, false, 10, 15);
    TEST_ASSERT_EQUAL(SYSTEM_ACTIVE, next);
}

/* Test that the system goes to sleep when time runs out */
void test_state_active_timeout_reached(void) {
    SystemState next = evaluateSystemState(SYSTEM_ACTIVE, false, 15, 15);
    TEST_ASSERT_EQUAL(SYSTEM_INACTIVE, next);

    next = evaluateSystemState(SYSTEM_ACTIVE, false, 25, 15);
    TEST_ASSERT_EQUAL(SYSTEM_INACTIVE, next);
}

/* Test that the system stays asleep if nobody moves */
void test_state_inactive_no_motion(void) {
    SystemState next = evaluateSystemState(SYSTEM_INACTIVE, false, 30, 15);
    TEST_ASSERT_EQUAL(SYSTEM_INACTIVE, next);
}

/* Test that the system wakes up when someone moves */
void test_state_inactive_motion_detected(void) {
    SystemState next = evaluateSystemState(SYSTEM_INACTIVE, true, 45, 15);
    TEST_ASSERT_EQUAL(SYSTEM_ACTIVE, next);
}

/* Run all the tests */
int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Alarm checks
    RUN_TEST(test_alarm_below_lower_threshold);
    RUN_TEST(test_alarm_exactly_lower_threshold);
    RUN_TEST(test_alarm_normal_value);
    RUN_TEST(test_alarm_exactly_upper_threshold);
    RUN_TEST(test_alarm_above_upper_threshold);

    // Screen page checks
    RUN_TEST(test_navigation_forward_sequence);
    RUN_TEST(test_navigation_forward_wraparound);
    RUN_TEST(test_navigation_reverse_sequence);
    RUN_TEST(test_navigation_reverse_wraparound);

    // Sleep state checks
    RUN_TEST(test_state_active_no_timeout);
    RUN_TEST(test_state_active_timeout_reached);
    RUN_TEST(test_state_inactive_no_motion);
    RUN_TEST(test_state_inactive_motion_detected);

    return UNITY_END();
}

// Required functions for Unity tests
void setUp(void) {} 
void tearDown(void) {}
