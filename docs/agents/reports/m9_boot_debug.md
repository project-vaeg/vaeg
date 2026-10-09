# M9 VA boot differential debug

Status: closed. The boot blocker documented here was resolved on
`topic/m9-va-portable`; remaining timing notes are carried as known
frontend/platform issues, not M9 boot blockers.

This report records the comparison setup for the PC-88VA FDC boot path.
It is instrumentation only; no boot behavior is intentionally changed.

## Config parity

Canonical comparison file:
`docs/agents/reports/m9_canonical_np2.cfg`

Use this file for both the portable build and the v141 legacy reference
when capturing boot traces.

| Key | Canonical value | Boot-path effect |
| --- | --- | --- |
| `pc_model` | `88VA2` | Selects VA model. This drives `pccore.model_va`, FDC default mode, VA memory, and VA I/O binding. |
| `clk_base` | `3993600` | Base CPU clock. Affects event timing including FDC and subsystem timing. |
| `clk_mult` | `2` | CPU clock multiplier. Keep identical for timing-sensitive FDC traces. |
| `DIPswtch` | `3e 73 7b` | Base system DIP bytes copied into the emulated machine state. |
| `MEMswtch` | `48 05 04 00 01 00 00 6e` | ITF memory switch bytes used by the guest boot code. |
| `ExMemory` | `1` | Extended memory size exposed to the guest. |
| `ITF_WORK` | `true` | VA ITF work-area behavior. Mismatch can alter the early boot path. |
| `biospath` | `.` | ROM lookup directory. For capture, ROMs live next to the executable. |
| `HDD1FILE`, `HDD2FILE`, `SCSIHDD0`..`SCSIHDD3` | empty | Disables HDD/SCSI boot paths so FDD is the only boot media. |
| `SampleHz`, `Latencys` | `22050`, `500` | Sound timing. Not a disk-data key, but kept fixed to avoid event cadence differences. |
| `SNDboard` | `200` | Enables VA Sound Board II binding. A mismatch leaves ports `0044`/`0045` unbound and contaminates traces. The portable parser is now 16-bit under `SUPPORT_PC88VA` at `sdl2/ini.c:369`. |
| `USE144FD` | `false` | Keeps the FDD geometry path on the legacy 2HD/2DD route. |
| `FDDRIVE1`..`FDDRIVE4` | `true`, `true`, `false`, `false` | Legacy FDD equipment bits. Portable builds currently use the same effective default, two drives. |
| `Sub_Mock` | `false` | Legacy-only switch between mock and real VA subsystem. Capture uses the real subsystem path. |
| `DipSw_VA` | `cd` | Legacy VA-specific DIP byte. Portable default matches this for the comparison. |
| `Use_VA91` | `false` | Disables VA91 extension path. |
| `Use_BMS_`, `BMS_Port`, `BMS_Size` | `false`, `00ec`, `16` | Disables BMS extension memory for the comparison. |

### Closed config item: VA sound board binding

A fresh `np2.cfg` leaves `SNDboard` at the non-VA default. In that
configuration neither the portable build nor the legacy reference binds the
VA Sound Board II, so `boardsb2_bind()` does not run, ports `0044`/`0045`
remain unhandled, and software that waits on the FM timer can hang. This is
configuration parity, not an emulation difference. The canonical comparison
config keeps `SNDboard=200`. Choosing a VA-specific default for fresh
configs is a later ADR candidate and is not changed on the M9 branch.

## Trace hooks

FDC operation lines include the current FDD interface mode in the common
format at `io/fdc.c:69` and `io/fdc.c:316`.

FDD interface mode writes are logged at the VA control register handler
`io/fdc.c:2470`. Reset-time default mode is logged at `io/fdc.c:2601`.
The register is `01b0h`; value bit 0 selects direct DMA mode when set
and Intelligent mode when clear.

If the boot path is Intelligent mode, the diff target is the subsystem
interface rather than the raw 765 data port. The real subsystem byte
stream is logged as `subiftrace` at `iova/subsystemif.c:34`; the mock
subsystem path logs `fdsubtrace` at `iova/fdsubsys.c:126`.

The legacy build can capture the same trace from process start by setting
`VAEG_TRACEOUT`. The environment variable is defined at
`win9x/trace.cpp:91` and opened during trace initialization at
`win9x/trace.cpp:476`.

## Portable capture

Command used:

```sh
mkdir -p /tmp/vaeg-m9-cfg/vaeg
cp docs/agents/reports/m9_canonical_np2.cfg /tmp/vaeg-m9-cfg/vaeg/np2.cfg
cd /home/maho/vaeg/build/linux-release/sdl2
timeout 30s env XDG_CONFIG_HOME=/tmp/vaeg-m9-cfg \
  SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./vaeg --fdctrace "MSDOS2.11.d88" \
  > /home/maho/vaeg/docs/agents/reports/m9_portable_msdos211.log 2>&1
```

`timeout` exits with status 124 because this is a normal boot run, not
`--smoke`. The captured log has 8164 lines.

## Portable observations

The previous `ports 0044/0045 unhandled` comparison contaminant is gone:
the portable log has no `iova-unhandled port=004*` entries.

The portable boot starts in Intelligent mode:

```text
fddiftrace reset mode=00/intelligent
```

The first successful boot-sector read in Intelligent mode is:

