# ICs currently in use

- `LM318` for the non-passive sinewave stages: DAC buffer and gain stage.
    Chosen for its high slew rate and cheap price. 5pF external compensation per stage.
    Not present in the square path at all — see below.
    There is no separate output buffer; the gain stage drives the switch and the output
    network directly. Bandwidth note: GBW ~15MHz, so closed-loop bandwidth in the gain
    stage falls as the pot is turned up. Slew rate ~70V/µs sets the large-signal limit,
    and ~±20mA output current is what restricts the instrument to Hi-Z loads.

- `ADG1419` SPDT analog switch: selects sine or square at the output.
    Low Ron, runs on the ±9V rails, break-before-make. LFCSP package
    (thermal/VEE pad served by a VEE island on the bottom layer).
    Sits ahead of the 50Ω series resistance, so its Ron (~2.1Ω) adds to the source
    impedance in both modes — measured 53Ω sine vs. nominal 50Ω is consistent with Ron
    alone. Square measures higher still (58Ω); the extra ~5Ω there is the TC4427's own
    output resistance, not the switch — see its entry below.

- `TC4427` gate driver used as squarewave buffer.
    Fast, clean edges from the Pico GPIO; comfortable driving capacitive loads.
    19ns typical rise/fall, which matches the ~20ns measured at the BNC. Because the
    square path goes Pico -> TC4427 -> ADG1419 -> BNC with no op-amp in between, the
    TC4427 and the switch set the edge rate, and the square path reaches 2MHz while the
    sine path is limited to ~1MHz. The same lack of buffering is why square's source
    impedance (58Ω measured) runs higher than sine's (53Ω): with no op-amp downstream to
    absorb it, the TC4427's own output resistance adds directly in series with the
    ADG1419 and the 50Ω resistor, on top of what the switch alone contributes.
