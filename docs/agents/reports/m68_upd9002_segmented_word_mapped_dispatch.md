# M68 uPD9002 segmented word mapped-memory dispatch report

## Status

M68 was reassigned by maintainer instruction to restore canonical
mapped-memory dispatch for uPD9002 segmented word access. The former
unapproved M68 scope was revoked and deferred for later reassignment.

G68 is not declared passed by this report.

## Fixed identities

- Branch: `topic/m68-upd9002-segmented-word-mapped-dispatch`
- Approved G67 semantic predecessor:
  `f8f350e1aadec4b6c79c20192d14c50bd39934be`
- Main integration base:
  `5e044f802c6cd3a1bb55f694897b0fe5561d146b`
- Task-authority SHA:
  `0b632c5d7feb2de65fee2ad516f3cb8ced7cd11c`
- Regression-test SHA:
  `3408f9b63933f6f6d1c2a8aa7806c92ec9f3738e`
- Production-fix SHA:
  `90258f26207b7ce7dc3473a5df2811da4bb0c19c`
- Evaluated behavior SHA:
  `90258f26207b7ce7dc3473a5df2811da4bb0c19c`
- Final candidate SHA: supplied by final handoff
- Target policy:
  `upd9002-g64-37ae2b706a9cbbe2d36cf7c98372c0cae7ca4b8d90e4f738973bc0ed3248eed6`

## M68 authority disposition

Before implementation, the repository was searched for formal G68 approval,
current M68 task authority, M68 reports, M68 branches, ROADMAP references,
migration references, and existing G68 references. No approved G68 gate was
found. No former unapproved M68 task file, report, local branch or remote
branch was found.

The new canonical task is:

- `docs/agents/tasks/M68_upd9002_segmented_word_mapped_dispatch.md`

The authority commit modified only milestone/task documentation:

- `docs/agents/ROADMAP.md`
- `docs/agents/UPD9002_SEMANTICS_MIGRATION.md`
- `docs/agents/tasks/M68_upd9002_segmented_word_mapped_dispatch.md`

## Original regression boundary

The PC-Engine/MS-DOS text-scroll regression was manually bisected to:

- M65d checkpoint `ef44acbf5183ac5a8233ac007b07de72fd61eae8`: OK
- M65e checkpoint `8350ca5d8345f3414e1864dcb6d70e391ea60cc1`: NG

The A5 semantic-isolation experiment established:

- M65e without only the A5/MOVSW change: PC-Engine/MS-DOS scroll OK
- M65d with only the A5/MOVSW change: PC-Engine/MS-DOS scroll NG

Therefore the M65e A5 segmented-word access change was necessary and
sufficient for the observed runtime regression.

## Architectural diagnosis

The defect was not MOVSW iteration semantics.

The defect was an independent host-memory fast path inside the segmented word
helper. That path bypassed the canonical VA mapped-memory dispatcher and read
flat `mem[]` instead of the active TVRAM backing store.

The faulty route was:

```text
instruction
  -> segmented word helper
     -> segment wrapping calculation
     -> independent flat mem[] fast path
     -> bypass of canonical mapped-memory dispatch
```

The corrected route is:

```text
instruction
  -> segmented word helper
     -> segment base plus 16-bit offset calculation
     -> FFFFh-to-0000h segment-wrap resolution only
     -> canonical generic memory API
        -> RAM, TVRAM, BMS, callbacks, side effects, dirty tracking and fast paths
```

The segmented word helper now owns only segment-offset address calculation and
FFFFh-to-0000h wrapping. The canonical generic memory API exclusively owns
RAM, TVRAM, BMS and device mapping, callbacks, side effects, dirty tracking and
fast-path selection.

## Exact wrong access route

The established trace showed:

- Canonical VA mapping classified physical `A0000h` as TVRAM.
- The segmented read helper used `I286_MEMREADMAX == 0xA4000` to select flat
  host memory for `A0000h-A3FFEh`.
- It read `mem[]`, not `textmem[]`.
- Real mapped TVRAM bytes: `5a a5 3c c3`
- Distinct flat shadow bytes: `10 20 30 40`
- M65e MOVSW result: `10 20 30 40`

CX, SI, DI, IP, DF, REP count and direction were not the defect.

## Helper change

Production file:

- `cpu/upd9002/memory.c`

The segmented word helpers now compute:

```c
address = segment_base + LOW16(off);
high_address = segment_base + LOW16(off + 1);
```

For physically contiguous bytes, they delegate to:

```c
upd9002_memoryread_w(address)
upd9002_memorywrite_w(address, value)
```

Only the noncontiguous segment-wrap case splits into two canonical byte
accesses. Direct `mem[]` accesses, fast-path ceilings and mapping-policy
decisions were removed from the segmented word helpers.

## Helper-consumer inventory

The shared helper consumers audited for M68 were:

