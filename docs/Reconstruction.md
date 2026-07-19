# Current sinewave reconstruction method

## First Stage: Buffering
    - lm318 buffers the R-2R DAC output. 5pF compensation cap per datasheet.

## Second Stage: Reconstruction Filter
    - 5th-order doubly-terminated LC Butterworth low-pass ("the butterscotch").
    - System impedance: 60Ω, built as 2x 120Ω in parallel on BOTH source and load side (same reel as the rest of the board).
    - Cutoff: ~1.5MHz.
    - Topology: shunt C - series L - shunt C - series L - shunt C
    - Values: 1nF - 10uH - 3.3nF - 10uH - 1nF
    - Caps: C0G ±1% (X7R would add distortion in the signal path — dielectric matters here).
    - Inductors: 0603, ±10%, SRF 17MHz, DCR 1.85Ω. Only Basic part available; tolerance is the weak link of the filter.
    - Replaced the previous 4x cascaded RC LPFs: maximally flat passband (sim ~-0.1dB at 1MHz, expect ≤0.3dB on real parts) instead of stacked droop, -100dB/decade rolloff, purely passive so it adds no noise and no op-amp bandwidth limits.
    - Cost of the topology: fixed -6dB from double termination, recovered in the gain stage.

## Third Stage: AC Coupling
    - 100uF electrolytic, HP corner ~1.3Hz. Removes the DAC's DC offset.
    - Polarized: DAC side sits at positive DC bias, so orientation matters (+ toward DAC).

## Fourth Stage: Amplification
    - lm318 inverting amplifier. 1kΩ input resistor, 10kΩ potentiometer feedback. Max gain of 10.
    - Pot is a temporary placeholder — digital amplitude control planned.

## Fifth Stage: Output Buffer
    - lm318 buffer with 50Ω series output resistance (2x 100Ω in parallel) into BNC.

## Squarewave path
    - Pico GPIO -> TC4427 driver/buffer -> ADG1419 analog switch.
    - The ADG1419 selects sine or square at the output; both share the output buffer and BNC.
