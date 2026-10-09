# VAEG 8087 v6 — bounded implementation work plan

This has two separately authorized stages. The implementation ladder is not executable
until the acquisition/human-OCR boundary below has passed.
All package states below are the initial delivery state. Record actual state only in
[records/progress.md](records/progress.md); link real results there, not invented checkmarks.

## Read order

Initially read [goal-contract.md](goal-contract.md) section 0 and [acquisition.md](acquisition.md).
Run A00-A03, then STOP. After the user OCR handoff and Stage B authorization, read the
implementation companions and run P00.
Thereafter read the current package, its dependencies' evidence, and only the relevant
architecture/clock/numeric/verification sections. Avoid loading every package repeatedly.

## Acquisition ladder and human gate

| Package | Scope | Prerequisite | Initial state |
|---|---|---|---|
| [A00](packages/a00-workspace.md) | External workspace and policy | Stage A activation | NOT_STARTED |
| [A01](packages/a01-download.md) | Download allowlisted originals | A00 | NOT_STARTED |
| [A02](packages/a02-inventory.md) | Verify actual bytes and queue | A01 | NOT_STARTED |
| [A03](packages/a03-handoff.md) | Handoff and unconditional STOP | A02 | NOT_STARTED |
| HUMAN | User OCR plus explicit Stage B authorization | A03 | WAITING_FOR_STAGE_A |

Acquisition failure still produces a partial handoff. Nothing in the implementation ladder
is authorized by finishing A03 or by the passage of time.

## Stage B implementation ladder

| Package | Scope | Prerequisites | Delivery state |
|---|---|---|---|
| [P00](packages/p00-baseline.md) | Baseline, repository seams, and executable gates | Accepted user OCR + Stage B authorization | NOT_STARTED |
| [P01](packages/p01-source-inventory.md) | Independent 8087 source inventory and core contracts | P00 | NOT_STARTED |
| [P02](packages/p02-softfloat.md) | Pinned SoftFloat-3e and backend build validation | P00 | NOT_STARTED |
| [P03](packages/p03-state.md) | Singleton state, raw80 codec, classifier, and backend scope | P01, P02 | NOT_STARTED |
| [P04](packages/p04-clock.md) | Integer 10 MHz timebase and serial executor foundation | P00, P03 | NOT_STARTED |
| [P05](packages/p05-cpu-bus.md) | Native ESC/POLL/FPO2 and correct bus side effects | P01, P03, P04 | NOT_STARTED |
| [P06](packages/p06-exceptions.md) | Early exception transaction and WAIT/FN recovery framework | P01, P03, P04, P05 | NOT_STARTED |
| [P07](packages/p07-vertical-slice.md) | First integrated guest computation at configurable clocks | P02, P03, P04, P05, P06 | NOT_STARTED |
| [P08](packages/p08-transfers.md) | All real, integer, and packed-BCD transfers | P07 | NOT_STARTED |
| [P09](packages/p09-arithmetic.md) | Basic arithmetic, sqrt, and precision/rounding control | P08 | NOT_STARTED |
| [P10](packages/p10-control-compare.md) | Comparison, constants, stack, sign, and administration | P09 | NOT_STARTED |
| [P11](packages/p11-environment.md) | Exact 14-byte environment and 94-byte save/restore | P10 | NOT_STARTED |
| [P12](packages/p12-remainder-scale-extract.md) | FPREM, FSCALE, and FXTRACT | P09, P10, P11 | NOT_STARTED |
| [P13](packages/p13-oracle-prototypes.md) | Certified numerical oracles and bounded transcendental prototypes | P01, P02, P03, P09 | NOT_STARTED |
| [P14](packages/p14-log-exp.md) | F2XM1, FYL2X, and FYL2XP1 production handlers | P12, P13 | NOT_STARTED |
| [P15](packages/p15-partial-trig.md) | 8087 FPTAN and FPATAN production handlers | P14 | NOT_STARTED |
| [P16](packages/p16-exception-closure.md) | Complete exceptional-operand and recovery coverage | P08, P09, P10, P11, P12, P14, P15 | NOT_STARTED |
| [P17](packages/p17-machine-integration.md) | Production VA interrupt route, mode gating, and timing | P16 | NOT_STARTED |
| [P18](packages/p18-config-savestate-ui.md) | Settings, complete savestate integration, and read-only debugger | P11, P16 | NOT_STARTED |
| [P19](packages/p19-final.md) | Integrated QA, zero omissions, and truthful completion | P17, P18 | NOT_STARTED |

P13 prototypes can begin once their own prerequisites pass; P18 is intentionally independent
of P17's physical-routing evidence. Keep a single writer on shared CPU/scheduler files.
Only in Stage B, the default execution order is P00 through P19. Dependency-aware independent work may proceed
when a later package is blocked, but the final gate still requires every prerequisite.

## Planning decisions already made

The default clock is 10000000 Hz, with a reset-time editable integer setting. Only one optional
8087 exists. The first release is serial timed, uses exact fractional clock conversion, and
implements every documented instruction/form. The exception framework precedes bulk handlers.
Oracle-certified prototypes precede production transcendental kernels. The production VA
interrupt path is required; physical silicon matching is a separate evidence field.

Do not reopen these as an unbounded architecture discussion. P00 adds actual paths and build
commands; P01 resolves source evidence; a genuine conflict is recorded with a concrete choice.
No global milestone number is hard-coded. A local passing package is not goal completion.

## Gate and record lifecycle

P00 creates the real gate runner, as specified in [verification.md](verification.md). Each
package emits command/results/source digest and affected test IDs. Targeted regression gates
run per package; full baseline runs at P00, P07, P17, and P19. The final command is:

```sh
python3 tools/8087/gate.py --final
```

Never run an empty wrapper that returns success before tests exist. On budget pause, preserve
the worktree and record the earliest incomplete/invalidated package and exact next command.
On resume, inspect real HEAD and source digests before trusting previous reports.
