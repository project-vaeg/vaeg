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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# ADR-0012: Assign uPD9002 core and register ownership

## Status

Accepted at G42. G44 was explicitly accepted at
`b5f6ee7da7665789ce23013f2f8418fe0889773c`, and G45 was explicitly accepted
at `5d1880c9446d05e863011df41e629801c9328779`, and G46 was explicitly
accepted at `5a9b4c1de72ce18fd7989c8db22a542ca49ede09`. G47 accepted the
fail-closed decision evidence at
`c683fe502647918d8ded2b4c2243da1b787c9d0a`; G48 accepted its implementation
at `339f5f62b3e69611f66f6689be8798f1c675b2cf`; G49 accepted the post-M48
reachability inventory at `2a21a5264a3830f5a393ed7fbd3fbe1e900f2926`; and
G50 accepted only the three approved deletion groups at
`8b818fca173b2e86b4a8af20e75bc069f7697cdf`. ADR-0013 supersedes this ADR's
original M47--M49 sequencing and its disproved assumption that the NP2
protected-mode cluster was unreachable. M51 is a mechanical ownership rename
and does not reopen those decisions.

## Context

Before M51, the active PC-88VA main CPU was a C implementation under `i286c/`.
It combines an inherited 80286-oriented base decoder, run-time V30 patches,
ABI-shaped raw save state, and the separate built-in register object at I/O
port `0xfff0`.
The supported machine always selects the V30-compatible native path, while the
source still exposes historical 286 and assembly-core choices. The M42
inventory and immutable dispatch baseline are needed before ownership can be
separated without changing behavior.

## Decision

### Ownership and final names

The instruction engine owns the `upd9002_core_*` namespace. The `0xfff0`
built-in register block in `iova/upd9002_regs.*` owns the `upd9002_regs_*`
namespace. These are separate components; a future aggregate
`Upd9002Device` is outside this series.

The final instruction-core directory is `cpu/upd9002/`. Active file moves and
public renames occur only in M51. Internal static opcode-handler identifiers
and `I286_*` implementation helpers may remain when changing them would alter
the immutable source-level dispatch identity or add cosmetic risk.

The expected public mapping is:

| Current API | Final API / disposition | Owner milestone |
|---|---|---|
| `i286c_initialize` | `upd9002_core_initialize` | M51 |
| `i286c_deinitialize` | `upd9002_core_deinitialize` | M51 |
| `i286c_reset` | `upd9002_core_reset` | M51 |
| `i286c_shut` | `upd9002_core_shut` | M51 |
| `i286c_setextsize` | `upd9002_core_set_ext_size` | M51 |
| `i286c_setemm` | `upd9002_core_set_emm` | M51 |
| `i286c_interrupt` | `upd9002_core_interrupt` | M51 |
| `v30c_step` | `upd9002_core_step` | M51 |
| `v30cinit` | `upd9002_dispatch_initialize` | M51 |
| `i286c_step` | removed after native invariant | M45 |
| `i286c`, `v30c` | remove block executors | M46 |
| `i286c_intnum`, `i286c_selector`, REP helpers | internalize or retain under the graph/name exception | M47–M51 evidence |
| `upd9002_reset`, `upd9002_bind`, `upd9002` | `upd9002_regs_reset`, `upd9002_regs_bind`, `upd9002_regs` | M51 |

The one authoritative final execution primitive is
`upd9002_core_step()`. A future `upd9002_core_run(cycle_budget)` may loop that
primitive, but no run-budget API is implemented in M42–M51.

### M51 mechanical ownership amendment

The maintainer accepted this exact move map after the M51 pre-implementation
audit:

| Before M51 | After M51 |
|---|---|
| `i286c/` | `cpu/upd9002/` |
| `i286c/i286c.c` | `cpu/upd9002/upd9002_core.c` |
| `i286c/v30patch.c` | `cpu/upd9002/upd9002_dispatch.c` |
| `i286c/v30patch.h` | `cpu/upd9002/upd9002_dispatch.h` |
| `iova/upd9002.c` | `iova/upd9002_regs.c` |
| `iova/upd9002.h` | `iova/upd9002_regs.h` |
| `cpuxva/memoryva.h` | `cpucva/memoryva.h` |

