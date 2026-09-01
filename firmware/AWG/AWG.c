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
// ADG1419 analog switch select. Set ADG_SQUARE_LEVEL to the logic level that
// routes the SQUARE path to the output (0 or 1 depending on wiring).
#define ADG_SELECT_PIN 10
#define ADG_SQUARE_LEVEL 1
#define OVERCLOCK_MHZ 225

// Full-scale sine amplitude keeps the R-2R ladder's inductor current under the
// 3mA design limit (docs/measurements.md). LOW mode further scales every sine
// table (SINE/NOISE/AM/FM all share these tables) to 1/4 amplitude - a
// user-toggleable, low-current mode ("<3mA" in the host UI), not just a fixed
// headroom margin. Runtime, not compile-time, so the UI can flip it live.
#define SINE_AMPLITUDE_SCALE_NORMAL 1.0f
#define SINE_AMPLITUDE_SCALE_LOW 0.25f
volatile float sine_amplitude_scale = SINE_AMPLITUDE_SCALE_NORMAL;
volatile bool low_amplitude_mode = false;
volatile bool amplitude_mode_changed = false;

// Ladder bit depth N is derived from the number of GPIO pins in the sine output
// path: DAC_PINS_BASE..DAC_PINS_BASE+7 = 8 pins drive the R-2R ladder. Full scale
// is 0..2^N-1, midscale is 2^(N-1).
#define DAC_BITS 8
#define DAC_FULL_SCALE ((1 << DAC_BITS) - 1)
#define DAC_MIDSCALE   (1 << (DAC_BITS - 1))

// Number of precomputed Gaussian noise samples. The runtime index is the top
// 16 bits of the xorshift32 output, so the noise sequence does not repeat until
// the LFSR rolls over (~2^32-1 samples).
#define NOISE_TABLE_SIZE 65536

// ===== WAVEFORM TYPES =====
typedef enum
{
    WAVEFORM_SINE,
    WAVEFORM_SQUARE,
    WAVEFORM_NOISE,
    WAVEFORM_AM,
    WAVEFORM_FM
} waveform_type_t;

// ===== GLOBALS =====
uint8_t sine_table[SINE_TABLE_SIZE];
int8_t raw_sine_table[SINE_TABLE_SIZE];
volatile uint32_t frequency_hz = 1000;
volatile bool frequency_changed = false;
volatile waveform_type_t current_waveform = WAVEFORM_SINE;
volatile bool waveform_changed = false;

// Modulation (AM/FM): carrier frequency is the existing "frequency_hz".
//  - mod_freq_hz: modulation oscillator frequency.
//  - mod_amount:  AM -> depth in percent (0..100); FM -> deviation in Hz.
volatile uint32_t mod_freq_hz = 1000;
volatile float mod_amount = 50.0f;
volatile bool mod_changed = false;

// SNR is 20*log10(V_signal_rms / V_noise_rms), measured over the full output
// bandwidth (DC to the reconstruction-filter corner ~1.5 MHz). SNR without a
// stated bandwidth is meaningless.
volatile float snr_db = 20.0f;
volatile bool snr_changed = false;

// NOISE-mode tables, owned exclusively by core 1:
//  - noise_sine_table[i] = round(A * sin(2*pi*i/256)), the sine component scaled
//    to peak amplitude A (codes). A is derived from the requested SNR.
//  - gauss_table[] holds unit-sigma Gaussian samples (std ~256 in fixed point)
//    built once from the xorshift32 LFSR via Irwin-Hall(12).
//  - noise_s_q = s * 256, the fixed-point scale that turns a unit-sigma draw
//    into a draw with sigma s (codes).
int8_t noise_sine_table[SINE_TABLE_SIZE];
int16_t gauss_table[NOISE_TABLE_SIZE];
int32_t noise_s_q = 0;

// ===== PIO PROGRAM FOR DAC (SINE) =====
const uint16_t dac_program_instructions[] = {
    0x6008, // out pins, 8
};

const struct pio_program dac_program = {
    .instructions = dac_program_instructions,
    .length = 1,
    .origin = -1,
};

