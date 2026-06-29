/**
 * @file crsf_input.h
 * @brief CRSF (ExpressLRS) UART receiver with thread-safe channel snapshot
 *
 * Owns a hardware UART, runs a dedicated RX task that parses CRSF frames, and
 * publishes a whole-struct snapshot of the RC channels and link state. Also
 * owns an independent failsafe timer that triggers a registered stop callback
 * when frames stop arriving, regardless of any consumer task state.
 */

#ifndef CRSF_INPUT_H
#define CRSF_INPUT_H

#include <esp_err.h>

#include <driver/gpio.h>
#include <driver/uart.h>

#include <stdbool.h>
#include <stdint.h>

#include "crsf_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Thread-safe snapshot of the RC link.
 */
typedef struct {
    uint16_t channels[CRSF_NUM_CHANNELS]; /**< Raw channel ticks (172..1811). */
    bool connected;                       /**< At least one valid frame seen. */
    bool failsafe;                        /**< RX-signalled failsafe. */
    int64_t last_frame_us;                /**< esp_timer time of last RC frame. */
    uint8_t link_lq;                      /**< Uplink link quality (0-100), if known. */
    int8_t link_rssi_dbm;                 /**< Uplink RSSI in dBm, if known. */
    uint32_t valid_frames;                /**< Count of accepted RC frames. */
    uint32_t crc_errors;                  /**< Count of rejected frames. */
} crsf_snapshot_t;

/**
 * @brief CRSF receiver configuration.
 */
typedef struct {
    uart_port_t uart_num;         /**< UART peripheral (UART_NUM_1 or _2). */
    gpio_num_t rx_gpio;           /**< RX pin (routed via the GPIO matrix). */
    gpio_num_t tx_gpio;           /**< TX pin, or -1 if unused. */
    uint32_t baud;                /**< Baud rate (typically 420000). */
    uint32_t failsafe_timeout_ms; /**< No-frame window before failsafe fires. */
    int task_priority;            /**< RX task priority. */
    int task_core_id;             /**< Core to pin the RX task to. */
} crsf_input_config_t;

/**
 * @brief Configure and install the CRSF UART (does not start the RX task).
 */
esp_err_t crsf_input_init(const crsf_input_config_t *config);

/**
 * @brief Start the RX task and the failsafe timer.
 */
esp_err_t crsf_input_start(void);

/**
 * @brief Get a consistent copy of the current link snapshot.
 */
crsf_snapshot_t crsf_input_get_snapshot(void);

/**
 * @brief Register the callback invoked by the failsafe timer on link loss.
 *
 * The callback runs in the esp_timer service task; keep it short and
 * non-blocking (e.g. stop the motor and center the servo).
 */
void crsf_input_register_failsafe_cb(void (*cb)(void));

/**
 * @brief Stop the RX task/timer and uninstall the UART.
 */
void crsf_input_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* CRSF_INPUT_H */
