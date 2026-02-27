# Current reconstruction method

## First Stage: Buffering
    - lm318 for digital to analog as well as buffering.
## Second stage: Filtering
    - 5 cascading RC filters:
        - 4 Low Pass Filters in order to smooth out the signal well (Cutoff ~1.5MHz)
        - 1 High Pas Filter (Cutoff ~0.0017Hz) to just remove DC
## Third Stage: Amplification
    -Using lm318 and a (temporary) analog potentiometer, in an inverted voltage amplifier configuration. Max gain of 10.
## Fourth stage: Filtering
    -Due to the amplification showing more noise, another simple RC filter was needed (Cutoff ~1.5MHz).
## Fith stage: Buffer
    -Buffering using lm318 setting 47Ω output resistance. 
