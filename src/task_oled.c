#include "common.h"

static void draw_kbd_state(const KbdState *s)
{
    char line[32];
    // NKRO/6KRO mode at top-right (right-aligned)
    int n = snprintf(line, sizeof(line), "%s", s->nkro_enabled ? "NKRO" : "6KRO");
    int x = 128 - n * afont8x6.w; if (x < 0) x = 0;
    OLED_PrintASCIIString((uint8_t)x, 0, line, &afont8x6, OLED_COLOR_NORMAL);

    // Layers right-aligned below
    n = snprintf(line, sizeof(line), "L%d/%d (%d)", s->base_layer, s->active_layer, s->stack_size);
    x = 128 - n * afont8x6.w; if (x < 0) x = 0;
    OLED_PrintASCIIString((uint8_t)x, afont8x6.h, line, &afont8x6, OLED_COLOR_NORMAL);

    // Layer name on right side (below layer info, right-aligned)
    if (s->active_layer < MAX_LAYERS) {
        const char* name = layer_names[s->active_layer];
        int name_len = strlen(name);
        x = 128 - name_len * afont8x6.w; if (x < 0) x = 0;
        OLED_PrintASCIIString((uint8_t)x, afont8x6.h * 2, (char*)name, &afont8x6, OLED_COLOR_NORMAL);
    }

    // Debug mode status (right-aligned below layer name)
    n = snprintf(line, sizeof(line), "Debug: %s", s->debug_mode ? "1" : "0");
    x = 128 - n * afont8x6.w; if (x < 0) x = 0;
    OLED_PrintASCIIString((uint8_t)x, afont8x6.h * 3, line, &afont8x6, OLED_COLOR_NORMAL);
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

// --------------- Cute idle animation: emoji kiss with floating heart ---------------
static void draw_heart(uint8_t cx, uint8_t cy, uint8_t s)
{
    if (s < 3) s = 3; // minimum size
    uint8_t r = (uint8_t)(s / 3 + (s % 3 != 0));
    int8_t off = (int8_t)(s / 2);
    // lobes
    if (cx > off && cy > r) {
        OLED_DrawFilledCircle((uint8_t)(cx - off), (uint8_t)(cy - r), r, OLED_COLOR_NORMAL);
    }
    if ((int)cx + off < 128 && cy > r) {
        OLED_DrawFilledCircle((uint8_t)(cx + off), (uint8_t)(cy - r), r, OLED_COLOR_NORMAL);
    }
    // Safe bottom fill using horizontal scanlines (avoid triangle divide-by-zero)
    for (int y = 0; y <= s; ++y) {
        int ypix = (int)cy + y;
        if (ypix < 0 || ypix >= 64) continue;
        int half = s - y; // taper
        int x1 = (int)cx - half;
        int x2 = (int)cx + half;
        if (x2 < 0 || x1 > 127) continue;
        if (x1 < 0) x1 = 0;
        if (x2 > 127) x2 = 127;
        OLED_DrawLine((uint8_t)x1, (uint8_t)ypix, (uint8_t)x2, (uint8_t)ypix, OLED_COLOR_NORMAL);
    }
}

static void draw_cute_kiss_anim(uint32_t frame)
{
    OLED_NewFrame();

    // Small PISO logo at top-left during idle
    OLED_PrintASCIIString(1, 1, "PISO", &afont8x6, OLED_COLOR_NORMAL);

    // Face
    uint8_t fx = 64, fy = 28, fr = 14;
    OLED_DrawCircle(fx, fy, fr, OLED_COLOR_NORMAL);

    // Eyes (left open, right wink toggles)
    bool wink = ((frame / 10) % 8) < 3; // wink for ~3 ticks out of 8
    // Left eye
    OLED_DrawFilledCircle((uint8_t)(fx - 6), (uint8_t)(fy - 4), 1, OLED_COLOR_NORMAL);
    // Right eye
    if (wink) {
        OLED_DrawLine((uint8_t)(fx + 5 - 2), (uint8_t)(fy - 4), (uint8_t)(fx + 5 + 2), (uint8_t)(fy - 4), OLED_COLOR_NORMAL);
    } else {
        OLED_DrawFilledCircle((uint8_t)(fx + 6), (uint8_t)(fy - 4), 1, OLED_COLOR_NORMAL);
    }

    // Puckering mouth: small pulsating dot near the right side
    int p = frame % 40;
    int puck = (p < 20) ? p : (40 - p); // 0..20..0
    uint8_t mr = (uint8_t)(1 + puck / 10); // 1..3..1
    uint8_t mx = (uint8_t)(fx + fr - 3);
    uint8_t my = (uint8_t)(fy + 2);
    OLED_DrawFilledCircle(mx, my, mr, OLED_COLOR_NORMAL);

    // Floating heart starting near mouth and drifting up-right
    int ht = frame % 60;
    uint8_t hx = (uint8_t)(mx + 3 + ht / 2);
    uint8_t hy = (uint8_t)(my - 2 - ht / 2);
    uint8_t hs = (uint8_t)(5 + ((frame / 8) % 2)); // slight pulse 5..6
    draw_heart(hx, hy, hs);

    // Footer signature at bottom-right
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
    uint32_t anim_frame = 0;
    bool in_idle_anim = false;

    // Software timer to enter idle after ~10s without SR updates
    uint32_t no_sr_ms = 0;

    for (;;) {
        // Drain to the latest keyboard state
        bool kbd_state_changed = false;
        while (xQueueReceive(xKbdStateQueue, &kbd_state, 0) == pdTRUE) {
            kbd_state_changed = true;
        }
        // If only state changed, redraw using the last snapshot to keep alignment
        if (kbd_state_changed && have_recv && !in_idle_anim) {
            render_main(&kbd_state, &last_recv);
        }

        if (!in_idle_anim) {
            // Short wait for SR update; keep servicing KbdState frequently
            if (xQueueReceive(xShiftRegisterOutputQueue_OLED, &recv, pdMS_TO_TICKS(5)) == pdTRUE) {
                last_recv = recv; have_recv = true;
                no_sr_ms = 0;
                render_main(&kbd_state, &last_recv);
            } else {
                // No SR this tick, advance idle timer
                if (no_sr_ms < 10000) no_sr_ms += 5;
                if (no_sr_ms >= 10000) {
                    in_idle_anim = true;
                    anim_frame = 0;
                }
            }
        } else {
            // Run one animation frame
            draw_cute_kiss_anim(anim_frame++);
            // On new snapshot, exit idle and render both
            if (xQueueReceive(xShiftRegisterOutputQueue_OLED, &recv, 0) == pdTRUE) {
                last_recv = recv; have_recv = true;
                // Drain to latest kbd state before drawing
                while (xQueueReceive(xKbdStateQueue, &kbd_state, 0) == pdTRUE) {}
                render_main(&kbd_state, &last_recv);
                in_idle_anim = false;
                no_sr_ms = 0;
                continue;
            }
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}