```text
fdctrace cmd=46/ReadData drive=0 C=00 H=00 R=01 N=03 mode=00/intelligent req=8192 st=00/00/00 xfer=1024 dma_ch=ff dma_access=00 dma_sysm_bank=00 sysm_bank=00 dma_len=0 dma_range=fffff-fffff
```

The actual data then flows through `subiftrace` response chunks, including
the boot sector contents beginning with:

```text
subiftrace mode=00 main2sub len=1 bytes=03
subiftrace mode=00 sub2main len=256 bytes=eb 1c 90 38 38 56 41 ...
```

Therefore `dma_ch=ff` on Intelligent-mode `fdctrace` lines does not prove
the DMAC is unprogrammed; the transfer path is the VA subsystem FIFO at
that point.

The guest later switches to direct mode:

```text
fddiftrace port=1b0 val=01 mode=01/direct
```

After that switch, DMAC programming is visible and direct-mode reads show
channel 2:

```text
dmatrace port=161 val=02
fdctrace cmd=46/ReadData drive=0 C=00 H=00 R=02 N=03 mode=01/direct req=7168 st=00/00/00 xfer=35 dma_ch=02 dma_access=00 dma_sysm_bank=00 sysm_bank=01 dma_len=1023 dma_range=32000-323fe
```

The `xfer=35` direct-mode count is not yet classified as truncation; it
must be compared against the legacy log captured with the same canonical
configuration and instrumentation.

## First divergence

Legacy capture:
`docs/agents/reports/m9_legacy_msdos211.log`

The first mechanical mismatch in the shared trace stream is an early
Intelligent-mode seek result:

```text
legacy:   docs/agents/reports/m9_legacy_msdos211.log:153
          fdctrace cmd=08/SenseInterruptStatus ... st=20/14/ff
portable: docs/agents/reports/m9_portable_msdos211.log:48
          fdctrace cmd=08/SenseInterruptStatus ... st=20/0a/ff
```

That mismatch is not yet treated as the boot blocker because both runs
recover and later return the same boot sector through the subsystem FIFO:
legacy line 1825 and portable line 169 both begin with
`eb 1c 90 38 38 56 41`.

The first boot-critical divergence is after the guest writes `01b0h <- 01`
and switches from Intelligent mode to direct DMA mode:

```text
legacy:   docs/agents/reports/m9_legacy_msdos211.log:1955
portable: docs/agents/reports/m9_portable_msdos211.log:251
```

The next direct-mode `ReadData` operation has matching command, status,
DMA channel, `sysm_bank`, and target range, but the portable build only
services 35 bytes before the FDC operation completes:

```text
legacy:   docs/agents/reports/m9_legacy_msdos211.log:2335
          fdctrace cmd=46/ReadData drive=0 C=00 H=00 R=02 N=03 \
          mode=01/direct req=7168 st=00/00/00 xfer=1024 dma_ch=02 \
          sysm_bank=01 dma_len=1023 dma_range=32000-323fe

portable: docs/agents/reports/m9_portable_msdos211.log:379
          fdctrace cmd=46/ReadData drive=0 C=00 H=00 R=02 N=03 \
          mode=01/direct req=7168 st=00/00/00 xfer=35 dma_ch=02 \
          sysm_bank=01 dma_len=1023 dma_range=32000-323fe
```

Causal hypothesis: the direct-mode FDC command is correct, and the DMAC is
programmed to the same channel/range in both builds. The failure is that
the portable CPU/DMAC loop does not consume a full 1024-byte sector before
the FDC completes the command. This points at portable DMA service cadence
or V30 DMA dispatch rather than disk geometry, side/track addressing, or
`sysm_bank` selection. One concrete audit point is that the legacy V30
loop calls `dmap_i286` (`i286x/v30patch.cpp:2418`), while the portable
V30 loop currently calls `dmap_v30` (`i286c/v30patch.c:1420`). Do not fix
this in the trace commit; verify it in the next implementation step.

## Legacy capture recipe

Build `win9x/np2_v141.sln` as usual, copy the canonical config next to
the legacy executable, and run one process with the trace file environment
variable set before launch. The v141 argument parser treats non-INI
positional arguments as FDD images (`win9x/np2arg.cpp:52`), and the disk
is queued for insertion at startup (`win9x/np2.cpp:2278`).

PowerShell:

```powershell
cd C:\Users\maho\vaeg\bin
copy C:\Users\maho\vaeg\docs\agents\reports\m9_canonical_np2.cfg .\np2.cfg
$env:VAEG_TRACEOUT = "$PWD\m9_legacy_msdos211.log"
.\vaegd.exe "MSDOS2.11.d88"
```

Let the run reach the same failure/success point, then close the emulator.
Send the complete `m9_legacy_msdos211.log`; do not use a windowed
DebugView copy, because it drops the power-on prefix and is hard to copy
when the trace is busy.

## Closed pacing note

The maintainer measured the portable SDL2 build against the v141 reference
over a 60 second span and found the average tempo within a sub-1% delta.
The remaining symptom is short-term jitter: notes can stretch and then
compress. This matches the observed WSLg pacing profile where host present
stalls feed the catch-up path; one captured `--pacelog` second showed only
37 executed frames with `max_pending=19`. This is registered as a known
WSLg/frontend pacing issue. Normative measurement and any platform pacing
change are deferred to G11 native Windows verification.
