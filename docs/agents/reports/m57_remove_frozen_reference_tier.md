<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# M57 frozen Win9x/i286x reference-tier removal

Status: the frozen reference tier is removed, its legal and lineage evidence
is preserved, and the required machine-verifiable no-behavior-change evidence
is complete. This report does not declare G57 passed. The standard V3/demo/OS
human gate and explicit maintainer review remain external.

The report cannot contain the SHA of the commit that contains itself. The
final report/remote SHA is therefore supplied in the maintainer handoff.

## Identity and approved predecessor

- Initial repository branch: `main`
- Initial repository HEAD:
  `39b982801ac85a6e01219d4404e79b9f06534b0f`
- Initial remote:
  `origin git@github.com:nakatamaho/vaeg.git` for fetch and push
- M57 branch: `topic/m57-remove-frozen-tier`
- Exact approved G56 predecessor and M57 base:
  `b72e641733ddea6f0e8faef2507093f7c3aee5a4`
- Pre-report implementation SHA:
  `6da8e1525e8fe5e69813b4a10fac9b69eaa2f495`
- Final report/remote SHA: supplied in the handoff after this report commit

The initially inspected primary worktree contained a tracked `README.md`
change and untracked maintainer handoff documents. Its exact
`git status --short` output began:

```text
 M README.md
?? ALL_TASKS_CONCATENATED.md
?? MANIFEST.sha256.json
?? MILESTONE_RENUMBERING.md
?? docs/agents/UPD9002_SEMANTICS_MIGRATION.md
?? docs/agents/reports/TEMPLATE_upd9002_semantics_gate_report.md
?? docs/agents/tasks/M57_remove_frozen_reference_tier.md
?? docs/agents/tasks/M58_upd9002_ssts_ratchet.md
...
?? docs/agents/tasks/M67_upd9002_divergence_consolidation.md
```

Those user files were not reset, cleaned, overwritten, staged, or committed
from the primary worktree. A separate linked worktree was created at the exact
approved G56 SHA, verified clean, and branched as
`topic/m57-remove-frozen-tier`. All M57 edits and validation occurred there.

The approved predecessor was not inferred from the initial HEAD, a branch tip,
G51, the latest remote commit, or the milestone number. It was resolved as
follows:

1. The accepted
   `docs/agents/reports/m56_hostfs_readonly_redirector.md` contains exactly one
   field labelled `Approved G56 SHA`, whose value is
   `b72e641733ddea6f0e8faef2507093f7c3aee5a4`.
2. The current ROADMAP and accepted M56 task independently record the same
   SHA and describe the same administrative closure.
3. Initial `main` commit
   `39b982801ac85a6e01219d4404e79b9f06534b0f` is a documentation-only
   administrative-closure child of `b72e641733ddea6f0e8faef2507093f7c3aee5a4`;
   it was not used as the M57 base.
4. The remote archive tag peels to the approved SHA, proving the remote
   contains the exact predecessor.

The G56 closure establishes only the predecessor gate. No M57 evidence treats
it as proof that uPD9002 semantics-migration work was completed.

M57/G57 was absent from the pre-M57 ROADMAP sequence and therefore did not
conflict with the unrelated historical M52--M56 records. The accepted M56
task, research, and report blobs brought from administrative commit
`39b982801ac85a6e01219d4404e79b9f06534b0f` are byte-identical to that commit;
M57 did not rewrite or reinterpret them.

## Immutable archive tag

`archive/frozen-win9x-i286x-g56` already existed as an annotated tag, so M57
did not create or move it.

| Location | Tag object | Peeled commit |
|---|---|---|
| local | `6236e7487c733d9fdf0840c7b878473a3932ee97` | `b72e641733ddea6f0e8faef2507093f7c3aee5a4` |
| `origin` | `6236e7487c733d9fdf0840c7b878473a3932ee97` | `b72e641733ddea6f0e8faef2507093f7c3aee5a4` |

The local object type is `tag`. Both object identities and both peeled
commits are exact.

## Commit separation

The implementation has the required concern separation:

1. `4d9e226b385aa85ab5fa4531120a32f2b38ccce0` —
   `M57: preserve legacy source provenance`;
2. `31f5fc59ac46c7b7c9ca6495c875e923a9e8785b` —
   `M57: remove frozen reference tier`;
3. `6da8e1525e8fe5e69813b4a10fac9b69eaa2f495` —
   `M57: repair live references after tier removal`.

