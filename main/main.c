/**
 * @file main.c
 * @brief ESP32-S3-CAM Robot Controller Entry Point
 *
 * Initializes all subsystems and starts the robot controller:
 * - WiFi connection
 * - Camera streaming
 * - Motor and servo control
 * - HTTP REST API server
 * - Safety watchdog
 */

#include <stdio.h>
#include <string.h>

#include "app_config.h"
#include "camera_stream.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "http_server.h"
#include "motor_control.h"
#include "nvs_flash.h"
#include "robot.h"
#include "safety_handler.h"
#include "servo_control.h"
#include "wifi_manager.h"

static const char *TAG = "main";

/**
 * @brief Initialize NVS flash storage
 */
static esp_err_t init_nvs(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

/**
 * @brief Initialize SPIFFS filesystem for web UI files
 */
static esp_err_t init_spiffs(void) {
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {.base_path = APP_SPIFFS_BASE_PATH,
                                  .partition_label = NULL,
                                  .max_files = APP_SPIFFS_MAX_FILES,
                                  .format_if_mount_failed = false};

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount SPIFFS");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "SPIFFS partition not found");
        } else {
            ESP_LOGE(TAG, "SPIFFS init failed: %s", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS: total=%zu, used=%zu", total, used);
    }

    return ESP_OK;
}

/**
 * @brief Initialize motor control subsystem
 */
static esp_err_t init_motors(void) {
    motor_control_config_t config = {
        .left_motor = {.in1 = APP_MOTOR_LEFT_IN1, .in2 = APP_MOTOR_LEFT_IN2},
        .right_motor = {.in1 = APP_MOTOR_RIGHT_IN1, .in2 = APP_MOTOR_RIGHT_IN2},
        .enable_pin = APP_MOTORS_ENABLE,
        .pwm_frequency_hz = APP_PWM_FREQUENCY,
        .ramp_duration_ms = APP_PWM_RAMP_MS,
        .ramp_steps = APP_PWM_RAMP_STEPS};

    return motor_control_init(&config);
}

/**
 * @brief Initialize servo control subsystem
 */
static esp_err_t init_servo(void) {
    servo_config_t config = {.signal_pin = APP_SERVO_GPIO,
                             .min_pulse_us = APP_SERVO_MIN_PULSE,
                             .max_pulse_us = APP_SERVO_MAX_PULSE,
                             .min_angle = APP_SERVO_MIN_ANGLE,
                             .max_angle = APP_SERVO_MAX_ANGLE,
                             .default_angle = APP_SERVO_DEFAULT,
                             .step_angle = APP_SERVO_STEP,
                             .smooth_step_degrees = APP_SERVO_SMOOTH_STEP,
                             .smooth_delay_ms = APP_SERVO_SMOOTH_MS};

    return servo_init(&config);
}

/**
 * @brief Initialize robot core subsystem
 */
static esp_err_t init_robot(void) {
    robot_config_t config = {.movement_timeout_ms = APP_MOVEMENT_TIMEOUT,
                             .turret_timeout_ms = APP_TURRET_TIMEOUT,
                             .gpio_enabled = !APP_MOCK_MODE,
                             .camera_url = "/stream"};

    return robot_init(&config);
}

/**
 * @brief Initialize camera subsystem
 */
static esp_err_t init_camera(void) {
    camera_stream_config_t config = {.frame_size = APP_CAMERA_FRAME_SIZE,
                                     .jpeg_quality = APP_CAMERA_JPEG_QUALITY,
                                     .fb_count = APP_CAMERA_FB_COUNT};

    return camera_stream_init(&config);
}

/**
 * @brief Initialize safety handler
 */
static esp_err_t init_safety(void) {
    safety_config_t config = {.watchdog_timeout_ms = APP_WATCHDOG_TIMEOUT * 1000,
                              .movement_timeout_ms = APP_MOVEMENT_TIMEOUT,
                              .turret_timeout_ms = APP_TURRET_TIMEOUT};

    return safety_handler_init(&config);
}

/**
 * @brief Start HTTP server with all endpoints
 */
static esp_err_t start_server(void) {
    http_server_config_t config = {
        .port = APP_HTTP_PORT, .base_path = APP_SPIFFS_BASE_PATH, .auth_enabled = false};

    httpd_handle_t server = http_server_start(&config);
    if (server == NULL) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return ESP_FAIL;
    }

    /* Register camera stream handler */
    esp_err_t ret = camera_stream_register_handler(server);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Camera stream handler registration failed");
    }

    ESP_LOGI(TAG, "HTTP server started on port %d", APP_HTTP_PORT);
    return ESP_OK;
}

/**
 * @brief Application entry point
 */
void app_main(void) {
    ESP_LOGI(TAG, "ESP32-S3-CAM Robot Controller starting...");
    ESP_LOGI(TAG, "Mock mode: %s", APP_MOCK_MODE ? "enabled" : "disabled");

    /* Initialize NVS */
    ESP_ERROR_CHECK(init_nvs());

    /* Initialize SPIFFS for web UI */
    ESP_ERROR_CHECK(init_spiffs());

    /* Initialize WiFi AP (skip in mock mode for simulation) */
    if (!APP_MOCK_MODE) {
        ESP_ERROR_CHECK(wifi_manager_init());
        ESP_ERROR_CHECK(wifi_manager_start_ap(APP_WIFI_SSID, APP_WIFI_PASSWORD));
        ESP_LOGI(TAG, "WiFi AP started");
    } else {
        ESP_LOGI(TAG, "WiFi initialization skipped (mock mode)");
    }

    /* Initialize hardware subsystems */
    if (!APP_MOCK_MODE) {
        ESP_ERROR_CHECK(init_motors());
        ESP_ERROR_CHECK(init_servo());
        ESP_ERROR_CHECK(init_camera());
    } else {
        ESP_LOGI(TAG, "Hardware initialization skipped (mock mode)");
    }

    /* Initialize robot core */
    ESP_ERROR_CHECK(init_robot());

    /* Initialize safety handler */
    ESP_ERROR_CHECK(init_safety());

    /* Start HTTP server */
    ESP_ERROR_CHECK(start_server());

    ESP_LOGI(TAG, "Robot controller initialized successfully");
    ESP_LOGI(TAG, "Web UI available at http://10.42.0.1:%d/", APP_HTTP_PORT);

    /* Main loop - feed watchdog */
    while (1) {
        safety_feed_watchdog();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
