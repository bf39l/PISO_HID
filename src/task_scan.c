#include "common.h"

void KeyPressScan_Task(void *pvParameters)
{
    shift_register_init();
    ShiftRegister64 prev = {0}, current;

    while (1) {
        current = read_shift_registers();

        if (current.high != prev.high || current.low != prev.low) {
            xQueueSend(xShiftRegisterOutputQueue_USB,  &current, 0);
            xQueueSend(xShiftRegisterOutputQueue_OLED, &current, 0);
            prev = current;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}