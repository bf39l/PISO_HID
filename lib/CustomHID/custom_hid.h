#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "hardware/flash.h"
#include "pico/stdlib.h"
#include "hardware/sync.h"
#include <string.h>
#include <stdio.h>
#include "pico/bootrom.h"
// TinyUSB (only need tusb.h types here)
#include "tusb.h"
#include "hardware/watchdog.h"

// --------------------
// usb_descriptors.h
// --------------------
// Initialize TinyUSB stack for HID + CDC
void USB_HID_Init(void);

// Callbacks
uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance);
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                           uint8_t const* buffer, uint16_t bufsize);
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                               uint8_t* buffer, uint16_t reqlen);

// --------------------
// HID Keyboard Usage IDs (0x04 - 0xE7)
// --------------------
#define KC_A 0x04
#define KC_B 0x05
#define KC_C 0x06
#define KC_D 0x07
#define KC_E 0x08
#define KC_F 0x09
#define KC_G 0x0A
#define KC_H 0x0B
#define KC_I 0x0C
#define KC_J 0x0D
#define KC_K 0x0E
#define KC_L 0x0F
#define KC_M 0x10
#define KC_N 0x11
#define KC_O 0x12
#define KC_P 0x13
#define KC_Q 0x14
#define KC_R 0x15
#define KC_S 0x16
#define KC_T 0x17
#define KC_U 0x18
#define KC_V 0x19
#define KC_W 0x1A
#define KC_X 0x1B
#define KC_Y 0x1C
#define KC_Z 0x1D

#define KC_1 0x1E
#define KC_2 0x1F
#define KC_3 0x20
#define KC_4 0x21
#define KC_5 0x22
#define KC_6 0x23
#define KC_7 0x24
#define KC_8 0x25
#define KC_9 0x26
#define KC_0 0x27

#define KC_ENTER  0x28
#define KC_ESCAPE 0x29
#define KC_ESC    KC_ESCAPE
#define KC_BSPACE 0x2A
#define KC_TAB    0x2B
#define KC_SPACE  0x2C
#define KC_MINUS  0x2D
#define KC_EQUAL  0x2E
#define KC_LBRACKET 0x2F
#define KC_RBRACKET 0x30
#define KC_BSLASH 0x31
#define KC_NONUS_HASH 0x32
#define KC_SCOLON 0x33
#define KC_QUOTE  0x34
#define KC_GRAVE  0x35
#define KC_COMMA  0x36
#define KC_DOT    0x37
#define KC_SLASH  0x38
#define KC_CAPSLOCK 0x39
#define KC_CAPS_LOCK KC_CAPSLOCK
#define KC_CAPS      KC_CAPSLOCK

#define KC_F1  0x3A
#define KC_F2  0x3B
#define KC_F3  0x3C
#define KC_F4  0x3D
#define KC_F5  0x3E
#define KC_F6  0x3F
#define KC_F7  0x40
#define KC_F8  0x41
#define KC_F9  0x42
#define KC_F10 0x43
#define KC_F11 0x44
#define KC_F12 0x45

#define KC_PSCREEN 0x46
#define KC_SCROLLLOCK 0x47
#define KC_PAUSE 0x48
#define KC_INSERT 0x49
#define KC_HOME 0x4A
#define KC_PGUP 0x4B
#define KC_DELETE 0x4C
#define KC_END 0x4D
#define KC_PGDOWN 0x4E
#define KC_RIGHT 0x4F
#define KC_LEFT 0x50
#define KC_DOWN 0x51
#define KC_UP 0x52

// --------------------
// Modifiers
// --------------------
#define KC_LCTRL  0xE0
#define KC_LSHIFT 0xE1
#define KC_LALT   0xE2
#define KC_LGUI   0xE3
#define KC_RCTRL  0xE4
#define KC_RSHIFT 0xE5
#define KC_RALT   0xE6
#define KC_RGUI   0xE7
#define KC_APP    0x65  // Application (Menu) key

// --------------------
// Non-HID Keycodes
// --------------------
#define KC_NO        0x00  // No action
#define KC_TRANSPARENT 0x01  // Fall through to lower layer
#define KC_TRNS KC_TRANSPARENT
#define KC_ANY       0x02  // Wildcard (used in combos/macros)

// --------------------
// QMK Layer & Function Macros
// --------------------
#define SAFE_RANGE 0x10000  // Start of non-HID keycodes (safe custom range, using upper 16 bits)

// Layer handling
#define MO(layer)      (SAFE_RANGE + 0x01000 + (layer))  // Momentary layer switch
#define TG(layer)      (SAFE_RANGE + 0x02000 + (layer))  // Toggle layer
#define TO(layer)      (SAFE_RANGE + 0x03000 + (layer))  // Jump to layer
// #define DF(layer)      (SAFE_RANGE + 0x04000 + (layer))  // Set default layer
// #define TT(layer)      (SAFE_RANGE + 0x05000 + (layer))  // Tap-Toggle layer
// #define LT(layer, kc)  (SAFE_RANGE + 0x06000 + ((layer) << 8) + (kc)) // Layer Tap

// Function control
#define RESET      (SAFE_RANGE + 0x10000)
#define DEBUG      (SAFE_RANGE + 0x10001)
#define BOOT       (SAFE_RANGE + 0x10002)
#define FN_NKRO_TG (SAFE_RANGE + 0x10003)  // Toggle NKRO mode
#define FN_RESET   RESET
#define FN_BOOT    BOOT
#define FN_DEBUG   DEBUG

