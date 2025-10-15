#include "custom_hid.h"

// ---------------------------
// Keymaps definition
// ---------------------------
uint32_t keymaps[MAX_LAYERS][MATRIX_ROWS][MATRIX_COLS] = {
    // --- Layer 0: Base ---
    {
        {
            MT(1, KC_SPACE), MT(MD_LCTRL | MD_LSHIFT | MD_LALT, KC_1), KC_2, KC_3, KC_4, KC_5, KC_6, KC_V,
            TG(1), TO(1), KC_W, KC_E, KC_R, KC_T, KC_Y, KC_BSPACE,
            KC_CAPSLOCK, KC_DELETE, KC_S, KC_D, KC_F, KC_G, KC_H, KC_J,
            KC_LSHIFT, KC_Z, KC_X, KC_C, KC_V, KC_B, KC_N, KC_ESC,
            KC_LCTRL, KC_LGUI, KC_LALT, KC_SPACE, KC_RALT, KC_RGUI, KC_APP, KC_RCTRL,
            FN_BOOT, FN_RESET, KC_F3, KC_F4, KC_F5, KC_F6, KC_F7, KC_F8,
            MT( MD_LSHIFT | MD_LALT, KC_2), MT( MD_LALT, KC_3), KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
            FN_NKRO_TG, MO(1), KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO
        }
    },

    // --- Layer 1: Function / Media ---
    {
        {
            KC_GRAVE, KC_F1, KC_F2, KC_F3, KC_F4, KC_F5, KC_F6, KC_F7,
            KC_TRNS, TO(0), KC_UP, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
            KC_TRNS, KC_LEFT, KC_DOWN, KC_RIGHT, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
            KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
            KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
            FN_BOOT, FN_RESET, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
            KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
            MO(2), KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO
        }
    },
    // --- Layer 2: Another layer ---
    {
        {
            KC_1, KC_F1, KC_F2, KC_F3, KC_F4, KC_F5, KC_F6, KC_F7,
            KC_TRNS, TO(0), KC_UP, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
            KC_TRNS, KC_LEFT, KC_DOWN, KC_RIGHT, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
            KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
            KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
            FN_BOOT, FN_RESET, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
            KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
            KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO
        }
    },
};
