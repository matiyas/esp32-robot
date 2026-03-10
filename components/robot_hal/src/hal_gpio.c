/**
 * @file hal_gpio.c
 * @brief GPIO HAL implementation using ESP-IDF driver
 */

#include "hal_gpio.h"

#include <esp_log.h>

static const char *TAG = "hal_gpio";

esp_err_t hal_gpio_init_output(gpio_num_t pin) {
    gpio_config_t io_conf = {.pin_bit_mask = (1ULL << pin),
                             .mode = GPIO_MODE_OUTPUT,
                             .pull_up_en = GPIO_PULLUP_DISABLE,
                             .pull_down_en = GPIO_PULLDOWN_DISABLE,
                             .intr_type = GPIO_INTR_DISABLE};

    esp_err_t ret = gpio_config(&io_conf);
    if (ret == ESP_OK) {
        gpio_set_level(pin, 0);
        ESP_LOGD(TAG, "GPIO %d initialized as output", pin);
    } else {
        ESP_LOGE(TAG, "Failed to init GPIO %d: %s", pin, esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t hal_gpio_set_level(gpio_num_t pin, uint8_t level) {
    esp_err_t ret = gpio_set_level(pin, level ? 1 : 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set GPIO %d to %d: %s", pin, level, esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "GPIO %d set to %d", pin, level ? 1 : 0);
    }
    return ret;
}

uint8_t hal_gpio_get_level(gpio_num_t pin) {
    return gpio_get_level(pin) ? 1 : 0;
}

void hal_gpio_reset(gpio_num_t pin) {
    gpio_set_level(pin, 0);
    ESP_LOGD(TAG, "GPIO %d reset to LOW", pin);
}

void hal_gpio_reset_multiple(const gpio_num_t *pins, size_t count) {
    for (size_t i = 0; i < count; i++) {
        hal_gpio_reset(pins[i]);
    }
    ESP_LOGD(TAG, "Reset %zu GPIO pins", count);
}
