#ifndef __SHIFTREGISTER_PISO__H
#define __SHIFTREGISTER_PISO__H

#include <stdio.h>
#include "pico/stdlib.h"

#define PIN_SHLD  6  // SH/LD control
#define PIN_CLK   7  // Shift clock
#define PIN_CLKEN 8  // Shift clock Enable
#define PIN_DATA  9  // Serial data out from first 74HC165 ^QH
#define MAX_BITS  8
#define NUM_SHIFT_REGISTERS 8

typedef struct {
  uint32_t high;  // bits 63..32 (first 4 chips)
  uint32_t low;   // bits 31..0  (last 4 chips)
} ShiftRegister64;

void shift_register_init(void);
ShiftRegister64 read_shift_registers();
void shift_bits_to_strings(ShiftRegister64 bits, char out[NUM_SHIFT_REGISTERS][MAX_BITS + 1]);
void print_shift_bits_usb(ShiftRegister64 bits);

#endif