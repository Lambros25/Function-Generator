#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "pico/time.h"

#define LED_PIN 15

// Pick your sine settings
#define N_SAMPLES 256
#define SINE_HZ   2000.0f
#define PWM_HZ    100000.0f

static uint16_t sine_lut[N_SAMPLES];
static volatile uint32_t idx = 0;
static uint slice;
static uint16_t wrap_val;

static bool timer_cb(struct repeating_timer *t) {
    pwm_set_gpio_level(LED_PIN, sine_lut[idx]);
    idx = (idx + 1) & (N_SAMPLES - 1); // works because 256 is power of 2
    return true;
}

int main() {
    stdio_init_all();

    // --- PWM setup for ~100 kHz ---
    gpio_set_function(LED_PIN, GPIO_FUNC_PWM);
    slice = pwm_gpio_to_slice_num(LED_PIN);

    // Compute wrap for desired PWM_HZ with clkdiv=1
    // f_pwm = f_sys / (clkdiv * (wrap+1))
    uint32_t f_sys = clock_get_hz(clk_sys); // usually 125 MHz
    wrap_val = (uint16_t)((f_sys / PWM_HZ) - 1); // for 125MHz & 100kHz -> 1249

    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, 1.0f);
    pwm_config_set_wrap(&cfg, wrap_val);
    pwm_init(slice, &cfg, true);

    // --- Build sine lookup table scaled to wrap ---
    for (int i = 0; i < N_SAMPLES; i++) {
        float s = sinf(2.0f * (float)M_PI * (float)i / (float)N_SAMPLES); // -1..+1
        float u = (s + 1.0f) * 0.5f; // 0..1
        sine_lut[i] = (uint16_t)(u * (float)wrap_val); // 0..wrap
    }

    // --- Fixed update rate: N_SAMPLES * SINE_HZ ---
    float update_hz = N_SAMPLES * SINE_HZ;         // 25600 Hz
    int64_t period_us = (int64_t)(1000000.0f / update_hz); // ~39 us

    struct repeating_timer timer;
    add_repeating_timer_us(-period_us, timer_cb, NULL, &timer);

    while (true) {
        tight_loop_contents();
    }
}
