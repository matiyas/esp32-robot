/**
 * @file camera_stream.h
 * @brief Camera MJPEG streaming interface
 */

#ifndef CAMERA_STREAM_H
#define CAMERA_STREAM_H

#include <esp_err.h>
#include <esp_http_server.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Camera stream configuration
 */
typedef struct {
    int frame_size;   /**< Frame size enum (FRAMESIZE_VGA, etc.) */
    int jpeg_quality; /**< JPEG quality 10-63 (lower = better) */
    int fb_count;     /**< Frame buffer count */
} camera_stream_config_t;

/**
 * @brief Initialize camera
 *
 * @param config Camera configuration
 * @return ESP_OK on success
 */
esp_err_t camera_stream_init(const camera_stream_config_t *config);

/**
 * @brief Register MJPEG stream handler
 *
 * Registers /stream endpoint for MJPEG streaming.
 *
 * @param server HTTP server handle
 * @return ESP_OK on success
 */
esp_err_t camera_stream_register_handler(httpd_handle_t server);

/**
 * @brief Get camera stream URL path
 *
 * @return Stream URL path (e.g., "/stream")
 */
const char *camera_stream_get_path(void);

/**
 * @brief Check if camera is initialized
 *
 * @return true if camera is ready
 */
bool camera_stream_is_ready(void);

/**
 * @brief Cleanup camera resources
 */
void camera_stream_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* CAMERA_STREAM_H */
