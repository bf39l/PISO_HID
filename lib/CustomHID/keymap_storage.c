#include "keymap.h"
#include "hardware/flash.h"
#include "pico/stdlib.h"
#include "hardware/sync.h"
#include <string.h>

// Add this include for CDC debug
#include "usb_descriptors.h"

#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE) // Last sector

static inline uint32_t keymap_flash_size(void) {
    return MAX_LAYERS * MATRIX_ROWS * MATRIX_COLS * sizeof(uint16_t);
}

void keymap_save_to_flash(void) {
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, (const uint8_t*)keymaps, keymap_flash_size());
    restore_interrupts(ints);
    CDC_SendString("Keymap saved to flash\r\n");
}

void keymap_load_from_flash(void) {
    const uint16_t* flash_keymaps = (const uint16_t*)(XIP_BASE + FLASH_TARGET_OFFSET);
    // Simple validity check: first key must be a valid keycode
    char dbg[64];
    snprintf(dbg, sizeof(dbg), "Flash first key: 0x%04X\r\n", flash_keymaps[0]);
    CDC_SendString(dbg);

    if (flash_keymaps[0] != 0xFFFF && flash_keymaps[0] != 0x0000) {
        memcpy(keymaps, flash_keymaps, keymap_flash_size());
        CDC_SendString("Keymap loaded from flash\r\n");
    } else {
        CDC_SendString("Keymap NOT loaded from flash (using default)\r\n");
    }
}