// ===== PIO PROGRAM FOR SQUARE WAVE (asymmetric, autonomous) =====
// One program covers the whole frequency range and supports independent
// HIGH/LOW timing (needed for the duty-cycle correction below). The two
// half-period counts are pulled ONCE, at configure time, into the persistent
// ISR (high count) and Y (low count) registers; the wrap loop below then reads
// from those on every cycle with zero further FIFO/CPU involvement. That is
// the key property this program needs: the old CPU-fed counter method (one
// pio_sm_put_blocking per half-period) had core1's loop/scheduling jitter
// riding on every edge, which is fine at low frequency but would swamp the
// ~27ns correction this program exists to make. Running the SM undivided
// (clkdiv = 1.0) also gives single-sys-clock-cycle timing resolution.
//
// .wrap_target is the loop body; the two "pull"/"mov ..., osr" pairs above it
// are a one-time prologue that only runs once per pio_sm_init (program start).
//
//     pull   block         ; prologue: consumes the HIGH count word
//     mov    isr, osr       ;   stash it in ISR (persistent for the SM's life)
//     pull   block         ; prologue: consumes the LOW count word
//     mov    y, osr        ;   stash it in Y (persistent for the SM's life)
// .wrap_target
//     mov    x, isr        ; reload working counter = HIGH count
//     set    pins, 1
// high:
//     jmp    x--, high
//     mov    x, y          ; reload working counter = LOW count
//     set    pins, 0
// low:
//     jmp    x--, low
// .wrap
//
// HIGH duration = (high_count + 3) SM cycles, LOW duration = (low_count + 3)
// SM cycles (the "+3" is the fixed set/jmp/mov overhead each phase pays - see
// compute_square_counts()). Local instruction addresses (0-indexed within
// this program): 0=pull,1=mov isr,osr,2=pull,3=mov y,osr,4=mov x,isr (wrap
// target),5=set 1,6=jmp x--,7=mov x,y,8=set 0,9=jmp x-- (wrap).
#define SQUARE_ASYM_WRAP_TARGET 4
#define SQUARE_ASYM_WRAP 9

const uint16_t square_asym_instructions[] = {
    0x80a0, // pull   block
    0xa0c7, // mov    isr, osr
    0x80a0, // pull   block
    0xa047, // mov    y, osr
    0xa026, // mov    x, isr      (wrap_target)
    0xe001, // set    pins, 1
    0x0046, // jmp    x--, 6      (high:)
    0xa022, // mov    x, y
    0xe000, // set    pins, 0
    0x0049, // jmp    x--, 9      (low:) (wrap)
};

const struct pio_program square_asym_program = {
    .instructions = square_asym_instructions,
    .length = 10,
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
        uint8_t value = (uint8_t)(127.5f + 127.5f * sine_amplitude_scale * sinf(angle));
        sine_table[i] = reverse_bits(value);
    }
}

void generate_raw_sine_table()
{
    for (int i = 0; i < SINE_TABLE_SIZE; i++)
    {
        float angle = (2.0f * M_PI * i) / SINE_TABLE_SIZE;
        // Signed full-scale sine (approx sin*128) for AM/FM modulation math.
        raw_sine_table[i] = (int8_t)(127.0f * sine_amplitude_scale * sinf(angle));
    }
}

// ===== NOISE GENERATION (xorshift32 -> Irwin-Hall) =====
// Gaussian noise: xorshift32 LFSR -> Irwin-Hall(12), i.e. the sum of 12
// independent uniform 8-bit draws (three xorshift32 words, four bytes each).
// Each draw is uniform on [0,255]; the centred sum has std ~256. Irwin-Hall(12)
// is bounded at +/-6 sigma, so the resulting crest factor is ~6 (a true Gaussian
// has an unbounded crest factor). The 65536-entry gauss_table is generated once
// at startup and then sampled at a random index, which keeps the per-sample cost
// low (no per-sample CLT summation) while still producing a non-repeating stream.

static uint32_t xorshift_state = 0x9E3779B9u;

static uint32_t xorshift32(void)
{
    uint32_t x = xorshift_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    xorshift_state = x;
    return x;
}

static int32_t irwin_hall12(void)
{
    uint32_t a = xorshift32();
    uint32_t b = xorshift32();
    uint32_t c = xorshift32();
    int32_t sum = 0;
    sum += a & 0xFF; sum += (a >> 8) & 0xFF; sum += (a >> 16) & 0xFF; sum += a >> 24;
    sum += b & 0xFF; sum += (b >> 8) & 0xFF; sum += (b >> 16) & 0xFF; sum += b >> 24;
    sum += c & 0xFF; sum += (c >> 8) & 0xFF; sum += (c >> 16) & 0xFF; sum += c >> 24;
    return sum - 1530;
}

void generate_gauss_table()
{
    for (int i = 0; i < NOISE_TABLE_SIZE; i++)
    {
        gauss_table[i] = (int16_t)irwin_hall12();
    }
}

