# Future Work

This document lists features, improvements, and extensions that are
intentionally postponed to preserve focus and momentum. May or may not be developed.

## High Priority

### DC Power supply integration
- 5 rail (used for big breadboards) design.
- Banana or 9V input.
- LCD Showing Current/ Watthours
- Digitally controlled with PICO
- Adjustable for every rail (3.3V, 5V, 9, 12V, 18, Function Generator) positive and negative.
- Short circuit protection, audible beep.
- Stable current and voltage
- Ground hookup for easy probing.
- BNC out, BNC in for external signals, and long jumper wire(s) for signal input on the breadboard.

### Chaining capability
- Connectors that allow for chaining. Using 2 generators, in order to power all 10 of the rails, and having a connector that allows another board as an "extension". Should look like 10 rails on the fimware.

## Lower Priority

### Discrete parts
Building the generator using only discrete components.

### Modulated signals
- Ability to produce modulated signals up to MHz

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
