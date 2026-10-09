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
# M39 private Z80 integration manifest

This file defines tests but contains no ROM, disk image, restricted screen,
derived payload, or private trace. Tracked records use only neutral stable
asset identifiers. Private filenames, absolute paths, and hashes remain in a
maintainer-only log only when the maintainer explicitly authorizes that exact
metadata. Raw screenshots, logs, traces, save files, and writable media copies
remain outside Git. Suggested failure evidence paths below are outside this
repository.

The M39 procedure below is historical dual-core evidence. It built separate
trees from the same commit with `VAEG_Z80_CORE=legacy` and
`VAEG_Z80_CORE=suzukiplan`. Current M41 production has no selector. Run the
current single-core build uninstrumented first; repeat a relevant case with
`VAEG_Z80_COMPAT_INTEGRATION_TRACE=ON` and `--fdctrace` only when event evidence is
needed.

## Test definitions

| Stable ID | Mode and private identifiers | Core selection | Startup and action | Expected screen/system and FDD result | Evidence and save point | Pass criteria / failure evidence path |
|---|---|---|---|---|---|---|
| `m39-boot-va-v3` | VA/V3; record neutral owned-ROM-set ID | Run both separately | Cold boot to system menu or BASIC | Normal prompt/menu; no repeated reset or FDD timeout | Record boot completion and representative `f4`/IRQ/DRQ trace | Both reach the same usable state; failures: `private-evidence/m39/m39-boot-va-v3/<core>/` |
| `m39-demo` | VA mode; record neutral legal-demo ID if available | Run both separately | Cold boot and launch the bundled legal demo | Demo reaches its expected stable scene and remains responsive | Record startup and completion state; no save required | Same visible completion and no subsystem stall; failure path uses stable ID/core |
| `m39-os-operation` | VA/V3; record neutral ROM and expendable-OS IDs | Run both separately | Boot OS, list a directory, open/read a file | Directory and file data are correct; no retry loop | Capture final listing plus FDD command/status and IRQ/DRQ sequence | Results and transferred data agree; failure path uses stable ID/core |
| `m39-reset-repeat` | Same assets as boot case | Run both separately | Boot, reset, and boot again at least three times | Every boot reaches the same state | Record each completion and any failed iteration | No intermittent loader, WAIT, IRQ, or FDD failure |
| `m39-fdd-read` | VA/V3; record neutral readable-media ID | Run both separately | Read known sectors through the guest | Operation completes once without timeout | Trace command/status, port `0xf4`, DRQ/IRQ, byte count, and final data result | No missing/extra/reordered event and identical data |
| `m39-fdd-write` | VA/V3; record neutral expendable-media ID | Run both separately | Write known data, flush/eject normally, then read back | One completed command and correct readback | Trace `0xf4`, command/status, DRQ/IRQ; record readback result | No timeout/retry/corruption; failure path uses stable ID/core |
| `m39-loader` | VA/V3; record neutral representative-loader ID | Run both separately | Cold boot through loader to its expected destination | Loader completes without repeated command or hang | Record final state and FDD event ordering | Both complete with matching device-visible result |
| `m39-loader-timing` | VA/V3; record neutral timing-sensitive-loader ID | Run both separately | Repeat cold boot at least three times | Each run completes | Compare eventual events, not `Exec()` slice numbers; preserve `f4` trace | Any timeout, transfer failure, IRQ/DRQ change, or corruption blocks G39 |
| `m39-sleep-va` | VA idle firmware; record neutral ROM-set ID | Run both separately | Reach normal VA idle path, wait, then assert actual ATN/8255 wake | Sleep engages and guest resumes | Trace live/public PC, memory `7f67`, IN `fe`, `Wait(true)`, drained clocks, wake, `Wait(false)` | Constants remain `1732`; no missed sleep or stuck WAIT |
| `m39-sleep-sorcerian` | Record neutral lawful-asset ID if available | Run both separately | Reach Sorcerian idle path and use actual wake source | Sleep engages and guest resumes | Same trace fields; memory marker is not required | Constant remains `700e`; unavailable asset is recorded, not substituted |
| `m39-state-legacy-new` | Same build family; record ROM/disk IDs | Save with legacy, load same file with suzukiplan | Save only after frame/`Exec()` return during ordinary operation | New selection resumes the same next operation | Record decoded state, next instruction/effect, and final guest state | No revision error, altered transfer, or guest-visible divergence |
| `m39-state-new-new` | Same build family; record ROM/disk IDs | suzukiplan only | Save and reload after frame return | Same next instruction and result | Record register/state summary and device result | Architectural/HALT/WAIT/IRQ/clock state resumes correctly |
| `m39-state-fdd` | Record neutral expendable-media ID | Run legacy-to-new and new-to-new | Save while FDD activity is logically in progress between CPU calls | Loaded operation completes once with correct data | Record save boundary, command/status, `f4`, IRQ/DRQ, and final data result | No replay, loss, reorder, timeout, or corruption |
| `m39-state-halt` | Record neutral ROM/system ID | Run both separately; new-to-new reload | Save at reproducible HALT/idle boundary and load | Guest wakes/resumes normally | Record whether HALT or the permitted idle alternative was used, wake source, and next effect | No stuck or premature wake |
| `m39-state-wait` | Record neutral ROM/system ID | Run both separately; new-to-new reload | Save after `Wait(true)` and `Exec()` return, load, then apply actual wake | WAIT drains clocks, remains stopped, then resumes | Record WAIT bits, clock balance, wake, and next PC | No execution during WAIT and no failed wake |

## M39 execution record

