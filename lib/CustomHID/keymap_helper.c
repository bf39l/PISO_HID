#include "keymap.h"

// ---------------------------
// Layer Stack System (unified for MO and TG)
// ---------------------------
#define MAX_LAYER_STACK 10

typedef enum {
    LAYER_TYPE_MO,  // Momentary - deactivates on key release
    LAYER_TYPE_TG   // Toggle - stays until toggled off
} LayerType;

typedef struct {
    uint8_t activated_layer;
    uint8_t source_layer;
    uint8_t row, col;
    LayerType type;  // MO or TG
} StackEntry;

typedef struct {
    StackEntry stack[MAX_LAYER_STACK];
    uint8_t size;
    uint8_t base_layer;
} LayerState;

// Key state tracking for the keyboard
typedef struct {
    bool pressed;
    uint8_t row;
    uint32_t cached_kc;  // Cache the resolved keycode
} KeyState;

static LayerState layer_state = {0};
static bool to_pressed[MATRIX_ROWS][MATRIX_COLS] = {{0}};
static bool layer_key_pressed[MATRIX_ROWS][MATRIX_COLS] = {{0}};  // Unified tracking
static KeyState key_state[MATRIX_COLS];  // Track key state across the keyboard

// NKRO mode flag is defined in usb_descriptors.c
// extern bool nkro_enabled;

// ---------------------------
// Initialize
// ---------------------------
void layer_manager_init(void)
{
    memset(&layer_state, 0, sizeof(layer_state));
    memset(to_pressed, 0, sizeof(to_pressed));
    memset(layer_key_pressed, 0, sizeof(layer_key_pressed));
    memset(key_state, 0, sizeof(key_state));
    
    // Default to NKRO mode
    nkro_enabled = true;
}

// ---------------------------
// Get current active layer
// ---------------------------
uint8_t keymap_get_active_layer(void)
{
    if (layer_state.size > 0)
        return layer_state.stack[layer_state.size - 1].activated_layer;
    return layer_state.base_layer;
}

// ---------------------------
// Find layer key in stack
// ---------------------------
static int find_in_stack(uint8_t row, uint8_t col)
{
    for (int i = 0; i < layer_state.size; i++) {
        if (layer_state.stack[i].row == row && layer_state.stack[i].col == col)
            return i;
    }
    return -1;
}

// ---------------------------
// Debug: Print layer stack
// ---------------------------
static void print_layer_stack(void)
{
    char buf[128];
    int pos = snprintf(buf, sizeof(buf), "Stack [%d]: [", layer_state.size);
    for (int i = 0; i < layer_state.size && pos < 110; i++) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "%d%s%s", 
                       layer_state.stack[i].activated_layer,
                       layer_state.stack[i].type == LAYER_TYPE_TG ? "T" : "M",
                       (i < layer_state.size - 1) ? ", " : "");
    }
    snprintf(buf + pos, sizeof(buf) - pos, "]\n");
    CDC_SendString(buf);
}

// ---------------------------
// Resolve active layer for key
// ---------------------------
uint8_t keymap_resolve_layer(uint8_t row, uint8_t col)
{
    // Don't apply layers to their own keys
    if (find_in_stack(row, col) >= 0)
        return layer_state.base_layer;
    
    // Return top of stack (MO or TG)
    return keymap_get_active_layer();
}

// ---------------------------
// Helper: Check if keycode is a layer switch key
// ---------------------------
static bool is_layer_switch_key(uint32_t kc)
{
    return (kc >= MO(0) && kc < MO(MAX_LAYERS)) ||
           (kc >= TO(0) && kc < TO(MAX_LAYERS)) ||
           (kc >= TG(0) && kc < TG(MAX_LAYERS));
}

