# Lessons

Mistakes made, what they cost, and the rules derived from them.

## 1. The 2.21Ω incident
The BOM matcher substituted 2.21Ω for 2.21kΩ across the entire R-2R ladder.
Cost: one full respin (boards arrived, failure discovered on delivery).

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
- Audit periodically via Parameter Manager grid — inconsistencies are visible in
  a column view that are invisible one component at a time.
- After any order-time substitution, copy the final chosen number back into the
  library the same day.
