#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include <stdlib.h>
#include <string.h>

// ===== CONFIGURATION =====
volatile float calibration_factor = 1.0f;
#define SINE_TABLE_SIZE 256
#define SAMPLE_RATE_HZ 4034843
#define DAC_PINS_BASE 0
#define SQUARE_OUT_PIN 9
#define OVERCLOCK_MHZ 225

// ===== WAVEFORM TYPES =====
typedef enum
{
    WAVEFORM_SINE,
    WAVEFORM_SQUARE
} waveform_type_t;

// ===== GLOBALS =====
uint8_t sine_table[SINE_TABLE_SIZE];
volatile uint32_t frequency_hz = 1000;
volatile bool frequency_changed = false;
volatile waveform_type_t current_waveform = WAVEFORM_SINE;
volatile bool waveform_changed = false;

// ===== PIO PROGRAM FOR DAC (SINE) =====
const uint16_t dac_program_instructions[] = {
    0x6008, // out pins, 8
};

const struct pio_program dac_program = {
    .instructions = dac_program_instructions,
    .length = 1,
    .origin = -1,
};

// ===== PIO PROGRAM FOR SQUARE WAVE =====
// This PIO program toggles a pin at the configured clock rate
// .wrap_target
//     set pins, 1
//     set pins, 0
// .wrap
const uint16_t square_program_instructions[] = {
    0xe001, // set pins, 1
    0xe000, // set pins, 0
};

const struct pio_program square_program = {
    .instructions = square_program_instructions,
    .length = 2,
    .origin = -1,
};

