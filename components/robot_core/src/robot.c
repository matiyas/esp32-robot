/**
 * @file robot.c
 * @brief Robot controller facade implementation
 */

#include "robot.h"

#include <esp_log.h>
#include <esp_timer.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <string.h>

#include "crsf_input.h"
#include "hal_gpio.h"
#include "motor_control.h"
#include "safety_handler.h"
#include "servo_control.h"

#define LED_GPIO          4
#define RC_TASK_PERIOD_MS 15

static const char *TAG = "robot";

/* Robot state */
static struct {
    bool initialized;
    robot_config_t config;
    SemaphoreHandle_t cmd_mutex; /**< Serializes RC task vs REST hardware commands */
    robot_rc_cfg_t rc_cfg;
    volatile bool rc_running; /**< RC control task is active */
    volatile bool rc_active;  /**< RC link is currently driving */
    bool seen_disarm;         /**< Observed a disarmed state since (re)connect */
    bool neutral_seen;        /**< Observed throttle neutral since (re)connect */
    bool prev_connected;      /**< Previous RC connected state (for transitions) */
} s_robot = {0};

/* True while a fresh CRSF frame is within the failsafe window (read directly
 * from the snapshot so a stalled control task can never permanently gate REST). */
static bool rc_link_live(void) {
    if (!s_robot.rc_running) {
        return false;
    }
    crsf_snapshot_t snap = crsf_input_get_snapshot();
    return rc_link_is_active(snap.last_frame_us, esp_timer_get_time(),
                             s_robot.rc_cfg.failsafe_timeout_ms) &&
           !snap.failsafe;
}

