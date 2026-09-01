# AWG / DDS Function Generator

Laboratory-oriented DDS/AWG signal generator. Current build: v1.2, first revision to
reach full bring-up and characterisation.

## Specs

| | |
|---|---|
| Waveforms | Sine, Square, Sine+Noise, AM, FM |
| Sine range | 0Hz – 1MHz (characterised; DDS/phase-accumulator, <1mHz step) |
| Sine amplitude | ~16Vp-p max at 1kHz (analog pot) |
| Sine flatness | −0.07dB @ 100kHz, −3.1dB @ 1MHz at full drive; flat to −0.09dB @ 1MHz in low-current mode |
| Square range | to at least 2MHz |
| Square edges | ~20ns rise/fall, 10–90% at the BNC |
| Square duty | user-settable 1–99% (default 50%), TC4427-corrected to within ~12ns of target across 1kHz–2MHz |
| Noise | Gaussian-ish, SNR 0–60dB (nominal, not bench-calibrated) |
| AM / FM | depth 0–100% / deviation in Hz |
| Output | BNC, 50Ω series; measured source impedance 53Ω (sine) / 58Ω (square) at 1kHz, Hi-Z loads only |
| DC offset | none observed |
| Resolution | 8-bit R-2R ladder, ~7 effective bits (1% resistor matching) |
| Supply | USB and 2× 9V batteries (measured rails 9.52 / −9.54V) |
| Control | USB CDC serial (plain-text commands) + desktop UI (`scripts/awg_ui.py`) |
| MCU | Raspberry Pi Pico W (RP2040), 225MHz overclocked |

Full measured numbers and the outstanding measurement list: `docs/measurements.md`.
Known limitations and their causes: `docs/limitations.md`.

---

## Structure
- `logs/` — Detailed overview of every work session. (timestamped `.txt`)
- `docs/` — structured technical docs (Markdown)
- `hardware/` — schematics, PCB, BOM, simulations
- `firmware/` — RP2040/Pico firmware
- `scripts/` — helper scripts for committing/pushing logs
- `Push all.bat` / `Push Engineering logs.bat` — Batch files to make git pushing quick and easy.
- `images/` — Images of the project. Used for progress tracking.

### Documentation index (`docs/`)
Start with `design-philosophy.md`, then the version history in `version_requirements.md`,
then `v1_2_architecture.md`.

| File | Contents |
|---|---|
| `design-philosophy.md` | Why this exists and what is out of scope |
| `version_requirements.md` | Version history, requirements and their status |
| `v1_2_architecture.md` | Block diagram, board architecture, open questions |
| `Reconstruction.md` | The sine chain stage by stage, with values |
| `integrated-circuits.md` | Which ICs, in which stages, and why |
| `measurements.md` | Measured performance and what is still uncharacterised |
| `limitations.md` | Known limits, their causes, and mitigations |
| `lessons.md` | Mistakes, what they cost, and the rules derived from them |
| `fabrication.md` | JLCPCB process decisions, DRC, BOM and fee strategy |
| `firmware.md` | Pico W firmware |
| `future-work.md` | Deliberately deferred ideas |
| `overview.md` | Map of the documentation set |

---

## Tooling

- **EDA:** Altium Designer, custom library `lambros25` with LCSC part numbers stored at
  the symbol level so every placement inherits them.
- **Simulation:** LTspice. Behavioural ZOH plus noise source used to model the DAC
  output realistically.
- **Fabrication:** JLCPCB, Economic PCBA tier. Components from LCSC.
- **MCU:** Raspberry Pi Pico W.

Board is 70×70mm, 2-layer FR-4, all-SMD, with a single uninterrupted ground pour on the
bottom layer. Everything is machine-placed except the BNC and the Pico, which are
hand-soldered. `docs/fabrication.md` has the process detail.

---

# Versioning
Integers = major milestones with a defined goal. Decimals = fixes, optimizations, small
additions toward that goal.

| Version | Outcome |
|---|---|
| v0.x | Breadboard. Architecture validated; discrete square path abandoned above ~200kHz. |
| v1.0 | First PCB. Dead on arrival — BOM matcher substituted 2.21Ω for 2.21kΩ across the whole ladder. |
| v1.1 | All-SMD, LC Butterworth introduced. Dead on arrival — LM318 inputs merged onto one net during the KiCad→Altium migration. |
| v1.2 | Working. Current build, characterised. |

Two of three PCB revisions were lost to silent tool-transformation errors rather than
design errors. Both boards looked correct and passed every check that was not a physical
measurement. That is the through-line of this project so far.
