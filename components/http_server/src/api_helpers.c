/**
 * @file api_helpers.c
 * @brief API response and parsing helpers implementation
 */

#include "api_helpers.h"

#include <esp_log.h>

#include <stdio.h>
#include <string.h>

#include "robot_types.h"

static const char *TAG = "api_helpers";

esp_err_t api_send_success(httpd_req_t *req, const char *json_body) {
    api_set_cors_headers(req);
    httpd_resp_set_type(req, "application/json");

    char response[512];
    int len;

    if (json_body == NULL || strlen(json_body) == 0) {
        len = snprintf(response, sizeof(response), "{\"success\":true}");
    } else {
        len = snprintf(response, sizeof(response), "{\"success\":true,%s}", json_body);
    }

    if (len >= (int)sizeof(response)) {
        ESP_LOGE(TAG, "Response buffer overflow");
        return ESP_FAIL;
    }

    return httpd_resp_send(req, response, len);
}

esp_err_t api_send_error(httpd_req_t *req, int status_code, const char *message) {
    api_set_cors_headers(req);
    httpd_resp_set_type(req, "application/json");

    char status_str[4];
    snprintf(status_str, sizeof(status_str), "%d", status_code);
    httpd_resp_set_status(req, status_str);

    char response[256];
    int len = snprintf(response, sizeof(response), "{\"success\":false,\"error\":\"%s\"}",
                       message ? message : "Unknown error");

    return httpd_resp_send(req, response, len);
}

esp_err_t api_read_body(httpd_req_t *req, char *buffer, size_t buffer_size) {
    if (req == NULL || buffer == NULL || buffer_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    int content_len = req->content_len;

    if (content_len <= 0) {
        buffer[0] = '\0';
        return ESP_OK;
    }

    if ((size_t)content_len >= buffer_size) {
        ESP_LOGE(TAG, "Body too large: %d >= %zu", content_len, buffer_size);
        return ESP_FAIL;
    }

    int received = httpd_req_recv(req, buffer, content_len);
    if (received != content_len) {
        ESP_LOGE(TAG, "Failed to receive body: %d != %d", received, content_len);
        return ESP_FAIL;
    }

    buffer[content_len] = '\0';

    return ESP_OK;
}

bool api_parse_direction(const char *direction, int *result) {
    if (direction == NULL || result == NULL) {
        return false;
    }

    if (strcmp(direction, "forward") == 0) {
        *result = ROBOT_DIR_FORWARD;
        return true;
    }
    if (strcmp(direction, "backward") == 0) {
        *result = ROBOT_DIR_BACKWARD;
        return true;
    }
    if (strcmp(direction, "left") == 0) {
        *result = ROBOT_DIR_LEFT;
        return true;
    }
    if (strcmp(direction, "right") == 0) {
        *result = ROBOT_DIR_RIGHT;
        return true;
    }

    return false;
}

void api_set_cors_headers(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Authorization");
}

const char *api_get_mime_type(const char *filename) {
    if (filename == NULL) {
        return "application/octet-stream";
    }

    const char *ext = strrchr(filename, '.');
    if (ext == NULL) {
        return "application/octet-stream";
    }

    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) {
        return "text/html";
    }
    if (strcmp(ext, ".css") == 0) {
        return "text/css";
    }
    if (strcmp(ext, ".js") == 0) {
        return "application/javascript";
    }
    if (strcmp(ext, ".json") == 0) {
        return "application/json";
    }
    if (strcmp(ext, ".yaml") == 0 || strcmp(ext, ".yml") == 0) {
        return "text/yaml";
    }
    if (strcmp(ext, ".png") == 0) {
        return "image/png";
    }
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) {
        return "image/jpeg";
    }
    if (strcmp(ext, ".gif") == 0) {
        return "image/gif";
    }
    if (strcmp(ext, ".svg") == 0) {
        return "image/svg+xml";
    }
    if (strcmp(ext, ".ico") == 0) {
        return "image/x-icon";
    }

    return "application/octet-stream";
}
