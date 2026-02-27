#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include <stdlib.h>
#include <string.h>

// ===== CONFIGURATION =====
volatile float calibration_factor = 1.0f;  // User adjustable
#define SINE_TABLE_SIZE 256
#define SAMPLE_RATE_HZ 4034843
#define DAC_PINS_BASE 0          // R-2R on pins 0-7

// ===== WAVEFORM TYPES =====
typedef enum {
    WAVEFORM_SINE,
    WAVEFORM_SQUARE
} waveform_type_t;

// ===== GLOBALS =====
uint8_t sine_table[SINE_TABLE_SIZE];
uint8_t square_table[SINE_TABLE_SIZE];
volatile uint32_t frequency_hz = 1000;  // Default 1 kHz
volatile bool frequency_changed = false;
volatile waveform_type_t current_waveform = WAVEFORM_SINE;
volatile bool waveform_changed = false;

// ===== PIO PROGRAM =====
// Single instruction: output 8 bits to pins
const uint16_t dac_program_instructions[] = {
    0x6008,  // out pins, 8
};

const struct pio_program dac_program = {
    .instructions = dac_program_instructions,
    .length = 1,
    .origin = -1,
};

// ===== HELPER FUNCTION =====
// Reverse bit order in a byte
uint8_t reverse_bits(uint8_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;  // Swap nibbles
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;  // Swap pairs
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;  // Swap bits
    return b;
}

// ===== WAVEFORM TABLE GENERATION =====
void generate_sine_table() {
    for (int i = 0; i < SINE_TABLE_SIZE; i++) {
        // Generate sine wave: 0-255 range, centered at 127.5
        float angle = (2.0f * M_PI * i) / SINE_TABLE_SIZE;
        uint8_t value = (uint8_t)(127.5f + 127.5f * sinf(angle));
        sine_table[i] = reverse_bits(value);  // Pre-reverse for flipped wiring
    }
}

void generate_square_table() {
    for (int i = 0; i < SINE_TABLE_SIZE; i++) {
        // First half: high (255), second half: low (0)
        uint8_t value = (i < SINE_TABLE_SIZE / 2) ? 255 : 0;
        square_table[i] = reverse_bits(value);  // Pre-reverse for flipped wiring
    }
}

// ===== CORE 1: WAVEFORM GENERATION =====
void core1_entry() {
    // Setup PIO
    PIO pio = pio0;
    uint sm = 0;
    
    // Load PIO program
    uint offset = pio_add_program(pio, &dac_program);
    
    // Configure pins 0-7 for PIO output
    for (int i = 0; i < 8; i++) {
        pio_gpio_init(pio, DAC_PINS_BASE + i);
    }
    pio_sm_set_consecutive_pindirs(pio, sm, DAC_PINS_BASE, 8, true);
    
    // Configure state machine
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_out_pins(&c, DAC_PINS_BASE, 8);
    sm_config_set_out_shift(&c, true, true, 8); // Auto-pull every 8 bits
    sm_config_set_wrap(&c, offset, offset);
    
    // Set PIO clock divider for exact sample rate
    float div = (float)clock_get_hz(clk_sys) / SAMPLE_RATE_HZ;
    sm_config_set_clkdiv(&c, div);
    
    // Start state machine
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
    
    // Waveform generation loop
    uint32_t phase_accumulator = 0;
    uint32_t local_frequency = frequency_hz;
    float local_calibration = calibration_factor;
    waveform_type_t local_waveform = current_waveform;
    uint8_t *active_table = sine_table;  // Pointer to current waveform table
    uint32_t phase_increment = ((uint64_t)local_frequency * 4294967296ULL) / (uint32_t)(SAMPLE_RATE_HZ * local_calibration);
    
    while (true) {
        // Check for frequency or calibration updates from Core 0
        if (frequency_changed) {
            local_frequency = frequency_hz;
            local_calibration = calibration_factor;
            phase_increment = ((uint64_t)local_frequency * 4294967296ULL) / (uint32_t)(SAMPLE_RATE_HZ * local_calibration);
            frequency_changed = false;
        }
        
        // Check for waveform changes
        if (waveform_changed) {
            local_waveform = current_waveform;
            active_table = (local_waveform == WAVEFORM_SINE) ? sine_table : square_table;
            waveform_changed = false;
        }
        
        // Generate samples
        for (int i = 0; i < 32; i++) {
            // Get table index from upper bits of phase accumulator
            uint8_t table_index = phase_accumulator >> 24;
            uint8_t sample = active_table[table_index];  // Already bit-reversed in the table
            
            // Feed to PIO (blocking if FIFO full)
            pio_sm_put_blocking(pio, sm, sample);
            
            // Increment phase
            phase_accumulator += phase_increment;
        }
    }
}

