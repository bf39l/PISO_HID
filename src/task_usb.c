#include "common.h"

#define NKRO_USAGE_MIN 0x04
#define NKRO_USAGE_MAX 0xE7
#define NKRO_BITS_TOTAL (NKRO_USAGE_MAX - NKRO_USAGE_MIN + 1)
#define NKRO_BYTES_TOTAL ((NKRO_BITS_TOTAL + 7) / 8)
#define NKRO_REPORT_LEN (1 + NKRO_BYTES_TOTAL)

// Store keycode cache for each key position
typedef struct {
    bool pressed;
    uint8_t row;
    uint16_t cached_kc;  // **NEW: Cache the resolved keycode**
} KeyState;

static KeyState key_state[MATRIX_COLS];

// ---------------------------------------------------------------------------
// Build HID reports from cached keycodes
static void build_hid_reports(uint8_t *modifier_out,
                              uint8_t keycodes6[6],
                              uint8_t nkro_bitmap[NKRO_BYTES_TOTAL])
{
    *modifier_out = 0;
    memset(keycodes6, 0, 6);
    memset(nkro_bitmap, 0, NKRO_BYTES_TOTAL);
    int k6 = 0;
    
    for (int col = 0; col < MATRIX_COLS; ++col) {
        if (!key_state[col].pressed)
            continue;
        
        // **FIX: Use cached keycode instead of calling keymap_get_keycode()**
        uint16_t kc = key_state[col].cached_kc;
        
        if (kc == KC_NO || kc == KC_TRNS)
            continue;
        
        // Modifiers
        if (kc >= KC_LCTRL && kc <= KC_RGUI)
            *modifier_out |= (1 << (kc - KC_LCTRL));
        
        // 6KRO keys
        if (kc < 0xE0 && k6 < 6)
            keycodes6[k6++] = (uint8_t)kc;
        
        // NKRO bitmap
        if (kc >= NKRO_USAGE_MIN && kc <= NKRO_USAGE_MAX) {
            uint16_t bit_index = kc - NKRO_USAGE_MIN;
            nkro_bitmap[bit_index / 8] |= (1u << (bit_index % 8));
        }
        
        // Special keys
        if (kc == KC_NKRO_TOGGLE) {
            static TickType_t last_tick = 0;
            TickType_t tick = xTaskGetTickCount();
            if ((tick - last_tick) > pdMS_TO_TICKS(200)) {
                nkro_enabled = !nkro_enabled;
                last_tick = tick;
                CDC_SendString(nkro_enabled ? "NKRO enabled\n" : "6KRO enabled\n");
            }
        } else if (kc == KC_BOOTLOADER) {
            CDC_SendString("Entering bootloader...\n");
            tud_disconnect();
            reset_usb_boot(0, 0);
            while (1);
        }
    }
}

// ---------------------------------------------------------------------------
// Main USB HID task
void USB_Task(void *pvParameters)
{
    uint8_t modifier, keycodes6[6], nkro_bitmap[NKRO_BYTES_TOTAL];
    uint8_t prev6[8] = {0}, cur6[8];
    uint8_t prev_nkro[NKRO_REPORT_LEN] = {0}, cur_nkro[NKRO_REPORT_LEN];
    KeyEvent ev;
    
    for (;;) {
        tud_task();
        
        // --- Process key events from queue ---
        while (xQueueReceive(xKeyEventQueue, &ev, 0) == pdPASS) {
            key_state[ev.col].row = ev.row;
            
            // **Call keymap_get_keycode() ONCE per event**
            uint16_t kc = keymap_get_keycode(ev.row, ev.col, ev.pressed);
            
            if (ev.pressed) {
                // **Cache the keycode on press**
                key_state[ev.col].pressed = true;
                key_state[ev.col].cached_kc = kc;
            } else {
                // **Clear on release**
                key_state[ev.col].pressed = false;
                key_state[ev.col].cached_kc = KC_NO;
            }
            
            // Debug log
            char buf[64];
            snprintf(buf, sizeof(buf), "%s row=%d col=%d kc=0x%04X\n",
                     ev.pressed ? "Press" : "Release", ev.row, ev.col, kc);
            CDC_SendString(buf);
        }
        
        // --- Build HID reports ---
        build_hid_reports(&modifier, keycodes6, nkro_bitmap);
        
        // Prepare 6KRO
        cur6[0] = modifier;
        cur6[1] = 0;
        memcpy(&cur6[2], keycodes6, 6);
        
        // Prepare NKRO
        cur_nkro[0] = modifier;
        memcpy(&cur_nkro[1], nkro_bitmap, NKRO_BYTES_TOTAL);
        
        // Send reports if changed
        if (nkro_enabled) {
            if (memcmp(cur_nkro, prev_nkro, NKRO_REPORT_LEN) != 0) {
                if (tud_hid_n_ready(0))
                    tud_hid_n_report(0, 2, cur_nkro, NKRO_REPORT_LEN);
                memcpy(prev_nkro, cur_nkro, NKRO_REPORT_LEN);
            }
        } else {
            if (memcmp(cur6, prev6, sizeof(cur6)) != 0) {
                if (tud_hid_n_ready(0))
                    tud_hid_n_report(0, 1, cur6, sizeof(cur6));
                memcpy(prev6, cur6, sizeof(cur6));
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}