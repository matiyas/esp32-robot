/**
 * @file api_handlers.c
 * @brief REST API route handlers implementation
 */

#include "api_handlers.h"

#include <esp_log.h>

#include <cJSON.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "api_helpers.h"
#include "camera_stream.h"
#include "robot.h"

static const char *TAG = "api_handlers";

/* SPIFFS base path for static files */
static const char *s_base_path = NULL;

/**
 * @brief Serve static file from SPIFFS
 */
static esp_err_t serve_static_file(httpd_req_t *req, const char *filename) {
    if (s_base_path == NULL) {
        return api_send_error(req, 500, "SPIFFS not mounted");
    }

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", s_base_path, filename);

    struct stat file_stat;
    if (stat(filepath, &file_stat) != 0) {
        ESP_LOGE(TAG, "File not found: %s", filepath);
        return api_send_error(req, 404, "File not found");
    }

    FILE *f = fopen(filepath, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", filepath);
        return api_send_error(req, 500, "Failed to open file");
    }

    httpd_resp_set_type(req, api_get_mime_type(filename));
    api_set_cors_headers(req);

    char buf[512];
    size_t read_bytes;

    while ((read_bytes = fread(buf, 1, sizeof(buf), f)) > 0) {
        if (httpd_resp_send_chunk(req, buf, read_bytes) != ESP_OK) {
            fclose(f);
            ESP_LOGE(TAG, "Failed to send file chunk");
            return ESP_FAIL;
        }
    }

    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0);

    return ESP_OK;
}

/**
 * @brief Handle OPTIONS preflight requests
 */
