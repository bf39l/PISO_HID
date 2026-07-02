#pragma once

#include <stdint.h>

void wpm_init(void);
void wpm_record_keystroke(void);
uint16_t wpm_get(void);
