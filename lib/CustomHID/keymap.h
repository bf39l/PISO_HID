#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "keycodes.h"

// How many layers you want
#define MAX_LAYERS 2

// Each layer: 1x64 matrix → up to 64 keys
#define MATRIX_ROWS 1
#define MATRIX_COLS 64

// Keymap access
extern uint16_t keymaps[MAX_LAYERS][MATRIX_ROWS][MATRIX_COLS];

// Function to resolve active layer
uint16_t keymap_get_keycode(uint8_t layer, uint8_t row, uint8_t col);

// Optional: process QMK special keys (MO, TG, etc.)
void keymap_process_layer_keys(uint8_t row, uint8_t col, uint16_t keycode, bool pressed);

// Layer state getter/setter
uint8_t keymap_get_active_layer(void);
void keymap_set_layer(uint8_t layer);
void keymap_toggle_layer(uint8_t layer);
void keymap_momentary_layer(uint8_t layer, bool active);

void keymap_save_to_flash(void);
void keymap_load_from_flash(void);