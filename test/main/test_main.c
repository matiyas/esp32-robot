/**
 * @file test_main.c
 * @brief Unity test runner entry point
 */

#include <unity.h>

/* Test function declarations */
extern void test_robot_init(void);
extern void test_robot_move_forward(void);
extern void test_robot_move_backward(void);
extern void test_robot_turn_left(void);
extern void test_robot_turn_right(void);
extern void test_robot_stop(void);
extern void test_robot_led_on(void);
extern void test_robot_led_off(void);

extern void test_motor_control_init(void);
extern void test_motor_direction_forward(void);
extern void test_motor_direction_backward(void);
extern void test_motor_stop(void);

extern void test_servo_init(void);
extern void test_servo_step_left(void);
extern void test_servo_step_right(void);
extern void test_servo_center(void);

extern void test_safety_validate_duration(void);
extern void test_safety_auto_stop(void);

void setUp(void)
{
    /* Called before each test */
}

void tearDown(void)
{
    /* Called after each test */
}

void app_main(void)
{
    UNITY_BEGIN();

    /* Robot core tests */
    RUN_TEST(test_robot_init);
    RUN_TEST(test_robot_move_forward);
    RUN_TEST(test_robot_move_backward);
    RUN_TEST(test_robot_turn_left);
    RUN_TEST(test_robot_turn_right);
    RUN_TEST(test_robot_stop);
    RUN_TEST(test_robot_led_on);
    RUN_TEST(test_robot_led_off);

    /* Motor control tests */
    RUN_TEST(test_motor_control_init);
    RUN_TEST(test_motor_direction_forward);
    RUN_TEST(test_motor_direction_backward);
    RUN_TEST(test_motor_stop);

    /* Servo control tests */
    RUN_TEST(test_servo_init);
    RUN_TEST(test_servo_step_left);
    RUN_TEST(test_servo_step_right);
    RUN_TEST(test_servo_center);

    /* Safety handler tests */
    RUN_TEST(test_safety_validate_duration);
    RUN_TEST(test_safety_auto_stop);

    UNITY_END();
}
