# Fabrication (v1.1)

## Board
- 70x70mm, 2-layer FR-4, all-SMD.
- Single uninterrupted GND pour on bottom layer. ±15V / +9V as wide traces and local pours on top.
- VEE copper island on bottom layer under the ADG1419 thermal pad (resolves GND/VEE mismatch).
- Ground stitching vias around op-amps.
- Standard via: 0.6mm outer / 0.3mm drill (JLCPCB annular ring + drill minimums).

## DRC calibration
- Altium defaults are conservative vs. JLCPCB and generate false positives.
- Thresholds aligned to process limits: 0.15mm silk-to-silk, 0.1mm solder mask sliver.
- Component-scoped rules (e.g. OnComponent for the LFCSP) preferred over global overrides.

## JLCPCB order (rev A)
- Economic PCBA: 5 boards fabricated, 2 assembled, 3 bare spares.
- DNP: pot (RK09 10k) and BNC — sourced separately (EU), hand-soldered.
  Skips THT assembly fees and their Extended fees.
- U1 (Pico), J1, and test points intentionally have no JLCPCB part number.

## Extended fee strategy
- Pay: core ICs (LM318, ADG1419, TC4427), electrolytics (no Basic SMD electrolytics
  exist), C0G filter caps (dielectric is the design intent).
- Swap to Basic: decouplers (C28233), anything where only the value/function matters.
- Note: Extended passives carry minimum purchase quantities that inflate component cost.

## BOM workflow
- LCSC Part # lives as a parameter in the `lambros25` library — every placement inherits it.
- Audit via Tools -> Parameter Manager (grid view of all part numbers at once).
- Before paying: read the matched part DESCRIPTION of every line on the review page.
  Value AND unit. See lessons.md.
- Placement render check: polarity of electrolytics and LEDs, pin 1 of all ICs
  (especially the LFCSP, where a rotation is invisible on the pads).

## Shipping (EU / Cyprus)
- VAT (19%) collected at checkout on merchandise + shipping — every dollar saved
  in the BOM saves 1.19 at the total.
- Economy shipping tiers save ~half vs. courier; only latency changes.

## Git
- Repo tagged per fabrication submission so docs snapshot matches shipped Gerbers.
