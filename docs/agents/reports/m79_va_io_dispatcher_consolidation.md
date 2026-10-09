# M79: VA I/O dispatcher consolidation report

## Scope

M79 starts from approved G78/main
[`23e9f4673e2e122835a5ad2fb256e6961f860866`](https://github.com/nakatamaho/vaeg/commit/23e9f4673e2e122835a5ad2fb256e6961f860866).
The implementation is recorded at
[`70da1cee1ba947e7c5f671e4891b0301372422ea`](https://github.com/nakatamaho/vaeg/commit/70da1cee1ba947e7c5f671e4891b0301372422ea)
on `topic/m79-va-io-dispatcher-consolidation`. The candidate gate is G79.
M79 owns dispatcher consolidation; removal of proven 98-only device
implementations remains deferred to M80.

## Findings and design

- The common and VA registration maps are semantically distinct. The active
  maps can contain different handlers for the same port number, so they remain
  separate while sharing lifecycle and dispatch structure.
- `_IOCORE` now owns the common and VA `_IOMODE` maps and one bus clock.
- `iocore_out8`, `iocore_inp8`, `iocore_out16`, and `iocore_inp16` are the sole
  active dispatch entry points; `iomode_va` selects the active map.
- VA default unhandled-port tracing and VA registration entry points now live
  in `io/iocore.c`.
- The duplicate dispatcher implementation `io/iocoreva.c` was removed. No
  98-only device implementation was removed; that is an M80 concern.
- Active `io/`, C-bus, and QA call sites use the canonical VA registration
  entry points. The HOSTFAT selftest explicitly exercises both common and VA
  dispatcher modes.

## Preserved behavior

The consolidation preserves the existing registration calls and map selection
for active VA devices and routes, including HOSTFAT, SASI/SCSI, keyboard,
mouse, sound, display, FDC, DMA, PIC, PIT, and C-bus board paths. The common
dispatcher terminator behavior, EGC/ARTIC special handling, VA 16-bit direct
access behavior, CPU remainder-clock updates, tracing, and state hooks remain
in the canonical dispatcher.

## Validation

The candidate was validated from the M79 implementation commit with:

- `cmake --preset linux-debug`
- `cmake --build --preset linux-debug --clean-first -j4` — PASS
- `ctest --test-dir build/linux-debug --output-on-failure` — no tests found
- `build/linux-debug/sdl2/vaeg --selftest` — all tests passed
- `python3 tools/repo/check_case.py` — `0 finding(s)`
- `python3 tools/repo/check_encoding.py` — PASS
- `python3 tools/repo/check_eol.py` — PASS
- `python3 tools/qa/upd9002_rename.py` — PASS
- `python3 tools/qa/m75_scsi_controller.py --root <candidate-checkout>` —
  `M75_SCSI_CONTROLLER_OK`
- active-source search for `iocoreva_` — no old dispatcher symbols found
- `git diff --check` — PASS
- `cmake --preset mingw-cross`
- `CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j4` — PASS

The MinGW artifact is a PE32+ x86-64 GUI executable at
`build/mingw-cross/sdl2/vaeg.exe`; its SHA-256 is
`c9d3e0b13678da23576180331a988ecb5b4110b8586be660ee8fdd5846b49251`.

Manual VA boot and device validation remains a required human G79 step and is
not claimed by this report.

## Gate status
G79 human gate passed on 2026-08-11. The maintainer confirmed the required
manual VA boot and device validation, so the candidate is approved for
fast-forward merge to `main`. M80 has not started.
