# The Project

## Description
The final goal of this project is to build a laboratory-grade AWG that satisfies
my needs and ambitions. The emphasis is not only on the final instrument, but on
the learning process itself — understanding signal generation, signal integrity,
and real engineering trade-offs.  
This is why the device is built rather than bought.

---

# Documentation Overview

This folder contains the structured technical documentation of the project.
Each file has a specific role.

- `v1_2_architecture.md`  
  Overview of the architecture of the system. Goals currently being worked on as well
  as the next steps, and open questions the measurements have not closed.

- `design-philosophy.md`  
  Window into the philosophy and the reason for the construction of this project.

- `fabrication.md`  
  PCB fabrication and assembly knowledge: JLCPCB process decisions, DRC
  calibration, BOM/fee strategy, ordering workflow.

- `firmware.md`  
  Pico W firmware: signal generation, host interface, features — read off `AWG.c`
  directly, including the pin map, command reference, and known gaps.

- `future-work.md`  
  Outlines medium- and long-term ideas that are intentionally deferred.

- `integrated-circuits.md`  
  Tracks which ICs are currently in use, in which stages and why.

- `lessons.md`  
  Mistakes made, what they cost, and the rules derived from them.

- `limitations.md`  
  Lists known hardware and software limitations, their causes, and potential
  mitigation strategies.

- `measurements.md`  
  Measured performance of the current build, and what remains uncharacterised.

- `overview.md`  
  The current file, meant to give a coherent overview of the documentation.

- `Reconstruction.md`  
  Details on the reconstruction process, stage by stage.

- `version_requirements.md`  
  Version history, current and past project goals, performance targets and scope.
  Requirements are updated as milestones are achieved.

---

## Where to start
Read `design-philosophy.md`, then the version history at the top of
`version_requirements.md`, then `v1_2_architecture.md`. `lessons.md` is the honest
part.