All runs were made on 2026-07-15, Linux x86_64, from vaeg commit
`cbdbafe808c05fa9d0d525bb0673f6cff4bcc777`, with GCC 15.2.0 Release builds.
The two production trees selected `legacy` and `suzukiplan` respectively.
Tracked evidence uses only `private-romset-a`, `private-os-a`,
`private-demo-a`, and `private-timing-loader-a`. Originals were never mounted
writable; write cases used expendable copies. The maintainer did not authorize
filenames or hashes in tracked records. Screenshots, filtered event logs, raw
traces, save files, and working media stayed outside Git.

| Stable ID | Legacy result | Suzukiplan result | Sanitized evidence and difference |
|---|---|---|---|
| `m39-boot-va-v3` | PASS | PASS | VA2/VA3 reached the same stable OS prompt with no retry or timeout. |
| `m39-demo` | PASS | PASS | `private-demo-a` reached the expected animated scene and remained responsive. |
| `m39-os-operation` | PASS | PASS | Directory listing, guest file copy, and subsequent readback completed. |
| `m39-reset-repeat` | PASS | PASS | Three reset/boot cycles per core reached the same prompt without a hang. |
| `m39-fdd-read` | PASS | PASS | Guest directory and binary comparison reads completed once with no status error. |
| `m39-fdd-write` | PASS | PASS | An expendable copy received one guest file copy; binary readback reported no difference. Originals were unchanged. |
| `m39-loader` | PASS | PASS | The representative loader reached the same final demo state without retry. |
| `m39-loader-timing` | PASS | PASS | `private-timing-loader-a` reached the same stable menu. The new core appeared in a later wall-clock sample, then converged with no device-visible failure. |
| `m39-sleep-va` | PASS | PASS | `IN 0xfe` observed live/public PC `1734/1732`, memory `ff`, WAIT assertion, fixed-PC clock drain, ATN release, and resume. |
| `m39-sleep-sorcerian` | PASS | PASS | `IN 0xfe` observed live/public PC `7010/700e`, repeated WAIT assertion, ATN release, and resume. |
| `m39-state-legacy-new` | PASS (save) | PASS (load) | A revision-1 legacy state loaded under the new core; the OS prompt and subsequent directory read resumed normally. |
| `m39-state-new-new` | NOT APPLICABLE | PASS | A new-core state restored the saved prompt/display and subsequent FDD directory operation. |
| `m39-state-fdd` | PASS (save) | PASS (legacy load and new-to-new) | Saves were taken during visible loader activity after a frame return. Both restored runs completed the loader once with no timeout or replay. |
| `m39-state-halt` | PASS | PASS | The task-permitted reproducible idle alternative was used; no private HALT assertion is claimed. Each load resumed one directory operation without a stuck or premature wake. |
| `m39-state-wait` | PASS | PASS | State save occurred after `Wait(true)` at live/public PC `7010/7010`; load restored that PC, actual ATN released WAIT, and later sleep/wake cycles continued. |

The timing-sensitive trace emitted the same 21 ordered port-`0xf4` values in
both runs: nine `00`, then `10 14 04 04 04 14 1c`, then five `0c`. The first
164 completed FDC command records were byte-identical. The longer legacy
wall-clock capture contained 24 additional post-convergence records; no
common-prefix command differed. FDC records included DMA transfer counts and
ranges, and the Z80 trace recorded IRQ levels and acceptance-time acknowledge.
The current diagnostic stream has no dedicated DRQ-edge record, so no separate
DRQ edge-count claim is made. Uninstrumented final runs also reached the same
stable results.

The four maintainer-designated pre-existing untracked paths were not opened,
hashed, copied, modified, or used. No private asset, screenshot, trace, save,
or derived payload is staged or tracked.

**G39 PASSED.** None of the blocking boot, loader, FDD, IRQ-visible, sleep,
WAIT, or state-restore failures occurred. M40 has not started.

## M41 final single-core regression

Fresh M41 evidence was collected on 2026-07-15 from the final unconditional
wrapper build on Linux x86_64. The deleted M39 private evidence was not used.
Tracked records use neutral IDs only; private filenames, absolute paths,
hashes, ROM bytes, screenshots, traces, state files, and writable media copies
remain outside Git.

| Stable ID | Result | Sanitized evidence |
|---|---|---|
| `m41-boot-final-a` | PASS | VA2/VA3 reached the OS date prompt and usable command prompt without retry or timeout. |
| `m41-fdd-read-a` | PASS | A directory listing and known small file read completed with the expected value. |
| `m41-fdd-write-a` | PASS | An expendable copy received a small guest write and exact readback; the archival image was never mounted writable. |
| `m41-loader-timing-a` | PASS | The timing-sensitive loader reached its stable `Ready` destination without timeout or repeated completion. |
| `m41-sleep-va-a` | PASS | Live/public PC `1734/1732` triggered unchanged VA sleep, WAIT drained at fixed PC, ATN released it, and execution resumed. |
| `m41-sleep-sorcerian-a` | PASS | Live/public PC `7010/700e` triggered unchanged Sorcerian sleep, WAIT drained, ATN released it, and execution resumed. |
| `m41-state-legacy-final-a` | PASS | A fresh same-build-family legacy revision-1 save loaded in the final build; the command prompt and subsequent FDD read resumed. |
| `m41-state-final-final-a` | PASS | The final build restored its saved screen/prompt and subsequent FDD operation. |
| `m41-state-fdd-a` | PASS | A frame-boundary save captured a loader mid-word (`Read`); reload returned there and completed once at `Ready` without timeout or replay. |
| `m41-disasm-private-a` | PASS | At a live boundary, 16 bounded disassemblies all advanced; live/public PC remained unchanged before and after. |

The fresh diagnostic sleep capture contained 21 port-`0xf4` writes and showed
real ATN WAIT release for both sleep paths. Only sanitized counts and state
transitions are retained here. No raw trace or private asset is tracked.
