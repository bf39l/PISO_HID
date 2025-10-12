#include "common.h"

#define DEBOUNCE 3  // number of stable samples required

typedef struct {
    bool last_sample;  // last raw reading
    uint8_t count;     // consecutive same readings
    bool state;        // last stable state
} KeyDebounce;

void KeyPressScan_Task(void *pvParameters)
{
    shift_register_init();
    ShiftRegister64 current;

    KeyDebounce keys[64] = {0};

    while (1) {
        current = read_shift_registers();

        uint64_t curr_combined = ((uint64_t)current.high << 32) | current.low;

        for (int col = 0; col < 64; ++col) {
            bool raw = (curr_combined & (1ULL << (63 - col))) != 0;
            KeyDebounce *k = &keys[col];

            if (raw == k->last_sample) {
                if (k->count < DEBOUNCE) k->count++;
            } else {
                k->count = 1;
                k->last_sample = raw;
            }

            if (k->count >= DEBOUNCE && raw != k->state) {
                // stable change detected
                k->state = raw;
                KeyEvent ev = {
                    .row = 0,
                    .col = col,
                    .pressed = raw
                };
                xQueueSend(xKeyEventQueue, &ev, 0);
            }
        }

        // send snapshot to OLED (optional)
        xQueueSend(xShiftRegisterOutputQueue_OLED, &current, 0);

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}