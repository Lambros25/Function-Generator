# Architecture 

## Block Diagram (current)
[PC UI] -> [USB Protocol] -> [Raspberry Pi Pico W] 
    -> [R-2R] -> [Reconstruction Filter] -> [Variable Gain] -> [Output Buffer] -> [Output]
        -> [Variable Gain] -> [Output Buffer] -> [Output]
    -> [Buffer] -> [Variable Gain] -> [Output Buffer] -> [Output]


## Goals (v1.0)
- PC UI control (USB)
- Sinewave, Squarewave and AWG all High-Z
- Frequency range: 0Hz-1MHz

## Next in line (v2.0+)
- Protection (ESD/short/overvoltage)
- 50Ω Output
- Frequency up to 2MHz for sinewave, more for square through overclocking the pico

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