// Derive the sine amplitude A (peak, codes) and noise sigma s (codes) from SNR.
// Amplitude budget: A/sqrt(2) = s * 10^(SNR/20), and A + 4*s must fit under
// midscale (k = 4 clips ~0.006% of samples for a Gaussian; Irwin-Hall(12) rarely
// exceeds 4 sigma either). Solving gives s = 127/(sqrt(2)*10^(SNR/20) + 4) and
// A = sqrt(2) * s * 10^(SNR/20), so A is automatically scaled down as SNR drops
// and A + 4*s = 127 always. The final summed sample is clamped to [0, 2^N-1] in
// the hot loop so it can never wrap.
void recompute_noise_params()
{
    float r = powf(10.0f, snr_db / 20.0f);
    float headroom = (float)(DAC_MIDSCALE - 1);
    float s = headroom / (sqrtf(2.0f) * r + 4.0f);
    float a = sqrtf(2.0f) * r * s;

    noise_s_q = (int32_t)(s * 256.0f + 0.5f);

    for (int i = 0; i < SINE_TABLE_SIZE; i++)
    {
        float angle = (2.0f * M_PI * i) / SINE_TABLE_SIZE;
        float v = a * sine_amplitude_scale * sinf(angle);
        noise_sine_table[i] = (int8_t)(v >= 0.0f ? (v + 0.5f) : (v - 0.5f));
    }
}

// ===== MODULATION (AM/FM) PARAMETERS =====
// Recompute the modulation phase increment and the fixed-point depth/deviation
// from mod_freq_hz / mod_amount. Called on core 1 whenever modulation changes.
volatile uint32_t mod_phase_increment = 0;
volatile int32_t am_depth_q = 0;
volatile int32_t am_carrier_q = 128;
volatile int32_t fm_dev_q = 0;

void recompute_modulation()
{
    mod_phase_increment = ((uint64_t)mod_freq_hz * 4294967296ULL) / SAMPLE_RATE_HZ;

    // AM: mod_amount is depth in percent. am_depth_q = depth * 128 (Q7), 0..128.
    // To keep the envelope peaks on full scale (never clip), the carrier is
    // scaled to 1/(1+depth): am_carrier_q = 128/(1+depth) (Q7), 64..128. At 0%
    // depth the carrier is full scale; at 100% depth it is halved so the peak
    // (1+depth) = 2x envelope still lands exactly on the rail.
    float depth = mod_amount / 100.0f;
    if (depth > 1.0f) depth = 1.0f;
    if (depth < 0.0f) depth = 0.0f;
    am_depth_q = (int32_t)(depth * 128.0f + 0.5f);
    am_carrier_q = (int32_t)(128.0f / (1.0f + depth) + 0.5f);

    // FM: mod_amount is deviation in Hz. fm_dev_q is the peak phase-increment
    // change per sample, pre-shifted right by 7 so it can multiply sin_m (which is
    // sin*128) without overflowing 32 bits.
    float dev = mod_amount;
    if (dev < 0.0f) dev = 0.0f;
    fm_dev_q = (int32_t)((dev * 4294967296.0 / SAMPLE_RATE_HZ) / 128.0);
}

// ===== SQUARE-WAVE DUTY-CYCLE CORRECTION =====
// Base term: the TC4427 gate driver ahead of the ADG1419 has asymmetric
// propagation delay (t_D1 ~= 20ns typical turn-on, t_D2 ~= 40ns typical
// turn-off), which stretches every cycle's HIGH time by a fixed ~27ns as
// measured at the BNC. This alone fully explains (and corrects) the error up
// to ~100kHz.
#define SQUARE_DUTY_CORRECTION_NS 27.0f

// Above ~100kHz, a flat 27ns correction alone leaves a growing residual (a
// second, smaller fixed-time asymmetry - likely the ADG1419 itself and/or PCB
// parasitics - that the single-TC4427-constant model doesn't capture): a
// second BNC measurement pass with the base correction applied still showed
// 49.90%..52.60% from 300kHz to 2MHz instead of a flat 50%. Rather than
// modeling that second source, square_duty_cal_curve[] is a manually-measured
// total-correction-vs-frequency curve, fitted at the same points as
// docs/measurements.md's duty-cycle table, that replaces the flat constant
// above 100kHz. Below the lowest and above the highest point it holds flat;
// between points it's interpolated linearly in log10(frequency), since the
// points span decades. Re-measure and update this table if the analog
// front-end (driver, switch, PCB layout) changes.
typedef struct
{
    float freq_hz;
    float correction_ns; // total HIGH-time correction at this frequency (replaces SQUARE_DUTY_CORRECTION_NS, not added to it)
} duty_cal_point_t;

#define SQUARE_DUTY_CAL_POINTS 8
static const duty_cal_point_t square_duty_cal_curve[SQUARE_DUTY_CAL_POINTS] = {
    {    1000.0f, SQUARE_DUTY_CORRECTION_NS }, // 50.00% measured with the flat correction - on curve
    {   10000.0f, SQUARE_DUTY_CORRECTION_NS }, // 50.00%
    {  100000.0f, SQUARE_DUTY_CORRECTION_NS }, // 50.00%
    {  300000.0f,               23.7f       }, // 49.90% measured -> correction trimmed down ~3ns
    {  500000.0f,               39.0f       }, // 50.60% measured -> +12ns over the flat value
    {  700000.0f,               37.0f       }, // 50.70% measured -> +10ns
    { 1000000.0f,               37.0f       }, // 51.00% measured -> +10ns
    { 2000000.0f,               40.0f       }, // 52.60% measured -> +13ns
};

