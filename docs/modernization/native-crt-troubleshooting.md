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
# Native CRT troubleshooting

Native CRT is intentionally fail-closed. A failure should leave the normal
SDL presentation path usable and should be visible in the diagnostic output.

## Read the startup message

Typical messages include:

| Message or error | Meaning | Action |
| --- | --- | --- |
| `Native CRT unavailable` | Runtime, ABI, preset, or native-device initialization failed | Confirm the platform runtime and preset path; continue with SDL if CRT is not required |
| `Native CRT selected but unavailable; using SDL fallback` | The selected native presenter could not be created | Check the following error line and the platform requirements |
| `Preset load failed` | The `.slangp` path or its shader closure is unavailable/invalid | Start from the package directory or set an absolute `NativeCRTPreset` path |
| `Native CRT presentation failed` | A frame, filter, drawable, or device operation failed | Restart once; if it repeats, disable `NativeCRT=0` and retain the diagnostic |
| `Native CRT disabled after failure; SDL presentation restored` | VAEG recovered by returning to its established SDL renderer | No data or guest state was discarded; investigate the preceding error if needed |
| `rendered capture is unavailable` | A post-scale SDL capture was requested while native owns the window | Use `--screenshot` or `--screen-tvram-dump` for deterministic evidence |

The error names `runtime_missing`, `abi_mismatch`, `device_failure`,
`preset_failure`, `filter_failure`, and `resource_failure` identify the
failure class in the native presenter tests and diagnostic output.

## Runtime checks

The dynamic loader expects the exact basename for the host:

```text
Linux:  librashader.so
macOS:  librashader.dylib
Windows: librashader.dll
```

Keep it beside the executable in a release package. On Linux/macOS development
runs, use the package directory as the loader search path when necessary:

```sh
LD_LIBRARY_PATH="$PWD" ./vaeg
DYLD_LIBRARY_PATH="$PWD" ./vaeg
```

The runtime must match the packaged architecture. The release checker rejects
a runtime with the wrong platform name, and the librashader loader rejects an
incompatible ABI. VAEG does not silently substitute a different runtime.

## Platform requirements

- macOS requires the Metal path; this feature does not create an OpenGL
  context on macOS.
- Windows requires the D3D11/DXGI path and a usable application window.
- Linux requires the OpenGL 3.3 core path. A dummy SDL video driver bypasses
  native presentation so headless tests remain deterministic.

Software or virtual display output is useful for startup and fallback smoke
tests, but it is not evidence of real-GPU CRT performance. Report the actual
host, driver, and display environment when filing a platform result.

## Recovering a run

1. Start once with `--no-cfg` to confirm that the established SDL renderer can
   run without the saved Native CRT setting.
2. If that succeeds, set `NativeCRT=0` in the current `vaeg.cfg` and retain
   the preset/runtime paths for inspection.
3. Restore the default preset path and verify that the asset closure exists.
4. Re-enable CRT only after the runtime basename, architecture, and platform
   API have been checked.

Do not use a filtered screenshot as a replacement for a raw golden capture.
Keep the startup diagnostic and the exact package identity with any GPU report.