// ---------------------------
// Process special keys (NKRO toggle, bootloader)
// ---------------------------
void keymap_process_special_keys(uint32_t kc)
{
    if (IS_NKRO_TOGGLE(kc)) {
        static uint32_t last_time = 0;
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        if ((current_time - last_time) > 200) {
            nkro_enabled = !nkro_enabled;
            last_time = current_time;
            CDC_SendString(nkro_enabled ? "NKRO enabled\n" : "6KRO enabled\n");
        }
    }
    else if (IS_BOOTLOADER_KEY(kc)) {
        CDC_SendString("Entering bootloader...\n");
        tud_disconnect();
        reset_usb_boot(0, 0);
        while (1);
    }
}

// ---------------------------
// Get keycode with unified layer stack
// ---------------------------
KeyReport keymap_get_keycode(uint8_t row, uint8_t col, bool pressed)
{
    KeyReport report = {0};
    
    if (row >= MATRIX_ROWS || col >= MATRIX_COLS)
        return report;

    int stack_idx = find_in_stack(row, col);
    uint32_t kc = KC_NO;
    uint8_t check_layer;

    if (!pressed && stack_idx >= 0) {
        // Layer key release - use SOURCE layer
        check_layer = layer_state.stack[stack_idx].source_layer;
        kc = keymaps[check_layer][row][col];
    } else {
        // Check if there's a TG layer active in the stack
        bool has_tg = false;
        for (int i = 0; i < layer_state.size; i++) {
            if (layer_state.stack[i].type == LAYER_TYPE_TG) {
                has_tg = true;
                break;
            }
        }
        
        if (has_tg) {
            // TG is active - ONLY check the current active layer
            check_layer = keymap_resolve_layer(row, col);
            kc = keymaps[check_layer][row][col];
        } else {
            // No TG - check base layer first, then active layer
            kc = keymaps[layer_state.base_layer][row][col];
            
            if (!is_layer_switch_key(kc)) {
                check_layer = keymap_resolve_layer(row, col);
                kc = keymaps[check_layer][row][col];
            }
        }
    }

    // --- TO ---
    if (kc >= TO(0) && kc < TO(MAX_LAYERS)) {
        if (pressed && !to_pressed[row][col]) {
            to_pressed[row][col] = true;
        }
        if (!pressed && to_pressed[row][col]) {
            to_pressed[row][col] = false;
            layer_state.base_layer = kc - TO(0);
            layer_state.size = 0;  // Clear entire stack
            CDC_SendString("TO\n");
            print_layer_stack();
        }
        return report; // Empty report
    }

    // --- MO: Activates on press, deactivates on release ---
    else if (kc >= MO(0) && kc < MO(MAX_LAYERS)) {
        uint8_t target = kc - MO(0);
        
        if (pressed && stack_idx < 0) {
            if (layer_state.size < MAX_LAYER_STACK) {
                layer_state.stack[layer_state.size].activated_layer = target;
                layer_state.stack[layer_state.size].source_layer = keymap_get_active_layer();
                layer_state.stack[layer_state.size].row = row;
                layer_state.stack[layer_state.size].col = col;
                layer_state.stack[layer_state.size].type = LAYER_TYPE_MO;
                layer_state.size++;
                
                char buf[64];
                snprintf(buf, sizeof(buf), "MO(%d) ", target);
                CDC_SendString(buf);
                print_layer_stack();
            }
        }
        if (!pressed && stack_idx >= 0) {
            // MO deactivates on release
            uint8_t released = layer_state.stack[stack_idx].activated_layer;
            
            for (int i = stack_idx; i < layer_state.size - 1; i++) {
                layer_state.stack[i] = layer_state.stack[i + 1];
            }
            layer_state.size--;
            
            char buf[64];
            snprintf(buf, sizeof(buf), "~MO(%d) ", released);
            CDC_SendString(buf);
            print_layer_stack();
        }
        return report; // Empty report
    }

    // --- TG: Toggles on/off on release ---
    else if (kc >= TG(0) && kc < TG(MAX_LAYERS)) {
        uint8_t target = kc - TG(0);
        
        if (pressed && !layer_key_pressed[row][col]) {
            layer_key_pressed[row][col] = true;
        }
        if (!pressed && layer_key_pressed[row][col]) {
            layer_key_pressed[row][col] = false;
            
            if (stack_idx < 0) {
                // Not in stack - add it (toggle ON)
                if (layer_state.size < MAX_LAYER_STACK) {
                    layer_state.stack[layer_state.size].activated_layer = target;
                    layer_state.stack[layer_state.size].source_layer = keymap_get_active_layer();
                    layer_state.stack[layer_state.size].row = row;
                    layer_state.stack[layer_state.size].col = col;
                    layer_state.stack[layer_state.size].type = LAYER_TYPE_TG;
                    layer_state.size++;
                    
                    char buf[64];
                    snprintf(buf, sizeof(buf), "TG(%d) ON ", target);
                    CDC_SendString(buf);
                    print_layer_stack();
                }
            } else {
                // Already in stack - remove it (toggle OFF)
                uint8_t released = layer_state.stack[stack_idx].activated_layer;
                
                for (int i = stack_idx; i < layer_state.size - 1; i++) {
                    layer_state.stack[i] = layer_state.stack[i + 1];
                }
                layer_state.size--;
                
                char buf[64];
                snprintf(buf, sizeof(buf), "TG(%d) OFF ", released);
                CDC_SendString(buf);
                print_layer_stack();
            }
        }
        return report; // Empty report
    }

    // Check if it's a special key that needs handling
    if (IS_NKRO_TOGGLE(kc) || IS_BOOTLOADER_KEY(kc)) {
        report.special_keys = true;
        report.keycodes[0] = kc;
        report.keycount = 1;
        return report;
    }

    // Handle normal key or modifier
    if (kc != KC_NO && kc != KC_TRNS) {
        // Store in key_state for the HID report building
        key_state[col].pressed = pressed;
        key_state[col].row = row;
        key_state[col].cached_kc = kc;
        
        // Check if it's a modifier key
        if (IS_MODIFIER(kc)) {
            report.modifiers |= (1 << (kc - KC_LCTRL));
        } 
        // Otherwise add as normal key
        else {
            report.keycodes[0] = kc;
            report.keycount = 1;
        }
    }

    return report;
}

