#include "keymap.h"
#include <string.h>
#include <stdio.h>

// ---------------------------
// Layer State
// ---------------------------
typedef struct {
    uint8_t base_layer;       // TO(x)
    uint8_t mo_layer;         // MO(x)
    bool mo_active;
    uint8_t tg_layer;         // TG(x)
    bool tg_active;
    uint8_t tg_row, tg_col;   // TG key axis
    uint8_t tg_prev_layer;    // previous layer before TG
} LayerState;

static LayerState layer_state = {0};

// ---------------------------
// Edge trigger tracking
// ---------------------------
static bool mo_pressed[MATRIX_ROWS][MATRIX_COLS] = {{0}};
static bool tg_pressed[MATRIX_ROWS][MATRIX_COLS] = {{0}};
static bool to_pressed[MATRIX_ROWS][MATRIX_COLS] = {{0}};

// ---------------------------
// Sticky keycodes for USB task
// ---------------------------
static uint16_t sticky_keycode[MATRIX_COLS] = {0};
static uint8_t sticky_layer[MATRIX_COLS] = {0};

// ---------------------------
// Initialize
// ---------------------------
void layer_manager_init(void)
{
    memset(&layer_state, 0, sizeof(layer_state));
    memset(sticky_keycode, 0, sizeof(sticky_keycode));
    memset(sticky_layer, 0, sizeof(sticky_layer));
    memset(mo_pressed, 0, sizeof(mo_pressed));
    memset(tg_pressed, 0, sizeof(tg_pressed));
    memset(to_pressed, 0, sizeof(to_pressed));
}

// ---------------------------
// Resolve active layer for key
// ---------------------------
static uint8_t resolve_layer(uint8_t row, uint8_t col)
{
    // TG layer first
    if (layer_state.tg_active && !(row == layer_state.tg_row && col == layer_state.tg_col))
        return layer_state.tg_layer;

    // MO layer second
    if (layer_state.mo_active && !(mo_pressed[row][col]))
        return layer_state.mo_layer;

    // base layer
    return layer_state.base_layer;
}

// ---------------------------
// Get keycode for USB task
// ---------------------------
uint16_t keymap_get_keycode(uint8_t row, uint8_t col, bool pressed)
{
    if (row >= MATRIX_ROWS || col >= MATRIX_COLS)
        return KC_NO;

    uint16_t kc = keymaps[layer_state.base_layer][row][col];

    // --- TO ---
    if (kc >= TO(0) && kc < TO(MAX_LAYERS)) {
        if (pressed && !to_pressed[row][col]) {
            // layer_state.base_layer = kc - TO(0);
            // layer_state.mo_active = false;
            // layer_state.tg_active = false;
            CDC_SendString("TO down\n");
            to_pressed[row][col] = true;
        }
        if (!pressed && to_pressed[row][col]) {
            to_pressed[row][col] = false;
            layer_state.base_layer = kc - TO(0);
            layer_state.mo_active = false;
            layer_state.tg_active = false;
            CDC_SendString("TO pressed, base_layer updated\n");
        }
        kc = KC_NO;
    }

    // // --- MO ---
    // else if (kc >= MO(0) && kc < MO(MAX_LAYERS)) {
    //     if (pressed && !mo_pressed[row][col]) {
    //         layer_state.mo_layer = kc - MO(0);
    //         layer_state.mo_active = true;
    //         mo_pressed[row][col] = true;
    //         CDC_SendString("MO pressed, temp layer active\n");
    //     }
    //     if (!pressed) {
    //         mo_pressed[row][col] = false;
    //         layer_state.mo_active = false;
    //     }
    //     kc = KC_NO;
    // }

    // // --- TG ---
    // else if (kc >= TG(0) && kc < TG(MAX_LAYERS)) {
    //     if (pressed && !tg_pressed[row][col]) {
    //         if (!layer_state.tg_active) {
    //             // activate TG layer
    //             layer_state.tg_prev_layer = layer_state.base_layer;
    //             layer_state.tg_layer = kc - TG(0);
    //             layer_state.tg_active = true;
    //             layer_state.tg_row = row;
    //             layer_state.tg_col = col;
    //             CDC_SendString("TG pressed, layer toggled on\n");
    //         } else if (layer_state.tg_active &&
    //                    row == layer_state.tg_row &&
    //                    col == layer_state.tg_col) {
    //             // deactivate TG
    //             layer_state.tg_active = false;
    //             layer_state.base_layer = layer_state.tg_prev_layer;
    //             CDC_SendString("TG pressed again, layer toggled off\n");
    //         }
    //         tg_pressed[row][col] = true;
    //     }
    //     if (!pressed)
    //         tg_pressed[row][col] = false;
    //     kc = KC_NO;
    // }

    // --- Resolve normal keycode
    uint8_t layer = resolve_layer(row, col);
    if (kc == KC_NO || kc == KC_TRNS)
        kc = keymaps[layer][row][col];

    // // --- Update sticky key info ---
    // if (pressed) {
    //     sticky_layer[col] = layer;
    //     sticky_keycode[col] = kc;
    // } else {
    //     sticky_layer[col] = 0;
    //     sticky_keycode[col] = KC_NO;
    // }

    return kc;
}

// ---------------------------
// Sticky helpers
// ---------------------------
uint16_t keymap_get_sticky_keycode(uint8_t col) { return sticky_keycode[col]; }
uint8_t keymap_get_sticky_layer(uint8_t col) { return sticky_layer[col]; }