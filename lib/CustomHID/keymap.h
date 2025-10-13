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

#define MAX_LAYERS 3
#define MATRIX_ROWS 1
#define MATRIX_COLS 64
#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE) // Last sector

// Keymap storage
extern uint16_t keymaps[MAX_LAYERS][MATRIX_ROWS][MATRIX_COLS];

// Initialize layer manager
void layer_manager_init(void);

// Get current keycode (handles MO/TG/TO internally)
uint16_t keymap_get_keycode(uint8_t row, uint8_t col, bool pressed);

// Get active layer (for debugging or other purposes)
uint8_t keymap_get_active_layer(void);

uint8_t keymap_resolve_layer(uint8_t row, uint8_t col);

// Save/load keymap to flash
void keymap_save_to_flash(void);
void keymap_load_from_flash(void);