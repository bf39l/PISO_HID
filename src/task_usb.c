#include "common.h"

// Main USB HID task

// ---------------------------------------------------------------------------
// Main USB HID task
void USB_Task(void *pvParameters)
{
    KeyEvent ev;
    uint32_t last_ver = 0;

    for (;;) {
        tud_task();
        // run MT tick so holds fire even without events
        keymap_mt_tick();
        
        // --- Process key events from queue ---
        while (xQueueReceive(xKeyEventQueue, &ev, 0) == pdPASS) {
            keymap_process_queue_item(ev.row, ev.col, ev.pressed);
        }

        // Publish KbdState changes to OLED (decoupled)
        uint32_t ver = keymap_get_kbd_state_version();
        if (ver != last_ver && xKbdStateQueue) {
            last_ver = ver;
            KbdState s;
            keymap_get_kbd_state(&s);
            if (xQueueSend(xKbdStateQueue, &s, 0) != pdPASS) {
                (void)xQueueReceive(xKbdStateQueue, &s, 0);
                (void)xQueueSend(xKbdStateQueue, &s, 0);
            }
        }

        keymap_send_hid_report();

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}