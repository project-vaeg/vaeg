# UBSan Backlog

Scope: M9 ASan/UBSan smoke on `topic/m9-va-portable` after rebasing onto
the M10 canonical-canvas frontend.

## Reproduction

```sh
cmake --preset linux-asan
cmake --build --preset linux-asan
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ASAN_OPTIONS=detect_leaks=0 \
  ./build/asan/sdl2/vaeg --smoke
```

## Recorded Items

| File | Line | UBSan finding | Status |
|---|---:|---|---|
| `sound/tms3631c.c` | 51 | left shift of a negative value in `tms3631_setvol()` while building `tms3631cfg.feet[]` | Pre-existing shared-core UB; recorded only, not fixed in M9. |
| `sound/psggenc.c` | 119 | shift overflow after `psg->noise.freq <<= PSGFREQPADBIT` | Pre-existing shared-core UB; recorded only, not fixed in M9. |
| `sound/psggeng.c` | 61 | signed integer overflow while subtracting `psg->noise.freq` from `psg->noise.count` | Pre-existing shared-core UB observed in the same run; recorded only, not fixed in M9. |
| `common/parts.c` | 15 | signed integer overflow in the legacy `rand_get()` linear-congruential multiply/add | Pre-existing shared-core UB observed in the same run; recorded only, not fixed in M9. |
