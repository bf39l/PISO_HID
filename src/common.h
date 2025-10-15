#pragma once

#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
// FreeRTOS
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
// Shift Register
#include "ShiftRegister_PISO.h"
// Custom HID
#include "custom_hid.h"
// OLED
#include "oled.h"

#define I2C_PORT i2c0
#define I2C_SDA 20
#define I2C_SCL 21

typedef struct {
    uint8_t row;      // optional, always 0 for now if 1-row keyboard
    uint8_t col;      // which column (0–63)
    bool pressed;     // true = pressed, false = released
} KeyEvent;

extern QueueHandle_t xShiftRegisterOutputQueue_OLED;
// extern QueueHandle_t xShiftRegisterOutputQueue_USB;
extern QueueHandle_t xKeyEventQueue;
// Queue to publish KbdState updates to OLED task
extern QueueHandle_t xKbdStateQueue;