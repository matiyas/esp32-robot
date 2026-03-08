/**
 * @file hal_gpio.h
 * @brief GPIO Hardware Abstraction Layer
 *
 * Provides a platform-independent interface for GPIO operations.
 * Can be mocked for testing without hardware.
 */

#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include "hal_types.h"
#include <esp_err.h>
#include <driver/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize GPIO pin as output
 *
 * Configures the specified GPIO pin for output mode with no pull-up/pull-down.
 *
 * @param pin GPIO pin number
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t hal_gpio_init_output(gpio_num_t pin);

/**
 * @brief Set GPIO pin output level
 *
 * @param pin GPIO pin number
 * @param level Output level (0 = LOW, 1 = HIGH)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t hal_gpio_set_level(gpio_num_t pin, uint8_t level);

/**
 * @brief Get GPIO pin input level
 *
 * @param pin GPIO pin number
 * @return Pin level (0 or 1)
 */
uint8_t hal_gpio_get_level(gpio_num_t pin);

/**
 * @brief Reset GPIO pin to default state
 *
 * Sets pin to LOW output level.
 *
 * @param pin GPIO pin number
 */
void hal_gpio_reset(gpio_num_t pin);

/**
 * @brief Reset multiple GPIO pins
 *
 * @param pins Array of GPIO pin numbers
 * @param count Number of pins in array
 */
void hal_gpio_reset_multiple(const gpio_num_t *pins, size_t count);

#ifdef __cplusplus
}
#endif

#endif /* HAL_GPIO_H */
