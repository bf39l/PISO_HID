#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "keycodes.h"
#include "hardware/flash.h"
#include "pico/stdlib.h"
#include "hardware/sync.h"
#include <string.h>
#include "usb_descriptors.h"
#include <stdio.h>
#include "pico/bootrom.h"

#define MAX_LAYERS 3
#define MATRIX_ROWS 1
#define MATRIX_COLS 64
#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE) // Last sector

// NKRO definitions
#define NKRO_USAGE_MIN 0x04
#define NKRO_USAGE_MAX 0xE7
#define NKRO_BITS_TOTAL (NKRO_USAGE_MAX - NKRO_USAGE_MIN + 1)
#define NKRO_BYTES_TOTAL ((NKRO_BITS_TOTAL + 7) / 8)
#define NKRO_REPORT_LEN (1 + NKRO_BYTES_TOTAL)
#define MAX_KEYS_PER_REPORT 12  // Support up to 12 keys per press

// Key Report structure to handle multiple keycodes and modifiers
typedef struct {
    uint32_t keycodes[MAX_KEYS_PER_REPORT];  // Array of keycodes
    uint8_t keycount;                       // Number of active keycodes
    uint8_t modifiers;                      // Modifier bitmask
    bool special_keys;                      // Special keys flag (NKRO, bootloader, etc)
} KeyReport;

// External NKRO flag
extern bool nkro_enabled;

// Keymap storage
extern uint32_t keymaps[MAX_LAYERS][MATRIX_ROWS][MATRIX_COLS];

// Initialize layer manager
void layer_manager_init(void);

// Get current keycode (handles MO/TG/TO internally)
KeyReport keymap_get_keycode(uint8_t row, uint8_t col, bool pressed);

// Build HID reports from keys
void keymap_build_hid_reports(uint8_t *modifier_out, uint8_t keycodes6[6], uint8_t nkro_bitmap[NKRO_BYTES_TOTAL]);

// Process special keys (NKRO toggle, bootloader)
void keymap_process_special_keys(uint32_t kc);

// Get active layer (for debugging or other purposes)
uint8_t keymap_get_active_layer(void);

uint8_t keymap_resolve_layer(uint8_t row, uint8_t col);

// Save/load keymap to flash
void keymap_save_to_flash(void);
void keymap_load_from_flash(void);