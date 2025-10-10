#include "common.h"

static void draw_shift_bits_oled(ShiftRegister64 bits, const ASCIIFont *font) {
    char lines[NUM_SHIFT_REGISTERS][MAX_BITS + 1];
    shift_bits_to_strings(bits, lines);

    for (int i = 0; i < NUM_SHIFT_REGISTERS; i++) {
        char label[16];
        snprintf(label, sizeof(label), "SR%d:%s", i + 1, lines[i]);
        OLED_PrintASCIIString(0, i * font->h, label, font, OLED_COLOR_NORMAL);
    }
    OLED_PrintASCIIString(14 * font->w+1, 28, "PISO", &afont8x6, OLED_COLOR_NORMAL);
}

void OLED_Task(void *pvParameters)
{
    OLED_Init(i2c0, 2);
    OLED_NewFrame();
    OLED_DrawEllipse(128/2-1, 64/2, 32, 18, OLED_COLOR_NORMAL);
    OLED_DrawEllipse(128/2-1, 64/2, 16, 30, OLED_COLOR_NORMAL);
    OLED_PrintASCIIString(128/2 - 4 * afont24x12.w / 2, 64/2 - afont24x12.h/2, "PISO", &afont24x12, OLED_COLOR_NORMAL);
    OLED_PrintASCIIString(128-5*afont8x6.w, 64 - afont8x6.h, "bf39L", &afont8x6, OLED_COLOR_NORMAL);
    OLED_ShowFrame();

    ShiftRegister64 recv;
    while (1) {
        if (xQueueReceive(xShiftRegisterOutputQueue_OLED, &recv, portMAX_DELAY)) {
            OLED_NewFrame();
            draw_shift_bits_oled(recv, &afont8x6);
            OLED_ShowFrame();
        }
    }
}