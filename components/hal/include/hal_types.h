/**
 * @file hal_types.h
 * @brief Hardware Abstraction Layer type definitions
 */

#ifndef HAL_TYPES_H
#define HAL_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PWM channel handle
 */
typedef uint8_t hal_pwm_channel_t;

/**
 * @brief Invalid PWM channel marker
 */
#define HAL_PWM_CHANNEL_INVALID 0xFF

/**
 * @brief Maximum number of PWM channels
 */
#define HAL_PWM_MAX_CHANNELS 8

/**
 * @brief PWM duty cycle maximum value (100%)
 */
#define HAL_PWM_DUTY_MAX 100

/**
 * @brief Servo PWM frequency (Hz)
 */
#define HAL_SERVO_PWM_FREQ 50

#ifdef __cplusplus
}
#endif

#endif /* HAL_TYPES_H */
