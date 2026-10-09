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

# M74 - Deterministic emulator debug harness

M74 builds reusable diagnostic infrastructure for bounded, deterministic
emulator investigation without changing guest-visible behavior.

Predecessor: current `main` after the approved G77 integration. M74 is
independent of the already completed M75-M78 work.

Branch: `topic/m74-debug-harness`

Commit prefix: `M74:`

Candidate gate: `G74`

Report: `docs/agents/reports/m74_debug_harness.md`

Do not start a dependent production-fix milestone from this task. The
maintainer passed the G74 human gate; M74 is closed.

## Scope

M74 owns diagnostic infrastructure only:

- define a default-off fixed-address counter API;
- define bounded one-shot and ordinal-selected capture APIs;
- provide deterministic reset-, frame-, event-, and command-window snapshots;
- isolate model-specific persistent diagnostic state;
- provide stable machine-readable schemas for counter and capture output;
- provide a reusable command runner that accepts only neutral test identifiers;
- record source, worker, runner, model, guest-bound, and output identities;
- add ROM-less tests for disabled equivalence, bounds, ordinal selection, and
  schema stability;
- document how private integration inputs remain outside Git and public logs.

## Accepted operator contract

The primary M74 workflow is:

> Inject input at a specified guest frame, start a bounded CPU trace when a
> specified PC ordinal is reached, capture registers, TVRAM, and the rendered
> screen at that same event, and exchange an FDD during the same run.

M74 provides this workflow through one versioned, sequential debug script.
The initial command set is:

```text
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
```

Declarations do not execute guest actions. Runtime commands execute in file
order. The required `limit-frame` declaration stops the run at a deterministic
guest-frame bound even when a selected PC is never reached. A `wait-frame`
action uses completed guest frames, never host elapsed time. A `wait-pc` action counts pre-instruction appearances of the exact
`CS:IP` pair after that action is armed.

When the selected PC ordinal is reached, the CPU pauses before executing the
matched instruction. Consecutive `trace` and `capture` actions therefore
observe one architectural event: the register snapshot is the pre-instruction
state, the trace begins with the matched instruction, and TVRAM/rendered-screen
capture occurs before guest execution resumes. The pause must not advance the
guest clock, consume the instruction, or count as a guest frame.

Media paths are declared behind neutral resource identifiers. Event and result
logs contain only those identifiers; they do not emit the script path, output
directory, or resource path. Capture identifiers are restricted to stable
lowercase ASCII names suitable for deterministic filenames.

The legacy `--headless-input-script`, end-of-run screen capture, and
startup-only `--trace-cpu` interfaces remain compatible. The new integrated
workflow uses `--debug-script` with `--debug-output-dir`; it may not be
combined with startup-only `--trace-cpu`.

## Accepted persistence isolation contract

M74 isolates host persistence so deterministic runs do not accidentally share
state across working directories or machine models:

- `vaeg.cfg` is read and written in the process current working directory by
  default;
- VA backup memory defaults to `vabkupmem.dat`, while VA2/VA3 defaults to
  `va2bkupmem.dat`;
- `--cfg path` and `--bkupmem path` select exact files, with relative paths
  resolved from the process current working directory;
- `--no-cfg` and `--no-bkupmem` disable both reads and writes for that state;
- explicit path and disable options for the same state are mutually exclusive;
- no executable-directory or user-state fallback/migration is performed.

A missing selected file starts from built-in/default state and is created by
the existing normal save lifecycle.

## Required invariants

- Diagnostics are disabled by default.
- Disabled diagnostics do not change deterministic guest-visible state.
- Captures are bounded by explicit address, ordinal, event, and output limits.
- The harness does not implement per-instruction session tracing.
- The harness performs no guest-visible writes unless a separately authorized
  disposable intervention explicitly requires them.
- Private ROMs, disks, screenshots, save data, filenames, paths, and hashes are
  never committed, uploaded, packaged, or emitted by default.
- Public evidence uses neutral stable identifiers and synthetic or ROM-less
  fixtures.
- Diagnostic code remains separate from production mapping and execution
  semantics wherever practical.

## Non-goals

M74 does not investigate or correct a particular guest-software failure. It
does not change CPU, memory, I/O, storage, video, sound, or ROM-loading
semantics. Persistence changes are limited to the accepted isolation contract
above. M74 does not import private integration evidence into the repository.

## Deliverables

- a documented diagnostic API and lifecycle;
- a reusable deterministic runner;
- bounded counter and capture schemas;
- focused ROM-less tests and negative tests;
- disabled-versus-enabled deterministic equivalence evidence;
- a report containing no private integration identity or payload.

## Validation

Run repository encoding, EOL, case, and diff checks; the trace-enabled build;
selftests; the ROM-less suite; focused counter/capture tests; disabled and
enabled deterministic-equivalence tests with synthetic inputs; native builds;
and the established MinGW/cross-build validation where available.

The maintainer passed the required human gate; G74 is closed.
