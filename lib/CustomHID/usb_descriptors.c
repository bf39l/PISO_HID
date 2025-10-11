#include "usb_descriptors.h"

// -----------------------------
// HID report structs
// -----------------------------

// 6KRO report
typedef struct {
    uint8_t modifier;
    uint8_t reserved;
    uint8_t keycode[6];
} keyboard_6kro_report_t;

// NKRO report (16 bytes bitmap)
typedef struct {
    uint8_t keys[16];
} keyboard_nkro_report_t;

// -----------------------------
// HID report descriptors
// -----------------------------

// 6KRO keyboard
uint8_t const hid_report_desc_keyboard_6kro[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

// NKRO keyboard (bitmap 16 bytes)
uint8_t const hid_report_desc_keyboard_nkro[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x80,        //   Report Count (128 bits / 16 bytes)
    0x05, 0x07,        //   Usage Page (Keyboard)
    0x19, 0x00,        //   Usage Minimum
    0x29, 0x7F,        //   Usage Maximum
    0x81, 0x02,        //   Input (Data,Var,Abs)
    0xC0               // End Collection
};

// Mouse (3 buttons + X/Y/Wheel)
uint8_t const hid_report_desc_mouse[] = {
    TUD_HID_REPORT_DESC_MOUSE()
};

// -----------------------------
// HID send helpers
// -----------------------------

void HID_SendKeyboard6KRO(uint8_t modifier, uint8_t keycodes[6]) {
    if (tud_hid_ready()) {
        keyboard_6kro_report_t report = {modifier, 0, {0}};
        for (int i = 0; i < 6; i++) report.keycode[i] = keycodes[i];
        tud_hid_keyboard_report(0, report.modifier, report.keycode);
    }
}

void HID_SendKeyboardNKRO(const uint8_t nkro_bitmap[16]) {
    if (tud_hid_ready()) {
        tud_hid_report(1, nkro_bitmap, 16);
    }
}

void HID_SendMouse(int8_t x, int8_t y, int8_t wheel, uint8_t buttons) {
    if (tud_hid_ready()) {
        tud_hid_mouse_report(2, buttons, x, y, wheel, 0);
    }
}

int CDC_SendString(const char* str) {
    if (tud_cdc_connected()) return tud_cdc_write_str(str);
    return 0;
}

// -----------------------------
// USB device descriptor
// -----------------------------

uint8_t const desc_device[] = {
    0x12,                   // bLength
    TUSB_DESC_DEVICE,       // bDescriptorType
    0x00, 0x02,             // bcdUSB 2.00
    0x00,                   // bDeviceClass (Interface-defined)
    0x00,                   // bDeviceSubClass
    0x00,                   // bDeviceProtocol
    CFG_TUD_ENDPOINT0_SIZE, // bMaxPacketSize0
    0x4C, 0x42,             // idVendor = 'BL' (little endian)
    0x53, 0x50,             // idProduct = 'PS' (little endian)
    0x01, 0x00,             // bcdDevice
    0x01,                   // iManufacturer
    0x02,                   // iProduct
    0x03,                   // iSerialNumber
    0x01                    // bNumConfigurations
};

uint8_t const* tud_descriptor_device_cb(void) {
    return desc_device;
}

// -----------------------------
// USB configuration descriptor
// -----------------------------
// Interface numbers
enum {
    ITF_NUM_CDC = 0,
    ITF_NUM_CDC_DATA,
    ITF_NUM_HID_6KRO,
    ITF_NUM_HID_NKRO,
    ITF_NUM_HID_MOUSE,
    ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + 3*TUD_HID_DESC_LEN)

uint8_t const desc_configuration[] = {
    // Configuration descriptor: 5 interfaces (CDC + 3 HID)
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

    // CDC (Virtual Serial)
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 0, 0x81, 8, 0x02, 0x82, 64),

    // HID 6KRO keyboard
    TUD_HID_DESCRIPTOR(ITF_NUM_HID_6KRO, 0, HID_ITF_PROTOCOL_KEYBOARD, sizeof(hid_report_desc_keyboard_6kro), 0x83, CFG_TUD_HID_EP_BUFSIZE, 1),
    // HID NKRO keyboard
    TUD_HID_DESCRIPTOR(ITF_NUM_HID_NKRO, 0, HID_ITF_PROTOCOL_NONE, sizeof(hid_report_desc_keyboard_nkro), 0x84, CFG_TUD_HID_EP_BUFSIZE, 1),
    // HID Mouse
    TUD_HID_DESCRIPTOR(ITF_NUM_HID_MOUSE, 0, HID_ITF_PROTOCOL_MOUSE, sizeof(hid_report_desc_mouse), 0x85, CFG_TUD_HID_EP_BUFSIZE, 1),
};

uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return desc_configuration;
}

// -----------------------------
// String descriptors
// -----------------------------

static const uint16_t string_desc_langid[] = {0x0409}; // English (US)

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void) langid;
    static uint16_t _desc_str[32];

    if (index == 0) return string_desc_langid;

    static const char* string_desc_arr[] = {
        "Mintys",         // 1: manufacturer
        "PISO",           // 2: product
        "PISO-01-051025"  // 3: serial
    };

    if (index > 3) return NULL;

    const char* str = string_desc_arr[index - 1];
    size_t len = strlen(str);

    for (size_t i = 0; i < len && i < 31; i++) {
        _desc_str[i+1] = str[i];
    }
    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2*len + 2);
    return _desc_str;
}

// -----------------------------
// HID callbacks
// -----------------------------

uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance) 
{
    switch(instance) {
        case 0: return hid_report_desc_keyboard_6kro;
        case 1: return hid_report_desc_keyboard_nkro;
        case 2: return hid_report_desc_mouse;
        default: return NULL;
    }
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                           uint8_t const* buffer, uint16_t bufsize) 
{
    char dbg[64];
    snprintf(dbg, sizeof(dbg), "tud_hid_set_report_cb: instance=%u, report_id=%u, type=%u, len=%u\r\n",
             instance, report_id, report_type, bufsize);
    CDC_SendString(dbg);
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                               uint8_t* buffer, uint16_t bufsize) 
{
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)bufsize;
    return 0;
}

// -----------------------------
// Initialization
// -----------------------------

void USB_HID_Init(void) 
{
    tusb_init();
}