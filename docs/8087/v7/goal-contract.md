# VAEG + Intel 8087 — Implementation contract v7

**Target:** the checked-out VAEG repository.  
**Evidence root:** `/Users/maho/work/vaeg-8087-evidence-v6`.  
**Execution ladder:** P00-P19 in [work-plan.md](work-plan.md).

## Binding requirements

| Requirement | Value |
|---|---|
| Coprocessor | Intel 8087 only |
| Maximum installed | one |
| Default enabled | false unless current VAEG policy requires otherwise |
| Default clock | `8000000` Hz |
| Clock configuration | persisted integer Hz, independent of CPU frequency |
| Apply point | machine reset |
| Emulator policy range | 1,000,000 through 20,000,000 Hz inclusive |
| Presets | 5, 8, 10 MHz |
| FPO2 | no attached 8087 |
| Native/8080 modes | separate decode paths |
| Instruction coverage | all documented 8087 forms |
| Timing model | `SERIAL_TIMED_V1` |
| Arithmetic substrate | exact upstream SoftFloat Release 3e from S03 |
| Runtime host FP | prohibited in new 8087 runtime |

The default was amended to 8 MHz after the VA2 field measurement documented in
[`docs/modernization/8087.md`](../../modernization/8087.md). The 5 MHz and
10 MHz values remain selectable custom/preset values; this default is a
hardware-informed VAEG policy, not a claim that every PC-88VA board has the
same clock route.

## Completion

`SOFTWARE_VERIFIED` requires:

1. the real VAEG target and applicable baseline tests pass;
2. every documented 8087 form has a production handler and executed semantic test;
3. all 2048 D8-DF ModR/M decoder slots are classified exactly once;
4. no documented form reaches a stub, generic later-x87 fallback, or untested path;
5. the one-device limit and 8 MHz default/configuration lifecycle pass;
6. configured clock changes alter emulated NDP service time without changing
   non-time-dependent arithmetic results;
7. CPU-side FPO1/FPO2/POLL and no-device bus behavior are tested;
8. architectural state, raw 80-bit values, CW/SW/TW, TOP, environment/save images,
   exception masks, IEM/IC, WAIT/FN behavior, and pending exception handling pass;
9. all five 8087 transcendental instructions have deterministic production
   implementations and independent numerical/architectural tests;
10. the actual VAEG machine adapter has evidence-backed production BUSY/INT wiring
    and a guest handler test, or the final state remains `INTEGRATION_BLOCKED`;
11. provenance and link audits show no forbidden runtime dependency or private source;
12. `python3 tools/8087/gate.py --final` passes on the tested tree.

Physical silicon bit-for-bit transcendental identity and cycle-exact CPU/NDP overlap
are separate evidence axes, not implied by `SOFTWARE_VERIFIED`.

## Work discipline

Read all applicable `AGENTS.md` files and current repository conventions first.
Preserve unrelated changes. Do not run `git reset --hard`, `git clean`, force push,
broad reverts, automatic stashing, or history rewrites. Do not push or merge.

Continue automatically after each passing package. A package may stop only for a
real contract-defined blocker, unavailable required evidence, irreconcilable user
edits, unrepairable local test infrastructure, or system/run-budget interruption.

Never downgrade a documented instruction to unknown/reserved to satisfy coverage.
