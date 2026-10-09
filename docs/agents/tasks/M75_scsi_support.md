# M75 - SCSI support cleanup and validation

M75 is reserved for SCSI support cleanup, validation, and VA integration.

Predecessor: the M73 integration baseline. The redefined M74 debug-harness
milestone is independent and is not a prerequisite for the completed G75
evidence.

Branch: `topic/m75-scsi-support`

Commit prefix: `M75:`

Candidate gate: `G75`

Report: `docs/agents/reports/m75_scsi_support.md`

Do not start M76. Do not merge M75 to `main` before G75 approval. Do not
declare G75 passed.

## Scope

M75 owns the active SCSI support path after M72 folds `SUPPORT_SCSI` to the
enabled side.

The PCPLUS documentation identifies `$SCSIBIOS` as the software SCSI BIOS
service supplied by `PCPLUS.SYS`. The PC-88VA SCSI55 guidance permits the
interface-board firmware ROM to be disconnected without affecting operation.
M75 therefore keeps the board ROM disconnected by default and does not claim
the historical `DC000h-DCFFFh` board-ROM window or a substitute `D2000h`
window in the VA guest map.

The supplied `SCSI55.TXT` identifies `0CC0h`, `0CC2h`, and `0CC4h` as the
documented board I/O addresses. The inherited `0CC6h` byte stream is retained
as the controller data leg in M75, but its independent hardware designation
remains unclaimed until PCPLUS/SCHD tracing or authoritative documentation
establishes it. The supplied `SETDMA.ASM` additionally proves that `0CCh` is
the software `$SCSIBIOS` interrupt: `SETDMA.COM` locates the `INT 0CCh`
handler, checks the `PCPLUS` signature at offset `000Ah`, and requests DMA
mode with `AX=82C0h`, `BL=01h`. It does not access `0CC6h` or program a DMA
channel. M75's default target is therefore the documented VA PIO path; the
`0CC6h` handler is a compatibility implementation rather than a new hardware
claim.

The supplied `SCHD.SYS`/`SCHD.DOC`/`SCHD.LOG`/`SCHD.TXT` are the PC-88VA DOS
block-driver evidence for this milestone. `PCPLUS.SYS` must precede `SCHD.SYS`;
`-I0..7` selects the target ID, while `-C`, `-S`, `-B`, and `-X` are driver
geometry, buffer, and removable-media options. A byte scan of the supplied
driver finds five `INT 0CCh` call sites and no `CD 1Bh` or literal direct
`0CC0h`-`0CC6h` port setup. This supports the documented PCPLUS software
SCSIBIOS boundary; M75's `0CC6h` registration is therefore kept as a
compatibility data path and remains subject to guest-level validation.

The standard local validation artifact names are:

- source disk: `pcengine110-bootonly.d88`;
- generated SCSI support disk: `pcengine110-scsi-support.d88`.

The source disk is user-supplied and remains outside Git. The generated disk
is also a local validation artifact and must not be committed.

M75 must:

- audit `cbus/scsiio.c`, `cbus/scsicmd.c`, BIOS SxSI helpers, SDL2 media UI,
  configuration, save-state entries, and ROM-less tests;
- confirm which PC-9801-55-compatible SCSI board behavior is active
  VA-supported behavior;
- validate against the documented PC-88VA SCSI support disk flow in
  `docs/modernization/scsi-support.md`, including a driver-installed boot disk
  when available;
- preserve SASI, HOSTFAT, and existing disk-image behavior;
- add or update focused validation for the active SCSI path where practical;
- document remaining hardware or guest-OS gaps.

The implementation checkpoints are:

1. expose the target-controlled SCSI phase and interrupt lifecycle after
   SELECT;
2. implement phase-aware TRANSFER INFO with REQ/ACK-sized completion;
3. cover STATUS and MESSAGE IN transitions and the PCPLUS software SCSIBIOS
   boundary;
4. implement the SCHD-observed TEST UNIT READY, INQUIRY, READ CAPACITY, and
   MODE SENSE commands against the existing SxSI image backend;
5. validate the support disk, SCFORM, and SCHD registration while retaining
   SASI and HOSTFAT behavior.

## WD33C93 host-contract checkpoints

M75 treats the PC-9801-55-compatible controller as a WD33C93-family
register interface, not as a freely designed SCSI state machine.  The
controller's host-visible contract is the authority for this milestone.

The host selects a register through `0CC0h` and accesses it through `0CC2h`.
`0CC0h` reads the auxiliary status; the documented PIO handshake is the
DBR bit, together with the controller-busy and command-in-progress bits.
Register `19h` is the fixed DATA window.  The CDB registers (`03h` through
`0Eh`) retain the normal address progression, and the NEC extension range
`30h` through `35h` must be audited before the phase engine is changed.

The required low-level sequence is event-driven:

```text
SELECT -> SCSI status 11h -> COMMAND request 8Ah
TRANSFER INFO/CDB -> DATA request 89h or 88h
DATA complete -> STATUS request 8Bh
STATUS complete -> MESSAGE IN request 8Fh
MESSAGE complete -> disconnect 85h
```

The `8Ah` request must be derived from the target/controller phase event.  A
wall-clock delay that happens to expose `8Ah` is not an acceptable
implementation.  The trace-only M75a checkpoint must record every `0CC0h`,
`0CC2h`, `0CC4h`, and `0CC6h` access with AR, value, `CS:IP`, auxiliary
status, SCSI status, phase, and interrupt assertion/clear events.

The observed PCPLUS path is specifically the low-level `07h Select without
ATN` followed by `20h Transfer Info` path.  It does not send MESSAGE OUT or
IDENTIFY.  The target LUN must therefore be obtained from the incoming CDB
where the protocol requires it; an unconditional IDENTIFY or intermediate
disconnect is not permitted.  `85h` is reserved for command completion and
bus-free handling unless the Control register explicitly authorizes another
disconnect interrupt.

The controller status register is a depth-one latch.  A second target event
must remain pending while the current status is unread.  Reading `17h`
consumes the latched status and permits the next pending event to assert INT.
An 8259 EOI is not the WD33C93 device-INT clear operation.  M75b/c tests must
cover ordered delivery of two events and must prove that an unread status is
not overwritten or cleared by EOI.

