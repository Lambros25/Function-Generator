# Firmware

Raspberry Pi Pico W. Everything the instrument does that isn't a resistor is here: the
sine is a lookup table clocked into an R-2R ladder, the square is a PIO-generated GPIO
toggle, and the host UI is the only user interface.

This document is a description of `firmware/AWG/AWG.c`, not a design intent — every
figure below is read off the code or the build files, not assumed. Re-verify it after
any firmware change; a stale firmware.md is worse than none.

---

## Target and toolchain
- Raspberry Pi Pico W (RP2040), soldered directly to the v1.2 board — not socketed, so
  reflashing is over USB only.
- Language/SDK: C (C11), built against the **Raspberry Pi Pico SDK 2.2.0** via CMake
  (`firmware/AWG/CMakeLists.txt`). Not MicroPython — `project(AWG C CXX ASM)` links
  `pico_stdlib`, `pico_multicore`, and the `hardware_pio`/`hardware_clocks` etc. SDK
  libraries directly. `hardware_spi`, `hardware_i2c`, `hardware_dma`, `hardware_interp`,
  `hardware_timer` and `hardware_watchdog` are linked but unused by `AWG.c` as it stands
  — worth pruning if the build is ever slimmed down.
- Developed in VS Code. A standalone host application instead of running from the editor
  is listed under `future-work.md`.
- **Build and flash procedure** (as actually run to build/flash this firmware):
  1. Toolchain lives under `~/.pico-sdk/` (installed by the Pico VS Code extension):
     SDK 2.2.0, ARM toolchain `14_2_Rel1`, CMake `v3.31.5`, Ninja `v1.12.1`, picotool
     `2.2.0-a4`. `firmware/AWG/.vscode/settings.json` has the exact paths.
  2. Set `PICO_SDK_PATH` / `PICO_TOOLCHAIN_PATH` and prepend the toolchain/cmake/ninja/
     picotool `bin` dirs to `PATH`, then run `ninja -C firmware/AWG/build` (the CMake
     configure step has already been run once into `firmware/AWG/build/`; re-run
     `cmake -G Ninja ..` there if `CMakeLists.txt` changes). Output: `AWG.uf2` in that
     `build/` directory.
  3. Flash: put the Pico in BOOTSEL mode — either hold BOOTSEL while plugging in USB, or,
     if it's already running firmware with USB CDC enabled, force it with
     `picotool reboot -f -u`. It re-enumerates as a `RPI-RP2` USB mass-storage drive;
     copy `AWG.uf2` onto it. The Pico reboots into the new firmware automatically and the
     drive disappears — that disappearance is the confirmation the flash took.

## Pin map
All of it is `#define`d at the top of `AWG.c`; nothing else in this firmware drives a
GPIO.

| Signal | GPIO(s) | Notes |
|---|---|---|
| R-2R ladder (sine DAC) | GPIO0–GPIO7 | `DAC_PINS_BASE = 0`, 8 consecutive pins, driven by PIO0 SM0. Bit order: `generate_sine_table()`/`generate_raw_sine_table()`/`recompute_noise_params()` write each sample through `reverse_bits()` before it reaches the FIFO, and the PIO's `out pins, 8` (`sm_config_set_out_shift(shift_right=true, threshold=8)`) then places OSR bit *i* on GPIO*i* unshifted (a single 8-bit `out` moves the whole OSR at once, so shift direction doesn't reorder it). Net effect: **GPIO0 = MSB, GPIO7 = LSB** of the logical sample value. This is derived from the code, not cross-checked against the Altium net names — but the sine output measures correctly (`measurements.md`), so the code/hardware pairing is self-consistent in practice. |
| Square wave out | GPIO9 | `SQUARE_OUT_PIN`. PIO0 SM1, into the TC4427. |
| ADG1419 select | GPIO10 | `ADG_SELECT_PIN`. Logic level 1 routes SQUARE, 0 routes SINE (`ADG_SQUARE_LEVEL = 1`). |
| LEDs / test outputs | none | No other GPIO is touched by this firmware. |
| UART | none | `pico_enable_stdio_uart(AWG 0)` — UART stdio is disabled; host comms are USB CDC only. |

---

## Sine generation

The ladder is fed from a precomputed lookup table. Output frequency is set by how fast
the table is clocked out, not by the table contents.