| Source | Function/form | Access | Mapped-memory relevance | Coverage |
| --- | --- | --- | --- | --- |
| `cpu/upd9002/upd9002_mn.c` | A5 MOVSW non-REP | read/write | high | M68 MOVSW TVRAM/BMS probes, A5 SST |
| `cpu/upd9002/upd9002_rp.c` | A5 REP MOVSW | read/write | high | REP count 1/>1, DF=0/1 probes |
| `cpu/upd9002/upd9002_mn.c` | 61 POPA | read | shared helper | M65e tail, SST |
| `cpu/upd9002/upd9002_mn.c` | 81 word RMW including `/6` | read/write | shared helper | M65e tail, SST |
| `cpu/upd9002/upd9002_mn.c` | 83 word-immediate RMW | read/write | shared helper | SST full |
| `cpu/upd9002/upd9002_fe.c` | FF `/3` far CALL | read | shared helper | SST full |
| `cpu/upd9002/upd9002_fe.c` | FF `/5` far JMP | read | shared helper | M65e tail, SST |
| `cpu/upd9002/upd9002_dispatch.c` | PUSHF helper | write | shared helper | M65e tail |
| `cpu/upd9002/upd9002_mn.c` | 9C PUSHF | write | shared helper | M65e tail |
| `cpu/upd9002/upd9002_mn.c` | D1 `/6` | read/write | shared helper | M65e tail, SST |
| `cpu/upd9002/upd9002_mn.c` | C8 ENTER | read/write | shared helper | M65e tail, SST |
| `cpu/upd9002/upd9002_mn.c` | C4 LES | read | shared helper | M65e tail, SST |
| `cpu/upd9002/upd9002_mn.c` | C5 LDS | read | shared helper | SST full |
| `cpu/upd9002/memory.c` | `meml_read16()` | read | direct helper wrapper | M68 segmented read probes |
| `cpu/upd9002/memory.c` | `meml_write16()` | write | direct helper wrapper | M68 segmented write probes |

No active caller was found to rely on the erroneous flat-memory bypass.

## Mapped-memory regression tests

The focused test entry point is:

```text
build/m68-macos-tests/sdl2/vaeg --upd9002-m68-segmented-memory
```

Predecessor result at regression-test SHA before the production fix:

```text
exit status: 1
9 / 13 checks failed
```

Observed predecessor failures included TVRAM and BMS reads returning flat
shadow values, mapped writes not updating mapped backing stores, REP and
non-REP MOVSW copying flat shadow values, and DF=1 MOVSW selecting the wrong
source data.

Fixed result:

```text
exit status: 0
upd9002-m68-segmented-memory: mapped dispatch checks passed
```

Coverage:

- normal RAM
- VA TVRAM
- BMS mapped memory below `A0000h`
- segmented word read
- segmented word write
- non-REP MOVSW
- REP MOVSW count 1 and count greater than 1
- normal RAM to TVRAM
- TVRAM to normal RAM
- TVRAM to TVRAM
- mapped below `A0000h` to normal RAM
- normal RAM to mapped below `A0000h`
- ordinary aligned and unaligned offsets
- `FFFEh`
- `FFFFh -> 0000h` segment wrap
- DF=0 and DF=1

## A5 population

- Executable A5 population count: `2502`
- Executable A5 population digest:
  `41fdaa8706a890b86c04b609466910f332494a62fa17e2f2b2971fd71d0788b0`
- Owned A5 wrap case:
  `cbad10077f6e4b2dd631f45baffb3a862400450f561bedd74c9bd5be7d52b9da`
- Architectural full A5 form counts:
  `2502 selected / 1870 applicable / 1870 pass / 632 known_target_gap`

The M65e segment-wrap correction was preserved. The predecessor A5 failure
signature did not return.

## Protected populations

| Population | Result |
| --- | --- |
| M65a FF `/7` | `5000 / 5000` |
| M65b BOUND | `1244 / 1244` |
| BOUND frame-only | `3565 / 3565` |
| M65c F7 `/2` | `5000 / 5000` |
| M65d FF `/6` | `5000 / 5000` |
| M65e tail | `10 / 10` |

Focused CTest preservation command:

```text
ctest --test-dir build/m68-macos-tests -R 'vaeg_upd9002_m(65a|65b|65c|65d|65e|68)' --output-on-failure
exit status: 0
6 / 6 tests passed
```

## Full SST profiles

Dataset:

```text
ssts-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21-1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4
```

Architectural CI:

```text
selected:              180000
applicable/executed:   169300
pass:                  169300
fail/timeout/crash:    0 / 0 / 0
selected digest:       d30dd9c864fbbaa74c661e1b829c66264f2184a8fbbb72b654b2baa825664ae6
applicable/pass digest:6f10f47cd0f939145f99dbe6b1d820c79082c90083963b61cd39b5f56503537f
failure/signature:     4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945
```

Architectural full:

```text
selected:              1562502
applicable/executed:   1474594
pass:                  1474594
fail/timeout/crash:    0 / 0 / 0
selected digest:       0aa3dbb24323223b3a9595a0bd7cfd5666596741157c14b60f6969318475f8f7
applicable/pass digest:4f0f19a6496f4c4463da092c7d5df7a9a0365a951821d9428eac8662d0c76e7c
failure/signature:     4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945
```