SELECT must return `11h` only for a configured target ID.  Other IDs return
`42h` (select/reselect timeout), with the timeout policy derived from AR
`02h` rather than treating every ID as present.

The supplied PCPLUS/SCHD trace records no `0CC6h` access in the active
low-level path.  Keep the inherited handler as a compatibility mapping, but
classify it as unused by this guest path rather than as a required WD33C93
port.  It must not be treated as a PC-9801-55 specification port without
separate guest or primary-source evidence.

The M75b1 register-value evidence must retain these observed inputs:

```text
AR=00h <- 07h
AR=01h <- 08h
AR=02h <- 80h, readback 80h
AR=11h <- 00h
AR=15h <- 07h, later 00h
AR=16h <- 00h
AR=18h <- 00h (RESET), later 07h (SELECT)
AR=30h read 00h, write 04h
AR=33h read 17h
0CC4h <- 02h (DMER reset; PIO selected)
```

The complete M75a trace also contains a reset command and must not be
described as a SELECT-only initialization.  A repeated run is accepted only
when the filtered `scsitrace` records are byte-identical and the no-access
interval after EOI is explicitly recorded.

M75c is complete only when the post-`8Ah` trace contains the controller's
transfer count writes at AR `12h`-`14h`, AR `18h`=`20h`, and the actual CDB
source (PIO DATA-window/DBR or an explicitly evidenced DMA path), followed by
CSR `1Ah` and a second interrupt.  PCPLUS merely progressing is not a
semantic acceptance criterion.

Before M75c implementation, the following M75b2 invariants are mandatory:

- reading AR `17h` advances AR to `18h`; the next write reaches COMMAND;
- three writes selected at AR `12h` reach `12h`, `13h`, and `14h`;
- repeated writes at AR `18h` remain COMMAND writes;
- repeated writes at AR `19h` remain DATA-window writes;
- AR `1Ah`-`2Fh` is held and reported as undefined/hardware-pending, with no
  invented wrap behavior;
- IRQ delivery is gated by `30h` IRE1 (membank bit 2), while the device CSR
  latch remains independent of the PIC wire;
- AR `31h` is the implemented memory-window register;
- Auxiliary Status LCI bit 6 and PE bit 1 are defined as zero/unmodeled until
  direct PCPLUS/SCHD evidence requires more behavior.

M75c is internally divided into three implementation checkpoints, but they
share the single terminal G75 gate:

1. **M75c1 — Service Required generation:** expose target COMMAND-phase
   `8Ah` only after the unread `11h` CSR has been consumed, then stop after
   the host writes AR `12h`-`14h` and AR `18h`=`20h`.  The transfer count is
   evidence, not a hard-coded CDB length.  M75c1 is implemented and its
   bounded trace reaches this exact stop.
2. **M75c2 — Transfer Info PIO:** pump bytes through fixed AR `19h` using
   DBR until the host-programmed transfer count reaches zero, then expose
   CSR `1Ah`.  Decode the CDB only after the transfer count completes.
   M75c2 reaches the AR `19h` CDB transfer and CSR `1Ah` boundary, but does
   not execute the decoded CDB or advance DATA/STATUS/MESSAGE phases.
3. **M75c3 — transfer classification trace:** record the phase, protocol
   direction, AR `19h` read/write counts, host count, legacy `0CC6h` access
   count, source path, IRQ request/assertion counts, completion CSR, and the
   captured CDB bytes for each TRANSFER INFO.  This checkpoint is trace-only
   and distinguishes an active DATA IN path from the current still-COMMAND
   path before CDB execution is connected.

None of these checkpoints is independently approvable or a new milestone
gate.

M75 does not claim that the physical REQ/ACK wires must be exposed as guest
ports.  The controller must instead reproduce the WD33C93 register and
interrupt contract that PCPLUS/SCHD observe.  NP2's simplified SCSI model is
not a specification source for this checkpoint.

The WD33C93A data sheet is the primary register authority for this milestone:

`http://www.bitsavers.org/components/westernDigital/WD33C93A_Data_Sheet_and_Application_Notes_Nov1990.pdf`

It specifies that indirect-register address auto-increment excludes the
Auxiliary Status, DATA, and COMMAND registers.  It also specifies that
Control DMA mode `000b` is polled I/O, with the host polling DBR before each
DATA access.  M75b2 implements only this PIO register boundary.  The 0CC4h
DMER reset observed from PCPLUS is evidence that DMA is disabled; TCIR, TCMR,
TCMS, and DMES remain hardware-pending and must not be emulated speculatively.

M75b2 also requires:

- AR `19h` DATA and AR `18h` COMMAND fixed-window accesses with no AR advance;
- Auxiliary Status composition with INT, LCI, BSY, CIP, PE, and DBR bits;
- AR `17h` as the only device-side CSR-latch consume operation;
- 8259 EOI not consuming or overwriting a WD33C93 CSR;
- AR `32h`, `34h`, and `35h` to remain explicit unsupported/hardware-pending
  accesses until PCPLUS/SCHD or board documentation proves their behavior;
- no DMA transfer path, DMA-channel synthesis, or 0CC6h hardware claim.

M75b1, M75b2, and M75c1-c3 are implementation checkpoints, not separate
human gates.
The single terminal G75 review remains the only approval boundary.  M75c must
still demonstrate the post-8Ah transfer-count writes, AR `18h`=`20h`, actual
AR `19h` CDB transfer, CSR `1Ah`, and the subsequent phase sequence.

### M75d1 — CDB decode and next-phase derivation

M75d1 uses one shared phase contract for the internal phase, WD33C93 service
request (`88h` through `8Fh`), and transfer direction.  The target command
helper owns CDB execution and selects the next phase; the AR `19h` PIO pump
only consumes the host-programmed transfer count and never selects a phase.

The INQUIRY acceptance sequence is allocation-length aware:

