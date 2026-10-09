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
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->
# M56 PC-Engine DOS redirector-bridge probe

## Question

Can an independently authored DOS network redirector receive ordinary
PC-Engine file operations through the conventional `INT 2FH/AH=11H`
multiplex?

This is a prerequisite question. It does not test a HOSTFS backend and does
not claim that the absence of this bridge prevents every possible custom
PC-Engine file-sharing design.

## Probe construction

[`r2fprobe.asm`](../../../tools/pc88va/hostfs/r2fprobe.asm) builds a flat,
non-resident 8086 `.COM` program. It:

1. records the reported DOS version and the pre-existing `INT 2FH/AX=1100H`
   response;
2. records the read-only `INT 21H/AX=5D06H` SDA query;
3. saves and temporarily replaces the `INT 2FH` vector;
4. calls `INT 2FH/AX=1100H` and requires `AX=11FFH, CF=0` to prove the
   temporary handler is active;
5. invokes the read-only redirection-list query `INT 21H/AX=5F02H`;
6. invokes `INT 21H/AX=5F03H`, while its handler rejects every attempted
   redirector callback with error 1 before an assignment can be committed;
7. restores the original vector before displaying any result; and
8. returns without staying resident or modifying guest media.

The probe prints through the PC-Engine console service
`INT 83H/AH=02H`. Its deterministic structural checker verifies the expected
calls, result markers, output path, and flat-binary size. The final binary is
1,354 bytes with SHA-256:

```text
74554f9c0fd2f1aa645444dcfaf13b2ba4e274b164c1935cde0bec59faf1d780
```

No private system-disk identity, path, or digest is part of the committed
evidence. A disposable copy of a maintainer-authorized PC-Engine system disk
was used for the emulator run; the source disk was not modified.

## Normalized result

```text
DOS=02.00
INT2F/1100 before hook AX=1100 CF=0
INT2F/1100 after hook AX=11FF CF=0
INT21/5D06 AX=0001 CF=1
INT21/5F02 AX=0001 CF=1 CALLS=0000 LAST=0000 STACK=0000
INT21/5F03 AX=0001 CF=1 CALLS=0000 LAST=0000 STACK=0000
RESULT=NO_DOS_REDIRECTOR_BRIDGE
```

The pointer returned in `DS:SI` after the failed `5D06H` call is undefined
and is intentionally excluded from this normalized record.

## Interpretation

- PC-Engine reports a DOS 2.00-compatible interface in this environment.
- The pre-hook installation query did not report an installed network
  redirector (`AL` remained zero).
- The post-hook self-test returned `AX=11FF, CF=0`, proving that the temporary
  handler was installed and callable before the DOS redirection functions ran.
- Both public redirection calls failed with `AX=0001, CF=1`.
- Most importantly, both calls recorded `CALLS=0000`; DOS never entered the
  temporary `AH=11H` handler.

Installing a conventional `INT 2FH/AH=11H` redirector therefore cannot make
ordinary PC-Engine file operations reach HOSTFS. Writing the remaining TSR
and emulator RPC would produce an unreachable implementation, so M56 stops
before doing so.

## Decision boundary

The following are outside the authorized M56 design and require an explicit
new decision:

- patching or replacing PC-Engine/DOS file-service dispatch;
- intercepting undocumented PC-Engine internal file-manager entry points;
- hooking broad `INT 21H` services and reproducing DOS internals;
- offering a non-transparent copy utility instead of a drive; or
- replacing HOSTFAT's accepted block-device model.

Until such a design is approved, the accepted read-only HOSTFAT path remains
the supported host-to-guest transfer mechanism.

## Maintainer disposition

On 2026-07-23 the maintainer administratively closed G56 at approved SHA
`b72e641733ddea6f0e8faef2507093f7c3aee5a4` because the prerequisite
redirector bridge is absent. This closure is not evidence of a successful
HOSTFS implementation and does not authorize an alternate file-service
bridge.
