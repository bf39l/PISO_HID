#pragma once
#include "tusb_option.h"

// ===============================================================
// TinyUSB Configuration for Raspberry Pi Pico + FreeRTOS
// Composite Device: CDC + Multiple HID Interfaces (6KRO, NKRO, Mouse)
// ===============================================================

// --------------------------------------------------------------------
// MCU & OS CONFIGURATION
// --------------------------------------------------------------------

// MCU target: RP2040 (Raspberry Pi Pico)
#define CFG_TUSB_MCU            OPT_MCU_RP2040

// OS: FreeRTOS
#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS             OPT_OS_FREERTOS
#endif

// Root Hub Port 0 configuration: Device mode, Full-speed (default for Pico)
#define CFG_TUSB_RHPORT0_MODE   (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

// Memory alignment (TinyUSB requires 4-byte alignment)
#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN      TU_ATTR_ALIGNED(4)

// --------------------------------------------------------------------
// USB DEVICE CONFIGURATION
// --------------------------------------------------------------------

// Endpoint 0 size (control transfers)
#define CFG_TUD_ENDPOINT0_SIZE  64

// --------------------------------------------------------------------
// ENABLED USB CLASSES
// --------------------------------------------------------------------

// ✅ 1x CDC (Virtual Serial Port)
// ✅ 3x HID (6KRO keyboard, NKRO keyboard, Mouse)
// 🚫 No MSC, MIDI, or Vendor classes
#define CFG_TUD_CDC             1
#define CFG_TUD_HID             2
#define CFG_TUD_MSC             0
#define CFG_TUD_MIDI            0
#define CFG_TUD_VENDOR          0

// --------------------------------------------------------------------
// CDC CONFIGURATION
// --------------------------------------------------------------------
// FIFO buffer sizes for serial communication
#define CFG_TUD_CDC_RX_BUFSIZE  256
#define CFG_TUD_CDC_TX_BUFSIZE  256

// --------------------------------------------------------------------
// HID CONFIGURATION
// --------------------------------------------------------------------
// Each HID interface can use up to 64 bytes per report
#define CFG_TUD_HID_EP_BUFSIZE  64

// --------------------------------------------------------------------
// OPTIONAL: Board RH Port Selection (not required, but for clarity)
// --------------------------------------------------------------------
#define BOARD_TUD_RHPORT        0

// ===============================================================
// END OF CONFIG
// ===============================================================