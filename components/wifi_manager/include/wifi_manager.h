/**
 * @file wifi_manager.h
 * @brief WiFi connection management
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <esp_err.h>

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize WiFi subsystem
 *
 * Must be called before connect.
 *
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief Connect to WiFi network
 *
 * @param ssid Network SSID
 * @param password Network password (empty for open networks)
 * @return ESP_OK on connection start
 */
esp_err_t wifi_manager_connect(const char *ssid, const char *password);

/**
 * @brief Disconnect from WiFi network
 *
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_disconnect(void);

/**
 * @brief Wait for WiFi connection
 *
 * Blocks until connected or timeout.
 *
 * @param timeout_ms Timeout in milliseconds (portMAX_DELAY for infinite)
 * @return true if connected, false on timeout
 */
bool wifi_manager_wait_connected(uint32_t timeout_ms);

/**
 * @brief Check if WiFi is connected
 *
 * @return true if connected
 */
bool wifi_manager_is_connected(void);

/**
 * @brief Get current IP address as string
 *
 * @param buffer Buffer to store IP string
 * @param buffer_size Size of buffer
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_get_ip(char *buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_MANAGER_H */
