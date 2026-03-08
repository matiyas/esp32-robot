/**
 * @file pwm_ramper.c
 * @brief PWM soft-start ramper implementation
 */

#include "pwm_ramper.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

static const char *TAG = "pwm_ramper";

/* Ramper state */
static struct {
    bool initialized;
    hal_pwm_channel_t channel;
    pwm_ramper_config_t config;
    TaskHandle_t task_handle;
    SemaphoreHandle_t mutex;
    volatile bool ramp_active;
    volatile bool stop_requested;
    uint8_t current_duty;
} s_ramper = {0};

/**
 * @brief Ramp task - runs in background
 */
static void ramp_task(void *arg)
{
    (void)arg;

    while (1) {
        /* Wait for notification to start ramping */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (s_ramper.stop_requested) {
            continue;
        }

        xSemaphoreTake(s_ramper.mutex, portMAX_DELAY);
        s_ramper.ramp_active = true;
        xSemaphoreGive(s_ramper.mutex);

        uint32_t step_delay_ms =
            s_ramper.config.ramp_duration_ms / s_ramper.config.num_steps;
        uint8_t duty_step =
            s_ramper.config.max_duty_percent / s_ramper.config.num_steps;

        if (duty_step == 0) {
            duty_step = 1;
        }

        ESP_LOGD(TAG, "Starting ramp: %lu ms, %d steps, %d%% max",
                 s_ramper.config.ramp_duration_ms,
                 s_ramper.config.num_steps,
                 s_ramper.config.max_duty_percent);

        s_ramper.current_duty = 0;

        for (uint8_t step = 0; step < s_ramper.config.num_steps; step++) {
            if (s_ramper.stop_requested) {
                break;
            }

            s_ramper.current_duty += duty_step;
            if (s_ramper.current_duty > s_ramper.config.max_duty_percent) {
                s_ramper.current_duty = s_ramper.config.max_duty_percent;
            }

            hal_pwm_set_duty(s_ramper.channel, s_ramper.current_duty);

            vTaskDelay(pdMS_TO_TICKS(step_delay_ms));
        }

        /* Ensure max duty is set at the end */
        if (!s_ramper.stop_requested) {
            s_ramper.current_duty = s_ramper.config.max_duty_percent;
            hal_pwm_set_duty(s_ramper.channel, s_ramper.current_duty);
        }

        xSemaphoreTake(s_ramper.mutex, portMAX_DELAY);
        s_ramper.ramp_active = false;
        xSemaphoreGive(s_ramper.mutex);

        ESP_LOGD(TAG, "Ramp complete, duty=%d%%", s_ramper.current_duty);
    }
}

esp_err_t pwm_ramper_init(hal_pwm_channel_t channel, const pwm_ramper_config_t *config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!hal_pwm_is_valid(channel)) {
        ESP_LOGE(TAG, "Invalid PWM channel");
        return ESP_ERR_INVALID_ARG;
    }

    s_ramper.channel = channel;
    s_ramper.config = *config;
    s_ramper.ramp_active = false;
    s_ramper.stop_requested = false;
    s_ramper.current_duty = 0;

    /* Create mutex */
    s_ramper.mutex = xSemaphoreCreateMutex();
    if (s_ramper.mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }

    /* Create ramp task */
    BaseType_t ret = xTaskCreate(
        ramp_task,
        "pwm_ramp",
        2048,
        NULL,
        5,
        &s_ramper.task_handle
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create ramp task");
        vSemaphoreDelete(s_ramper.mutex);
        return ESP_ERR_NO_MEM;
    }

    s_ramper.initialized = true;

    ESP_LOGI(TAG, "PWM ramper initialized (duration=%lu ms, steps=%d)",
             config->ramp_duration_ms, config->num_steps);

    return ESP_OK;
}

void pwm_ramper_start(void)
{
    if (!s_ramper.initialized) {
        return;
    }

    /* Cancel any active ramp */
    s_ramper.stop_requested = true;

    /* Wait for ramp to stop */
    while (s_ramper.ramp_active) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    s_ramper.stop_requested = false;

    /* Notify task to start ramping */
    xTaskNotifyGive(s_ramper.task_handle);
}

void pwm_ramper_stop(void)
{
    if (!s_ramper.initialized) {
        return;
    }

    s_ramper.stop_requested = true;

    /* Wait for ramp to stop */
    while (s_ramper.ramp_active) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    /* Set duty to zero */
    s_ramper.current_duty = 0;
    hal_pwm_set_duty(s_ramper.channel, 0);

    s_ramper.stop_requested = false;
}

void pwm_ramper_set_duty(uint8_t duty_percent)
{
    if (!s_ramper.initialized) {
        return;
    }

    /* Stop any active ramp */
    pwm_ramper_stop();

    /* Set immediate duty */
    if (duty_percent > s_ramper.config.max_duty_percent) {
        duty_percent = s_ramper.config.max_duty_percent;
    }

    s_ramper.current_duty = duty_percent;
    hal_pwm_set_duty(s_ramper.channel, duty_percent);
}

bool pwm_ramper_is_active(void)
{
    return s_ramper.ramp_active;
}

void pwm_ramper_cleanup(void)
{
    if (!s_ramper.initialized) {
        return;
    }

    pwm_ramper_stop();

    if (s_ramper.task_handle != NULL) {
        vTaskDelete(s_ramper.task_handle);
        s_ramper.task_handle = NULL;
    }

    if (s_ramper.mutex != NULL) {
        vSemaphoreDelete(s_ramper.mutex);
        s_ramper.mutex = NULL;
    }

    s_ramper.initialized = false;

    ESP_LOGI(TAG, "PWM ramper cleanup complete");
}
