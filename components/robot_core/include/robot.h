/**
 * @file robot.h
 * @brief Robot controller facade interface
 *
 * Provides the main API for controlling the robot. This component
 * orchestrates motor control, servo control, and safety features.
 * Mirrors the Ruby Robot class functionality.
 */

#ifndef ROBOT_H
#define ROBOT_H

#include <esp_err.h>

#include <stdbool.h>
#include <stdint.h>

#include "rc_map.h"
#include "robot_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Robot configuration structure
 */
typedef struct {
    uint32_t movement_timeout_ms; /**< Max movement duration (default: 5000) */
    uint32_t turret_timeout_ms;   /**< Max turret duration (default: 2000) */
    bool gpio_enabled;            /**< Hardware mode vs mock mode */
    const char *camera_url;       /**< Camera stream URL path */
} robot_config_t;

/**
 * @brief Robot action result
 */
typedef struct {
    robot_action_t action; /**< Action that was performed */
    uint32_t duration_ms;  /**< Actual duration (0 = continuous) */
    bool success;          /**< Whether action succeeded */
} robot_result_t;

/**
 * @brief RC (CRSF) control configuration for CAR kinematics
 */
typedef struct {
    uint8_t steering_ch;          /**< Steering channel, 1-based */
    uint8_t throttle_ch;          /**< Throttle channel, 1-based */
    uint8_t arm_ch;               /**< Arm AUX channel, 1-based (0 = always armed) */
    rc_throttle_cfg_t throttle;   /**< Throttle mapping */
    rc_steer_cfg_t steer;         /**< Steering mapping */
    uint32_t failsafe_timeout_ms; /**< No-frame window before failsafe */
    int task_core_id;             /**< Core to pin the RC control task to */
} robot_rc_cfg_t;

/**
 * @brief Robot status information
 */
typedef struct {
    bool connected;         /**< Controller is active */
    bool gpio_enabled;      /**< Hardware mode active */
    bool rc_active;         /**< RC (CRSF) link is driving the robot */
    const char *camera_url; /**< Camera stream URL */
} robot_status_t;

/**
 * @brief Initialize the robot subsystem
 *
 * @param config Robot configuration
 * @return ESP_OK on success
 */
esp_err_t robot_init(const robot_config_t *config);

/**
 * @brief Start RC (CRSF) control for CAR kinematics
 *
 * Spawns the control task that polls the CRSF snapshot and drives the single
 * drive motor + steering servo, registers the failsafe stop callback, and
 * makes the RC link the primary controller (REST/web movement is ignored while
 * the link is active). Call after robot_init() and crsf_input_start().
 *
 * @param cfg RC control configuration
 * @return ESP_OK on success
 */
esp_err_t robot_start_rc(const robot_rc_cfg_t *cfg);

/**
 * @brief Move the robot in specified direction
 *
 * Validates duration against timeout and dispatches to motor control.
 * Duration is clamped to movement_timeout_ms if exceeded.
 *
 * @param direction Movement direction (forward/backward/left/right)
 * @param duration_ms Duration in milliseconds (0 for continuous)
 * @return Result of the move operation
 */
robot_result_t robot_move(robot_direction_t direction, uint32_t duration_ms);

/**
 * @brief Rotate the turret
 *
 * Steps the servo in the specified direction. Duration is used for
 * validation but servo moves by fixed step angle.
 *
 * @param direction Turret direction (left/right only)
 * @param duration_ms Duration for validation (ignored for step mode)
 * @return Result of the turret operation
 */
robot_result_t robot_turret(robot_direction_t direction, uint32_t duration_ms);

/**
 * @brief Emergency stop all motors
 *
 * Immediately stops all motors and cancels pending auto-stop timers.
 * Servo holds its current position.
 *
 * @return Result of stop operation
 */
robot_result_t robot_stop(void);

/**
 * @brief Get current robot status
 *
 * @return Robot status structure
 */
robot_status_t robot_get_status(void);

/**
 * @brief Set LED state
 *
 * @param state true to turn on, false to turn off
 * @return Result of LED operation
 */
robot_result_t robot_led(bool state);

/**
 * @brief Cleanup robot resources
 *
 * Stops all motors, releases servo, and frees resources.
 */
void robot_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* ROBOT_H */