static esp_err_t handle_options(httpd_req_t *req) {
    api_set_cors_headers(req);
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

esp_err_t api_handle_move(httpd_req_t *req) {
    char body[API_MAX_BODY_SIZE];

    esp_err_t ret = api_read_body(req, body, sizeof(body));
    if (ret != ESP_OK) {
        return api_send_error(req, 400, "Failed to read body");
    }

    cJSON *json = cJSON_Parse(body);
    if (json == NULL) {
        return api_send_error(req, 400, "Invalid JSON");
    }

    /* Parse direction */
    cJSON *dir_json = cJSON_GetObjectItem(json, "direction");
    if (!cJSON_IsString(dir_json)) {
        cJSON_Delete(json);
        return api_send_error(req, 400, "Missing direction");
    }

    int direction;
    if (!api_parse_direction(dir_json->valuestring, &direction)) {
        cJSON_Delete(json);
        return api_send_error(req, 400, "Invalid direction");
    }

    /* Parse duration (optional, default 0 = continuous) */
    uint32_t duration = 0;
    cJSON *dur_json = cJSON_GetObjectItem(json, "duration");
    if (cJSON_IsNumber(dur_json)) {
        duration = (uint32_t)dur_json->valuedouble;
    }

    cJSON_Delete(json);

    /* Execute movement */
    robot_result_t result = robot_move((robot_direction_t)direction, duration);

    if (!result.success) {
        return api_send_error(req, 500, "Move failed");
    }

    char response[128];
    snprintf(response, sizeof(response), "\"action\":\"%s\",\"duration\":%lu",
             robot_action_to_str(result.action), (unsigned long)result.duration_ms);

    return api_send_success(req, response);
}

esp_err_t api_handle_turret(httpd_req_t *req) {
    char body[API_MAX_BODY_SIZE];

    esp_err_t ret = api_read_body(req, body, sizeof(body));
    if (ret != ESP_OK) {
        return api_send_error(req, 400, "Failed to read body");
    }

    cJSON *json = cJSON_Parse(body);
    if (json == NULL) {
        return api_send_error(req, 400, "Invalid JSON");
    }

    /* Parse direction */
    cJSON *dir_json = cJSON_GetObjectItem(json, "direction");
    if (!cJSON_IsString(dir_json)) {
        cJSON_Delete(json);
        return api_send_error(req, 400, "Missing direction");
    }

    /* Turret only supports left/right */
    int direction;
    if (strcmp(dir_json->valuestring, "left") == 0) {
        direction = ROBOT_DIR_LEFT;
    } else if (strcmp(dir_json->valuestring, "right") == 0) {
        direction = ROBOT_DIR_RIGHT;
    } else {
        cJSON_Delete(json);
        return api_send_error(req, 400, "Invalid direction (use left/right)");
    }

    /* Parse duration (optional) */
    uint32_t duration = 0;
    cJSON *dur_json = cJSON_GetObjectItem(json, "duration");
    if (cJSON_IsNumber(dur_json)) {
        duration = (uint32_t)dur_json->valuedouble;
    }

    cJSON_Delete(json);

    /* Execute turret movement */
    robot_result_t result = robot_turret((robot_direction_t)direction, duration);

    if (!result.success) {
        return api_send_error(req, 500, "Turret failed");
    }

    char response[128];
    snprintf(response, sizeof(response), "\"action\":\"%s\"", robot_action_to_str(result.action));

    return api_send_success(req, response);
}

esp_err_t api_handle_stop(httpd_req_t *req) {
    robot_result_t result = robot_stop();

    if (!result.success) {
        return api_send_error(req, 500, "Stop failed");
    }

    return api_send_success(req, "\"action\":\"stop\"");
}

esp_err_t api_handle_status(httpd_req_t *req) {
    robot_status_t status = robot_get_status();

    char response[256];
    snprintf(response, sizeof(response), "\"connected\":%s,\"gpio_enabled\":%s",
             status.connected ? "true" : "false", status.gpio_enabled ? "true" : "false");

    return api_send_success(req, response);
}

esp_err_t api_handle_camera(httpd_req_t *req) {
    if (!camera_stream_is_ready()) {
        return api_send_error(req, 503, "Camera not ready");
    }

    const char *path = camera_stream_get_path();

    char response[128];
    snprintf(response, sizeof(response), "\"stream_url\":\"http://10.42.0.1:4568%s\"", path);

    return api_send_success(req, response);
}

esp_err_t api_handle_health(httpd_req_t *req) {
    api_set_cors_headers(req);
    httpd_resp_set_type(req, "application/json");

    const char *response = "{\"status\":\"ok\"}";
    return httpd_resp_send(req, response, strlen(response));
}

esp_err_t api_handle_index(httpd_req_t *req) {
    return serve_static_file(req, "index.html");
}

esp_err_t api_handle_docs(httpd_req_t *req) {
    return serve_static_file(req, "docs.html");
}

/**
 * @brief Generic static file handler
 */
static esp_err_t static_file_handler(httpd_req_t *req) {
    const char *uri = req->uri;

    /* Skip leading slash */
    if (uri[0] == '/') {
        uri++;
    }

    return serve_static_file(req, uri);
}

esp_err_t api_handlers_register(httpd_handle_t server, const char *base_path) {
    if (server == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    s_base_path = base_path;

    /* API routes */
    static const httpd_uri_t routes[] = {
        {.uri = "/api/v1/move", .method = HTTP_POST, .handler = api_handle_move, .user_ctx = NULL},
        {.uri = "/api/v1/turret",
         .method = HTTP_POST,
         .handler = api_handle_turret,
         .user_ctx = NULL},
        {.uri = "/api/v1/stop", .method = HTTP_POST, .handler = api_handle_stop, .user_ctx = NULL},
        {.uri = "/api/v1/status",
         .method = HTTP_GET,
         .handler = api_handle_status,
         .user_ctx = NULL},
        {.uri = "/api/v1/camera",
         .method = HTTP_GET,
         .handler = api_handle_camera,
         .user_ctx = NULL},
        {.uri = "/health", .method = HTTP_GET, .handler = api_handle_health, .user_ctx = NULL},
        {.uri = "/", .method = HTTP_GET, .handler = api_handle_index, .user_ctx = NULL},
        {.uri = "/docs", .method = HTTP_GET, .handler = api_handle_docs, .user_ctx = NULL},
        /* CORS preflight handlers */
        {.uri = "/api/v1/move",
         .method = HTTP_OPTIONS,
         .handler = handle_options,
         .user_ctx = NULL},
        {.uri = "/api/v1/turret",
         .method = HTTP_OPTIONS,
         .handler = handle_options,
         .user_ctx = NULL},
        {.uri = "/api/v1/stop",
         .method = HTTP_OPTIONS,
         .handler = handle_options,
         .user_ctx = NULL}};

    for (size_t i = 0; i < sizeof(routes) / sizeof(routes[0]); i++) {
        esp_err_t ret = httpd_register_uri_handler(server, &routes[i]);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to register %s", routes[i].uri);
            return ret;
        }
    }

    /* Static file routes */
    static const char *static_files[] = {"/style.css", "/api-client.js", "/robot-controller.js",
                                         "/openapi.yaml"};

    for (size_t i = 0; i < sizeof(static_files) / sizeof(static_files[0]); i++) {
        httpd_uri_t static_route = {.uri = static_files[i],
                                    .method = HTTP_GET,
                                    .handler = static_file_handler,
                                    .user_ctx = NULL};

        esp_err_t ret = httpd_register_uri_handler(server, &static_route);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to register static file: %s", static_files[i]);
        }
    }

    ESP_LOGI(TAG, "API handlers registered");

    return ESP_OK;
}
