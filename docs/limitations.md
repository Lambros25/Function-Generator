# Current Limitations

## Equipment

### Measuring tools
    Oscilloscope smallest division is 10ns. Edge rates around 20ns are therefore
    only ~2 divisions wide and should be read as approximate.
    Probes add visible parasitic capacitance at higher frequencies.
    No THD or jitter measurement capability — those specs remain uncharacterised.

## Hardware

### R-2R Resistors
    - Ladder uses 2.2kΩ 1% thick film (the only Basic part available at JLCPCB).
    - Matching at 1% limits monotonicity around the MSB; effective DAC linearity ~7 bits worst-case.
    - 0.1% thin film is the known upgrade path for a future revision (costed, deferred).

### Filter inductors — under-rated for current
    - Rated current is 3mA. Actual peak current through L1 at full scale is ~15mA.
    - Consequence: the ferrite saturates, producing passband insertion loss well below
      the corner. Measured -3.1dB at 1MHz where the design predicts ≤0.3dB.
    - Mitigation in firmware: a low-current mode drops DAC amplitude by 4 via the R-2R
      lookup table, bringing peak current to ~3.75mA. The passband goes flat (-0.09dB at
      1MHz). Cost: 4x less output amplitude and two bits of resolution, leaving ~5
      effective bits.
    - This is a workaround, not a fix. The fix is a higher-saturation-current 10µH in
      0805/1206. See lessons.md #4.
    - ±10% tolerance: filter corner may shift a few % board-to-board.
    - SRF 17MHz: stopband attenuation degrades above ~17MHz. Acceptable — with the chosen
      sample rate the strong DAC images land below SRF.
    - Q = 30 at 2MHz, i.e. ~4.2Ω effective series resistance against a 1.85Ω DCR.
      Finite Q in a doubly-terminated ladder adds droop toward the corner.

### Filter corner below simulation
    - Simulated 1.5MHz, measured ~0.98-1.00MHz, in both drive modes.
    - Unexplained. Saturation accounts for the passband loss but not the corner shift —
      falling inductance should move the corner up, not down. Finite Q and ±10%
      tolerance stacking across six reactive parts are the remaining candidates.

### Low-frequency operation
    - AC coupling corner is ~1.3Hz. Very-low-frequency signals near the corner put real AC
      across the polarized coupling cap. The "0Hz" spec is nominal, not tested territory.
    - The corner is calculated, not measured.

### Output amplitude
    - Max sine is ~16Vp-p (8V peak) at 1kHz on ±9.5V rails. The 9V peak requirement is
      not reachable with an LM318 on this supply.
    - Amplitude falls to ~7Vp-p at 1MHz at a pot setting giving 10Vp-p at 1kHz.

### Amplitude control
    - Analog potentiometer only in this build. Digital amplitude control is deferred
      (firmware R-2R lookup-table scaling planned).

### Squarewave amplitude
    - Fixed. The square path runs Pico -> TC4427 -> switch -> BNC with no gain stage,
      so amplitude is set by the TC4427 supply and cannot be varied.
    - Variable squarewave gain is listed as a future requirement.

### Hi-Z loads only
    - There is no output buffer. The gain-stage LM318 drives the switch and the output
      network on its own, and can source roughly ±20mA.
    - Into a 50Ω terminated load, full swing would demand ~80mA. The instrument cannot
      deliver rated amplitude into 50Ω; Hi-Z is a constraint, not a preference.
    - The 50Ω series resistor sits after the switch and is the same physical component
      for both modes, but measured source impedance differs by mode: 53Ω sine, 58Ω
      square at 1kHz (`measurements.md`) — sine picks up the ADG1419's Ron, square adds
      the TC4427's own output resistance on top of that.

# Intentional Limitations
    Early revisions (breadboard, v1.0) accepted parasitics and noise
    in exchange for rapid iteration and learning.

    The v1.2 PCB is the first completed version: all-SMD, single board, solid ground plane.
    Remaining intentional non-idealities: BNC and Pico hand-soldered, 1% ladder and
    ±10% inductors accepted for cost and Basic-part availability.

    The Pico is soldered directly on v1.2 with no socket, so it is not swappable
    without rework.
