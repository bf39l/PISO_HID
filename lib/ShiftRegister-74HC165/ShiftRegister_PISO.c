#include "ShiftRegister_PISO.h"

void shift_register_init(void)
{
    gpio_init(PIN_SHLD);
    gpio_init(PIN_CLK);
    gpio_init(PIN_CLKEN);
    gpio_init(PIN_DATA);

    gpio_set_dir(PIN_SHLD, GPIO_OUT);
    gpio_set_dir(PIN_CLK, GPIO_OUT);
    gpio_set_dir(PIN_CLKEN, GPIO_OUT);
    gpio_set_dir(PIN_DATA, GPIO_IN);

    // Default states
    gpio_put(PIN_SHLD, 1);   // not loading
    gpio_put(PIN_CLK, 0);
    gpio_put(PIN_CLKEN, 1);  // disable clock
}

ShiftRegister64 read_shift_registers()
{
    ShiftRegister64 out = {0, 0};

    // Disable clock, latch parallel inputs
    gpio_put(PIN_CLKEN, 1);
    sleep_us(1);
    gpio_put(PIN_SHLD, 0);
    sleep_us(1);
    gpio_put(PIN_SHLD, 1);
    sleep_us(1);
    gpio_put(PIN_CLKEN, 0); // enable clock

    // Read 64 bits (MSB first)
    for (int i = 0; i < NUM_SHIFT_REGISTERS * MAX_BITS; i++) {
        // Shift the output left
        out.high <<= 1;
        if (out.low & 0x80000000u) {
            out.high |= 1;
        }
        out.low <<= 1;
        out.low |= gpio_get(PIN_DATA);

        // Clock pulse
        gpio_put(PIN_CLK, 1);
        sleep_us(1);
        gpio_put(PIN_CLK, 0);
        sleep_us(1);
    }

    return out;
}

void shift_bits_to_strings(ShiftRegister64 bits, char out[NUM_SHIFT_REGISTERS][MAX_BITS + 1]) {
    for (int reg = 0; reg < NUM_SHIFT_REGISTERS; reg++) {
        for (int bit = 0; bit < MAX_BITS; bit++) {
            int bitIndex = reg * MAX_BITS + bit;
            uint8_t value;

            if (bitIndex >= 32)
                value = (bits.high >> (bitIndex - 32)) & 1;
            else
                value = (bits.low >> bitIndex) & 1;

            // Convert to '0' or '1'
            out[NUM_SHIFT_REGISTERS - 1 - reg][MAX_BITS - 1 - bit] = value ? '1' : '0';
        }

        out[NUM_SHIFT_REGISTERS - 1 - reg][MAX_BITS] = '\0';
    }
}

void print_shift_bits_usb(ShiftRegister64 bits) {
    char lines[NUM_SHIFT_REGISTERS][MAX_BITS + 1];
    shift_bits_to_strings(bits, lines);

    printf("Bits: (");
    // print from SR8 -> SR1 to match the original bit order (MSB first)
    for (int i = 0; i < NUM_SHIFT_REGISTERS; i++) {
        for (int j = 0; j < MAX_BITS; j++) {
            printf("%c", lines[i][j]);
        }

        if (i != NUM_SHIFT_REGISTERS - 1)  // space between each shift register
            printf(" ");
    }
    printf(")\n");
}
