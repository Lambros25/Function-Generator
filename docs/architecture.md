# Architecture 

## Block Diagram (current)
[PC UI] -> [USB Protocol] -> [Raspberry Pi Pico 2W] -> [DAC] -> [Reconstruction Filter] -> [Output Buffer] -> [Output]

## Goals (v1.0)
- PC UI control (USB)
- DDS/AWG waveforms + modulation
- Protection (ESD/short/overvoltage)
- Phased frequency goals (start low, extend upward)
- Frequency range: 0Hz-1MHz

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