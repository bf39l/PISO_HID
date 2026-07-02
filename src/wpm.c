#include "wpm.h"
#include "pico/stdlib.h"

#define WPM_INTERVAL_MS  1000

static uint32_t wpm_count;
static uint32_t wpm_last_calc_ms;
static uint16_t wpm_cached;

void wpm_init(void)
{
    wpm_count = 0;
    wpm_last_calc_ms = 0;
    wpm_cached = 0;
}

void wpm_record_keystroke(void)
{
    wpm_count++;
}

uint16_t wpm_get(void)
{
    uint32_t now = to_ms_since_boot(get_absolute_time());
    uint32_t elapsed = now - wpm_last_calc_ms;

    if (elapsed >= WPM_INTERVAL_MS)
    {
        wpm_last_calc_ms = now;

        if (wpm_count > 0)
        {
            wpm_cached = (uint16_t)(wpm_count * 12);
            wpm_count = 0;
        }
        else
        {
            // Decay: drop by ~50% per second, floor at 0
            if (wpm_cached > 0)
            {
                uint16_t delta = (wpm_cached * 50) / 100;
                if (delta < 1)
                    delta = 1;
                wpm_cached -= delta;
            }
        }
    }

    return wpm_cached;
}