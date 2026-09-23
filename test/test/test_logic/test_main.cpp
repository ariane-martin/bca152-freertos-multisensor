#include <unity.h>

#include "alarm.h"
#include "display_mode.h"
#include "system_state.h"

void setUp(void)
{
}

void tearDown(void)
{
}

/* ===== Temperature Alarm Tests ===== */

void test_temperature_normal_25(void)
{
    TEST_ASSERT_EQUAL(ALARM_NORMAL, evaluateTemperature(25.0f));
}

void test_temperature_low_17(void)
{
    TEST_ASSERT_EQUAL(ALARM_LOW_TEMPERATURE, evaluateTemperature(17.0f));
}

void test_temperature_high_31(void)
{
    TEST_ASSERT_EQUAL(ALARM_HIGH_TEMPERATURE, evaluateTemperature(31.0f));
}

void test_temperature_lower_boundary_18(void)
{
    TEST_ASSERT_EQUAL(ALARM_NORMAL, evaluateTemperature(18.0f));
}

void test_temperature_upper_boundary_30(void)
{
    TEST_ASSERT_EQUAL(ALARM_NORMAL, evaluateTemperature(30.0f));
}

/* ===== Display Navigation Tests ===== */

void test_next_temperature_to_humidity(void)
{
    TEST_ASSERT_EQUAL(
        DISPLAY_HUMIDITY,
        nextDisplayMode(DISPLAY_TEMPERATURE)
    );
}

void test_next_humidity_to_light(void)
{
    TEST_ASSERT_EQUAL(
        DISPLAY_LIGHT,
        nextDisplayMode(DISPLAY_HUMIDITY)
    );
}

void test_next_motion_wraps_to_temperature(void)
{
    TEST_ASSERT_EQUAL(
        DISPLAY_TEMPERATURE,
        nextDisplayMode(DISPLAY_MOTION)
    );
}

void test_previous_humidity_to_temperature(void)
{
    TEST_ASSERT_EQUAL(
        DISPLAY_TEMPERATURE,
        previousDisplayMode(DISPLAY_HUMIDITY)
    );
}

void test_previous_temperature_wraps_to_motion(void)
{
    TEST_ASSERT_EQUAL(
        DISPLAY_MOTION,
        previousDisplayMode(DISPLAY_TEMPERATURE)
    );
}

/* ===== System State Tests ===== */

void test_active_remains_active_without_timeout(void)
{
    TEST_ASSERT_EQUAL(
        SYSTEM_ACTIVE,
        evaluateSystemState(SYSTEM_ACTIVE, false, false)
    );
}

void test_active_becomes_inactive_after_timeout(void)
{
    TEST_ASSERT_EQUAL(
        SYSTEM_INACTIVE,
        evaluateSystemState(SYSTEM_ACTIVE, false, true)
    );
}

void test_motion_wakes_inactive_system(void)
{
    TEST_ASSERT_EQUAL(
        SYSTEM_ACTIVE,
        evaluateSystemState(SYSTEM_INACTIVE, true, false)
    );
}

void test_inactive_remains_inactive_without_motion(void)
{
    TEST_ASSERT_EQUAL(
        SYSTEM_INACTIVE,
        evaluateSystemState(SYSTEM_INACTIVE, false, false)
    );
}

int main(void)
{
    UNITY_BEGIN();

    /* Temperature Alarm Tests */
    RUN_TEST(test_temperature_normal_25);
    RUN_TEST(test_temperature_low_17);
    RUN_TEST(test_temperature_high_31);
    RUN_TEST(test_temperature_lower_boundary_18);
    RUN_TEST(test_temperature_upper_boundary_30);

    /* Display Navigation Tests */
    RUN_TEST(test_next_temperature_to_humidity);
    RUN_TEST(test_next_humidity_to_light);
    RUN_TEST(test_next_motion_wraps_to_temperature);
    RUN_TEST(test_previous_humidity_to_temperature);
    RUN_TEST(test_previous_temperature_wraps_to_motion);

    /* System State Tests */
    RUN_TEST(test_active_remains_active_without_timeout);
    RUN_TEST(test_active_becomes_inactive_after_timeout);
    RUN_TEST(test_motion_wakes_inactive_system);
    RUN_TEST(test_inactive_remains_inactive_without_motion);

    return UNITY_END();
}