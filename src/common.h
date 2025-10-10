#pragma once

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
// FreeRTOS
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
// TinyUSB
#include "bsp/board.h"
#include "tusb.h"
#include "usb_descriptors.h"
// Shift Register
#include "ShiftRegister_PISO.h"
// Keycodes
#include "keycodes.h"
#include "keymap.h"
// OLED
#include "oled.h"

extern QueueHandle_t xShiftRegisterOutputQueue_OLED;
extern QueueHandle_t xShiftRegisterOutputQueue_USB;
#define I2C_PORT i2c0
#define I2C_SDA 20
#define I2C_SCL 21