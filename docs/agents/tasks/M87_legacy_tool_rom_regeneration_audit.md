# M87 - Legacy tool and ROM regeneration audit

M87 audits remaining legacy tool sources and ROM/resource regeneration flows
after active machine-core sources have moved out of the repository root. The
`lio/` BIOS/LIO compatibility path was resolved early in M81 and is not an M87
open item.

Predecessor: approved G86.

Branch: `topic/m87-legacy-tool-rom-regeneration-audit`

Commit prefix: `M87:`

Candidate gate: `G87`

Report: `docs/agents/reports/m87_legacy_tool_rom_regeneration_audit.md`

Do not start M88. Do not merge M87 to `main` before G87 approval. Do not
declare G87 passed.

## Scope

M87 must audit:

- `accessories/`, including `bin2txt`, `lzxpack`, and `textout`;
- `np2tool/` legacy assembly utilities;
- `romimage/` regeneration makefiles and assembly sources;
- current CMake asset embedding through `cmake/embed_binary.cmake`;
- generated-asset consumers in the SDL2 frontend;
- release packaging assumptions for assets and ROM-related files.

`accessories/` is an initial deletion candidate because it is not part of the
active CMake build and appears to serve only old ROM/resource generation
flows. Any deletion must prove that active asset embedding uses
`cmake/embed_binary.cmake` and that no current build, test, release, or
manual gate depends on the legacy tools.

## Non-goals

M87 must not modify binary payloads, ROM images, guest ROM contents, fonts,
icons, splash assets, or approved historical evidence. It must not replace
ROM generation tooling with a new unapproved ROM build pipeline.

## Validation

Run repository invariant checks, normal builds, native tests, release-package
smoke checks where available, and any focused checks for generated assets or
ROM/resource regeneration references.

## Closure

The final report must list each legacy tool and ROM-generation path as
active-required, inactive-removable, deferred, or blocked by an evidence gap.
