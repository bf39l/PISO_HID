#include "common.h"

// Main USB HID task

// ---------------------------------------------------------------------------
// Main USB HID task
void USB_Task(void *pvParameters)
{
    uint8_t modifier, keycodes6[6], nkro_bitmap[NKRO_BYTES_TOTAL];
    uint8_t prev6[8];
    uint8_t cur6[8];
    uint8_t prev_nkro[NKRO_REPORT_LEN];
    uint8_t cur_nkro[NKRO_REPORT_LEN];
    KeyEvent ev;

    // Force first frame to send by initializing prev to different values
    memset(prev6, 0xFF, sizeof(prev6));
    memset(prev_nkro, 0xFF, sizeof(prev_nkro));
    
    for (;;) {
        tud_task();
        // run MT tick so holds fire even without events
        keymap_mt_tick();
        
        // --- Process key events from queue ---
        while (xQueueReceive(xKeyEventQueue, &ev, 0) == pdPASS) {
            // Call keymap_get_keycode to get KeyReport 
            KeyReport report = keymap_get_keycode(ev.row, ev.col, ev.pressed);
            
            // Process special keys if needed
            for (uint8_t i = 0; i < report.keycount; i++) {
                if (report.special_keys) {
                    keymap_process_special_keys(report.keycodes[i]);
                }
            }
            
            // Debug log
            char buf[128];
            uint32_t raw_kc = keymaps[keymap_get_active_layer()][ev.row][ev.col];
            snprintf(buf, sizeof(buf), "%s row=%d col=%d kc=0x%08X mods=0x%02X\n",
                     ev.pressed ? "Press" : "Release", ev.row, ev.col, raw_kc, report.modifiers);
            CDC_SendString(buf);
        }
        
        // --- Build HID reports using keymap helper ---
        keymap_build_hid_reports(&modifier, keycodes6, nkro_bitmap);
        
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
                if (tud_hid_n_ready(0)) {
                    tud_hid_n_report(0, 2, cur_nkro, NKRO_REPORT_LEN);
                    memcpy(prev_nkro, cur_nkro, NKRO_REPORT_LEN);
                    keymap_on_report_sent();
                }
            }
        } else {
            if (memcmp(cur6, prev6, sizeof(cur6)) != 0) {
                if (tud_hid_n_ready(0)) {
                    tud_hid_n_report(0, 1, cur6, sizeof(cur6));
                    memcpy(prev6, cur6, sizeof(cur6));
                    keymap_on_report_sent();
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}