// --------------------
// Utility helpers
// --------------------
#define IS_MODIFIER(code)    ((code) >= 0xE0 && (code) <= 0xE7)
#define IS_BOOTLOADER_KEY(code) ((code) == FN_BOOT)
#define IS_NKRO_TOGGLE(code) ((code) == FN_NKRO_TG)
#define IS_RESET_KEY(code) ((code) == FN_RESET)
#define IS_KBD_FUNCTIONAL_KEY(code) ((code) == FN_BOOT || (code) == FN_NKRO_TG || (code) == FN_DEBUG || (code) == FN_RESET)

// --------------------
// Simple, robust Mod-Tap encoding
// --------------------
// Modifier bitmasks for Mod-Tap (supports up to 8 modifiers)
#define MD_LCTRL  (0x01)
#define MD_LSHIFT (0x02)
#define MD_LALT   (0x04)
#define MD_LGUI   (0x08)
#define MD_RCTRL  (0x10)
#define MD_RSHIFT (0x20)
#define MD_RALT   (0x40)
#define MD_RGUI   (0x80)

// Encoding layout (32-bit):
// [ SAFE_RANGE + 0x70000 ] | [ TYPE(8) ] | [ PAYLOAD(8) ] | [ TAP_KEY(8) ]
// TYPE = 1 (mods), PAYLOAD = modifier bitmask KC_MODS_* (8 bits)
// TYPE = 2 (layer), PAYLOAD = layer index (0..255)
#define MT_TAG_BASE      (0x70000)
#define MT_TYPE_MODS     0x01
#define MT_TYPE_LAYER    0x02
#define MT_MAX_TYPE      MT_TYPE_LAYER

#define MT_PREFIX        ( (uint32_t)(SAFE_RANGE + MT_TAG_BASE) )

#define MT_ENC(type, payload, key) ( (uint32_t)(MT_PREFIX) \
                                   | ((uint32_t)((type)    & 0xFF) << 16) \
                                   | ((uint32_t)((payload) & 0xFF) << 8)  \
                                   | ((uint32_t)((key)     & 0xFF)) )

// Explicit macros (recommended)
#define MTM(modmask, key)   MT_ENC(MT_TYPE_MODS,  (modmask), (key))
#define MTL(layer, key)     MT_ENC(MT_TYPE_LAYER, (layer),   (key))

// Convenience macro to match user's examples:
// If value <= MAX_LAYERS, treat as layer. Otherwise treat as modifiers.
// Note: Using a single modifier bit equal to a small number (e.g., 1) will be
// treated as a layer by this heuristic. Prefer MTM() in that case.
#define MT(arg, key)  ( ((arg) <= (MAX_LAYERS)) ? MTL((arg), (key)) : MTM((arg), (key)) )

// Helpers to decode
#define MT_TYPE_RAW(code)   ( ((uint32_t)(code) >> 16) & 0xFF )
#define MT_PREFIX_BYTE      ( ((uint32_t)(MT_PREFIX) >> 16) & 0xFF )
#define IS_MT(code)         ( MT_TYPE_RAW(code) >= MT_PREFIX_BYTE && MT_TYPE_RAW(code) <= (MT_PREFIX_BYTE + MT_MAX_TYPE) )
#define MT_TYPE(code)       ( (uint8_t)( MT_TYPE_RAW(code) - MT_PREFIX_BYTE ) )
#define MT_PAYLOAD(code)    ( ((uint32_t)(code) >> 8)  & 0xFF )
#define MT_KEY(code)        ( ((uint32_t)(code)      ) & 0xFF )
#define MT_IS_MODS(code)    ( IS_MT(code) && MT_TYPE(code) == MT_TYPE_MODS )
#define MT_IS_LAYER(code)   ( IS_MT(code) && MT_TYPE(code) == MT_TYPE_LAYER )

// Tap timeout (ms)
#define MT_TAP_TIMEOUT_MS   200

// ==================== EXISTING KEYMAP DECLARATIONS ====================

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
// 6KRO definitions
#define SIXKRO_BYTES_TOTAL 6
#define SIXKRO_REPORT_LEN (2 + SIXKRO_BYTES_TOTAL) // <modifier><0><6 x keycode>


// Key Report structure to handle multiple keycodes and modifiers
typedef struct {
    uint32_t keycodes[MAX_KEYS_PER_REPORT]; // Array of keycodes
    uint8_t keycount;                       // Number of active keycodes
    uint8_t modifiers;                      // Modifier bitmask
    bool kbd_functional_keys;               // kbd functional keys flag (NKRO, bootloader, etc)
} KeyReport;

// External NKRO flag
extern bool nkro_enabled;

// -----------------------------
// Public keyboard state snapshot (for OLED/status)
// -----------------------------
typedef struct {
    bool nkro_enabled;       // true = NKRO, false = 6KRO
    uint8_t base_layer;      // current base layer (set by TO)
    uint8_t active_layer;    // top-most active layer (MO/TG/MT resolved)
    uint8_t stack_size;      // number of active layer entries (MO/TG/MT holds)
} KbdState;

// Return the latest keyboard state snapshot
void keymap_get_kbd_state(KbdState* out);
// Monotonic version that increments when state changes (layers/NKRO)
uint32_t keymap_get_kbd_state_version(void);

// Keymap storage
extern uint32_t keymaps[MAX_LAYERS][MATRIX_ROWS][MATRIX_COLS];

void keymap_init(void);
void keymap_process_queue_item(uint8_t row, uint8_t col, bool pressed);
void keymap_send_hid_report();

// Save/load keymap to flash
void keymap_save_to_flash(void);
void keymap_load_from_flash(void);

// MT periodic tick to process hold detection while key is held
void keymap_mt_tick(void);
