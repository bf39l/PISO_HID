#include "custom_hid.h"
#include "os_detection.h"

// TinyUSB BSP forward declaration (avoid pulling board.h into public headers)
void board_init(void);

// ---------------------------
// Layer Stack System (unified for MO and TG)
// ---------------------------
#define MAX_LAYER_STACK 10

typedef enum
{
    LAYER_TYPE_MO, // Momentary - deactivates on key release
    LAYER_TYPE_TG  // Toggle - stays until toggled off
} LayerType;

typedef struct
{
    uint8_t activated_layer;
    uint8_t source_layer;
    uint8_t row, col;
    LayerType type; // MO or TG
} StackEntry;

typedef struct
{
    StackEntry stack[MAX_LAYER_STACK];
    uint8_t size;
    uint8_t base_layer;
} LayerState;

// Key state tracking for the keyboard
typedef struct
{
    bool pressed;
    uint8_t row;
    uint32_t cached_kc; // Cache the resolved keycode
} KeyState;

static LayerState layer_state = {0};
static bool to_pressed[MATRIX_ROWS][MATRIX_COLS] = {{0}};
static bool layer_key_pressed[MATRIX_ROWS][MATRIX_COLS] = {{0}}; // Unified tracking
static KeyState key_state[MATRIX_COLS];                          // Track key state across the keyboard
static bool oneshot_tap[MATRIX_COLS];
static uint8_t oneshot_mods[MATRIX_COLS]; // One-shot modifiers for CH keys

// HID report buffers
uint8_t modifier, keycodes6[SIXKRO_BYTES_TOTAL], nkro_bitmap[NKRO_BYTES_TOTAL];
uint8_t prev6[SIXKRO_REPORT_LEN];
uint8_t cur6[SIXKRO_REPORT_LEN];
uint8_t prev_nkro[NKRO_REPORT_LEN];
uint8_t cur_nkro[NKRO_REPORT_LEN];

// functional_key_handler.c
void handle_functional_keys(uint32_t keycode, bool pressed);
// usb_descriptors.c
int CDC_SendString(const char* str);
bool HID_SendKeyboard6KRO(uint8_t modifier, uint8_t keycodes[6]);
bool HID_SendKeyboardNKRO(uint8_t modifier, const uint8_t nkro_bitmap[NKRO_BYTES_TOTAL]);
bool HID_SendMouse(int8_t x, int8_t y, int8_t wheel, uint8_t buttons);

// -----------------------------
// Initialization
// -----------------------------