Commit 1 changes no production source. Commit 2 contains only deletions below
the four approved targets. Commit 3 contains current documentation,
repository-check, workflow, and provenance-reference repairs; it changes no
active emulator production source.

## Legal and historical evidence

At approved G56, `win9x/readme.txt` is UTF-8 without BOM, uses LF, and is
compatible with the repository encoding policy. It is 19,255 bytes and 409
lines. M57 preserves it byte-for-byte at:

- `LICENSES/legacy-vaeg.txt`;
- `docs/legal/legacy-source-provenance.md`, which identifies the archived
  source, lineage, licenses, immutable tag, and relationship of the copy.

| Evidence | SHA-256 |
|---|---|
| `win9x/readme.txt` at approved G56 | `9c15b317020a58cf58341b00959ffa9e9dd220b0d574aa0c4900c477aeaa6cbc` |
| `LICENSES/legacy-vaeg.txt` | `9c15b317020a58cf58341b00959ffa9e9dd220b0d574aa0c4900c477aeaa6cbc` |

`git show G56:win9x/readme.txt | sha256sum` and
`sha256sum LICENSES/legacy-vaeg.txt` are equal. The preserved file is therefore
a byte-identical copy, not a transcription. A narrow `.gitattributes`
whitespace exemption preserves one historical trailing space without
rewriting legal evidence.

## Exact removal

The deletion commit removes exactly:

| Approved target | Files removed |
|---|---:|
| `win9x/` | 133 |
| `i286x/` | 21 |
| `hlp/` | 96 |
| `cpuxva/memoryva.x86` | 1 |
| **Total** | **251** |

Before committing, the staged name-status list was checked programmatically:
all 251 entries had status `D`; every path was either below `win9x/`,
`i286x/`, or `hlp/`, or was exactly `cpuxva/memoryva.x86`; and no other path
was present. The active `cpucva/memoryva.c` and every similarly named active
path remained outside the deletion.

After M57 all four approved targets are absent. `romimage/` and all other
binary/private payload locations were untouched.

## Live-reference repair boundary

The third commit repairs only references whose current-tree use would
otherwise break:

- active repository overview and build documentation;
- current AGENTS, CONVENTIONS, and ROADMAP text;
- the encoding and case checks;
- the rename ownership guard;
- CI's encoding invocation;
- current asset and modernization provenance links, now pinned to the
  archived G56 commit;
- the authoritative M57 task, renumbering note, and semantics-migration
  boundary document.

Historical task reports, archived plans, ADR decisions, M42--M51 evidence,
M43 baseline files, and historical provenance strings were not rewritten for
wording consistency. Remaining old-path strings are either immutable history,
archived-source citations, or checker rules that assert those paths stay
retired; none is a live build, package, or filesystem dependency.

## M43/G51 pre-deletion baseline

Before the first M57 commit, a tests-enabled GCC worker was built at the exact
approved G56 predecessor. Both required architectural profiles ran without
external skip. The committed M43 corpus, manifest, support map, summaries,
sidecars, and known-gap evidence have no diff between immutable G51 evidence
SHA `78c712f0bab53a6960cfc102eae7ee54b3fc29ef` and approved G56. The
`cpu/upd9002/` tree also has no diff over that interval.

The verified dataset is:

```text
ssts-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21-1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4
```

Additional immutable identity fields are:

| Field | Value |
|---|---|
| upstream commit | `9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21` |
| dataset digest | `1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4` |
| corpus SHA-256 | `12f1d146f7070ed9a83fa516cf9dc2a6771b572d93bd92ab26e251c3be8f0294` |
| metadata SHA-256 | `71c12e705960941a73981891852674649c3332539579634ea34d1dae40c1795a` |
| manifest file SHA-256 | `f9aa17e0f5a24102f437c6ab5a061a891339f49f1d5af88c73228d54205f2d0b` |
| corpus files verified | 360 |
| committed known-gap SHA-256 | `11ec1496a96d661c0656720b13939b1ade3448b693b923f8acf36576cd7048c1` |

The historical and reproduced totals are:

| Profile | Selected | Applicable/executed | Pass | Semantic failure | Skip |
|---|---:|---:|---:|---:|---:|
| deterministic CI | 180,000 | 166,821 | 156,228 | 10,593 | 13,179 |
| complete full | 1,562,502 | 1,443,876 | 1,359,547 | 84,329 | 118,626 |

