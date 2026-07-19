# Current Limitations

## Equipment

### Measuring tools
    Oscilloscope smallest division is 10ns.
    Probes add visible parasitic capacitance at higher frequencies.

## Hardware

### R-2R Resistors
    - Ladder uses 2.2kΩ 1% thick film (the only Basic part available at JLCPCB).
    - Matching at 1% limits monotonicity around the MSB; effective DAC linearity ~7 bits worst-case.
    - 0.1% thin film is the known upgrade path for a future revision (costed, deferred).

### Filter inductors
    - ±10% tolerance: filter corner may shift a few % board-to-board.
    - SRF 17MHz: stopband attenuation degrades above ~17MHz. Acceptable — with the chosen
      sample rate the strong DAC images land below SRF.

### Low-frequency operation
    - AC coupling corner is ~1.3Hz. Very-low-frequency signals near the corner put real AC
      across the polarized coupling cap. The "0Hz" spec is nominal, not tested territory.

### Amplitude control
    - Analog potentiometer only in this build. Digital amplitude control is deferred
      (firmware R-2R lookup-table scaling planned).

### Variable Squarewave Gain
    Had faced great issues maintaining signal quality while using a simple gain control amplifier.
    - A more expensive IC may be required.

# Intentional Limitations
    Early revisions (breadboard, planned modular boards) accepted parasitics and noise
    in exchange for rapid iteration and learning.

    The v1.1 PCB is the consolidation step: all-SMD, single board, solid ground plane.
    Remaining intentional non-idealities: hand-soldered pot/BNC and test points kept
    for reworkability, 1% ladder and ±10% inductors accepted for cost/availability.