- **Table depth / width:** `SINE_TABLE_SIZE = 256` samples per cycle. There are three
  separate 256-entry, 8-bit tables, all regenerated together whenever amplitude mode or
  SNR changes: `sine_table` (unsigned, bit-reversed, feeds the DAC directly in SINE/NOISE
  mode), `raw_sine_table` (signed, `~sin*128`, feeds the AM/FM math), and
  `noise_sine_table` (signed, scaled to the SNR-derived amplitude, feeds NOISE mode).
- **Timing mechanism: PIO, CPU-fed — not DMA, not a timer ISR.** `dac_program` is a
  1-instruction PIO0 SM0 program (`out pins, 8`) clocked by a fractional divider set from
  `clock_get_hz(clk_sys) / SAMPLE_RATE_HZ`, so the PIO alone sets the sample *rate*.
  core1 is the one pushing sample *values*: a tight loop calls `pio_sm_put_blocking()`
  once per sample (32 per outer iteration), which blocks on the PIO's TX FIFO and so
  paces core1 to the DAC's actual output rate. `hardware_dma` is linked in
  `CMakeLists.txt` but no DMA channel is used anywhere in `AWG.c` — dead link, not a
  hidden data path.
- **Maximum update rate achieved:** `SAMPLE_RATE_HZ = 4034843` (~4.03MS/s), fixed at
  compile time, shared by SINE/NOISE/AM/FM (SQUARE has its own independent PIO SM and no
  such fixed rate — see below). At 1MHz output that is **4034843 / 1000000 ≈ 4.03
  samples/cycle** — genuinely few, and a real, separate contributor to any droop at the
  top of the sine range, on top of the inductor saturation in `lessons.md` #4. It also
  fixes the DDS's Nyquist limit at `SAMPLE_RATE_HZ / 2 ≈ 2.017MHz`: any commanded
  frequency above that aliases rather than attenuates. This is exactly the mechanism
  behind `measurements.md`'s open item "the earlier '2MHz sine' reading was an alias."
- **Frequency resolution: phase-accumulator, i.e. this genuinely is a DDS,** not a
  fixed-table player or an integer-divider scheme — for SINE/NOISE/AM/FM.
  `sine_phase_increment = (freq_hz * 2^32) / (SAMPLE_RATE_HZ * calibration_factor)` is a
  32-bit accumulator advanced every sample; the table index is its top 8 bits. Step size
  is `SAMPLE_RATE_HZ / 2^32 ≈ 9.39e-4 Hz` (<1mHz) — far below any other error source, so
  effectively continuous. **SQUARE is different and NOT phase-accumulator based** — see
  "Square generation" below; it has its own, much coarser, frequency quantization.
- The command validator (`f<number>`) accepts up to 50,000,000 Hz for *every* waveform,
  including the DDS ones, with no Nyquist check. Commanding a SINE/NOISE/AM/FM frequency
  above ~2.017MHz will alias silently rather than error or clamp — worth fixing, not
  fixed here since it's outside this pass's scope.

### Effective resolution
The ladder is ~7 effective bits worst-case, limited by 1% resistor matching rather than
by firmware (`limitations.md`). Firmware cannot recover this; only 0.1% thin-film parts
can.

---

## Low-current mode

A runtime-toggleable mode that scales every sine-derived table (`sine_table`,
`raw_sine_table`, `noise_sine_table` — so SINE, NOISE, AM and FM all share it; SQUARE is
unaffected) to 1/4 amplitude before output.

- Purpose: peak current through the filter inductors drops from ~15mA to ~3.75mA,
  bringing the ferrite out of saturation. Measured effect is decisive — the −3.1dB droop
  at 1MHz disappears entirely and the passband goes flat to −0.09dB, with no hardware
  change. This is the experiment that identified the fault.
- Cost: 4× less output amplitude, and two bits of an already ~7-bit-effective ladder,
  leaving ~5 effective bits.
- **Status: this is no longer purely a one-off diagnostic.** It started as a compile-time
  hack used once to isolate the saturation fault; it is now a `volatile` runtime state
  (`low_amplitude_mode`, regenerated live via `amplitude_mode_changed`), toggleable at any
  time over serial (`amp<0/1>`) or from the host UI's `<3mA` button — a standing,
  user-facing low-current output option, not just a debug-only measurement. The fix for
  the underlying saturation is still a higher-saturation-current inductor; this mode
  works around it rather than fixing it.
