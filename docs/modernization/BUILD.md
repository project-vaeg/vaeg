<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->
# Modernized Build Recipes

These are the G11 maintainer recipes for the portable SDL2/ImGui build.
The legacy Visual Studio projects remain reference artifacts and are not
covered here.

## Windows Native: MSYS2 MINGW64

This is the supported Windows runtime configuration. Build and run the
native Windows binary from an MSYS2 MINGW64 environment, then distribute
`vaeg.exe`. The executable statically links SDL2, LibArchive, zlib, lzma,
librashader, and the MinGW gcc, libstdc++, and winpthread runtimes.

Open the MSYS2 MINGW64 shell, then install the build prerequisites:

```sh
pacman -S --needed mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja \
  mingw-w64-x86_64-gcc mingw-w64-x86_64-pkgconf python git
```

Install Rust 1.88.0 with the GNU Windows target. Prepare librashader's
static archive and dependency notices, then configure and build:

```sh
rustup toolchain install 1.88.0 --profile minimal \
  --target x86_64-pc-windows-gnu
rustup default 1.88.0
tools/release/build-mingw-static-librashader.sh
cmake --preset mingw-release
cmake --build --preset mingw-release
```

The output is `build/mingw-release/sdl2/vaeg.exe`. The preset builds a
GUI-subsystem executable. For a console while debugging, configure a
separate tree with `-DVAEG_WINDOWS_CONSOLE=ON`.

The historical VAEG icon is embedded both as a Windows executable resource
and as the cross-platform SDL window icon. No adjacent icon file is required.

## macOS Release

The release preset fetches pinned SDL2 and links it statically:

```sh
cmake --preset macos-release
cmake --build --preset macos-release
```

The output is a plain `build/macos-release/sdl2/vaeg` binary. macOS
system frameworks remain dynamic operating-system dependencies. The
historical VAEG icon data is embedded and supplied to SDL for the running
application. The plain executable is not an application bundle, so Finder
does not assign it a bundle document icon; that would require a separately
audited `.icns` asset and an app-bundle packaging milestone.

## macOS Development: MacPorts

The M11 baseline is MacPorts under `/opt/local`, not Homebrew. Install:

```sh
sudo port install cmake ninja libsdl2 pkgconfig
```

Configure and build:

```sh
cmake --preset macos-macports
cmake --build --preset macos-macports
```

The MacPorts preset sets `CMAKE_PREFIX_PATH=/opt/local` and
`PKG_CONFIG_PATH=/opt/local/lib/pkgconfig`. A plain binary is accepted for
G11; no app bundle is required.

## Linux To Windows Cross Check

The agent-side MinGW cross check uses the same fully static dependency stack:

```sh
rustup toolchain install 1.88.0 --profile minimal \
  --target x86_64-pc-windows-gnu
rustup default 1.88.0
tools/release/build-mingw-static-librashader.sh
cmake --preset mingw-cross
cmake --build --preset mingw-cross
python3 tools/release/check-static-windows-imports.py \
  --binary build/mingw-cross/sdl2/vaeg.exe
```

The staged output is `build/mingw-cross/sdl2/vaeg.exe`. Its import audit
allows only Windows system DLLs. This preset remains a link-check tier;
Windows release artifacts should be produced from the supported MSYS2
MINGW64 native configuration.

Linux executables also contain the same icon data and set it through SDL for
window-manager and task-switcher use. The project does not currently install
a desktop entry or an icon-theme payload, so launchers do not receive a
system-wide application icon from this unpacked-binary distribution.

## GitHub Actions CI

The M12 workflow is `.github/workflows/build.yml`. It covers:

- Ubuntu with system SDL2 from `apt`, in separate gcc and clang jobs for both
  production Z80 selections.
- Ubuntu ASan/UBSan smoke. The known UBSan backlog messages documented in
  `../agents/reports/ubsan_backlog.md` may appear, but sanitizer process
  failures still fail the job. Both Z80 selections are covered.
- Windows on `windows-latest` through MSYS2/MINGW64 with pinned static
  FetchContent SDL2, for both Z80 selections.
- macOS on `macos-latest` with `VAEG_FETCH_SDL2=ON`. The maintainer
  baseline remains MacPorts, but MacPorts is impractical on hosted
  runners; this CI job deliberately exercises the pinned FetchContent path
  from ADR-0006. Both Z80 selections build and run ROM-less tests natively.

CI has no ROMs or disk images by design. `--smoke` therefore runs in a
reduced-scope ROM-less mode on hosted runners: SDL initialization, core
loop, one GUI frame, and clean exit. If a complete VA ROM set is present,
the same smoke command enables the uniform-screen detector and logs that
mode. Behavioral ROM and disk validation remains a human gate.

