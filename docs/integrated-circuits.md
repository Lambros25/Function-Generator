# ICs currently in use

- `LM318` for all (non-passive) sinewave stages: DAC buffer, gain stage, output buffer.
    Chosen for its high slew rate and cheap price. 5pF external compensation per stage.
- `ADG1419` SPDT analog switch: selects sine or square at the output.
    Low Ron, runs on the ±15V rails, break-before-make. LFCSP package
    (thermal/VEE pad served by a VEE island on the bottom layer).
- `TC4427` gate driver used as squarewave buffer.
    Fast, clean edges from the Pico GPIO; comfortable driving capacitive loads.
