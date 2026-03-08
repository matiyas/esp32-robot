/**
 * @file safety_handler.h
 * @brief Safety watchdog and emergency shutdown
 */

#ifndef SAFETY_HANDLER_H
#define SAFETY_HANDLER_H

#include <esp_err.h>

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Safety handler configuration
 */
typedef struct {
    uint32_t watchdog_timeout_ms; /**< Watchdog timeout (default: 10000) */
    uint32_t movement_timeout_ms; /**< Max movement duration */
    uint32_t turret_timeout_ms;   /**< Max turret duration */
} safety_config_t;

/**
 * @brief Initialize safety handler
 *
 * @param config Safety configuration
 * @return ESP_OK on success
 */
esp_err_t safety_handler_init(const safety_config_t *config);

/**
 * @brief Trigger emergency shutdown
 *
 * Immediately stops all motors and resets GPIO to safe state.
 */
void safety_emergency_shutdown(void);

/**
 * @brief Feed the watchdog timer
 *
 * Call periodically to prevent watchdog reset.
 */
void safety_feed_watchdog(void);

/**
 * @brief Validate and clamp duration
 *
 * @param duration_ms Requested duration
 * @param max_duration_ms Maximum allowed duration
 * @return Validated duration (clamped to max)
 */
uint32_t safety_validate_duration(uint32_t duration_ms, uint32_t max_duration_ms);

/**
 * @brief Schedule auto-stop after duration
 *
 * Creates a timer that will call motor_stop_all() after duration.
 *
 * @param duration_ms Duration before auto-stop
 * @return ESP_OK on success
 */
esp_err_t safety_schedule_auto_stop(uint32_t duration_ms);

/**
 * @brief Cancel pending auto-stop timer
 */
void safety_cancel_auto_stop(void);

/**
 * @brief Cleanup safety handler
 */
void safety_handler_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* SAFETY_HANDLER_H */
