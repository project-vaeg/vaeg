# M8 — SDL2 frontend (`sdl2/`) on the PC-98 core, Linux

Status: not-started
Branch: topic/m8-sdl2-frontend
Gate: G8 (human: frontend gate on the PC-98 scaffold)
Depends on: G7 passed

## Goal

A new SDL2 frontend under `sdl2/` — main loop, video, audio, input,
timer — that links `vaeg_core`/`vaeg_common` and runs the plain PC-98
machine on Linux. This is a PORT of the SDL1 code in `sdl/`, not a
rewrite: keep the module boundaries (np2.c, scrnmng, soundmng,
inputmng/sdlkbd, mousemng, joymng, dosio, ini, fontmng, commng) and
translate SDL1 APIs to SDL2.

## API translation map (from prior analysis; verify against the code)

- scrnmng: SDL_SetVideoMode/SDL_Surface flip →
  SDL_Window + SDL_Renderer + streaming SDL_Texture
  (SDL_PIXELFORMAT_RGB565 or ARGB8888 to match the core's surface
  format — check scrnmng.c bpp handling before choosing).
- soundmng: SDL_OpenAudio → SDL_OpenAudioDevice; callback signature and
  sample format audit (AUDIO_S16SYS).
- sdlkbd: SDLK_LAST is gone; keysym tables become SDL_Scancode-based.
- taskmng/main loop: SDL_WaitEvent/PollEvent semantics; frame pacing via
  SDL_GetPerformanceCounter, not SDL_Delay-only.
- fontmng: include-path changes only, verify.
- dosio/ini: POSIX paths; config dir = XDG (~/.config/vaeg/) with the
  ini format unchanged.

## Constraints

- New files under sdl2/ only (plus CMake target vaeg_sdl2). Core
  untouched. sdl/ untouched (reference material).
- C for the frontend modules (C++17 permitted but not required here;
  ImGui in M10 will introduce C++ — keep main loop C++-compatible).
- SDL2 via find_package/pkg-config. No SDL_ttf/SDL_image dependencies.
- Add a `--smoke` flag: init video+audio (honoring SDL_VIDEODRIVER=dummy),
  run N frames of the core, exit 0. This is the agent-side check for
  every later milestone.
- UI text minimal in this milestone (window title, error messages),
  UTF-8 literals are fine (sources are UTF-8, gcc/clang execution
  charset is UTF-8; no CP932 anywhere in sdl2/).

## Steps

1. Port module by module in the order: dosio/ini → taskmng/main →
   scrnmng → sdlkbd/inputmng → soundmng → mousemng/joymng → commng.
   One commit per module.
2. CMake target vaeg_sdl2; wire into presets.
3. Smoke flag + headless run.
4. Write sdl2/README.md: build, run, ROM placement (romimage/ lookup
   order), config location.

## Machine checks (paste into PR)

    cmake --build build/linux-debug --target vaeg_sdl2
    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/.../vaeg --smoke
    tools/repo checkers as in M7

## Gate G8 (human, Linux)

Window opens, PC-98 machine starts (with ROMs if available; without
ROMs, defined error path — no crash), keyboard input reaches the guest,
audio initializes, clean exit. No VA functionality expected yet.

## Do not do

- No VA subsystem, no ImGui, no menu system beyond what SDL1 np2.c had.
- No Windows/macOS code paths yet (that is M11).
- Do not delete sdl/.

## Risks to report

- Core assumptions about surface format/pitch that fight the SDL2
  texture path.
- Timing coupling between soundmng callback and the main loop.
- Anything in dosio that presumes case-insensitive filesystems.