Category and termination counts are:

| Profile | Applicable | Known target gap | Upstream nonblocking | Normal | Type 0 | Timeout | Crash |
|---|---:|---:|---:|---:|---:|---:|---:|
| CI | 166,821 | 8,179 | 5,000 | 165,546 | 1,275 | 0 | 0 |
| full | 1,443,876 | 68,626 | 50,000 | 1,431,180 | 12,696 | 0 | 0 |

No count was accepted on its own. The G56 pre-deletion output was compared
with immutable M43/G51 evidence before deletion. The candidate output was
then compared with that verified G56 output.

## Exact post-deletion SST preservation

Both required profiles ran again at
`6da8e1525e8fe5e69813b4a10fac9b69eaa2f495`, without skip and with the
same pinned worker contract. Every command exited zero.

### Summary, selection, classification, and signature digests

| Evidence | CI | Full |
|---|---|---|
| selection digest | `0be9aeb1bfad2db3e10e9abd4ba2fbf2921a7899824b04ac9a219607657db073` | `3d856d20aa5ef2170b193cb8b2710d3dec577948c89b50083de8dfee90357010` |
| selected upstream-hash set | `b1dc14fa7750cbc5b674c8ca87c909f22425f9693479e1a2bce16b88c3b07ec2` | `bc6e2bd30708f5c25869744f5e5b43f25c6967bbcc960a3f7fa82fdfe04f72c8` |
| generated classification JSON | `638b8fe30e2623320fa7c3f9a1802aa8c9ca15bfa76947192c133886e0c87f18` | `fac9ed96c268b0eb7aaf09c0421c2abe30cfaf290968ae03257271a906b5c9f9` |
| generated known-gap JSON | `bf1b77ab32d529d15ff6a791c5ec09553f3f2d5014e910330ae3fc32b2dccc56` | `eca3f63bbf98582a5a4c103053041cb78df6a0161815d8fca9a5afe0629e1cd5` |
| result summary JSON | `a5db6a6cc82ae794fd2f60306c3d4a70136d6030e17e1aec733523bece864e31` | `dd3247774afe5c5a19228d3a08f01ac6f614e6b67a3a6f454c1abc58f3dbf3d9` |
| failure-signature index | `946268103309f8dc7d442fade21596b46c734f48bf0b1e9e32a18736e5e85597` | `50087f8f6b9483ac70ce5e2dc922ab11fade51a58bea9cb72322ef85ef264ec0` |
| compressed-sidecar aggregate | `526594bf31acdbefd4761067af83fd03b66eff5490e9c1e3b1b554da5910db73` | `4042b830953197b5e4809306864d04606697034203d219489b3edf1cad34374e` |

The generated classification and known-gap JSON files compare byte-identical
between G56 and M57. This proves exact category counts, every per-form
classification, every resolved known-gap rule, and every rule's sorted hash
list and digest. The result summaries and complete compressed sidecar
directories also compare byte-identical.

### Resolved category and applicable result sets

The following audit digests are SHA-256 over each deterministic sorted member
set. Counts and digests are identical before and after deletion:

| Profile | Set | Count | SHA-256 |
|---|---|---:|---|
| CI | applicable upstream hashes | 166,821 | `6720a811d6ef9f11c942fc05bc0e8266bc7e168a8988055286a0a0ce5661cb2` |
| CI | known-target-gap upstream hashes | 8,179 | `5653100d66ff3d60d2ebca95288ca3629d77e99fbbbf875d3fba0d590e5310c3` |
| CI | upstream-nonblocking hashes | 5,000 | `773aff211ac7459cb1ec1f469336d6eb4ed74896992ef1d3cc4bc1e3f4ebb7fd` |
| CI | applicable pass upstream hashes | 156,228 | `4de8e7594098eb7e85e8f573781642eaac12aaec225aa4af2d016ced6e8f0892` |
| CI | applicable pass canonical records | 156,228 | `053195c1ac7001fa553da6201cd9e7a6843e8de2846796ca608cfca09ab30c78` |
| CI | applicable failure upstream hashes | 10,593 | `b5ed2f5c078d8e85606ed6d76b4bb3d12f56a33241f9881608e9c4714ce84d6c` |
| CI | applicable failure canonical records | 10,593 | `6129fb38d6ad32d739027819ec24e0ffe7caba26ef7e8e7e595f9cfd987cb63e` |
| full | applicable upstream hashes | 1,443,876 | `bdc7dc4baf92a9e77cbffa6d3400c46ea364fbb351d9f4693ec95365b39015bc` |
| full | known-target-gap upstream hashes | 68,626 | `7a5812a44cca9339e3fe20b4fdd296d0066188a777d195b176b4724e9c9681d4` |
| full | upstream-nonblocking hashes | 50,000 | `deff310fad72bf972292c580afcd3b6edd6063e6b60edc1e379a9f955c881dc2` |
| full | applicable pass upstream hashes | 1,359,547 | `3a492443eae5d49a9cf68635e87939fd36a93cb9858641daa820d9033875a04d` |
| full | applicable pass canonical records | 1,359,547 | `64e33aabb7ad4e926c329105a13bf917559e8e6ee37ebc784def8a52256fdee1` |
| full | applicable failure upstream hashes | 84,329 | `96cbc575d2b2169b333c7c0824a996f9e1668d93ee720cebeeeb5a66cda26c18` |
| full | applicable failure canonical records | 84,329 | `cd3bac7b62f0c661d1c660dfadd13cb9e8690dca6d66449c792b3bc1b06d57b4` |

