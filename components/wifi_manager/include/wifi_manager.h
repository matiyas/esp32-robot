/**
 * @file wifi_manager.h
 * @brief WiFi Access Point management
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
 * Must be called before starting AP.
 *
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief Start WiFi Access Point
 *
 * @param ssid AP network name
 * @param password AP password (min 8 chars, empty for open network)
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_start_ap(const char *ssid, const char *password);

/**
 * @brief Stop WiFi Access Point
 *
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_stop_ap(void);

/**
 * @brief Check if AP is active
 *
 * @return true if AP is running
 */
bool wifi_manager_is_active(void);

/**
 * @brief Get number of connected stations
 *
 * @return Number of connected clients
 */
uint8_t wifi_manager_get_station_count(void);

/**
 * @brief Get AP IP address as string
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