```text
COMMAND 8Ah -> TC=6 -> AR19 write x6 (12 00 00 00 24 00) -> CSR=1Ah
DATA IN  89h -> TC=36 -> AR19 read x36 -> CSR=19h
```

When the target response is shorter than the allocation, the target must stop
at the real response length and report the early phase change:

```text
DATA IN  89h -> TC=36 -> AR19 read x32
short transfer -> CSR=4Bh, residual TC=4
STATUS   8Bh -> TC=1 -> AR19 read x1 -> CSR=1Bh
MESSAGE  8Fh -> TC=1 -> AR19 read x1 (00h) -> CSR=1Fh
BUS FREE 85h only when Control bit 3 permits it
```

When the allocation is shorter than the target response, TC reaches zero,
the unrequested suffix is discarded, and normal DATA IN completion `CSR=19h`
leads to STATUS. M75d1 is not complete until one of these two contract paths
is observed with zero AR19 writes during DATA IN and the correct residual TC.
The first integration run reaches the corrected
`8Bh` request after TUR but the supplied PCPLUS path stops while inspecting
the transfer-count registers; this remains a blocking evidence item for the
single G75 gate.  The `--scsitrace-limit N` option is the deterministic
transfer-count termination control for bounded diagnostic runs; wall-clock
timeouts remain safety bounds only.

### PCPLUS raw CSR acceptance set

The PCPLUS interrupt normalizer and dispatch table define the following
canonical raw CSR values for the active PCPLUS/SCHD path:

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

This is a host-contract set, not complete WD33C93 silicon coverage. VAEG
generated controller statuses must remain within this canonical set. The
disassembly does not prove that `00h`, `01h`, `10h`, `41h`, `43h`-`47h`,
`80h`-`84h`, `86h`, or `87h` are silently rejected; they also normalize into
dispatch-table entries and remain semantically unverified. Validators must
not classify those values as proven no-op or rejection cases without new
PCPLUS/SCHD evidence.

For the current integration blocker, `1C0Eh` is a `CS:[047Eh]` read followed
by return, not a branch to `1C32h`. The relevant trace-only path is:

```text
19BBh phase comparison
  -> 1B60h or 1BA1h
  -> 1C14h transfer setup
  -> 1C32h AR=18h <- 20h
```

## Non-goals

M75 must not:

- move `iova/` sources;
- delete 98-only `io/` devices;
- redesign state-save format outside the SCSI evidence needed by this task;
- remove SASI or HOSTFAT;
- start VA-only source tree consolidation;
- claim complete physical WD33C93 or PC-9801-55 silicon behavior beyond the
  PCPLUS/SCHD contract evidenced here.

## Validation

Run the repository invariant checks, the normal CMake build, available native
tests, and focused SCSI/SASI/HOSTFAT smoke checks. Record unavailable platform
checks with exact blocker details.

### Current diagnostic checkpoint (2026-08-02)

The trace-enabled `linux-ci-clang` CMake configuration and `vaeg_sdl2` target
build both complete successfully. The resulting worker is
`build/linux-ci-clang/sdl2/vaeg` (SHA-256
`d69b11ad7b9bc3427042d808d3d06c4a3900e50a6427b15032f0f51b43c58836`).

A dummy-SDL run using the complete VA2 ROM directory,
`pcengine110-scsi-support.d88`, and a temporary 40 MB VHD target reaches the
real PCPLUS/SCSI path and records `CSR=11h`, `CSR=8Ah`, six AR19 CDB writes,
`CSR=1Ah`, and then `CSR=8Bh` on IRQ6. It records one transfer IRQ request
and one assertion, with no AR19 reads. An eight-second external safety bound
ends the run with status 137 before DATA IN, STATUS, or MESSAGE IN. This is
diagnostic evidence only: it reproduces the current handoff blocker and does
not satisfy the M75d1 golden sequence or the G75 gate.

## Closure

The final report must include the audited SCSI dependency graph, retained and
removed code paths if any, validation commands and exit statuses, manual gate
results, and a G75 human-review checklist.


## M75d1 addendum — target phase readiness

The handoff blocker is classified as a controller timing/state-boundary defect,
not an incorrect `8Bh` normalization. The next target phase was released when
AR17 consumed the previous CSR, while the PCPLUS foreground was still in its
completion path. The fix must keep CSR consumption and target phase readiness
as separate gates, hold DBR low until the controller processing event, and
avoid delaying same-phase PIO continuation. It must remain independent of
PCPLUS addresses, CDB order, filenames, and guest timing.

The corrected TUR evidence must show `8Ah`, six CDB writes, `1Ah`, deferred
`8Bh`, one STATUS byte `00h` with `1Bh`, one MESSAGE byte `00h` with `1Fh`,
and optional `85h` only when Control bit 3 enables ending disconnect. REQUEST
SENSE (`03h`) is part of the observed PCPLUS probe and therefore has a fixed
no-sense DATA IN response. The later INQUIRY (`12h`) remains a required
M75d1 evidence item and is not complete until its allocation-length contract
is observed: either the full DATA IN response with CSR `19h`, or a short
response with `0x48 | STATUS` and the correct residual TC.

The only terminal approval remains G75. This addendum does not approve G75 and
does not begin M76.


### M75d1 post-cursor and bounded-trace addendum

PCPLUS may issue one-byte `TRANSFER INFO` requests while a DATA IN response
still has bytes remaining.  The target DATA cursor must survive those request
boundaries and reset only at command entry or a real phase boundary.  The
controller must not make the first response byte repeat indefinitely.

For long diagnostic runs, `--scsitrace-no-guest` suppresses only the disabled-by-
default UPD9002 guest observation seam, and `--scsitrace-compact` suppresses
per-port records while retaining transfer results, DATA bytes, phase waits, and
warnings.  These are diagnostic-output controls, not guest or controller
behavior.  INQUIRY acceptance requires the normal-speed allocation/response
contract, including `4Bh` and residual-TC handling when the response is
shorter than the allocation.  Accelerated `--cpumult` runs are diagnostic
only and cannot satisfy that requirement.


The historical M75a worker digest in the earlier diagnostic checkpoint is
retained as provenance.  The current post-cursor worker digest is recorded in
the report addendum and must be used for any new evaluation.