Thus no selected record changed category, and no applicable record moved
between pass and failure.

### Failure-sidecar contents

Each canonical digest below is over the uncompressed canonical JSON payload.
Every compressed `.json.gz` file is also byte-identical between verified G56
and M57.

| CI group | Failures | Canonical SHA-256 |
|---|---:|---|
| `0` | 686 | `89a9bedf0b9658c6eadfe9856f944760613672a9d88ddd854166141c77bb0cc2` |
| `2` | 7 | `1bd0b1c6467bf5478fb2875fbde1b35dc83384c9a7214c75970e5c6763510784` |
| `3` | 477 | `bb1a4c1e601a175845f7e02bbd32d32147e537382332618f543fec3fcef829e3` |
| `6` | 620 | `470f286b893b9efada54e47f6377514fcff13852514ccb4b58bc9c79a35d86b0` |
| `9` | 740 | `87cd438a9af49de3aae7c4ce9e3dd4d2a70afe2ba428875b15873547436a40bf` |
| `a` | 1 | `fb3d717ce04ffce4cc883238bbd05d2aaa2d2cf65bff8664675b673087059f5e` |
| `c` | 3,820 | `3b55b32283f12cd8d8808c0a343e7beef959a147a95f183fc9ce93c22d1cdfab` |
| `d` | 2,340 | `19e0c97613fb6817b5bb3e0fd93fc55477b103ab1179b479b19cef4c61ca1544` |
| `f` | 1,902 | `d5a63828e5d86ff81f4859f6d9865aa41bc7a342c602e532aa32a8fa0597fd63` |

| Full group | Failures | Canonical SHA-256 |
|---|---:|---|
| `0` | 5,087 | `ba390af18f4c25dd2145c965d72c54623ebdb975581fa603d61558e614135a1f` |
| `2` | 98 | `c67165845cd63b056f4d1a2e497aca00e988629b5a769cbd24b1c1da07148811` |
| `3` | 4,840 | `dbbd191fdad4086166bd7cc8cf9ec98d3e8a04a438e93b7383c9fd00c51c4761` |
| `6` | 5,453 | `88e0cd32d4277759b04b1e8df7c00f8b898627faa690e981e317ed16aa5a4908` |
| `8` | 1 | `edd92648f4c16f31f1311e7955d4dab21bad61b6218606c1b23f8d5ffb804585` |
| `9` | 7,501 | `10830c57447d78f3ec71d51afd6f70d70d18f09aba4f2885a3f453bb7f3ec373` |
| `a` | 1 | `24aada24a1e47d5403d903c6cc83de7bf9f46070afd095ca0c3e8a3b497bf762` |
| `c` | 28,416 | `c7d69ebce5e5041dbdb2eadc6dfb2e48b784c126f093c50de1e2f8bbff7a7482` |
| `d` | 13,974 | `0ed682fe9829dab3d697893756caa7baa7654ebd52f1927461c9e0b841640dfb` |
| `f` | 18,958 | `16a74ff6c138dd363af3b7e04e560c22ce03f197ce2875801a4de0434635ef71` |