void USB_HID_Init(void) 
{
    // Initialize TinyUSB BSP (USB clocks, pins)
    board_init();
    tusb_init();
    keymap_init();
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

// Internal keyboard state snapshot and change counter
static KbdState g_kbd_state = {0};
static uint32_t g_kbd_state_ver = 0;

void kbd_state_update(bool force)
{
    os_variant_t os = detected_host_os();
    kbd_os_variant_t kbd_os;
    switch (os) {
        case OS_WINDOWS: kbd_os = KBD_OS_WINDOWS; break;
        case OS_MACOS:   kbd_os = KBD_OS_MACOS; break;
        case OS_LINUX:   kbd_os = KBD_OS_LINUX; break;
        default:         kbd_os = KBD_OS_UNSURE; break;
    }

    KbdState s = {
        .nkro_enabled = nkro_enabled,
        .debug_mode   = debug_mode,
        .base_layer   = layer_state.base_layer,
        .active_layer = keymap_get_active_layer(),
        .stack_size   = layer_state.size,
        .wpm          = wpm_get(),
        .host_os      = kbd_os,
    };
    if (force || memcmp(&s, &g_kbd_state, sizeof(KbdState)) != 0) {
        g_kbd_state = s;
        g_kbd_state_ver++;
    }
}

void keymap_get_kbd_state(KbdState* out)
{
    if (!out) return;
    // Ensure we return a consistent snapshot
    kbd_state_update(false);
    *out = g_kbd_state;
}

uint32_t keymap_get_kbd_state_version(void)
{
    kbd_state_update(false);
    return g_kbd_state_ver;
}

void keymap_init(void)
{
    wpm_init();
    // Initialize HID report buffers
    // Force first frame to send by initializing prev to different values
    memset(prev6, 0xFF, sizeof(prev6));
    memset(prev_nkro, 0xFF, sizeof(prev_nkro));

    memset(&layer_state, 0, sizeof(layer_state));
    memset(to_pressed, 0, sizeof(to_pressed));
    memset(layer_key_pressed, 0, sizeof(layer_key_pressed));
    memset(key_state, 0, sizeof(key_state));
    memset(oneshot_tap, 0, sizeof(oneshot_tap));
    memset(oneshot_mods, 0, sizeof(oneshot_mods));

    // Default to 6KRO mode
    nkro_enabled = false;
    debug_mode = false;
    kbd_state_update(true);
}

// ---------------------------
// Find layer key in stack
// ---------------------------
static int find_in_stack(uint8_t row, uint8_t col)
{
    for (int i = 0; i < layer_state.size; i++)
    {
        if (layer_state.stack[i].row == row && layer_state.stack[i].col == col)
            return i;
    }
    return -1;
}

// ---------------------------
// Debug: Print layer stack (very safe, chunked)
// ---------------------------
static void print_layer_stack(void)
{
    char hdr[32];
    int w = snprintf(hdr, sizeof(hdr), "Stack [%u]: [", (unsigned)layer_state.size);
    if (w < 0)
        return;
    hdr[sizeof(hdr) - 1] = '\0';
    CDC_SendString(hdr);

    for (int i = 0; i < layer_state.size; i++)
    {
        char ent[16];
        const char *type = (layer_state.stack[i].type == LAYER_TYPE_TG) ? "T" : "M";
        const char *sep = (i < layer_state.size - 1) ? ", " : "";
        w = snprintf(ent, sizeof(ent), "%u%s%s", (unsigned)layer_state.stack[i].activated_layer, type, sep);
        if (w < 0)
            break;
        ent[sizeof(ent) - 1] = '\0';
        CDC_SendString(ent);
    }

    CDC_SendString("]\n");
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
// Minimal Mod-Tap state (per pressed key position)
// ---------------------------
// Minimal Mod-Tap state (per pressed key position)
// New behavior:
// 1. Press → Another key pressed: immediately activate hold action + other key
// 2. Press → Release within timeout: send tap key
// 3. Press → Hold past timeout (no other key): activate hold action
// ---------------------------
typedef struct
{
    bool active;
    uint8_t row, col;
    uint32_t mt_code;
    uint32_t t_down_ms;
    bool hold_activated;  // True if hold action has been activated
    bool tap_released;    // True if this MT key was released
} MTSlot;

#define MT_MAX_SLOTS 8
static MTSlot mt_slots[MT_MAX_SLOTS];
static bool mt_other_key_pressed = false; // Flag: another key pressed while MT active

static void mt_reset_all(void) { memset(mt_slots, 0, sizeof(mt_slots)); }

static int mt_find(uint8_t row, uint8_t col)
{
    for (int i = 0; i < MT_MAX_SLOTS; i++)
        if (mt_slots[i].active && mt_slots[i].row == row && mt_slots[i].col == col)
            return i;
    return -1;
}

static int mt_alloc(uint8_t row, uint8_t col, uint32_t mt)
{
    for (int i = 0; i < MT_MAX_SLOTS; i++)
        if (!mt_slots[i].active)
        {
            mt_slots[i].active = true;
            mt_slots[i].row = row;
            mt_slots[i].col = col;
            mt_slots[i].mt_code = mt;
            mt_slots[i].t_down_ms = to_ms_since_boot(get_absolute_time());
            mt_slots[i].hold_activated = false;
            mt_slots[i].tap_released = false;
            return i;
        }
    return -1;
}

static void mt_free_idx(int idx)
{
    if (idx >= 0 && idx < MT_MAX_SLOTS)
    {
        memset(&mt_slots[idx], 0, sizeof(MTSlot));
    }
}

// Check if any MT key is currently held (not released)
static bool mt_any_held(void)
{
    for (int i = 0; i < MT_MAX_SLOTS; i++)
        if (mt_slots[i].active && !mt_slots[i].tap_released)
            return true;
    return false;
}

// Activate hold action for an MT key
static void mt_activate_hold(int idx)
{
    if (idx < 0 || idx >= MT_MAX_SLOTS || !mt_slots[idx].active)
        return;
    
    if (mt_slots[idx].hold_activated)
        return; // Already activated
    
    uint32_t mt = mt_slots[idx].mt_code;
    uint8_t mt_type = MT_TYPE(mt);
    
    if (mt_type == MT_TYPE_MODS)
    {
        uint8_t m = MT_PAYLOAD(mt);
        // Cache combined modifiers using special marker 0xF000|bits
        key_state[mt_slots[idx].col].pressed = true;
        key_state[mt_slots[idx].col].row = mt_slots[idx].row;
        key_state[mt_slots[idx].col].cached_kc = 0xF000 | m; // marker + bits
    }
    else if (mt_type == MT_TYPE_LAYER)
    {
        uint8_t layer = MT_PAYLOAD(mt);
        // Momentary layer on hold
        if (layer_state.size < MAX_LAYER_STACK)
        {
            layer_state.stack[layer_state.size].activated_layer = layer;
            layer_state.stack[layer_state.size].source_layer = keymap_get_active_layer();
            layer_state.stack[layer_state.size].row = mt_slots[idx].row;
            layer_state.stack[layer_state.size].col = mt_slots[idx].col;
            layer_state.stack[layer_state.size].type = LAYER_TYPE_MO;
            layer_state.size++;
            // Log MT layer activation
            char buf[32];
            snprintf(buf, sizeof(buf), "MTL(%u) ON\n", (unsigned)layer);
            CDC_SendString(buf);
            // Mark state changed
            kbd_state_update(true);
        }
    }
    
    mt_slots[idx].hold_activated = true;
}

// Deactivate hold action for an MT key
static void mt_deactivate_hold(int idx)
{
    if (idx < 0 || idx >= MT_MAX_SLOTS || !mt_slots[idx].active)
        return;
    
    if (!mt_slots[idx].hold_activated)
        return; // Not activated yet
    
    uint32_t mt = mt_slots[idx].mt_code;
    uint8_t mt_type = MT_TYPE(mt);
    
    if (mt_type == MT_TYPE_MODS)
    {
        // Clear combined modifier cache
        key_state[mt_slots[idx].col].pressed = false;
        key_state[mt_slots[idx].col].cached_kc = KC_NO;
    }
    else if (mt_type == MT_TYPE_LAYER)
    {
        // Find and remove this layer from stack
        int si = find_in_stack(mt_slots[idx].row, mt_slots[idx].col);
        if (si >= 0)
        {
            uint8_t released = layer_state.stack[si].activated_layer;
            for (int i = si; i < layer_state.size - 1; i++)
                layer_state.stack[i] = layer_state.stack[i + 1];
            layer_state.size--;
            // Log MT layer deactivation
            char buf[32];
            snprintf(buf, sizeof(buf), "MTL(%u) OFF\n", (unsigned)released);
            CDC_SendString(buf);
            kbd_state_update(true);
        }
    }
    
    mt_slots[idx].hold_activated = false;
}

static void mt_tick_timeout(void)
{
    uint32_t now = to_ms_since_boot(get_absolute_time());
    
    // Check for timeout on held MT keys
    for (int i = 0; i < MT_MAX_SLOTS; i++)
    {
        if (!mt_slots[i].active || mt_slots[i].hold_activated || mt_slots[i].tap_released)
            continue;
        
        uint32_t mt = mt_slots[i].mt_code;
        if (!IS_MT(mt))
        {
            mt_free_idx(i);
            continue;
        }
        
        uint32_t dt = now - mt_slots[i].t_down_ms;
        
        // ✅ FIX: Always activate hold at timeout, regardless of mt_other_key_pressed
        // mt_other_key_pressed is for the COMBO path (already activated early)
        // Timeout path should activate whenever timeout passes
        if (dt > MT_TAP_TIMEOUT_MS)
        {
            mt_activate_hold(i);
        }
    }
}

// ---------------------------
// Get keycode with unified layer stack
// ---------------------------
KeyReport keymap_get_keycode(uint8_t row, uint8_t col, bool pressed)
{
    KeyReport report = (KeyReport){0};
    if (row >= MATRIX_ROWS || col >= MATRIX_COLS)
        return report;

    // ✅ CRITICAL: Check if non-MT key pressed while MT held BEFORE resolving keycode
    // This ensures layer is activated BEFORE we resolve the keycode
    if (pressed && mt_any_held())
    {
        // Peek at the keycode that would be resolved to check if it's NOT an MT key
        int temp_stack_idx = find_in_stack(row, col);
        uint32_t temp_kc = KC_NO;
        
        // Resolve what the keycode would be
        if (temp_stack_idx >= 0)
        {
            temp_kc = keymaps[layer_state.stack[temp_stack_idx].source_layer][row][col];
        }
        else
        {
            bool has_tg = false;
            for (int i = 0; i < layer_state.size; i++)
            {
                if (layer_state.stack[i].type == LAYER_TYPE_TG)
                {
                    has_tg = true;
                    break;
                }
            }
            uint8_t peek_layer;
            if (has_tg)
            {
                peek_layer = keymap_resolve_layer(row, col);
            }
            else
            {
                peek_layer = layer_state.base_layer;
                if (!is_layer_switch_key(keymaps[peek_layer][row][col]))
                {
                    peek_layer = keymap_resolve_layer(row, col);
                }
            }
            temp_kc = keymaps[peek_layer][row][col];
        }
        
        // If it's NOT an MT key, activate MT hold immediately
        if (!IS_MT(temp_kc))
        {
            mt_other_key_pressed = true;
            for (int i = 0; i < MT_MAX_SLOTS; i++)
            {
                if (mt_slots[i].active && !mt_slots[i].hold_activated && !mt_slots[i].tap_released)
                {
                    mt_activate_hold(i);
                }
            }
        }
    }

    // Resolve raw kc with existing layer logic
    int stack_idx = find_in_stack(row, col);
    uint32_t kc = KC_NO;
    uint8_t check_layer;
    if (!pressed && stack_idx >= 0)
    {
        check_layer = layer_state.stack[stack_idx].source_layer;
        kc = keymaps[check_layer][row][col];
    }
    else
    {
        bool has_tg = false;
        for (int i = 0; i < layer_state.size; i++)
        {
            if (layer_state.stack[i].type == LAYER_TYPE_TG)
            {
                has_tg = true;
                break;
            }
        }
        if (has_tg)
        {
            check_layer = keymap_resolve_layer(row, col);
            kc = keymaps[check_layer][row][col];
        }
        else
        {
            kc = keymaps[layer_state.base_layer][row][col];
            if (!is_layer_switch_key(kc))
            {
                check_layer = keymap_resolve_layer(row, col);
                kc = keymaps[check_layer][row][col];
            }
        }
    }

    // Handle TO/MO/TG first (existing)
    // --- TO ---
    if (kc >= TO(0) && kc < TO(MAX_LAYERS))
    {
        if (pressed && !to_pressed[row][col])
        {
            to_pressed[row][col] = true;
        }
        if (!pressed && to_pressed[row][col])
        {
            to_pressed[row][col] = false;
            uint8_t target = (uint8_t)(kc - TO(0));
            layer_state.base_layer = target;
            layer_state.size = 0; // Clear entire stack
            char buf[32];
            snprintf(buf, sizeof(buf), "TO(%u)\n", (unsigned)target);
            CDC_SendString(buf);
            print_layer_stack();
            kbd_state_update(true);
        }
        return report; // Empty report
    }

    // --- MO: Activates on press, deactivates on release ---
    else if (kc >= MO(0) && kc < MO(MAX_LAYERS))
    {
        uint8_t target = kc - MO(0);

        if (pressed && stack_idx < 0)
        {
            if (layer_state.size < MAX_LAYER_STACK)
            {
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
                kbd_state_update(true);
            }
        }
        if (!pressed && stack_idx >= 0)
        {
            // MO deactivates on release
            uint8_t released = layer_state.stack[stack_idx].activated_layer;

            for (int i = stack_idx; i < layer_state.size - 1; i++)
            {
                layer_state.stack[i] = layer_state.stack[i + 1];
            }
            layer_state.size--;

            char buf[64];
            snprintf(buf, sizeof(buf), "~MO(%d) ", released);
            CDC_SendString(buf);
            print_layer_stack();
            kbd_state_update(true);
        }
        return report; // Empty report
    }

    // --- TG: Toggles on/off on release ---
    else if (kc >= TG(0) && kc < TG(MAX_LAYERS))
    {
        uint8_t target = kc - TG(0);

        if (pressed && !layer_key_pressed[row][col])
        {
            layer_key_pressed[row][col] = true;
        }
        if (!pressed && layer_key_pressed[row][col])
        {
            layer_key_pressed[row][col] = false;

            if (stack_idx < 0)
            {
                // Not in stack - add it (toggle ON)
                if (layer_state.size < MAX_LAYER_STACK)
                {
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
                    kbd_state_update(true);
                }
            }
            else
            {
                // Already in stack - remove it (toggle OFF)
                uint8_t released = layer_state.stack[stack_idx].activated_layer;

                for (int i = stack_idx; i < layer_state.size - 1; i++)
                {
                    layer_state.stack[i] = layer_state.stack[i + 1];
                }
                layer_state.size--;

                char buf[64];
                snprintf(buf, sizeof(buf), "TG(%d) OFF ", released);
                CDC_SendString(buf);
                print_layer_stack();
                kbd_state_update(true);
            }
        }
        return report; // Empty report
    }

    // Mod-Tap (new behavior):
    // 1. Press → Another key pressed: immediately activate hold + other key
    // 2. Press → Release within timeout: send tap key
    // 3. Press → Hold past timeout (no other key): activate hold
    if (IS_MT(kc))
    {
        if (pressed)
        {
            // Check if any MT key is already held
            if (mt_any_held())
            {
                // Another key pressed while MT is held, activate hold immediately
                mt_other_key_pressed = true;
                // Find and activate the MT key's hold action
                for (int i = 0; i < MT_MAX_SLOTS; i++)
                {
                    if (mt_slots[i].active && !mt_slots[i].hold_activated && !mt_slots[i].tap_released)
                    {
                        mt_activate_hold(i);
                    }
                }
            }
            
            // Allocate new MT slot if not already present
            if (mt_find(row, col) < 0)
            {
                mt_alloc(row, col, kc);
            }
        }
        else
        {
            // Release of MT key
            int idx = mt_find(row, col);
            if (idx >= 0)
            {
                mt_slots[idx].tap_released = true;
                
                if (!mt_slots[idx].hold_activated)
                {
                    // Hold was never activated: send tap key
                    uint8_t tap = MT_KEY(mt_slots[idx].mt_code);
                    key_state[col].pressed = false;
                    key_state[col].cached_kc = tap;
                    oneshot_tap[col] = true;
                    if (IS_MODIFIER(tap))
                        report.modifiers |= (1u << (tap - KC_LCTRL));
                    else
                    {
                        report.keycodes[0] = tap;
                        report.keycount = 1;
                    }
                }
                else
                {
                    // Hold was activated: deactivate it
                    mt_deactivate_hold(idx);
                }
                
                mt_free_idx(idx);
                
                // Reset the other-key-pressed flag if no MT keys are held
                if (!mt_any_held())
                {
                    mt_other_key_pressed = false;
                }
            }
        }
        // run timeout tick each event
        mt_tick_timeout();
        return report; // don't emit base kc now; report built via cache
    }

    // KBD Functional keys
    if (IS_KBD_FUNCTIONAL_KEY(kc))
    {
        report.kbd_functional_keys = true;
        report.keycodes[0] = kc;
        report.keycount = 1;
        handle_functional_keys(kc, pressed);

        return report;
    }

    // Handle OS-aware key: resolve to OS-specific keycode
    if (IS_OSK(kc))
    {
        uint8_t osk_idx = OSK_IDX(kc);
        kc = osk_resolve(osk_idx);

        const char* osk_name = (osk_idx < OSK_REGISTRY_COUNT) ? osk_registry[osk_idx].name : "????";
        char osk_buf[64];
        snprintf(osk_buf, sizeof(osk_buf), "OSK(%u)[%s]->0x%08X\n", (unsigned)osk_idx, osk_name, (unsigned)kc);
        CDC_SendString(osk_buf);
    }

    // Handle KC_TRNS: fall through to base layer
    if (kc == KC_TRNS)
    {
        // Look up the keycode from the base layer
        uint32_t base_kc = keymaps[layer_state.base_layer][row][col];

        // Recursively resolve in case base layer also has KC_TRNS
        // (though that would be unusual for the base layer)
        if (base_kc != KC_TRNS && base_kc != KC_NO)
        {
            kc = base_kc;
        }
        else
        {
            kc = KC_NO;
        }
    }

    // Normal key path (cache for report builder)
    if (kc != KC_NO)
    {
        key_state[col].pressed = pressed;
        key_state[col].row = row;
        key_state[col].cached_kc = kc;
        if (pressed && kc >= KC_A && kc <= KC_0)
            wpm_record_keystroke();
        if (IS_MODIFIER(kc))
            report.modifiers |= (1u << (kc - KC_LCTRL));
        else
        {
            report.keycodes[0] = kc;
            report.keycount = 1;
        }
    }
    return report;
}

void keymap_process_queue_item(uint8_t row, uint8_t col, bool pressed)
{
    // Call keymap_get_keycode to get KeyReport
    KeyReport report = keymap_get_keycode(row, col, pressed);

    // Debug log
    char buf[128];
    uint32_t raw_kc = keymaps[keymap_get_active_layer()][row][col];
    snprintf(buf, sizeof(buf), "%s row=%d col=%d kc=0x%08X mods=0x%02X\n",
             pressed ? "D" : "U", row, col, raw_kc, report.modifiers);
    CDC_SendString(buf);
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
    for (int c = 0; c < MATRIX_COLS; c++)
    {
        // Include key if either pressed OR in one-shot mode (MT tap, CH)
        bool active = key_state[c].pressed || oneshot_tap[c];
        if (!active)
            continue;
        
        uint32_t kc = key_state[c].cached_kc;
        
        // Chord keys: extract and apply modifiers
        if (IS_CH(kc))
        {
            uint8_t mods = CH_MODS(kc);
            uint8_t key = CH_KEY(kc);
            *modifier_out |= mods;
            kc = key;
        }
        
        if (kc == KC_NO)
        {
            continue;
        }
        // modifiers
        if ((kc & 0xFF00u) == 0xF000u)
        { // combined mods marker (from MT hold)
            *modifier_out |= (uint8_t)(kc & 0xFFu);
            continue;
        }
        if (kc >= KC_LCTRL && kc <= KC_RGUI)
        {
            *modifier_out |= (1u << (kc - KC_LCTRL));
        }
        // 6KRO
        if (kc < 0xE0 && k6 < 6)
            keycodes6[k6++] = (uint8_t)kc;
        // NKRO
        if (kc >= NKRO_USAGE_MIN && kc <= NKRO_USAGE_MAX)
        {
            uint16_t bi = kc - NKRO_USAGE_MIN;
            nkro_bitmap[bi / 8] |= (1u << (bi % 8));
        }
    }
}

// Clear one-shot flags after report sent
void keymap_on_report_sent(void)
{
    for (int c = 0; c < MATRIX_COLS; c++)
    {
        if (oneshot_tap[c])
        {
            oneshot_tap[c] = false;
            key_state[c].cached_kc = KC_NO;
        }
    }
}

void keymap_send_hid_report()
{
    // --- Build HID reports ---
    keymap_build_hid_reports(&modifier, keycodes6, nkro_bitmap);

    // Send reports if changed
    if (nkro_enabled)
    {
        // Buffer NKRO, reduce report sends
        cur_nkro[0] = modifier;
        memcpy(&cur_nkro[1], nkro_bitmap, NKRO_BYTES_TOTAL);

        if (memcmp(cur_nkro, prev_nkro, NKRO_REPORT_LEN) != 0)
        {
            if (HID_SendKeyboardNKRO(modifier, nkro_bitmap))
            {
                memcpy(prev_nkro, cur_nkro, NKRO_REPORT_LEN);
                keymap_on_report_sent();
            }
        }
    }
    else
    {
        // Buffer 6KRO, reduce report sends
        cur6[0] = modifier;
        cur6[1] = 0;
        memcpy(&cur6[2], keycodes6, SIXKRO_BYTES_TOTAL);

        if (memcmp(cur6, prev6, sizeof(cur6)) != 0)
        {
            if (HID_SendKeyboard6KRO(modifier, keycodes6))
            {
                memcpy(prev6, cur6, sizeof(cur6));
                keymap_on_report_sent();
            }
        }
    }
}


uint32_t keymap_get_sticky_keycode(uint8_t col) { return 0; }
uint8_t keymap_get_sticky_layer(uint8_t col) { return 0; }

// Expose MT periodic tick
void keymap_mt_tick(void)
{
    mt_tick_timeout();
}