`VAEG_ENABLE_TESTS=ON` enables the ROM-less ctest suite. Current tests are
the codecnv wave-dash round-trip, ini read/write round-trip, and statsave
save/check/load on stable sections. M9 did not produce a memoryva fixture,
so the memoryva fixture test is intentionally skipped.

CI uploads build artifacts for all three operating systems. The Windows
artifact is the standalone `vaeg.exe`; its import audit rejects SDL2 and
MinGW runtime DLL dependencies.

## Standalone Z80 compatibility conformance

`VAEG_ENABLE_TESTS=ON` also builds the vendored suzukiplan/z80 header without
linking it into the emulator. The build covers the default callback API,
`Z80_NO_FUNCTIONAL`, the default debug API, and a release-oriented configuration
with debug, breakpoint, nest-check, and exception support disabled. CTest runs
the focused interrupt extension in both callback configurations.

ZEXDOC and ZEXALL are external GPL test inputs and are never stored in the
source tree or release archives. The dedicated CI job downloads the five files
from `suzukiplan/z80` commit
`e3926769a790fab0af1c34a5540e317f8d4f0ddc`, verifies every approved SHA-256,
and keeps them only in an optional CI cache. To acquire them online locally:

```sh
python3 tests/z80_compat/fetch_zex.py --output-dir build/zex-cache
```

For offline use, point the acquisition tool at a directory containing
`zexdoc.cim`, `zexall.cim`, `zexdoc.src`, `zexall.src`, and the upstream
`test-ex/LICENSE.txt`:

```sh
python3 tests/z80_compat/fetch_zex.py \
  --source-dir /path/to/suzukiplan-z80/test-ex \
  --output-dir build/zex-cache
```

Configure the test-bearing build with the verified cache, then run only the
standalone suite or the complete CTest set:

```sh
cmake --preset linux-ci-gcc \
  -DVAEG_ZEX_ARTIFACT_DIR="$PWD/build/zex-cache"
cmake --build --preset linux-ci-gcc
ctest --test-dir build/linux-ci-gcc --output-on-failure \
  -R '^vaeg_z80_compat_(header|interrupt|zex)'
```

The runner uses deterministic 64 KiB memory, loads at `0x0100`, supplies only
the CP/M CALL-5 services used by ZEX, and stops on an emulated-clock or wall-
clock limit with register and recent-output diagnostics. The archive checker
rejects both known artifact names and the five pinned content hashes.

The independently authored compatibility layer is the production Z80 seam;
the standalone `vaeg_z80_compat_wrapper` and `vaeg_z80_compat_wrapper_no_functional`
libraries remain permanent conformance targets. Their
consumer-facing declarations are under `cpu/`; third-party and STL types do
not cross that interface. The corresponding `vaeg_z80_compat_wrapper_default` and
`vaeg_z80_compat_wrapper_no_functional` tests cover the vaeg bus, clock, interrupt,
public-register mirror, and revision-1 state contracts. When a verified ZEX
cache is configured, `vaeg_z80_compat_wrapper_zexdoc` and
`vaeg_z80_compat_wrapper_zexall` run the same external inputs through the wrapper.
The production `vaeg_va` target unconditionally compiles
`cpu/z80_compat_cpu.cpp` and `cpu/z80_compat_state.cpp`.

The permanent M38-derived regression corpus uses two deterministic runs of the
current wrapper and a canonical trace comparator. The ordinary CI corpus is
ROM-less:

```sh
cmake --build --preset linux-ci-gcc --target \
  vaeg_z80_compat_trace_reference vaeg_z80_compat_trace_repeat vaeg_z80_compat_trace_compare
ctest --test-dir build/linux-ci-gcc --output-on-failure \
  -R '^vaeg_z80_compat_regression_'
```

The public generated test uses seeds `0x4d383001` through `0x4d383004` and
128 cases per seed. A longer maintainer-local run uses 4,096 cases per seed:

```sh
cmake \
  -DREFERENCE_RUNNER="$PWD/build/linux-ci-gcc/vaeg_z80_compat_trace_reference" \
  -DREPEAT_RUNNER="$PWD/build/linux-ci-gcc/vaeg_z80_compat_trace_repeat" \
  -DCOMPARATOR="$PWD/build/linux-ci-gcc/vaeg_z80_compat_trace_compare" \
  -DSUITE=generated -DCASES=4096 \
  -DOUTPUT_DIR=/tmp/vaeg-m38-long \
  -P tests/z80_compat/differential/run_differential.cmake
```

