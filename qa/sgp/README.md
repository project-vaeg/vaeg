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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# SGP line-rendering QA

These tests intentionally use the PC-Engine 1.1 disk only as a local DOS
execution harness. The final COM programs call the documented graphics BIOS or
the SGP directly; they do not use a host-side drawing shortcut. `AH=15h`
PAINT and SGP SCAN/PATBLT are disabled until the line-only tests have passed.

## Build the ladder

From the repository root, assemble the BIOS path as follows:

```sh
/opt/local/bin/nasm -f bin -dSGPSCAN_B1 demos/sgp-scan/16/sgpscanb.asm -o /private/tmp/SGPSCANB1.COM
/opt/local/bin/nasm -f bin -dSGPSCAN_B2 demos/sgp-scan/16/sgpscanb.asm -o /private/tmp/SGPSCANB2.COM
/opt/local/bin/nasm -f bin -dSGPSCAN_B3 demos/sgp-scan/16/sgpscanb.asm -o /private/tmp/SGPSCANB3.COM
/opt/local/bin/nasm -f bin demos/sgp-scan/16/sgpscanb.asm -o /private/tmp/SGPSCANB.COM
```

`B1`, `B2`, and `B3` are a short horizontal line, a short vertical line, and
a short diagonal line. The default is the closed triangle `B4`.

The direct-SGP ladder is assembled similarly:

```sh
/opt/local/bin/nasm -f bin -dSGPSCAN_P1 demos/sgp-scan/16/sgpscanp.asm -o /private/tmp/SGPSCANP1.COM
/opt/local/bin/nasm -f bin -dSGPSCAN_P2 demos/sgp-scan/16/sgpscanp.asm -o /private/tmp/SGPSCANP2.COM
/opt/local/bin/nasm -f bin -dSGPSCAN_P3 demos/sgp-scan/16/sgpscanp.asm -o /private/tmp/SGPSCANP3.COM
/opt/local/bin/nasm -f bin -dSGPSCAN_P4 demos/sgp-scan/16/sgpscanp.asm -o /private/tmp/SGPSCANP4.COM
/opt/local/bin/nasm -f bin demos/sgp-scan/16/sgpscanp.asm -o /private/tmp/SGPSCANP.COM
```

`P1` is a four-pixel horizontal segment; `P2`, `P3`, and `P4` are horizontal,
vertical, and diagonal controlled segments. The default is the triangle `P5`.

For each run, create a local copy of `docs/disks/pcengine110-bootonly.d88`,
install the COM into that copy with `tools/pc88va/pcengine_disk.py`, and run:

```sh
printf '@wait 1200\nSGPSCANB\n@wait 600\n' > /private/tmp/sgp-input.txt
VAEG_SCREEN_EXIT_MS=30000 build/linux-debug/sdl2/vaeg \
  --model va --roms docs/roms --fdd1 /private/tmp/sgpscan.d88 \
  --headless-input-script /private/tmp/sgp-input.txt \
  --screen-dump /private/tmp/sgpscan.bmp
```

`--screen-dump` may be a BMP on builds that select the BMP writer; convert it
to a true-color PNG before validation, for example
`convert /private/tmp/sgpscan.bmp PNG24:/private/tmp/sgpscan.png`. The current frontend adds a 22-pixel menu strip, so
the validator crops it and checks the guest viewport rather than counting DOS
text or menu pixels.

```sh
python3 qa/sgp/validate_triangle.py qa/artifacts/sgp/bios-triangle.png
python3 qa/sgp/validate_triangle.py qa/artifacts/sgp/direct-triangle.png
```

The validator checks endpoints, sampled pixels along all three edges, one
connected component, the expected bounding box, and rejects a component that
spans the viewport. A nonzero foreground-pixel count alone cannot pass it.

The generated PNGs under `qa/artifacts/sgp/` are local diagnostic artifacts;
they are not hardware golden data and must not be used to claim PC-88VA
real-hardware behavior.
