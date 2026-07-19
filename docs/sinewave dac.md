# Sinewave DAC

- `2.2kΩ single-value R-2R Ladder`
    2.2kΩ 1% thick film for the 2R nodes, two in parallel for the R nodes.
    Same-reel strategy: one BOM line for the whole ladder (plus LED droppers etc.),
    minimizes cost and eliminates value-mixup risk.
    Resistor ladder allows much higher update rates than PWM approaches.
    Couldn't use smaller R values as current would be too high for Pi Pico W pins.
    1% tolerance accepted (only Basic part available) — limits effective linearity
    to ~7 bits; 0.1% thin film is the future upgrade (see limitations.md).

- `lm318`
    Using the recommended layout, buffering and compensation handled at the DAC output.
