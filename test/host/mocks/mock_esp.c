/**
 * @file mock_esp.c
 * @brief Mock implementations for host testing
 */

#include "esp_err.h"
#include <stdint.h>

/* Mock safety_validate_duration - standalone implementation for testing */
uint32_t safety_validate_duration(uint32_t duration_ms, uint32_t max_duration_ms)
{
    if (duration_ms == 0) {
        return 0;
    }

    if (duration_ms > max_duration_ms) {
        return max_duration_ms;
    }

    return duration_ms;
}
