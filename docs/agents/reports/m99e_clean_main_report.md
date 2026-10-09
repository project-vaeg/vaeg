<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M99e clean-main baseline

Status: PASS

The rewritten `main` baseline was verified before the M99 implementation was
started. It contains the retained non-M99 history and no M99 implementation
files or M99 feature commits in its ordinary reference ancestry.

## Baseline identity

- Evaluated commit: `0b6e14883035cb17073c5c24c8c9d4b5c22b9162`
- `main` and `origin/main` resolve to the evaluated commit.
- The M99 topic branch was created from this baseline.
- The working tree used for this check was clean apart from the pre-existing
  untracked `va2bkupmem.dat` in the temporary topic worktree.

## Verification

Configured and built the clean baseline with:

```text
cmake -S <clean-main-checkout> -B <clean-main-build> -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON \
  -DVAEG_WERROR=OFF
cmake --build <clean-main-build> --parallel 4
ctest --test-dir <clean-main-build> --output-on-failure
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  <clean-main-build>/sdl2/vaeg --smoke
```

The build completed successfully. CTest registered 84 tests, ran 83, and
passed all runnable tests; one pre-existing external SSTS test was skipped.
The ROM-less smoke test exited successfully. It reported the expected absence
of the optional VA2/VA3 font ROM, so its uniform-screen check remained
disabled; this does not change the clean-baseline result.

This baseline is the starting point for M99f and later implementation work.
