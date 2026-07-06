# V1.0
- Sine and Square output 0-1MHz 9V peak
- Sine adjustable amplitude
- PC controlled via Pico W thorugh simple UI
- PCB for durability and cleanliness
- Strong documentation and reproducibility
- BNC Output
- USB & battery powered


# V2.0+

- PC-controlled user interface (visual, intuitive, USB-based)
UI must include self-assessment / signal-quality reporting:
  - Display measured or estimated jitter, THD/THD+N, amplitude error, spurs
  - Provide a clear “heads-up” to the user about signal cleanness and limitations

- DDS / AWG waveform generation
    - Standard waveforms + arbitrary waveforms
    - Modulated signals (AM, FM, PM, PWM; later digital modulation)
    - Emphasis on clean signal characteristics:
        - Jitter / phase noise
        - THD / distortion
        - Amplitude accuracy & flatness
        - Spur suppression

- Robust output stage:
  - High-voltage capability (≈40–50 Vpp at low frequency)
  - Proper buffering & impedance

- Protection:
  - Short-circuit
  - Overvoltage / back-drive protection

- Discrete components

- Protection (ESD/short/overvoltage)

- Variable squarewave gain

### Non-requirements
    - Commercial certification