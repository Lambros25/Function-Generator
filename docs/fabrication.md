# Fabrication (v1.2)

## Board
- 70x70mm, 2-layer FR-4, all-SMD.
- Single uninterrupted GND pour on bottom layer. -9V and +9V as wide traces and local
  pours on top.
- VEE copper island on bottom layer under the ADG1419 thermal pad (resolves GND/VEE mismatch).
- Ground stitching vias around op-amps.
- Standard via: 0.6mm outer / 0.3mm drill (JLCPCB annular ring + drill minimums).

## DRC calibration
- Altium defaults are conservative vs. JLCPCB and generate false positives.
- Thresholds aligned to process limits: 0.15mm silk-to-silk, 0.1mm solder mask sliver.
- Component-scoped rules (e.g. OnComponent for the LFCSP) preferred over global overrides.

## JLCPCB order
- Economic PCBA: 5 boards fabricated, 2 assembled, 3 bare spares.
- U1 (Pico) and test points intentionally have no JLCPCB part number.
- Hand-soldered after delivery: BNC connector and the Pico. The Pico is soldered
  directly to the board on v1.2, not socketed. Everything else is SMD and machine-placed,
  including the potentiometer.

## Extended fee strategy
- Pay: core ICs (LM318, ADG1419, TC4427), electrolytics (no Basic SMD electrolytics
  exist), C0G filter caps (dielectric is the design intent), and the input/output
  connectors, where contact quality is the point.
- Swap to Basic: decouplers (C28233), anything where only the value/function matters.
- Note: Extended passives carry minimum purchase quantities that inflate component cost.
- Retrospect: chasing Basic parts for the filter inductors is what produced the
  under-rated-current limitation. The saving was real; so was the -3dB it cost at 1MHz.
  Cheapest-Basic is a valid default for decoupling and a bad default for anything in the
  signal path. See lessons.md #4.

## BOM workflow
- LCSC Part # lives as a parameter in the `lambros25` library — every placement inherits it.
- Audit via Tools -> Parameter Manager (grid view of all part numbers at once).
- Before paying: read the matched part DESCRIPTION of every line on the review page.
  Value AND unit. See lessons.md.
- For any part in a low-impedance signal path, check current rating on the datasheet,
  not just value/package/tolerance.
- Placement render check: polarity of electrolytics and LEDs, pin 1 of all ICs
  (especially the LFCSP, where a rotation is invisible on the pads).
- After any cross-EDA import: ERC plus cold continuity check on op-amp input pins.

## Shipping (EU / Cyprus)
- VAT (19%) collected at checkout on merchandise + shipping — every dollar saved
  in the BOM saves 1.19 at the total.
- Economy shipping tiers save ~half vs. courier; only latency changes.

## Git
- Repo tagged per fabrication submission so docs snapshot matches shipped Gerbers.
