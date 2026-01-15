#include "pico/stdlib.h"
#include "hardware/pwm.h"

int main() {
    const uint LED_PIN = 15;

    // Set GPIO to PWM function
    gpio_set_function(LED_PIN, GPIO_FUNC_PWM);

    // Get PWM slice for this GPIO
    uint slice = pwm_gpio_to_slice_num(LED_PIN);

    // PWM config
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, 4.0f);   // slower PWM clock
    pwm_config_set_wrap(&cfg, 1000);       // 0..1000 duty steps

    pwm_init(slice, &cfg, true);

    while (true) {
        // Fade up
        for (int level = 0; level <= 1000; level++) {
            pwm_set_gpio_level(LED_PIN, level);
            sleep_ms(2);
        }
        // Fade down
        for (int level = 1000; level >= 0; level--) {
            pwm_set_gpio_level(LED_PIN, level);
            sleep_ms(2);
        }
    }
}
