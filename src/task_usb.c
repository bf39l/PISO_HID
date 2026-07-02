#include "common.h"
#include "bongo_cat_bitmap.h"

// Main USB HID task

// ---------------------------------------------------------------------------
// Main USB HID task
void USB_Task(void *pvParameters)
{
    KeyEvent ev;
    uint32_t last_ver = 0;
    uint32_t last_wpm_ms = 0;
    uint32_t last_cat_ms = 0;

    for (;;) {
        tud_task();
        // run MT tick so holds fire even without events
        keymap_mt_tick();

        uint32_t now = to_ms_since_boot(get_absolute_time());

        // Tick cat animation state machine
        uint32_t cat_elapsed = now - last_cat_ms;
        if (cat_elapsed >= 1) {
            last_cat_ms = now;
            cat_state_machine_tick(cat_elapsed);
        }

        if (now - last_wpm_ms >= 1000)
        {
            last_wpm_ms = now;
            kbd_state_update(true);
        }

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