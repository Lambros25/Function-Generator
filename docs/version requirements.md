# Versioning
Integers = major milestones with a defined goal. Decimals = fixes, optimizations, small additions toward that goal.

# V1.1 (current build)
V1.0 requirements, revised after the first PCB order:
- Reconstruction filter replaced: 4x cascaded RC -> 5th-order LC Butterworth
  (flat passband to 1MHz, see Reconstruction.md)
- Fully all-SMD, JLCPCB-assembled board
- BOM corrected after the 2.21Ω incident (see lessons.md)

# V1.0
- Sine and Square output 0-1MHz 9V peak
- Sine adjustable amplitude (analog pot in this build; digital amplitude deferred to v1.x)
- PC controlled via Pico W through simple UI
- PCB for durability and cleanliness
- Strong documentation and reproducibility
- BNC Output, 50Ω source impedance, Hi-Z loads
- USB & battery powered


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
