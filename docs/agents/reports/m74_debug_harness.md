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
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M74 deterministic debug harness

## Identity

- Branch: topic/m74-debug-harness
- Starting SHA: 7f5a15b344b58b7136d553b6a21813fb0fea497a
- Contract commit: a97f0f18
- Implementation commit: 4fa61415
- Obsolete-plan cleanup commit: 7e7b8050
- Persistence isolation implementation commit:
  433e6e31a89ed7af4cbff6a7a6a269c634913faa
- Current trace-enabled macOS worker SHA-256:
  336916dd61da0275ec3d7bba42420bd68b43617e8c987287c890553e01e14013
- Current MinGW worker SHA-256:
  e9ff4772bfb34d67f0a183606ead69cd6630bf096b26064d2e40c6b31d3715c2
- Current trace-disabled macOS worker SHA-256:
  9924d28ed59b0ffb7ad8b974b85d240f9f784934f3c8409373a22ab3282d5677
- Trace-enabled macOS worker SHA-256:
  fb33080416cd073fa7e4f2d0542b698c4498cd994e6483094ac753cab2f34a43
- MinGW worker SHA-256:
  1b33b5be88f816d2874189ca662a7375d52719314bd1e8f66822b37b71c41f87
- Trace-disabled macOS worker SHA-256:
  d14441b30fce280eab032fbcc4c2a8d0fdf97b18f1a1c63b960350bbfd8d29b7
- Runner: tools/m74-diagnostics/run_debug_case.sh
- Runner SHA-256:
  5640a2c1fb80946422a13d1f1933376832ade5c29da98ae65beeee7f79ae552b

The workers were built from the implementation commit. This report is a
documentation-only successor.

The three `Current` worker hashes above were built from persistence commit
433e6e31a89ed7af4cbff6a7a6a269c634913faa.

## Outcome

M74 provides the accepted workflow in one deterministic sequential script:

> Inject input at a specified guest frame, wait for an exact PC ordinal, start
> a bounded trace at that pre-instruction event, capture registers, TVRAM, and
> the rendered screen at the same event, and exchange an FDD during the run.

The diagnostic is default-off. No CPU instruction semantics, memory mapping,
I/O behavior, storage behavior, or guest-visible correction was changed.

## Script contract

The version-one command set is:

    debug-script 1
    limit-frame <absolute-guest-frame>
    resource <neutral-id> <local-path-or-none>
    counter <neutral-id> <segment>:<offset>
    wait-frame <absolute-guest-frame>
    input-line <ascii-text>
    enter
    mount-fdd <1-or-2> <resource-id>
    wait-pc <segment>:<offset> <nonzero-ordinal>
    trace <neutral-id> <bounded-step-count>
    capture <neutral-id> registers tvram screen
    exit

Commands execute in file order. limit-frame is required, unique, positive,
and uses completed guest frames rather than host elapsed time. A run therefore
has a deterministic guest-semantic bound even when a selected PC is never
reached.

input-line accepts printable ASCII and appends Return. enter sends a bare
Return. mount-fdd resolves only a declared neutral resource ID; a resource
whose value is none ejects the selected drive.

## PC event and pause semantics

The fixed-address observer runs in pccore_exec() immediately before the
uPD9002 instruction step. It counts exact CS:IP appearances and selects the
requested nonzero ordinal.

On a match:

1. registers, segment bases, flags, guest clock, and ordinal are copied;
2. the CPU loop returns before instruction execution;
3. no subsystem, SGP, event-queue, or guest-frame progress occurs;
4. contiguous trace and capture actions consume the same snapshot;
5. the pause is released;
6. the observer suppresses only the duplicate observation caused by resuming
   at the same CS:IP;
7. the matched instruction executes and is the first bounded trace entry.

The observer was deliberately kept outside cpu/upd9002/. The protected
instruction core has no M74 diff. This preserves the M60 semantic protection
boundary while still observing the canonical runtime instruction call site.

## Counters and bounds

Up to 32 fixed counters are registered before guest execution. They remain
active for the run and emit final rows to events.tsv. A counter increments
once per real pre-instruction appearance. The resume pass at a paused PC is
not counted twice.

The ROM-less test verifies this with the reset vector: the selected event,
capture, and one-step trace all occur at F000:FFF0, while the final counter is
exactly one.

The test also arms an unreachable DEAD:BEEF event and verifies that
limit-frame 1 stops at guest frame one without a PC event. Host timeout is
only external containment and is not used as the guest verdict.