## M75d1 follow-up: bus-free and multiplier evidence

The MESSAGE IN completion path must expose ending disconnect without requiring
a further `TRANSFER INFO` command.  For Control `08h`, the required terminal
status is `85h`; when ending disconnect is disabled, the corresponding bus-free
status is `80h`.  The implementation must keep the pending bus-free status
ready when `CSR=1Fh` is consumed, while retaining the processing gate for all
data-bearing phase changes.

PIO byte access remains synchronous with each AR19 access.  The byte helpers
must not schedule events or manipulate the CPU remaining-clock budget.  The
`--cpumult` diagnostic runs are not acceptance evidence until a normal-speed
36-byte INQUIRY record is observed.  Any accelerated partial-transfer symptom
must be investigated without adding a guest-tuned delay or a hard-coded CDB
path.

A clean build at predecessor `11cb0026ad646cff16237adef95e324fcedd40d9`
reproduced the same SASI unsupported-format/geometry rejection as the current
branch; this is not an M75 regression.  The terminal manual requirements remain
SCFORM initialization, SCHD registration, reboot, file create/read/delete,
and normal-speed INQUIRY DATA IN.


### M75d1 timing-unit audit and cpumult disposition

The CPU multiplier is a diagnostic timing mode, not an acceptance-speed
control.  Before treating accelerated partial transfers as a defect, audit
all SCSI delays in source:

- phase readiness and AR18=`20h` startup use
  `nevent_set(..., NEVENT_ABSOLUTE)` with emulated CPU-clock values;
- interrupt delivery uses the same emulated-clock event queue;
- AR19 PIO byte reads and writes remain synchronous to the individual I/O
  access;
- guest instruction work is scaled by `UPD9002_WORKCLOCK`, while VA I/O
  keeps the standard bus clock.

The audit establishes timing units but does not by itself classify the
behavior.  Because the normal-speed run currently stops before later DATA IN
while `--cpumult 8` reaches later CDBs, the timing mismatch remains an open
defect candidate.  Do not add a multiplier-specific delay or PCPLUS shortcut.
Compare the normal-speed and accelerated consumer paths first; normal-speed
release runs with compact/no-guest tracing and a generous external safety bound
remain required for INQUIRY acceptance.

The focused validator must also protect the interval from AR18=`20h` receipt
to the first DBR assertion: DBR is held low while the target-processing event
is pending, and that event is expressed in emulated CPU clocks.

The normal-speed INQUIRY allocation/response sequence remains unobserved and
is still a G75 blocker.  The current source response table is intentionally
32 bytes; byte4 is `1Bh` (32 minus five), and the controller must therefore
exercise the short-transfer `4Bh`/residual-TC contract for a 36-byte request.
No ANSI level, vendor string, or padding change is authorized without guest
branch evidence.


### M75d1 post-short-transfer run requirement

The short-transfer implementation is source-validated, but guest acceptance
requires a normal-speed run to observe either the full DATA IN completion or
`4Bh` with the residual count.  A bounded full trace reached TUR BUS FREE and
then a second SELECT/COMMAND request before its safety bound; it did not yet
reach the second CDB transfer.  Accelerated CPU multiplier runs may be used
only to inspect later CDB order and never satisfy the terminal G75 evidence.
SCHD registration, SCFORM, reboot, and file operations remain manual
acceptance requirements.


## M75d1 continuation — MODE SENSE geometry

