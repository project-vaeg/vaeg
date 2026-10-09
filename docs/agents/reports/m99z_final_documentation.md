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
# M99z — final documentation and evidence assembly

Status: PASS for the documentation and machine-verifiable evidence assembly;
overall M99 remains BLOCKED by required real-GPU and performance evidence.

## Documentation completed

- [`native-crt-presentation.md`](../../architecture/native-crt-presentation.md)
  records framebuffer flow, backend ownership, lifecycle, capture isolation,
  and fallback transitions.
- [`native-crt-user-guide.md`](../../modernization/native-crt-user-guide.md)
  records build, enablement, package layout, runtime names, and capture use.
- [`native-crt-troubleshooting.md`](../../modernization/native-crt-troubleshooting.md)
  records diagnostic messages, loader-path checks, and recovery steps.
- [`m99g_dependency_audit.md`](m99g_dependency_audit.md) completes the
  dependency/provenance report required by the milestone sequence.
- `sdl2/README.md` now links the three user and architecture documents.

## QA and correction

The source-archive audit initially rejected the newly approved
`external/librashader` root. `tests/z80_compat/check_zex_archive.py` now
contains that explicit root in its allow-list. A source archive containing the
complete candidate tree then passed with 1,925 files; the existing ZEX and
private-media protections remain active.

The following repository checks passed after the documentation and allow-list
changes:

```text
python3 tools/qa/milestone_ids.py --selftest
python3 tools/qa/milestone_ids.py --discover
python3 tools/qa/milestone_ids.py --audit
python3 tools/repo/check_encoding.py --expect utf8
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
git diff --check
```

The feature-on Linux build, focused librashader/ROM-less CTest (7/7), and
dummy-driver selftest were completed in M99y. The macOS feature-on build and
focused tests were also completed. Hosted smoke failures exposed an inverted
`SUCCESS == 0` test in the SDL resource startup/fallback path; M99z corrects
both comparisons, and the macOS dummy-driver `--smoke` run now exits 0. No
physical GPU result is claimed here.

## Gate disposition

G99-1, G99-2, and G99-6 remain PASS within their documented evidence scope.
G99-3, G99-4, G99-5, and G99-7 remain deferred because this environment does
not provide real Windows D3D11, Linux OpenGL, or macOS Metal presentation and
performance hardware. The final goal report records the hosted CI result and
the smallest remaining action.