// Interpolate square_duty_cal_curve at freq_hz (log-frequency, flat outside
// the table's span). This is what compute_square_counts() actually uses; the
// bare SQUARE_DUTY_CORRECTION_NS constant above is just the curve's origin
// and its value at the low end where the flat model held exactly.
static float square_duty_correction_ns_for(uint32_t freq_hz)
{
    float f = (float)freq_hz;
    if (f <= square_duty_cal_curve[0].freq_hz)
    {
        return square_duty_cal_curve[0].correction_ns;
    }
    if (f >= square_duty_cal_curve[SQUARE_DUTY_CAL_POINTS - 1].freq_hz)
    {
        return square_duty_cal_curve[SQUARE_DUTY_CAL_POINTS - 1].correction_ns;
    }
    for (int i = 0; i < SQUARE_DUTY_CAL_POINTS - 1; i++)
    {
        float f0 = square_duty_cal_curve[i].freq_hz;
        float f1 = square_duty_cal_curve[i + 1].freq_hz;
        if (f <= f1)
        {
            float t = (log10f(f) - log10f(f0)) / (log10f(f1) - log10f(f0));
            float c0 = square_duty_cal_curve[i].correction_ns;
            float c1 = square_duty_cal_curve[i + 1].correction_ns;
            return c0 + t * (c1 - c0);
        }
    }
    return square_duty_cal_curve[SQUARE_DUTY_CAL_POINTS - 1].correction_ns; // unreachable
}

// Requested duty is not yet an exposed user command, but every computation
// below is written in terms of an arbitrary duty_percent, so adding one later
// (a "duty<pct>" command, say) is just wiring a variable through - no rework
// of the correction math or the PIO program.
volatile float square_duty_percent = 50.0f;
volatile bool duty_correction_enabled = true; // runtime on/off switch (req. 7)
volatile bool duty_changed = false;
// Last achieved duty, post-clamp, reported back to the host alongside the
// requested value (see main()'s "square"/"duty"/"i" responses).
volatile float square_achieved_duty_percent = 50.0f;

// Fixed per-phase overhead of square_asym_program, in SM cycles: the "set"
// instruction plus the "jmp x--" loop's minimum one pass (count=0 still
// executes once) plus the following phase's "mov" reload, which still shows
// the current pin state. See the program's comment block for the full trace.
#define SQUARE_MIN_PHASE_CYCLES 3u

// Compute the (high_count, low_count) to load into square_asym_program for a
// given frequency/duty, with the TC4427 correction applied, and report what
// duty is actually achieved after integer-cycle rounding and edge clamping.
static void compute_square_counts(uint32_t freq_hz, float duty_percent, bool correction_enabled,
                                  uint32_t *out_high_count, uint32_t *out_low_count,
                                  volatile float *out_achieved_duty_percent)
{
    uint32_t sys_hz = clock_get_hz(clk_sys); // actual (possibly overclocked) sys clock, not a hardcoded 125MHz
    uint64_t period_cycles = ((uint64_t)sys_hz + freq_hz / 2) / freq_hz; // round to nearest
    if (period_cycles < 2 * SQUARE_MIN_PHASE_CYCLES)
    {
        period_cycles = 2 * SQUARE_MIN_PHASE_CYCLES; // clamp: fastest this program can produce
    }

    float duty = duty_percent / 100.0f;
    if (duty < 0.0f) duty = 0.0f;
    if (duty > 1.0f) duty = 1.0f;

    // Total loop-count budget once both phases' fixed overhead is paid.
    int64_t count_budget = (int64_t)period_cycles - 2 * (int64_t)SQUARE_MIN_PHASE_CYCLES;
    int64_t ideal_high_count = lroundf((float)count_budget * duty);

    int64_t correction_cycles = 0;
    if (correction_enabled)
    {
        // ns -> cycles at the ACTUAL sys clock, rounded to nearest. Residual:
        // up to +/-0.5 cycle is dropped by the rounding (~2.2ns at 225MHz,
        // ~4ns at 125MHz) - see the calibration note for the measured effect.
        float correction_ns = square_duty_correction_ns_for(freq_hz);
        correction_cycles = lroundf(correction_ns * (float)sys_hz / 1.0e9f);
    }

    // Subtract the correction from HIGH (add it to LOW) but never let either
    // side go negative or exceed the budget - if the correction doesn't fit
    // (very short periods at very high frequency), apply as much as fits
    // rather than silently dropping it; count_budget is preserved exactly
    // either way, so frequency is never distorted by this clamp.
    int64_t high_count = ideal_high_count - correction_cycles;
    if (high_count < 0) high_count = 0;
    if (high_count > count_budget) high_count = count_budget;
    int64_t low_count = count_budget - high_count;

    *out_high_count = (uint32_t)high_count;
    *out_low_count = (uint32_t)low_count;

    uint32_t high_cycles = *out_high_count + SQUARE_MIN_PHASE_CYCLES;
    uint32_t total_cycles = high_cycles + *out_low_count + SQUARE_MIN_PHASE_CYCLES;
    *out_achieved_duty_percent = 100.0f * (float)high_cycles / (float)total_cycles;
}