esp_err_t robot_init(const robot_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(&s_robot.config, config, sizeof(robot_config_t));

    if (s_robot.cmd_mutex == NULL) {
        s_robot.cmd_mutex = xSemaphoreCreateMutex();
        if (s_robot.cmd_mutex == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

    s_robot.initialized = true;

    /* Initialize LED (GPIO 4 - flash LED on ESP32-CAM) */
    if (config->gpio_enabled) {
        hal_gpio_init_output(LED_GPIO);
        hal_gpio_set_level(LED_GPIO, 0);
    }

    ESP_LOGI(TAG, "Robot initialized (gpio_enabled=%d, movement_timeout=%lu, turret_timeout=%lu)",
             config->gpio_enabled, config->movement_timeout_ms, config->turret_timeout_ms);

    return ESP_OK;
}

robot_result_t robot_move(robot_direction_t direction, uint32_t duration_ms) {
    robot_result_t result = {.success = false, .duration_ms = 0, .action = ROBOT_ACTION_STOP_ALL};

    if (!s_robot.initialized) {
        ESP_LOGE(TAG, "Robot not initialized");
        return result;
    }

    /* Arbitration: the RC link is primary. Ignore REST/web movement while live. */
    if (rc_link_live()) {
        ESP_LOGW(TAG, "RC link active; ignoring REST move");
        return result;
    }

    xSemaphoreTake(s_robot.cmd_mutex, portMAX_DELAY);

    /* Validate and clamp duration */
    uint32_t validated_duration =
        safety_validate_duration(duration_ms, s_robot.config.movement_timeout_ms);

    if (validated_duration != duration_ms && duration_ms > 0) {
        ESP_LOGW(TAG, "Duration clamped from %lu to %lu ms", duration_ms, validated_duration);
    }

    esp_err_t err = ESP_OK;

    switch (direction) {
        case ROBOT_DIR_FORWARD:
            result.action = ROBOT_ACTION_FORWARD;
            if (s_robot.config.gpio_enabled) {
                err = motor_move_forward(validated_duration);
            }
            break;

        case ROBOT_DIR_BACKWARD:
            result.action = ROBOT_ACTION_BACKWARD;
            if (s_robot.config.gpio_enabled) {
                err = motor_move_backward(validated_duration);
            }
            break;

        case ROBOT_DIR_LEFT:
            result.action = ROBOT_ACTION_LEFT;
            if (s_robot.config.gpio_enabled) {
                err = motor_turn_left(validated_duration);
            }
            break;

        case ROBOT_DIR_RIGHT:
            result.action = ROBOT_ACTION_RIGHT;
            if (s_robot.config.gpio_enabled) {
                err = motor_turn_right(validated_duration);
            }
            break;

        default:
            ESP_LOGE(TAG, "Invalid direction: %d", direction);
            xSemaphoreGive(s_robot.cmd_mutex);
            return result;
    }

    if (err == ESP_OK) {
        result.success = true;
        result.duration_ms = validated_duration;

        /* Schedule auto-stop if duration specified */
        if (validated_duration > 0) {
            safety_schedule_auto_stop(validated_duration);
        }

        ESP_LOGI(TAG, "Move %s for %lu ms", robot_action_to_str(result.action), validated_duration);
    } else {
        ESP_LOGE(TAG, "Move failed: %s", esp_err_to_name(err));
    }

    xSemaphoreGive(s_robot.cmd_mutex);
    return result;
}

robot_result_t robot_turret(robot_direction_t direction, uint32_t duration_ms) {
    robot_result_t result = {.success = false, .duration_ms = 0, .action = ROBOT_ACTION_STOP_ALL};

    if (!s_robot.initialized) {
        ESP_LOGE(TAG, "Robot not initialized");
        return result;
    }

    /* Arbitration: the RC link is primary. Ignore REST/web turret while live. */
    if (rc_link_live()) {
        ESP_LOGW(TAG, "RC link active; ignoring REST turret");
        return result;
    }

    xSemaphoreTake(s_robot.cmd_mutex, portMAX_DELAY);

    /* Validate duration (for logging, servo uses step mode) */
    uint32_t validated_duration =
        safety_validate_duration(duration_ms, s_robot.config.turret_timeout_ms);

    esp_err_t err = ESP_OK;

    switch (direction) {
        case ROBOT_DIR_LEFT:
            result.action = ROBOT_ACTION_TURRET_LEFT;
            if (s_robot.config.gpio_enabled) {
                err = servo_step_left();
            }
            break;

        case ROBOT_DIR_RIGHT:
            result.action = ROBOT_ACTION_TURRET_RIGHT;
            if (s_robot.config.gpio_enabled) {
                err = servo_step_right();
            }
            break;

        default:
            ESP_LOGE(TAG, "Invalid turret direction: %d", direction);
            xSemaphoreGive(s_robot.cmd_mutex);
            return result;
    }

    if (err == ESP_OK) {
        result.success = true;
        result.duration_ms = validated_duration;
        ESP_LOGI(TAG, "Turret %s", robot_action_to_str(result.action));
    } else {
        ESP_LOGE(TAG, "Turret failed: %s", esp_err_to_name(err));
    }

    xSemaphoreGive(s_robot.cmd_mutex);
    return result;
}

robot_result_t robot_stop(void) {
    robot_result_t result = {.action = ROBOT_ACTION_STOP_ALL, .duration_ms = 0, .success = true};

    if (!s_robot.initialized) {
        ESP_LOGE(TAG, "Robot not initialized");
        result.success = false;
        return result;
    }

    /* Cancel pending auto-stop */
    safety_cancel_auto_stop();

    /* Stop motors */
    if (s_robot.config.gpio_enabled) {
        esp_err_t err = motor_stop_all();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Motor stop failed: %s", esp_err_to_name(err));
            result.success = false;
        }
    }

    ESP_LOGI(TAG, "Emergency stop");
    return result;
}

robot_status_t robot_get_status(void) {
    robot_status_t status = {.connected = s_robot.initialized,
                             .gpio_enabled = s_robot.config.gpio_enabled,
                             .rc_active = s_robot.rc_active,
                             .camera_url = s_robot.config.camera_url};

    return status;
}

robot_result_t robot_led(bool state) {
    robot_result_t result = {.action = ROBOT_ACTION_STOP_ALL, .duration_ms = 0, .success = false};

    if (!s_robot.initialized) {
        ESP_LOGE(TAG, "Robot not initialized");
        return result;
    }

    if (s_robot.config.gpio_enabled) {
        esp_err_t err = hal_gpio_init_output(LED_GPIO);
        if (err == ESP_OK) {
            err = hal_gpio_set_level(LED_GPIO, state ? 1 : 0);
        }
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "LED control failed: %s", esp_err_to_name(err));
            return result;
        }
    }

    result.success = true;
    ESP_LOGI(TAG, "LED %s", state ? "on" : "off");

    return result;
}

/* Failsafe stop: invoked by the crsf_input timer on link loss (esp_timer task).
 * Independent of the RC control task so the vehicle stops even if it stalls. */
static void robot_failsafe_cb(void) {
    if (!s_robot.config.gpio_enabled) {
        return;
    }
    motor_drive_signed(0);
    servo_move_to(s_robot.rc_cfg.steer.servo_center_deg, false);
}

