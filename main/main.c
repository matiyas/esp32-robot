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
#include "crsf_input.h"
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
    motor_control_config_t config = {.enable_pin = APP_MOTORS_ENABLE,
                                     .pwm_frequency_hz = APP_PWM_FREQUENCY,
                                     .ramp_duration_ms = APP_PWM_RAMP_MS,
                                     .ramp_steps = APP_PWM_RAMP_STEPS};

#if APP_KINEMATICS_CAR
    /* CAR: one drive motor on the dedicated drive pins; right pair unused. */
    config.left_motor.in1 = APP_CAR_DRIVE_IN1;
    config.left_motor.in2 = APP_CAR_DRIVE_IN2;
    config.drive_single_pair = true;
#else
    /* TANK: two motors, skid-steer (original behavior). */
    config.left_motor.in1 = APP_MOTOR_LEFT_IN1;
    config.left_motor.in2 = APP_MOTOR_LEFT_IN2;
    config.right_motor.in1 = APP_MOTOR_RIGHT_IN1;
    config.right_motor.in2 = APP_MOTOR_RIGHT_IN2;
    config.drive_single_pair = false;
#endif

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

    /* Start separate camera stream server on different port */
    esp_err_t ret = camera_stream_start_server(APP_HTTP_PORT + 1);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Camera stream server failed to start");
    }

    ESP_LOGI(TAG, "HTTP server started on port %d", APP_HTTP_PORT);
    return ESP_OK;
}

#if APP_KINEMATICS_CAR
static bool s_rc_hw_ready = false;

/**
 * @brief Initialize the CRSF UART receiver (CAR mode only)
 */
static esp_err_t init_crsf(void) {
    crsf_input_config_t config = {.uart_num = APP_CRSF_UART_NUM,
                                  .rx_gpio = APP_CRSF_RX_GPIO,
                                  .tx_gpio = -1,
                                  .baud = APP_CRSF_BAUD,
                                  .failsafe_timeout_ms = APP_CRSF_FAILSAFE_TIMEOUT_MS,
                                  .task_priority = 6,
                                  .task_core_id = 1};

    return crsf_input_init(&config);
}

/**
 * @brief Start CRSF reception and RC car control (CAR mode only)
 */
static esp_err_t start_rc(void) {
    robot_rc_cfg_t cfg = {.steering_ch = APP_CRSF_CH_STEERING,
                          .throttle_ch = APP_CRSF_CH_THROTTLE,
                          .arm_ch = APP_CRSF_CH_ARM,
                          .throttle = {.center_tick = 0,
                                       .deadband_ticks = APP_CRSF_THROTTLE_DEADBAND,
                                       .max_duty = APP_CRSF_THROTTLE_MAX_DUTY,
                                       .reverse = APP_CRSF_THROTTLE_REVERSE},
                          .steer = {.center_tick = 0,
                                    .center_trim_deg = APP_CRSF_STEERING_TRIM,
                                    .max_angle_deg = APP_CRSF_STEERING_MAX_ANGLE,
                                    .servo_center_deg = APP_SERVO_DEFAULT,
                                    .reverse = APP_CRSF_STEERING_REVERSE},
                          .failsafe_timeout_ms = APP_CRSF_FAILSAFE_TIMEOUT_MS,
                          .task_core_id = 1};

    esp_err_t ret = crsf_input_start();
    if (ret != ESP_OK) {
        return ret;
    }
    return robot_start_rc(&cfg);
}
#endif /* APP_KINEMATICS_CAR */

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
#if APP_KINEMATICS_CAR
        /* RC is optional: a failure must not brick camera/Wi-Fi/REST. */
        if (init_crsf() == ESP_OK) {
            s_rc_hw_ready = true;
        } else {
            ESP_LOGW(TAG, "CRSF init failed - RC control disabled");
        }
#endif
        if (init_camera() != ESP_OK) {
            ESP_LOGW(TAG, "Camera init failed - continuing without camera");
        }
    } else {
        ESP_LOGI(TAG, "Hardware initialization skipped (mock mode)");
    }

    /* Initialize robot core */
    ESP_ERROR_CHECK(init_robot());

    /* Initialize safety handler */
    ESP_ERROR_CHECK(init_safety());

#if APP_KINEMATICS_CAR
    /* Start RC car control once robot core + safety are ready. */
    if (!APP_MOCK_MODE && s_rc_hw_ready) {
        if (start_rc() != ESP_OK) {
            ESP_LOGW(TAG, "RC control failed to start");
        }
    }
#endif

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
