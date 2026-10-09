# VAEG Intel 8087 v7 work plan

This is an implementation ladder, not a request to create another plan.

| Package | Scope | Prerequisites | Local result |
|---|---|---|---|
| P00 | [Baseline and evidence validation](packages/p00-baseline-and-evidence-validation.md) | None | PASS (local) |
| P01 | [Independent source and opcode inventory](packages/p01-independent-source-and-opcode-inventory.md) | P00 | PASS (local) |
| P02 | [SoftFloat 3e vendoring and backend validation](packages/p02-softfloat-3e-vendoring-and-backend-validation.md) | P00 | PASS (local) |
| P03 | [Singleton state and raw80 barrier](packages/p03-singleton-state-and-raw80-barrier.md) | P01, P02 | PASS (local) |
| P04 | [8087 clock and serial timing foundation](packages/p04-10-mhz-clock-and-serial-timing-foundation.md) | P00, P03 | PASS (local) |
| P05 | [Production CPU, bus, FPO1/FPO2](packages/p05-production-cpu,-bus,-fpo1-fpo2.md) | P01, P03, P04 | PASS (local) |
| P06 | [Exception transaction and WAIT/FN framework](packages/p06-exception-transaction-and-wait-fn-framework.md) | P01, P03, P04, P05 | PASS (local) |
| P07 | [Production vertical slice](packages/p07-production-vertical-slice.md) | P02-P06 | PASS (local) |
| P08 | [All transfers and packed BCD](packages/p08-all-transfers-and-packed-bcd.md) | P07 | PASS (local) |
| P09 | [Basic arithmetic and precision control](packages/p09-basic-arithmetic-and-precision-control.md) | P08 | PASS (local) |
| P10 | [Compare, constants, stack and control](packages/p10-compare,-constants,-stack-and-control.md) | P09 | PASS (local) |
| P11 | [8087 environment and save/restore](packages/p11-8087-environment-and-save-restore.md) | P10 | PASS (local) |
| P12 | [FPREM, FSCALE and FXTRACT](packages/p12-fprem,-fscale-and-fxtract.md) | P09-P11 | PASS (local) |
| P13 | [Independent transcendental oracle and prototypes](packages/p13-independent-transcendental-oracle-and-prototypes.md) | P01, P02, P03, P09 | PASS (local) |
| P14 | [F2XM1, FYL2X and FYL2XP1](packages/p14-f2xm1,-fyl2x-and-fyl2xp1.md) | P12, P13 | PASS (local) |
| P15 | [FPTAN and FPATAN](packages/p15-fptan-and-fpatan.md) | P14 | PASS (local) |
| P16 | [Full exceptional-case closure](packages/p16-full-exceptional-case-closure.md) | P08-P15 | PASS (local) |
| P17 | [VA machine integration and production interrupt route](packages/p17-va-machine-integration-and-production-interrupt-route.md) | P16 | INTEGRATION_BLOCKED |
| P18 | [Configuration, savestate and debugger](packages/p18-configuration,-savestate-and-debugger.md) | P11, P16 | PASS (local) |
| P19 | [Final QA and zero omissions](packages/p19-final-qa-and-zero-omissions.md) | P17, P18 | INTEGRATION_BLOCKED |

The default order is P00 through P19. Dependency-safe independent work may proceed
when a later machine-specific package is blocked, but final verification requires
all mandatory prerequisites.

The P00-P16 and P18 entries above are local machine-verifiable results from
the 2026-09-20 run. P17 is blocked by the missing VAEG-specific route evidence
recorded in [p17-route-blocker.md](records/p17-route-blocker.md); P19's
inventory, timing, core, and selftest components pass, but its final result is
fail-closed because P17 is unavailable. See [progress.md](records/progress.md)
for exact commands, digests, and the final gate output.

After each package, update `records/progress.md` and continue automatically.
