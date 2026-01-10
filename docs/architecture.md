# Architecture (Draft)

## Goals (v0.1)
- PC UI control (USB)
- DDS/AWG waveforms + modulation
- Protection (ESD/short/overvoltage)
- Phased frequency goals (start low, extend upward)

## Block Diagram (placeholder)
[PC UI] -> [USB Protocol] -> [MCU (RP2040)] -> [DDS/ARB Engine] -> [DAC] -> [Reconstruction Filter] -> [Output Buffer/Attenuator] -> [Output + Protection]

## Notes
- Define “clean” using measurable specs (THD, jitter, amplitude flatness, spurs).
