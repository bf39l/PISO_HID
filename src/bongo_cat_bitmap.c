#include "bongo_cat_bitmap.h"
#include "oled.h"
#include "wpm.h"

#define CAT_ANIM_FRAME_DURATION 75
#define CAT_IDLE_TIMEOUT 500

#define BONGO_PAGE_HEIGHT 8
#define BONGO_NUM_PAGES 4
#define BONGO_NUM_COLS 128

static CatState cat_current_state = CAT_STATE_IDLE;
static uint32_t cat_anim_timer = 0;
static uint32_t cat_idle_timeout_timer = 0;
static uint8_t cat_current_idle_frame = 0;
static uint8_t cat_current_tap_frame = 0;

static void draw_frame(uint8_t x, uint8_t y, const uint8_t *frame_data)
{
    for (uint8_t page = 0; page < BONGO_NUM_PAGES; ++page)
    {
        const uint8_t *page_data = frame_data + (uint16_t)page * BONGO_NUM_COLS;
        uint8_t gram_page = (uint8_t)(y / BONGO_PAGE_HEIGHT) + page;

        if (gram_page >= OLED_DISPLAY_PAGES)
        {
            break;
        }

        for (uint16_t col = 0; col < BONGO_NUM_COLS; ++col)
        {
            uint8_t byte = page_data[col];
            if (byte == 0u)
            {
                continue;
            }

            uint16_t gram_col = (uint16_t)x + col;
            if (gram_col >= OLED_DISPLAY_COLS)
            {
                continue;
            }

            for (uint8_t bit = 0; bit < BONGO_PAGE_HEIGHT; ++bit)
            {
                if (byte & (1u << bit))
                {
                    uint16_t gram_row = (uint16_t)y + (uint16_t)page * BONGO_PAGE_HEIGHT + bit;
                    if (gram_row < OLED_DISPLAY_ROWS)
                    {
                        OLED_GRAM[gram_page][gram_col] |= (1u << bit);
                    }
                }
            }
        }
    }
}

CatState cat_state_from_wpm(uint16_t wpm)
{
    if (wpm >= 20) return CAT_STATE_TAP;
    if (wpm >= 10) return CAT_STATE_PREP;
    return CAT_STATE_IDLE;
}

uint32_t cat_anim_frame_delay(uint16_t wpm)
{
    int32_t delay = 300 - (wpm * 2);
    if (delay < 80) delay = 80;
    if (delay > 250) delay = 250;
    return (uint32_t)delay;
}

void cat_state_machine_tick(uint32_t elapsed_ms)
{
    uint16_t wpm = (uint16_t)wpm_get();
    CatState target_state = cat_state_from_wpm(wpm);

    cat_anim_timer += elapsed_ms;
    cat_idle_timeout_timer += elapsed_ms;

    uint32_t frame_duration = cat_anim_frame_delay(wpm);

    if (cat_current_state != target_state)
    {
        cat_current_state = target_state;
        cat_anim_timer = 0;
        cat_idle_timeout_timer = 0;

        if (cat_current_state == CAT_STATE_IDLE)
        {
            cat_current_idle_frame = 0;
        }
        else if (cat_current_state == CAT_STATE_TAP)
        {
            cat_current_tap_frame = 0;
        }
    }

    if (cat_current_state == CAT_STATE_IDLE)
    {
        if (cat_idle_timeout_timer >= CAT_IDLE_TIMEOUT)
        {
            if (cat_anim_timer >= frame_duration)
            {
                cat_anim_timer = 0;
                cat_current_idle_frame = (cat_current_idle_frame + 1) % BONGO_IDLE_FRAMES;
            }
        }
    }
    else if (cat_current_state == CAT_STATE_TAP)
    {
        if (cat_anim_timer >= frame_duration)
        {
            cat_anim_timer = 0;
            cat_current_tap_frame = (cat_current_tap_frame + 1) % BONGO_TAP_FRAMES;
        }
    }
}

CatState cat_get_current_state(void)
{
    return cat_current_state;
}

uint8_t cat_get_current_frame(void)
{
    switch (cat_current_state)
    {
        case CAT_STATE_IDLE:
            return cat_current_idle_frame;
        case CAT_STATE_TAP:
            return cat_current_tap_frame;
        case CAT_STATE_PREP:
            return 0;
        default:
            return 0;
    }
}

void draw_cat_at(uint8_t x, uint8_t y, CatState state, uint8_t frame)
{
    const uint8_t *frame_data = (const uint8_t *)bongo_idle_frames[0];

    switch (state)
    {
        case CAT_STATE_IDLE:
            frame_data = (const uint8_t *)bongo_idle_frames[frame % BONGO_IDLE_FRAMES];
            break;
        case CAT_STATE_PREP:
            frame_data = (const uint8_t *)bongo_prep_frames[0];
            break;
        case CAT_STATE_TAP:
            frame_data = (const uint8_t *)bongo_tap_frames[frame % BONGO_TAP_FRAMES];
            break;
        default:
            frame_data = (const uint8_t *)bongo_idle_frames[0];
            break;
    }

    draw_frame(x, y, frame_data);
}
