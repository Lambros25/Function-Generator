# Architecture

## Block Diagram (current)
```
[PC UI] -> [USB Protocol] -> [Raspberry Pi Pico W]
    |
    +-- sine:   [R-2R] -> [LM318 Buffer] -> [LC Butterworth Filter] -> [AC Coupling]
    |                  -> [Variable Gain (LM318 + pot)] --+
    |                                                     |
    +-- square: [TC4427 Buffer] --------------------------+
                                                          |
                              [ADG1419 Switch] -> [50Ω series] -> [BNC]
```

Two things follow from this and are worth stating explicitly:

- **The square path contains no op-amp.** Pico GPIO -> TC4427 -> switch. Confirmed by
  measurement: 20ns edges on a ~10.8V swing are ~430V/µs, far beyond LM318 slew rate.
- **There is no output buffer after the switch.** The gain stage drives the ADG1419
  directly on the sine side, and the 50Ω series resistance sits between the switch and
  the BNC — that resistor is the same physical component for both modes, but total
  source impedance is not identical between them: measured 53Ω (sine) vs. 58Ω (square)
  at 1kHz (`measurements.md`). Sine's +3Ω over nominal is the ADG1419's Ron; square's
  extra ~5Ω on top of that is the TC4427's own output resistance, which sits in the
  square path only (no op-amp there to absorb it).

## Goals (v1.x)
- PC UI control (USB)
- Variable gain sinewave, squarewave.
- Frequency range: 0Hz-1MHz.
- 50Ω source impedance, intended for Hi-Z loads.

## Notes
- Define "clean" using measurable specs (THD, jitter, amplitude flatness, spurs).

## Consequences of dropping the output buffer
- With no buffer after the switch, the gain-stage LM318 is the only thing driving the
  output network. Into Hi-Z that is trivial. Into a 50Ω terminated load it is not:
  16Vp-p across the 50Ω series plus a 50Ω termination asks for ~80mA, well beyond the
  LM318's ~±20mA output. Hi-Z loads are the stated intent, but this should be written
  down as a hard constraint rather than a preference.
- The ADG1419's Ron (~2.1Ω) is in series with the 50Ω and adds to source impedance in
  both modes; the square path additionally carries the TC4427's output resistance.
  Measured: 53Ω sine, 58Ω square (see above and `measurements.md`).

## Open questions
- Filter corner measures ~1.0MHz against a simulated 1.5MHz; unexplained.
  See Reconstruction.md.

## Hardware Architecture (v1.2 PCB)
    Breadboard and early planning used a modular validation approach
    (separate DAC / filter / output stages, accessible for rework).

    The v1.2 PCB consolidates all stages onto a single 70x70mm 2-layer
    all-SMD board, JLCPCB-assembled. This is the "consolidation" step the
    modular philosophy was building toward.

    Assembly: everything is SMD and machine-placed except the BNC connector and the
    Pico, both hand-soldered. The Pico is soldered directly to the board on v1.2 —
    no socket, so it is not removable.

    Rework access is preserved via:
        - Test points / bare pads on stage boundaries and supply rails

    Ground strategy: single uninterrupted GND pour on the bottom layer,
    VEE island under the ADG1419, stitching vias around the op-amps.