Historical internal basenames remain when they do not express a public
ownership boundary. `cpuxva/memoryva.x86` remains in place and byte-identical
as a frozen reference.

These 18 internal historical REP-helper identifiers are exact M51 exceptions:

```text
i286c_rep_insb       i286c_rep_insw       i286c_rep_outsb
i286c_rep_outsw      i286c_rep_movsb       i286c_rep_movsw
i286c_rep_lodsb      i286c_rep_lodsw       i286c_rep_stosb
i286c_rep_stosw      i286c_repe_cmpsb      i286c_repne_cmpsb
i286c_repe_cmpsw     i286c_repne_cmpsw     i286c_repe_scasb
i286c_repne_scasb    i286c_repe_scasw      i286c_repne_scasw
```

The first 17 are graph/provenance-bound. `i286c_rep_outsw` remains as part of
the same cross-translation-unit REP-helper family. The repository guard
enumerates these names and their permitted active files exactly; it does not
allow an `i286c_*` wildcard. They are internal implementation names, not
supported public APIs.

The literal state tags `CPU286` and `UPD9002`, `Cpu286StateCompat`,
`Upd9002RuntimeState`, `Cpu286CompatImage`, `I286_*` macros, and
`i286c_initreg` remain historical compatibility identifiers. Their spelling
does not imply active 80286 dispatch and is not changed by M51.

### State model

Three state concepts remain distinct:

* `Cpu286StateCompat` is the exact ABI-specific byte layout stored by the
  literal `CPU286` section.
* `Upd9002RuntimeState` is every field read, written, or address-taken by
  active reset, shutdown, interrupt, DMA-coupled execution, and instruction
  handlers.
* `Cpu286CompatImage` is the opaque serialization shadow that retains a full
  imported or canonical `CPU286` payload, including inactive fields and
  padding.

The instruction engine may access only `Upd9002RuntimeState`; the future state
adapter exclusively owns `Cpu286StateCompat` and `Cpu286CompatImage`. Import
must stage and validate the complete payload before commit. Export starts from
the compatibility image and overlays runtime-owned ranges. Reset and shutdown
apply the exact byte-range effects captured by M42 fixtures.

Compatibility is limited to the same ABI/toolchain family already implied by
raw structure serialization. The M42 Linux x86_64 GCC 15.2.0 fixture records a
112-byte, 4-byte-aligned `CPU286` payload and a 16-byte, 1-byte-aligned
`UPD9002` payload. No cross-endian or new cross-ABI guarantee is introduced.
The literal section names, section version zero, sizes, and loader-facing
layouts remain unchanged throughout this series.

M44 added the only intentional externally visible behavior change in the
series: a serialized `CPU286` payload with `cpu_type != CPUTYPE_V30` is
rejected atomically before resume. This is a narrow compatibility firewall and
must not be generalized.

### Dispatch identity

Runtime table construction remains. M42 records two different artifacts:

* `upd9002_final_dispatch_graph.csv` is the immutable final source-level graph
  for six runtime roots and every recursively reachable secondary table.
* `upd9002_dispatch_provenance_m42.csv` records base entries, patch operations,
  and explicit DIV/IDIV replacements at M42.

The accepted M42 graph remains byte-identical through G47 and remains an
immutable historical artifact afterward. M48 may change only exact graph,
provenance, and support-map rows explicitly approved at G47 and must record a
separate transition artifact rather than overwrite M42 evidence. Construction
provenance otherwise changes only under an explicit later approval. Runtime
pointer values, addresses, symbolizers, and function-pointer object hashes are
not identity.

### Native-mode and shutdown invariants

M42 proved that all 13 supported active CMake presets compiled the C core and
reached `v30c_step()` for VA machines. M45 removes the now-redundant
`USE_I286C` selector surface and calls `v30c_step()` directly for every active
instruction. The historical `USE_I286C=off`, `i286x_step()`, `v30x_step()`,
and assembly-tree configurations are frozen unsupported references, not
product options.

