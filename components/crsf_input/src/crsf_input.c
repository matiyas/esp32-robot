/**
 * @file crsf_input.c
 * @brief CRSF UART receiver: RX task, snapshot publishing and failsafe timer.
 */

#include "crsf_input.h"

#include <esp_log.h>
#include <esp_timer.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <driver/uart.h>

#include <string.h>

#include "crsf_protocol.h"

static const char *TAG = "crsf_input";

#define CRSF_UART_RX_BUF      512
#define CRSF_RX_CHUNK         64
#define CRSF_ACC_SIZE         256
#define CRSF_FAILSAFE_TICK_MS 50

static struct {
    crsf_input_config_t cfg;
    SemaphoreHandle_t mutex;
    TaskHandle_t task;
    esp_timer_handle_t failsafe_timer;
    void (*failsafe_cb)(void);
    volatile bool running;
    bool installed;
    crsf_snapshot_t snap;
} s_crsf = {0};

/* Update the snapshot from a freshly decoded RC channels frame. */
static void publish_rc_frame(const crsf_frame_t *frame) {
    uint16_t channels[CRSF_NUM_CHANNELS];
    crsf_unpack_channels(frame->payload, channels);
    int64_t now = esp_timer_get_time();

    xSemaphoreTake(s_crsf.mutex, portMAX_DELAY);
    memcpy(s_crsf.snap.channels, channels, sizeof(channels));
    s_crsf.snap.last_frame_us = now;
    s_crsf.snap.connected = true;
    s_crsf.snap.failsafe = false;
    s_crsf.snap.valid_frames++;
    xSemaphoreGive(s_crsf.mutex);
}

/* Update link stats (RSSI/LQ) if present. */
static void publish_link_stats(const crsf_frame_t *frame) {
    if (frame->payload_len < 10) {
        return;
    }
    xSemaphoreTake(s_crsf.mutex, portMAX_DELAY);
    s_crsf.snap.link_rssi_dbm = (int8_t)(-(int)frame->payload[0]);
    s_crsf.snap.link_lq = frame->payload[2];
    xSemaphoreGive(s_crsf.mutex);
}

static void dispatch_frame(const crsf_frame_t *frame) {
    switch (frame->type) {
        case CRSF_FRAMETYPE_RC_CHANNELS_PACKED:
            if (frame->payload_len >= CRSF_RC_PAYLOAD_BYTES) {
                publish_rc_frame(frame);
            }
            break;
        case CRSF_FRAMETYPE_LINK_STATISTICS:
            publish_link_stats(frame);
            break;
        default:
            break;
    }
}

static void rx_task(void *arg) {
    (void)arg;
    uint8_t chunk[CRSF_RX_CHUNK];
    uint8_t acc[CRSF_ACC_SIZE];
    size_t acc_len = 0;

    while (s_crsf.running) {
        int n = uart_read_bytes(s_crsf.cfg.uart_num, chunk, sizeof(chunk), pdMS_TO_TICKS(20));
        if (n <= 0) {
            continue;
        }

        /* Append to the accumulator; on overflow drop the oldest bytes. */
        if (acc_len + (size_t)n > CRSF_ACC_SIZE) {
            size_t drop = acc_len + (size_t)n - CRSF_ACC_SIZE;
            if (drop > acc_len) {
                drop = acc_len;
            }
            memmove(acc, acc + drop, acc_len - drop);
            acc_len -= drop;
        }
        memcpy(acc + acc_len, chunk, (size_t)n);
        acc_len += (size_t)n;

        /* Parse as many complete frames as the buffer holds. */
        size_t off = 0;
        while (acc_len - off >= 2) {
            crsf_frame_t frame;
            size_t frame_len = 0;
            crsf_parse_status_t st = crsf_frame_parse(acc + off, acc_len - off, &frame, &frame_len);
            if (st == CRSF_PARSE_OK) {
                dispatch_frame(&frame);
                off += frame_len;
            } else if (st == CRSF_PARSE_INCOMPLETE) {
                break;
            } else {
                s_crsf.snap.crc_errors++;
                off += 1; /* resync */
            }
        }
        if (off > 0) {
            memmove(acc, acc + off, acc_len - off);
            acc_len -= off;
        }
    }
    vTaskDelete(NULL);
}