// ===== CORE 0: USER INTERFACE =====
int main() {
    stdio_init_all();
    
    // Wait for USB serial connection (optional, remove if annoying)
    sleep_ms(2000);
    
    printf("\n=== Pico Signal Generator ===\n");
    printf("Commands:\n");
    printf("  f<number> - Set frequency in Hz (e.g., f1000 for 1kHz)\n");
    printf("  c<number> - Set calibration factor (e.g., c1.06 for 6%% correction)\n");
    printf("  sine      - Switch to sine wave\n");
    printf("  square    - Switch to square wave\n");
    printf("  i         - Show current info\n\n");
    
    // Generate waveform lookup tables
    generate_sine_table();
    generate_square_table();
    
    // Launch Core 1 for waveform generation
    multicore_launch_core1(core1_entry);
    
    printf("Generator running at %d Hz\n", frequency_hz);
    printf("Waveform: SINE\n");
    printf("Calibration factor: %.4f\n", calibration_factor);
    
    // Command processing loop
    char input_buffer[64];
    int buffer_pos = 0;
    
    while (true) {
        int c = getchar_timeout_us(0);
        
        if (c != PICO_ERROR_TIMEOUT) {
            if (c == '\n' || c == '\r') {
                // Process command
                input_buffer[buffer_pos] = '\0';
                
                if (buffer_pos > 0) {
                    if (input_buffer[0] == 'f') {
                        // Frequency command
                        uint32_t new_freq = atoi(&input_buffer[1]);
                        if (new_freq > 0 && new_freq <= 1000000) {
                            frequency_hz = new_freq;
                            frequency_changed = true;
                            printf("Frequency set to %d Hz\n", new_freq);
                        } else {
                            printf("Invalid frequency (range: 1-1000000 Hz)\n");
                        }
                    } else if (input_buffer[0] == 'c') {
                        // Calibration command
                        float new_cal = atof(&input_buffer[1]);
                        if (new_cal > 0.5 && new_cal < 1.5) {
                            calibration_factor = new_cal;
                            frequency_changed = true;
                            printf("Calibration factor set to %.4f\n", new_cal);
                        } else {
                            printf("Invalid calibration (range: 0.5-1.5)\n");
                        }
                    } else if (strcmp(input_buffer, "sine") == 0) {
                        // Sine wave command
                        current_waveform = WAVEFORM_SINE;
                        waveform_changed = true;
                        printf("Waveform: SINE\n");
                    } else if (strcmp(input_buffer, "square") == 0) {
                        // Square wave command
                        current_waveform = WAVEFORM_SQUARE;
                        waveform_changed = true;
                        printf("Waveform: SQUARE\n");
                    } else if (input_buffer[0] == 'i') {
                        // Info command
                        printf("Current frequency: %d Hz\n", frequency_hz);
                        printf("Sample rate: %d Hz\n", SAMPLE_RATE_HZ);
                        printf("Waveform: %s\n", (current_waveform == WAVEFORM_SINE) ? "SINE" : "SQUARE");
                        printf("Calibration factor: %.4f\n", calibration_factor);
                        printf("Table size: %d samples\n", SINE_TABLE_SIZE);
                    }
                }
                
                buffer_pos = 0;
                printf("> ");
            } else if (buffer_pos < 63) {
                input_buffer[buffer_pos++] = c;
            }
        }
        
        sleep_ms(10);
    }
}