#include "common.h"

#define NKRO_USAGE_MIN  0x04
#define NKRO_USAGE_MAX  0xE7
#define NKRO_BITS_TOTAL (NKRO_USAGE_MAX - NKRO_USAGE_MIN + 1)
#define NKRO_BYTES_TOTAL ((NKRO_BITS_TOTAL + 7) / 8)
#define NKRO_REPORT_LEN  (1 + NKRO_BYTES_TOTAL)  // modifier + bitmap

static inline bool sr_get_bit(const ShiftRegister64 *sr, int col) {
    return (col < 32) ? ((sr->high & (1u << (31 - col))) != 0)
                      : ((sr->low  & (1u << (63 - col))) != 0);
}

void keymap_process_shift_register(const ShiftRegister64 *sr,
                                   uint8_t *modifier_out,
                                   uint8_t keycodes6[6],
                                   uint8_t nkro_bitmap[NKRO_BYTES_TOTAL])
{
    *modifier_out = 0;
    memset(keycodes6, 0, 6);
    if (nkro_bitmap) memset(nkro_bitmap, 0, NKRO_BYTES_TOTAL);

    uint8_t active_layer = keymap_get_active_layer();
    int k = 0;

    for (int col = 0; col < 64; ++col) {
        if (!sr_get_bit(sr, col)) continue;

        uint16_t kc = keymap_get_keycode(active_layer, 0, col);
        if (kc == KC_NO || kc == KC_TRNS) continue;

        // Modifiers
        if (kc >= KC_LCTRL && kc <= KC_RGUI) {
            *modifier_out |= (1 << (kc - KC_LCTRL));
        }

        // 6KRO keys
        if (kc < 0xE0 && k < 6) keycodes6[k++] = (uint8_t)kc;

        // NKRO bitmap
        if (nkro_bitmap && kc >= NKRO_USAGE_MIN && kc <= NKRO_USAGE_MAX) {
            uint16_t bit_index = kc - NKRO_USAGE_MIN;
            nkro_bitmap[bit_index / 8] |= (1u << (bit_index % 8));
        }

        // Special keys
        if (kc == KC_NKRO_TOGGLE) {
            TickType_t tick = xTaskGetTickCount();
            static TickType_t last_toggle_tick = 0;
            if ((tick - last_toggle_tick) > pdMS_TO_TICKS(200)) {
                nkro_enabled = !nkro_enabled;
                last_toggle_tick = tick;
                CDC_SendString(nkro_enabled ? "NKRO enabled\r\n" : "6KRO enabled\r\n");
            }
        } else if (kc == KC_BOOTLOADER) {
            tud_disconnect();
            reset_usb_boot(0, 0);
            while(1);
        }
    }
}

// Test: simulate 10 keys pressed in NKRO mode
// void USB_Simulate10Keys(void)
// {
//     if (!nkro_enabled) return;

//     uint8_t report[NKRO_REPORT_LEN];
//     memset(report, 0, sizeof(report));

//     report[0] = 0; // modifier

//     // Set first 10 bits in NKRO bitmap (keys 0x04..0x0D)
//     for (int i = 0; i < 10; i++) {
//         int byte_index = i / 8;
//         int bit_pos   = i % 8;
//         report[1 + byte_index] |= (1u << bit_pos);
//     }

//     if (tud_hid_n_ready(0)) {
//         tud_hid_n_report(0, 2, report, sizeof(report));
//         CDC_SendString("Simulated 10 keys pressed (NKRO)\r\n");
//     }
// }

void USB_Task(void *pvParameters)
{
    ShiftRegister64 cur;
    uint8_t modifier;
    uint8_t keycodes[6];
    uint8_t nkro_bitmap[NKRO_BYTES_TOTAL];

    uint8_t prev6[8] = {0}, cur6[8];
    uint8_t prev_nkro[NKRO_REPORT_LEN] = {0}, cur_nkro[NKRO_REPORT_LEN];

    for (;;) {
        tud_task();

        if (tud_cdc_connected() && tud_cdc_available()) {
            char buf[64];
            uint32_t count = tud_cdc_read(buf, sizeof(buf));
            tud_cdc_write(buf, count);
            tud_cdc_write_flush();
        }

        if (xQueueReceive(xShiftRegisterOutputQueue_USB, &cur, 0) == pdPASS) {
            // Build reports
            keymap_process_shift_register(&cur, &modifier, keycodes, nkro_bitmap);

            // 6KRO payload
            cur6[0] = modifier;
            cur6[1] = 0;
            memcpy(&cur6[2], keycodes, 6);

            // NKRO payload
            cur_nkro[0] = modifier;
            memcpy(&cur_nkro[1], nkro_bitmap, NKRO_BYTES_TOTAL);

            // Send report if changed
            if (nkro_enabled) {
                if (memcmp(cur_nkro, prev_nkro, NKRO_REPORT_LEN) != 0) {
                    if (tud_hid_n_ready(0)) {
                        tud_hid_n_report(0, 2, cur_nkro, NKRO_REPORT_LEN);
                        CDC_SendString("Sent NKRO (Report ID 2)\r\n");
                    }
                    memcpy(prev_nkro, cur_nkro, NKRO_REPORT_LEN);
                }
            } else {
                if (memcmp(cur6, prev6, sizeof(cur6)) != 0) {
                    if (tud_hid_n_ready(0)) {
                        tud_hid_n_report(0, 1, cur6, sizeof(cur6));
                        // CDC_SendString("Sent 6KRO (Report ID 1)\r\n");
                    }
                    memcpy(prev6, cur6, sizeof(cur6));
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}