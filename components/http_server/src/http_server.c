/**
 * @file http_server.c
 * @brief HTTP server initialization and management implementation
 */

#include "http_server.h"

#include <esp_log.h>

#include "api_handlers.h"

static const char *TAG = "http_server";

httpd_handle_t http_server_start(const http_server_config_t *config) {
    if (config == NULL) {
        ESP_LOGE(TAG, "Invalid config");
        return NULL;
    }

    httpd_config_t httpd_config = HTTPD_DEFAULT_CONFIG();
    httpd_config.server_port = config->port;
    httpd_config.max_uri_handlers = 20;
    httpd_config.max_resp_headers = 8;
    httpd_config.stack_size = 8192;
    httpd_config.lru_purge_enable = true;

    httpd_handle_t server = NULL;

    esp_err_t ret = httpd_start(&server, &httpd_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(ret));
        return NULL;
    }

    /* Register API handlers */
    ret = api_handlers_register(server, config->base_path);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register API handlers");
        httpd_stop(server);
        return NULL;
    }

    ESP_LOGI(TAG, "HTTP server started on port %d", config->port);

    return server;
}

esp_err_t http_server_stop(httpd_handle_t server) {
    if (server == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = httpd_stop(server);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "HTTP server stopped");
    }

    return ret;
}
