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

## Modular Hardware Architecture
    Early PCB revisions intentionally adopt a modular architecture consisting of a central controller board and multiple swappable functional modules (DAC, filtering, output stage).

    This approach:

        - Reduces iteration cost

        - Localizes design risk

        - Enables independent validation of subsystems

        - Supports rapid experimentation and learning

    Once subsystem behavior is fully characterized, modules may be consolidated into a monolithic PCB.