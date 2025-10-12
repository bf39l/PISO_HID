#pragma once
#include "tusb.h"
#include "keymap.h"
#include <stdbool.h>

extern bool nkro_enabled;

// Initialize TinyUSB stack for HID + CDC
void USB_HID_Init(void);


// Send reports
void HID_SendKeyboard6KRO(uint8_t modifier, uint8_t keycodes[6]);
void HID_SendKeyboardNKRO(uint8_t modifier, const uint8_t nkro_bitmap[29]);
void HID_SendMouse(int8_t x, int8_t y, int8_t wheel, uint8_t buttons);

// CDC helper
int CDC_SendString(const char* str);
void CDC_Log(const char* fmt, ...);

// Callbacks
uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance);
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                           uint8_t const* buffer, uint16_t bufsize);
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                               uint8_t* buffer, uint16_t reqlen);