# Current measurements

All measurements taken at the BNC unless stated otherwise.
Supply: 2x 9V batteries. Scope smallest division 10ns — treat sub-10ns figures as bounds.

## Rails
- V+: 9.52V
- V-: -9.54V

---

## Sinewave — full drive
Pot fixed so that Vp-p at 1kHz is 10.00V. Real max is ~16Vp-p at 1kHz.

| Frequency | Vmax | Vp-p | dB re 1kHz |
|-----------|--------|---------|--------|
| 1kHz      | 4.79V  | 10.00V  | 0      |
| 10kHz     | 4.78V  | 9.98V   | -0.02  |
| 100kHz    | 4.71V  | 9.92V   | -0.07  |
| 300kHz    | 4.36V  | 9.30V   | -0.63  |
| 500kHz    | 4.14V  | 8.96V   | -0.96  |
| 700kHz    | 4.03V  | 8.64V   | -1.27  |
| 1MHz      | 3.50V  | 7.00V   | -3.10  |

Note: Vmax and Vp-p are not self-consistent at the low frequencies (4.79V against a
10.00V swing implies Vmin = -5.21V), yet no DC offset is present at the BNC. Likely
auto-measure taken at different moments.

## Sinewave — low-current mode
DAC lookup table scaled by 4 to reduce current through the filter inductors.
Peak inductor current drops from ~15mA to ~3.75mA. Pot re-set for ~1V p-p at 1kHz.

| Frequency | Vmax      | Vp-p   | dB re 1kHz |
|-----------|-----------|--------|--------|
| 1kHz      | 497.47mV  | 1.01V  | 0      |
| 10kHz     | 496.7mV   | 1.01V  | 0      |
| 1MHz      | 494.7mV   | 1.00V  | -0.09  |

The -3.1dB droop at 1MHz disappears entirely at quarter drive, with no hardware change.
This is the confirmation that the droop is ferrite saturation in L1/L2 and not filter
design, op-amp bandwidth or slew limiting. See lessons.md #4.

The flat curve is the diagnosis, not the spec. As the instrument ships it runs the
ladder at full scale, so -3.1dB at 1MHz remains the honest number. Cost of the mode:
two bits of an already ~7-bit-effective ladder, so ~5 effective bits.

## DC offset
- None observed at the BNC. AC coupling removes the DAC's DC bias; additional filtering
  after the reconstruction filter and after the output buffer keeps the baseline clean.

---

## Output impedance
Measured on one unit at 1kHz into a 100Ω load (Vp-p unloaded vs. across the load).

| Mode | Source impedance |
|------|-------------------|
| Sine | 53Ω |
| Square | 58Ω |

Nominal is the 50Ω series resistor after the ADG1419, shared by both modes as a
physical component. The two modes are NOT the same figure, though: sine's +3Ω over
nominal is consistent with the ADG1419's Ron (~2.1Ω) alone; square's +8Ω is not — the
extra ~5Ω is the TC4427's own output resistance, which sits in the square path only
(square goes Pico -> TC4427 -> ADG1419 -> 50Ω with no op-amp buffering in between; sine
has the LM318 gain stage driving the switch instead). This corrects the earlier
assumption that "the 50Ω is shared by both modes, so one measurement covers it" — the
shared series resistor is, but total source impedance isn't, because what's in series
ahead of it differs by mode. Single unit; not yet checked for board-to-board variation.

---

## Squarewave
- Pico can produce signal at least up to 2MHz.
- Amplitude at the BNC: NOT RECORDED — measure.

### Edge rate (10-90%, at the BNC)
- Rise: ~20ns
- Fall: ~20ns (symmetric)
- Consistent with the TC4427's 19ns typical. The square path contains no op-amp, so the
  edge is set by the driver and the switch, not by LM318 slew rate.
- ~20ns is ~2 scope divisions; treat as approximate.
- Amplitude: 8.80V

### Duty cycle
| Frequency | Duty   | Pulse-width error |
|-----------|--------|-------------------|
| 1kHz      | 50.00% | below resolution  |
| 10kHz     | 50.00% | below resolution  |
| 100kHz    | 50.00% | below resolution  |
| 300kHz    | 49.90% | -3.3ns            |
| 500kHz    | 50.60% | +12ns             |
| 700kHz    | 50.78% | +11ns             |
| 1MHz      | 50.35% | +3.5ns            |
| 2MHz      | 50.29% | +1.5ns            |

Converted to time, the error is non-monotonic and never exceeds ~12ns, which is at or
below the scope's 10ns division. No systematic duty error is resolvable with the
present equipment.

---

## Still to characterise
- True filter corner: sweep 1.2 / 1.5 / 2MHz in low-current mode, where the filter is
  linear, and find the actual -3dB point.
- Jitter (limited by the scope's 10ns division).
- Low-frequency corner — sweep down to find -3dB, confirms the 1.3Hz calculation.
- Firmware feature validation: noise-mode SNR, AM depth.
