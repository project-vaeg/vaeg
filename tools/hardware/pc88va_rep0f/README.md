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
# PC-88VA REP+0F evidence probe

This directory builds eight standalone DOS `.COM` probes.  It is research
tooling only and is not linked into VAEG.  Each binary executes exactly one test
instruction and prints one `before` and one `after` record beginning with
`VAEG_REP0F,`.  INT 1 and INT 6 are temporarily hooked; the handler records the
saved IP, FLAGS, registers, segment registers, trap number, and guard memory,
then redirects to a recovery label.

The current M47 decision remains unresolved until these binaries are run on a
PC-88VA.  Emulator output and hardware output are evidence of different kinds
and must never be combined or relabelled.

## Build and identity check

Use NASM and an output directory outside Git:

```text
python3 tools/hardware/pc88va_rep0f/build_probe.py check \
  --output /tmp/vaeg-m47-rep0f-probes
```

The command rebuilds every binary and checks it against `probe_manifest.json`.
No generated `.COM` file is tracked.

## Safe hardware procedure

1. Boot a disposable DOS environment on the PC-88VA without writable test
   media mounted.  Keep the probe media write-protected.
2. Run only one `R0Fnnn.COM` per invocation.  Photograph or serial-capture
   the `before` line before waiting for completion.
3. If the `after` line appears, capture it verbatim and reboot before the next
   case.  If the machine hangs or resets, record the case as `hang` or `reset`,
   power-cycle, and do not infer any state from the previous boot.
4. Run r001 first, followed by r002 and r003.  Run r004--r008 only after those
   controls recover correctly.  r005/r006 use AX=0000 and therefore do not ask
   LMSW-shaped behavior to set MSW_PE.
5. Do not add a LOADALL286 case.  It can replace CS:IP, FLAGS, descriptors, and
   stack state from a fixed physical image and is not generally recoverable.

The probe never opens or writes a file.  DOS console services are used only
before the test and after the exception handler has redirected to safety.

## Result normalization

Save raw emulator and real-hardware captures in different untracked
directories.  Normalize one successful two-line capture with an explicit
source label:

```text
python3 tools/hardware/pc88va_rep0f/parse_results.py \
  --source hardware /tmp/hardware-r001.log
```

The JSON emitted on stdout includes the raw-log SHA-256 but no local path or
private machine/media identity.  A hang or reset has no `after` line and must be
recorded manually as an unresolved outcome; the parser intentionally rejects it
as a completed result.
