# M75 SCSI support progress report

Status: implementation in progress; G75 is pending and has not been
self-approved.

## Starting point and reviewed history

M75 was restarted from the current `main` integration point:

```text
starting SHA: 766a132ff6d66e335fe9bb1d0082d777a4a8fe14
branch: topic/m75-scsi-support
task-authority: 869b59f211ed7b9f3e87f3627a5ff769fe7ec2ff
```

The historical M75 commits supplied for reference were inspected. The GUI
SCSI slots, image creation, command-line attachment, ROM detachment, and
documentation commits were reapplied as separate commits. The historical
lightweight trace commit `6e1b65d` was not copied because it depended on
discarded branch-local diagnostic interfaces and would have mixed an unrelated
diagnostic seam into the clean M75 branch.

## Implemented boundary

The board ROM remains detached by default. The active SCSI path is split into:

1. the existing PCPLUS software SCSIBIOS compatibility entry;
2. the C-Bus controller register/data path;
3. the existing SxSI image backend.

The controller now retains the target-controlled phase state after SELECT and
implements the WD33C93 register/PIO boundary through the COMMAND transfer
completion point. The active low-level AR=18h/19h path does not yet decode
the CDB or advance to DATA IN/OUT, STATUS, or MESSAGE IN. The VA I/O
registration includes the inherited `0CC6h` byte stream as a legacy
compatibility path; the SCSI55 document independently specifies `0CC0h`,
`0CC2h`, and `0CC4h`, while guest-level evidence for a separate `0CC6h`
hardware designation remains pending.

The transfer-length counter uses the pre-existing serialized `cmdpos` slot.
The removed board-ROM storage is represented by reserved padding of the same
size, so `_SCSIIO` retains its historical serialized size while no longer
owning or mapping board firmware bytes. This is a state-layout preservation,
not a new save-state field or compatibility format.

The default expansion interrupt selection is INT2/IRQ6, avoiding the SASI
INT3/IRQ9 collision described by the supplied PC-88VA documentation.

## M75a WD33C93 trace checkpoint

The M75a correction is that the active controller is a WD33C93-family host
interface.  The specification boundary is therefore the register and
interrupt contract at `0CC0h`/`0CC2h`, not an independently invented physical
REQ/ACK state machine.  The auxiliary status DBR bit is the PIO data-ready
handshake; CBSY, CIP, and INT are also host-visible status.  AR `19h` is a
fixed DATA window, while the CDB registers `03h`-`0Eh` and NEC extension
registers `30h`-`35h` remain addressable controller registers.

The event-driven status sequence required for the low-level SELECT path is:

```text
SELECT complete  11h
COMMAND request  8Ah
DATA request     89h/88h
STATUS request   8Bh
MESSAGE request  8Fh
disconnect       85h
```

The temporary delayed-`8Ah` experiment was discarded before M75a.  It was a
diagnostic probe only and is not part of the production state machine.

The trace-only implementation is commit `9deafb3` and adds the disabled-by-
default `--scsitrace` option.  It records AR selection, `0CC2h` register
accesses, `0CC4h` and `0CC6h` accesses, `CS:IP`, controller status/phase,
auxiliary status, and the configured SCSI IRQ assertion/EOI clear.  It does
not alter guest state or timing when the option is absent.

Trace command used:

```text
SDL_VIDEODRIVER=dummy gtimeout -k 1 8 build/linux-debug/sdl2/vaeg \
  --model va2 --roms docs/roms \
  --fdd1 pcengine110-scsi-support.d88 \
  --scsi1 /tmp/m75-schd-40mb-512.hdd --scsitrace --nowait --mute
```

Observed predecessor boundary:

```text
AR=15h/18h, SELECT without ATN
SCSI status 11h, phase COMMAND, IRQ6 asserted
PCPLUS reads auxiliary status and status register, then clears IRQ6 by EOI
no CDB DATA-window access follows before the run timeout
```

This proves the current stopping point without claiming that a delayed timer
is a valid fix.  The next M75 checkpoint must determine why the target phase
request is not being exposed through the WD33C93 command/DBR contract.

### Full boot-to-SELECT access audit

The trace was rerun from emulator startup through the first SELECT, without
an output window or event filter.  It captured all accesses made through the
registered `0CC0h`/`0CC2h` callbacks.  The observed AR sequence includes:

```text
33(read) 02(write/read) 17(read) 33(read) 30(read/write)
33(read) 00(write) 18(write) 33(read) 17(read) 16(read)
01(write) 02(write) 11(write) 15(write) 16(write)
33(read) 33(read) 15(write) 18(write) 33(read) 17(read) 16(read)
33(read)
```

The trace seam therefore does not filter out the extended AR range: AR `30h`
and AR `33h` are visible.  No access to AR `31h`, `32h`, `34h`, or `35h` was
made by this PCPLUS initialization path before SELECT.  AR `00h`, `01h`, and
`02h` were observed, so their absence from an earlier excerpt was a window
selection issue rather than an untraceable register path.

The first low-level command is:

```text
AR=15h <- 00h
AR=18h <- 07h
CSR=11h, phase=COMMAND, IRQ6 asserted
PCPLUS reads Aux Status, then AR=17h and AR=16h
EOI clears the emulated PIC request
```

No AR `19h` CDB byte, DBR polling sequence, or AR `12h`-`14h` transfer count
appears after this point.  This confirms that the missing boundary is between
the target's COMMAND-phase request and the WD33C93 host-visible event; it is
not yet evidence about DATA, STATUS, or MESSAGE IN handling.

The observed `EOI` is an 8259-side request clear in the current emulator
trace.  M75b/c must separate that from the WD33C93 device-INT latch: reading
AR `17h` must consume the CSR, while EOI alone must not clear the device
event or overwrite a pending second event.

### Register values from the complete trace

The value-bearing trace fixes the initial contract inputs as follows:

| AR/port | access | value | interpretation at this checkpoint |
|---|---|---:|---|
| `33h` | read | `17h` | controller-provided RESET/INT/ID value |
| `02h` | write/read | `80h` | timeout value is written and read back |
| `17h` | read | `00h` | reset status is consumed |
| `0CC4h` | write | `02h` | DMER reset; DMA is explicitly disabled |
| `30h` | read/write | `00h`/`04h` | memory-bank register is probed and enabled |
| `00h` | write | `07h` | controller own ID |
| `18h` | write | `00h` | explicit controller RESET |
| `01h` | write | `08h` | PIO mode (`DmaModeSelect=000b`) with ending-disconnect policy set |
| `11h` | write | `00h` | synchronous-transfer setup |
| `15h` | write | `07h`, then `00h` | controller setup, then target ID 0 SELECT |
| `16h` | write | `00h` | source ID setup |
| `18h` | write | `07h` | SELECT without ATN |

The `18h <- 00h` RESET was present in the complete trace; it was omitted from
the earlier abbreviated report.  The `01h`, `02h`, `30h`, and `33h` values are
now recorded as evidence rather than inferred defaults.  The current emulator
maps the observed `33h=17h`/INT2 choice to VA IRQ6; the permanent
implementation must derive that mapping from one controller configuration
source instead of maintaining independent constants.

After the final EOI/IRQ clear, no further SCSI port access occurs before the
run timeout.  Two independent six-second runs produced 71 `scsitrace` records
each and byte-identical filtered output (`cmp` exit 0).  This makes the
M75b1 baseline deterministic and proves that the stop is an interrupt-wait
state, not a noisy polling loop.

## M75b1 single-depth CSR checkpoint

M75b1 adds a runtime-only CSR latch and one pending event slot without changing
the serialized `_SCSIIO` image. A new event is scheduled only when no CSR is
latched or in flight. While the CSR is unread, one successor is retained;
additional events are rejected at the admission boundary for the bus layer to
back-pressure rather than overwriting the visible CSR. Reading AR `17h`
consumes the latch and admits the single pending event. 8259 EOI handling is
not used to consume this latch.

M75b1 commits:

```text
821edef M75b1: add single-depth WD33C93 CSR latch
0fabeb3 M75b1: validate single-depth CSR latch
```

Validation:

```text
cmake --build build/linux-debug --target vaeg_sdl2 -j2       pass
python3 tools/qa/m75_scsi_controller.py --root .           pass
vaeg --selftest                                             pass
two filtered --scsitrace runs: 71 records each, cmp exit 0
M75a baseline vs M75b1 filtered trace: cmp exit 0
```

The bus-side phase back-pressure and guest-visible 17h/EOI semantics are not
yet claimed complete; phase back-pressure and the post-8Ah command/data
sequence remain the M75c implementation scope.

## M75b2 WD33C93 PIO register checkpoint

The WD33C93A primary data sheet defines indirect-register auto-increment with
three exceptions: Auxiliary Status, DATA, and COMMAND.  M75b2 applies those
exceptions to the active 0CC0h/0CC2h path.  AR `19h` is now a fixed DATA
window, and AR `18h` is a fixed COMMAND window.  DATA reads and writes use
the existing controller buffer positions and do not advance the selected AR.

The Auxiliary Status value is composed from the serialized controller status
plus the runtime CSR latch:

```text
bit 7 INT:  set while a CSR is latched; cleared only by reading AR=17h
bit 6 LCI:  retained stored status bit (currently zero unless raised later)
bit 5 BSY:  level-II controller operation is active
bit 4 CIP:  the last command is being interpreted
bit 1 PE:   parity-error state (currently zero)
bit 0 DBR:  PIO DATA window is ready
```

Reading 0CC0h no longer clears Auxiliary Status.  In particular, an 8259
EOI only clears the emulated PIC request; it does not consume the WD33C93 CSR
latch.  AR `17h` consumption is the device-side clear and admits the single
pending CSR event recorded by M75b1.

The observed `0CC4h <- 02h` is recorded as the DMER reset strobe.  M75 remains
PIO-only: TCIR, TCMR, TCMS, and DMES are not implemented, and a trace warning
marks any such unsupported strobe as `hardware-pending`.  No DMA path or DMA
channel is synthesized.

AR `30h` and `31h` retain the existing memory-bank/window state.  AR `32h`,
`34h`, and `35h` return the documented open/unsupported value (`FFh`) and
emit a `hardware-pending` trace warning; no speculative package or FIFO
behavior is added.  AR `33h` remains the read-only controller RESET/INT/ID
value used by the observed VA IRQ6 setup.

M75b2 commits:

```text
82c65ee M75b2: add WD33C93 register contract tests
7b5672b M75b2: implement WD33C93 PIO register windows
96419fa M75b2: validate WD33C93 register windows
```

Validation after M75b2:

```text
cmake --build build/linux-debug --target vaeg_sdl2 -j2       pass
python3 tools/qa/m75_scsi_controller.py --root .           pass
vaeg --selftest                                             pass
scsitrace boot through SELECT                                 pass
```

The post-SELECT trace still stops before AR `12h`-`14h`, AR `18h`=`20h`, and
AR `19h` CDB bytes.  Therefore M75b2 proves the register-window and
Auxiliary-Status boundary but does not claim the target phase request or
full PCPLUS/SCHD command progression.  Those remain M75c evidence.

The register progression is intentional and tested: reading AR `17h`
increments the selected address to AR `18h`, while a subsequent write lands
on the fixed COMMAND window.  AR `12h`-`14h` therefore accept three
successive transfer-count bytes; AR `18h` and AR `19h` do not advance.  AR
values `1Ah`-`2Fh` are held and produce a `hardware-pending` warning rather
than an invented wrap or increment rule.

The existing `scsiioint()` path gates delivery to the VA IRQ line on
`membank bit 2` (IRE1).  The CSR latch is still updated internally when IRE1
is clear, but no PIC request is asserted; this keeps device state and the
system interrupt wire separate.  AR `31h` remains the implemented memory
window register.  Auxiliary Status LCI (bit 6) and PE (bit 1) are defined but
currently return zero; neither speculative command-ignore nor parity behavior
is claimed by M75b2.

The M75c work is split without creating additional human gates:

```text
M75c1: expose the target COMMAND-phase Service Required event (8Ah) only.
       DoD ends after AR=12h-14h and AR=18h <- 20h are observed.

M75c2: implement Transfer Info PIO byte pumping through fixed AR=19h.
       DoD observes AR=19h bytes and CSR=1Ah; later phases are not yet active.
```

The 8Ah event must be back-pressured behind the unread 11h CSR rather than
generated in the same simulation call.  CDB decoding must wait until the
host-programmed transfer count reaches zero; no command-group length is
allowed to substitute for that observed count.

## M75c1 Service Required checkpoint

M75c1 now records a successful SELECT as two distinct controller events.  The
first event exposes CSR `11h` and latches a pending COMMAND-phase request.
That request is not generated in the same call.  Only after the host reads
AR `17h` (which advances AR to `18h`) is CSR `8Ah` scheduled and delivered.
This is the single-depth latch/back-pressure boundary required by the
WD33C93 initiator contract.

The deterministic guest trace reaches:

```text
CSR 11h read
CSR 8Ah delivered and read
AR 12h <- 00h, AR 13h <- 00h, AR 14h <- 06h
AR 18h <- 20h
M75c1 holds Transfer Info at COMMAND phase
```

The transfer count (`000006h` in this first observed command) is recorded as
evidence only; M75c2 must consume the host-programmed count rather than
assuming a six-byte CDB.  The predecessor's immediate CDB-copy path is
explicitly disabled at this checkpoint, so no `8Bh`, STATUS transition, or
AR `19h` access is produced by M75c1.

M75c1 commits:

```text
8df28c7 M75c1: add deferred command-request tests
99222f4 M75c1: protect Transfer Info boundary
9b4376d M75c1: defer command phase service request
```

The run is intentionally terminated by the bounded wall-clock harness after
the AR `18h` boundary (`exit=124`); the trace prefix through that boundary is
the acceptance evidence.  M75c2 must replace this bounded stop with a
deterministic transfer-count termination.

## M75c2 PIO CDB transfer checkpoint

