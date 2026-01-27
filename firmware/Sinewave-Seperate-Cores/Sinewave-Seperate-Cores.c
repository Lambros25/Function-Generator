#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

#define LADDER_MASK 0xFF // Pins 0 through 7

int main() {
    stdio_init_all();

    PIO pio = pio0;
    uint sm = 0;

    // 1. Tiny PIO program: Pull 32 bits, output 8 bits, loop.
    // This is the machine code for 'out pins, 8'
    uint16_t instructions[] = { 0x6008 }; 
    struct pio_program prg = {
        .instructions = instructions,
        .length = 1,
        .origin = -1,
    };
    uint offset = pio_add_program(pio, &prg);

    // 2. Configure the pins for PIO use
    for(int i = 0; i < 8; i++) {
        pio_gpio_init(pio, i);
    }
    pio_sm_set_consecutive_pindirs(pio, sm, 0, 8, true);

    // 3. Setup the State Machine Configuration
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_out_pins(&c, 0, 8);
    // Auto-pull: when the 32-bit shift register is empty, grab another 32 bits from FIFO
    sm_config_set_out_shift(&c, true, true, 32); 
    sm_config_set_wrap(&c, offset, offset); // Loop the single instruction
    
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);

    while (true) {
        // Push 4 steps of the ladder in one go
        // 0x00, 0x55, 0xAA, 0xFF (0, 85, 170, 255)
        if (!pio_sm_is_tx_fifo_full(pio, sm)) {
            pio_sm_put(pio, sm, 0xFF00AA55); 
        }
    }
}