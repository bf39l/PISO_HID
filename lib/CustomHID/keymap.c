#include "custom_hid.h"

// ---------------------------
// Layer names
// ---------------------------
const char* layer_names[MAX_LAYERS] = {
    "COLEMAK",
    "QWERTY",
    "FUNC",
    "CONFIG"
};

// ---------------------------
// Keymaps definition
// ---------------------------
uint32_t keymaps[MAX_LAYERS][MATRIX_ROWS][MATRIX_COLS] = {
    // --- Layer 0: Colmak ---
    {
        {
            KC_GRAVE,                                   KC_1, KC_2, KC_3, KC_4, KC_5, TG(3),                 TO(1),     KC_6, KC_7, KC_8,     KC_9,   KC_0,      KC_MINUS,
            KC_TAB,                                     KC_Q, KC_W, KC_F, KC_P, KC_G, KC_LBRACKET,           KC_BSLASH, KC_J, KC_L, KC_U,     KC_Y,   KC_SCOLON, KC_EQUAL,
            MT(MD_LCTRL | MD_LSHIFT | MD_LALT, KC_ESC), KC_A, KC_R, KC_S, KC_T, KC_D, KC_RBRACKET,           KC_ESCAPE, KC_H, KC_N, KC_E,     KC_I,   KC_O,      KC_QUOTE,
            KC_LSHIFT,                                  KC_Z, KC_X, KC_C, KC_V, KC_B,                                   KC_K, KC_M, KC_COMMA, KC_DOT, KC_SLASH,  KC_RSHIFT,
            KC_LCTRL,       KC_LALT,         KC_LGUI,                       KC_SPACE, KC_BSPACE,             KC_ENTER,  MT(2, KC_SPACE), KC_RGUI, KC_RALT, KC_RCTRL
        }
    },
    // --- Layer 1: QWERTY ---
    {
        {
            KC_GRAVE,                                   KC_1, KC_2, KC_3, KC_4, KC_5, TG(3),                 TO(0),     KC_6, KC_7, KC_8,     KC_9,   KC_0,      KC_MINUS,
            KC_TAB,                                     KC_Q, KC_W, KC_E, KC_R, KC_T, KC_LBRACKET,           KC_BSLASH, KC_Y, KC_U, KC_I,     KC_O,   KC_P,      KC_EQUAL,
            MT(MD_LCTRL | MD_LSHIFT | MD_LALT, KC_ESC), KC_A, KC_S, KC_D, KC_F, KC_G, KC_RBRACKET,           KC_ESCAPE, KC_H, KC_J, KC_K,     KC_L,   KC_SCOLON, KC_QUOTE,
            KC_LSHIFT,                                  KC_Z, KC_X, KC_C, KC_V, KC_B,                                   KC_N, KC_M, KC_COMMA, KC_DOT, KC_SLASH,  KC_RSHIFT,
            KC_LCTRL,       KC_LALT,         KC_LGUI,                       KC_SPACE, KC_BSPACE,             KC_ENTER,  MT(2, KC_SPACE), KC_RGUI, KC_RALT, KC_RCTRL
        }
    },
    // --- Layer 2: Function ---
    {
        {
            KC_TRNS,   KC_F1,      KC_F2,      KC_F3,      KC_F4,      KC_F5,      KC_TRNS,           KC_TRNS,   KC_F6,   KC_F7,   KC_F8,   KC_F9,    KC_F10,  KC_F11,
            KC_TRNS,   KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,           KC_PGUP,   KC_HOME, KC_TRNS, KC_UP,   KC_TRNS,  KC_TRNS, KC_F12,
            KC_CAPS,   KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,           KC_PGDOWN, KC_END,  KC_LEFT, KC_DOWN, KC_RIGHT, KC_TRNS, KC_TRNS,
            KC_TRNS,   KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,                                  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS,
            KC_TRNS,   KC_TRNS,    KC_TRNS,                            KC_TRNS,    KC_DELETE,         KC_TRNS,   KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
        } // CH(MD_LALT, KC_LEFT)
    },
    // --- Layer 3: Config ---
    {
        {
            FN_BOOT,    KC_NO, KC_NO,      KC_NO,   KC_NO,   KC_NO,   KC_NO,      KC_NO,   KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
            FN_RESET,   KC_NO, KC_NO,      KC_NO,   KC_NO,   KC_NO,   KC_NO,      KC_NO,   KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
            FN_NKRO_TG, KC_NO, KC_NO,      KC_NO,   KC_NO,   KC_NO,   KC_NO,      KC_NO,   KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
            FN_DEBUG,   KC_NO, KC_NO,      KC_NO,   KC_NO,   KC_NO,                        KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO,
            KC_NO,      KC_NO, KC_NO,                        KC_NO,   KC_NO,      KC_NO,   KC_NO, KC_NO, KC_NO, KC_NO
        }
    },
};
