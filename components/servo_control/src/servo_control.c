/**
 * @file servo_control.c
 * @brief SG90 servo controller implementation
 */

#include "servo_control.h"

#include <esp_log.h>
#include <esp_timer.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <rom/ets_sys.h>

#include "hal_pwm.h"
#include "hal_types.h"

static const char *TAG = "servo_control";

/* Servo state */
static struct {
    bool initialized;
    servo_config_t config;
    hal_pwm_channel_t pwm_channel;
    uint8_t current_angle;
} s_servo = {0};

/**
 * @brief Convert angle to pulse width
 */
static uint16_t angle_to_pulse(uint8_t angle) {
    if (angle < s_servo.config.min_angle) {
        angle = s_servo.config.min_angle;
    }
    if (angle > s_servo.config.max_angle) {
        angle = s_servo.config.max_angle;
    }

    uint16_t pulse_range = s_servo.config.max_pulse_us - s_servo.config.min_pulse_us;
    uint8_t angle_range = s_servo.config.max_angle - s_servo.config.min_angle;

    uint16_t pulse = s_servo.config.min_pulse_us +
                     ((angle - s_servo.config.min_angle) * pulse_range) / angle_range;

    return pulse;
}

/**
 * @brief Set servo to specific angle (internal)
 */
static esp_err_t set_angle_internal(uint8_t angle) {
    uint16_t pulse = angle_to_pulse(angle);
    esp_err_t ret = hal_pwm_set_servo_pulse(s_servo.pwm_channel, pulse);

    if (ret == ESP_OK) {
        s_servo.current_angle = angle;
        ESP_LOGI(TAG, "Servo GPIO %d: angle=%d° pulse=%uus", s_servo.config.signal_pin, angle,
                 pulse);
    }

    return ret;
}

esp_err_t servo_init(const servo_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Initialize PWM for servo (50Hz) */
    esp_err_t ret = hal_pwm_init(config->signal_pin, HAL_SERVO_PWM_FREQ, &s_servo.pwm_channel);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "PWM init failed");
        return ret;
    }

    s_servo.config = *config;
    s_servo.current_angle = config->default_angle;
    s_servo.initialized = true;

    /* Move to default position */
    ret = set_angle_internal(config->default_angle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set default angle");
        return ret;
    }

    ESP_LOGI(TAG, "Servo initialized (pin=%d, default=%d°)", config->signal_pin,
             config->default_angle);

    return ESP_OK;
}

uint8_t servo_get_angle(void) {
    return s_servo.current_angle;
}

esp_err_t servo_move_to(uint8_t angle, bool smooth) {
    if (!s_servo.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    /* Clamp angle to valid range */
    if (angle < s_servo.config.min_angle) {
        angle = s_servo.config.min_angle;
    }
    if (angle > s_servo.config.max_angle) {
        angle = s_servo.config.max_angle;
    }

    if (!smooth) {
        return set_angle_internal(angle);
    }

    /* Smooth movement with interpolation */
    uint8_t current = s_servo.current_angle;
    int8_t direction = (angle > current) ? 1 : -1;
    uint8_t step = s_servo.config.smooth_step_degrees;

    ESP_LOGD(TAG, "Smooth move from %d° to %d°", current, angle);

    while (current != angle) {
        int next = current + (direction * step);

        /* Clamp to target */
        if ((direction > 0 && next > angle) || (direction < 0 && next < angle)) {
            next = angle;
        }

        esp_err_t ret = set_angle_internal((uint8_t)next);
        if (ret != ESP_OK) {
            return ret;
        }

        current = (uint8_t)next;

        /* Precise microsecond delay for consistent timing */
        ets_delay_us(s_servo.config.smooth_delay_ms * 1000);
    }

    /* Yield once after movement to let other tasks run */
    vTaskDelay(1);

    return ESP_OK;
}

esp_err_t servo_step_left(void) {
    if (!s_servo.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    int new_angle = s_servo.current_angle + s_servo.config.step_angle;

    if (new_angle > s_servo.config.max_angle) {
        new_angle = s_servo.config.max_angle;
    }

    ESP_LOGD(TAG, "Step left: %d° -> %d°", s_servo.current_angle, new_angle);

    return servo_move_to((uint8_t)new_angle, true);
}

esp_err_t servo_step_right(void) {
    if (!s_servo.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    int new_angle = s_servo.current_angle - s_servo.config.step_angle;

    if (new_angle < s_servo.config.min_angle) {
        new_angle = s_servo.config.min_angle;
    }

    ESP_LOGD(TAG, "Step right: %d° -> %d°", s_servo.current_angle, new_angle);

    return servo_move_to((uint8_t)new_angle, true);
}

esp_err_t servo_center(void) {
    if (!s_servo.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGD(TAG, "Centering to %d°", s_servo.config.default_angle);

    return servo_move_to(s_servo.config.default_angle, true);
}

void servo_stop(void) {
    /* No-op for servo - it holds position */
}

void servo_release(void) {
    if (!s_servo.initialized) {
        return;
    }

    hal_pwm_stop(s_servo.pwm_channel);
    ESP_LOGD(TAG, "Servo released");
}

void servo_cleanup(void) {
    if (!s_servo.initialized) {
        return;
    }

    servo_release();
    hal_pwm_cleanup(s_servo.pwm_channel);
    s_servo.initialized = false;

    ESP_LOGI(TAG, "Servo cleanup complete");
}