## Capture outputs

Each neutral capture ID produces a deterministic subset of:

- <id>.registers.tsv: vaeg-registers-v1, pre-instruction registers and bases;
- <id>.tvram.bin: existing VAEGSCN1 raw TVRAM format;
- <id>.screen.bmp: rendered screen at the same paused event;
- <id>.trace.log: existing bounded upd9002-trace-v1 format;
- events.tsv: event chronology and final fixed-counter values.

The explicit-path capture API does not print paths when used by the harness.
The legacy environment-driven final screen capture retains its old interface.

## Input and FDD actions

Frame actions use the existing keyboard paste and normal FDD replacement
paths. The ROM-less integration test waits until frame one, ejects FDD1 via
the neutral resource ID empty, sends Return, and exits without reaching its
frame limit. This proves execution of frame, media, and input actions without
a private image.

The tracked runner accepts exactly one neutral case ID. Local worker, script,
output, model, and optional ROM directory paths are supplied through
VAEG_M74_* environment variables. It records source SHA, worker SHA-256,
runner SHA-256, model, and guest frame bound in identity.tsv. A synthetic
runner invocation passed at implementation commit 4fa61415.

## Privacy and repository scope

Private scripts, ROMs, disks, screenshots, traces, save data, paths, and
digests remain outside Git. Public artifacts use synthetic ROM-less state and
neutral IDs. Event output records resource IDs, not resource paths.

No ROM, disk image, generated worker, generated capture, absolute maintainer
path, or private asset identity is tracked by M74.

## Compatibility

The legacy --headless-input-script, --trace-cpu, and final screen capture
interfaces remain available. The integrated harness uses the paired options
--debug-script and --debug-output-dir. It rejects combinations with
--trace-cpu or --headless-input-script.

A trace-disabled build accepts scripts without trace and rejects a trace
action before guest execution with an explicit diagnostic. The established
enabled/disabled trace-equivalence test remains passing.

## Validation

All Git-dependent commands used process-local
GIT_CONFIG_GLOBAL=/dev/null and GIT_CONFIG_SYSTEM=/dev/null. Persistent Git
configuration was not changed.

| Check | Exact command or scope | Result |
|---|---|---|
| Encoding | python3 tools/repo/check_encoding.py --expect utf8 | PASS, 0 violations |
| EOL | python3 tools/repo/check_eol.py | PASS |
| Case | python3 tools/repo/check_case.py | PASS, 0 findings |
| Diff | git diff --check | PASS |
| Shell syntax | sh -n tools/m74-diagnostics/run_debug_case.sh | PASS |
| Trace configure | cmake --preset macos-macports with trace and tests ON | PASS |
| Trace build | cmake --build --preset macos-macports -j4 | PASS |
| Selftest | build/macos-macports/sdl2/vaeg --selftest | PASS |
| M74 integration | focused vaeg_m74_debug_harness CTest | PASS |
| Trace equivalence | focused vaeg_upd9002_trace_equivalence CTest | PASS |
| M68/M69/M70 | three focused canonical tests | PASS, 3/3 |
| Trace-disabled build | Release, trace OFF, tests OFF | PASS |
| Trace-disabled behavior | plain script and trace-action rejection | PASS |
| MinGW configure | mingw-cross with trace and tests ON | PASS |
| MinGW build | cmake --build --preset mingw-cross -j4 | PASS |
| Tracked runner | synthetic neutral case at 4fa61415 | PASS |
| Full ROM-less before CI repair | isolated ctest -L romless -j4 | 68 PASS, 3 FAIL, 1 SKIP of 72 |

The three full-suite failures are:

- vaeg_upd9002_m60d_frame_static
- vaeg_upd9002_m60e_iret_static
- vaeg_upd9002_m61_mov_imm_static

They reject protected-history identities after the repository-wide history
reconstruction. The same three commands, with the same signatures, fail at
the M74 starting SHA 7f5a15b344b58b7136d553b6a21813fb0fea497a in an
isolated temporary worktree. They are established starting-state failures,
not M74 regressions. M74 does not weaken or rewrite those validators. The
skipped test is the established external SST test and is recorded as SKIP,
not PASS.

An earlier non-isolated ROM-less invocation failed Git-dependent checks
because the sandbox could not read the maintainer global Git configuration.
That invocation is superseded by the process-local isolated run above.

