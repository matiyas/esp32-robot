/**
 * @file camera_stream.c
 * @brief Camera MJPEG streaming implementation
 */

#include "camera_stream.h"

#include <esp_camera.h>
#include <esp_log.h>

#include <string.h>

static const char *TAG = "camera_stream";

/* MJPEG stream content type */
#define PART_BOUNDARY "123456789000000000000987654321"
static const char *STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

/* Camera state */
static struct {
    bool initialized;
    camera_stream_config_t config;
} s_camera = {0};

/* Stream URL path */
static const char *STREAM_PATH = "/stream";

/**
 * @brief MJPEG stream handler
 */
static esp_err_t stream_handler(httpd_req_t *req) {
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    char part_buf[64];

    res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
    if (res != ESP_OK) {
        return res;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "X-Framerate", "25");

    ESP_LOGI(TAG, "MJPEG stream started");

    while (true) {
        fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            res = ESP_FAIL;
            break;
        }

        if (fb->format != PIXFORMAT_JPEG) {
            ESP_LOGE(TAG, "Camera not in JPEG mode");
            esp_camera_fb_return(fb);
            res = ESP_FAIL;
            break;
        }

        size_t hlen = snprintf(part_buf, sizeof(part_buf), STREAM_PART, fb->len);

        res = httpd_resp_send_chunk(req, STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
        if (res != ESP_OK) {
            esp_camera_fb_return(fb);
            break;
        }

        res = httpd_resp_send_chunk(req, part_buf, hlen);
        if (res != ESP_OK) {
            esp_camera_fb_return(fb);
            break;
        }

        res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
        esp_camera_fb_return(fb);

        if (res != ESP_OK) {
            break;
        }
    }

    ESP_LOGI(TAG, "MJPEG stream ended");

    return res;
}

esp_err_t camera_stream_init(const camera_stream_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* AI-Thinker ESP32-CAM pin configuration */
    camera_config_t cam_config = {.pin_pwdn = 32,
                                  .pin_reset = -1,
                                  .pin_xclk = 0,
                                  .pin_sccb_sda = 26,
                                  .pin_sccb_scl = 27,
                                  .pin_d7 = 35,
                                  .pin_d6 = 34,
                                  .pin_d5 = 39,
                                  .pin_d4 = 36,
                                  .pin_d3 = 21,
                                  .pin_d2 = 19,
                                  .pin_d1 = 18,
                                  .pin_d0 = 5,
                                  .pin_vsync = 25,
                                  .pin_href = 23,
                                  .pin_pclk = 22,

                                  .xclk_freq_hz = 20000000,
                                  .ledc_timer = LEDC_TIMER_0,
                                  .ledc_channel = LEDC_CHANNEL_0,

                                  .pixel_format = PIXFORMAT_JPEG,
                                  .frame_size = config->frame_size,
                                  .jpeg_quality = config->jpeg_quality,
                                  .fb_count = config->fb_count,
                                  .fb_location = CAMERA_FB_IN_PSRAM,
                                  .grab_mode = CAMERA_GRAB_WHEN_EMPTY};

    esp_err_t ret = esp_camera_init(&cam_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Adjust sensor settings for better quality */
    sensor_t *sensor = esp_camera_sensor_get();
    if (sensor) {
        sensor->set_brightness(sensor, 0);
        sensor->set_contrast(sensor, 0);
        sensor->set_saturation(sensor, 0);
        sensor->set_whitebal(sensor, 1);
        sensor->set_awb_gain(sensor, 1);
        sensor->set_wb_mode(sensor, 0);
        sensor->set_exposure_ctrl(sensor, 1);
        sensor->set_aec2(sensor, 1);
        sensor->set_gain_ctrl(sensor, 1);
        sensor->set_agc_gain(sensor, 0);
        sensor->set_gainceiling(sensor, (gainceiling_t)2);
        sensor->set_bpc(sensor, 1);
        sensor->set_wpc(sensor, 1);
        sensor->set_raw_gma(sensor, 1);
        sensor->set_lenc(sensor, 1);
        sensor->set_hmirror(sensor, 0);
        sensor->set_vflip(sensor, 0);
    }

    s_camera.config = *config;
    s_camera.initialized = true;

    ESP_LOGI(TAG, "Camera initialized (frame_size=%d, quality=%d, fb_count=%d)", config->frame_size,
             config->jpeg_quality, config->fb_count);

    return ESP_OK;
}

esp_err_t camera_stream_register_handler(httpd_handle_t server) {
    if (!s_camera.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (server == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    httpd_uri_t stream_uri = {
        .uri = STREAM_PATH, .method = HTTP_GET, .handler = stream_handler, .user_ctx = NULL};

    esp_err_t ret = httpd_register_uri_handler(server, &stream_uri);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register stream handler");
        return ret;
    }

    ESP_LOGI(TAG, "Stream handler registered at %s", STREAM_PATH);

    return ESP_OK;
}

const char *camera_stream_get_path(void) {
    return STREAM_PATH;
}

bool camera_stream_is_ready(void) {
    return s_camera.initialized;
}

void camera_stream_cleanup(void) {
    if (!s_camera.initialized) {
        return;
    }

    esp_camera_deinit();
    s_camera.initialized = false;

    ESP_LOGI(TAG, "Camera cleanup complete");
}