`--suite scheduling` is a deliberate slice-exact evidence case, not an
ordinary green CTest. It retains the unallowlisted FDD port-`0xf4` timing
divergence documented in `docs/agents/reports/m38_z80_differential.md`.
`vaeg_z80_compat_regression_convergence` retains the green policy reproducer: the
program and initial `1,7` slices receive additional `4,1` slices and emit
exactly one `OUT (0xf4),0x5a`, with stable final state and event order.

## Production Z80 compatibility backend

The suzukiplan-backed wrapper is the only production Z80 implementation. No
build-time or runtime core selector remains. A normal build is sufficient:

```sh
cmake --preset linux-ci-gcc
cmake --build --preset linux-ci-gcc
ctest --test-dir build/linux-ci-gcc --output-on-failure
```

The ROM-less selftest exercises the production subsystem, including the
revision-1 state bridge, SLEEP_HACK/WAIT behavior, acknowledge timing, and a
port-`0xf4` state boundary. See [Z80 compatibility production integration](z80-compatibility-integration.md)
for the contract and trace command.

Public CI performs one native Linux, Windows, and macOS production build and
keeps the standalone wrapper, state, interrupt, ZEX, disassembler, and
M38-derived regression coverage.

## M40 production uPD780C disassembly

Production compiles and uses the vaeg-owned `cpu/upd780/upd780_disasm.cpp`. The
subsystem exposes the capacity-aware
`subsystem_disassemble_bounded()` seam and retains the historical
`subsystem_disassemble()` signature as a 64-byte compatibility adapter.
Neither interface exposes STL or third-party types.

The ROM-less disassembler test is part of ordinary CTest. Run it directly
with:

```sh
cmake --build build/linux-ci-gcc --target vaeg_upd780_disasm
ctest --test-dir build/linux-ci-gcc --output-on-failure \
  -R '^vaeg_upd780_disasm$'
```

The target checks all base, CB, ED, DD, FD, DDCB, and FDCB opcode pages,
reviewed canonical output, address wrapping, deterministic memory reads, and
bounded buffers. See [Z80 disassembly](upd780-disassembly.md) for the API,
license, formatting, reserved-encoding, and M41 dependency policy.

Private ROM/disk testing is not public CI. Use the committed
[M39 private integration manifest](z80-compatibility-private-integration.md), record asset
hashes and traces outside Git, and require explicit maintainer approval at
G39. A boot alone is not sufficient.

ROMs are deliberately absent from CI and release artifacts. Users place the
VA unsuffixed ROM set or the MAME-compatible VA2/VA3 `*_va2.rom` set beside
the executable, together with the extra `vasubsys.rom`, using ROMs extracted
from hardware they own. Runtime size/CRC32/SHA-1 checks warn on differences
from MAME. No ROM placeholder is packaged.

## VA Configuration Prerequisites

Before running the VA checklist, ensure the portable `vaeg.cfg` selects
the VA machine and matching sound hardware:

```ini
pc_model=88VA1
SNDboard=100
clk_base=3993600
clk_mult=2
```

`SNDboard=100` is the VA built-in YM2203/OPN. A VA with Sound Board II
uses `SNDboard=200`; `pc_model=88VA2` also uses `SNDboard=200` for its
YM2608/OPNA. Missing or stale values can produce a V2 halt, an FM-timer
wait with no sound hardware bound, or the wrong clock domain. See
`../../sdl2/README.md` for the full state directory and warning policy.

## G11 Verification Notes

G11 passed on the maintainer-verified native Windows and macOS builds.
The macOS build used the SDL renderer backed by Metal. Retina output was
not blurry at integer display scales, and writable state files were
created under `~/Library/Application Support/vaeg` as required by
ADR-0005.

## uPD9002 M42 evidence targets

Test-bearing builds expose behavior-neutral uPD9002 inventory and regression
targets. M42 proved that every supported product preset compiled the C core
and reached its V30 step function; the frozen assembly reference was not a
supported preset. Run the complete M42 ROM-less evidence set with:

```sh
cmake --preset linux-ci-gcc
cmake --build --preset linux-ci-gcc
ctest --test-dir build/linux-ci-gcc --output-on-failure \
  -L upd9002
```

The labeled tests regenerate the fail-closed dispatch graph, exercise parser
selftests, verify the recorded raw-state ABI, run the graph-derived direct
instruction manifest, compare deterministic `--trace-cpu 8` output with its
golden, and verify the reset, executed, and CPU_SHUT fixtures. The trace is
disabled by default. Its schema uses ordered CPU, DMA, and device records and
records CPU memory writes and eight-bit I/O transactions when active.

