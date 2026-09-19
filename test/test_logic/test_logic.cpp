// Unit tests for the hardware-independent decision logic required by the
// lab: evaluateTemperature(), nextDisplayMode()/previousDisplayMode(),
// and evaluateSystemState(). Run with `pio test`.
//
// The logic functions themselves (evaluateTemperature, nextDisplayMode,
// previousDisplayMode, evaluateSystemState) touch no hardware, but their
// .cpp files sit alongside hardware-dependent code (gpio/i2c drivers), so
// this suite runs on-target via `pio test -e esp32dev` rather than in a
// native/host environment.

#include <unity.h>
#include "alarm.h"
#include "display.h"
#include "system_state.h"

// ---------------------------------------------------------------------
// Temperature alarm logic — 5 required cases
// ---------------------------------------------------------------------
void test_temp_below_lower_threshold() {
    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::LOW_TEMPERATURE),
                       static_cast<int>(evaluateTemperature(17.9f)));
}
void test_temp_exactly_lower_threshold() {
    // 18.0 is NOT below the limit -> NORMAL (boundary is inclusive of normal range)
    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::NORMAL),
                       static_cast<int>(evaluateTemperature(18.0f)));
}
void test_temp_normal_value() {
    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::NORMAL),
                       static_cast<int>(evaluateTemperature(24.0f)));
}
void test_temp_exactly_upper_threshold() {
    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::NORMAL),
                       static_cast<int>(evaluateTemperature(30.0f)));
}
void test_temp_above_upper_threshold() {
    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::HIGH_TEMPERATURE),
                       static_cast<int>(evaluateTemperature(30.1f)));
}

// ---------------------------------------------------------------------
// Display navigation — forward/reverse + wraparound (4 required cases)
// ---------------------------------------------------------------------
void test_nav_forward_step() {
    TEST_ASSERT_EQUAL(static_cast<int>(DisplayMode::HUMIDITY),
                       static_cast<int>(nextDisplayMode(DisplayMode::TEMPERATURE)));
}
void test_nav_forward_wraparound() {
    TEST_ASSERT_EQUAL(static_cast<int>(DisplayMode::TEMPERATURE),
                       static_cast<int>(nextDisplayMode(DisplayMode::MOTION)));
}
void test_nav_reverse_step() {
    TEST_ASSERT_EQUAL(static_cast<int>(DisplayMode::LIGHT),
                       static_cast<int>(previousDisplayMode(DisplayMode::MOTION)));
}
void test_nav_reverse_wraparound() {
    TEST_ASSERT_EQUAL(static_cast<int>(DisplayMode::MOTION),
                       static_cast<int>(previousDisplayMode(DisplayMode::TEMPERATURE)));
}

// ---------------------------------------------------------------------
// System state machine — 4 required cases
// ---------------------------------------------------------------------
void test_state_active_no_timeout() {
    SystemState s = evaluateSystemState(SystemState::ACTIVE, false, 5000, 15000);
    TEST_ASSERT_EQUAL(static_cast<int>(SystemState::ACTIVE), static_cast<int>(s));
}
void test_state_active_timeout_elapsed() {
    SystemState s = evaluateSystemState(SystemState::ACTIVE, false, 15000, 15000);
    TEST_ASSERT_EQUAL(static_cast<int>(SystemState::INACTIVE), static_cast<int>(s));
}
void test_state_inactive_no_motion_stays_inactive() {
    SystemState s = evaluateSystemState(SystemState::INACTIVE, false, 999999, 15000);
    TEST_ASSERT_EQUAL(static_cast<int>(SystemState::INACTIVE), static_cast<int>(s));
}
void test_state_inactive_motion_returns_active() {
    SystemState s = evaluateSystemState(SystemState::INACTIVE, true, 0, 15000);
    TEST_ASSERT_EQUAL(static_cast<int>(SystemState::ACTIVE), static_cast<int>(s));
}

// ---------------------------------------------------------------------
void setUp(void) {}
void tearDown(void) {}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_temp_below_lower_threshold);
    RUN_TEST(test_temp_exactly_lower_threshold);
    RUN_TEST(test_temp_normal_value);
    RUN_TEST(test_temp_exactly_upper_threshold);
    RUN_TEST(test_temp_above_upper_threshold);

    RUN_TEST(test_nav_forward_step);
    RUN_TEST(test_nav_forward_wraparound);
    RUN_TEST(test_nav_reverse_step);
    RUN_TEST(test_nav_reverse_wraparound);

    RUN_TEST(test_state_active_no_timeout);
    RUN_TEST(test_state_active_timeout_elapsed);
    RUN_TEST(test_state_inactive_no_motion_stays_inactive);
    RUN_TEST(test_state_inactive_motion_returns_active);

    return UNITY_END();
}
