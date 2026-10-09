# Architecture: one 8087 inside VAEG

> Applies only to Stage B, after the explicit user OCR handoff and acceptance in
> [goal-contract.md](goal-contract.md) section 0. Stage A must not execute this work.

Normative with [goal-contract.md](goal-contract.md). Evidence IDs refer to
[source-register.md](source-register.md). This fixes behavior boundaries, not guessed
repository filenames or unverified hardware facts.

## 1. Ownership and object count

The emulated machine owns one optional device state, directly or through its established
optional-device pattern. There is no socket list, `fpu_count` setting, multi-device bus,
per-slot clock, or second 8087. `enabled=false` means zero installed devices. Repeated reset,
configuration reload, and savestate restore must never accumulate callbacks or devices.

The state is owned by the same thread/execution context as the emulated CPU. Save/restore
SoftFloat's mutable settings around each call even for one device; test sequential CW
changes and preexisting backend state. Do not implement a generalized multi-instance
locking framework. If P00 discovers actual concurrent entry, use the existing ownership
mechanism or a minimal guard; never assume saving globals provides thread safety.

FPO1 is the native CPU interface used by the 8087. FPO2 is a separate native instruction
family and has **no attached device** in this goal. Preserve FPO2's CPU decode/bus behavior
without mapping it to the same 8087 or constructing another one. NEC N01 pp. 83-86 is the
starting source; applicability to the checked-out CPU must be audited.

## 2. Three distinct forms of state

**Architectural state:** eight physical 80-bit registers, TOP, full tags, control/status
words, instruction/data pointers, opcode field, and documented exception state. Use TOP
only for logical ST(i) addressing. Compute all source and destination physical indices
from the pre-instruction TOP where required, including FLD ST(i), FSTP ST(i), arithmetic
pop forms, FXTRACT, and the two-result/transcendental forms.

**Machine state:** enabled/present/accessibility, active clock, clock-conversion residue,
INT/BUSY adapter state, and any CPU wait continuation. These are not stored in guest
FSAVE/FSTENV images. Do not reset the active oscillator or user setting on guest FINIT,
FNSAVE, FLDENV, or FRSTOR.

**Configuration intent:** requested enabled value and requested clock, persisted through
the existing configuration mechanism. Pending settings are not silently applied to a live
machine. After loading a savestate, show saved active values separately from pending user
intent until the next machine reset.

## 3. Raw register encoding and backend barrier

A raw register consists of a 64-bit significand, including its explicit integer bit, and a
16-bit sign/exponent. An in-memory struct may have padding; the wire representation may
not. Its ten-byte codec writes significand little-endian at bytes 0-7, then sign/exponent
little-endian at bytes 8-9. Use integer loads/shifts, not type punning or host casts.

Wire-format canonicality is byte order only. Preserve all 80 bits, including unusual raw
encodings, in VAEG snapshots and traces. Guest instruction transfers follow their own
8087 rules: do not use a raw-preservation rule to bypass a documented conversion or
exception. Denormal short/long loads must be checked for their **8087 raw result**, not
only their mathematical value; a modern canonical normalization can be observably wrong.

Classify raw values before arithmetic: exponent zero/interior/all-ones, explicit integer
bit zero/one, fraction zero/nonzero, sign, and quiet/signaling interpretation. Keep tags
independent of that classifier: an empty tag is not a zero-valued register. Resolve normal,
zero, denormal, infinity, NaN, unnormal and each supported pseudo class per the instruction
matrix. SoftFloat does not define all non-canonical inputs (S01 section 4.4).

Only backend-safe canonical temporaries may enter SoftFloat. Save/set/clear/call/capture/
restore `softfloat_roundingMode`, `softfloat_detectTininess`, `extF80_roundingPrecision`,
and `softfloat_exceptionFlags` on every exit. Map 24/53/64 significand precision to backend
32/64/80 only for the operations to which that control applies. Backend intermediate
flags are not automatically guest flags. The 8087 denormal-operand exception, stack/tag
rules, IC/projective infinity, IEM, and old encodings remain device responsibilities.

## 4. CPU decode and memory ownership

Add hooks to the **production native CPU path** found by P00. Do not create a parallel
emulator used only in tests. Pass an explicit decoded operation containing opcode/ModR/M,
instruction-start physical address, effective-address information, bus-latch metadata,
and memory direction/width. Never let the FPU parse guest bytes a second time.

For memory-form ESC/FPO1, the CPU owns instruction consumption, default/override segment
selection, effective address, and the CPU dummy read. Intel I01 p. 3-97 states that a load
can capture the first data word during this read. Therefore pass that latched word to the
FPU and fetch only the remaining operand bytes through the machine bus when appropriate.
A duplicate first-word read is a bug on MMIO, even if RAM tests pass. Stores still retain
CPU-side dummy reads where required; these are not phantom coprocessor writes.

The bus API, not a cast of RAM to a C struct, performs m16/m32/m64/m80/BCD/environment
accesses. Audit odd addresses, ROM/open-bus behavior, memory-bank boundaries, 16-bit offset
wrap, 20-bit physical wrap, segment overrides, wait states, and any DMA-visible mapping.
Latch the physical starting address once when the hardware does; do not silently choose
between per-byte segmented wrapping and physical increment. Freeze the audited rule and
exercise boundary fixtures. Do not add modern protected-mode page faults or #MF to a
V30 path.

