/**
 * @file wifi_manager.c
 * @brief WiFi Access Point management implementation
 */

#include "wifi_manager.h"

#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_wifi.h>

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

#include <string.h>

static const char *TAG = "wifi_manager";

/* AP network configuration */
#define AP_IP_ADDR "10.42.0.1"
#define AP_GATEWAY "10.42.0.1"
#define AP_NETMASK "255.255.255.0"

/* WiFi state */
static struct {
    bool initialized;
    bool active;
    esp_netif_t *netif;
    uint8_t station_count;
} s_wifi = {0};

/**
 * @brief WiFi event handler
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
                               void *event_data) {
    (void)arg;

    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_AP_START:
                s_wifi.active = true;
                ESP_LOGI(TAG, "AP started");
                break;

            case WIFI_EVENT_AP_STOP:
                s_wifi.active = false;
                s_wifi.station_count = 0;
                ESP_LOGI(TAG, "AP stopped");
                break;

            case WIFI_EVENT_AP_STACONNECTED:
                s_wifi.station_count++;
                ESP_LOGI(TAG, "Station connected (total: %d)", s_wifi.station_count);
                break;

            case WIFI_EVENT_AP_STADISCONNECTED:
                if (s_wifi.station_count > 0) {
                    s_wifi.station_count--;
                }
                ESP_LOGI(TAG, "Station disconnected (total: %d)", s_wifi.station_count);
                break;

            default:
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_AP_STAIPASSIGNED) {
        ip_event_ap_staipassigned_t *event = (ip_event_ap_staipassigned_t *)event_data;
        ESP_LOGI(TAG, "Station assigned IP: " IPSTR, IP2STR(&event->ip));
    }
}

esp_err_t wifi_manager_init(void) {
    if (s_wifi.initialized) {
        return ESP_OK;
    }

    /* Initialize network interface */
    esp_err_t ret = esp_netif_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "netif init failed");
        return ret;
    }

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "event loop create failed");
        return ret;
    }

    s_wifi.netif = esp_netif_create_default_wifi_ap();
    if (s_wifi.netif == NULL) {
        ESP_LOGE(TAG, "netif create failed");
        return ESP_FAIL;
    }

    /* Configure custom IP address */
    esp_netif_dhcps_stop(s_wifi.netif);

    esp_netif_ip_info_t ip_info = {0};
    ip_info.ip.addr = esp_ip4addr_aton(AP_IP_ADDR);
    ip_info.gw.addr = esp_ip4addr_aton(AP_GATEWAY);
    ip_info.netmask.addr = esp_ip4addr_aton(AP_NETMASK);

    ret = esp_netif_set_ip_info(s_wifi.netif, &ip_info);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set IP info");
        return ret;
    }

    esp_netif_dhcps_start(s_wifi.netif);

    /* Initialize WiFi */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi init failed");
        return ret;
    }

    /* Register event handlers */
    ret = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler,
                                              NULL, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register WiFi event handler");
        return ret;
    }

    ret = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED,
                                              &wifi_event_handler, NULL, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register IP event handler");
        return ret;
    }

    s_wifi.initialized = true;
    ESP_LOGI(TAG, "WiFi manager initialized");

    return ESP_OK;
}

esp_err_t wifi_manager_start_ap(const char *ssid, const char *password) {
    if (!s_wifi.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (ssid == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    wifi_config_t wifi_config = {
        .ap =
            {
                .max_connection = 4,
                .authmode = WIFI_AUTH_OPEN,
            },
    };

    strncpy((char *)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid) - 1);
    wifi_config.ap.ssid_len = strlen(ssid);

    if (password != NULL && strlen(password) >= 8) {
        strncpy((char *)wifi_config.ap.password, password, sizeof(wifi_config.ap.password) - 1);
        wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    } else if (password != NULL && strlen(password) > 0) {
        ESP_LOGW(TAG, "Password too short (min 8 chars), creating open network");
    }

    esp_err_t ret = esp_wifi_set_mode(WIFI_MODE_AP);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi mode");
        return ret;
    }

    ret = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi config");
        return ret;
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi");
        return ret;
    }

    /* Disable WiFi power saving for consistent streaming performance */
    esp_wifi_set_ps(WIFI_PS_NONE);

    ESP_LOGI(TAG, "AP started - SSID: %s, Password: %s, IP: %s", ssid,
             (wifi_config.ap.authmode == WIFI_AUTH_OPEN) ? "(open)" : "***", AP_IP_ADDR);

    return ESP_OK;
}

esp_err_t wifi_manager_stop_ap(void) {
    if (!s_wifi.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = esp_wifi_stop();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop WiFi");
        return ret;
    }

    return ESP_OK;
}

bool wifi_manager_is_active(void) {
    return s_wifi.active;
}

uint8_t wifi_manager_get_station_count(void) {
    return s_wifi.station_count;
}

esp_err_t wifi_manager_get_ip(char *buffer, size_t buffer_size) {
    if (buffer == NULL || buffer_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_wifi.active) {
        return ESP_ERR_INVALID_STATE;
    }

    strncpy(buffer, AP_IP_ADDR, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';

    return ESP_OK;
}