M75c2 replaces the former immediate CDB copy for the low-level COMMAND phase.
When the host writes `AR=18h <- 20h`, the three transfer-count registers are
read as one 24-bit value and become the only active byte count.  Each AR `19h`
write is accepted only while DBR is set, is appended to the CDB buffer, and
leaves AR at `19h`.  The opcode/group is not decoded early and the count is
not inferred from the first byte.

When the host count reaches zero, the controller emits CSR `1Ah` (successful
Transfer Info completion in the COMMAND phase).  M75c2 does not yet advance
to DATA IN/OUT, STATUS, or MESSAGE IN and does not claim CDB command
execution.  A zero transfer count remains `hardware-pending` and is not
treated as a successful transfer.

The first deterministic PCPLUS trace now contains:

```text
CSR 8Ah
AR 12h/13h/14h <- 00h/00h/06h
AR 18h <- 20h
AR 19h <- six CDB bytes
CSR 1Ah
```

The same trace shows later host-programmed counts such as `24h`, `0Ah`, and
`08h`; this is why a fixed six-byte CDB assumption would be incorrect. The
M75c3 trace-only classification below establishes whether these are DATA IN
transfers or are incorrectly being consumed by the COMMAND path.

M75c2 commits:

```text
d04747f M75c2: add PIO transfer boundary tests
acf588f M75c2: pump PIO CDB bytes through DATA window
bffa7cf M75c2: validate PIO CDB completion
```

M75c3 commits:

```text
da3f469 M75c3: trace transfer phase classification
4047f58 M75c3: validate transfer phase tracing
1260764 M75c3: document transfer phase classification
```

## Existing command-helper coverage (not active low-level AR=19h execution)

| CDB | Existing `scsicmd_cmd()` helper | Active low-level path |
|---|---|---|
| `00h` TEST UNIT READY | successful status when a SCSI image is mounted | not reached from M75c2 |
| `12h` INQUIRY | fixed direct-access HDD identification, allocation-length bounded | not reached from M75c2 |
| `25h` READ CAPACITY (10) | big-endian last LBA and logical block length | not reached from M75c2 |
| `1Ah` MODE SENSE (6) | direct-access header and one block descriptor, allocation-length bounded | not reached from M75c2 |

The helper table is retained for the existing BIOS compatibility path. The
active low-level controller path currently leaves `scsiio.phase` at COMMAND,
does not call `scsicmd_cmd()`, and reports CSR `1Ah` after every host-counted
AR=19h transfer. DATA IN/STATUS/MESSAGE behavior is therefore not claimed.

## M75c3 transfer-phase classification checkpoint

M75c3 adds trace-only accounting; it does not change guest behavior. Each
`18h <- 20h` TRANSFER INFO records the controller phase, direction, host
transfer count, source path, the number of AR=19h accesses, the number of
legacy 0CC6h data-port accesses, and the resulting CSR. A bounded run with
the PC-Engine 1.1 SCSI support disk produced:

```text
exit=124 (bounded wall-clock stop)
phase=1Ah (COMMAND) for every observed transfer
direction=host-to-spc for every observed transfer
source=m75c2-ar19-pio for every observed transfer
tc=000006 -> ar19_accesses=6,  csr=1Ah
tc=000024 -> ar19_accesses=36, csr=1Ah
tc=00000A -> ar19_accesses=10, csr=1Ah
tc=000008 -> ar19_accesses=8,  csr=1Ah
data_port_accesses=0 in the classified records
```

The direction and interrupt columns are now explicit. For the same records,
all AR=19h accesses were writes (`ar19_reads=0`, `ar19_writes=TC`), and each
transfer produced exactly one CSR event request and one VA IRQ6 assertion:

```text
tc=000024 ar19_reads=0 ar19_writes=36 irq_requests=1 irq_assertions=1
tc=000008 ar19_reads=0 ar19_writes=8  irq_requests=1 irq_assertions=1
```

There were no per-byte IRQs; the single assertion occurred at transfer
completion. This is the concrete M75c3 load test for the depth-one CSR latch
and back-pressure boundary.

The captured CDB bytes resolve the command sequence without inference:

```text
tc=000006 cdb=12 00 00 00 24 00
tc=000024 cdb=00 00 00 00 00 00 00 00 00 00 00 00
tc=00000a cdb=25 00 00 00 00 00 00 00 00 00
tc=000008 cdb=00 00 00 00 00 00 00 00
tc=000006 cdb=1a 00 04 00 24 00
```

Thus `12h` with allocation length `24h` and `25h` followed by length `08h`
are confirmed. The `24h` and `08h` records themselves contain response
bytes treated as COMMAND data, proving the (b) result: the low-level path has
not yet transitioned to DATA IN.

The `24h` and `08h` values are therefore not evidence that DATA IN is already
implemented in the current branch. They are host-programmed counts consumed
by the still-COMMAND M75c2 path. Their lengths are consistent with an
INQUIRY response and READ CAPACITY response in the guest command sequence,
but the active controller has not decoded those CDBs or generated the DATA
IN/STATUS/MESSAGE phases. This is a demonstrated contract gap, not a reason
to weaken the M75c2 scope statement.

The trace also proves that the 0CC6h legacy data path is not the source of
these records. It is classified as unused by the supplied PCPLUS/SCHD path,
while the compatibility mapping remains retained for now. The next
implementation checkpoint must connect completed
COMMAND CDBs to the existing command helper only through a general phase
transition, then verify DATA IN with distinct CSR `19h`, followed by STATUS
and MESSAGE IN transfers with TC=1.

## Validation performed

The following checks passed on the current branch:

```text
cmake --preset linux-debug
cmake --build build/linux-debug --target vaeg_sdl2 -j2                  pass
cmake -S . -B build/m75-tests -DCMAKE_BUILD_TYPE=Debug \
  -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_INTEGRATION_TRACE=ON               pass
cmake --build build/m75-tests --target vaeg_sdl2 -j2                   pass
python3 tools/qa/m75_scsi_controller.py --root .                      pass
ctest --test-dir build/m75-tests -R vaeg_m75_scsi_controller \
  --output-on-failure                                                  pass
python3 tools/qa/m75_scsi_controller.py --root .                      pass
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy vaeg --selftest            pass
vaeg --model va2 --roms docs/roms --scsi1 /tmp/m75-headless40/scsi40.hdd \
  --smoke --nowait --mute                                               pass
SDL_VIDEODRIVER=dummy gtimeout -k 2 12 build/linux-debug/sdl2/vaeg \
  --model va2 --roms /Users/maho/vaeg/docs/roms \
  --fdd1 /Users/maho/vaeg/pcengine110-scsi-support.d88 \
  --scsi1 /tmp/m75-scsi40.hdd --scsitrace --nowait --mute              exit=124;
                                                                          required M75c3
                                                                          transfer records present
```

The ROM override was added so validation can use the maintained local ROM
directory without copying or modifying ROM payloads. The smoke run verified
the complete VA2 ROM set and accepted the SCSI VHD image. The full guest
PCPLUS/SCHD registration path, SCFORM, and file operations remain the M75
manual validation gate and have not yet been claimed as passing.

## Commits on the current branch

```text
869b59f M75: define SCSI support cleanup and validation
26a140a M75: add SCSI HDD menu slots
99a1b1b M75: add SCSI image creation menu
d468011 M75: add command-line SCSI attachment
d3ccebe M75: detach SCSI board ROM by default
cce1d4b M75: document PCPLUS software SCSIBIOS path
1c8d82a M75: document SCSI port and SCSIBIOS evidence
2f07711 M75: document SCHD driver evidence
32942be M75: document SCSI GUI mounting
f341f85 M75: document SCSI image creation
c246e71 M75: document command-line SCSI attachment
93f7e79 M75: implement SCSI command phases and media geometry
859dd72 M75: validate SCSI controller phase contract
7166336 M75: signal SCSI phase completion interrupts
1a82af6 M75: add explicit ROM directory override
9d6ecdb M75: document explicit ROM directory validation
54f65c7 M75: preserve SCSI state layout for transfer length
d2001aa M75: align SCSI validator with state layout
602f6ab M75: document SCSI phase implementation progress
82c65ee M75b2: add WD33C93 register contract tests
7b5672b M75b2: implement WD33C93 PIO register windows
96419fa M75b2: validate WD33C93 register windows
```

## Remaining M75 gate

The branch must still be exercised with the user-supplied
`pcengine110-scsi-support.d88` and a disposable SCSI image. The maintainer
should confirm PCPLUS loads before SCHD, SCHD registration completes, SCFORM
can initialize the target, and a reboot can create/read/delete a test file.
SASI, HOSTFAT, and the existing non-SCSI disk paths must remain unchanged.

G75 remains a human gate. This report does not declare G75 passed and does
not start M76.

## M75d1 phase-contract checkpoint

The M75c3 trace classified the apparent 36-byte and 8-byte DATA transfers as
host-to-controller writes while the controller still reported COMMAND
(`1Ah`).  The captured CDBs were `12 00 00 00 24 00` and `25 00 00 00 00
00 00 00 00 00`, so the host is following the reported phase rather than
inventing a direction.  M75d1 therefore makes the target's next phase the
single source of the service-request code, direction, and transfer pump
selection.

The phase contract table is:

| internal phase | service request | direction |
|---|---:|---|
| DATA OUT (`18h`) | `88h` | host to controller |
| DATA IN (`19h`) | `89h` | controller to host |
| COMMAND (`1Ah`) | `8Ah` | host to controller |
| STATUS (`1Bh`) | `8Bh` | controller to host |
| INFORMATION OUT (`1Ch`) | `8Ch` | host to controller |
| INFORMATION IN (`1Dh`) | `8Dh` | controller to host |
| MESSAGE OUT (`1Eh`) | `8Eh` | host to controller |
| MESSAGE IN (`1Fh`) | `8Fh` | controller to host |

The table is shared by the phase decoder and transfer trace; no second
direction switch is used.  CDB completion now invokes the existing target
command helper, records the next service request behind the CSR latch, and
keeps the pump phase-neutral.  DATA IN, STATUS, and MESSAGE IN transfers use
the same fixed AR `19h` PIO path, with completion CSRs `19h`, `1Bh`, and
`1Fh` respectively.  A command-complete message byte is `00h`; bus-free is
`85h` only when Control bit 3 permits the ending-disconnect interrupt.

The implementation checkpoint is not yet a terminal acceptance.  The first
guest run reaches the corrected `8Bh` STATUS request after TUR, but the
PCPLUS handler does not yet issue the expected one-byte STATUS Transfer Info
sequence; it reads the transfer-count bytes and stops.  This is recorded as
an M75d1 integration blocker, not as a passing DATA/STATUS result.  The
phase table and the AR19 byte-pump path are covered by the static validator,
but the required INQUIRY DATA IN golden (`CSR=19h`, AR19 read x36) has not
yet been observed.

The deterministic trace option is now `--scsitrace-limit N`.  It requests
termination after N completed transfer records (the run-loop observes the
request at a frame boundary, so a final frame may contain additional records).
The old wall-clock timeout remains useful as a safety bound only; it is not
evidence of a completed phase sequence.

M75d1 local result:

```text
phase-contract validator: pass (all 8 phases)
Linux debug SDL2 build: pass
focused M75 CTest: pass
bounded PCPLUS/SCHD run: blocked after TUR STATUS request
INQUIRY DATA IN golden: not observed
G75: not eligible
```

#### Evidence reconciliation

An earlier intermediate handoff incorrectly described the current branch as
having no production changes after the M75c3 evidence and therefore mixed two
different execution states. The branch advanced through the following
commits before the current documentation checkpoint:

```text
00e1dba M75d1: derive SCSI phases from decoded commands
5abba93 M75d1: validate the shared SCSI phase table
c526872 M75d1: document phase integration blocker
```

The first two commits are production/validation changes, not merely report
edits. They connect COMMAND completion to `scsicmd_command()`, derive the
next target phase from the shared contract, and queue the next service
request behind the CSR latch. Consequently, the post-TUR `8Bh` records belong
to the post-`00e1dba` implementation state; the earlier M75c3 records, which
showed only COMMAND/`1Ah`, remain valid predecessor evidence.

The post-TUR diagnostic records are exact at the register boundary:

```text
event: CSR=8Bh, internal phase=1Bh, IRQ6 asserted
AR=17h read: 8Bh
AR=16h read: 00h
AR=13h read: 00h
AR=14h read: 00h
AR=18h write: absent
```

A separate diagnostic changed only the phase-transfer setup count to one;
that run returned `AR=13h=00h`, `AR=14h=01h` and still produced no
`AR=18h <- 20h`. The experiment was reverted and is not part of the current
source tree. It therefore does not establish that a count value alone is
the missing contract.

The `AR=16h` value is not an unobserved reset default in this path: the
complete initialization sequence writes `AR=16h <- 00h` before SELECT, and
the handler reads it after each service request. PCPLUS's status handler
tests the upper `ER/ES` bits of this register; the observed zero follows its
normal path and is not evidence that the service request was rejected.

The remaining blocker is therefore the handoff from the interrupt/status
handler to the PCPLUS phase dispatcher, not proof that `8Bh` was absent. The
next diagnostic must monitor the internal status byte written by the handler
and the first subsequent dispatcher read, while preserving the exact AR13,
AR14, and AR16 values above. No DATA IN, STATUS Transfer Info, or MESSAGE IN
acceptance is claimed until that handoff is observed.

## Static PCPLUS CSR acceptance set and handoff narrowing

The PCPLUS interrupt path was rechecked against the normalized status
dispatch table. Raw `8Ah` and `8Bh` both normalize to service-request dispatch
key `11h` and enter `1972h`. The successful `8Ah` path therefore proves that
the common interrupt/status handoff is alive. The next diagnostic boundary is
the phase comparison at `19BBh`, followed by the `1B60h`/`1BA1h` choice,
`1C14h` transfer setup, and the `1C32h` `AR=18h <- 20h` command.

