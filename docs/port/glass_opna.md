<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
-->

# GLASS ORBIT P6: OPNA/YM2608 audio

The final audio path uses one backend on the VA port: the YM2608/OPNA register path. OPL,
OPL2, OPL3, YM3812, and YMF262 are not compiled into this payload. Graphics,
SGP lists, CPU stars, page exchange, and the existing VA BIOS exit path remain
unchanged.

## Hardware contract

The VA Music BIOS notes in `docs/tekumani/611MUSIC.TXT` define the OPNA register
pairs as `44h/45h` for the low bank and `46h/47h` for the high bank. The active
VAEG Sound Board II binding in `io/boardsb2.c:201-236` attaches exactly those
four guest ports; the high pair is present only for the OPNA board. The payload
performs a bounded, read-only status probe on both address ports. A missing high
bank disables audio without stopping the graphics scene.

The data write path polls the YM2608 busy bit (bit 7) with a finite limit before
each address/data pair. It does not install an interrupt or depend on a DOS
service. The port layer is isolated in `src/glass_opna.inc`.

## Music source and timing

The original `demos/neon/GLASS_1_0/GLASS_OPNA.INC` and `GLASS_DATA.INC` use a
channel-major score with 512 steps. The first 256 steps are repeated verbatim
in the original file. The VA payload therefore stores one 256-step phrase for
each of the original audible FM channels 0, 1, and 2 and masks the index at
256; this is byte-for-byte equivalent to the original 512-step data. Original
channels 3--11 are all `FFh`, and the original OPNA rhythm path is disabled.

The final scene calls `glass_opna_init` once, advances the score from `glass_opna_tick` once
per completed graphics frame, and calls `glass_opna_shutdown` on both the ESC
path and the failure path. The original divider of six frames per score step is
retained. At a nominal 60 Hz frame cadence this is approximately 10 score steps
per second; VAEG does not provide a cycle-accurate timing claim here.

The OPNA FM patch uses the first three original channel patches, F-number table,
key-on/key-off register, and block/note encoding. SSG registers are initialized
and silenced transactionally. The GLASS score's SSG channels are intentionally
silent in the source data, so no unrelated accompaniment sequencer was added.

## Verification

The source/data contract can be checked without ROMs or hardware:

```sh
python3 demos/glass-orbit/tools/verify-opna-source.py demos/glass-orbit
```

The final payload build uses the existing command:

```sh
demos/glass-orbit/build.sh /absolute/path/GLASS.COM
```

For VAEG, select `va2` (the default YM2608 model) and use the existing
`run-vaeg.sh` harness. The capture must still reach the same
`3000:177d` frame-ready checkpoint, and `verify-temporal.py` remains the
graphics regression check. These are emulator-side checks only; audible output
and post-ESC audio silence still require a physical PC-88VA/VA2 gate.

## Scope boundary

This is an OPNA-only adaptation of the original GLASS music semantics:

```text
original GLASS OPNA score -> VA YM2608 register layer -> OPNA
```

It is not a new unrelated sequencer and it does not claim real-hardware
conformance until the same payload is heard and exited on the target machines.
