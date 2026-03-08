/**
 * @file robot_types.h
 * @brief Shared type definitions for robot control
 */

#ifndef ROBOT_TYPES_H
#define ROBOT_TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Movement/turret directions
 */
typedef enum {
    ROBOT_DIR_FORWARD = 0,
    ROBOT_DIR_BACKWARD,
    ROBOT_DIR_LEFT,
    ROBOT_DIR_RIGHT
} robot_direction_t;

/**
 * @brief Robot action types (for API responses)
 */
typedef enum {
    ROBOT_ACTION_FORWARD = 0,
    ROBOT_ACTION_BACKWARD,
    ROBOT_ACTION_LEFT,
    ROBOT_ACTION_RIGHT,
    ROBOT_ACTION_TURRET_LEFT,
    ROBOT_ACTION_TURRET_RIGHT,
    ROBOT_ACTION_STOP_ALL
} robot_action_t;

/**
 * @brief Motor direction modes (DRV8833 truth table)
 */
typedef enum {
    MOTOR_MODE_COAST = 0,   /**< IN1=LOW,  IN2=LOW  - freewheeling */
    MOTOR_MODE_FORWARD,     /**< IN1=HIGH, IN2=LOW  */
    MOTOR_MODE_BACKWARD,    /**< IN1=LOW,  IN2=HIGH */
    MOTOR_MODE_BRAKE        /**< IN1=HIGH, IN2=HIGH */
} motor_mode_t;

/**
 * @brief Convert direction enum to string
 */
static inline const char *robot_direction_to_str(robot_direction_t dir)
{
    switch (dir) {
        case ROBOT_DIR_FORWARD:  return "forward";
        case ROBOT_DIR_BACKWARD: return "backward";
        case ROBOT_DIR_LEFT:     return "left";
        case ROBOT_DIR_RIGHT:    return "right";
        default:                 return "unknown";
    }
}

/**
 * @brief Convert action enum to string
 */
static inline const char *robot_action_to_str(robot_action_t action)
{
    switch (action) {
        case ROBOT_ACTION_FORWARD:      return "forward";
        case ROBOT_ACTION_BACKWARD:     return "backward";
        case ROBOT_ACTION_LEFT:         return "left";
        case ROBOT_ACTION_RIGHT:        return "right";
        case ROBOT_ACTION_TURRET_LEFT:  return "turret_left";
        case ROBOT_ACTION_TURRET_RIGHT: return "turret_right";
        case ROBOT_ACTION_STOP_ALL:     return "stop_all";
        default:                        return "unknown";
    }
}

#ifdef __cplusplus
}
#endif

#endif /* ROBOT_TYPES_H */