The sidecar counts sum to 10,593 and 84,329 respectively. Exact sidecar
directory equality plus the exact signature-index digests proves that every
failure record hash, canonical failure signature, and ordering is unchanged.
Exact summary equality proves every termination class is unchanged.

## No active uPD9002 or production behavior change

These checks exit zero:

```text
git diff --exit-code \
  b72e641733ddea6f0e8faef2507093f7c3aee5a4..HEAD -- cpu/upd9002/
git diff --name-status \
  b72e641733ddea6f0e8faef2507093f7c3aee5a4..HEAD -- cpu/upd9002/
```

The second command prints no paths. No instruction semantics, FLAGS behavior,
interrupt/exception behavior, dispatch, state layout, timing, prefetch,
cycle, or protected-mode implementation changed. No production C or C++
source in the active tree changed anywhere in M57.

## Builds and tests

All accepted builds used fresh out-of-tree directories. The pinned SST root
was:

```text
/tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21
```

| Configuration | Build directory | Result |
|---|---|---|
| G56 GCC tests-enabled baseline | `/tmp/vaeg-m57-g56-build.wAWcRq` | configure/build pass; both required SST profiles pass |
| M57 Linux GCC CI | `/tmp/vaeg-m57-linux-gcc.LxbJUZ` | configure/build pass; CTest 37/37 |
| M57 Linux Clang CI | `/tmp/vaeg-m57-linux-clang.fVghXC` | configure/build pass; CTest 37/37 |
| M57 Linux ASan/UBSan CI | `/tmp/vaeg-m57-linux-asan.G4ECnx` | configure/build pass; CTest 37/37 |
| M57 Linux release | `/tmp/vaeg-m57-linux-release.yttsxU` | configure/build pass; selftest and ROM-less smoke pass |
| M57 MinGW-w64 cross release | `/tmp/vaeg-m57-mingw-cross.ge84R6` | configure/build pass; Wine selftest and ROM-less smoke pass |
| Hosted Linux/Windows/macOS/Z80/invariants | build 30055319654 | all seven jobs passed |

