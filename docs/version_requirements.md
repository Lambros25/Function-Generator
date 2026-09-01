# Versioning
Integers = major milestones with a defined goal. Decimals = fixes, optimizations,
small additions toward that goal.

---

# Version History

## v0.x — Breadboard prototype
    - Pico-driven R-2R ladder, discrete square-wave path, cascaded RC reconstruction.
    - Found that a discrete square path could not be kept clean past ~200kHz.
      This is what led to the TC4427 driver.
    - Parasitics and noise accepted deliberately in exchange for iteration speed.
    - Outcome: architecture validated, moved to PCB.

## v1.0 — First PCB order
    - Mixed THT/SMD. LM318 and filter passives kept through-hole for rework.
    - Reconstruction: 4x cascaded RC low-pass.
    - Outcome: DEAD ON ARRIVAL. The BOM matcher substituted 2.21Ω for 2.21kΩ across
      every R-2R position. Failure found on delivery. See lessons.md #1.

## v1.1 — Second PCB order
    - Fully all-SMD, JLCPCB-assembled.
    - Reconstruction filter replaced: 4x cascaded RC -> 5th-order doubly-terminated
      LC Butterworth (flat passband, see Reconstruction.md).
    - Test points reduced to bare pads to cut cost.
    - BOM corrected after the 2.21Ω incident.
    - Outcome: DEAD ON ARRIVAL. LM318 inputs (pins 2/3) were merged onto a single net
      during the KiCad -> Altium migration. See lessons.md #3.

## v1.2 — Third PCB order (current build)
    - LM318 input short fixed.
    - Pico soldered directly to the board.
    - First revision to reach full bring-up and characterisation. See measurements.md.
    - Outcome: WORKING. Known open issue: filter inductors under-rated for the
      current actually flowing through them (lessons.md #4).

Two of three PCB revisions were lost to silent tool-transformation errors rather than
design errors. Both boards looked correct and passed every check that was not a
physical measurement.

---

# V1.x Requirements — status

- Sine and Square output 0-1MHz
    PARTIAL. Sine is characterised to 1MHz; the usable upper limit above 1MHz has not
    been established. Square confirmed to 2MHz.

- 9V peak output
    NOT MET. Max sine is ~16Vp-p (8V peak) at 1kHz, falling to ~7Vp-p at 1MHz with the
    pot fixed for 10Vp-p. On ±9.5V rails an LM318 cannot reach 9V peak. Either the
    requirement moves to ±15V rails (see the bench PSU project) or the spec is restated.

- Sine adjustable amplitude
    MET via analog pot. Digital amplitude control still deferred.

- PC controlled via Pico W through simple UI
    MET. Firmware is undocumented — see firmware.md.

- PCB for durability and cleanliness
    MET.

- Strong documentation and reproducibility
    IN PROGRESS.

- BNC output, 50Ω source impedance, Hi-Z loads
    MET, with the actual figure now measured rather than assumed: 53Ω sine / 58Ω square
    at 1kHz into 100Ω (`measurements.md`), on top of the shared 50Ω series resistor after
    the ADG1419 — sine's excess is the switch's Ron, square's larger excess also carries
    the TC4427's output resistance. Hi-Z is a hard constraint — with no output buffer
    the gain stage cannot drive a 50Ω terminated load at full amplitude.

- USB & battery powered
    MET. Characterised on 2x 9V batteries (rails 9.52 / -9.54V).

---

# Future Versions

- PC-controlled user interface (visual, intuitive, USB-based)
  UI must include self-assessment / signal-quality reporting:
  - Display measured or estimated jitter, THD/THD+N, amplitude error, spurs
  - Provide a clear "heads-up" to the user about signal cleanness and limitations

- Digital amplitude control (firmware R-2R lookup-table scaling,
  potentially with coarse analog gain switching)

- DDS / AWG waveform generation
    - Standard waveforms + arbitrary waveforms
    - Modulated signals (AM, FM, PM, PWM; later digital modulation)
    - Emphasis on clean signal characteristics:
        - Jitter / phase noise
        - THD / distortion
        - Amplitude accuracy & flatness
        - Spur suppression

- Robust output stage:
  - High-voltage capability (≈40-50 Vpp at low frequency)
  - Proper buffering & impedance

- Protection:
  - Short-circuit
  - Overvoltage / back-drive protection
  - ESD

- Discrete components

- Variable squarewave gain

### Non-requirements
    - Commercial certification
