# Current sinewave reconstruction method

## First Stage: Sinewave DAC

- `2.2kΩ single-value R-2R Ladder`
    2.2kΩ 1% thick film for the 2R nodes, two in parallel for the R nodes.
    Same-reel strategy: one BOM line for the whole ladder (plus LED droppers etc.),
    minimizes cost and eliminates value-mixup risk.
    Resistor ladder allows much higher update rates than PWM approaches.
    Couldn't use smaller R values as current would be too high for Pi Pico W pins.
    1% tolerance accepted (only Basic part available) — limits effective linearity
    to ~7 bits; 0.1% thin film is the future upgrade (see limitations.md).

- `lm318` buffer
    Using the recommended layout. Buffering and 5pF compensation handled at the
    DAC output, ahead of the filter.

## Second Stage: Reconstruction Filter
    - 5th-order doubly-terminated LC Butterworth low-pass.
    - Topology: shunt C - series L - shunt C - series L - shunt C
    - Values: 1nF - 10uH - 3.3nF - 10uH - 1nF
    - System impedance: 60Ω, built as 2x 120Ω in parallel on BOTH source and load side
      (same reel as the rest of the board).
    - Caps: C0G ±1% (X7R would add distortion in the signal path — dielectric matters here).
    - Inductors: 0603, ±10%, SRF 17MHz, DCR 1.85Ω, rated current 3mA, Q=30 at 2MHz.
      Only Basic part available. The current rating turned out to be the design error
      of this revision — see lessons.md #4.
    - Replaced the previous 4x cascaded RC LPFs: maximally flat passband instead of
      stacked droop, -100dB/decade rolloff, purely passive so it adds no noise and no
      op-amp bandwidth limits.
    - Cost of the topology: fixed -6dB from double termination, recovered in the gain stage.

### Cutoff: simulated vs measured
    - Simulated (LTspice, ideal parts): ~1.5MHz, ~-0.1dB at 1MHz.
    - Measured: corner sits at ~0.98-1.00MHz in both drive modes.
    - The corner is stable with drive level; what changes with drive level is passband
      LOSS, not corner frequency. At full drive the chain reads -3.1dB at 1MHz; in
      low-current mode it is flat to within 0.1dB.
    - Cause of the loss: ferrite saturation. ~15mA peak flows through L1 at full scale
      against a 3mA rating. A saturating ferrite loses permeability and gains core loss,
      which produces insertion loss well below the corner — exactly the observed shape.
      Confirmed by a firmware-only test: scaling the lookup table by 4 removed the
      entire -3.5dB droop with no hardware change.
    - OPEN: the corner shift from a simulated 1.5MHz to a measured ~1.0MHz is still
      unexplained. Saturation accounts for the loss, not the shift — if anything,
      falling inductance should move the corner UP. Remaining candidates are finite Q
      (Q=30 at 2MHz backs out to ~4.2Ω effective series resistance, well above the
      1.85Ω DCR) and ±10% tolerance stacking across six reactive parts. Adding ~4Ω in
      series with L1/L2 in LTspice is the next test.

## Third Stage: AC Coupling
    - 100uF electrolytic, HP corner ~1.3Hz. Removes the DAC's DC offset.
    - Polarized: DAC side sits at positive DC bias, so orientation matters (+ toward DAC).
    - Series coupling caps (high-pass sections) appear after the reconstruction filter
      and again after the gain stage. Between them, measured DC offset at the BNC is nil.

## Fourth Stage: Amplification
    - lm318 inverting amplifier. 1kΩ input resistor, 10kΩ potentiometer feedback.
    - Stage gain Rf/Rin is up to 10, but end-to-end gain from the DAC buffer is lower:
      the filter is doubly terminated (-6dB) and the 1kΩ input resistor loads the 60Ω
      termination (60‖1k ≈ 56.6Ω), giving a practical end-to-end maximum of roughly x4.85.
    - Pot is a temporary placeholder — digital amplitude control planned.

## Fifth Stage: Output Network
    - No output buffer. The gain stage drives the ADG1419 directly; the switch drives
      the output network on its own.
    - 50Ω series output resistance (2x 100Ω in parallel) sits AFTER the switch, so it is
      shared by the sine and square paths and the 50Ω source spec holds in both modes.
    - Consequence: the gain-stage LM318 is the only device driving the output. Fine into
      Hi-Z; it cannot deliver full amplitude into a 50Ω terminated load (~80mA required
      at full swing against ~±20mA available). Hi-Z loads are a constraint, not a
      preference.

## Squarewave path
    - Pico GPIO -> TC4427 driver/buffer -> ADG1419 analog switch -> 50Ω -> BNC.
    - The square path contains NO op-amp. This is why edges measure ~20ns: an LM318
      slewing a ~10.8V swing at ~70V/µs would take ~150ns. The edge rate is set by the
      TC4427 (19ns typical) and the switch.
    - It is also why square reaches 2MHz while sine tops out around 1MHz — the two paths
      have genuinely different bandwidth limits.
    - The ADG1419 selects sine or square; both share the output network and the BNC.
