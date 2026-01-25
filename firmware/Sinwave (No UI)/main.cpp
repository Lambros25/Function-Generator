#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/spi.h"

#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19
#define N_SAMPLES 8
#define SPI_PORT  spi0

// Shared variables between cores
static uint16_t sine_lut[N_SAMPLES];
static volatile uint32_t delay_us = 100; 

// --- CORE 1: THE MUSICIAN ---
// This core ONLY runs the high-speed loop. No interruptions allowed.
void core1_entry() {
    uint8_t counter = 0;
    while (true) {
        gpio_put(PIN_CS, 0);
        spi_write16_blocking(SPI_PORT, &sine_lut[counter], 1);
        gpio_put(PIN_CS, 1);

        counter = (counter + 1) % N_SAMPLES;

        if (delay_us > 0) {
            sleep_us(delay_us);
        }
    }
}

// --- CORE 0: THE MANAGER ---
int main() {
    stdio_init_all();

    // Setup SPI
    spi_init(SPI_PORT, 20 * 1000 * 1000);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    // Prepare Sine Table
    for (int i = 0; i < N_SAMPLES; i++) {
        float val = (sinf((2.0f * M_PI * i) / N_SAMPLES) + 1.0f) * 0.5f;
        sine_lut[i] = 0x7000 | ((uint16_t)(val * 4095.0f) & 0x0FFF);
    }

    // Launch the Musician on Core 1
    multicore_launch_core1(core1_entry);

    printf("Pico Sine Gen Ready! Enter Frequency in Hz:\n");

    char buffer[32];
    int idx = 0;

    while (true) {
        int c = getchar(); // This is blocking, but it doesn't matter anymore!
        if (c == '\n' || c == '\r') {
            buffer[idx] = '\0';
            float target_f = atof(buffer);
            if (target_f > 0) {
                delay_us = (uint32_t)(1000000.0f / (target_f * N_SAMPLES));
                printf("Updated to %.2f Hz\n", target_f);
            }
            idx = 0;
        } else if (idx < 31) {
            buffer[idx++] = (char)c;
        }
    }
}