// ===== HELPER FUNCTION =====
uint8_t reverse_bits(uint8_t b)
{
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

// ===== WAVEFORM TABLE GENERATION =====
void generate_sine_table()
{
    for (int i = 0; i < SINE_TABLE_SIZE; i++)
    {
        float angle = (2.0f * M_PI * i) / SINE_TABLE_SIZE;
        uint8_t value = (uint8_t)(127.5f + 127.5f * sinf(angle));
        sine_table[i] = reverse_bits(value);
    }
}

// ===== CORE 1: WAVEFORM GENERATION =====
void core1_entry()
{
    // Setup PIO0 SM0 for DAC (sine wave)
    PIO pio_dac = pio0;
    uint sm_dac = 0;

    uint offset_dac = pio_add_program(pio_dac, &dac_program);

    for (int i = 0; i < 8; i++)
    {
        pio_gpio_init(pio_dac, DAC_PINS_BASE + i);
    }
    pio_sm_set_consecutive_pindirs(pio_dac, sm_dac, DAC_PINS_BASE, 8, true);

    pio_sm_config c_dac = pio_get_default_sm_config();
    sm_config_set_out_pins(&c_dac, DAC_PINS_BASE, 8);
    sm_config_set_out_shift(&c_dac, true, true, 8);
    sm_config_set_wrap(&c_dac, offset_dac, offset_dac);

    float div_dac = (float)clock_get_hz(clk_sys) / SAMPLE_RATE_HZ;
    sm_config_set_clkdiv(&c_dac, div_dac);

    pio_sm_init(pio_dac, sm_dac, offset_dac, &c_dac);
    pio_sm_set_enabled(pio_dac, sm_dac, true);

    // Setup PIO0 SM1 for square wave
    PIO pio_square = pio0;
    uint sm_square = 1;

    uint offset_square = pio_add_program(pio_square, &square_program);

    pio_gpio_init(pio_square, SQUARE_OUT_PIN);
    pio_sm_set_consecutive_pindirs(pio_square, sm_square, SQUARE_OUT_PIN, 1, true);

    pio_sm_config c_square = pio_get_default_sm_config();
    sm_config_set_set_pins(&c_square, SQUARE_OUT_PIN, 1);
    sm_config_set_wrap(&c_square, offset_square, offset_square + 1);

    // Calculate clock divider for square wave
    // PIO runs 2 instructions per cycle (set 1, set 0)
    // So output frequency = clk_sys / (clkdiv * 2)
    // Therefore: clkdiv = clk_sys / (2 * desired_frequency)
    uint32_t sys_clock = clock_get_hz(clk_sys);
    float div_square = (float)sys_clock / (2.0f * frequency_hz);
    sm_config_set_clkdiv(&c_square, div_square);

    pio_sm_init(pio_square, sm_square, offset_square, &c_square);
    // Don't enable square wave PIO yet - only when user selects it

    uint32_t phase_accumulator = 0;
    uint32_t local_frequency = frequency_hz;
    float local_calibration = calibration_factor;
    waveform_type_t local_waveform = current_waveform;

    uint32_t sine_phase_increment = ((uint64_t)local_frequency * 4294967296ULL) / (uint32_t)(SAMPLE_RATE_HZ * local_calibration);

    while (true)
    {
        if (frequency_changed)
        {
            local_frequency = frequency_hz;
            local_calibration = calibration_factor;

            // Update sine phase increment
            sine_phase_increment = ((uint64_t)local_frequency * 4294967296ULL) / (uint32_t)(SAMPLE_RATE_HZ * local_calibration);

            // Update square wave PIO clock divider
            float new_div_square = (float)sys_clock / (2.0f * local_frequency);
            pio_sm_set_clkdiv(pio_square, sm_square, new_div_square);

            frequency_changed = false;
        }

        if (waveform_changed)
        {
            local_waveform = current_waveform;

            if (local_waveform == WAVEFORM_SQUARE)
            {
                // Enable square wave PIO
                pio_sm_set_enabled(pio_square, sm_square, true);
            }
            else
            {
                // Disable square wave PIO and set pin low
                pio_sm_set_enabled(pio_square, sm_square, false);
                gpio_init(SQUARE_OUT_PIN);
                gpio_set_dir(SQUARE_OUT_PIN, GPIO_OUT);
                gpio_put(SQUARE_OUT_PIN, 0);
                // Re-init for PIO control
                pio_gpio_init(pio_square, SQUARE_OUT_PIN);
            }

            waveform_changed = false;
            phase_accumulator = 0;
        }

        if (local_waveform == WAVEFORM_SINE)
        {
            // --- SINE MODE: feed R-2R DAC via PIO ---
            for (int i = 0; i < 32; i++)
            {
                uint8_t table_index = phase_accumulator >> 24;
                uint8_t sample = sine_table[table_index];
                pio_sm_put_blocking(pio_dac, sm_dac, sample);
                phase_accumulator += sine_phase_increment;
            }
        }
        else
        {
            // --- SQUARE MODE: PIO handles everything, just sleep ---
            sleep_ms(10);
        }
    }
}

// ===== CORE 0: USER INTERFACE =====
int main()
{
    // Overclock to 225MHz
    vreg_set_voltage(VREG_VOLTAGE_1_20);
    sleep_ms(10);
    set_sys_clock_khz(OVERCLOCK_MHZ * 1000, true);

    stdio_init_all();
    sleep_ms(2000);

    printf("\n=== Pico Signal Generator (Overclocked to %d MHz) ===\n", OVERCLOCK_MHZ);
    printf("Commands:\n");
    printf("  f<number> - Set frequency in Hz (e.g., f1000 for 1kHz)\n");
    printf("  c<number> - Set calibration factor (e.g., c1.06, SINE ONLY)\n");
    printf("  sine      - Switch to sine wave (output: pins 0-7 R-2R DAC)\n");
    printf("  square    - Switch to square wave (output: GPIO 9, PIO-generated)\n");
    printf("  i         - Show current info\n\n");

    generate_sine_table();

    multicore_launch_core1(core1_entry);

    printf("Generator running at %d Hz\n", frequency_hz);
    printf("Waveform: SINE\n");
    printf("Calibration factor: %.4f (applies to SINE only)\n", calibration_factor);

    char input_buffer[64];
    int buffer_pos = 0;

    while (true)
    {
        int c = getchar_timeout_us(0);

        if (c != PICO_ERROR_TIMEOUT)
        {
            if (c == '\n' || c == '\r')
            {
                input_buffer[buffer_pos] = '\0';

                if (buffer_pos > 0)
                {
                    if (input_buffer[0] == 'f')
                    {
                        uint32_t new_freq = atoi(&input_buffer[1]);
                        if (new_freq > 0 && new_freq <= 50000000)
                        {
                            frequency_hz = new_freq;
                            frequency_changed = true;
                            printf("Frequency set to %d Hz\n", new_freq);
                        }
                        else
                        {
                            printf("Invalid frequency (range: 1-50000000 Hz)\n");
                        }
                    }
                    else if (input_buffer[0] == 'c')
                    {
                        float new_cal = atof(&input_buffer[1]);
                        if (new_cal > 0.5 && new_cal < 1.5)
                        {
                            calibration_factor = new_cal;
                            frequency_changed = true;
                            printf("Calibration factor set to %.4f (SINE mode only)\n", new_cal);
                        }
                        else
                        {
                            printf("Invalid calibration (range: 0.5-1.5)\n");
                        }
                    }
                    else if (strcmp(input_buffer, "sine") == 0)
                    {
                        current_waveform = WAVEFORM_SINE;
                        waveform_changed = true;
                        printf("Waveform: SINE (output on pins 0-7, calibration applied)\n");
                    }
                    else if (strcmp(input_buffer, "square") == 0)
                    {
                        current_waveform = WAVEFORM_SQUARE;
                        waveform_changed = true;
                        printf("Waveform: SQUARE (output on GPIO 9, PIO-generated)\n");
                    }
                    else if (input_buffer[0] == 'i')
                    {
                        printf("Current frequency: %d Hz\n", frequency_hz);
                        printf("Sample rate (SINE): %d Hz\n", SAMPLE_RATE_HZ);
                        printf("System clock: %d MHz\n", clock_get_hz(clk_sys) / 1000000);
                        printf("Waveform: %s\n", (current_waveform == WAVEFORM_SINE) ? "SINE (pins 0-7, calibrated)" : "SQUARE (GPIO 9, PIO)");
                        printf("Calibration factor: %.4f %s\n", calibration_factor,
                               (current_waveform == WAVEFORM_SINE) ? "(ACTIVE)" : "(INACTIVE)");
                        printf("Table size: %d samples\n", SINE_TABLE_SIZE);
                    }
                }

                buffer_pos = 0;
                printf("> ");
            }
            else if (buffer_pos < 63)
            {
                input_buffer[buffer_pos++] = c;
            }
        }

        sleep_ms(10);
    }
}