/* RC control loop: polls the CRSF snapshot and drives the car (CAR kinematics).
 * Throttle is gated by arm-latch (requires a disarm->arm cycle) and a
 * neutral-before-go latch after each (re)connect. */
static void robot_rc_task(void *arg) {
    (void)arg;
    const TickType_t period = pdMS_TO_TICKS(RC_TASK_PERIOD_MS);

    while (s_robot.rc_running) {
        crsf_snapshot_t snap = crsf_input_get_snapshot();
        bool connected = rc_link_is_active(snap.last_frame_us, esp_timer_get_time(),
                                           s_robot.rc_cfg.failsafe_timeout_ms) &&
                         !snap.failsafe;

        if (connected) {
            xSemaphoreTake(s_robot.cmd_mutex, portMAX_DELAY);

            if (!s_robot.prev_connected) {
                /* REST -> RC handover: flush any stale REST command and require
                 * the operator to re-establish neutral and re-arm. */
                safety_cancel_auto_stop();
                if (s_robot.config.gpio_enabled) {
                    motor_drive_signed(0);
                }
                s_robot.seen_disarm = false;
                s_robot.neutral_seen = false;
            }

            uint16_t thr_tick = snap.channels[s_robot.rc_cfg.throttle_ch - 1];
            uint16_t str_tick = snap.channels[s_robot.rc_cfg.steering_ch - 1];

            bool armed_now = (s_robot.rc_cfg.arm_ch == 0)
                                 ? true
                                 : rc_is_armed(snap.channels[s_robot.rc_cfg.arm_ch - 1]);
            if (!armed_now) {
                s_robot.seen_disarm = true;
            }
            bool armed = s_robot.seen_disarm && armed_now;

            int8_t throttle = rc_map_throttle(thr_tick, &s_robot.rc_cfg.throttle);
            if (!s_robot.neutral_seen && throttle == 0) {
                s_robot.neutral_seen = true;
            }
            int8_t out_throttle = (armed && s_robot.neutral_seen) ? throttle : 0;
            uint8_t angle = rc_map_steering(str_tick, &s_robot.rc_cfg.steer);

            if (s_robot.config.gpio_enabled) {
                motor_drive_signed(out_throttle);
                servo_move_to(angle, false);
            }

            s_robot.rc_active = true;
            s_robot.prev_connected = true;
            xSemaphoreGive(s_robot.cmd_mutex);
        } else {
            if (s_robot.prev_connected) {
                /* RC -> lost: stop and recenter (the failsafe timer also does
                 * this); then release control back to REST/web. */
                xSemaphoreTake(s_robot.cmd_mutex, portMAX_DELAY);
                if (s_robot.config.gpio_enabled) {
                    motor_drive_signed(0);
                    servo_move_to(s_robot.rc_cfg.steer.servo_center_deg, false);
                }
                s_robot.prev_connected = false;
                s_robot.seen_disarm = false;
                s_robot.neutral_seen = false;
                xSemaphoreGive(s_robot.cmd_mutex);
            }
            s_robot.rc_active = false;
        }

        vTaskDelay(period);
    }
    vTaskDelete(NULL);
}

esp_err_t robot_start_rc(const robot_rc_cfg_t *cfg) {
    if (!s_robot.initialized || cfg == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_robot.rc_running) {
        return ESP_OK;
    }

    memcpy(&s_robot.rc_cfg, cfg, sizeof(robot_rc_cfg_t));
    crsf_input_register_failsafe_cb(robot_failsafe_cb);

    s_robot.rc_running = true;
    BaseType_t ok =
        xTaskCreatePinnedToCore(robot_rc_task, "robot_rc", 4096, NULL, 5, NULL, cfg->task_core_id);
    if (ok != pdPASS) {
        s_robot.rc_running = false;
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "RC control started (steer=ch%u, throttle=ch%u, arm=ch%u)", cfg->steering_ch,
             cfg->throttle_ch, cfg->arm_ch);
    return ESP_OK;
}

void robot_cleanup(void) {
    if (!s_robot.initialized) {
        return;
    }

    ESP_LOGI(TAG, "Robot cleanup");

    s_robot.rc_running = false;
    safety_cancel_auto_stop();

    if (s_robot.config.gpio_enabled) {
        motor_stop_all();
        motor_control_cleanup();
        servo_cleanup();
    }

    s_robot.initialized = false;
}
