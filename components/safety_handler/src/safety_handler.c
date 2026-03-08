/**
 * @file safety_handler.c
 * @brief Safety watchdog and emergency shutdown implementation
 */

#include "safety_handler.h"
#include "motor_control.h"
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "safety_handler";

/* Safety state */
static struct {
    bool initialized;
    safety_config_t config;
    esp_timer_handle_t auto_stop_timer;
    esp_timer_handle_t watchdog_timer;
    volatile bool watchdog_fed;
} s_safety = {0};

/**
 * @brief Auto-stop timer callback
 */
static void auto_stop_callback(void *arg)
{
    (void)arg;
    ESP_LOGW(TAG, "Auto-stop triggered");
    motor_stop_all();
}

/**
 * @brief Watchdog timer callback
 */
static void watchdog_callback(void *arg)
{
    (void)arg;

    if (!s_safety.watchdog_fed) {
        ESP_LOGE(TAG, "Watchdog timeout - emergency shutdown!");
        safety_emergency_shutdown();
    }

    s_safety.watchdog_fed = false;
}

esp_err_t safety_handler_init(const safety_config_t *config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    s_safety.config = *config;

    /* Create auto-stop timer (one-shot) */
    esp_timer_create_args_t auto_stop_args = {
        .callback = auto_stop_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "auto_stop"
    };

    esp_err_t ret = esp_timer_create(&auto_stop_args, &s_safety.auto_stop_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create auto-stop timer");
        return ret;
    }

    /* Create watchdog timer (periodic) */
    if (config->watchdog_timeout_ms > 0) {
        esp_timer_create_args_t watchdog_args = {
            .callback = watchdog_callback,
            .arg = NULL,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "watchdog"
        };

        ret = esp_timer_create(&watchdog_args, &s_safety.watchdog_timer);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to create watchdog timer");
            esp_timer_delete(s_safety.auto_stop_timer);
            return ret;
        }

        /* Start watchdog timer */
        ret = esp_timer_start_periodic(
            s_safety.watchdog_timer,
            config->watchdog_timeout_ms * 1000
        );
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to start watchdog timer");
            esp_timer_delete(s_safety.auto_stop_timer);
            esp_timer_delete(s_safety.watchdog_timer);
            return ret;
        }

        s_safety.watchdog_fed = true;
    }

    s_safety.initialized = true;

    ESP_LOGI(TAG, "Safety handler initialized (watchdog=%lu ms)",
             config->watchdog_timeout_ms);

    return ESP_OK;
}

void safety_emergency_shutdown(void)
{
    ESP_LOGE(TAG, "EMERGENCY SHUTDOWN");

    /* Stop all motors immediately */
    motor_stop_all();

    /* Cancel any pending auto-stop */
    safety_cancel_auto_stop();
}

void safety_feed_watchdog(void)
{
    s_safety.watchdog_fed = true;
}

uint32_t safety_validate_duration(uint32_t duration_ms, uint32_t max_duration_ms)
{
    if (duration_ms == 0) {
        return 0;
    }

    if (duration_ms > max_duration_ms) {
        return max_duration_ms;
    }

    return duration_ms;
}

esp_err_t safety_schedule_auto_stop(uint32_t duration_ms)
{
    if (!s_safety.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (duration_ms == 0) {
        return ESP_OK;
    }

    /* Cancel any existing auto-stop */
    safety_cancel_auto_stop();

    /* Schedule new auto-stop */
    esp_err_t ret = esp_timer_start_once(
        s_safety.auto_stop_timer,
        duration_ms * 1000
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to schedule auto-stop");
        return ret;
    }

    ESP_LOGD(TAG, "Auto-stop scheduled in %lu ms", duration_ms);

    return ESP_OK;
}

void safety_cancel_auto_stop(void)
{
    if (!s_safety.initialized) {
        return;
    }

    esp_timer_stop(s_safety.auto_stop_timer);
    ESP_LOGD(TAG, "Auto-stop cancelled");
}

void safety_handler_cleanup(void)
{
    if (!s_safety.initialized) {
        return;
    }

    /* Stop timers */
    if (s_safety.auto_stop_timer != NULL) {
        esp_timer_stop(s_safety.auto_stop_timer);
        esp_timer_delete(s_safety.auto_stop_timer);
        s_safety.auto_stop_timer = NULL;
    }

    if (s_safety.watchdog_timer != NULL) {
        esp_timer_stop(s_safety.watchdog_timer);
        esp_timer_delete(s_safety.watchdog_timer);
        s_safety.watchdog_timer = NULL;
    }

    s_safety.initialized = false;

    ESP_LOGI(TAG, "Safety handler cleanup complete");
}
