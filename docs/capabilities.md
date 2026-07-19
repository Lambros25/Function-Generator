# Current measurements

## Breadboard prototype (historical)

### Sinewave
- Clean signal up to 1MHz
- Slight jitter/vibration.

### Squarewave
- Pico can produce signal at least up to 2MHz

## V1.1 PCB (JLCPCB, all-SMD) — measurements pending

To characterize after bring-up:
- Amplitude flatness 0-1MHz (expect ≤0.3dB droop at 1MHz from filter + op-amp rolloff)
- Actual filter corner vs. simulated 1.5MHz (±10% inductors will shift it)
- THD at 1kHz / 100kHz / 1MHz
- Squarewave edge rate at the BNC
- Jitter (limited by scope's 10ns division)
- Board-to-board variation across the 2 assembled units
