# Current Limitations

## Equipment

### Measuring tools
    Oscilloscope smallest division is 10ns.
    Probes add visible parasitic capacitance at higher frequencies.

### Breadboard
    High parasitic capacitances.

### R-2R Resistors
    - Although they have a tolerance of 1%, less tolerant resistors would improve the overall signal
    - 5k and 10k would be ideal.

### Variable Squarewave Gain
    Had faced great issues maintaining signal quality while usinga simple gain control amplifier.
    - A more expensive IC may be required. 

# Intentional Limitations
    Certain architectural choices (e.g. modular PCBs, external connectors, THT Componenets) introduce non-idealities such as added parasitics and increased noise.

    These tradeoffs are accepted in early revisions to enable rapid iteration and learning.

    Later revisions may consolidate modules to improve signal integrity once subsystem behavior is validated.