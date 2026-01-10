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

- `requirements.md`  
  Defines current and past project goals, performance targets, and scope.
  Requirements are updated as milestones are achieved.

- `architecture.md`  
  Describes the system at a high level: block diagrams, data flow, clocking,
  and how the subsystems connect.

- `dds.md`  
  Details the digital signal generation approach (DDS / AWG), including
  waveform synthesis, frequency control, and modulation concepts.

- `dac.md`  
  Documents the DAC choice, configuration, clocking, and limitations, including
  pre- and post-DAC considerations.

- `output-stage.md`  
  Covers post-DAC analog processing: filtering, buffering, amplitude control,
  output impedance, and protection.

- `measurements.md`  
  Contains measurement results, scope screenshots, photographs, and performance
  tracking over time.

- `calibration.md`  
  Describes calibration procedures, correction methods, and achieved accuracy.

- `limitations.md`  
  Lists known hardware and software limitations, their causes, and potential
  mitigation strategies.

- `future-work.md`  
  Outlines medium- and long-term ideas that are intentionally deferred.

- `next-step.md`  
  A short, practical file describing the immediate next task to work on when
  returning to the project.
