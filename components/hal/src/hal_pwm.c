/**
 * @file hal_pwm.c
 * @brief PWM HAL implementation using ESP-IDF LEDC driver
 */

#include "hal_pwm.h"
#include <esp_log.h>
#include <driver/ledc.h>

static const char *TAG = "hal_pwm";

/* LEDC configuration */
#define LEDC_TIMER_RESOLUTION LEDC_TIMER_13_BIT
#define LEDC_MAX_DUTY         ((1 << LEDC_TIMER_RESOLUTION) - 1)

/* Track allocated channels */
static bool channel_used[HAL_PWM_MAX_CHANNELS] = {false};
static uint32_t channel_freq[HAL_PWM_MAX_CHANNELS] = {0};

/**
 * @brief Find next available LEDC channel
 */
static hal_pwm_channel_t find_free_channel(void)
{
    for (uint8_t i = 0; i < HAL_PWM_MAX_CHANNELS; i++) {
        if (!channel_used[i]) {
            return i;
        }
    }
    return HAL_PWM_CHANNEL_INVALID;
}

esp_err_t hal_pwm_init(gpio_num_t pin, uint32_t frequency_hz, hal_pwm_channel_t *channel)
{
    if (channel == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    hal_pwm_channel_t ch = find_free_channel();
    if (ch == HAL_PWM_CHANNEL_INVALID) {
        ESP_LOGE(TAG, "No free PWM channels available");
        return ESP_ERR_NO_MEM;
    }

    /* Configure LEDC timer */
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = (ledc_timer_t)(ch / 2),  /* 2 channels per timer */
        .duty_resolution = LEDC_TIMER_RESOLUTION,
        .freq_hz = frequency_hz,
        .clk_cfg = LEDC_AUTO_CLK
    };

    esp_err_t ret = ledc_timer_config(&timer_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC timer config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Configure LEDC channel */
    ledc_channel_config_t channel_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = (ledc_channel_t)ch,
        .timer_sel = (ledc_timer_t)(ch / 2),
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = pin,
        .duty = 0,
        .hpoint = 0
    };

    ret = ledc_channel_config(&channel_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC channel config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    channel_used[ch] = true;
    channel_freq[ch] = frequency_hz;
    *channel = ch;

    ESP_LOGI(TAG, "PWM channel %d initialized on GPIO %d at %lu Hz",
             ch, pin, frequency_hz);

    return ESP_OK;
}

esp_err_t hal_pwm_set_duty(hal_pwm_channel_t channel, uint8_t duty_percent)
{
    if (!hal_pwm_is_valid(channel)) {
        return ESP_ERR_INVALID_ARG;
    }

    if (duty_percent > HAL_PWM_DUTY_MAX) {
        duty_percent = HAL_PWM_DUTY_MAX;
    }

    uint32_t duty = (LEDC_MAX_DUTY * duty_percent) / HAL_PWM_DUTY_MAX;

    esp_err_t ret = ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)channel, duty);
    if (ret == ESP_OK) {
        ret = ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)channel);
    }

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set duty: %s", esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t hal_pwm_set_servo_pulse(hal_pwm_channel_t channel, uint16_t pulse_us)
{
    if (!hal_pwm_is_valid(channel)) {
        return ESP_ERR_INVALID_ARG;
    }

    /*
     * For servo control at 50Hz:
     * Period = 20000us (20ms)
     * Duty cycle = (pulse_us / 20000) * max_duty
     */
    uint32_t period_us = 1000000 / channel_freq[channel];
    uint32_t duty = (LEDC_MAX_DUTY * pulse_us) / period_us;

    if (duty > LEDC_MAX_DUTY) {
        duty = LEDC_MAX_DUTY;
    }

    esp_err_t ret = ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)channel, duty);
    if (ret == ESP_OK) {
        ret = ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)channel);
    }

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set servo pulse: %s", esp_err_to_name(ret));
    }

    return ret;
}

void hal_pwm_stop(hal_pwm_channel_t channel)
{
    if (!hal_pwm_is_valid(channel)) {
        return;
    }

    ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)channel, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)channel);
    ESP_LOGD(TAG, "PWM channel %d stopped", channel);
}

void hal_pwm_cleanup(hal_pwm_channel_t channel)
{
    if (!hal_pwm_is_valid(channel)) {
        return;
    }

    hal_pwm_stop(channel);
    ledc_stop(LEDC_LOW_SPEED_MODE, (ledc_channel_t)channel, 0);
    channel_used[channel] = false;
    channel_freq[channel] = 0;

    ESP_LOGI(TAG, "PWM channel %d cleaned up", channel);
}

bool hal_pwm_is_valid(hal_pwm_channel_t channel)
{
    return channel < HAL_PWM_MAX_CHANNELS && channel_used[channel];
}