The apparent `1C0Eh` branch is rejected by the disassembly. `1C0Eh` reads
`CS:[047Eh]` and returns; it does not branch to `1C32h`. `1C05h` calls
`1C95h` to read AR `13h`/`14h`, adjusts the residual count, reads `047Eh`, and
returns. `1C14h` is a separate function called by `1B60h` and `1BA1h`; its
`1C32h` path emits the Transfer Info command. Seeing AR `13h`/`14h` after
`8Bh` therefore proves entry into the residual-count path, but does not by
itself prove that `1C32h` was reached.

The canonical raw CSR values accepted by the PCPLUS contract and their static
dispatch destinations are:

| raw CSR | normalized/status key | static destination |
|---|---:|---:|
| `11h` | `01h` | `186Ch` |
| `16h` | `06h` | `1884h` |
| `18h` | `08h` | `1893h` |
| `19h`-`1Bh` | `09h`-`0Bh` | `1818h` |
| `1Fh` | `0Fh` | `1818h` |
| `42h` | `02h` | `1878h` |
| `48h` | `08h` | `1893h` |
| `49h`-`4Fh` | `09h`-`0Fh` | `1818h` |
| `85h` | `15h`, dispatch key `10h` | `1935h` |
| `88h`-`8Fh` | `18h`-`1Fh`, dispatch key `11h` | `1972h` |

This table is the M75 reference for which controller-generated statuses may
be emitted by the active PCPLUS/SCHD path. It is a host-contract table, not
a claim of complete WD33C93 silicon coverage.

The disassembly does not support labeling `00h`, `01h`, `10h`, `41h`,
`43h`-`47h`, `80h`-`84h`, `86h`, or `87h` as silently rejected: those values
also normalize into dispatch-table entries. Their semantic meaning remains
unverified. The validator must therefore enforce that VAEG-generated
statuses stay within the canonical set above, while treating other
dispatchable values as unverified rather than as proven no-op or rejection
cases.

No production behavior was changed by this extraction. The remaining
trace-only observation points are `19BBh` (comparison value and result), the
selected `1B60h`/`1BA1h` path, `1C14h` entry/exit, `1C32h` command emission,
and all relevant `047Eh` reads/writes.

## Latest headless integration check

After the diagnostic trace additions, the M75 branch was configured and built
with the trace-enabled `linux-ci-clang` preset. The build completed with exit
status 0 and produced:

```text
build/linux-ci-clang/sdl2/vaeg
SHA-256 d69b11ad7b9bc3427042d808d3d06c4a3900e50a6427b15032f0f51b43c58836
```

The headless run used the complete VA2 ROM directory and the user-provided
PC-Engine 1.1 SCSI support disk. A temporary 40 MB VHD-format target was
created outside the repository at `/private/tmp/m75-scsi-40mb.hdd` solely for
this check:

```text
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \\
  build/linux-ci-clang/sdl2/vaeg \\
  --model va2 \\
  --roms /Users/maho/vaeg/docs/roms \\
  --fdd1 /Users/maho/vaeg/pcengine110-scsi-support.d88 \\
  --scsi1 /private/tmp/m75-scsi-40mb.hdd \\
  --scsi2 none --scsi3 none --scsi4 none \\
  --scsitrace --scsitrace-limit 2 --nowait --mute
```

The executable started with the complete VA2 ROM set and reached the active
PCPLUS/SCSI path. The observed sequence was:

```text
SELECT             CSR=11h, IRQ6
COMMAND request    CSR=8Ah, IRQ6
Transfer Info      TC=6, AR=19h write x6
CDB completion     CSR=1Ah
next service       CSR=8Bh, IRQ6
```

The transfer accounting was:

```text
AR19 accesses       6
AR19 reads          0
AR19 writes         6
transfer IRQs       requested=1, asserted=1
```

The run did not reach DATA IN (`CSR=19h`), STATUS, or MESSAGE IN. It was
terminated by the external eight-second safety timeout with exit status 137;
this is a bounded diagnostic stop, not an emulator crash or a passing result.
The current M75 blocker is therefore reproduced: CDB PIO and the subsequent
`8Bh` service request are observable, but the PCPLUS phase handoff does not
yet emit the next `AR=18h <- 20h` Transfer Info command. G75 remains
ineligible.


## M75d1 target-phase readiness correction (current implementation)

The previous handoff diagnosis is now resolved at the controller boundary. The
raw `8Bh` value was normalized correctly by PCPLUS, but the controller exposed
the next target phase as soon as the previous CSR was consumed. The foreground
was still completing the preceding transfer, so the `8Bh` interrupt was picked
up by the main event pump (`1742h`/`1747h`) instead of the transfer wait path.
`1747h` clears memory `CS:[047Eh]` from `BBh` to `3Bh` and `1791h` branches
using the copied value; it does not reread the cleared byte.

The production correction is general and has no PCPLUS-address or CDB shortcut:

1. A completed transfer records the target's next phase as a pending event.
2. The host-visible `CSR` latch is still consumed only by an `AR=17h` read.
3. When the host issues `AR=18h <- 20h` for that pending phase, DBR is held
   low and a controller processing event is scheduled.
4. The event then exposes the service request. The event quantum is the
   existing 100-clock PIO controller event quantum; it is not a guest-tuned
   timer or an injected CSR.
5. If the target remains in the same phase, no processing event is inserted;
   the next PIO byte remains available. A phase transition alone requires the
   processing event.

The disassembly establishes that the command/status foreground wait is
`1B67h`/`1B73h`; `1CBDh` is a separate helper used by another transfer path and
is not claimed as the observed wait point. The corrected trace is:

```text
CDB 00 00 00 00 00 00
  CSR=1Ah, AR19 writes=6
  target-phase-wait phase=1Bh, TC=010000 (DBR held low)
  CSR=8Bh delivered at guest IP=1B67h
  AR17=8Bh, AR16=00h, AR13=00h, AR14=00h
  CS:[047Eh] after normalization/consumption = 3Bh
  1BA1h -> 1C14h -> 1C32h, AR18=20h
  AR19 read=00h, CSR=1Bh
  target-phase-wait phase=1Fh
  CSR=8Fh delivered at guest IP=1BB7h
  AR19 read=00h, CSR=1Fh
```

The same run then reaches the PCPLUS REQUEST SENSE command (`03 00 00 00
0D 00`). Its fixed no-sense response begins with `70h`, and the first DATA IN
transfer is observed as `CSR=19h`, AR19 read, data `70h`. The bounded trace did
not reach the later INQUIRY CDB (`12h`) before the safety limit; therefore the
INQUIRY 36-byte golden remains unclaimed. This is an evidence boundary, not a
claim that INQUIRY is unsupported.

Commits for this checkpoint are:

```text
dffa008 M75d1: add phase handoff regression checks
1cd3edb M75d1: gate target phase requests behind PIO readiness
e56a16e M75d1: implement request sense data phase
efb19cb M75d1: use controller phase processing quantum
07c1074 M75d1: record PIO data bytes in SCSI trace
0527db6 M75d1: validate PIO byte evidence
9e6919f M75d1: keep same-phase PIO requests ready
```

Validation after the correction:

```text
python3 tools/qa/m75_scsi_controller.py --root .       PASS
cmake --build build/linux-ci-clang --target vaeg_sdl2 -j2  PASS
cmake -S . -B build/m75-tests -DCMAKE_BUILD_TYPE=Debug \
  -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_INTEGRATION_TRACE=ON  PASS
cmake --build build/m75-tests --target vaeg_sdl2 -j2      PASS
ctest --test-dir build/m75-tests -R vaeg_m75_scsi_controller \
  --output-on-failure                                      PASS
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  build/linux-ci-clang/sdl2/vaeg --selftest                PASS
```

The current Linux worker digest after the trace-byte rebuild is
`ae3930bd2738b7685621b5722a5e6699990fa75abaf1881cef82267b693f7f5e`.
The real-ROM bounded run with `--scsitrace-limit 6` exited 0 at the diagnostic
completion limit and observed TUR STATUS/MESSAGE plus REQUEST SENSE DATA IN;
the longer `--scsitrace-limit 20` run was externally bounded with exit 124
before INQUIRY. Neither result is a G75 approval. Manual SCFORM, SCHD
registration, file operations, SASI, HOSTFAT, and non-SCSI regression gates
remain outstanding.


## M75d1 post-cursor evidence (current implementation)

The first corrected real-ROM run exposed a second general PIO state defect.  The
PCPLUS path programs `TC=1` for each byte of a DATA IN response.  Resetting
`rddatpos` at every `TRANSFER INFO` request therefore returned the first byte
(`70h`) repeatedly for REQUEST SENSE.  Commit `4ab457b` preserves the target
DATA cursor across repeated requests and resets it only when a new command or a
new phase starts.  The subsequent phase-boundary reset is committed as
`dafeae0`.

The low-overhead diagnostic options are now explicit and disabled by default:
`--scsitrace-no-guest` keeps SCSI register/transfer evidence without the
UPD9002 guest observation seam, and `--scsitrace-compact` keeps only transfer,
data-byte, phase-wait, and warning records.  These options do not change SCSI
state or guest timing; they only reduce diagnostic output overhead.

With the current worker (`ae3930bd2738b7685621b5722a5e6699990fa75abaf1881cef82267b693f7f5e`),
normal-speed compact tracing records the corrected cursor sequence:

```text
REQUEST SENSE DATA IN: data=70 index=0, CSR=19h
REQUEST SENSE DATA IN: data=00 index=1, CSR=19h
```

The 30-second bounded run still ended with the external safety status 124
before the full REQUEST SENSE allocation and later INQUIRY command.  A longer
normal-speed compact run produced no additional DATA bytes before it was
stopped; it likewise did not reach INQUIRY.  Diagnostic `--cpumult 8/32` runs
reach INQUIRY and later CDBs, but change emulated timing and produce incomplete
DATA transfers; they are not counted as M75 acceptance evidence.

Current local regression checks:

```text
python3 tools/qa/m75_scsi_controller.py --root .                       PASS
cmake --build build/linux-ci-clang --target vaeg_sdl2 -j2             PASS
cmake --build build/m75-tests --target vaeg_sdl2 -j2                  PASS
ctest --test-dir build/m75-tests -R vaeg_m75_scsi_controller            PASS
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  build/linux-ci-clang/sdl2/vaeg --selftest                            PASS
```

Session-only smoke checks using the maintained ROM directory passed for the
non-SCSI path, a SCSI image, and an empty HOSTFAT snapshot.  SASI could not be
run because every available temporary HDD image was rejected by the existing
validator with `unsupported format or geometry`; no SASI result is claimed.
SCFORM, SCHD registration, reboot, and file create/read/delete remain manual
PC-Engine guest checks and were not run by this headless session.

The normal-speed INQUIRY allocation/response contract has not yet been
observed.  M75d1 and G75 therefore remain open; no G75 approval is claimed.


## M75d1 bus-free completion and CPU-multiplier evidence (2026-08-02)

