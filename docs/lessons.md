# Lessons

Mistakes made, what they cost, and the rules derived from them.

The through-line: three of the four below produced a board or a design that looked
correct and passed every check that was not a physical measurement. None of them were
design errors. Two of them cost a full PCB revision each.

## 1. The 2.21Ω incident
The BOM matcher substituted 2.21Ω for 2.21kΩ across the entire R-2R ladder.
Cost: one full respin (v1.0 boards arrived, failure discovered on delivery).

Rules:
- Before paying, read the matched part DESCRIPTION of every BOM line. Value AND unit.
- Part numbers live at the library level (`lambros25`), not assigned per-order —
  the library prevents the error class, the review-page read catches the rest.

## 2. Name/number drift
A library entry was named as a 0.1% thin-film (ERA6A) but carried the LCSC number
of a 1% thick-film part. Harmless in that position (feeding a ±20% pot), but the
same drift in a precision position would silently break the design.

Rules:
- Library name, description, and LCSC number must agree.
- Audit periodically via Parameter Manager grid — inconsistencies are visible in a
  column view that are invisible one component at a time.
- After any order-time substitution, copy the final chosen number back into the
  library the same day.

## 3. Migration errors
While migrating from KiCad into Altium, the two inputs of an LM318 were merged onto a
single net by a copy-paste that carried no visible sign of the merge. ERC passed.
Cost: one full respin (v1.1 unusable).

Rules:
- After any cross-EDA paste or import, run ERC AND a cold continuity check between the
  input pins of every op-amp instance — not just the one you were working on. The paste
  replicates; the fault can appear in stages you never touched.
- A schematic that compiles clean is not a schematic that is correct.

## 4. Max current rating
While picking components for the Butterworth filter, I selected the inductors on
inductance, package, tolerance and SRF, and did not check current rating. The parts are
rated 3mA. Actual peak current through L1 at full scale is ~15mA — five times over.

The saturating ferrite loses permeability and gains core loss, which shows up as
passband insertion loss well below the corner: -3.1dB at 1MHz where a Butterworth
should be flat. Confirmed by scaling the DAC lookup table by 4 in firmware, which
removed the entire droop with no hardware change.

Rules:
- For series elements in a low-impedance signal path, compute the actual current and
  check it against the rated/saturation current before selecting. A 60Ω system turns
  volts into real milliamps; in a filter you think in volts and hertz, not amps, which
  is exactly why the check has to be explicit.
- Simulation will never catch this. LTspice inductors are ideal and current-independent.

Hotfix / fix:
- Software: the low-current mode (lookup table /4) restores linearity at the cost of
  amplitude and two bits of resolution. Diagnostic-grade, not a fix.
- Hardware: same 10µH in 0805 or 1206 with an adequate saturation current. Costs board
  area, not money. 0603 two-terminal parts can be swapped with a soldering iron — flood
  both ends with leaded solder, alternate between pads, lift with tweezers. Price a
  named candidate on LCSC so the write-up can say what the right part would have been.