// ===== CORE 1: WAVEFORM GENERATION =====
// (Re)configure the square-wave SM with a fresh HIGH/LOW count pair. Always
// leaves the SM disabled (matches pio_sm_init's own behavior) - callers that
// need square actively running must re-enable it afterward.
static void configure_square_sm(PIO pio, uint sm, uint offset,
                                uint32_t high_count, uint32_t low_count)
{
    pio_sm_set_enabled(pio, sm, false);
    pio_sm_clear_fifos(pio, sm);

    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_set_pins(&c, SQUARE_OUT_PIN, 1);
    sm_config_set_wrap(&c, offset + SQUARE_ASYM_WRAP_TARGET, offset + SQUARE_ASYM_WRAP);
    sm_config_set_clkdiv(&c, 1.0f); // full sys clock: counts give single-cycle resolution
    pio_sm_init(pio, sm, offset, &c);

    // Prime the persistent HIGH/LOW counts. The program's one-time prologue
    // consumes exactly these two words into ISR/Y and then never pulls again,
    // so this must happen before pio_sm_set_enabled() and needs no further
    // feeding for the lifetime of this configuration.
    pio_sm_put_blocking(pio, sm, high_count);
    pio_sm_put_blocking(pio, sm, low_count);
}

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

    // Setup PIO0 SM1 for square wave: one autonomous asymmetric-duty program
    // (square_asym_program) drives the whole frequency range - see its
    // header comment for why that matters to the duty-cycle correction.
    PIO pio_square = pio0;
    uint sm_square = 1;

    uint offset_square = pio_add_program(pio_square, &square_asym_program);

    pio_gpio_init(pio_square, SQUARE_OUT_PIN);
    pio_sm_set_consecutive_pindirs(pio_square, sm_square, SQUARE_OUT_PIN, 1, true);

    uint32_t square_high_count, square_low_count;
    compute_square_counts(frequency_hz, square_duty_percent, duty_correction_enabled,
                          &square_high_count, &square_low_count, &square_achieved_duty_percent);
    configure_square_sm(pio_square, sm_square, offset_square, square_high_count, square_low_count);
    // Don't enable square wave PIO yet - only when user selects it

    // ADG1419 select: route the sine path by default.
    gpio_init(ADG_SELECT_PIN);
    gpio_set_dir(ADG_SELECT_PIN, GPIO_OUT);
    gpio_put(ADG_SELECT_PIN, ADG_SQUARE_LEVEL ? 0 : 1);

    uint32_t phase_accumulator = 0;
    uint32_t mod_phase = 0;
    uint32_t fm_phase = 0;
    uint32_t local_frequency = frequency_hz;
    float local_calibration = calibration_factor;
    waveform_type_t local_waveform = current_waveform;

    uint32_t sine_phase_increment = ((uint64_t)local_frequency * 4294967296ULL) / (uint32_t)(SAMPLE_RATE_HZ * local_calibration);

    generate_gauss_table();
    recompute_noise_params();
    recompute_modulation();

    while (true)
    {
        if (frequency_changed)
        {
            local_frequency = frequency_hz;
            local_calibration = calibration_factor;

            // Update sine phase increment
            sine_phase_increment = ((uint64_t)local_frequency * 4294967296ULL) / (uint32_t)(SAMPLE_RATE_HZ * local_calibration);

            // Update square wave: recompute HIGH/LOW counts for the new
            // frequency (duty and correction unchanged) and reload the SM.
            // configure_square_sm() (via pio_sm_init) always leaves the SM
            // disabled; re-enable it if a square wave is actively playing.
            compute_square_counts(local_frequency, square_duty_percent, duty_correction_enabled,
                                  &square_high_count, &square_low_count, &square_achieved_duty_percent);
            configure_square_sm(pio_square, sm_square, offset_square, square_high_count, square_low_count);
            if (local_waveform == WAVEFORM_SQUARE)
            {
                pio_sm_set_enabled(pio_square, sm_square, true);
            }

            frequency_changed = false;
        }

        if (waveform_changed)
        {
            local_waveform = current_waveform;

            if (local_waveform == WAVEFORM_SQUARE)
            {
                // Route square path and enable square wave PIO
                gpio_put(ADG_SELECT_PIN, ADG_SQUARE_LEVEL);
                pio_sm_set_enabled(pio_square, sm_square, true);
            }
            else
            {
                // Route sine path, disable square wave PIO, set pin low
                gpio_put(ADG_SELECT_PIN, ADG_SQUARE_LEVEL ? 0 : 1);
                pio_sm_set_enabled(pio_square, sm_square, false);
                gpio_init(SQUARE_OUT_PIN);
                gpio_set_dir(SQUARE_OUT_PIN, GPIO_OUT);
                gpio_put(SQUARE_OUT_PIN, 0);
                // Re-init for PIO control
                pio_gpio_init(pio_square, SQUARE_OUT_PIN);
            }

            waveform_changed = false;
            phase_accumulator = 0;
            mod_phase = 0;
            fm_phase = 0;
        }

        if (snr_changed)
        {
            recompute_noise_params();
            snr_changed = false;
        }

        if (mod_changed)
        {
            recompute_modulation();
            mod_changed = false;
        }

        if (duty_changed)
        {
            // Requested duty and/or the correction on/off switch changed;
            // frequency is unchanged, so just recompute and reload counts.
            compute_square_counts(local_frequency, square_duty_percent, duty_correction_enabled,
                                  &square_high_count, &square_low_count, &square_achieved_duty_percent);
            configure_square_sm(pio_square, sm_square, offset_square, square_high_count, square_low_count);
            if (local_waveform == WAVEFORM_SQUARE)
            {
                pio_sm_set_enabled(pio_square, sm_square, true);
            }
            duty_changed = false;
        }

        if (amplitude_mode_changed)
        {
            // Regenerate every sine-derived table (SINE/NOISE/AM/FM share
            // them) at the new amplitude scale. Safe without locking: core1
            // is the sole writer AND reader of these tables.
            sine_amplitude_scale = low_amplitude_mode ? SINE_AMPLITUDE_SCALE_LOW : SINE_AMPLITUDE_SCALE_NORMAL;
            generate_sine_table();
            generate_raw_sine_table();
            recompute_noise_params();
            amplitude_mode_changed = false;
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
        else if (local_waveform == WAVEFORM_NOISE)
        {
            // --- NOISE MODE: sine + Gaussian noise, clamped, same sine path ---
            for (int i = 0; i < 32; i++)
            {
                uint8_t table_index = phase_accumulator >> 24;
                int32_t g = gauss_table[xorshift32() >> 16];
                int32_t noise = (g * noise_s_q + 0x8000) >> 16;
                int32_t sample = DAC_MIDSCALE + noise_sine_table[table_index] + noise;
                if (sample < 0)
                {
                    sample = 0;
                }
                else if (sample > DAC_FULL_SCALE)
                {
                    sample = DAC_FULL_SCALE;
                }
                pio_sm_put_blocking(pio_dac, sm_dac, reverse_bits((uint8_t)sample));
                phase_accumulator += sine_phase_increment;
            }
        }
        else if (local_waveform == WAVEFORM_AM)
        {
            // --- AM MODE: carrier * (1 + depth * sin(mod)), anti-clipped ---
            for (int i = 0; i < 32; i++)
            {
                uint8_t ci = phase_accumulator >> 24;
                uint8_t mi = mod_phase >> 24;
                int32_t sin_c = raw_sine_table[ci];
                int32_t sin_m = raw_sine_table[mi];
                int32_t env = 128 + ((am_depth_q * sin_m) >> 7);
                int32_t sample = DAC_MIDSCALE + ((sin_c * am_carrier_q * env + 8192) >> 14);
                if (sample < 0)
                {
                    sample = 0;
                }
                else if (sample > DAC_FULL_SCALE)
                {
                    sample = DAC_FULL_SCALE;
                }
                pio_sm_put_blocking(pio_dac, sm_dac, reverse_bits((uint8_t)sample));
                phase_accumulator += sine_phase_increment;
                mod_phase += mod_phase_increment;
            }
        }
        else if (local_waveform == WAVEFORM_FM)
        {
            // --- FM MODE: carrier phase advanced by base + deviation*sin(mod) ---
            for (int i = 0; i < 32; i++)
            {
                uint8_t mi = mod_phase >> 24;
                int32_t sin_m = raw_sine_table[mi];
                uint32_t step = sine_phase_increment + (uint32_t)(fm_dev_q * sin_m);
                fm_phase += step;
                uint8_t ci = fm_phase >> 24;
                int32_t sin_c = raw_sine_table[ci];
                int32_t sample = DAC_MIDSCALE + sin_c;
                if (sample < 0)
                {
                    sample = 0;
                }
                else if (sample > DAC_FULL_SCALE)
                {
                    sample = DAC_FULL_SCALE;
                }
                pio_sm_put_blocking(pio_dac, sm_dac, reverse_bits((uint8_t)sample));
                mod_phase += mod_phase_increment;
            }
        }
        else
        {
            // --- SQUARE MODE: square_asym_program runs autonomously once
            // configured (see its header comment) - nothing to feed here.
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
    printf("  c<number> - Set calibration factor (e.g., c1.06, SINE/NOISE only)\n");
    printf("  sine      - Switch to sine wave (output: pins 0-7 R-2R DAC)\n");
    printf("  square    - Switch to square wave (output: GPIO 9, PIO-generated)\n");
    printf("  noise     - Switch to sine + Gaussian noise (output: pins 0-7)\n");
    printf("  snr<db>   - Set noise SNR in dB, 0.0-60.0 (e.g., snr20)\n");
    printf("  am        - Switch to amplitude modulation (carrier = f)\n");
    printf("  fm        - Switch to frequency modulation (carrier = f)\n");
    printf("  m<number> - Set modulation frequency in Hz (AM/FM)\n");
    printf("  d<number> - Set modulation: AM depth %% (0-100), FM deviation Hz\n");
    printf("  duty<pct> - Set square duty cycle %% (1-99, e.g. duty30)\n");
    printf("  corr<0/1> - Enable/disable TC4427 duty-cycle correction (SQUARE only)\n");
    printf("  amp<0/1>  - Low-current (<3mA) 1/4-amplitude mode, SINE/NOISE/AM/FM\n");
    printf("  i         - Show current info\n\n");

    generate_sine_table();
    generate_raw_sine_table();

    multicore_launch_core1(core1_entry);

    printf("Generator running at %d Hz\n", frequency_hz);
    printf("Waveform: SINE\n");
    printf("SNR: %.1f dB\n", snr_db);
    printf("Calibration factor: %.4f (applies to SINE/NOISE only)\n", calibration_factor);

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
                    if (strcmp(input_buffer, "sine") == 0)
                    {
                        current_waveform = WAVEFORM_SINE;
                        waveform_changed = true;
                        printf("Waveform: SINE (output on pins 0-7, calibration applied)\n");
                    }
                    else if (strcmp(input_buffer, "square") == 0)
                    {
                        current_waveform = WAVEFORM_SQUARE;
                        waveform_changed = true;
                        printf("Waveform: SQUARE (output on GPIO 9, PIO-generated, duty target %.2f%%, achieved %.2f%%, correction %s)\n",
                               square_duty_percent, square_achieved_duty_percent,
                               duty_correction_enabled ? "ON" : "OFF");
                    }
                    else if (strcmp(input_buffer, "noise") == 0)
                    {
                        current_waveform = WAVEFORM_NOISE;
                        waveform_changed = true;
                        printf("Waveform: NOISE (sine + Gaussian noise, SNR %.1f dB, output on pins 0-7)\n", snr_db);
                    }
                    else if (strcmp(input_buffer, "am") == 0)
                    {
                        current_waveform = WAVEFORM_AM;
                        waveform_changed = true;
                        printf("Waveform: AM (carrier %d Hz, mod %d Hz, depth %.1f%%)\n", frequency_hz, mod_freq_hz, mod_amount);
                    }
                    else if (strcmp(input_buffer, "fm") == 0)
                    {
                        current_waveform = WAVEFORM_FM;
                        waveform_changed = true;
                        printf("Waveform: FM (carrier %d Hz, mod %d Hz, deviation %.1f Hz)\n", frequency_hz, mod_freq_hz, mod_amount);
                    }
                    else if (input_buffer[0] == 'f')
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
                    else if (strncmp(input_buffer, "corr", 4) == 0 && strlen(input_buffer) > 4)
                    {
                        // Checked ahead of the single-'c' calibration branch below,
                        // since "corr0"/"corr1" would otherwise match it first.
                        int new_corr = atoi(&input_buffer[4]);
                        duty_correction_enabled = (new_corr != 0);
                        duty_changed = true;
                        printf("Duty-cycle correction (TC4427, SQUARE only): %s\n",
                               duty_correction_enabled ? "ON" : "OFF");
                    }
                    else if (input_buffer[0] == 'c')
                    {
                        float new_cal = atof(&input_buffer[1]);
                        if (new_cal > 0.5 && new_cal < 1.5)
                        {
                            calibration_factor = new_cal;
                            frequency_changed = true;
                            printf("Calibration factor set to %.4f (SINE/NOISE mode only)\n", new_cal);
                        }
                        else
                        {
                            printf("Invalid calibration (range: 0.5-1.5)\n");
                        }
                    }
                    else if (strncmp(input_buffer, "snr", 3) == 0 && strlen(input_buffer) > 3)
                    {
                        float new_snr = atof(&input_buffer[3]);
                        if (new_snr >= 0.0f && new_snr <= 60.0f)
                        {
                            snr_db = new_snr;
                            snr_changed = true;
                            printf("SNR set to %.1f dB\n", new_snr);
                        }
                        else
                        {
                            printf("Invalid SNR (range: 0.0-60.0 dB)\n");
                        }
                    }
                    else if (input_buffer[0] == 'm' && strlen(input_buffer) > 1)
                    {
                        uint32_t new_mod = atoi(&input_buffer[1]);
                        if (new_mod > 0 && new_mod <= 1000000)
                        {
                            mod_freq_hz = new_mod;
                            mod_changed = true;
                            printf("Modulation frequency set to %d Hz\n", new_mod);
                        }
                        else
                        {
                            printf("Invalid modulation frequency (range: 1-1000000 Hz)\n");
                        }
                    }
                    else if (strncmp(input_buffer, "duty", 4) == 0 && strlen(input_buffer) > 4)
                    {
                        // Checked ahead of the single-'d' mod-amount branch below,
                        // since "duty30" would otherwise match it first.
                        float new_duty = atof(&input_buffer[4]);
                        if (new_duty >= 1.0f && new_duty <= 99.0f)
                        {
                            square_duty_percent = new_duty;
                            duty_changed = true;
                            printf("Square duty target set to %.2f%% (SQUARE only)\n", new_duty);
                        }
                        else
                        {
                            printf("Invalid duty (range: 1.0-99.0 %%)\n");
                        }
                    }
                    else if (strncmp(input_buffer, "amp", 3) == 0 && strlen(input_buffer) > 3)
                    {
                        int new_amp = atoi(&input_buffer[3]);
                        low_amplitude_mode = (new_amp != 0);
                        amplitude_mode_changed = true;
                        printf("Amplitude mode (SINE/NOISE/AM/FM): %s\n",
                               low_amplitude_mode ? "LOW (<3mA, 1/4 scale)" : "NORMAL");
                    }
                    else if (input_buffer[0] == 'd' && strlen(input_buffer) > 1)
                    {
                        float new_amount = atof(&input_buffer[1]);
                        if (new_amount >= 0.0f && new_amount <= 100000.0f)
                        {
                            mod_amount = new_amount;
                            mod_changed = true;
                            printf("Modulation amount set to %.1f\n", new_amount);
                        }
                        else
                        {
                            printf("Invalid modulation amount (range: 0-100000)\n");
                        }
                    }
                    else if (input_buffer[0] == 'i')
                    {
                        const char *mode_str;
                        if (current_waveform == WAVEFORM_SINE)
                        {
                            mode_str = "SINE (pins 0-7, calibrated)";
                        }
                        else if (current_waveform == WAVEFORM_SQUARE)
                        {
                            mode_str = "SQUARE (GPIO 9, PIO)";
                        }
                        else if (current_waveform == WAVEFORM_NOISE)
                        {
                            mode_str = "NOISE (pins 0-7)";
                        }
                        else if (current_waveform == WAVEFORM_AM)
                        {
                            mode_str = "AM (pins 0-7)";
                        }
                        else
                        {
                            mode_str = "FM (pins 0-7)";
                        }
                        printf("Current frequency: %d Hz\n", frequency_hz);
                        printf("Sample rate (SINE): %d Hz\n", SAMPLE_RATE_HZ);
                        printf("System clock: %d MHz\n", clock_get_hz(clk_sys) / 1000000);
                        printf("Waveform: %s\n", mode_str);
                        printf("Mod frequency: %d Hz\n", mod_freq_hz);
                        printf("Mod amount: %.1f%s\n", mod_amount,
                               (current_waveform == WAVEFORM_AM) ? "% depth" :
                               (current_waveform == WAVEFORM_FM) ? " Hz deviation" : "");
                        printf("SNR: %.1f dB%s\n", snr_db, (current_waveform == WAVEFORM_NOISE) ? " (NOISE active)" : "");
                        printf("Calibration factor: %.4f %s\n", calibration_factor,
                               (current_waveform == WAVEFORM_SQUARE) ? "(INACTIVE)" : "(ACTIVE)");
                        printf("Square duty: target %.2f%%, achieved %.2f%%, correction %s %s\n",
                               square_duty_percent, square_achieved_duty_percent,
                               duty_correction_enabled ? "ON" : "OFF",
                               (current_waveform == WAVEFORM_SQUARE) ? "(ACTIVE)" : "(INACTIVE)");
                        printf("Amplitude mode: %s %s\n",
                               low_amplitude_mode ? "LOW (<3mA, 1/4 scale)" : "NORMAL",
                               (current_waveform == WAVEFORM_SQUARE) ? "(INACTIVE)" : "(ACTIVE)");
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