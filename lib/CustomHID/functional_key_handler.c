#include "keymap.h"

// ---------------------------
// Process KBD functional keys (NKRO toggle, bootloader etc..)
// ---------------------------

bool process_nkro_toggle(uint32_t kc, bool pressed)
{
    if (IS_NKRO_TOGGLE(kc))
    {
        if (pressed) return true; // only on key release
        
        nkro_enabled = !nkro_enabled;
        CDC_SendString(nkro_enabled ? "NKRO enabled\n" : "6KRO enabled\n");
        
        return true;
    }
    return false;
}

bool process_bootloader_key(uint32_t kc, bool pressed)
{
    if (IS_BOOTLOADER_KEY(kc))
    {
        if (pressed) return true; // only on key release
        
        CDC_SendString("Entering bootloader...\n");
        tud_disconnect();
        reset_usb_boot(0, 0);
        while (1);
        
        return true;
    }
    return false;
}

void handle_functional_keys(uint32_t kc, bool pressed) 
{
    if (process_nkro_toggle(kc, pressed)) return;
    if (process_bootloader_key(kc, pressed)) return;
}
