# AWG / DDS Function Generator (Project)

This repository contains my ongoing engineering work to build a laboratory-oriented DDS/AWG signal generator.

## Structure
- `logs/` — conversation-style Engineering Logs (timestamped `.txt`)
- `docs/` — structured technical docs (Markdown)
- `hardware/` — schematics, PCB, BOM, simulations
- `firmware/` — RP2040/Pico firmware
- `scripts/` — helper scripts for committing/pushing logs

## Workflow (Engineering Logs)
1. Create a new log file in `logs/` (downloaded from chat as `.txt`).
2. Run one of:
   - Windows PowerShell: `scripts\push_logs.ps1`
   - macOS/Linux: `bash scripts/push_logs.sh`

That will add/commit/push changes to `logs/`.