The full normal-speed trace now includes the terminal bus-free event.  After
MESSAGE IN DATA `00h` and completion `CSR=1Fh`, the controller raises
`CSR=85h` with `phase=00h`; PCPLUS reads `AR=17h -> 85h` and `AR=16h -> 00h`.
This was corrected by [bc29d9e](https://github.com/nakatamaho/vaeg/commit/bc29d9e7cecc426c4da22cbc628ab95f8c7efe8f).
The correction is general: bus-free has no following `TRANSFER INFO`, so the
pending ending-disconnect status is marked target-ready when MESSAGE IN
completes.  Data-bearing phase changes retain the controller processing
quantum and DBR gate.

The resulting TUR sequence is now:

```text
COMMAND request  8Ah; AR19 write x6; completion 1Ah
STATUS request   8Bh; AR19 read x1 = 00h; completion 1Bh
MESSAGE request  8Fh; AR19 read x1 = 00h; completion 1Fh
BUS FREE         85h; AR17 read = 85h; AR16 read = 00h
```

The same trace proceeds to a later SELECT/COMMAND request.  It does not yet
provide a normal-speed INQUIRY DATA IN record before the bounded run ends,
so G75 remains open.  No `CSR=19h` or short-transfer `CSR=4Bh` INQUIRY record
is claimed here.

### CPU multiplier diagnostic

The `--cpumult 8` and `--cpumult 32` runs reach later CDBs, including the
INQUIRY CDB `12 00 00 00 24 00`, but the guest requests only partial DATA IN
transfers before issuing later commands.  These accelerated traces are not
acceptance evidence.  Source inspection confirms that the PIO byte pump is
synchronous: `scsiio_data_read()` and `scsiio_data_write()` contain no event
scheduling or direct `CPU_REMCLOCK` manipulation.  Only phase readiness and
interrupt delivery use `nevent_set()`, whose absolute clock is expressed in
emulated CPU-clock units.  The focused validator now rejects any future
asynchronous byte-pump regression in [0c80c44](https://github.com/nakatamaho/vaeg/commit/0c80c447b6b655b81b3d08e5b67c8a1457d5be91).

The accelerated guest-level ordering mismatch is classified as an expected
CPU/device timing-ratio observation; no unsupported production timing
workaround was added.  Normal-speed evidence is the only evidence eligible
for the INQUIRY golden.

### CPU multiplier timing-unit assessment (superseded, 2026-08-02)

The earlier assessment closed the accelerated `--cpumult 8` and `--cpumult 32`
runs as timing-mode observations.  That closure is superseded by the later
normal-speed comparison below; the unit audit alone did not establish that the
chosen device-processing quantum is correct.  The unit audit
covered all three relevant boundaries:

- `scsiio_schedule_transfer_phase()` and the AR18=`20h` TRANSFER INFO path
  schedule `SCSI_TARGET_PROCESSING_CLOCKS` with `nevent_set(...,
  NEVENT_ABSOLUTE)`.
- `scsiintr_immediate()` and `scsiintr()` also schedule their delivery in
  the same emulated CPU-clock domain.
- `nevent_set()` converts absolute event clocks against `CPU_REMCLOCK`; it
  does not use wall-clock seconds or a guest-instruction counter.
- `scsiio_data_read()` and `scsiio_data_write()` perform one synchronous
  byte operation per AR19 access and neither schedule an event nor modify
  `CPU_REMCLOCK`.
- `UPD9002_WORKCLOCK()` applies the requested CPU multiplier to guest
  instruction consumption, while `iocoreva` retains the standard bus access
  clock.  Therefore `--cpumult 8/32` intentionally changes the guest
  CPU/device time ratio.

A finite guest polling budget can consequently expire before a device event
that is still pending in emulated time.  The partial accelerated DATA IN
traces are therefore not acceptance evidence and do not authorize a
CPU-multiplier-specific delay or guest shortcut.  The synchronous-PIO
validator was extended in [df2981e](https://github.com/nakatamaho/vaeg/commit/df2981e)
to require both the AR18-to-first-DBR target-processing event and the DBR-low
interval in the TRANSFER INFO path.

At the time of this checkpoint the cpumult item was recorded as `guest timing
assumption violation, not a production defect`; that classification is now
superseded by the normal-speed comparison in the later correction section.
Normal-speed execution remains the only valid source for the 36-byte INQUIRY
golden.  The current release worker still has not produced a normal-speed
`CSR=19h` / AR19-read-x36 record within the available bounded runs, so that
evidence and all manual SCFORM/SCHD/file-operation checks remain open.

The source pre-check also records that the current `hdd_inquiry` table is a
32-byte response while the observed CDB requests allocation length `24h`
(36 bytes).  No ANSI level, vendor string, or padding change is made from
this observation alone; the payload contract must be resolved and tested when
the normal-speed INQUIRY path is reached.

### M75d1 short-transfer contract (2026-08-02)

The current INQUIRY response table contains 32 bytes, while the observed CDB
requests allocation length `24h` (36 bytes).  This is not itself a protocol
violation: SCSI transfers `min(allocation length, response length)` and changes
phase when the response is exhausted.  The missing controller behavior was the
short-transfer completion contract.

Commit [ca29efb](https://github.com/nakatamaho/vaeg/commit/ca29efb) now handles
the two allocation cases generally:

- response shorter than the host allocation: after the last real AR19 DATA IN
  byte, residual TC is preserved, STATUS becomes the target phase, and the
  controller emits `0x48 | (STATUS & 7)` = `4Bh`;
- host allocation shorter than the response: TC reaches zero, the unrequested
  response suffix is discarded, and normal DATA IN completion `19h` advances
  to STATUS.

The residual count is not fabricated and no INQUIRY-specific branch is used.
The response metadata invariant was corrected and guarded in
[4eeacda](https://github.com/nakatamaho/vaeg/commit/4eeacda): byte4 of the
32-byte table is `1Bh`, equal to table length minus five.  The QA validator
parses the table and rejects any future length/additional-length mismatch.

A normal-speed guest trace proving either the short `4Bh` path or a full
`19h` path remains outstanding; this implementation and its source checks do
not constitute G75 approval.

### SASI predecessor comparison

The same temporary SASI image that is rejected by the current branch was tested
with a clean binary built at the synchronized M75 predecessor
`11cb0026ad646cff16237adef95e324fcedd40d9`.  Both runs return exit status 1
with `unsupported format or geometry`.  This rejection predates the M75d1
controller changes and is not counted as an M75 regression.  No SASI pass is
claimed.

### Current evidence digest

The evaluated Linux SDL2 worker after the bus-free correction is:

```text
build/linux-ci-clang/sdl2/vaeg
SHA-256: 706b1d6a7bebc2f7a2270e49e0e55c4d79e458515d01fc4c7bc183b9af534d8a
```

The focused checks are:

```text
python3 tools/qa/m75_scsi_controller.py --root .             PASS
cmake --build build/linux-ci-clang --target vaeg_sdl2 -j2    PASS
```

The existing debug CTest, SDL selftest, repository invariant checks, SCSI
smoke, and HOSTFAT smoke remain green as recorded above.  The predecessor SASI
comparison is recorded as `exit=1` with the same pre-existing geometry error.
SCFORM, SCHD registration/reboot, create/read/delete file operations, and the
36-byte normal-speed INQUIRY golden remain manual or unobserved.  G75 is not
self-approved.


### M75d1 post-short-transfer integration evidence (2026-08-02)

After the shortened DATA IN correction, a normal-speed full `scsitrace` run
was externally bounded at 45 seconds.  It proved the complete TUR controller
sequence, including `CSR=85h`, and then reached a second SELECT/COMMAND
request (`CSR=11h` followed by `CSR=8Ah`).  The bounded run ended before the
second CDB's AR19 transfer; this is a progress boundary, not a registration
result.

A longer normal-speed compact run remained in the TUR sequence before its
external safety bound.  The compact filter omits the bus-free and indirect
register records, so it is not used to claim that `85h` was absent.

A diagnostic-only `--cpumult 8` run reached these later CDBs:

```text
INQUIRY      12 00 00 00 24 00
READ CAPACITY 25 00 00 00 00 00 00 00 00 00
MODE SENSE   1A 00 04 00 24 00
```

Those accelerated records show the guest progressing but request partial DATA
IN transfers under a deliberately changed CPU/device timing ratio.  They do
not establish the normal-speed short `4Bh` path, SCHD registration, SCFORM, or
file operations.

The evaluated worker after the short-transfer and metadata corrections is:

```text
build/linux-ci-clang/sdl2/vaeg
SHA-256: 6dd5469a751dd0ba15d86fd1dc2b3d42ab0c5241c2efa98b52cfee28a1ef2df9
```

The SCSI QA, Linux SDL2 build, focused CTest, and existing selftest remain
passing.  G75 remains open.


### M75d1 MODE SENSE page-04 geometry correction (2026-08-02)

The observed SCHD CDB `1A 00 04 00 24 00` requests MODE SENSE(6), page
`04h` (Rigid Disk Drive Geometry), with a 36-byte allocation.  The previous
implementation ignored the page code and returned only a 12-byte header and
block descriptor.  That could not provide the cylinder/head geometry needed by
SCHD registration.

Commit [03d4cd7](https://github.com/nakatamaho/vaeg/commit/03d4cd76541a3058cf32b0c239b499e0c0431627)
now derives the response from the mounted `SXSIDEV` and supports empty page `00h`,
rigid-disk page `04h`, and the all-pages request `3Fh` (page 00h followed by
page 04h).  With DBD clear, the page-04 response is 36 bytes: a four-byte
mode header, an eight-byte big-endian block descriptor, and a 24-byte page-04
payload.  With DBD set, the descriptor is omitted and the
response is 28 bytes.  The mode data length reports the available response
length minus one, while the transfer length remains bounded by the host
allocation count.  Cylinder count, head count, block count, and block length
are encoded from the mounted image; the geometry invariant is checked as
`totals == cylinders * surfaces * sectors`.

Unsupported page codes and contradictory image geometry now return CHECK
CONDITION with ILLEGAL REQUEST (`sense key=05h`, `ASC=24h`).  The existing
REQUEST SENSE path returns that sense data and clears it after delivery.  No
fixed 40 MB geometry or PCPLUS-specific branch was introduced.

The accelerated diagnostic trace reaches the later CDB sequence and confirms
that MODE SENSE is now classified as DATA IN:

```text
INQUIRY    12 00 00 00 24 00
READ CAPACITY 25 00 00 00 00 00 00 00 00 00 00
MODE SENSE 1A 00 04 00 24 00
MODE DATA IN request: phase=19h, allocation TC=24h
```

The `--cpumult 8` run remains diagnostic-only and shows partial transfers due
to the intentionally changed CPU/device timing ratio.  It is not used as
acceptance evidence.  A normal-speed 180-second compact run still completed
only the TUR sequence before its external safety bound (`exit=124`); it did
not reach the later MODE SENSE transfer.  Therefore SCHD registration has not
been demonstrated.

Validation after this commit:

```text
python3 tools/qa/m75_scsi_controller.py --root .                 PASS
cmake --build build/linux-ci-clang --target vaeg_sdl2 -j2        PASS
cmake --build build/m75-tests --target vaeg_sdl2 -j2             PASS
ctest --test-dir build/m75-tests -R vaeg_m75_scsi_controller    PASS
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy vaeg --selftest       PASS
worker SHA-256: 88454f0e1176c3a2f1b82573e9ca7170373cd17f21e4b6253855e879e1344b4d
```

G75 remains open.  Normal-speed INQUIRY/MODE SENSE DATA IN accounting,
PCPLUS/SCHD registration, SCFORM initialization, reboot, and SCSI file
create/read/delete operations still require evidence.  No G75 approval is
claimed.


### M75d1 page-00/all-pages follow-up (2026-08-02)

The task contract also requires MODE SENSE page `00h` and a real all-pages
response for page `3Fh`.  Follow-up commit
[56848c0](https://github.com/nakatamaho/vaeg/commit/56848c0f68cbe2b4381003343bf753e1c61d930b)
adds the empty page-00 response and composes page 00h followed by page 04h for
3Fh.  The response remains allocation-bounded and uses the same DBD-dependent
header and descriptor layout.  Unsupported pages still return CHECK CONDITION
with ILLEGAL REQUEST.  The rebuilt worker after this follow-up has SHA-256
`51aae76205dcb71f5bc447cbdbf7ac8f33d220bad6852cd594a11d61b40ec3df`.


### M75d1 timing classification correction (2026-08-02)

The CPU-multiplier classification is reopened.  The normal-speed run uses the
intended CPU/device ratio but, in the 180-second bounded run, produced only the
TUR transfer results and no later MODE SENSE DATA IN.  The diagnostic
`--cpumult 8` run reached the later CDBs, including:

```text
INQUIRY      12 00 00 00 24 00
READ CAPACITY 25 00 00 00 00 00 00 00 00 00
MODE SENSE   1A 00 04 00 24 00
DATA IN      phase=19h, allocation TC=24h
```

The accelerated run is not acceptance evidence because its DATA IN transfers
are partial, but progress under an artificial ratio means the previous
`f406b86` conclusion (`guest timing assumption violation, not a production
defect`) is no longer valid.  The current evidence supports an open timing
mismatch: the normal-speed device event may arrive before the guest has
returned to the intended `1CCDh` wait consumer, while the accelerated ratio
happens to provide more processing margin.  This mechanism is not yet proven
by a paired consumer trace.

No delay constant has been changed in response.  The next diagnostic must
compare the second SELECT/COMMAND event at normal speed and `--cpumult 8`:
`1CCDh` is the expected wait consumer, whereas `1742h`/`1747h` identify the
main event-pump path.  Only after that comparison may a general device-time
correction be considered; PCPLUS addresses, CDB ordering, and multiplier-
specific workarounds remain prohibited.

This correction changes the ledger classification but does not approve G75.

### M75d1 timing follow-up: normal versus accelerated DATA IN (2026-08-02)

A paired bounded run was repeated after restoring the production constant
`SCSI_TARGET_PROCESSING_CLOCKS=100`; the temporary 4000-clock experiment was
not retained.  The worker used for this check is the rebuilt source at
`02d5ed802e086860a8907e76e5fa4a9da315f384` with SHA-256
`51aae76205dcb71f5bc447cbdbf7ac8f33d220bad6852cd594a11d61b40ec3df`.

At normal speed, a 120-second compact guest-trace run produced the complete
TUR sequence and no `cdb0=12`, `cdb0=1a`, or MODE SENSE DATA IN transfer.  A
normal-speed compact run without guest tracing reached the second SELECT
(`CSR=11h`) and COMMAND request (`CSR=8Ah`) after the TUR bus-free event, but
no subsequent CDB transfer was observed before the safety bound.  The external
`exit=124` is only the safety timeout and is not a semantic completion result.

With `--cpumult 8`, a 60-second no-guest compact run reached the MODE SENSE
CDB `1A 00 04 00 24 00` and issued DATA IN requests with `phase=19h` and
`TC=24h`.  The first 24-byte request was abandoned with zero AR19 accesses;
subsequent one-byte `phase=19h` requests completed.  Thus the accelerated run
proves later command reachability, but not a complete MODE SENSE payload
transfer or SCHD registration.  It remains diagnostic-only.

A temporary, uncommitted change from 100 to 4000 target-processing clocks was
also tested at normal speed for 60 seconds.  It produced only the TUR results
and no later CDB, so it is not an evidence-backed correction.  The source was
restored to 100 and rebuilt; no production timing change was committed.

The consumer-path comparison remains incomplete: the normal-speed second
`8Ah` was observed only in the low-overhead controller trace, while the
full guest trace did not reach that point within its tracing bound.  Therefore
it is not yet proven whether `1CCDh` or `1742h/1747h` consumes that event at
normal speed.  G75 remains open and no delay tuning is authorized.

### INQUIRY revision identification (2026-08-02)

The INQUIRY response revision field (bytes 24-27) now returns the fixed
ASCII revision `1.00`; the remaining four bytes of the eight-byte revision
area remain space padded.  The response remains a 32-byte table with byte4
`1Bh`; this does not change the allocation-length or short-transfer contract.
The M75 controller QA validator now rejects a response table whose revision is
not `1.00`.  This is an identification change only and does not constitute
normal-speed INQUIRY DATA IN acceptance.


### M75d1 Phase A command-request window trace (2026-08-02)

The next diagnostic is trace-only and does not change SCSI production
behavior.  The option `--scsitrace-cmdreq-windows` numbers windows from VAEG's
raw `CSR=8Ah` presentation, not from a guest-side `CS:[047Eh]=BAh` write.
Therefore a window with no `BAh` write is still represented and is evidence of
non-presentation or an incomplete handoff.

Each window records the preceding and following raw CSR presentation events,
including `11h` SELECT completion and `85h` bus-free, the guest instruction
counter, and the same emulated clock used by the SCSI event queue.  Within each
`8Ah` window the disabled-by-default seam records the `1D67h` `047Eh` write,
`1CBDh` wait-point entry, `1CCDh` wait-loop consumption,
`1742h/1747h` main-pump consumption, `1791h` exit, the `1972h` phase path,
`19A7h/19BBh` comparisons, and `1C32h` Transfer Info reachability.  Summary
records preserve the presentation, `BAh` write, and consumer timestamps plus
their instruction/clock deltas.

The seam is wired through a no-op trace API from `scsiioint`; it does not alter
CSR state, IRQ state, guest memory, registers, FLAGS, timing, or the PIO pump.
The option is OFF by default.  Existing `--scsitrace` uses the original guest
trace path when the option is absent.  A paired normal-speed run is required:
`1CCDh` for the first and `1747h` for the second indicates early phase
presentation; two `1CCDh` consumers rejects that hypothesis; and an absent
second raw `8Ah` identifies non-presentation instead.  No production delay or
phase change is authorized until this evidence exists.

The Phase A code and validator checks build successfully and the focused M75
CTest/selftest pass.  The current macOS Cocoa SDL environment aborts during
window initialization before guest execution, so no real-ROM window result is
claimed from this host.  G75 remains open.


Phase A trace-only commit: `3667ed08701ba3e1863d659dfd47ddc954e25183`.
Validation at that commit completed with exit status 0 for:

```text
cmake --build build/linux-ci-clang --target vaeg_sdl2 -j2
cmake --build build/m75-tests --target vaeg_sdl2 -j2
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy build/m75-tests/sdl2/vaeg --selftest
python3 tools/qa/m75_scsi_controller.py --root .
ctest --test-dir build/m75-tests -R vaeg_m75_scsi_controller --output-on-failure
```

The normal-speed real-ROM command was attempted with
`--scsitrace-cmdreq-windows`, `--scsitrace-compact`, and
`--scsitrace-limit 7`; it exited 134 before guest execution because this
macOS host's SDL Cocoa backend raised `NSInternalInconsistencyException`
while initializing `SystemAppearance`.  This is an environment failure, not a
SCSI result.  No first/second raw-`8Ah` classification is claimed yet.

### M75d1 transfer-count byte-order experiment (superseded)

An initial interpretation attributed the SCFORM/SENSE stop to a transfer-count
byte-order defect and tested a low/middle/high decode.  That experiment is
retained only as history.  Its WSLg trace later showed `TC=060000` for a
six-byte CDB and `TC=010000` for a one-byte transfer, so it multiplied the
expected counts by 65536.  No acceptance result is based on that experiment.

The original high/middle/low implementation is restored by
[c959453](https://github.com/nakatamaho/vaeg/commit/c959453a0a482994ac25ab6db0b33e425306a0e9).
The separate MODE SENSE block-length correction remains under evaluation.
The local Linux/MinGW builds, M75 QA, focused CTest, and SDL selftest pass;
manual SCHD/SCFORM acceptance remains open.

### M75d1 MODE SENSE block-descriptor correction (2026-08-02)

The corrected run now passes the SENSE transfer and reaches SCHD's INQUIRY
summary: device code 0, response-data format 0, and fixed-media mode.  These
messages are informational and show that INQUIRY completed; the subsequent
halt is in the MODE SENSE/geometry step.

Static comparison with SCHD's documented request (`1A 00 04 00 24 00`) found a
second controller-contract defect.  In a MODE SENSE(6) block descriptor, the
block length occupies response bytes 9--11 (header bytes 0--3, descriptor
bytes 4--11).  VAEG wrote the three-byte block length at byte 8, overwriting
the reserved byte and leaving a shifted value (for example, 256 appeared as
`01 00 00` when SCHD reads bytes 9--11).  The correction writes at byte 9;
mounted `SXSIDEV` geometry remains the sole source of the value.

The static M75 QA validator now protects this offset.  Linux build, focused
CTest, M75 QA, and SDL selftest pass after the correction.  A corrected MinGW
artifact must still be run through SCHD registration and SCFORM to confirm the
manual symptom is cleared; G75 remains open.

### M75d1 transfer-count order correction from WSLg trace (2026-08-02)

The WSLg MinGW trace disproved the earlier low/middle/high interpretation.  In
one run the controller reported `TC=060000` and attempted 393216 CDB bytes for
`CDB=00`; the following one-byte STATUS request reported `TC=010000`.  The same
trace showed READ CAPACITY `TC=0a0000` with `CDB=25` and DATA IN `TC=080000`.
These values are exactly the expected 6/1/10/8 counts multiplied by 65536.
The trace therefore proves that PCPLUS writes AR12/AR13/AR14 as high/middle/low.

The low/middle/high experiment from [9f11430](https://github.com/nakatamaho/vaeg/commit/9f11430a52e7d660c18cb0cfad3bef448f6c157c)
is superseded without rewriting history.  Commit
[c959453](https://github.com/nakatamaho/vaeg/commit/c959453a0a482994ac25ab6db0b33e425306a0e9)
restores the original high/middle/low decode and borrow order.  The MODE SENSE
block-length-at-byte-9 correction remains in effect.  The corrected MinGW
artifact was rebuilt; its SHA-256 is
`d17a62569568d51ddfda1a8739824e52922772f9be870dfa98780d3abf4eac25`.

### M75d1 CSR provenance trace (2026-08-02)

The synchronized starting SHA for this diagnostic checkpoint was
`afd3dabe19dcca670847f7e397ead67c7cf38e33` on
`topic/m75-scsi-support`, matching the remote branch before the change.
Trace-only commit
[ad9c99b](https://github.com/nakatamaho/vaeg/commit/ad9c99b2d36a810793a57b1162fc195229b009a3)
adds provenance to the existing CSR latch/pending path.  It is not a
production correction.

The new records are: `csr-request` (monotonic sequence, raw CSR, origin),
`csr-latch`, `csr-hostread`, `csr-promote`, `csr-overrun`, and `csr-drop`.
Each includes the request origin and sequence plus event-active, latched, and
pending status/sequence/origin, phase, selected AR, auxiliary status,
transfer/command pending state, target readiness, and `CS:IP`.  The trace is
opt-in through `--scsitrace`; compact mode retains these records.  No guest
state, CSR behavior, event clocks, IRQ behavior, or PIO behavior changes when
tracing is disabled.

The normal-speed VA1 run used the standard support-disk/SCSI-image command
with `--scsitrace --scsitrace-no-guest --scsitrace-compact
--scsitrace-limit 7`.  It timed out at the external 30-second safety bound
(exit 124), after the complete TUR sequence and the second
SELECT/COMMAND request.  The trace contained 11 CSR requests, 11 latches,
and 10 host reads at the cutoff; every request that became visible followed
request -> latch -> hostread.  Counts for `csr-promote`, `csr-overrun`, and
`csr-drop` were all zero.  A separate semantic-limit run exited 0 and showed
the same ordered prefix.  Thus this run does not reproduce the stale `11h`
followed by late `1Ah` sequence reported from the older WSLg binary.  The
trace is evidence that the current run did not overflow the pending slot, not
proof that the older report was impossible.

Focused validation and build results:

```text
python3 tools/qa/m75_scsi_controller.py --root .                         PASS
cmake --build build/linux-ci-clang --target vaeg_sdl2 -j2                  PASS
cmake --build build/m75-tests --target vaeg_sdl2 -j2                      PASS
ctest --test-dir build/m75-tests -R vaeg_m75_scsi_controller --output-on-failure  PASS
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy build/m75-tests/sdl2/vaeg --selftest  PASS
```

The evaluated Linux SDL2 executable SHA-256 is
`5e55a19ecc2eeca505fa0bde0923cfdb9ce7781b65343ddadedc39035efec76d`.
No production-fix SHA is claimed by this checkpoint.  G75 remains pending;
normal-speed INQUIRY DATA IN accounting, SCHD/SCFORM/reboot/file-operation
acceptance, and SASI/HOSTFAT/non-SCSI regression evidence are still open.


### M75d1 CSR admission correction: pull-model target gating (2026-08-02)

The provenance detector was committed first in
[23b2752](https://github.com/nakatamaho/vaeg/commit/23b2752f36bc0571a705fecc3f25c964caf1d410).
The maintainer-provided pre-correction WSLg trace contains the decisive
collision: `seq=13`, `CSR=1Ah`, was requested while `seq=12`, `CSR=11h`, was
still the active scheduled event; the trace then records `pending=1` and later
`csr-overrun` records.  This is the detector evidence for the old two-stage
CSR path.  A separate replay of the pre-correction source did not reach that
collision within its shorter safety bound, so the replay is not claimed as a
second reproduction.

The production correction is
[ccb0666](https://github.com/nakatamaho/vaeg/commit/ccb066695907456314783cc3bb9a28dfad279c55).
The CSR pending slot and its promotion path were removed.  Target-origin
selection, command-request, phase-ready, and bus-free events now remain as
persistent target state and are pulled only after AR17 consumes the visible
CSR.  A target-processing event is then scheduled; host-synchronous transfer
completion (`1Ah`, `1Bh`, `1Fh`, and short-transfer `48h`--`4Fh`) remains
serialized by the host I/O access and is not put through the target queue.
The one-device CSR latch remains independent of 8259 EOI.  A second CSR is
never silently queued or overwrites the visible latch; the trace records an
`invariant ...-overlap`, `csr-overrun`, and `csr-drop` if a producer violates
that admission boundary.

A deterministic watchdog event (`NEVENT_SCSIWATCHDOG`) reports an unread CSR,
a target phase-delay that does not complete, or an internal DATA IN decision
for which no `CSR=89h` request appears within the watchdog interval.  The
latter includes a monotonic missing-request count and the guest `CS:IP` at the
observation.  These diagnostics are opt-in with `--scsitrace` and do not alter
guest state or the controller contract.

The target processing quantum remains the emulated-clock constant
`SCSI_TARGET_PROCESSING_CLOCKS=100`; it was not tuned to the guest.  A
trace-only seeded jitter facility is available with
`--scsitrace-jitter-seed N --scsitrace-jitter-span N`; it varies only target
processing event clocks, records the seed/span/effective samples, and is
reproducible.  Five seeds (1 through 5, span 200) produced zero
`csr-overrun`, `csr-drop`, and `invariant` records.  Seed 2 additionally
reported the watchdog's unconsumed `CSR=8Ah` at the bounded-run cutoff; this
is retained as an open guest-progress observation, not hidden as a pass.

Fixed-clock stress runs at 4000 and 40000 target-processing clocks likewise
produced zero overrun/drop/invariant records.  The 40000 run reached the
second TUR, INQUIRY, READ CAPACITY, and MODE SENSE CDB boundaries; the 4000
run exposed a different incomplete guest transfer pattern but no CSR
admission violation.  These are structural stress results, not G75
acceptance evidence.

The final normal-speed bounded run (`exit=124` safety bound) completed the
TUR STATUS and MESSAGE IN phases and reached the second SELECT/COMMAND
request, then the watchdog reported an unread `CSR=8Ah` before the run ended.
It did not produce the normal-speed INQUIRY DATA IN golden sequence.  Thus the
pull-model correction removes the old CSR queue collision, but it does not yet
prove that the guest consumes the later command request or that SCHD registers
the device.

Focused validation for [ccb0666](https://github.com/nakatamaho/vaeg/commit/ccb066695907456314783cc3bb9a28dfad279c55):

```text
cmake --build build/m75-tests --target vaeg_sdl2 -j2                         PASS
python3 tools/qa/m75_scsi_controller.py --root .                           PASS
ctest --test-dir build/m75-tests -R vaeg_m75_scsi_controller --output-on-failure PASS
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy build/m75-tests/sdl2/vaeg --selftest PASS
```

The evaluated executable was `build/m75-tests/sdl2/vaeg`, SHA-256
`5c417772db1385e65c8aaea65d03ce43c4d84574fb0cff59a990151b3565532b`.
The current M75 gate remains open: normal-speed INQUIRY DATA IN, SCHD
registration, SCFORM, reboot, file operations, SASI, HOSTFAT, and non-SCSI
regressions still require evidence.  G75 is not approved.

### M75d1 DATA IN repeated-TC correction (2026-08-02)

The WSLg SCFORM/SCHD run reached ID0 and completed TEST UNIT READY through
STATUS, MESSAGE IN, and bus free.  ID0 INQUIRY also reached the DATA IN request,
but the trace showed the PCPLUS access pattern explicitly: it first programmed
`TC=0024h`, then, after consuming `CSR=89h`, reprogrammed `TC=0001h` and issued
TRANSFER INFO for each byte.  The old DATA IN completion branch treated the
first `TC=1` completion as allocation exhaustion, changed the target phase to
STATUS, and emitted `CSR=19h` after one byte.  SCHD therefore saw only one
INQUIRY byte, rejected ID0, and continued with `42h` select timeouts for the
other IDs.  The run did not hang, but the device was not registered.

This is a general WD33C93 PIO contract defect, not a PCPLUS-address or disk
image workaround.  Commit
[84bc2ef](https://github.com/nakatamaho/vaeg/commit/84bc2efe1de9e5661fd28d31ba087a304f1a82ac)
keeps `SCSIPH_DATAIN` active when the host-programmed count reaches zero while
`rddatpos < cmdpos`; the next `TRANSFER INFO` then pumps the next byte.  The
phase changes to STATUS only after the target response cursor is exhausted.
The static QA validator now requires this repeated-short-transfer invariant.
The preceding trace-only cursor-state instrumentation is in
[d2da983](https://github.com/nakatamaho/vaeg/commit/d2da9835149d5d8dd4fb560c20bfd407db2719cc).

Validation for the correction:

```text
python3 tools/qa/m75_scsi_controller.py --root .                         PASS
ctest --test-dir build/m75-tests -R vaeg_m75_scsi_controller --output-on-failure PASS
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy build/m75-tests/sdl2/vaeg --selftest PASS
CCACHE_DISABLE=1 cmake --build build/mingw-cross --target vaeg_sdl2 -j2 PASS
```

The diagnostic MinGW executable copied to `/tmp/vaeg-m75-datain-repeat.exe`
has SHA-256
`1327093c303f08a7fda7f55499ef4d85ced878e932b578db81e5f26dee066dc2`.
The supplied pre-correction WSLg run is not acceptance evidence for the fixed
binary; a new run must show repeated DATA IN reads, then the complete INQUIRY
STATUS/MESSAGE sequence, before SCHD registration is reconsidered.  G75
remains open and no M76 work is authorized.

The post-correction focused test build was rebuilt from `d345d96` and the
selftest passed.  Its SHA-256 is
`f6c2758a7fe5576fdeadca9c7d5876a557174105bebaaa174cc3b3d82c2e3bf5`.
This is machine validation only; the corrected MinGW binary still requires
the manual WSLg SCFORM/SCHD run.

### M75d1 explicit WD33C93A Transfer Info lifecycle (2026-08-03)

The bounded trace was taken before the production change.  It classified the
old path as an accepted `AR18=20h` transfer that could remain in a legacy
phase-wait/abandon path instead of being represented as a Level-II command.
The correction is in [f0b14d7](https://github.com/nakatamaho/vaeg/commit/f0b14d71a2015b9469c92ea51abe2b9ebf964b43)
and the follow-up already-asserted-REQ fix is in
[9827d09](https://github.com/nakatamaho/vaeg/commit/9827d09756779943d46b0973436f26f32142dced).
The trace-only provenance checkpoint is
[23b2752](https://github.com/nakatamaho/vaeg/commit/23b2752f36bc0571a705fecc3f25c964caf1d410).
The synchronized branch start was `6442f06a98b51f67068bc56d5f61621df4e43d2c`;
[790b737](https://github.com/nakatamaho/vaeg/commit/790b737) records the
fast-forward synchronization in this worktree.

The Transfer Info lifecycle is now explicit:

```text
idle
  -> wait_for_req                 AR18=20h accepted, REQ absent
  -> transfer_byte_pending        REQ asserted (or already asserted)
  -> transfer_byte_pending        one completed REQ/ACK byte, TC decremented
  -> wait_for_post_count_req      TC becomes zero
  -> completed_or_terminated      distinct post-count REQ or 4MCI phase change
```

`INT=1` rejects the command, sets LCI, and preserves the active command, TC,
and CSR latch.  `89h` is rejected while a Level-II command is active.  DATA
register reads/writes require a pending REQ and DBR; TC is decremented only
after the byte handshake.  A phase change before TC zero produces the
phase-derived `4MCI` status.  TC zero waits for a separate post-count REQ
before producing `19h`, `1Bh`, or `1Fh`.  CSR remains depth one and is not
overwritten while INT is pending.

The decisive after-change trace is:

```text
command-write-pre command=20 int=0 ... tc=000006 state=idle
command-accepted command=20 tc=000006
transfer-start phase=1a direction=host-to-spc tc=000006
post-count-wait completion=1a next=8b state=wait_for_post_count_req tc=000000
req-assert seq=7 kind=post-count status=8b
csr-request/latch/hostread seq=4 status=1a
command-write-pre command=20 int=0 ... tc=010000 state=idle
command-accepted command=20 tc=010000
transfer-start phase=1b direction=spc-to-host tc=010000
target-phase-wait phase=1b tc=010000 state=wait_for_req
req-assert seq=8 kind=active status=8b
```

The earlier pre-correction trace showed `transfer-abandoned` and the
single-byte recovery path.  The new trace instead keeps the command active
while REQ is absent and does not synthesize `89h` from that active state.  A
Transfer Info issued while REQ is already asserted now starts the byte state
immediately; this is covered by the follow-up test.

Validation at the evaluated Linux SDL2 build:

```text
cmake --build build/linux-ci-clang --target vaeg_sdl2 -j4              PASS
python3 tools/qa/m75_transfer_info.py --selftest                       PASS (10 tests)
python3 tools/qa/m75_scsi_controller.py --root .                     PASS
ctest --test-dir build/linux-ci-clang -R 'vaeg_m75_(scsi_controller|transfer_info)' --output-on-failure  PASS (2/2)
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy build/linux-ci-clang/sdl2/vaeg --selftest  PASS
MinGW cross build                                                         UNAVAILABLE (build/mingw-cross is not configured)
```

The evaluated executable SHA-256 is
`24b78da6b70e28e865f54fed642c1ce5bdbbd347c66d76a81a20ba6487eb74ef`.
A real-ROM bounded run exited `124` at the external safety timeout.  It
reached TUR CDB transfer and the STATUS Transfer Info wait, but did not reach
the complete INQUIRY DATA IN golden sequence.  The trace contains repeated
phase-direction-mismatch writes from the guest while the strict active
Transfer Info state was waiting for a STATUS REQ; no PCPLUS-address special
case or payload change was added.  Consequently SCHD registration, SCFORM,
reboot, file operations, SASI, HOSTFAT, and non-SCSI manual gates remain
unverified.  G75 is not passed.


## G75 corrective implementation result (2026-08-03)

Starting/base SHA: `bfaced3fe6e3d59b067d5fc8e514ff4cc1cf4084` (local and remote before this work).
Production/test commit: [4e17c6f](https://github.com/nakatamaho/vaeg/commit/4e17c6f3fee67642ca69329147808cd18c71c9a7).

### Root causes and correction

The concrete defects were: successful Transfer Info completion used the wrong `0x10 | MCI` encoding (so next STATUS `8Bh` became invalid `1Ah`); the post-count target REQ was consumed or re-requested instead of retained; REQ and ACK were coupled; Message-In Transfer Info returned the previous-phase completion instead of `20h`; and the generic active-Level-II rejection blocked Level-I Negate ACK.  Transfer-count evidence was also ambiguous until every AR12/AR13/AR14 write was traced; the corrected trace shows normal high/middle/low programming and no unexplained `010000h`.

The fix in `cbus/scsiio.c` uses an explicit Transfer Info state machine.  `scsiio_success_status_from_service()` derives `19h/1Bh/1Fh/1Ah`; `scsiio_target_assert_req()`, `scsiio_target_negate_req()`, `scsiio_initiator_assert_ack()`, `scsiio_initiator_negate_ack()`, and `scsiio_complete_byte_handshake()` separate ownership.  A retained post-count request stores its sequence, phase, and direction.  CSR read clears INT only.  Message-In latches `20h` with ACK retained; Negate ACK only negates ACK and schedules the later target bus-free event.  Transfer-count writes are logged with before/after values and SBT.

### Compiled validation

The compiled C controller selftest is called from `sdl2/selftest.c` and registered in `CMakeLists.txt` as `vaeg_m75_transfer_info_compiled`.  All 21 named Transfer Info tests passed, including `success_status_encodes_19_1b_1f`, `post_count_req_survives_completion_interrupt`, `next_transfer_uses_same_req_id`, `message_in_transfer_returns_20`, `message_in_holds_ack_until_negate_ack`, `negate_ack_clears_ack_without_direct_interrupt`, `command_during_int_pending_is_ignored`, `ignored_command_sets_lci`, `transfer_count_register_order`, and `single_byte_transfer_command_semantics`.  The controller validator and the 10-case Python model check also passed.  Focused CTest passed 2/2 (`vaeg_m75_transfer_info_compiled`, `vaeg_romless_tests`).

### Before/after real-ROM evidence

Before the correction, TUR logged `post-count-wait completion=1a next=8b` and lost the target request.  After the correction the semantic trace contains: `post-count-retained req=7 phase=1b`, CSR `1Bh` read with the same retained request; STATUS Transfer Info reads one byte and latches `1Fh`; `post-count-retained req=8 phase=1f`; MESSAGE-IN Transfer Info reads one byte and latches `20h`; CSR `20h` read shows `req=0 retained=0 ack=1`; Negate ACK then clears ACK and a later target bus-free event latches `85h`.  The request sequence IDs are preserved across each completion CSR.  No duplicate `8Bh` is generated for the retained STATUS request.

The same run reached INQUIRY: CDB `12 00 00 00 24 00`, DATA IN TC=36, 32 AR19 reads, and phase-change completion `4Bh` with residual TC=4, followed by STATUS/MESSAGE-IN completion.  No direction mismatch, CSR overrun/drop, unexplained `010000h`, or `0CC6h` access was observed.  The existing 32-byte INQUIRY payload remains unchanged.  The bounded command used semantic `--scsitrace-limit 30` and exited 0; that exit is not a claim that the full guest run completed.

Linux executable SHA-256: `01388de8ec1fe2a0e7df87d38f8607a734b6454688f7cb82471730e7c8cfca1d`.  MinGW cross configuration and build passed with `x86_64-w64-mingw32-gcc`; the Windows executable was not run on this host.  SASI/HOSTFAT selftest coverage passed, but the manual SASI/HOSTFAT gate and non-SCSI disk-path gate remain unverified.

G75 status: **FAIL/pending**.  Remaining gates are PCPLUS loaded before SCHD, SCHD registration, SCFORM initialization, reboot, create/read/delete file test, manual SASI and HOSTFAT regression, and existing non-SCSI disk-path regression.  No G75 PASS declaration and no M76 work.


## G75 LUN enumeration and INQUIRY correction (2026-08-03)

The implementation/test commit is
[103d59e](https://github.com/nakatamaho/vaeg/commit/103d59e), based on
`1f2099eed524411113dda6db1a7bf8a77820c319`.

### Root cause and correction

The backend previously exposed the mounted target without a centralized CDB
LUN check, allowing discovery behavior to be interpreted as multiple logical
units.  The normal INQUIRY response was also a 32-byte table with additional
length `1Bh`; SCHD therefore read beyond the response when it displayed the
four-byte revision.  The correction requires both the WD Target LUN register
and CDB LUN to be zero for the mounted SXSIDEV.  Unsupported LUN INQUIRY is a
GOOD 36-byte `7Fh` response, while other unsupported-LUN commands return
CHECK CONDITION with `05/25/00`.  The normal response is exactly 36 bytes,
with `1Fh` additional length, `NEC     `, `NP2-HDD         `, and `1.00` at
the mandated fixed-width offsets.  No LUN aliases or guest-address special
cases were added.

### Enumeration evidence

The compact trace records each selection and completed CDB as:

```text
target_id target_lun cdb_lun opcode cdb selected_index inquiry0 response_length status sense asc ascq
```

The bounded after-run matrix was:

| target ID | target LUN | CDB LUN | observed CDBs | selected backend | INQUIRY byte 0 | result |
|---:|---:|---:|---|---:|---:|---|
| 0 | 0 | 0 | 00, 12, 25, 1A, 28, 03 | 2 | 00h for INQUIRY (36 bytes) | selected / GOOD for supported metadata |

No target-ID >0 or CDB LUN >0 record appeared in the bounded after-run.
The prior approximately-eight-device report predates this matrix, so it is
not sufficient to classify the aliases as LUN aliases versus target-ID
aliases.  The current trace proves only one configured target/LUN identity.
At the time of that pre-block-I/O checkpoint, the later READ(10) (`28h`) was
reported as CHECK CONDITION `05/20/00`; the separate block-I/O correction is
recorded in the later G75 section below.

### INQUIRY bytes

Before correction the table was 32 bytes:

```text
00 00 02 02 1B 00 00 18 4E 45 43 20 20 20 20 20
4E 50 32 2D 48 44 44 20 31 2E 30 30 20 20 20 20
```

After correction the exact 36-byte response is:

```text
00 00 02 02 1F 00 00 18 4E 45 43 20 20 20 20 20
4E 50 32 2D 48 44 44 20 20 20 20 20 20 20 20 20
31 2E 30 30
```

### Validation

```text
python3 tools/qa/m75_scsi_controller.py --root .                 PASS
python3 tools/qa/m75_transfer_info.py --selftest                  PASS (10)
SDL ... build/linux-ci-clang/sdl2/vaeg --selftest                  PASS
ctest ... -R 'vaeg_m75_transfer_info_compiled|vaeg_romless_tests'  PASS (2/2)
cmake --build build/linux-ci-clang --target vaeg_sdl2 -j4         PASS
cmake --build build/mingw-cross --target vaeg_sdl2 -j4            PASS
python3 tools/repo/check_case.py                                   PASS
python3 tools/repo/check_encoding.py                               PASS
python3 tools/repo/check_eol.py                                    PASS
```

The MinGW executable is `build/mingw-cross/sdl2/vaeg.exe`.  Linux SHA-256 is
`75546b9b75df995cfe93c8dbb332baa6e4360f52bc7a072ac120e0d866d0f3f8` and
MinGW SHA-256 is
`2b070c348d9bdc789842cfb937ed4c93243dffcf2e7f05853988d418d03595ca`.  SCFORM was intentionally not rerun at that pre-block-I/O checkpoint because
metadata discovery still encountered unsupported READ(10); the later block-I/O
run corrected that command path, but the exact-one-disk SCHD gate is still not
proven.
G75 remains FAIL/pending; no G75 PASS or M76 work is claimed.


## G75 block I/O corrective implementation (2026-08-03)

### Root cause and correction

The guest-visible “C: has no sectors” result was caused by missing target command coverage, not by the Transfer Info controller state machine: SCHD reached READ(10) (`28h`), but the command layer returned CHECK CONDITION `05/20/00` because READ/WRITE block commands were unsupported.  The correction is [a4d21e9](https://github.com/nakatamaho/vaeg/commit/a4d21e9a5e0a3b31818cc1dfcd8b281b3b62a67d).

`cbus/scsicmd.c` now has one common SXSIDEV-backed implementation for READ(6), WRITE(6), READ(10), and WRITE(10).  The reused backend functions are `sxsi_read()` and `sxsi_write()` in `fdd/sxsi.c`; the SCSI layer does not open or seek host files directly.  READ/WRITE(6) decodes the 21-bit LBA and maps zero length to 256 blocks.  READ/WRITE(10) decodes big-endian LBA/count and treats zero count as a successful no-data command.  Range checks are overflow-safe and return `05/21/00`; read-only media returns `07/27/00` for writes.

Reads stage and stream controller-sized chunks through the existing PIO/Transfer Info DATA IN path.  Writes collect complete DATA OUT chunks and commit with `sxsi_write()` only after the expected bytes arrive; commit count is traced and incomplete writes cannot report GOOD.  Chunk-boundary tests cover continuity without duplicate or missing bytes.

### Before/after evidence

Before the correction the normal discovery trace reached READ(10) and returned unsupported-command CHECK CONDITION `05/20/00`; no backend block was read.  After the correction the same path records:

```text
target_id=0 wd_target_lun=0 cdb_lun=0
CDB=28 00 00 00 00 00 00 00 01 00
LBA=0 block_count=1 sector_size=256 byte_count=256
DATA IN reads=256 backend_blocks=1 residual_bytes=0 status=00
STATUS/MESSAGE IN completion and bus free follow
```

The earlier “approximately eight devices” report did not include enough target/LUN/registration records to classify duplicates as LUN aliases, target-ID aliases, retries, or guest slots.  The corrected bounded path observes only target ID 0/LUN 0; a complete SCHD registration matrix is still required.

### Tests and validation

Production C selftests pass for READ/WRITE(6/10), zero-count rules, range/sense handling, persistent one- and multi-block writes, read-only protection, incomplete DATA OUT, chunking, read-after-write, and unsupported-LUN non-aliasing.  Existing Transfer Info, LUN, INQUIRY, SDL, and HOSTFAT/SASI selftest coverage remains green.

Passed commands:

```text
cmake --build build/linux-ci-clang --target vaeg_sdl2 -j2
cmake --build build/mingw-cross --target vaeg_sdl2 -j2
cmake --build build/macos-release --target vaeg_sdl2 -j2
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy build/linux-ci-clang/sdl2/vaeg --selftest
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy build/macos-release/sdl2/vaeg --selftest
ctest --test-dir build/m75-tests -R vaeg_m75_scsi_controller --output-on-failure
python3 tools/qa/m75_scsi_controller.py --root .
```

The real-ROM bounded run reached the corrected READ(10) DATA IN and completed 256 bytes with GOOD status.  A safety timeout later ended the run before SCFORM/filesystem acceptance; it is not reported as a semantic pass.  Final executable SHA-256 values are Linux `ec92e9e44b3ca2464e1861127b951d74f3945715d856f47086b4334adf15d7c4`, MinGW `ca0ff10048223f081d5a0c6b3836a48adba40186d72ca589085126214200b18c`, and macOS `f10f2a7b505f92633d27d0c1bbae7bc74717a82547ed4a034aab4cecbcaa5991`.

G75 is **FAIL/pending**.  Exact one-disk guest registration, SCFORM persistence, reboot/file round trip, manual SASI/HOSTFAT, and non-SCSI disk-path gates remain open.


## G75 FAT free-space / DATA OUT follow-up (2026-08-03)

The synchronized implementation head before this correction was
`02db38627d33612669043cd9b2146382170d9cbc`.  The successful support-disk
configuration extracted from the supplied D88 is:

```text
DEVICE = A:\PCPLUS.SYS
DEVICE = A:\SCHD.SYS -I0
```

That image does not contain a `-S256` option.  The bundled strings identify
PCPLUS as v1.08, SCHD as revision 1.55, SCFORM as revision 1.24, and the
PC-Engine as 1.10.  The `-S256` setting therefore remains a separate
configuration choice and was not silently inferred for this run.

### Production correction

The prior compatibility `0CC6h` output handler wrote directly into the DATA
buffer and, when the staging count was reached, fabricated STATUS/`8Bh`
without invoking the block command completion path.  This bypassed
`scsicmd_block_dataout_complete()` and `sxsi_write()`, so a legacy DATA OUT
request could report completion without a persistent backend commit.  The
correction is [d284468](https://github.com/nakatamaho/vaeg/commit/d284468fd256598489e07307fda58fbd1a0aa302).

`cbus/scsiio.c` now routes AR19 and legacy 0CC6h DATA OUT bytes through the
same payload accounting helper.  The 0CC6h path calls
`scsicmd_transinfo()` at a complete chunk; backend commit, chunk continuation,
status selection, and error propagation remain in the common command layer.
It no longer changes to STATUS or emits `8Bh` merely because a buffer offset
reached the expected count.  The data source is recorded as `ar19` or
`0cc6` in the trace, and `sxsi_write()` remains the sole media write API.

`scsicmd_transinfo()` also no longer overwrites a selected CHECK CONDITION
with GOOD when STATUS is transferred.  A failed `sxsi_write()` remains
`02h` through the STATUS phase and its sense data remains available to
REQUEST SENSE.

### Compiled verification

The production SDL selftest now includes:

```text
legacy_0cc6_write_reaches_backend_commit       PASS
legacy_0cc6_does_not_directly_complete_status   PASS
failed_backend_write_does_not_return_good      PASS
check_condition_survives_status_transfer       PASS
```

The existing READ/WRITE, chunking, LUN, INQUIRY, Transfer Info, SDL, and
focused controller tests remain green:

```text
cmake --build build/linux-ci-clang --target vaeg_sdl2 -j2             PASS
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy .../sdl2/vaeg --selftest   PASS
ctest --test-dir build/m75-tests -R vaeg_m75_scsi_controller          PASS
python3 tools/qa/m75_scsi_controller.py --root .                   PASS
```

### FAT evidence status

The available disposable 40 MB artifact
`/private/tmp/m75-scsi-40mb.hdd` has a 256-byte VHD header, 256-byte physical
blocks, 163840 physical blocks, and SHA-256
`b0e9ac0be0ddf010676ca8edcaddd650460bb988da00a081b191786fd15831c8`.
Its payload is all zero bytes and therefore contains no post-SCFORM BPB, FAT,
or root directory to parse.  The similarly named `m75-schd-40mb.hdd` has an
inconsistent legacy header and an all-zero payload; it is not used as FAT
acceptance evidence.  No fresh post-SCFORM guest trace or metadata image was
present in this checkout, so the zero-free-cluster result cannot be assigned
to a specific BPB/FAT byte here.  The exact first incorrect disk byte and the
AR19-versus-0CC6 classification remain open for the next WSLg run.

G75 remains **FAIL**.  The correction is committed and machine-tested, but a
fresh 40 MB SCFORM run must still prove the BPB, both FAT copies, nonzero free
clusters, persistent one-byte file creation/read/delete, and reboot
persistence.  The 160 MB multi-partition check and manual SASI/HOSTFAT and
non-SCSI gates also remain open.


## G75 FAT16 forensic inspection (2026-08-03)

A reusable read-only inspector was added as `tools/inspect_vaeg_fat.py`.  It
recognizes the VAEG `VHD1.00` container using the same 220-byte header layout
as `fdd/sxsi.c`, reports physical geometry, searches for structurally valid
FAT16 BPBs without silently selecting an ambiguous candidate, assembles
1024-byte logical sectors from four 256-byte physical blocks, reports FAT1 /
FAT2 equality and free-cluster counts, inspects the fixed root directory, and
compares changed physical-LBA ranges.  `--json` and `--compare` are supported.
Generated fixtures cover header and partition offsets, four-block BPB
assembly, healthy and full FAT16 tables, mismatched FAT copies, unused root
entries, and changed-LBA reporting.

At the initial inspection attempt the source and previously formatted images
under `/Users/maho/88VA/images` were inaccessible (`Operation not permitted`),
so no values were guessed.  A later user-provided copy under `/Users/maho/vaeg`
was inspected read-only; the resulting truncation and hash evidence supersedes
that earlier access limitation and is recorded below.

The previously available disposable VHD evidence remains separate: its valid
40MB VHD header reports 256-byte physical blocks and 163840 blocks, but its
data area is all zero bytes and contains no FAT BPB.  It is not used as
formatted-volume evidence.

Validation:

```text
PYTHONPYCACHEPREFIX=/tmp/vaeg-m75-fat-analysis/pycache \
  python3 -m unittest tools/qa/test_inspect_vaeg_fat.py -v       PASS (7)
ctest --test-dir build/m75-tests -R vaeg_m75_fat_inspection       PASS
```

A post-fix SCFORM image was not generated because the supplied image directory
was inaccessible and GUI guest automation is not available in this execution
sandbox.  G75 remains FAIL; the remaining sub-gate is to run SCFORM on a fresh
copy, inspect it with the new tool, and prove positive free clusters plus
reopen persistence before file-operation acceptance.


### Supplied `scsi40` image inspection (2026-08-03)

The user-provided files are now accessible at `/Users/maho/vaeg/scsi40.hdd`
and `/Users/maho/vaeg/scsi40_formatted.hdd`.  They were opened read-only and
were not modified.  Python `hashlib.sha256` was used because the sandboxed
`shasum` Perl runtime is unavailable.

| image | file size | SHA-256 |
|---|---:|---|
| `scsi40.hdd` | 1,244 bytes | `47ee49ebe280ff69d28a5f57e018e3d34da1f579ddb95ad7316222309980976a` |
| `scsi40_formatted.hdd` | 167,132 bytes | `c0b9d419638077e9e02b18854aabfcb978d35be5d091dff3fda5a3193754c60c` |

Both files have a `VHD1.00` header of 220 bytes (`sizeof(VHDHDR)` in
`fdd/sxsi.h`).  The header reports 256-byte physical blocks, 163,840
physical blocks, 40 MiB, and geometry `sectors=32`, `surfaces=8`,
`cylinders=640`.  The actual files are truncated: `scsi40.hdd` contains only
4 complete data blocks and `scsi40_formatted.hdd` contains 652 complete data
blocks.  They therefore do not contain the reported 40 MiB data area.

The inspector was run as follows (exit 1 is the intentional structural-error
result, not a crash):

```text
python3 tools/inspect_vaeg_fat.py \
  --image /Users/maho/vaeg/scsi40_formatted.hdd \
  --physical-block-size 256 \
  --compare /Users/maho/vaeg/scsi40.hdd \
  --json
exit=1
```

It found zero structurally valid FAT16 BPB candidates.  Consequently no
partition start, BPB, FAT1/FAT2 equality, free-cluster count, or root-directory
classification can be derived from these truncated artifacts without guessing.
The formatted file does contain early non-FAT/formatter data: physical block 0
contains the PC-88VA IPL/partition area, and later available blocks contain
partial formatter patterns.  This is not sufficient evidence for a complete
filesystem.

Among the available complete physical blocks, the formatted file differs from
the source at LBA 0 and at LBA 3--650 (649 changed complete blocks).  The
220-byte partial tail at physical LBA 651 also differs; the source tail is
zero-filled while the formatted tail contains nonzero bytes.  The comparison
therefore reports `compared_blocks=651`, `changed_blocks=649`, and a separate
changed partial tail rather than treating missing blocks as zero-filled media.

No post-fix SCFORM image was generated from these files: the source artifact is
truncated and cannot be used as a complete disposable 40 MiB target.  G75
remains FAIL.  A complete image (or a fresh run whose full sparse-file length is
preserved) is required before FAT16 metadata and persistent free-space/file
operations can be accepted.

## G75 complete SCSI image backing correction (2026-08-03)

Implementation commit: [e862711](https://github.com/nakatamaho/vaeg/commit/e862711) based on synchronized starting SHA `dcdb8797dd2eda76b5adf883d07157532401462f`.

The production source defines the VHD1.00 header as `VHDHDR` in
`fdd/sxsi.h`; its size is 220 bytes, not 256.  The canonical layout is:

```text
header             = sizeof(VHDHDR) = 220 bytes
data offset        = 220
physical block     = header.sectorsize (256 for the M75 image)
physical blocks    = header.totals (163840 for 40 MiB)
logical file size  = 220 + 163840 * 256 = 41943260 bytes
```

The previous creator wrote the header and IPL but never extended the backing
file to the declared logical length.  The new `newdisk_vhd_create()` path uses
checked 64-bit geometry arithmetic, writes the complete header, sets the exact
logical length with `ftruncate` (POSIX) or `_chsize_s` (Windows), flushes, and
renames a temporary `.hdd` file atomically.  Existing destinations are not
overwritten unless `--force` is explicitly supplied.  The SCSI open path now
rejects truncated or overlong files and reports declared blocks, block size,
expected bytes, actual bytes, and missing bytes.  `sxsi_read()` and
`sxsi_write()` enforce aligned in-range requests; short host I/O and flush
failures remain backend errors.

The native creation interface is available as:

```text
vaeg --create-scsi-hdd --output PATH --size-mib 40 --block-size 256
```

and the repository wrapper is `tools/create_vaeg_scsi_hdd.py`.  The inspection
tool now uses the same 220-byte production header, reports declared/actual
logical lengths, complete blocks, missing bytes and classification, and does
not attempt FAT decoding after a truncated image unless `--forensic-partial`
is requested.

The supplied images were not modified.  With the corrected production header,
`scsi40.hdd` is 1244 bytes = 220 + 4*256 and
`scsi40_formatted.hdd` is 167132 bytes = 220 + 652*256; both remain truncated
against their declared 163840 blocks and are rejected by the production open
path.  The inspector reports no FAT candidate for either artifact.

A fresh complete image was generated outside the repository:

```text
path        /tmp/vaeg-m75-image-create/scsi40_full.hdd
logical     41943260 bytes
allocated   4096 bytes (sparse)
SHA-256     79cbf423942f258454700666e56fa0a4a4d9d7027222cd1a40d67d4acb57bd5e
classification valid complete image
```

A disposable SCFORM preparation copy is
`/tmp/vaeg-m75-image-create/scsi40-scform-test.hdd`; no guest SCFORM was
performed in this run.  The production selftest covers first, middle and last
LBA reads, deterministic first/last block write/read persistence after reopen,
out-of-range and misaligned requests, exact file length, truncated-image
rejection, and refusal to overwrite an existing destination.  Linux and macOS
selftests pass.  The MinGW cross-build passes with `CCACHE_DISABLE=1`; the
initial ccache attempt was blocked by the sandbox's inaccessible host ccache
temporary directory.

Validation commands and results:

```text
cmake --build build/linux-ci-clang --target vaeg_sdl2 -j2             PASS
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy build/linux-ci-clang/sdl2/vaeg --selftest PASS
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy build/macos-release/sdl2/vaeg --selftest PASS
CCACHE_DISABLE=1 cmake --build build/mingw-cross --target vaeg_sdl2 -j2 PASS
PYTHONPYCACHEPREFIX=/tmp/vaeg-m75-image-create/pycache python3 -m unittest tools/qa/test_inspect_vaeg_fat.py -v PASS (9)
```

G75 remains **FAIL**.  The complete backing-store sub-gate is now machine
verified, but SCFORM on the new disposable image, FAT16 free-space evidence,
reboot persistence, file create/read/compare/delete, manual SASI/HOSTFAT, and
the non-SCSI disk-path gate remain to be performed.


## Guest-visible FAT accounting and exact-64KiB READ correction (2026-08-03)

Implementation commit: [a7d244d](https://github.com/nakatamaho/vaeg/commit/a7d244d61d93eedaf8498185ec55f8e8ac743926), based on the synchronized branch SHA `0a701ec65aed67f7f5df98c5a8d36f46c04ccf7a`.

The formatted VHD evidence is structurally valid and is not changed by this
correction.  Its decoded FAT16 geometry is: 1024-byte logical sectors, two
logical sectors per cluster, one reserved sector, two FATs, 39 sectors per
FAT, 640 root entries, partition start at physical LBA 256, and 19,918 valid
data clusters.  Each FAT has capacity for 19,968 entries; the 48 entries
after cluster `ClusterCount + 1` are padding and are not part of the data
cluster count.  FAT1 and FAT2 are equal, all 19,918 valid data-cluster entries
are free, and the root directory begins with an unused entry.  The inspector
and its generated tests now report valid-cluster, padding, reserved, bad, and
nonzero-padding counts separately.

The controller's 64KiB data window had two silent wrap hazards: AR19 DATA IN
used a 16-bit mask and the compatibility 0CC6h DATA IN path used a 15-bit
mask.  A 65,536-byte READ(6) (`cdb[4]=00h`, 256 blocks at 256 bytes) could
therefore repeat or address the wrong window.  Both paths now use checked
32-bit positions and report a trace invariant instead of wrapping.  Transfer
count programming is also recorded with the CDB transfer-length field, decoded
block/byte count, AR12/AR13/AR14 and reconstructed TC; `010000h` is treated as
65,536 bytes, not as one byte.  Backend, staging and delivered DATA IN bytes
now have per-command FNV digests and counts; matching READs report
`digest_equal=1`.

The SXSIDEV mount trace records the canonical image path, logical size, header
size, block size, declared block count, read-only classification and a stable
header/data fingerprint.  In the bounded normal-speed run against the valid
formatted image, READ(10) LBA 0, LBA 1 and LBA 256 each transferred one
256-byte block with zero residual and GOOD status; backend, staging and AR19
digests matched for all three commands.  TUR, 36-byte INQUIRY, READ CAPACITY,
and 36-byte MODE SENSE also completed.  The bounded run exited on its external
time limit before the guest reached the full CHKDSK/free-space sequence, so no
positive guest free-space or file-lifecycle acceptance is claimed.

Compiled production selftests cover the 65,535-byte, exact-65,536-byte and
out-of-window boundaries, READ(6) zero length as 256 blocks, READ(10) 256
blocks, a 65,537-byte chunk transition, transfer-count register order, and
backend/staging persistence.  The FAT inspector has ten passing generated
tests, including a fixture with nonzero FAT padding that must not inflate the
free-cluster count.  G75 remains **FAIL** pending bounded CHKDSK evidence,
nonzero guest free space, and persistent file create/read/compare/delete after
reopen.


## G75b WRITE path and standard screen capture (2026-08-04)

The guest-visible capacity gate is MET on the same-run text-plane capture. The
four CHKDSK lines are:

```text
  40792064 バイト : 全ディスク容量
  40792064 バイト : 使用可能ディスク容量
    524288 バイト : 全メモリ
    380144 バイト : 使用可能メモリ
```

The BPB arithmetic is `39,936 - 1 - 78 - 20 = 39,837` physical blocks,
`39,837 / 2 = 19,918` valid data clusters, and
`19,918 x 2,048 = 40,792,064` bytes. `CHKDSK completes` and `positive
available capacity` are MET. The capture format stores the run ID in both the
binary screen dump and the trace, so the decoded screen and controller trace
are proven to be from the same run.

The standard QA harness now sets `VAEG_SCREEN_TVRAM_DUMP` and
`VAEG_SCREEN_RUN_ID`, captures the text plane at scenario exit, decodes the
JIS character cells using the PC-Engine text-table geometry, and requires the
same run ID in the trace. Its focused decoder tests cover ASCII and JIS cells.
The separate `VAEG_SCREEN_DUMP` path captures the rendered SDL target as a BMP.

The first isolated guest file-creation WRITE was a WRITE(10):

```text
CDB       2a000000023c000004002b00
LBA       572
blocks    4
bytes     1024
TC        000400 (AR12=00h, AR13=04h, AR14=00h)
AR15h     00h (DataDirection bit 6 clear)
DATA      AR19, 1024 bytes
STATUS    GOOD; residual 0; commit_count 1
```

The former path committed the backend before the guest's AR19 DATA OUT
window was consumed, so the backend received stale bytes and the following
DATA OUT was rejected by the phase-direction check. The corrected path keeps
the direct WRITE active in DATA OUT, accepts every AR19 byte, and commits only
after the final byte. The completed trace reports equal backend, staging and
delivered byte counts and digests. The image changed on disk: the root entry
for `G75.TST` points to cluster 2, its size is one byte, cluster 2 contains
`X`, and both FAT copies agree with one valid cluster consumed.

The guest lifecycle scenarios also pass with screen output as the primary
result:

```text
G75 READ-REOPEN-DELETE OK
G75 DELETE PERSISTED
```

The first line verifies one-byte readback, close/reopen readback, and delete
in one boot. The second is a separate boot against the resulting image and
verifies that deletion persisted. After deletion the FAT inspector reports
19,918 free valid clusters, equal FAT1/FAT2, and an unused first root entry.

G75 remains open for the required SASI, HOSTFAT, and non-SCSI disk-path
regressions. The implementation and evidence do not claim those gates.

## HOSTFAT configuration recovery (2026-08-04)

The GUI now keeps a changed `HOSTFATDIR` pending until the asynchronous
replacement snapshot has been built and mounted successfully. Only then does
it persist `HOSTFAT`/`HOSTFATDIR` and reset the guest. A failed rebuild or
an emulator exit while the worker is active therefore preserves the previous
configuration and mounted snapshot.

At startup, an empty or unbuildable HOSTFAT directory loaded from the saved
configuration is treated as recoverable: vaeg reports the failure, writes
`HOSTFAT=false` while retaining the path, and continues boot without
HOSTFAT. An explicit `--hostfat-dir` failure remains fatal so command-line
automation does not silently lose its requested media.

Verification: Linux SDL selftest, HOSTFAT manager failed-rebuild retention selftest, invalid configured-directory startup recovery probe, and `git diff --check` passed. The change is committed in [bc51051](https://github.com/nakatamaho/vaeg/commit/bc510511326b9fdb3f61018d751dfc598159512a).

## HOSTFAT Windows Dropbox-root compatibility (2026-08-04)

The HOSTFAT GUI and snapshot builder now trim surrounding whitespace and
quotes from a selected path. On Windows the folder browser falls back to
`USERPROFILE` when `HOME` is not defined. A selected Windows root that is a
junction or directory reparse point is canonicalized before the immutable
snapshot is built; contained links and reparse points are canonicalized, while
links that escape the root remain rejected. This supports redirected Dropbox
roots without weakening the contents and containment checks.

The HOSTFAT snapshot selftest covers quoted paths on all platforms and covers a
Windows directory-reparse root plus a contained reparse-point directory when
the host allows the temporary test links.
Linux debug and MinGW cross builds were run after the change. The
implementation is in [1ec024b](https://github.com/nakatamaho/vaeg/commit/1ec024b),
with contained reparse-point support in
[7e6ede7](https://github.com/nakatamaho/vaeg/commit/7e6ede7).


## HOSTFAT rebuild error visibility (2026-08-04)

The Configure dialog now displays asynchronous HOSTFAT rebuild failures in
red directly below the `Rebuild + reset on OK` button, preserving the detailed
builder message such as FAT12 capacity, entry limit, depth, or unsupported-file
errors. A failed asynchronous rebuild also reopens the Configure dialog
automatically. The stale `127.44 MiB` label was
corrected to the actual `63.72 MiB` usable payload limit. The implementation
is in [55800c6](https://github.com/nakatamaho/vaeg/commit/55800c6), with
automatic Configure reopening in
[2515598](https://github.com/nakatamaho/vaeg/commit/2515598), with the error
positioned below the rebuild button in
[eb65a14](https://github.com/nakatamaho/vaeg/commit/eb65a14).

## HOSTFAT rebuild error dialog (2026-08-04)

A failed `Rebuild + reset on OK` operation now opens an explicit red
`HOSTFAT error` modal immediately after the asynchronous worker reports
its failure. The detailed failure text remains visible below the rebuild
button after the modal is dismissed, and the Configure dialog is reopened
for correction. Synchronous failure to start the rebuild uses the same
notification path. The implementation is in
[024855e](https://github.com/nakatamaho/vaeg/commit/024855e799750d762be0526cb10ada43a573f30d).

Verification: Linux debug `--selftest`, `M75_SCSI_CONTROLLER_OK`, Linux
release, macOS release, and MinGW cross builds passed.


## G75 disposable storage automation

The storage regression harness now covers the guest-level disposable flows
without modifying the source D88, ROM directory, or HOSTFAT source tree.

- --sasi-format boots a copy of the supplied PC-Engine 1.1 D88, executes
  its HDFORM.COM with HDFORM C: and confirmation input, and requires the
  completed-format screen with positive available capacity. It then runs
  separate guest processes for file creation, close/reopen readback to A:,
  and deletion, comparing the readback bytes with HDFORM.COM and checking
  the final SASI root directory.
- --g75-scsi creates a blank 40MiB, 256-byte-block VHD through the native
  image creator, runs SCFORM, validates the FAT16 geometry and both FAT
  copies, then runs separate guest processes for create, close/reopen
  readback, and delete.
- The readback step copies the SCSI file to the disposable A: disk and
  compares its bytes with the source file. The delete step checks both the
  guest screen and the backing FAT/root directory.
- --g75-scsi-two creates a disposable two-target boot copy with both SCHD
  target IDs, formats both images, and tests ID 0/C: and ID 1/D: separately.
- --full-g75 composes the SASI, one-disk SCSI, and two-target SCSI flows.
  Every guest step retains a same-run screen/trace pair and its headless
  input script.

The script's guest modes are mutually exclusive. `--selftest` checks only
fixture generation and input-script construction. All booting modes require
`--worker`, `--support-d88`, and `--roms`; `--sasi-format` and `--full-g75`
also require a source D88 containing `HDFORM.COM`. Each guest step is
performed by `tools/qa/m75_scsi_harness.py` with generated input and records
`screen.bin`, `trace.log`, and `headless-input.txt`. The runner rejects a
missing screen/trace pair, a mismatched run ID, or a termination other than
`process-exit`. The JSON emitted on stdout is the machine-readable result;
raw disposable evidence remains outside Git.

The normal-speed run completed the SASI lifecycle and the full SCSI flow:
HDFORM created the SASI filesystem, a separate process created G75SASI.COM,
a further process copied it after close/reopen to G75SASB.COM on A: with
identical bytes, and a final process deleted G75SASI.COM. The final SASI
screen reported no matching file and positive free space. SCFORM likewise
created one G75TEST.COM file, the next process copied identical bytes to
G75BACK.COM on A:, and the following process deleted the SCSI file. The
final SCSI root directory was empty and the FAT inspector reported positive
free clusters in both copies.

This is machine evidence for the disposable storage paths. It does not
approve the remaining M75 human gates, including non-SCSI disk regression,
GUI reset interaction, and any real-hardware comparison.

## M75 human gate checklist

The manual gate uses the normal frontend and keyboard path with disposable
copies of the support D88 and media images. Use the MinGW artifact at
`build/mingw-cross/sdl2/vaeg.exe` relative to the checkout. For the dual-SCSI
case, the boot D88 must load both `SCHD.SYS -I0` and `SCHD.SYS -I1`, yielding
C: for SCSI ID 0 and D: for SCSI ID 1.

The reviewer must record the final screen after each restart and verify:

- SASI: `HDFORM C:`, create `G75SASI.COM`, reset/reopen, copy it to A: as
  `G75SASB.COM`, compare bytes with `HDFORM.COM`, delete it, and confirm it
  is absent from the final `DIR C:` with positive free capacity.
- SCSI ID 0 and ID 1: format each target, create a file, reset/reopen,
  read it back to A:, compare bytes, delete it, and confirm the final root
  directory and free-cluster state.
- HOSTFAT: `TYPE` succeeds for the snapshot file and `DEL` is rejected.
- GUI recovery: a valid `Rebuild + reset on OK` changes the mounted HOSTFAT
  directory after reboot; an invalid directory shows the red error and
  leaves the emulator restartable without manually deleting `vaeg.cfg`.
- Non-SCSI disk operation and the bundled VA demo still work independently.

Any failed row keeps G75 at FAIL until the exact screen, configuration, and
writable disposable media copy have been retained for diagnosis. The
automated harness does not replace this frontend, GUI, non-SCSI, or real-
hardware review.


## G75 two-target SCSI automation

The disposable storage harness now also creates a boot-disk copy with both
`DEVICE = A:\SCHD.SYS -I0` and `DEVICE = A:\SCHD.SYS -I1`. It formats both
targets using SCFORM and verifies the normal PC-Engine drive assignment:
SCSI ID 0 is C: and SCSI ID 1 is D:.

For both targets, separate guest processes perform file creation, close/reopen
readback to the A: disk, and deletion. The host-side verifier compares each
readback byte-for-byte with `A:\BIN\SCFORM.COM`, checks both FAT copies, and
requires empty root directories and positive free-cluster counts after delete.
The source/support D88 remains unchanged; the generated two-target D88 and
images are disposable evidence. The normal-speed two-target run completed
create, close/reopen readback, and delete for both IDs. Both final root
directories were empty and both FAT copies reported 19,918 free clusters.