One selftest abort occurred only while selftest and another selftest-bearing
CTest were launched concurrently and raced in temporary drop-media cleanup.
The serial rerun completed with selftest: all tests passed. No M74 evidence
depends on the concurrent abort.

## Diagnostic integrity

The M74 ROM-less test proves:

- pre-instruction ordinal selection at F000:FFF0;
- register capture at that exact PC;
- TVRAM and BMP output schemas;
- bounded trace beginning with the matched instruction;
- one real counter hit with no resume double-count;
- deterministic frame-limit behavior for an unreachable PC;
- frame-delayed input and FDD action execution.

The existing trace-equivalence test proves unchanged deterministic behavior
between enabled and disabled trace builds for its synthetic contract. No
private integration run is required for these diagnostic invariants.

## Changed files

The implementation:

- adds fixed observer and snapshot state under diagnostics/;
- adds the sequential script engine under sdl2/;
- adds CLI integration and pccore pause/resume coordination;
- extends screen capture with an explicit-path diagnostic API;
- adds the tracked runner and ROM-less integration test;
- documents the operator contract.

The permanent bug-fix ledger is unchanged because M74 adds diagnostic
infrastructure and does not correct guest-visible behavior.


That statement applies to the original harness implementation. The later
persistence extension changes host file selection and is recorded in the
permanent bug-fix ledger; it changes no CPU, memory, ROM, or device semantics
and tracks no private persistence payload.

## Worktree and hosted CI

At implementation commit 4fa61415, the worktree was clean. This report is the
only report-commit addition before the hosted result was recorded.

Hosted CI was run once after local native, ROM-less, trace-equivalence,
trace-disabled, and MinGW cross-build validation was complete:

- run: 31358987797
- evaluated SHA: fc95c1a01477abf53f3d1d9f8bd92683ae9d5aa2
- result: FAIL
- successful jobs: repo invariants, standalone compatibility conformance,
  and Windows release artifact
- failed jobs: Ubuntu GCC, Ubuntu Clang, Ubuntu ASan, macOS compatibility,
  Windows compatibility, and the uPD9002 architectural SST ratchet

The five compatibility jobs report the same five protected-history failures:

- vaeg_upd9002_m60b_authority_static
- vaeg_upd9002_m60c_audit_static
- vaeg_upd9002_m60d_frame_static
- vaeg_upd9002_m60e_iret_static
- vaeg_upd9002_m61_mov_imm_static

The first three fail because protected-history object
ba2b7d3f5c76646b30d63fd8951f4a1964817b15 is absent after the repository
history reconstruction. The last two reject the unavailable symmetric
difference rooted at 8736f8afe6d8eeb58e58c7afdaf5951e2306cb63. The focused
SST-ratchet job runs the first three static checks and fails on the same
missing protected-history object.

This is an established starting-state CI failure, not an M74 regression.
Hosted run 31356046763 evaluated the M74 starting SHA
7f5a15b344b58b7136d553b6a21813fb0fea497a on main before the M74 branch run;
it failed the same five compatibility tests and the same SST-ratchet subset.
The new vaeg_m74_debug_harness test passed in the hosted Linux GCC, Linux
Clang, Linux ASan, and macOS compatibility logs. No additional M74-attributable
failure was observed.

This was the pre-repair state. The CI repair integrated later in this branch
updates only the rewritten protected-history topology identities and the
serialization of two tests that share a mutable SCSI image.

## Persistence isolation extension

Commit 433e6e31a89ed7af4cbff6a7a6a269c634913faa adds the accepted host-state
isolation contract:

- configuration defaults to `./vaeg.cfg`;
- VA backup memory defaults to `./vabkupmem.dat`;
- VA2/VA3 backup memory defaults to `./va2bkupmem.dat`;
- `--cfg` and `--bkupmem` select exact paths, with relative paths resolved from
  the process current working directory;
- `--no-cfg` and `--no-bkupmem` suppress both reads and writes;
- path and disable options for the same state are mutually exclusive;
- executable-directory and per-user fallback lookup and implicit migration are
  not performed.

The final model is selected before the default backup filename is chosen. A
missing selected file begins from built-in/default state and is created by the
normal save lifecycle.

The persistence selftest verified explicit config/backup writes and disabled
non-writes. Fresh ROM-less smoke directories verified that VA creates only a
16,384-byte `vabkupmem.dat`, VA2 creates only a 16,384-byte
`va2bkupmem.dat`, an explicit `--bkupmem` path suppresses both defaults, and
`--no-bkupmem` creates no backup file. Those smoke runs used `--no-cfg` so no
configuration file could affect the model-selection result.

