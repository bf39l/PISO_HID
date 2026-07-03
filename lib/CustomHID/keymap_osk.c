#include "custom_hid.h"
#include "os_detection.h"

// OS-aware key registry
// Each entry: [name, LINUX, WINDOWS, MACOS]
// If host OS is undetected (UNSURE), falls back to LINUX.
//
// How to use:
//   keymap.c:        OSK(OSK_FWORD)
//   custom_hid.h:    enum osk_idx_t { OSK_FWORD = 0, ... }
//   keymap_osk.c:    registry entry at index 0
//
// To add a new OSK entry:
//   1. Add name to enum osk_idx_t in custom_hid.h
//   2. Add matching entry here (order must match enum)
const osk_entry_t osk_registry[] = {
    { "FWORD",
      CH(MD_LCTRL, KC_RIGHT),
      CH(MD_LCTRL, KC_RIGHT),
      CH(MD_LALT,  KC_RIGHT) },

    { "BWORD",
      CH(MD_LCTRL, KC_LEFT),
      CH(MD_LCTRL, KC_LEFT),
      CH(MD_LALT,  KC_LEFT) },

    { "FLINE",
      KC_END,
      KC_END,
      CH(MD_LGUI,  KC_RIGHT) },

    { "BLINE",
      KC_HOME,
      KC_HOME,
      CH(MD_LGUI,  KC_LEFT) },
};

const uint8_t OSK_REGISTRY_COUNT = sizeof(osk_registry) / sizeof(osk_registry[0]);

uint32_t osk_resolve(uint8_t idx)
{
    if (idx >= OSK_REGISTRY_COUNT)
        return KC_NO;

    os_variant_t os = detected_host_os();

    switch (os) {
        case OS_WINDOWS: return osk_registry[idx].keycode_win;
        case OS_MACOS:   return osk_registry[idx].keycode_mac;
        default:         return osk_registry[idx].keycode_lnx;
    }
}
