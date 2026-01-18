#include <math.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "pico/time.h"

#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19

#define N_SAMPLES 8
#define SINE_HZ   10000.0f

static uint16_t sine_lut[N_SAMPLES];
static volatile uint32_t idx = 0;

static inline void dac_write_12bit(uint16_t value) {
    value &= 0x0FFF;

    // MCP4921 control bits (typical):
    // bit 15: 0 = DAC A
    // bit 14: 1 = buffered (often fine either way)
    // bit 13: 1 = gain 1x
    // bit 12: 1 = active
    uint16_t cmd = (0 << 15) | (1 << 14) | (1 << 13) | (1 << 12) | value;

    uint8_t buf[2] = { (uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xFF) };

    gpio_put(PIN_CS, 0);
    spi_write_blocking(spi0, buf, 2);
    gpio_put(PIN_CS, 1);
}

static bool timer_cb(struct repeating_timer *t) {
    dac_write_12bit(sine_lut[idx]);
    idx = (idx + 1) & (N_SAMPLES - 1); // N_SAMPLES must be power of 2
    return true;
}

int main() {
    stdio_init_all();

    // SPI @ 5 MHz (safe, plenty fast for audio-ish updates)
    spi_init(spi0, 5 * 1000 * 1000);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    // Build sine LUT: full-scale 0..4095
    for (int i = 0; i < N_SAMPLES; i++) {
        float s = sinf(2.0f * (float)M_PI * (float)i / (float)N_SAMPLES); // -1..+1
        float u = (s + 1.0f) * 0.5f;                                     // 0..1
        sine_lut[i] = (uint16_t)(u * 4095.0f);
    }

    // Timer period (us) for update rate = N_SAMPLES * SINE_HZ
    float update_hz = N_SAMPLES * SINE_HZ;                 // 51200 Hz
    int64_t period_us = (int64_t)(1000000.0f / update_hz); // ~19 us

    struct repeating_timer timer;
    add_repeating_timer_us(-period_us, timer_cb, NULL, &timer);

    while (true) {
        tight_loop_contents();
    }
}