`tests/upd9002/direct_harness.h` is test-only. It accepts fixed-width CPU,
program, and RAM input structures and returns a fixed-width result without
exposing emulator globals. The SingleStepTests V20 dataset is deliberately not
downloaded or integrated in M42.

## uPD9002 M43 external V20 baseline

M43 extends the direct harness with a test-only child-process adapter for the
pinned MIT-licensed SingleStepTests V20 `v1_native` corpus. The corpus remains
outside the repository. Acquisition, digest verification, deterministic CI and
full commands, and baseline-report commands are documented in
`tests/ssts/README.md`.

Set `VAEG_SSTS_V20_ROOT` to the verified pinned checkout when configuring a
test-bearing build. CTest then reproduces the CI profile and requires exact
agreement with the committed result and failure signatures. When the setting
is absent, ordinary hosted CI records a visible external-data skip while still
running the synthetic fail-closed adapter test and checking both committed
result summaries. An external-data skip never satisfies the M43 human gate or
the corresponding comparison gate in M44--M49.

The worker entry point exists only when `VAEG_ENABLE_TESTS=ON`; production
builds do not compile the adapter, deterministic flat-memory seam, or test I/O
seam. V20 bus timing and prefetch-cycle details are diagnostic rather than
uPD9002 timing requirements.

## uPD9002 M58 SST ratchet

M58 preserves the M43 result files byte-for-byte and records their immutable
content identities in `tests/ssts/epochs/g43/manifest.json`. The blocking
architectural CI and full profiles use the metadata-defined FLAGS mask. The
separate, non-blocking fingerprint full profile compares all 16 FLAGS bits.
Neither contract includes cycles, prefetch, or bus timing.

With `VAEG_SSTS_V20_ROOT` configured, CTest runs the architectural CI profile
through `tools/qa/upd9002_ssts_ratchet.py ci-enforce`. The command requires
the configured Git commit and compares it with the explicit approved G57 SHA,
not with a mutable worktree result. When the corpus is unavailable, CTest
reports the existing visible skip; that skip never satisfies G58.

The always-available repository tests verify immutable M43 files, schema
versions, deterministic JSON and gzip, closed classification registries,
strict lettered milestone identifiers, and fail-closed negative cases:

```sh
python3 tools/qa/upd9002_ssts_ratchet.py verify-static --root .
python3 tools/qa/upd9002_ssts_ratchet.py selftest
python3 tools/qa/milestone_ids.py --root . --selftest --discover --audit
```

## uPD9002 M45 native execution invariant

M45 makes the proved build/runtime choice unconditional. All 13 supported
CMake presets compile the active `cpu/upd9002/` core; the scheduler calls
`upd9002_core_step()` directly. There is no active CMake or header selector for
an assembly core or per-instruction 80286 execution. Historical assembly-core
configurations are frozen unsupported references and must not be offered as
product build options.

Normal initialization and reset always establish V30-compatible register
state. Current save states use the `UPD9CPU` CPU section and reject obsolete
CPU286 compatibility input before mutating machine state. CPU_SHUT remains the
sole documented initializer-level exception: it preserves the M42
register-init fixture, including its upper-FLAGS anomaly, without selecting a
286 opcode dispatcher.

Run the fail-closed source/build/reference map with:

```sh
python3 tools/qa/upd9002_native_invariant.py --root .
```

The check also requires the historical block executor removal to remain
complete after M46.

## Known Issues

US physical host keyboards do not yet have a dedicated mapping mode for
the JIS PC-88VA guest layout. On all host platforms with a US keyboard,
the known symptoms are swapped `@` / `"` positions, no direct `:` key, and
unreachable `-`, `^`, and yen-key positions. The numeric keypad path is
not affected. A keyboard-layout mapping mode is tracked as a later GUI
parity item.

State-save (`statsave`) files are not portable across architectures or
builds. This was established during the M11 LLP64 audit: runtime pointer
and handle-size fixes are safe for process memory, but serialized emulator
state should be treated as tied to the exact architecture/build family
that wrote it.

VA1 mode has an inherited V3 BASIC execution defect: commands including
`BEEP`, `LIST`, and `FILES` can leave the guest running in a repeated
text-display path and make it appear frozen. VA2/VA3 mode is not known to be
affected. The problem also reproduces in the original VAEG and predates the
SDL2 display-effects work. Disabling host sound, suppressing BEEP PCM
registration, and suppressing the BEEP event did not prevent it, so this is
not currently classified as an audio-backend defect.