Normal initialization and reset establish the V30-compatible state
unconditionally. The legacy runtime `cpu_type` slot remains temporarily for
the M44 compatibility adapter, but it is never read as execution, reset,
interrupt, scheduler, diagnostic, or state-resume control. Only the state
adapter validates the serialized byte as `CPUTYPE_V30`.

The current CPU_SHUT path is an explicit initializer anomaly: it clears bytes
only through `offsetof(I286STAT, cpu_type)`, retains the tail beginning with
`cpu_type`, invokes the 286-style register initializer, and therefore leaves
upper FLAGS unlike native reset. The anomaly does not enable 286 dispatch and
must remain byte-identical to the M42 shutdown fixture.

### Milestone ownership

* M45 owns removal of `i286c_step()` and the supported-selector branch after
  native-only evidence.
* M46 owns removal of `i286c()` and `v30c()` and construction normalization.
* M47 determines REP-prefixed 0x0F correctness evidence and prepares an
  explicit protected-residue state-policy decision without changing behavior.
* M48 may implement only the exact semantic, state, dispatch, and baseline
  transition approved at G47.
* M49 inventories the remaining partial NP2 80286 protected-mode cluster after
  that transition without assuming it is unreachable.
* M50 may delete only dependency-closed groups explicitly approved at G49.
* M51 owns active moves, public renames, register-model renames, and final
  repository guards.

NP2 80286 protected-mode handlers, state, and helpers remain present through
G49. The accepted M47 audit proved active REPNE/REPE 0x0F edges into
`i286c_cts`, and imported MSW_PE state can activate `i286c_selector`. No
deletion is inferred from a symbol name, an overwritten base slot, or presumed
native unreachability.

### External oracle and non-goals

SingleStepTests V20 is an external semantic oracle only for the intersection
of the supported uPD9002/V52 native profile and faithfully representable V20
records. M42 records the internal support map but does not download, vendor,
classify, or baseline the corpus. M43 pins upstream commit
`9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21` and content-addresses the corpus
from canonical record digests, ordered opcode/form digests, the pinned metadata
digest, and corpus digest. README and metadata version strings are
informational and do not participate in identity. The corpus remains external
MIT-licensed test data and is not part of vaeg source or release archives.

The CI selection is the first 500 empty-prefetch-queue records per resolved
form after stable upstream-test-hash ordering. The full selection is every
empty-queue record. Prefetched records are `unsupported_fixture`. Implemented
and representable forms are `applicable`; absent target forms are narrowly
resolved `known_target_gap`; undefined/FPU oracle records are
`upstream_nonblocking`. M43 records no approved `expected_target_divergence`.
Applicable semantic mismatches are immutable evidence, not permission to
change CPU behavior during this structural series.

The pinned `v1_native/metadata.json` has a coarse primary `0F` entry with
status `extension`, but, contrary to a literal reading that it describes only
that coarse entry, it also contains explicit keys for the represented `0Fxx`
forms. Classification uses the actual second byte and M42 support-map form in
all cases and never treats the primary `0F` entry as an allowlist for the
extension space.

Broader V20/V30 family membership does not prove target support. Known missing
instruction forms are target gaps, not failures or implementation requests.

The following remain out of scope: uPD9002 compatibility mode, missing
instructions, timing changes, prefetch modeling, performance optimization,
multithreading, and a run-budget API. A state-version change remains forbidden
unless it is an exact part of the G47-approved M48 protected-residue policy.

### Baseline and tags

`pre-upd9002-series` is immutable and points to the accepted G41 SHA
`dc8a72da974f0ea328613e480f1de662c28f4436`. M42 adds graph, trace, harness,
ABI, reset, fixed-execution, and CPU_SHUT baselines. M42 must not create
`pre-upd9002-refactor`. That second tag is created only after M43 has completed
and G43 has been explicitly accepted.

## Consequences

The series can separate runtime state and remove dormant implementation only
against reproducible identity and behavior evidence. Raw save compatibility
remains deliberately ABI-scoped. The CPU_SHUT anomaly and current missing
instructions remain visible known constraints rather than opportunistic fixes.
