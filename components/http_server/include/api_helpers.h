/**
 * @file api_helpers.h
 * @brief API response and parsing helpers
 */

#ifndef API_HELPERS_H
#define API_HELPERS_H

#include <esp_err.h>
#include <esp_http_server.h>

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum JSON body size */
#define API_MAX_BODY_SIZE 512

/**
 * @brief Send JSON success response
 *
 * Wraps body in {"success": true, ...} format.
 *
 * @param req HTTP request
 * @param json_body JSON content (without success field)
 * @return ESP_OK on success
 */
esp_err_t api_send_success(httpd_req_t *req, const char *json_body);

/**
 * @brief Send JSON error response
 *
 * @param req HTTP request
 * @param status_code HTTP status code
 * @param message Error message
 * @return ESP_OK on success
 */
esp_err_t api_send_error(httpd_req_t *req, int status_code, const char *message);

/**
 * @brief Read and parse JSON body from request
 *
 * @param req HTTP request
 * @param buffer Buffer to store body
 * @param buffer_size Size of buffer
 * @return ESP_OK on success, ESP_FAIL if body too large
 */
esp_err_t api_read_body(httpd_req_t *req, char *buffer, size_t buffer_size);

/**
 * @brief Parse direction from JSON string
 *
 * @param direction Direction string ("forward", "backward", "left", "right")
 * @param[out] result Parsed direction value
 * @return true if valid direction
 */
bool api_parse_direction(const char *direction, int *result);

/**
 * @brief Set CORS headers on response
 *
 * @param req HTTP request
 */
void api_set_cors_headers(httpd_req_t *req);

/**
 * @brief Get MIME type for file extension
 *
 * @param filename Filename with extension
 * @return MIME type string
 */
const char *api_get_mime_type(const char *filename);

#ifdef __cplusplus
}
#endif

#endif /* API_HELPERS_H */