static void failsafe_timer_cb(void *arg) {
    (void)arg;
    crsf_snapshot_t snap = crsf_input_get_snapshot();
    int64_t now = esp_timer_get_time();
    bool live = snap.last_frame_us > 0 && !snap.failsafe &&
                (now - snap.last_frame_us) < (int64_t)s_crsf.cfg.failsafe_timeout_ms * 1000;
    if (!live && s_crsf.failsafe_cb != NULL) {
        s_crsf.failsafe_cb();
    }
}

esp_err_t crsf_input_init(const crsf_input_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    s_crsf.cfg = *config;
    if (s_crsf.mutex == NULL) {
        s_crsf.mutex = xSemaphoreCreateMutex();
        if (s_crsf.mutex == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

    uart_config_t uart_cfg = {
        .baud_rate = (int)config->baud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t ret = uart_driver_install(config->uart_num, CRSF_UART_RX_BUF, 0, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_param_config(config->uart_num, &uart_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_param_config failed: %s", esp_err_to_name(ret));
        uart_driver_delete(config->uart_num);
        return ret;
    }

    /* CRSF is true-level UART; do NOT invert. Route via the GPIO matrix. */
    ret = uart_set_pin(config->uart_num, config->tx_gpio, config->rx_gpio, UART_PIN_NO_CHANGE,
                       UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_pin failed: %s", esp_err_to_name(ret));
        uart_driver_delete(config->uart_num);
        return ret;
    }

    s_crsf.installed = true;
    ESP_LOGI(TAG, "CRSF UART%d ready (rx=%d, baud=%lu)", (int)config->uart_num,
             (int)config->rx_gpio, (unsigned long)config->baud);
    return ESP_OK;
}

esp_err_t crsf_input_start(void) {
    if (!s_crsf.installed) {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_crsf.running) {
        return ESP_OK;
    }

    s_crsf.running = true;
    BaseType_t ok =
        xTaskCreatePinnedToCore(rx_task, "crsf_rx", 4096, NULL, s_crsf.cfg.task_priority,
                                &s_crsf.task, s_crsf.cfg.task_core_id);
    if (ok != pdPASS) {
        s_crsf.running = false;
        return ESP_FAIL;
    }

    const esp_timer_create_args_t targs = {
        .callback = failsafe_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "crsf_failsafe",
    };
    esp_err_t ret = esp_timer_create(&targs, &s_crsf.failsafe_timer);
    if (ret != ESP_OK) {
        return ret;
    }
    return esp_timer_start_periodic(s_crsf.failsafe_timer, CRSF_FAILSAFE_TICK_MS * 1000);
}

crsf_snapshot_t crsf_input_get_snapshot(void) {
    crsf_snapshot_t copy;
    if (s_crsf.mutex != NULL) {
        xSemaphoreTake(s_crsf.mutex, portMAX_DELAY);
        copy = s_crsf.snap;
        xSemaphoreGive(s_crsf.mutex);
    } else {
        copy = s_crsf.snap;
    }
    return copy;
}

void crsf_input_register_failsafe_cb(void (*cb)(void)) {
    s_crsf.failsafe_cb = cb;
}

void crsf_input_deinit(void) {
    s_crsf.running = false;
    if (s_crsf.failsafe_timer != NULL) {
        esp_timer_stop(s_crsf.failsafe_timer);
        esp_timer_delete(s_crsf.failsafe_timer);
        s_crsf.failsafe_timer = NULL;
    }
    if (s_crsf.installed) {
        uart_driver_delete(s_crsf.cfg.uart_num);
        s_crsf.installed = false;
    }
}
