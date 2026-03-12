/**
 * @file api_handlers.h
 * @brief REST API route handlers
 *
 * Implements all API endpoints matching the OpenAPI specification.
 */

#ifndef API_HANDLERS_H
#define API_HANDLERS_H

#include <esp_err.h>
#include <esp_http_server.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register all API route handlers
 *
 * @param server HTTP server handle
 * @param base_path SPIFFS base path for static files
 * @return ESP_OK on success
 */
esp_err_t api_handlers_register(httpd_handle_t server, const char *base_path);

/* Individual handlers for testing */

/**
 * @brief Handle POST /api/v1/move
 */
esp_err_t api_handle_move(httpd_req_t *req);

/**
 * @brief Handle POST /api/v1/turret
 */
esp_err_t api_handle_turret(httpd_req_t *req);

/**
 * @brief Handle POST /api/v1/stop
 */
esp_err_t api_handle_stop(httpd_req_t *req);

/**
 * @brief Handle GET /api/v1/status
 */
esp_err_t api_handle_status(httpd_req_t *req);

/**
 * @brief Handle GET /api/v1/camera
 */
esp_err_t api_handle_camera(httpd_req_t *req);

/**
 * @brief Handle POST /api/v1/led
 */
esp_err_t api_handle_led(httpd_req_t *req);

/**
 * @brief Handle GET /health
 */
esp_err_t api_handle_health(httpd_req_t *req);

/**
 * @brief Handle GET / (serve index.html)
 */
esp_err_t api_handle_index(httpd_req_t *req);

/**
 * @brief Handle GET /docs (serve Swagger UI)
 */
esp_err_t api_handle_docs(httpd_req_t *req);

#ifdef __cplusplus
}
#endif

#endif /* API_HANDLERS_H */