Hosted implementation
[build 30055319654](https://github.com/nakatamaho/vaeg/actions/runs/30055319654)
completed successfully at
`6da8e1525e8fe5e69813b4a10fac9b69eaa2f495`. Its repository-invariant,
Ubuntu GCC, Ubuntu Clang, Ubuntu ASan, Windows MSYS2 MinGW64, macOS
FetchContent, and standalone Z80 conformance jobs all passed.

The GCC, Clang, and ASan CTest suites each include the external CI SST profile
and complete with zero failures. The sanitizer run emits only the documented
pre-existing UBSan backlog and exits zero with `ASAN_OPTIONS=detect_leaks=0`.

The Linux release selftest reports all component selftests passed. Its
ROM-less smoke exits zero. The MinGW artifact is a PE32+ x86-64 GUI
executable. `objdump -p` lists only Windows system DLLs; it imports none of
`SDL2.dll`, `libgcc_s_seh-1.dll`, `libstdc++-6.dll`, or
`libwinpthread-1.dll`. Wine reports every selftest passed and its ROM-less
smoke exits zero.

### Exact build and local test commands

The applicable commands below exited zero:

```text
cmake --preset linux-ci-gcc -B /tmp/vaeg-m57-g56-build.wAWcRq \
  -DVAEG_SSTS_V20_ROOT=/tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21
cmake --build /tmp/vaeg-m57-g56-build.wAWcRq --parallel

cmake --preset linux-ci-gcc -B /tmp/vaeg-m57-linux-gcc.LxbJUZ \
  -DVAEG_SSTS_V20_ROOT=/tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21
cmake --build /tmp/vaeg-m57-linux-gcc.LxbJUZ --parallel
ctest --test-dir /tmp/vaeg-m57-linux-gcc.LxbJUZ --output-on-failure

cmake --preset linux-ci-clang -B /tmp/vaeg-m57-linux-clang.fVghXC \
  -DVAEG_SSTS_V20_ROOT=/tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21
cmake --build /tmp/vaeg-m57-linux-clang.fVghXC --parallel
ctest --test-dir /tmp/vaeg-m57-linux-clang.fVghXC --output-on-failure

cmake --preset linux-ci-asan -B /tmp/vaeg-m57-linux-asan.G4ECnx \
  -DVAEG_SSTS_V20_ROOT=/tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21
cmake --build /tmp/vaeg-m57-linux-asan.G4ECnx --parallel
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy ASAN_OPTIONS=detect_leaks=0 \
  ctest --test-dir /tmp/vaeg-m57-linux-asan.G4ECnx --output-on-failure

cmake --preset linux-release -B /tmp/vaeg-m57-linux-release.yttsxU
cmake --build /tmp/vaeg-m57-linux-release.yttsxU --parallel
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  /tmp/vaeg-m57-linux-release.yttsxU/sdl2/vaeg --selftest
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  /tmp/vaeg-m57-linux-release.yttsxU/sdl2/vaeg --smoke

cmake --preset mingw-cross -B /tmp/vaeg-m57-mingw-cross.ge84R6
cmake --build /tmp/vaeg-m57-mingw-cross.ge84R6 --parallel
file /tmp/vaeg-m57-mingw-cross.ge84R6/sdl2/vaeg.exe
x86_64-w64-mingw32-objdump -p \
  /tmp/vaeg-m57-mingw-cross.ge84R6/sdl2/vaeg.exe
WINEDEBUG=-all WINEPREFIX=/tmp/vaeg-m57-wine.6uyfhz \
WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
  wine64 /tmp/vaeg-m57-mingw-cross.ge84R6/sdl2/vaeg.exe --selftest
WINEDEBUG=-all WINEPREFIX=/tmp/vaeg-m57-wine.6uyfhz \
WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
  wine64 /tmp/vaeg-m57-mingw-cross.ge84R6/sdl2/vaeg.exe --smoke
```

## Exact SST commands

The corpus verification command exited zero:

```text
python3 tools/qa/upd9002_ssts.py verify \
  --dataset-root /tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21 \
  --manifest tests/ssts/v20_dataset_manifest.json
```

At approved G56, the two no-skip runs were:

```text
python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21 \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m48.csv \
  --worker /tmp/vaeg-m57-g56-build.wAWcRq/sdl2/vaeg \
  --profile ci --shard-timeout 120 \
  --output /tmp/vaeg-m57-g56-ssts.f7dgTz/v20_native_ci.json \
  --failure-directory /tmp/vaeg-m57-g56-ssts.f7dgTz/v20_native_ci_failures \
  --expect tests/ssts/baseline/v20_native_ci.json
python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21 \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m48.csv \
  --worker /tmp/vaeg-m57-g56-build.wAWcRq/sdl2/vaeg \
  --profile full --shard-timeout 300 \
  --output /tmp/vaeg-m57-g56-ssts.f7dgTz/v20_native_full.json \
  --failure-directory /tmp/vaeg-m57-g56-ssts.f7dgTz/v20_native_full_failures \
  --expect tests/ssts/baseline/v20_native_full.json
```

At the M57 implementation SHA, the two no-skip runs were:

```text
python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21 \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m48.csv \
  --worker /tmp/vaeg-m57-linux-gcc.LxbJUZ/sdl2/vaeg \
  --profile ci --shard-timeout 120 \
  --output /tmp/vaeg-m57-post-ssts.QVWuWQ/v20_native_ci.json \
  --failure-directory /tmp/vaeg-m57-post-ssts.QVWuWQ/v20_native_ci_failures \
  --expect tests/ssts/baseline/v20_native_ci.json
python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21 \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m48.csv \
  --worker /tmp/vaeg-m57-linux-gcc.LxbJUZ/sdl2/vaeg \
  --profile full --shard-timeout 300 \
  --output /tmp/vaeg-m57-post-ssts.QVWuWQ/v20_native_full.json \
  --failure-directory /tmp/vaeg-m57-post-ssts.QVWuWQ/v20_native_full_failures \
  --expect tests/ssts/baseline/v20_native_full.json
```

The same `classify` command was run for both `--profile ci` and
`--profile full` at G56 and M57, with the pinned manifest/support map and
profile-specific classification and known-gap output paths.

The exact comparisons were:

```text
cmp /tmp/vaeg-m57-g56-ssts.f7dgTz/v20_native_ci.json \
  /tmp/vaeg-m57-post-ssts.QVWuWQ/v20_native_ci.json
cmp /tmp/vaeg-m57-g56-ssts.f7dgTz/v20_native_full.json \
  /tmp/vaeg-m57-post-ssts.QVWuWQ/v20_native_full.json
diff -qr /tmp/vaeg-m57-g56-ssts.f7dgTz/v20_native_ci_failures \
  /tmp/vaeg-m57-post-ssts.QVWuWQ/v20_native_ci_failures
diff -qr /tmp/vaeg-m57-g56-ssts.f7dgTz/v20_native_full_failures \
  /tmp/vaeg-m57-post-ssts.QVWuWQ/v20_native_full_failures
cmp /tmp/vaeg-m57-g56-ssts.f7dgTz/v20_native_ci_classification.json \
  /tmp/vaeg-m57-post-ssts.QVWuWQ/v20_native_ci_classification.json
cmp /tmp/vaeg-m57-g56-ssts.f7dgTz/v20_native_full_classification.json \
  /tmp/vaeg-m57-post-ssts.QVWuWQ/v20_native_full_classification.json
cmp /tmp/vaeg-m57-g56-ssts.f7dgTz/v20_native_ci_known_gaps.json \
  /tmp/vaeg-m57-post-ssts.QVWuWQ/v20_native_ci_known_gaps.json
cmp /tmp/vaeg-m57-g56-ssts.f7dgTz/v20_native_full_known_gaps.json \
  /tmp/vaeg-m57-post-ssts.QVWuWQ/v20_native_full_known_gaps.json
```

All eight comparisons exit zero and print nothing.

## Repository and reference checks

The following final commands exit zero:

```text
python3 tools/repo/check_encoding.py --expect utf8
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
python3 tools/repo/find_unreferenced.py --report
python3 tools/qa/upd9002_rename.py
python3 tools/qa/upd9002_native_invariant.py --root .
python3 tools/qa/upd9002_dispatch_normalization.py --root .
git diff --exit-code \
  b72e641733ddea6f0e8faef2507093f7c3aee5a4..HEAD -- cpu/upd9002/
git diff --check
```

Results:

- encoding: `0 violation(s)`;
- EOL: no finding;
- case/collision: `0 finding(s)`;
- rename ownership: PASS, retired active paths absent;
- native invariant: all 13 presets use the uPD9002 core;
- dispatch normalization: one constructor and exact immutable roots
  `256,256,256,256,8,8`;
- unreferenced scan: report-only, 98 pre-existing entries;
- active uPD9002 diff: empty;
- whitespace diff check: clean.

The reference audit finds no broken current build, package, or check path.
Archived path names that remain are intentionally pinned provenance,
historical records, or negative retirement assertions.

## Deviations and remaining risks

- The initially inspected primary worktree was dirty with maintainer handoff
  material. M57 used a clean linked worktree at exact G56 and left that
  primary worktree untouched.
- The first Linux release configure invocation did not reach a cache before
  the long FetchContent session returned control. Repeating the identical
  configure command completed, followed by a complete 501-step build,
  selftest, and smoke.
- A first post-M57 full `classify` invocation contained a mistyped temporary
  corpus path and exited before reading any shard. The corrected pinned path
  completed, and its output is byte-identical to the verified G56 output.
- Wine initially encountered the sandbox's read-only `/run/user/1000`. The
  same artifact and prefix were rerun outside that filesystem restriction;
  selftest and smoke both passed.
- `upd9002_protected_reachability.py` reports that regenerated post-M48
  provenance differs. The same result reproduces on untouched G56/current
  `main`; M57 does not modify its golden evidence or active CPU source. All
  configured reachability/deletion CTests pass. This pre-existing archaeology
  mismatch is not rebaselined in M57.
- Native Windows and macOS validation is supplied by hosted CI. This Linux
  host additionally supplies an independent MinGW cross-build and Wine run.
- The sanitizer build retains the documented existing UBSan backlog; M57 adds
  no sanitizer failure.
- Private ROM/disk assets were neither inspected nor recorded. The standard
  clean-checkout V3 boot, bundled VA demo, OS operations, keyboard, media,
  sound, save/load, and reset checks remain the maintainer's human G57 gate.

## G57 review boundary

M57 provides a candidate only. Maintainer review should confirm:

- the archive tag object and peeled commit;
- the byte-identical legal copy and provenance document;
- the exact 251-file deletion boundary;
- the three required implementation commit concerns;
- hosted CI and the local native/cross-platform results;
- both no-skip SST profiles and every exact digest/set comparison;
- the empty `cpu/upd9002/` diff;
- the standard V3/demo/OS human gate.

After the report commit, final `git status --short` in the linked M57
worktree prints no output. The primary worktree retains its original user
changes and was not modified. No M58 or later milestone work was started.
