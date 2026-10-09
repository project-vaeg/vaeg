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
# PC-Engine DOS redirector bridge probe

`R2FPROBE.COM` safely tests whether PC-Engine DOS forwards its public
redirection calls to the MS-DOS 3.1 network-redirector multiplex.

The probe does not install a drive, edit the CDS, remain resident, access a
host directory, or write guest media. It temporarily hooks `INT 2FH`, calls
the read-only assign-list query `INT 21H/AX=5F02H` and a deliberately rejected
assign request `INT 21H/AX=5F03H`, records any `INT 2FH/AX=111EH` callback, and
restores the original vector before printing its result. Its temporary
handler returns DOS error 1 for every callback other than the installation
query, so the attempted assignment cannot be committed.

Immediately after installing the hook, the probe calls `AX=1100H` itself and
requires `AX=11FFH, CF=0`. A failed self-test produces
`RESULT=PROBE_HOOK_FAILED` instead of drawing a bridge conclusion.

Screen output uses the documented PC-Engine console service
`INT 83H/AH=02H`; it does not assume that conventional MS-DOS character or
string-output functions are implemented by the PC-Engine environment.

Build and validate:

```sh
nasm -f bin -o r2fprobe.com tools/pc88va/hostfs/r2fprobe.asm
python3 tools/pc88va/hostfs/check_probe.py --input r2fprobe.com
```

Run `R2FPROBE.COM` from PC-Engine. The decisive line is one of:

```text
RESULT=DOS_REDIRECTOR_BRIDGE_PRESENT
RESULT=NO_DOS_REDIRECTOR_BRIDGE
```

`CALLS`, `LAST`, and `STACK` show the number of intercepted redirector calls,
the last `AX`, and the DOS-pushed public function word. A compatible bridge is
expected to report `LAST=111E` and `STACK=5F02` or `5F03`. The `5D06` line is
supporting SDA evidence only; it is not sufficient by itself.

PC-Engine's command screen requires the `.COM` suffix. To prepare a disposable
probe disk without modifying a source system disk:

```sh
python3 tools/pc88va/pcengine_disk.py vanilla \
  --source pcengine-system.d88 --output /tmp/pcengine-probe.d88
mkdir -p /tmp/pcengine-probe-payload/root
cp r2fprobe.com /tmp/pcengine-probe-payload/root/R2FPROBE.COM
python3 tools/pc88va/pcengine_disk.py install \
  --image /tmp/pcengine-probe.d88 --payload /tmp/pcengine-probe-payload
```

Boot that copy and enter `R2FPROBE.COM` at `Ready`.

## Accepted emulator observation

The PC-Engine 1.0 environment tested by M56 produced:

```text
DOS=02.00
INT2F/1100 before hook AX=1100 CF=0
INT2F/1100 after hook AX=11FF CF=0
INT21/5D06 AX=0001 CF=1
INT21/5F02 AX=0001 CF=1 CALLS=0000 LAST=0000 STACK=0000
INT21/5F03 AX=0001 CF=1 CALLS=0000 LAST=0000 STACK=0000
RESULT=NO_DOS_REDIRECTOR_BRIDGE
```

Because `5D06H` failed, its displayed pointer is undefined and is deliberately
omitted above. Both public redirection calls failed without invoking the
temporary `AH=11H` hook. A conventional DOS network redirector therefore
cannot service ordinary PC-Engine file operations in this environment.
