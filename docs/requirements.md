#Initial Long Term Requirements 10/01/2026

- PC-controlled user interface (visual, intuitive, USB-based)
UI must include self-assessment / signal-quality reporting:
  - Display measured or estimated jitter, THD/THD+N, amplitude error, spurs
  - Provide a clear “heads-up” to the user about signal cleanness and limitations

- DDS / AWG waveform generation
    - Standard waveforms + arbitrary waveforms
    - Modulated signals (AM, FM, PM, PWM; later digital modulation)
    - Phased frequency targets:
        - Start low (≤100 kHz)
        - Extend to MHz range
        - High-frequency/RF as a long-term goal
    - Emphasis on clean signal characteristics:
        - Jitter / phase noise
        - THD / distortion
        - Amplitude accuracy & flatness
        - Spur suppression

- Robust output stage:
  - Start at 3.3 V / 5 V
  - Later high-voltage capability (≈40–50 Vpp at low frequency)
  - Proper buffering & impedance

- Protection:
  - ESD
  - Short-circuit
  - Overvoltage / back-drive protection
- Strong documentation and reproducibility
