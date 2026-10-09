<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M98x - Runtime ZUNDAMON count controls and load ladder

Status: **G98x passed; automated and human-gate evidence complete**

M98x starts from the accepted M98w head `b65d6c50af1f9bd7f574a17683c637e65212be78`.
It keeps the public ZUNDAMON renderer, M98u phase/order generator, M98w
page-local dirty unions, cadence ladder, HUD FPS field, and cleanup behavior.
The normal release guest is one binary with a default count of four. It accepts
one exact `/N1` through `/N16` option (case-insensitive like `/V`), plus the
existing `/V1` through `/V8` option, in either order. UP and DOWN queue bounded
requested-count changes; count state is latched for a complete hidden
transaction and the G0 count field changes only with the matching DSA1
publication. Counts saturate at one and sixteen and never wrap. The global
phase remains continuous modulo 64.

The runtime count path is enabled by `M98X_RUNTIME_MODE=1` in the local build
script. `M98V_ACTIVE_COUNT` remains only for legacy M98v/M98w fixed-count QA
builds. No `/N` or UP/DOWN path is compiled into those compatibility builds.
The release path has statically bounded 16-record, 16-index, and page-footprint
storage and one shared 128-KiB atlas; it adds no private image or payload copy.

The independent host model validates all 1,024 count/phase states, the 32,768
ordered page/count transition cases, count-one equality with M98t, deterministic
HUD tiles, parser negatives, and runtime build reproducibility. The report at
`docs/agents/reports/m98x_zundamon_runtime_count_ladder.md` records the exact
commands, identities, evidence limits, and the completed VA2 human gate.

M98y remains the later private IDA milestone. M98x does not integrate IDA,
change atlas pixels, alter orbit geometry, add `/N`-specific code copies, or
add gameplay. `REAL_HW_PENDING` remains in force.

`G98x passed`
