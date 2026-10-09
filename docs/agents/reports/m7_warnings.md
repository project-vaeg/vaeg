# M7 warning inventory

Build log sources:

- `/tmp/m7_gcc_configure.log`
- `/tmp/m7_gcc_build.log`
- `/tmp/m7_clang_configure.log`
- `/tmp/m7_clang_build.log`

GCC commands:

```text
cmake --preset linux-debug
cmake --build build/linux-debug --clean-first
```

Result: gcc build passed.

## GCC warning counts

| Directory | Warning category | Count |
|---|---:|---:|
| bios | `-Wunused-const-variable=` | 4 |
| font | `-Wsizeof-pointer-memaccess` | 1 |
| i286c | `-Wunused-but-set-variable` | 3 |
| io | `-Wunused-const-variable=` | 10 |
| sound | `-Wunused-but-set-variable` | 1 |
| sound/getsnd | `-Wunused-but-set-variable` | 8 |
| sound/vermouth | `-Wunused-but-set-variable` | 1 |

Total gcc warnings: 28.

## Clang warning counts

Clang commands:

```text
CC=clang CXX=clang++ cmake -B build/clang -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/clang --clean-first
```

Result: clang build passed.

| Directory | Warning category | Count |
|---|---:|---:|
| bios | `-Wunused-but-set-variable` | 1 |
| bios | `-Wunused-const-variable` | 4 |
| font | `-Wsizeof-array-decay` | 1 |
| font | `-Wsizeof-pointer-memaccess` | 1 |
| i286c | `-Wunused-but-set-variable` | 3 |
| io | `-Wunused-const-variable` | 10 |
| sound | `-Wunused-but-set-variable` | 2 |
| sound/getsnd | `-Wunused-but-set-variable` | 8 |
| sound/vermouth | `-Wunused-but-set-variable` | 1 |

Total clang warnings: 31.

## Warning flags

`-Wall` is set for both CMake static library targets:
`CMakeLists.txt:204` applies it to `vaeg_common`, and
`CMakeLists.txt:213` applies it to `vaeg_core`.

## Unity build and `str_np2`

No CMake unity build mechanism is enabled. `CMakeLists.txt` and
`CMakePresets.json` contain no `UNITY`, `CMAKE_UNITY_BUILD`, or unity
property setting.

The `io/np2sysp.c` local rename was not for unity builds. The file
includes `strres.h` at `io/np2sysp.c:2`, and `common/strres.h:52`
declares `extern const OEMCHAR str_np2[]`. Before M7, the same
translation unit also declared `static const char str_np2[] = "NP2"` at
`io/np2sysp.c:105`, which conflicts with the prior external declaration.
M7 therefore renamed only the local command string to `str_np2cmd`.

## Compile-blocking source deviations

These were required to compile the M7 plain PC-98 C core with gcc. No
warning-only fixes were made.

| File | Reason |
|---|---|
| `io/pic.c:6` | `iocoreva.h` is VA-only and `iova/` is outside the M7 include path (`CMakeLists.txt:34`-`50`); guarded the include with `SUPPORT_PC88VA` for the plain PC-98 core build. |
| `io/np2sysp.c:105` | Local command string `str_np2` conflicted with `common/strres.h:52` extern `str_np2` in the same translation unit; renamed the local static symbol only. |
| `pccore.c:16` | `biosva.h` is VA-only and `biosva/` is outside the M7 include path (`CMakeLists.txt:34`-`50`); guarded the include with `SUPPORT_PC88VA` for the plain PC-98 core build. |
| `pccore.c:941`, `pccore.c:965`, `pccore.c:995`, `pccore.c:1173` | Debug/breakpoint callbacks are only declared under `VAEG_EXT` (`pccore.h:165`-`169`, `breakpoint.h:1`-`97`); guarded their use for the portable CMake core build. |
| `pccore.c:1223` | Single-step dispatch now uses `USE_I286C`, defined only by CMake for `vaeg_core` (`CMakeLists.txt:212`), so future VA-on-C builds can combine `SUPPORT_PC88VA` with `USE_I286C`; the legacy `#else` path still calls `i286x_step`/`v30x_step`. |

## `VAEG_EXT` scope

`VAEG_EXT` is defined by the frozen Win9x frontend in
`win9x/compiler.h:126`. It is not defined by `sdl2/compiler.h` or by
the M7 CMake build. In shared core files it exposes debugger and
breakpoint extension hooks, including `DEBUGCALLBACK` in
`pccore.h:165`-`169`, breakpoint declarations in `breakpoint.h:1`-`97`,
and pccore calls at `pccore.c:941`, `pccore.c:965`, `pccore.c:995`, and
`pccore.c:1173`.

The extension is tied to the Win32 debugger/tool windows: for example,
`win9x/debuguty/debugctrl.cpp:2` includes `<windowsx.h>`,
`win9x/debuguty/debugctrl.cpp:50` stores an `HWND`, and
`win9x/debuguty/debugctrl.cpp:230` declares a `LRESULT CALLBACK` window
procedure. `win9x/np2.cpp:2232`-`2236` wires that frontend callback into
`pccore_debugsetcallback`. M7 therefore leaves `VAEG_EXT` undefined and
keeps the portable core build out of that Win32-only debugger path.
