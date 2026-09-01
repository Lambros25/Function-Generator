# Future Work

This document lists features, improvements, and extensions that are
intentionally postponed to preserve focus and momentum. May or may not be developed.

## High Priority

### Digital amplitude control
- Firmware R-2R lookup-table scaling (deferred from v1.0/v1.1, which ship with an analog pot).
- Potentially combined with coarse analog gain switching.


## Lower Priority

### Discrete parts
Building the generator using only discrete components.


### High-frequency extensions
- Extend clean sine generation beyond the initial MHz range
- Investigate RF-specific PCB layout techniques
- Separate low-frequency/high-voltage and high-frequency/low-voltage paths

---

### Output stage improvements
- Higher-voltage output capability (40–50 Vpp at low frequency)
- Selectable output impedance (50 Ω / Hi-Z)
- Improved output protection for misuse scenarios

---

### Signal quality improvements
- Lower phase noise / jitter through improved clock sources
- Better reconstruction filtering strategies

---

### User interface extensions
- Onboard OLED display and rotary encoder
- Standalone operation without PC
- Preset storage and recall

---

### Calibration & metrology
- Automated self-calibration routines
- Temperature drift characterization
- Improved self-reporting of signal quality metrics

---

### Advanced modulation & communications
- Digital modulation schemes (FSK, PSK, ASK)
- External modulation inputs
- Use as a basic signal source for communications experiments

## Final Stage

### Pico Integration
- Integrate MCU inside the PCB
- Use of USB C
- Proper application instead of vscode
