/**
 * @file http_server.h
 * @brief HTTP server initialization and management
 */

#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <esp_err.h>
#include <esp_http_server.h>

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief HTTP server configuration
 */
typedef struct {
    uint16_t port;         /**< Server port (default: 80) */
    const char *base_path; /**< SPIFFS base path for static files */
    bool auth_enabled;     /**< Enable authentication (future) */
} http_server_config_t;

/**
 * @brief Start HTTP server
 *
 * Initializes server and registers all API endpoints.
 *
 * @param config Server configuration
 * @return Server handle or NULL on failure
 */
httpd_handle_t http_server_start(const http_server_config_t *config);

/**
 * @brief Stop HTTP server
 *
 * @param server Server handle
 * @return ESP_OK on success
 */
esp_err_t http_server_stop(httpd_handle_t server);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_SERVER_H */
