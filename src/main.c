#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include "ShiftRegister_PISO.h"
#include "oled.h"
#include "bsp/board.h"
#include "tusb.h"
#include "usb_descriptors.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 20
#define I2C_SCL 21

QueueHandle_t xShiftRegisterOutputQueue_OLED;
QueueHandle_t xShiftRegisterOutputQueue_USB;

void ShiftRegister64To6KRO(ShiftRegister64 sr, uint8_t *modifier, uint8_t keycodes[6]) {
    *modifier = 0;  // initialize modifiers
    for (int i = 0; i < 6; i++) keycodes[i] = 0;

    int k = 0; // index for keycodes
    for (int i = 0; i < 64; i++) {
        bool pressed;
        if (i < 32) pressed = (sr.low >> i) & 1;
        else        pressed = (sr.high >> (i-32)) & 1;

        if (!pressed) continue;

        uint8_t keycode = i + 4; // example: map SR bit 0 → HID keycode 4 ('a')
        if (keycode >= 0xE0 && keycode <= 0xE7) {
            *modifier |= (1 << (keycode - 0xE0));
        } else if (k < 6) {
            keycodes[k++] = keycode;
        }
    }
}

void USB_Task(void *pvParameters) {
    (void) pvParameters;
    ShiftRegister64 recv;
    uint8_t modifier;
    uint8_t keycodes[6];

    while (1) {
        // TinyUSB device task
        tud_task();  // must be called regularly

        // Example: read from CDC and echo back
        if (tud_cdc_connected() && tud_cdc_available()) {
            char buf[64];
            uint32_t count = tud_cdc_read(buf, sizeof(buf));
            tud_cdc_write(buf, count);
            tud_cdc_write_flush();
        }

        // Process key press from shift register
        if (xQueueReceive(xShiftRegisterOutputQueue_USB, &recv, 0) == pdPASS) {
            ShiftRegister64To6KRO(recv, &modifier, keycodes);
            HID_SendKeyboard6KRO(modifier, keycodes);
        }


        // Optional: delay to yield to other tasks
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void KeyPressScan_Task(void *pvParameters) 
{
    shift_register_init();

    ShiftRegister64 prev = {0, 0};  // previous scan result
    ShiftRegister64 current;

    while(1) {
        current = read_shift_registers();

        // Compare 64 bits — print only when something changed
        if (current.high != prev.high || current.low != prev.low) {
            xQueueSend(xShiftRegisterOutputQueue_USB, &current, 0);
            xQueueSend(xShiftRegisterOutputQueue_OLED, &current, 0);
            prev = current;
        }
    }
}

void draw_shift_bits_oled(ShiftRegister64 bits, const ASCIIFont *font) 
{
    char lines[NUM_SHIFT_REGISTERS][MAX_BITS + 1];
    shift_bits_to_strings(bits, lines);

    for (int i = 0; i < NUM_SHIFT_REGISTERS; i++) {
        char label[16];
        label[0] = 'S';
        label[1] = 'R';
        label[2] = '0' + (i + 1);  // SR index
        label[3] = ':';
        for (int j = 0; j < MAX_BITS; j++) {
            label[4 + j] = lines[i][j];
        }
        label[4 + MAX_BITS] = '\0';
        OLED_PrintASCIIString(0, i * font->h, label, font, OLED_COLOR_NORMAL);
    }
    OLED_PrintASCIIString(14 * font->w+1, 28, "PISO", &afont8x6, OLED_COLOR_NORMAL);
}

void OLED_Task(void *pvParameters)
{
    OLED_Init(i2c0, 2);
    ShiftRegister64 recv;
    // Small splash screen
    OLED_NewFrame();
    OLED_DrawEllipse(128/2-1, 64/2, 32, 18, OLED_COLOR_NORMAL);
    OLED_DrawEllipse(128/2-1, 64/2, 16, 30, OLED_COLOR_NORMAL);
    OLED_PrintASCIIString(128/2 - 4 * afont24x12.w / 2, 64/2 - afont24x12.h/2, "PISO", &afont24x12, OLED_COLOR_NORMAL);
    OLED_PrintASCIIString(128-5*afont8x6.w, 64 - afont8x6.h, "bf39L", &afont8x6, OLED_COLOR_NORMAL);
    OLED_ShowFrame();

    while (1) {
        if (xQueueReceive(xShiftRegisterOutputQueue_OLED, &recv, portMAX_DELAY)) {
            OLED_NewFrame();
            draw_shift_bits_oled(recv, &afont8x6);
            OLED_ShowFrame();
        }
    }
}

int main()
{
    stdio_init_all();

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c
    // TinyUSB
    board_init();
    USB_HID_Init();   // start TinyUSB stack

    xShiftRegisterOutputQueue_OLED = xQueueCreate(5, sizeof(ShiftRegister64)); // queue can hold 5 events
    xShiftRegisterOutputQueue_USB = xQueueCreate(5, sizeof(ShiftRegister64)); // queue can hold 5 events

    uint8_t keycodes[6] = {0};
    HID_SendKeyboard6KRO(0, keycodes);
    // create tasks
    xTaskCreate(KeyPressScan_Task, "KeyPressScan", 256, NULL, 3, NULL); // higher priority
    xTaskCreate(USB_Task, "USB_Task", 256, NULL, 2, NULL);
    xTaskCreate(OLED_Task, "OLED_Task", 256, NULL, 1, NULL); // low priority

    vTaskStartScheduler();

    while (true) tight_loop_contents();
}


void vApplicationMallocFailedHook(void)
{
    // This runs if pvPortMalloc() fails
    printf("ERROR: FreeRTOS malloc failed!\n");
    portDISABLE_INTERRUPTS();
    for(;;); // Halt
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    // This runs if Stack overflow
    printf("ERROR: Stack overflow!\n");
    portDISABLE_INTERRUPTS();
    for(;;); // Halt
}
