#!/usr/bin/env python3
"""Reject non-Windows runtime DLL imports from a static MinGW VAEG build.

Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""
import argparse
import pathlib
import re
import subprocess
import sys


WINDOWS_SYSTEM_DLLS = {
    "advapi32.dll", "bcrypt.dll", "bcryptprimitives.dll", "cfgmgr32.dll",
    "combase.dll", "comctl32.dll", "comdlg32.dll", "crypt32.dll",
    "d3d11.dll", "d3d12.dll", "d3dcompiler_47.dll", "dwmapi.dll",
    "dxgi.dll", "gdi32.dll", "hid.dll", "imm32.dll", "kernel32.dll",
    "msimg32.dll", "msvcrt.dll", "ntdll.dll", "ole32.dll", "oleaut32.dll", "opengl32.dll",
    "rpcrt4.dll", "setupapi.dll", "shell32.dll", "shlwapi.dll", "user32.dll",
    "userenv.dll", "uuid.dll", "version.dll", "winmm.dll", "winspool.drv",
    "ws2_32.dll",
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=pathlib.Path, required=True)
    args = parser.parse_args()
    if not args.binary.is_file():
        parser.error("binary does not exist: " + str(args.binary))
    output = subprocess.check_output(["objdump", "-p", str(args.binary)], text=True)
    imports = [match.group(1).lower() for line in output.splitlines()
               if (match := re.search(r"DLL Name:\s*(\S+)", line, re.I))]
    if not imports:
        print("no PE DLL imports found", file=sys.stderr)
        return 1
    print("Windows DLL imports:")
    for name in imports:
        print("  " + name)
    unexpected = [name for name in imports
                  if name not in WINDOWS_SYSTEM_DLLS
                  and not name.startswith(("api-ms-win-", "ext-ms-"))]
    if unexpected:
        print("unexpected non-Windows runtime DLL imports: " + ", ".join(unexpected),
              file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
