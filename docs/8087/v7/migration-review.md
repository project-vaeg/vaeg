# v7 changes from v6

v7 removes the acquisition/OCR handoff stage because the user has already
prepared the evidence workspace and OCR results.

The implementation begins by validating the existing material, not downloading
or OCRing it again.

SoftFloat 3e is also already present as an upstream evidence archive under S03.
P02 must use that exact archive rather than silently downloading a different copy.

All v6 architectural decisions retained by v7 remain binding:

- one optional 8087 maximum
- default 8 MHz independently configurable clock (amended from the original
  10 MHz software policy after the VA2 field measurement)
- no 8087 on FPO2
- full documented 8087 instruction/form coverage
- `SERIAL_TIMED_V1`
- exact raw80/state handling
- primary-source-first clean implementation
- no host floating-point fallback