- **Naming:** exposed as `"<3mA"` in both firmware (`amp<0/1>` response text) and the UI
  button. It does not reach 3mA — it reaches ~3.75mA, which is still above the part's
  rating and yet measurably linear. Suggest renaming to "low-current mode (÷4)" so the
  label doesn't assert something the measurement doesn't support. Not renamed as part of
  this pass.

---

## Square generation

Pico GPIO straight into the TC4427. No op-amp anywhere in this path, which is why edges
measure ~20ns and why square reaches 2MHz while sine stops around 1MHz.

- **Generation method: PIO** (`square_asym_program`, PIO0 SM1) — not a PWM slice, not a
  timer. It runs autonomously once configured: the commanded HIGH/LOW cycle counts are
  pulled into the PIO's persistent ISR/Y registers exactly once, and the wrap loop then
  toggles the pin forever with zero further CPU or FIFO involvement, at the raw
  (undivided) system clock. Frequency is therefore quantized in whole PIO cycles, not a
  phase accumulator — resolution is `1/sys_clk` of period, i.e. coarser in relative terms
  the higher the frequency (≈4.4ns steps at 225MHz, ~0.44% of a 1MHz period, ~0.9% of a
  2MHz period).
- **Duty cycle IS user-settable** (`duty<pct>`, 1.0–99.0%, default 50%) — not fixed. A
  correction is applied by default and can be disabled at runtime (`corr<0/1>`): a base
  27ns HIGH-time cut (`SQUARE_DUTY_CORRECTION_NS`, from the TC4427's t_D1/t_D2
  propagation-delay asymmetry) below ~100kHz, replaced above that by
  `square_duty_cal_curve[]`, a manually-measured correction-vs-frequency table
  (log-frequency interpolated) fitted to a second BNC measurement pass, since the flat
  27ns model alone left a growing residual above 100kHz. **This directly supersedes the
  previous claim that "no correction is currently applied"** — that was true of an
  earlier build; it is not true of the current one. Current measured result
  (`measurements.md`): within ~12ns of 50% and non-monotonic across 1kHz–2MHz, at or
  below the scope's resolution.
- If a systematic pulse-width offset reappears on better equipment, it's correctable by
  extending `square_duty_cal_curve[]` with more measured points — the mechanism already
  exists; it is not a single hardcoded constant to hunt for anymore.

## Output selection
- **Confirmed:** the ADG1419 select line (GPIO10) is driven from firmware, and
  sine/square routing is a host command — every `sine`/`square`/`noise`/`am`/`fm` command
  sets `current_waveform`, and core1's `waveform_changed` handler drives GPIO10
  accordingly (SQUARE routes to 1, everything else — SINE/NOISE/AM/FM all share the DAC
  path — routes to 0).
- **Break-before-make: not implemented in firmware** — there is no explicit dead-time
  delay around the GPIO10 write. This relies entirely on the ADG1419 itself being a
  break-before-make SPDT switch by design (its standard datasheet characteristic). This
  is a part-characteristic claim, not a firmware behavior; it hasn't been independently
  re-verified against the datasheet PDF in this pass.

---

## Additional modes

**Confirmed implemented, all three — not experiments.** `firmware.md` previously listed
only Noise and AM as unconfirmed; **FM is a third fully implemented mode** the previous
version of this document didn't mention at all. All three share the sine DDS path
(`sine_phase_increment`) and are affected by low-current mode; none are mentioned as met
requirements in `version_requirements.md`, which still lists "Modulated signals (AM, FM,
PM, PWM...)" under **Future Versions** — that entry is stale and should move to the met
list (flagged here, not edited, since it's outside `firmware.md`).

- **Noise injection**, requested SNR settable 0–60dB (`snr<db>`).
  **Mechanism:** a Gaussian-ish noise sample (xorshift32 LFSR → Irwin-Hall(12), i.e. the
  sum of 12 uniform draws, crest factor ≈6) is added to the same sine table used for
  SINE mode, sample-by-sample, then clamped to the 8-bit DAC range — one shared PIO/DAC
  path, not a separate one. **SNR is calculated, not bench-calibrated:** the sine
  amplitude and noise sigma are derived from a closed-form budget
  (`A/√2 = s·10^(SNR/20)`, `A + 4s = 127`) assuming ideal Gaussian statistics on the
  ladder codes; it has not been measured against a real bench SNR reading. This matches
  `measurements.md`'s own "Still to characterise: Firmware feature validation: noise-mode
  SNR, AM depth" — i.e. the gap this document flags is the same one already tracked
  there, not a new one.
- **Amplitude modulation**, depth settable 0–100% (`d<number>` while in AM mode).
  **Source/definition:** a second sine phase accumulator at `mod_freq_hz` (`m<number>`)
  modulates the carrier as `carrier · (1 + depth · sin(2π f_m t))`; the carrier itself is
  pre-scaled to `1/(1+depth)` so the envelope peak always lands exactly on full scale and
  never clips, at any depth. The doc's own suggested validation,
  `(Vmax−Vmin)/(Vmax+Vmin)`, is the correct check for this definition and hasn't been run
  yet.
- **Frequency modulation** (undocumented previously): deviation settable in Hz
  (`d<number>` while in FM mode). The carrier phase is advanced by
  `sine_phase_increment + deviation_hz · sin(2π f_m t)` each sample — true instantaneous
  FM, not a fixed-table sweep.

---

## Host interface
- **Transport: USB CDC serial**, not UART, not the Pico W's wireless radio (present on
  the part, unused — `pico_enable_stdio_uart(AWG 0)` / `pico_enable_stdio_usb(AWG 1)` in
  `CMakeLists.txt`). Matches the stated USB requirement in `version_requirements.md`.
