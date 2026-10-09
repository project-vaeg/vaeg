Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# NEON text-console ownership

## Root cause

The `DIR A:`, `DIR B:`, `COPY`, and `TIME` row is not a NEON string.  The
NEON payload has no such literals, while `neon_status_clear_rows` explicitly
left the resident system-line rows to the loader/editor environment.  That
made the row visible whenever the TEXT plane was composed above G0.

GLASS did not expose the row because its composition is graphics-only.  NEON
must keep TEXT enabled, so hiding TEXT is not a valid correction.

## GLASS versus NEON

| Operation | GLASS | NEON | Relevance |
| --- | --- | --- | --- |
| TEXT composition | graphics-only composition; TEXT is off | `CX=0031h`, TEXT above G0 | NEON must keep text enabled |
| Soft-key guide | invisible with TEXT off | inherited from the caller unless owned | source of the bottom row |
| Text output | none in the graphics path | VA Text BIOS INT 83h | intended overlay must remain |

GLASS's graphics-only composition explains why the stale guide was not visible;
it is not a suitable NEON fix.

## BIOS service

The PC-88VA Text BIOS documents `INT 83h/AH=2Fh` as soft-key display control.
The low field of `AL` selects the number of function-key entries: `0`, `5`,
or `10`; the `SHMD` bit controls shift-state display.  Startup uses
`AH=2Fh, AL=00h` to stop the guide producer while leaving TEXT enabled.  The
BNN manual's OCR has unrelated following text in this section, so the table
and parameter description at
`docs/tekumani/PC88VA_テクニカルマニュアル_BNN.md:38281-38308` are the
authoritative local reference for this call.

VAEG showed that suppressing the producer alone leaves the system-line mode
marker visible.  The documented Screen Editor `INT 94h/AH=01h, AL=FFh` call
therefore follows it to hide the complete system-line display
(`docs/tekumani/PC88VA_テクニカルマニュアル_BNN.md:49790-49829`).  This is
not a TEXT-off operation: it removes only the system-line presentation.  On
exit the pair is restored with `INT 83h/AH=2Fh, AL=0Ah` and
`INT 94h/AH=01h, AL=0Ah`.

## Startup sequence

`neon_text_console_init` runs before VA graphics mode entry:

```text
Text BIOS INT 83h/AH=2Fh AL=00h (stop producer)
Screen Editor INT 94h/AH=01h AL=FFh (hide system line)
Text BIOS INT 83h/AH=25h AL=00h (hide cursor and blink)
VA graphics INT 8Fh ($ScnMode/$DefBuf/$DefWin/$Compose/$ScnDsp)
one-time TVRAM clear and static overlay labels
VBLANK-boundary fixed-width live fields
```

The startup clearing path covers all 25 main rows after the soft-key producer
and system-line display have been disabled.  During the continuous timeline,
the payload does not clear rows or toggle display composition: it overwrites
the fixed-width frame fields after the completed page is presented and rewrites
the scene title field only on a scene transition.  No DOS `INT 21h`, periodic
bottom-row erase, or direct hard-coded TVRAM erase is used.

## Exit

The existing VA graphics leave path remains unchanged.  Before returning, the
payload restores the normal ten-entry guide with Text BIOS
`INT 83h/AH=2Fh, AL=0Ah`, Screen Editor `INT 94h/AH=01h, AL=0Ah`, and the
normal cursor blink with Text BIOS `INT 83h/AH=25h, AL=03h`.  The loader/editor
then returns with its saved continuation.

## QA status

The source-level checks are:

* TEXT composition remains `CX=0031h` (text above G0), so TEXT is enabled;
* intended NEON strings continue to use `INT 83h`;
* `INT 83h/AH=2Fh` stops the soft-key producer and `INT 94h/AH=01h` hides
  the system-line presentation;
* `INT 83h/AH=25h, AL=00h` suppresses the cursor and its blink during the demo;
* no DOS `INT 21h` appears in the NEON VA payload;
* graphics SGP/G0 code is unchanged.

The VAEG capture's visible-main TVRAM region (4000 bytes, 80x25 cells) was
checked with `demos/neon3/tools/verify_text_console.py`:

```text
VISIBLE_NONBLANK_BYTES=1082
FORBIDDEN_VISIBLE=NONE
TEXT_VISIBLE=PASS
BOTTOM_GUIDE=PASS
```

The captured backing area may still contain old caller bytes, but `AL=00h`
stops the soft-key producer and the visible 80x25 main plane is clean.  No
destructive TVRAM wipe is required.  The VAEG stale-console check is complete;
a hardware human gate remains required for VA/VA2 cursor, post-return, and
visual graphics behavior.  The human check must verify that intended NEON text
is visible, the `DIR A:`/`COPY`/`TIME` guide is absent, and TEXT remains enabled
above the unchanged graphics.
