#include "common.h"
#include "bongo_cat_bitmap.h"

static void draw_kbd_state(const KbdState *s)
{
    char line[32];
    int n, x;

    // NKRO/6KRO mode
    n = snprintf(line, sizeof(line), "%s", s->nkro_enabled ? "NKRO" : "6KRO");
    x = s->debug_mode ? (128 - n * afont8x6.w) : 0; if (x < 0) x = 0;
    OLED_PrintASCIIString((uint8_t)x, 0, line, &afont8x6, OLED_COLOR_NORMAL);

    // Keyboard name (top-right, non-debug only)
    if (!s->debug_mode) {
        OLED_PrintASCIIString(128 - 4 * afont8x6.w, 0, "PISO", &afont8x6, OLED_COLOR_NORMAL);
    }

    // Layers
    n = snprintf(line, sizeof(line), "L%d/%d (%d)", s->base_layer, s->active_layer, s->stack_size);
    x = s->debug_mode ? (128 - n * afont8x6.w) : 0; if (x < 0) x = 0;
    OLED_PrintASCIIString((uint8_t)x, afont8x6.h, line, &afont8x6, OLED_COLOR_NORMAL);

    // Layer name
    if (s->active_layer < MAX_LAYERS) {
        const char* name = layer_names[s->active_layer];
        int name_len = strlen(name);
        x = s->debug_mode ? (128 - name_len * afont8x6.w) : 0; if (x < 0) x = 0;
        OLED_PrintASCIIString((uint8_t)x, afont8x6.h * 2, (char*)name, &afont8x6, OLED_COLOR_NORMAL);
    }

    // Author name (right side, non-debug only)
    if (!s->debug_mode) {
        OLED_PrintASCIIString(128 - 5 * afont8x6.w, afont8x6.h * 2, "bf39L", &afont8x6, OLED_COLOR_NORMAL);
    }

    // Debug mode status
    n = snprintf(line, sizeof(line), "Debug: %s", s->debug_mode ? "1" : "0");
    x = s->debug_mode ? (128 - n * afont8x6.w) : 0; if (x < 0) x = 0;
    OLED_PrintASCIIString((uint8_t)x, afont8x6.h * 3, line, &afont8x6, OLED_COLOR_NORMAL);

    // WPM
    n = snprintf(line, sizeof(line), "WPM:%u", s->wpm);
    x = s->debug_mode ? (128 - n * afont8x6.w) : 0; if (x < 0) x = 0;
    OLED_PrintASCIIString((uint8_t)x, afont8x6.h * 4, line, &afont8x6, OLED_COLOR_NORMAL);
}

static void draw_shift_bits_oled(ShiftRegister64 bits, const ASCIIFont *font)
{
    char lines[NUM_SHIFT_REGISTERS][MAX_BITS + 1];
    shift_bits_to_strings(bits, lines);

    for (int i = 0; i < NUM_SHIFT_REGISTERS; i++) {
        char label[16];
        snprintf(label, sizeof(label), "SR%d:%s", i + 1, lines[i]);
        // SR1 on same line as NKRO/6KRO (Y=0), then SR2 onwards shifted up one line
        uint8_t y = (i == 0) ? 0 : (uint8_t)((i - 1) * font->h + afont8x6.h);
        OLED_PrintASCIIString(0, y, label, font, OLED_COLOR_NORMAL);
    }  
}

// Render a full frame with current state and last shift-register snapshot
static inline void render_main(const KbdState *state, const ShiftRegister64 *sr)
{
    OLED_NewFrame();
    draw_kbd_state(state);

    if (state->debug_mode) {
        draw_shift_bits_oled(*sr, &afont8x6);
    } else {
        uint8_t cx = 0;
        uint8_t cy = 32;
        draw_cat_at(cx, cy, cat_get_current_state(), cat_get_current_frame());
    }

    OLED_ShowFrame();
}

// Draw splash screen
static void draw_splash(void)
{
    OLED_NewFrame();
    OLED_DrawEllipse(128/2-1, 64/2, 32, 18, OLED_COLOR_NORMAL);
    OLED_DrawEllipse(128/2-1, 64/2, 16, 30, OLED_COLOR_NORMAL);
    OLED_PrintASCIIString(128/2 - 4 * afont24x12.w / 2, 64/2 - afont24x12.h/2, "PISO", &afont24x12, OLED_COLOR_NORMAL);
    OLED_PrintASCIIString(128-5*afont8x6.w, 64 - afont8x6.h, "bf39L", &afont8x6, OLED_COLOR_NORMAL);
    OLED_ShowFrame();
}

void OLED_Task(void *pvParameters)
{
    OLED_Init(i2c0, 2);

    // Show splash initially
    draw_splash();

    ShiftRegister64 recv;
    ShiftRegister64 last_recv = (ShiftRegister64){0};
    bool have_recv = false;
    KbdState kbd_state = (KbdState){0};

    for (;;) {
        // Drain to the latest keyboard state
        bool kbd_state_changed = false;
        while (xQueueReceive(xKbdStateQueue, &kbd_state, 0) == pdTRUE) {
            kbd_state_changed = true;
        }
        // If only state changed, redraw using the last snapshot to keep alignment
        if (kbd_state_changed && have_recv) {
            render_main(&kbd_state, &last_recv);
        }

        // Short wait for SR update; keep servicing KbdState frequently
        if (xQueueReceive(xShiftRegisterOutputQueue_OLED, &recv, pdMS_TO_TICKS(5)) == pdTRUE) {
            last_recv = recv; have_recv = true;
            render_main(&kbd_state, &last_recv);
        }
    }
}
