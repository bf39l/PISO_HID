#include "keymap.h"

// ---------------------------
// Layer state handling
// ---------------------------
static uint8_t active_layer = 0;
static bool layer_toggle_state[MAX_LAYERS] = {false};

uint8_t keymap_get_active_layer(void) {
    return active_layer;
}

void keymap_set_layer(uint8_t layer) {
    if (layer < MAX_LAYERS) {
        active_layer = layer;
    }
}

void keymap_toggle_layer(uint8_t layer) {
    if (layer >= MAX_LAYERS) return;
    layer_toggle_state[layer] = !layer_toggle_state[layer];
    if (layer_toggle_state[layer]) active_layer = layer;
    else active_layer = 0;
}

void keymap_momentary_layer(uint8_t layer, bool active) {
    if (layer >= MAX_LAYERS) return;
    if (active) active_layer = layer;
    else active_layer = 0;
}

// ---------------------------
// Lookup + layer processing
// ---------------------------
uint16_t keymap_get_keycode(uint8_t layer, uint8_t row, uint8_t col) {
    if (layer >= MAX_LAYERS || row >= MATRIX_ROWS || col >= MATRIX_COLS) return KC_NO;
    uint16_t kc = keymaps[layer][row][col];
    if (kc == KC_TRNS && layer > 0) {
        return keymap_get_keycode(layer - 1, row, col);
    }
    return kc;
}

void keymap_process_layer_keys(uint8_t row, uint8_t col, uint16_t keycode, bool pressed) {
    if (!IS_QMK_KEYCODE(keycode)) return;

    uint16_t base = keycode - SAFE_RANGE;
    uint8_t layer = (base & 0xFF);

    if (keycode >= MO(0) && keycode < MO(MAX_LAYERS)) {
        keymap_momentary_layer(layer, pressed);
    } else if (keycode >= TG(0) && keycode < TG(MAX_LAYERS)) {
        if (pressed) keymap_toggle_layer(layer);
    } else if (keycode >= TO(0) && keycode < TO(MAX_LAYERS)) {
        if (pressed) keymap_set_layer(layer);
    }
}