Current extension validation used process-local Git configuration isolation:

| Check | Exact command or scope | Result |
|---|---|---|
| Encoding | `python3 tools/repo/check_encoding.py --expect utf8` | PASS, 0 violations |
| EOL | `python3 tools/repo/check_eol.py` | PASS |
| Case | `python3 tools/repo/check_case.py` | PASS, 0 findings |
| Diff | `git diff --check` | PASS |
| Shell syntax | `sh -n tools/m74-diagnostics/run_debug_case.sh` | PASS |
| Trace configure/build | macos-macports, tests and trace ON, `-j2` | PASS |
| Selftest | current macOS worker `--selftest` | PASS, including persistence controls |
| Persistence smoke | VA, VA2, explicit path, disabled mode | PASS |
| M74/M68/M69/M70 | five focused canonical tests including trace equivalence | PASS, 5/5 |
| Trace-disabled configure/build | Release, trace OFF, tests OFF, `-j2` | PASS |
| Trace-disabled behavior | plain script accepted; trace action rejected | PASS |
| MinGW configure/build | mingw-cross, tests and trace ON, `-j2` | PASS |
| Full ROM-less before CI repair | `ctest -L romless -j1` | 66 PASS, 5 FAIL, 1 SKIP of 72 |

The five current full-suite failures are
`vaeg_upd9002_m60b_authority_static`,
`vaeg_upd9002_m60c_audit_static`,
`vaeg_upd9002_m60d_frame_static`,
`vaeg_upd9002_m60e_iret_static`, and
`vaeg_upd9002_m61_mov_imm_static`. The first three reject unavailable
protected-history object `ba2b7d3f5c76646b30d63fd8951f4a1964817b15`; the last
two reject the unavailable symmetric-difference base
`8736f8afe6d8eeb58e58c7afdaf5951e2306cb63`. These are the same five
starting-state signatures documented by the pre-M74 hosted baseline and the
prior M74 report. The persistence extension introduced no additional failing
test. The external SST test is SKIP, not PASS.

Commit 7e7b805 removes four obsolete transitional milestone planning bundles
after confirming that the canonical roadmap and task files remain. Historical
SST inventories and reports were intentionally left unchanged because they
record their evaluated snapshots.

Hosted run 31369034095 evaluated persistence-report commit
`51ed50cf33dac0f681f32eaf93e20fa01f0033d7` and reproduced the same five
protected-history failures. No persistence or harness test failed.

## CI repair integration

The branch integrates the already validated CI repair from
`hotfix/ci-rewritten-history-identities` as two separate commits:

- `1ba7db1` updates the five protected-history validators to retain their
  evidence identities while using the corresponding post-rewrite Git-topology
  identities;
- `f3cb274` serializes the two tests that mutate the same SCSI selftest image
  with a CTest resource lock.

The source hotfix was independently exercised by hosted run 31365516073 at
`d87158a5553fe35395747f92b040532fbce572b8`; all nine jobs passed. After the
two commits were integrated here, the current local validation is:

| Check | Exact command or scope | Result |
|---|---|---|
| Encoding | `python3 tools/repo/check_encoding.py --expect utf8` | PASS, 0 violations |
| EOL | `python3 tools/repo/check_eol.py` | PASS |
| Case | `python3 tools/repo/check_case.py` | PASS, 0 findings |
| Diff | `git diff --check` | PASS |
| Native trace build | `cmake --build --preset macos-macports -j4` | PASS |
| Selftest | `build/macos-macports/sdl2/vaeg --selftest` | PASS |
| Full ROM-less | `ctest --test-dir build/macos-macports --output-on-failure -L romless -j4` | 71 PASS, 0 FAIL, 1 SKIP of 72 |
| MinGW cross-build | `cmake --build --preset mingw-cross -j4` | PASS |

The skipped test is the established external SST test and remains recorded as
SKIP. No ROM, disk, generated worker, or private integration artifact is part
of the CI repair.

## Gate status

The maintainer passed the required G74 human gate on 2026-08-11. M74 is
closed at the final `topic/m74-debug-harness` SHA
`3785cc115155c52928817b8c95d38b40268a7bde`. This is an explicit maintainer
approval, not an agent self-approval.