Numerical precommit is not transactional memory: decide whether a store is permitted
before starting it, then perform the machine's normal ordered bus operations. Never
promise rollback of an MMIO read/write or a partly completed multiword bus transaction.
Test existing bus failure semantics only where the machine exposes them.

## 5. Native mode is not 8080 mode

Evaluate CPU mode **before native decoding**. The same byte values have different meanings
in 8080 mode. In particular, a native absence test saying D8-DF consumes ModR/M or 9B waits
must never be applied to the 8080 decoder. Keep that decoder, its cycle rules, and CPU flags
unchanged. Test representative overlapping bytes through the actual 8080 path and prove
that no 8087 callback occurred.

Temporary native-to-8080 transitions preserve the device registers by default; they gate
new native access rather than detach/reinitialize the optional chip. Record the
machine-specific INT behavior across that transition from evidence. Do not silently clear
sticky exceptions. The original VA/VA-91 policy and invalid-model config handling must not
create a hidden accessible FPU.

## 6. Exception effects, not a single rollback switch

Build the exception transaction machinery **before** implementing broad instruction
families. Every handler supplies proposed register/tag/TOP/condition-code/pointer effects,
permitted memory effects, exception candidates in documented priority order, and the
specific masked/unmasked disposition. No eager pop/store before this decision.

Do not implement `if (unmasked) discard_everything`. Different exceptions and destinations
can require different result handling; unmasked overflow/underflow/precision must be
checked against 8087 sources, including any adjusted results. Conversely, do not always
commit the rounded result before recording an exception. P06 freezes this mechanism;
P08-P16 add instruction-specific cases through it.

Keep the six sticky flags, per-exception masks, general IEM/IM bit, IR summary, status B,
physical BUSY behavior, and external INT level conceptually separate. Recompute the
relevant derived signals after FLDCW, FENI/FDISI, FCLEX, FINIT, FLDENV, and FRSTOR according
to source evidence. FDISI does not mean clearing every exception flag. Stack faults use
8087 invalid-operation behavior; do not import the later x87 stack-fault status bit.

For each condition-code bit record `SET`, `CLEAR`, `PRESERVE`, or `UNDEFINED_POLICY`.
An undefined bit may preserve its prior value as an explicit compatibility policy; a bit
specified as preserved must actually preserve it. Test initial condition codes both set
and clear. In particular audit the early FPREM quotient-bit preservation in I01 Table 4b,
not just the familiar later-x87 quotient mapping.

## 7. WAIT and no-WAIT instructions

Use actual byte streams. A WAIT-prefixed administrative spelling contains a separate 9Bh
CPU instruction followed by an ESC instruction; do not merge them in a way that erases
interrupt recognition or instruction-pointer semantics. Also test raw ESC forms, using
explicit bytes when an assembler inserts a WAIT automatically.

In native absent-device state, only the audited CPU POLL cost remains; no nonexistent
BUSY source blocks. Present idle POLL is similarly finite. Pending unmasked exceptions
must preserve pending state and use the real CPU wait/interrupt machinery. A blocked POLL
must yield to the machine scheduler and eligible interrupts, not spin forever in a host
loop. IF=0 or a masked machine route may legitimately stall guest progress, but the UI,
reset, and unrelated device events must remain responsive. Never time out and silently
clear the guest exception.

Test allowed no-WAIT administrative recovery while pending. The entry policy is
instruction-specific; no blanket early `if (pending) return` may prevent FNCLEX/FNINIT or
other source-permitted recovery. Do not automatically retry a suppressed faulting store
without a documented mechanism.

## 8. Guest environment versus emulator savestate

Guest FSTENV/FLDENV transfer 14 bytes; FSAVE/FRSTOR transfer 94 bytes. I01 Figure 8 is the
initial layout source: CW, SW, TW at offsets 0,2,4; low instruction address at 6; high
instruction address/opcode at 8; low data address at 10; high data address at 12. The upper
address nibbles occupy the source-defined positions; do not pack a segment selector there.
Use exact bit masks and independent golden byte images, including reserved output bits.

Resolve the 11-bit opcode capture point, instruction/prefix address rules, pointer update
exemptions, saved register order, and post-save initialization from the detailed 8087
manual before declaring P11 passed. FSTENV's mask side effects and FN versus WAIT forms
need tests. Do not blindly copy FXSAVE, a 287 protected-mode image, or host structs.

VAEG savestates additionally store active clock, conversion residue, presence, version,
and pending machine state. Load into a validated temporary state, reject malformed lengths,
invalid clocks, impossible device counts, and incompatible versions before mutation.
After restoration reconcile interrupt-controller line state without creating an extra
edge, stale callback, or duplicate event. Do not execute FINIT after restoring the payload.
For old states, apply the repository's audited compatibility policy; never guess missing
bytes are a valid clock. User disk configuration must not be overwritten by state loading.

## 9. Machine interrupt integration

Locate the actual VA interrupt controller and route from existing machine documentation,
service evidence, or user-provided disassembly. Record source ID/page/address plus the
production signal path. The interface must retain assertion/deassertion, machine mask,
CPU interrupt eligibility, acknowledgment, handler entry, clear, return, and subsequent
POLL behavior. Counting the host callback does not test these.

Do not add a permanent routing selector to avoid resolving a missing connection. A
developer-only synthetic route is useful for core tests but must be unmistakably marked
`TEST_FIXTURE_ONLY` and cannot pass the production route gate. Missing physical hardware
is acceptable; a missing implemented production route is not. See completion rules in
the main contract.