// ---------------------------
// Build HID reports from cached keycodes
// ---------------------------
void keymap_build_hid_reports(uint8_t *modifier_out, uint8_t keycodes6[6], uint8_t nkro_bitmap[NKRO_BYTES_TOTAL])
{
    *modifier_out = 0;
    memset(keycodes6, 0, 6);
    memset(nkro_bitmap, 0, NKRO_BYTES_TOTAL);
    int k6 = 0;
    
    for (int col = 0; col < MATRIX_COLS; ++col) {
        if (!key_state[col].pressed)
            continue;
        
        // Use cached keycode
        uint32_t kc = key_state[col].cached_kc;
        
        if (kc == KC_NO || kc == KC_TRNS)
            continue;
        
        // Modifiers
        if (kc >= KC_LCTRL && kc <= KC_RGUI) {
            *modifier_out |= (1 << (kc - KC_LCTRL));
        }
        
        // 6KRO keys
        if (kc < 0xE0 && k6 < 6) {
            keycodes6[k6++] = (uint8_t)kc;
        }
        
        // NKRO bitmap
        if (kc >= NKRO_USAGE_MIN && kc <= NKRO_USAGE_MAX) {
            uint16_t bit_index = kc - NKRO_USAGE_MIN;
            nkro_bitmap[bit_index / 8] |= (1u << (bit_index % 8));
        }
        
        // Process any special keys (NKRO toggle, bootloader)
        if (IS_NKRO_TOGGLE(kc) || IS_BOOTLOADER_KEY(kc)) {
            keymap_process_special_keys(kc);
        }
    }
}

uint32_t keymap_get_sticky_keycode(uint8_t col) { return 0; }
uint8_t keymap_get_sticky_layer(uint8_t col) { return 0; }