<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M97 P4 transient-overfill follow-up

## Scope and baseline

The baseline was commit `ed70a11` (`M97a: render GLASS faces as convex
polygons`).  The final VAEG frame already had zero face underfill/overfill and
no internal holes.  The remaining report from real hardware was a temporary
roughly four-logical-pixel protrusion while a span was being written.

The VAEG run does not model a real-PC-88VA display observer at each CPU/SGP
write.  Consequently, the exact hardware transient is not claimed as
reproduced here.

## Before/after ownership

Before this change, the SGP range was calculated using equivalent outward
pixel-coordinate rounding, while the endpoint pass replayed **every** word in
the exact span.  For a representative span `[103,246]`, the intended SGP range
was words `26..60`; the endpoint pass subsequently walked words `25..61`.
The endpoint RMW preserved the final pixels, but complete interior words had
two owners and a second write stage.

The corrected partition is explicit and disjoint:

```text
logical span [103,246]
  left partial RMW: word 25
  SGP full words:   words 26..60
  right partial RMW: word 61
```

Same-word spans use one masked RMW and no SGP full-word command.  A partial
endpoint is never included in the SGP full-word set.  Complete interior words
are no longer replayed by the endpoint pass.

## Implementation

`glass_p4_sgp_emit_span` now derives `first_word`, `last_word`,
`full_first`, `full_last`, and `full_count` directly from the inclusive logical
span.  The endpoint record stores that partition and the face identifier.
`glass_p4_sgp_apply_endpoint_spans` applies only partial endpoint words and
skips the SGP-owned interior range.  The `GLASS_P4_SGP_AUDIT=1` build exports
the records to a gated GVRAM diagnostic area; the production build leaves this
disabled.

## Temporal QA

The independent checker is
`demos/glass-orbit/tools/verify-p4-temporal.py`.  It reconstructs each
operation in the order `left-RMW`, `SGP-full`, `right-RMW`, checks endpoint and
interior disjointness, and checks that every intermediate face-color set is a
subset of the exact logical span.  It also runs endpoint-residue and sloped
convex-polygon matrices.

VAEG audit result:

```text
records: 192
max_transient_overfill: 0
max_transient_underfill: 170 (allowed during progressive fill)
monotonic_fill: true
alignment_matrix: PASS (40 cases)
slope_matrix: PASS
overall temporal partition: PASS
```

The audit is a write-partition witness, not a silicon timing trace.  The audit
area is used only in the gated diagnostic payload and is masked by the normal
visual checker.

Standard (audit-disabled) VAEG capture remains:

```text
final face underfill: 0
final face overfill: 0
internal holes: 0
triangle-union mismatch: 0
visual verifier: PASS
```

Focused source, payload, and unit checks pass:

```text
check-p4-no-repair.py: PASS
check-p4-convex.py (sgp): PASS
test_verify_p4_temporal.py: PASS
```

## Hardware status

No new real-hardware observation was performed in this follow-up.  Existing
human-gate observations remain unchanged:

```text
PC-88VA:
  graphics: PASS
  transient overfill: UNVERIFIED
  ESC return: PASS
  keyboard after return: UNRESOLVED

PC-88VA2:
  graphics: PASS
  transient overfill: UNVERIFIED
  ESC return: PASS
  keyboard after return: PASS
```

This change does not claim SCAN_LEFT/SCAN_RIGHT hardware conformance.