- **Command format:** newline-terminated plain text, one command per line, `> ` echoed
  as a prompt after each. Every numeric field is parsed with `atoi`/`atof`, so malformed
  non-numeric input generally reduces to 0 and is rejected by the same range check as an
  out-of-range value — there's no separate parse-error path.

  | Command | Effect | Valid range |
  |---|---|---|
  | `f<Hz>` | Set frequency (all waveforms; SQUARE also re-derives duty counts) | 1–50,000,000 |
  | `c<factor>` | Sample-rate calibration trim (SINE/NOISE/AM/FM only) | 0.5–1.5 |
  | `sine` / `square` / `noise` / `am` / `fm` | Switch waveform | — |
  | `snr<dB>` | Noise SNR (NOISE mode) | 0.0–60.0 |
  | `m<Hz>` | Modulation frequency (AM/FM) | 1–1,000,000 |
  | `d<amount>` | Modulation amount: AM depth % / FM deviation Hz | 0–100,000 |
  | `duty<pct>` | Square duty target | 1.0–99.0 |
  | `corr<0/1>` | Square duty-cycle correction on/off | 0 or 1 (any non-zero = on) |
  | `amp<0/1>` | Low-current (`<3mA`) mode on/off | 0 or 1 (any non-zero = on) |
  | `i` | Print full current state | — |

- **Error handling: outright rejection, not silent clamping** — for every range-checked
  command (`f`, `c`, `snr`, `m`, `d`, `duty`) an out-of-range value prints
  `"Invalid ..."` and leaves the parameter unchanged. The two newest commands, `corr` and
  `amp`, have no invalid case at all: any non-zero parse result is "on", anything else
  (including unparseable text, which `atoi` reduces to 0) is silently "off" — a minor,
  real inconsistency worth knowing about rather than a bug to fix here.
- **What the UI exposes, and what it doesn't:** `scripts/awg_ui.py` (Tkinter) exposes
  waveform mode buttons (sine/square/noise/am/fm), frequency/calibration/SNR/mod-freq/
  mod-amount dials, and the `<3mA` amplitude toggle. **It does not expose `duty` or
  `corr` at all** — square duty and its correction are reachable only over raw serial,
  with no control surface in the host UI and no way to send an arbitrary raw command from
  it either. That's a real gap, not an oversight to paper over in this doc.

### Not implemented
- Continuous digital amplitude control. The ÷4 toggle is a step function, not a control.
- Self-assessment / signal-quality reporting in the UI (jitter, THD, amplitude error,
  spurs), which is a stated requirement for the future UI.
- Standalone operation without a PC.
- Square duty/correction controls in the host UI (see above).

---

## Open firmware-side questions
1. Whether the DDS paths' Nyquist limit (~2.017MHz) should be enforced in the `f`
   command, given it currently accepts up to 50MHz for every waveform and aliases
   silently above ~2.017MHz for SINE/NOISE/AM/FM.
2. Whether `square_duty_cal_curve[]` needs more measurement points, or whether the
   current 8-point curve already keeps the residual within the scope's ~10ns division
   everywhere in 1kHz–2MHz.
3. Whether noise/AM/FM should be promoted out of `version_requirements.md`'s "Future
   Versions" list now that they're implemented and running, and what "shipped" should
   require (see `measurements.md`'s outstanding SNR/depth validation).
