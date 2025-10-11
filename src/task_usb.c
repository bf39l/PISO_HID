#include "common.h"

/**
 * Convert 64-bit shift register state into HID 6KRO report.
 */
void keymap_process_shift_register(const ShiftRegister64 *sr, uint8_t *modifier, uint8_t keycodes[6]) {
    *modifier = 0;
    memset(keycodes, 0, 6);

    uint8_t active_layer = keymap_get_active_layer();
    int k = 0;

    for (int col = 0; col < 64; col++) {
        bool pressed;
        if (col < 32)
            pressed = (sr->high & (1u << (31 - col))) != 0;
        else
            pressed = (sr->low & (1u << (63 - col))) != 0;

        if (!pressed) continue;

        uint16_t keycode = keymap_get_keycode(active_layer, 0, col);
        if (keycode == KC_NO || keycode == KC_TRNS)
            continue;

        // Handle QMK-style layer switching keys
        keymap_process_layer_keys(0, col, keycode, true);

        // Handle standard modifiers
        if (keycode >= KC_LCTRL && keycode <= KC_RGUI) {
            *modifier |= (1 << (keycode - KC_LCTRL));
        }
        else if (keycode < 0xE0 && k < 6) {  // regular keys
            keycodes[k++] = (uint8_t)keycode;
        }
    }
}


void USB_Task(void *pvParameters)
{
    static bool keymap_loaded = false;
    ShiftRegister64 recv;
    uint8_t modifier, keycodes[6];
    keymap_load_from_flash();

    for (;;) {
        tud_task();  // keep TinyUSB alive

        if (tud_cdc_connected() && tud_cdc_available()) {
            char buf[64];
            uint32_t count = tud_cdc_read(buf, sizeof(buf));
            tud_cdc_write(buf, count);
            tud_cdc_write_flush();
        }

        if (xQueueReceive(xShiftRegisterOutputQueue_USB, &recv, 0) == pdPASS) {
            keymap_process_shift_register(&recv, &modifier, keycodes);
            HID_SendKeyboard6KRO(modifier, keycodes);
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}