Fingerprint full:

```text
selected:              1562502
applicable/executed:   1474594
pass:                  1402202
fail:                  72392
timeout/crash:         0 / 0
pass digest:           ea521512c9f49b3a73558db6ccf0a01c6b889d1df8a82fb897a9d9d1af8316f4
failure digest:        0692676136061b956d0b7f1c06a35cfc4c5ffff7b925ba83f2d07d37310f22c5
signature digest:      79913b4f99c54d263315235829f6f937c5956268d9239a4b371301e8acbcdee8
```

Transition deltas from G67:

```text
newly passing:     0
newly applicable:  0
newly failing:     0
changed failures:  0
```

## Manual runtime acceptance

Manual gate input was supplied by the maintainer on 2026-07-28:

```text
gate passed
```

Manual test executable:

```text
path:   build/mingw-cross/sdl2/vaeg.exe
sha256: 5da1d9eaa6d110b2a46a4914b6187ad2c3ef6815974f9aeb524e421a92244103
config: mingw-cross
```

Recorded acceptance coverage:

- PC-Engine/MS-DOS cold boot
- `DIR A:`
- `CHKDSK A:`
- multi-screen `TYPE` or equivalent output
- `CLS` after scrolling
- demo/game
- save state
- load state
- Sound Board II

Required result: after output reaches the bottom row, existing lines move
upward normally and multiple rows remain readable.

## Artifact family

The deterministic artifact family is stored under:

```text
tests/ssts/campaigns/g68/
```

Generation was run twice into independent temporary directories and compared
with `diff -r`; the result was byte-identical.

Key artifact digests:

- Artifact tree digest:
  `a3c4f91947d29b8f2360bb94d223bec34e798406d87cc14eb25d6de4b8f350f5`
- Mapped-memory matrix:
  `946bfc60ae9b3e452026a71aedeb5dc38a0a3a1b5231c7af099bebbada8940c4`
- Consumer inventory:
  `828bb5be19a331a6ffbdd3c9587d4ce7bac1a788c746eab181d0deb94fb147d5`
- Closure audit:
  `1359fa412a3d299eea5d21fc1c99c0e60301f920659692f56b0237c66be416c7`

## Repository and build checks

Completed:

| Command | Exit |
| --- | --- |
| `git diff --check` | 0 |
| `build/m68-macos-tests/sdl2/vaeg --upd9002-m68-segmented-memory` | 0 |
| `ctest --test-dir build/m68-macos-tests -R 'vaeg_upd9002_m(65a\|65b\|65c\|65d\|65e\|68)' --output-on-failure` | 0 |
| `ctest --test-dir build/m68-macos-tests-py314 -LE external --output-on-failure` | 0 |
| `tools/repo/check_encoding.py --report` | 0 |
| `tools/repo/check_eol.py` | 0 |
| `tools/repo/check_case.py` | 0 |
| `tools/qa/milestone_ids.py --root . --selftest --discover --audit` | 0 |
| `tools/qa/upd9002_m66_state.py --root . verify-m66a` | 0 |
| `tools/qa/upd9002_m66_identity.py verify` | 0 |
| `tools/qa/upd9002_m67_divergence.py selftest` | 0 |
| `cmake --build build/mingw-cross --target vaeg_sdl2 -j 4` | 0 |
| `cmake --build build/macos-asan --target vaeg_sdl2 -j 4` | 0 |
| `build/macos-asan/sdl2/vaeg --upd9002-m68-segmented-memory` | 0 |

Deviations:

- `tools/qa/upd9002_m67_divergence.py verify` returned exit status 1 with a
  source digest mismatch for `docs/agents/ROADMAP.md`. The protected G67
  artifact directories were unchanged; the mismatch is the authorized M68
  ROADMAP/task-authority update.
- The MacPorts GCC 15 build returned exit status 2 in the C++ standard
  library declarations for `at_quick_exit` and `quick_exit`, before M68 code
  could be evaluated.
- Wine was not available in the local environment.

## TSP exclusion

M68 did not modify:

- `iova/tsp.c`
- `0142H` IDP status-port behavior
- BUSY/VB expression
- IBF/OBF behavior
- TSP timing

The TSP status-port defect remains explicitly excluded for a later separate
milestone.

## Known limitations

- The existing flat-memory SST observation boundary cannot distinguish flat
  `mem[]` from mapped TVRAM/BMS backing stores for this runtime defect. M68
  therefore expects full SST identity and adds focused mapped-memory runtime
  probes for the corrected behavior.
- The report cannot embed its own final evidence commit SHA before that commit
  exists; the final candidate SHA is supplied by the final handoff.

## Recommended predecessor wording

Subsequent uPD9002 semantic or mapped-memory work should use the final M68
candidate SHA supplied by the handoff as the predecessor, not the M65e
diagnostic branch or the pre-fix M68 production SHA.
