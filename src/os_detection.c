#include "os_detection.h"
#include "pico/stdlib.h"

#ifndef OS_DETECTION_DEBOUNCE_MS
#define OS_DETECTION_DEBOUNCE_MS 300
#endif

static struct {
    uint8_t  count;
    uint8_t  cnt_02;
    uint8_t  cnt_04;
    uint8_t  cnt_ff;
    uint16_t last_wlength;
} setups_data = {0};

static volatile os_variant_t detected_os = OS_UNSURE;

static volatile uint32_t last_detect_ms = 0;
static volatile bool     debouncing = false;

// Strong override of TinyUSB's weak tud_os_detect_wlength_cb stub.
// TinyUSB calls this from process_get_descriptor() for every GET_DESCRIPTOR request.
void tud_os_detect_wlength_cb(uint16_t wlength)
{
    process_wlength(wlength);
}

const char* os_variant_string(os_variant_t os)
{
    switch (os) {
        case OS_WINDOWS: return "WIN";
        case OS_MACOS:   return "MAC";
        case OS_LINUX:   return "LNX";
        default:         return "???";
    }
}

void process_wlength(const uint16_t w_length)
{
    setups_data.count++;
    setups_data.last_wlength = w_length;

    if (w_length == 0x02) {
        setups_data.cnt_02++;
    } else if (w_length == 0x04) {
        setups_data.cnt_04++;
    } else if (w_length == 0xFF) {
        setups_data.cnt_ff++;
    }

    os_variant_t guessed = OS_UNSURE;

    if (setups_data.count >= 3) {
        // Windows: cnt_ff >= 2 && cnt_04 >= 1
        if (setups_data.cnt_ff >= 2 && setups_data.cnt_04 >= 1) {
            guessed = OS_WINDOWS;
        }
        // Linux: all packets are 0xFF
        else if (setups_data.count == setups_data.cnt_ff) {
            guessed = OS_LINUX;
        }
        // macOS: last is 0xFF, cnt_ff >= 1, cnt_02 >= 2
        else if (setups_data.count >= 5 &&
                 setups_data.last_wlength == 0xFF &&
                 setups_data.cnt_ff >= 1 &&
                 setups_data.cnt_02 >= 2) {
            guessed = OS_MACOS;
        }
        // Fallback: if only 0xFF seen, treat as Linux
        else if (setups_data.cnt_ff > 0 &&
                 setups_data.cnt_02 == 0 &&
                 setups_data.cnt_04 == 0) {
            guessed = OS_LINUX;
        }
    }

    if (guessed != OS_UNSURE) {
        detected_os = guessed;
    }

    last_detect_ms = to_ms_since_boot(get_absolute_time());
    debouncing = true;
}

os_variant_t detected_host_os(void)
{
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (debouncing && (now - last_detect_ms < OS_DETECTION_DEBOUNCE_MS)) {
        return OS_UNSURE;
    }
    if (!debouncing && detected_os == OS_UNSURE) {
        return OS_UNSURE;
    }
    debouncing = false;
    return detected_os;
}

void erase_wlength_data(void)
{
    setups_data.count = 0;
    setups_data.cnt_02 = 0;
    setups_data.cnt_04 = 0;
    setups_data.cnt_ff = 0;
    setups_data.last_wlength = 0;
    detected_os = OS_UNSURE;
    last_detect_ms = 0;
    debouncing = false;
}
