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
# M56 HOSTFS redirector prerequisite closure

## Gate disposition

G56 was administratively closed by the maintainer on 2026-07-23.

Approved G56 SHA:

```text
b72e641733ddea6f0e8faef2507093f7c3aee5a4
```

M56 was closed because the required DOS redirector bridge was absent. This is
not evidence of a successful HOSTFS implementation.

## Repository identity

- M56 branch: `topic/m56-hostfs-readonly-redirector`
- M56 prerequisite-stop commit:
  `9062b23966f3e5c1c3577fbc4fed51d8c3d35eb6`
- Approved integration commit:
  `b72e641733ddea6f0e8faef2507093f7c3aee5a4`
- Integration commit parents:
  `27894562fdc20e3ab3ed444ea14b75bab5d2a9da` and
  `9062b23966f3e5c1c3577fbc4fed51d8c3d35eb6`

The approved integration commit contains the complete M56 branch history.

## Accepted evidence

The non-resident probe established that PC-Engine reports a DOS 2.00
interface, while the public `INT 21H/AX=5F02H` and `5F03H` calls fail without
entering a temporary, independently verified `INT 2FH/AH=11H` hook. The
conventional DOS network-redirector design therefore cannot provide the
requested transparent HOSTFS drive in the accepted environment.

The accepted evidence and interpretation remain in:

- `docs/agents/tasks/M56_hostfs_readonly_redirector.md`
- `docs/agents/research/m56_pcengine_redirector_probe.md`

## Acceptance boundary

The administrative closure accepts only the prerequisite probe and the
decision to stop the infeasible implementation. It does not claim that:

- `HOSTFS.COM` was implemented;
- transparent `DIR`, `TYPE`, program loading, or copy-out was provided;
- the dormant HOSTFS implementation checklist passed; or
- an alternate PC-Engine file-service bridge was authorized.

The existing read-only HOSTFAT path remains unchanged.
