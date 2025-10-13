#include "common.h"

// Main USB HID task

// ---------------------------------------------------------------------------
// Main USB HID task
void USB_Task(void *pvParameters)
{
    keymap_init();
    KeyEvent ev;

    for (;;) {
        tud_task();
        // run MT tick so holds fire even without events
        keymap_mt_tick();
        
        // --- Process key events from queue ---
        while (xQueueReceive(xKeyEventQueue, &ev, 0) == pdPASS) {
            keymap_process_queue_item(ev.row, ev.col, ev.pressed);
        }

        keymap_send_hid_report();

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}