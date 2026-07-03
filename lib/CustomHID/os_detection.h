#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    OS_UNSURE,
    OS_LINUX,
    OS_WINDOWS,
    OS_MACOS,
} os_variant_t;

// Feed a wLength value from USB GET_DESCRIPTOR setup packet
void process_wlength(const uint16_t w_length);

// Return currently detected OS
os_variant_t detected_host_os(void);

// Reset detection state (call on USB disconnect/reconnect)
void erase_wlength_data(void);

// Return human-readable OS string (3 chars max)
const char* os_variant_string(os_variant_t os);
