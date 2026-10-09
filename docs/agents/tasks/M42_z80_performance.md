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
# M42: Optional profiling-driven Z80 performance configuration

Status: optional draft; do not schedule automatically after G41

Branch: `topic/m42-z80-performance`

## Entry condition

Begin only after the maintainer passes G41 and profiling of representative VA
workloads shows the subsystem Z80 consumes enough host time to justify change.
If not, record that result and do not implement an optimization.

## Evaluation

Benchmark raw core and wrapper separately with deterministic programs. Report
iterations/instructions per second, wrapper and callback overhead, debug/
release mode, compiler, architecture, and architectural/memory checksums.
Evaluate each supported upstream switch independently, including
`Z80_NO_FUNCTIONAL`, debug/breakpoint/nesting/exception controls. Never enable
`Z80_CALLBACK_PER_INSTRUCTION` if it weakens fine-grained clock accounting.

Accept a setting only when improvement is measurable, architectural and bus
output is identical, all conformance/integration/state/disassembly/CI tests
remain green, public interfaces do not regress, and debug builds retain useful
diagnostics. Record accepted and rejected configurations with measurements in
ADR-0011 or a superseding ADR.

## Gate G42

Run full permanent QA and representative human smoke after each accepted
configuration. Report baselines, statistical method, exact commands/results,
SHAs, and behavior-neutral evidence. Push and stop at G42.