The SCHD probe uses MODE SENSE(6) CDB `1A 00 04 00 24 00`.  The controller
must decode page `04h`, not silently return only the mode header and block
descriptor.  Commit
[03d4cd7](https://github.com/nakatamaho/vaeg/commit/03d4cd76541a3058cf32b0c239b499e0c0431627)
implements the empty page-00 response, page-04 rigid-disk geometry response,
page `3Fh` as the page-00 plus page-04 all-pages request, DBD-dependent
response layout, allocation-bounded transfer, and CHECK CONDITION/ILLEGAL REQUEST
for unsupported pages or
contradictory mounted geometry.  All counts and lengths derive from
`SXSIDEV`; fixed geometry values are prohibited.

The correction is source-validated and the accelerated diagnostic sequence
reaches MODE SENSE as DATA IN.  Normal-speed SCHD registration remains a
required evidence item and this continuation does not alter the G75 human
gate.


### M75d1 timing classification correction

The earlier cpumult closure is superseded.  Normal speed currently reaches
TUR but not the later MODE SENSE DATA IN within the bounded run, whereas
`--cpumult 8` reaches INQUIRY, READ CAPACITY, and MODE SENSE.  This asymmetry
keeps the timing mismatch open.  The next step is a paired consumer trace for
the second SELECT/COMMAND event: `1CCDh` is the intended wait consumer and
`1742h`/`1747h` indicate the main event pump.  No production delay change is
authorized until that evidence is collected.

### Timing follow-up record (2026-08-02)

The restored production build (`SCSI_TARGET_PROCESSING_CLOCKS=100`) confirms
that the timing mismatch is still open, not closed.  Normal speed reaches TUR
and a later SELECT/COMMAND request in low-overhead tracing, but no later CDB
transfer within the bounded run.  `--cpumult 8` reaches the MODE SENSE CDB and
`phase=19h, TC=24h` DATA IN requests within the 60-second bound; the 24-byte
request is abandoned before any AR19 byte and later one-byte requests complete.
This is not complete DATA IN evidence.

A 4000-clock target-delay experiment was run only diagnostically and reverted;
it did not advance normal speed beyond TUR.  Do not commit a delay change
without the paired second-`8Ah` consumer trace showing a general controller
contract defect.  G75 remains blocked on normal-speed INQUIRY/MODE SENSE
transfer accounting and the manual SCHD/SCFORM/file-operation checks.

### INQUIRY revision identification (2026-08-02)

The INQUIRY response revision field (bytes 24-27) now returns the fixed
ASCII revision `1.00`; the remaining four bytes of the eight-byte revision
area remain space padded.  The response remains a 32-byte table with byte4
`1Bh`; this does not change the allocation-length or short-transfer contract.
The M75 controller QA validator now rejects a response table whose revision is
not `1.00`.  This is an identification change only and does not constitute
normal-speed INQUIRY DATA IN acceptance.


### M75d1 Phase A — raw command-request windows

Before changing any production timing, use the trace-only option
`--scsitrace-cmdreq-windows`.  Window numbering is assigned by VAEG when raw
`CSR=8Ah` is presented; a guest `1D67h` write of `BAh` is recorded inside the
window and is not the window boundary.  Raw `11h` SELECT completion and `85h`
bus-free are recorded as surrounding events, along with the existing
`AR=18h <- 07h`/`AR=18h <- 20h` controller trace.

The seam uses the same emulated CPU clock as `nevent_set(...,
NEVENT_ABSOLUTE)`.  It records the first two `8Ah` windows, guest instruction
and clock timestamps, `CS:[047Eh]` before/after, `1CBDh`, `1CCDh`,
`1742h/1747h`, `1791h`, `1972h`, `19A7h/19BBh`, and `1C32h`.  It remains
completely disabled unless explicitly requested and must not alter guest
state, IRQ timing, CSR values, or PIO behavior.

Phase A is accepted only when the paired normal-speed trace distinguishes:

- first `1CCDh`, second `1747h`: early target phase presentation;
- both `1CCDh`: early-arrival hypothesis rejected;
- both `1747h`: the prior first-window interpretation is invalid; stop and
  reassess;
- no second raw `8Ah`: phase was not presented.

The trace-only change itself must preserve the existing `--scsitrace` output
when the new option is absent.  No delay constant or phase engine correction
may be implemented until this comparison is recorded.


Phase A trace-only implementation is committed at
`3667ed08701ba3e1863d659dfd47ddc954e25183`.  Local build, selftest, M75
validator, and focused CTest pass.  The real-ROM paired run remains pending
because the current macOS SDL Cocoa environment aborts during window
initialization before guest execution; this does not change the gate status.

### M75d1 transfer-count byte-order experiment (superseded)

An initial low/middle/high transfer-count experiment is superseded.  The WSLg
trace showed that it converted six-, one-, ten-, and eight-byte requests into
`TC=060000`, `TC=010000`, `TC=0a0000`, and `TC=080000`.  The original
high/middle/low order is restored by
[c959453](https://github.com/nakatamaho/vaeg/commit/c959453a0a482994ac25ab6db0b33e425306a0e9).
The MODE SENSE block-length offset correction remains.  Manual SCHD/SCFORM
acceptance and G75 are still open.

### M75d1 MODE SENSE block-descriptor correction (2026-08-02)

After the transfer-count correction, the guest reached SCHD's INQUIRY summary
(device code 0, response-data format 0, fixed-media mode) and then halted in
the following geometry path.  SCHD requests MODE SENSE(6) page 04h with
allocation length 24h.  The SCSI block descriptor stores block length in
response bytes 9--11, but VAEG had written it at byte 8.  This shifted the
reported block size and could make SCHD reject the geometry.

The production correction writes the block length at byte 9, preserving the
reserved byte at 8 and deriving the value from the mounted image geometry.  QA
now checks the source-level offset.  Local build, focused CTest, M75 QA, and
SDL selftest pass; MinGW/SCFORM manual confirmation remains required and G75
is still open.

### M75d1 transfer-count order correction from WSLg trace (2026-08-02)

The WSLg MinGW trace showed `TC=060000`/393216 CDB writes for a six-byte TUR,
`TC=010000` for one-byte STATUS, and `TC=0a0000`/`TC=080000` for READ CAPACITY
command/data.  These are the expected 6/1/10/8 values shifted by 16 bits and
prove the PCPLUS register order is high/middle/low (AR12/AR13/AR14).

The earlier low/middle/high experiment is superseded by
[c959453](https://github.com/nakatamaho/vaeg/commit/c959453a0a482994ac25ab6db0b33e425306a0e9).
The MODE SENSE block-length offset correction remains.  The rebuilt MinGW
artifact SHA-256 is
`d17a62569568d51ddfda1a8739824e52922772f9be870dfa98780d3abf4eac25`.
G75 remains open pending a new manual run.

### M75d1 CSR provenance trace (2026-08-02)

The trace-only CSR queue diagnosis is committed at
[ad9c99b](https://github.com/nakatamaho/vaeg/commit/ad9c99b2d36a810793a57b1162fc195229b009a3).
The synchronized starting SHA for this checkpoint was
`afd3dabe19dcca670847f7e397ead67c7cf38e33`; the upstream branch pointed to
the same SHA before the trace commit.  The change does not alter CSR admission,
event delay, guest memory, register state, IRQ timing, or PIO behavior.

Each CSR request now receives a trace-only sequence number and origin.  The
trace records `csr-request`, `csr-latch`, `csr-hostread`, `csr-promote`,
`csr-overrun`, and `csr-drop`, together with the event/latch/pending state,
raw status, phase, AR, auxiliary status, target-readiness flags, and `CS:IP`.
The sequence metadata is reset with the controller and is emitted only when
`--scsitrace` is enabled.  The existing pending slot is intentionally
unchanged until the provenance evidence distinguishes duplicate production
requests from a delayed host read.

A normal-speed VA1 bounded run with the standard SCSI support disk and a 40 MB
SCSI image produced request sequences 1 through 11.  Every observed event had
the order request -> latch -> hostread; no `csr-overrun`, `csr-drop`, or
`csr-promote` record was emitted in that run.  The run reached the second
SELECT/COMMAND request but did not reach a second CDB transfer before the
30-second external safety bound (`exit=124`).  A shorter semantic-limit run
exited 0 after the diagnostic stop and showed the same ordered prefix.  This
does not reproduce the older WSLg stale-`11h` report and does not justify a
production queue or delay change.

Validation for this trace-only checkpoint passed:

```text
python3 tools/qa/m75_scsi_controller.py --root .                         PASS
cmake --build build/linux-ci-clang --target vaeg_sdl2 -j2                  PASS
cmake --build build/m75-tests --target vaeg_sdl2 -j2                      PASS
ctest --test-dir build/m75-tests -R vaeg_m75_scsi_controller               PASS
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy build/m75-tests/sdl2/vaeg --selftest  PASS
```

The evaluated executable was `build/linux-ci-clang/sdl2/vaeg`, SHA-256
`5e55a19ecc2eeca505fa0bde0923cfdb9ce7781b65343ddadedc39035efec76d`.
G75 remains pending: the normal-speed INQUIRY DATA IN acceptance, SCHD
registration, SCFORM, reboot, file operations, and SASI/HOSTFAT/non-SCSI
regressions still require their respective evidence.


## M75d1 CSR admission and diagnostic invariants

The provenance detector is committed before the production correction:
[23b2752](https://github.com/nakatamaho/vaeg/commit/23b2752f36bc0571a705fecc3f25c964caf1d410).
Its pre-correction evidence includes a transfer-completion request arriving
while a SELECT-completion event was still active and a one-slot `pending`
state was already occupied.  The correction is
[ccb0666](https://github.com/nakatamaho/vaeg/commit/ccb066695907456314783cc3bb9a28dfad279c55).

The correction removes the CSR pending field rather than increasing queue
capacity.  Target-origin events are represented as target state and pulled
after AR17 consumes the visible CSR; host-synchronous transfer completion is
serialized by the host's TRANSFER INFO/data access.  Overlap is fail-closed
and trace-visible.  `NEVENT_SCSIWATCHDOG` reports unread CSR, stalled target
phase delay, and DATA IN decisions that do not become `CSR=89h` requests.

The diagnostic-only seeded jitter options are:

```text
--scsitrace-jitter-seed N --scsitrace-jitter-span N
```

The seed and effective samples are logged.  Fixed processing-clock stress at
4000 and 40000, plus five seeded runs at span 200, produced no CSR overrun,
drop, or invariant records.  A watchdog report in a bounded run remains a
real open progress observation and is not a pass.

M75d1 remains incomplete until normal-speed INQUIRY DATA IN, SCHD/SCFORM,
reboot/file operations, and required SASI, HOSTFAT, and non-SCSI regressions
are evidenced.  Do not declare G75 passed.

### M75d1 current stop and correction

The latest SCFORM/SCHD WSLg trace reached ID0.  TUR completed through STATUS,
MESSAGE IN, and `85h` bus free.  INQUIRY CDB `12 00 00 00 24 00` and DATA IN
`89h` were accepted, but PCPLUS reprogrammed `TC=1` after an initial `TC=36`
setup.  The previous implementation moved to STATUS after the first byte,
so SCHD saw only one INQUIRY byte and reported that no device was connected.

Commit [84bc2ef](https://github.com/nakatamaho/vaeg/commit/84bc2efe1de9e5661fd28d31ba087a304f1a82ac)
keeps DATA IN active across those repeated one-byte Transfer Info requests;
STATUS is selected only after the target response cursor is exhausted.  The
trace-only state capture is [d2da983](https://github.com/nakatamaho/vaeg/commit/d2da9835149d5d8dd4fb560c20bfd407db2719cc).

The QA validator, focused CTest, SDL selftest, and MinGW cross-build passed.
A fresh SCFORM/SCHD run with the corrected executable is still required.
G75 remains pending; do not start M76.

## M75d1 current implementation result (2026-08-03)

The required trace was captured once, then the WD33C93A Transfer Info path was
changed in production.  Commits [f0b14d7](https://github.com/nakatamaho/vaeg/commit/f0b14d71a2015b9469c92ea51abe2b9ebf964b43)
and [9827d09](https://github.com/nakatamaho/vaeg/commit/9827d09756779943d46b0973436f26f32142dced)
add an explicit Level-II lifecycle and the already-asserted-REQ case.  The
states are `idle`, `wait_for_req`, `transfer_byte_pending`,
`wait_for_post_count_req`, and `completed_or_terminated`.

The implementation rejects commands with INT pending (sets LCI without
changing the CSR/active command), keeps BSY for the active Level-II command,
requires REQ+DBR for byte transfer, decrements TC only after ACK completion,
terminates early phase changes with `4MCI`, and waits for a distinct
post-count REQ before successful completion.  `89h` is not generated while a
Transfer Info command is active, and the CSR latch remains depth one.

Machine validation is complete: the Transfer Info model reports 10 passing
tests, the controller validator passes, focused M75 CTest passes 2/2, the
Linux SDL2 build passes, and SDL selftest passes.  The evaluated executable
SHA-256 is
`24b78da6b70e28e865f54fed642c1ce5bdbbd347c66d76a81a20ba6487eb74ef`.
The MinGW cross build was unavailable because `build/mingw-cross` is not
configured.  Real-ROM integration remains open: bounded execution exited
`124` at the safety timeout after reaching STATUS Transfer Info wait, without
the INQUIRY DATA IN golden sequence.  SCHD/SCFORM, reboot/file operations,
SASI, HOSTFAT, and non-SCSI regression gates are therefore not passed.


## M75d1/G75 corrective implementation (2026-08-03)

The corrective implementation was completed from base `bfaced3fe6e3d59b067d5fc8e514ff4cc1cf4084`; it is recorded in [4e17c6f](https://github.com/nakatamaho/vaeg/commit/4e17c6f3fee67642ca69329147808cd18c71c9a7).  The prior trace identified concrete Transfer Info defects, so this change is production code plus compiled controller-path tests, not trace-only work.

`cbus/scsiio.c` now represents Transfer Info with the explicit lifecycle `idle`, `wait_for_req`, `transfer_byte_pending`, `wait_for_post_count_req`, and `completed_or_terminated`.  Successful completion is derived from the next service status (`0x18 | (service_status & 7)`): COMMAND-to-STATUS produces `1Bh`, DATA IN produces `19h`, STATUS-to-MESSAGE IN produces `1Fh`, and COMMAND produces `1Ah`.  The post-count REQ remains asserted with ACK clear and is retained across CSR read; the next Transfer Info consumes the same request sequence and does not generate a duplicate `8MCI`.

REQ and ACK transitions are separate operations.  Byte TC is decremented only after the byte handshake.  Message-In consumes the retained REQ, latches `20h`, negates target REQ while retaining ACK, and waits for Level-I Negate ACK; Negate ACK clears ACK and does not directly fabricate `85h`.  Commands with a pending CSR are ignored with LCI, while Level-I commands remain admissible when a Level-II command is active.  Transfer-count writes are traced in WD33C93 high/middle/low order; SBT single-byte semantics are explicit and no `010000h` compatibility exception exists.

Compiled tests are production C paths exposed by `--selftest` and registered as `vaeg_m75_transfer_info_compiled`; they cover successful CSR encoding, retained REQ identity, ACK/REQ transitions, message completion, Negate ACK, Level-I admission, CSR stability, LCI, count order, and SBT.  The existing controller validator remains part of the validation.

The real-ROM bounded run at semantic `--scsitrace-limit 30` exited 0 (semantic limit, not an external timeout) and showed: TUR COMMAND completion `1Bh` with retained STATUS REQ, STATUS completion `1Fh` with retained MESSAGE-IN REQ, MESSAGE-IN completion `20h` with ACK held, and later bus-free `85h`.  INQUIRY `12 00 00 00 24 00` transferred 32 response bytes from a TC=36 request and correctly returned `4Bh` with residual TC=4; the existing 32-byte payload was intentionally not changed.  READ CAPACITY, MODE SENSE, and READ paths also reached their protocol completions.  The trace showed no `010000h` count, no direction mismatch, no CSR overrun/drop, and no `0CC6h` access in this path.

Validation performed: `cmake --preset linux-ci-clang`; Linux SDL2 build; `python3 tools/qa/m75_scsi_controller.py --root .`; `python3 tools/qa/m75_transfer_info.py --selftest`; SDL selftest; focused CTest (`vaeg_m75_transfer_info_compiled` and `vaeg_romless_tests`, 2/2); and configured MinGW cross-build (`cmake --preset mingw-cross`, `vaeg_sdl2`, compiler `x86_64-w64-mingw32-gcc`).  The evaluated Linux executable digest was `SHA-256 01388de8ec1fe2a0e7df87d38f8607a734b6454688f7cb82471730e7c8cfca1d`.

G75 remains **FAIL/pending**: PCPLUS/SCHD registration, SCFORM, reboot, create/read/delete file operations, manual SASI/HOSTFAT regression, and the existing non-SCSI disk-path gate still require the manual integration run.  Do not start M76 or declare G75 passed.


### G75 LUN enumeration and INQUIRY correction (2026-08-03)

Starting from `1f2099eed524411113dda6db1a7bf8a77820c319`, the target backend was
corrected in commit
[103d59e](https://github.com/nakatamaho/vaeg/commit/103d59e).  The implementation
now has one central LUN predicate: the WD Target LUN register and the CDB LUN
must both be zero.  It is used by PIO and Select-and-Transfer paths for TUR,
REQUEST SENSE, INQUIRY, MODE SENSE, READ CAPACITY, and the common unsupported
command path.  Unsupported-LUN INQUIRY is GOOD with a 36-byte response whose
byte 0 is `7Fh`; other unsupported-LUN commands return CHECK CONDITION with
sense `05/25/00`.  No synthetic LUN devices are created.

The normal INQUIRY table is now exactly 36 bytes:

```text
00 00 02 02 1F 00 00 18 4E 45 43 20 20 20 20 20
4E 50 32 2D 48 44 44 20 20 20 20 20 20 20 20 20
31 2E 30 30
```

The additional length is `1Fh`, vendor is eight-byte `NEC` padding, product is
sixteen-byte `NP2-HDD` padding, and revision occupies offsets `20h--23h`.
Allocation length still truncates the response without changing byte 4.

The new compact selection/CDB-result trace records target ID, WD Target LUN,
CDB LUN, opcode/CDB bytes, selected backend index, INQUIRY byte 0, response
length, status, and sense fields.  The bounded normal-speed run observed only
`target_id=0`, `target_lun=0`, `cdb_lun=0`, and `selected_index=2`; normal
INQUIRY returned byte 0 `00h`, response length 36, and GOOD status.  No
LUN-1--7 or target-ID alias was observed in this run.  The earlier report of
approximately eight devices did not contain the new LUN/target matrix, so its
alias class is not retroactively claimed; the after-run evidence is
single-target/LUN0 only.

Compiled production-path selftests now cover all required normal and
unsupported-LUN cases, including Select-and-Transfer admission and the exact
36-byte fields.  Linux SDL selftest, the M75 controller validator, the
Transfer Info model check, focused CTest, Linux SDL2 build, MinGW cross-build,
and repository case/encoding/EOL checks pass.  The real-ROM bounded trace
also shows TUR, INQUIRY, READ CAPACITY, and MODE SENSE on target 0/LUN 0 with
the corrected 36-byte INQUIRY.  It subsequently issues READ(10), which is
outside the currently implemented command subset and returns CHECK CONDITION;
therefore SCHD registration and SCFORM have not been accepted yet.

G75 remains FAIL/pending.  Do not run SCFORM until a normal SCHD enumeration
run demonstrates exactly one configured disk; reboot/file operations, manual
SASI/HOSTFAT, and the non-SCSI disk-path gate remain open.


## G75 block I/O corrective implementation (2026-08-03)

The production correction is [a4d21e9](https://github.com/nakatamaho/vaeg/commit/a4d21e9a5e0a3b31818cc1dfcd8b281b3b62a67d), based on the synchronized branch head `1c5aeb75932b5eb73ce4fffc2650bd6c311a18eb`.  The target-side discovery path was already reaching READ(10); the concrete reason the guest later reported that C: had no sectors was that opcode `28h` (and the corresponding WRITE opcodes) fell through to CHECK CONDITION `05/20/00`.

`cbus/scsicmd.c` now decodes READ/WRITE(6) and READ/WRITE(10) in one common block path.  READ/WRITE(6) uses the 21-bit LBA and maps a zero transfer length to 256 blocks.  READ/WRITE(10) uses big-endian 32-bit LBA and 16-bit block count; zero means a successful no-data command.  Range validation uses `lba >= total || blocks > total - lba` semantics without unsigned wrap.  The byte count is checked against the WD 24-bit transfer representation.

The implementation reuses the existing SXSIDEV backend APIs `sxsi_read()` and `sxsi_write()` from `fdd/sxsi.c`; no direct host-file I/O was added to the SCSI command layer.  Reads stage at most the controller data buffer and continue as internal chunks while preserving LBA order and one final STATUS phase.  Writes collect complete DATA OUT chunks, call `sxsi_write()` only after a complete chunk, and count each backend commit once.  Incomplete, aborted, out-of-range, or read-only writes do not return GOOD.  Sense mappings are `05/21/00` for range errors, `07/27/00` for CD-ROM/read-only writes, and the existing medium-error convention for backend failures.

The compact block trace records the sequence, target ID, WD Target LUN, CDB LUN, all 12 CDB bytes, LBA, block count, sector size, byte count, backend index, and read-only flag at start.  Completion records transferred and residual bytes, backend block count/result, status, sense, and write commit count.

Compiled production selftests now cover READ(10) LBA/count decoding, zero-count semantics, last valid block and range errors; READ/WRITE(6) decoding including zero-length=256; one- and multi-block write persistence; out-of-range and read-only writes; no commit before complete DATA OUT; chunk-boundary read/write continuity; backend round trips; and unsupported-LUN isolation.  All block and pre-existing Transfer Info/LUN/INQUIRY tests pass through `--selftest`.

The normal-speed real-ROM trace after this correction shows target 0/LUN 0, READ(10) `28 00 00 00 00 00 00 00 01 00`, block-start LBA 0/count 1/sector 256, 256 DATA IN reads, backend block count 1, residual 0, GOOD status, then STATUS/MESSAGE IN and bus free.  This proves the previous `READ(10) unsupported -> CHECK CONDITION` defect is corrected.  The earlier eight-or-more-device report still lacks a complete guest registration matrix; no target-ID/LUN alias class is claimed from that report.

Validation after the implementation: Linux SDL2 build, MinGW cross-build, macOS SDL2 build, SDL selftests, focused M75 CTest, and `tools/qa/m75_scsi_controller.py` all pass.  The bounded normal-speed integration command reached and completed READ(10), but its external safety bound ended before a full SCFORM/filesystem run; that exit is not semantic success.  Final executable SHA-256 values are Linux `ec92e9e44b3ca2464e1861127b951d74f3945715d856f47086b4334adf15d7c4`, MinGW `ca0ff10048223f081d5a0c6b3836a48adba40186d72ca589085126214200b18c`, and macOS `f10f2a7b505f92633d27d0c1bbae7bc74717a82547ed4a034aab4cecbcaa5991`.

G75 remains **FAIL**.  Remaining acceptance gates are exact guest-visible one-disk SCHD registration, SCFORM against a disposable image, persisted sector changes, reboot and file create/read/compare/delete, manual SASI/HOSTFAT checks, and the existing non-SCSI disk-path regression.  Do not declare G75 passed or start M76.


## G75 FAT free-space / DATA OUT corrective follow-up

The current production correction is [d284468](https://github.com/nakatamaho/vaeg/commit/d284468fd256598489e07307fda58fbd1a0aa302).
The compatibility 0CC6h DATA OUT path now shares payload accounting and
backend completion with the AR19 path; it does not fabricate STATUS.  Failed
backend writes preserve CHECK CONDITION through STATUS.  The remaining gate
is an evidence-backed fresh SCFORM/FAT run using the exact support-disk
configuration (`PCPLUS.SYS`, `SCHD.SYS -I0`) and a new 256-byte-block image.
Do not infer FAT type or free-space state from the guest message alone.


## G75 FAT16 forensic tool

`tools/inspect_vaeg_fat.py` and `tools/qa/test_inspect_vaeg_fat.py` provide
read-only VHD/FAT16 inspection and generated regression fixtures.  The tool
must be run on private copies of the supplied source and old-formatted images;
it assembles 1024-byte logical sectors from four 256-byte physical blocks and
reports BPB, FAT1/FAT2, free clusters, root-directory state, and changed
physical-LBA ranges.  The original image directory was inaccessible in the
current sandbox, so no FAT result is claimed until those copies are exposed.


### Accessible image evidence correction (2026-08-03)

The previously inaccessible image paths are now readable under
`/Users/maho/vaeg`.  The supplied `scsi40.hdd` and
`scsi40_formatted.hdd` are VHD1.00 files with 256-byte blocks and the production 220-byte header.  The
header reports 163,840 blocks (40 MiB), but their actual lengths contain only
4 and 652 complete data blocks respectively; there is no partial data-block
tail under the production layout.  The read-only
inspector reports no valid FAT16 BPB candidate and cannot safely derive FAT or
root-directory state.  Their exact hashes, changed ranges, and truncation are
recorded in the M75 report.  Do not treat these partial artifacts as a valid
formatted-volume result or declare G75.

## Complete SCSI image backing follow-up (2026-08-03)

Implementation commit: [e862711](https://github.com/nakatamaho/vaeg/commit/e862711).

The production VHD1.00 layout is authoritative in `fdd/sxsi.h`: `VHDHDR` is
220 bytes, data begins at offset 220, and the 40 MiB/256-byte geometry has
163840 physical blocks.  The expected logical file size is therefore
`220 + 163840 * 256 = 41943260` bytes.  The earlier 256-byte-header wording
in this task was corrected after verifying the production struct.

The creator and validator correction is implemented in [the M75 report](../reports/m75_scsi_support.md): complete sparse logical length,
atomic temporary creation, strict open-time size validation, checked aligned
SXSIDEV I/O, and generated creator/inspection tests.  The supplied images
remain untouched and are still rejected as truncated against their declared
capacity.  A complete fresh image and disposable SCFORM preparation copy are
under `/tmp/vaeg-m75-image-create/`; G75 remains FAIL until guest SCFORM and
persistent filesystem operations are completed.
