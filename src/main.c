#include "common.h"

// Queues visible to all tasks
QueueHandle_t xShiftRegisterOutputQueue_OLED;
// QueueHandle_t xShiftRegisterOutputQueue_USB;
QueueHandle_t xKeyEventQueue;

void KeyPressScan_Task(void *pvParameters);
void USB_Task(void *pvParameters);
void OLED_Task(void *pvParameters);

int main(void)
{
    stdio_init_all();
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    board_init();
    USB_HID_Init();

    xShiftRegisterOutputQueue_OLED = xQueueCreate(256, sizeof(ShiftRegister64));
    // xShiftRegisterOutputQueue_USB  = xQueueCreate(10, sizeof(ShiftRegister64));
    xKeyEventQueue = xQueueCreate(256, sizeof(KeyEvent));

    xTaskCreate(KeyPressScan_Task, "Scan", 256, NULL, 3, NULL);
    xTaskCreate(USB_Task, "USB", 256, NULL, 2, NULL);
    xTaskCreate(OLED_Task, "OLED", 256, NULL, 1, NULL);

    vTaskStartScheduler();
    while (1);
}

void vApplicationMallocFailedHook(void) {
    printf("ERROR: malloc failed!\n");
    portDISABLE_INTERRUPTS();
    for(;;);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    printf("ERROR: Stack overflow in %s!\n", pcTaskName);
    portDISABLE_INTERRUPTS();
    for(;;);
}
