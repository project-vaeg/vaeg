; Copyright (c) 2026 Nakata Maho
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
; 1. Redistributions of source code must retain the above copyright notice,
;    this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright notice,
;    this list of conditions and the following disclaimer in the documentation
;    and/or other materials provided with the distribution.
;
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
; IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
; OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
; IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
; INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
; BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
; AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
; TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
; OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

; NEON RELAY 4 P3/P5 VA payload.
;
; The 4bpp path is the 640x400, 16-colour VA palette profile.  Each pixel
; selects one of sixteen palette entries; the entries themselves use the
; documented 4096-colour (12-bit) G/R/B layout.  The 8bpp path follows
; topic/m97-sgp-tekumani:demos/sgp-pseudo-sprite/256:
; INT 8Fh enters 320x200 G0/G1 with CX=0808h, G1 uses a 320x400 backing
; surface, and FB1 DSA selects its two 64,000-byte pages.  The 16bpp path
; follows the corresponding direct-colour G0 setup from the 65536 demo and
; uses two 128,000-byte pages in the same 256 KiB GVRAM window.

        cpu     286
        bits    16
        org     0

%ifndef NEON4_STAGE
%define NEON4_STAGE 8
%endif
%define NEON4_TEXT_SCENE_COUNT 8
%define NEON4_TEXT_TOTAL_FRAMES 3072
%ifndef NEON4_P5_BPP
%define NEON4_P5_BPP 8
%endif
%if (NEON4_P5_BPP != 4) && (NEON4_P5_BPP != 8) && (NEON4_P5_BPP != 16)
%error "NEON4_P5_BPP must be 4, 8, or 16"
%endif
%ifndef NEON4_PIXEL_ARGS
%if NEON4_P5_BPP == 4
%define NEON4_PIXEL_ARGS 0004h
%elif NEON4_P5_BPP == 16
%define NEON4_PIXEL_ARGS 0010h
%else
%define NEON4_PIXEL_ARGS 0808h
%endif
%endif
%ifndef NEON4_MODE
%if NEON4_P5_BPP == 4
%define NEON4_MODE 0a000h
%elif NEON4_P5_BPP == 16
%define NEON4_MODE 0a00eh
%else
%define NEON4_MODE 0e00eh
%endif
%endif
%ifndef NEON4_USE_DEFAULT_BUFFERS
%define NEON4_USE_DEFAULT_BUFFERS 1
%endif
%ifndef NEON4_DIRECT_REGS
%define NEON4_DIRECT_REGS 0
%endif
%ifndef NEON4_DIAG_MARKER
; Direct TVRAM writes are intentionally disabled on the 8bpp path.  The
; graphics BIOS owns the text-plane mapping during mode transition; enabling
; this probe can corrupt the mode state before the first GVRAM write.
%define NEON4_DIAG_MARKER 0
%endif
%ifndef NEON4_P4_BACKEND
; P4 diagnostic backend: 0 = CPU reference, 1 = SGP command list.
%define NEON4_P4_BACKEND 0
%endif
%if NEON4_STAGE == 8
%define NEON4_286 1
%define NEON4_P0 1
%define OPL_PROBE_AUTO 0
%define OPL_DETECT_NONE 0
%define EGC_LENGTH_PORT 0
%define EGC_SHIFT_PORT 0
%endif

%define VIDEO_BIOS_INT          8fh
%define KEYBOARD_BIOS_INT       82h
%define TEXT_BIOS_INT           83h
%define SCREEN_EDITOR_BIOS_INT  94h
%define VIDEO_DATA_SEG          0338h

%define PORT_MEMORY_MAP         0153h
%define PORT_TSP_STATUS         0142h
%define PORT_SGP_COMMAND        0500h
%define PORT_SGP_CONTROL        0504h
%define PORT_SGP_STATUS         0506h
%define PORT_GVRAM_WRITE_MODE   0580h
%define PORT_COL_COMP           0106h
%define PORT_RGB_COMP           0108h
%define PORT_G0_TRANSPARENCY    0124h
%define PORT_G1_TRANSPARENCY    0126h
%define PORT_FB1_FBW            0224h
%define PORT_FB1_DOT            0228h
%define PORT_FB1_OFX            022ah
%define PORT_FB1_OFY            022ch
%define PORT_FB1_DSA_LOW        022eh
%define PORT_FB1_DSA_HIGH       0230h
%define PORT_FB1_DSH            0232h
%define PORT_FB1_DSP            0236h
%define PORT_GRMODE             0100h
%define PORT_GRRES              0102h
%define PORT_FB0_FBW            0204h
%define PORT_FB0_FBL            0206h
%define PORT_FB0_DOT            0208h
%define PORT_FB0_OFX            020ah
%define PORT_FB0_OFY            020ch
%define PORT_FB0_DSA_LOW        020eh
%define PORT_FB0_DSA_HIGH       0210h
%define PORT_FB0_DSH            0212h
%define PORT_FB0_DSP            0216h

%define MEMORY_MAP_TVRAM        041h
%define MEMORY_MAP_GVRAM        054h
%define GVRAM_CPU_WRITE_MODE    010h
%define TSP_VBLANK              040h
%define SGP_BUSY                001h

%define COMPOSE_G1_OVER_G0      0034h
%define RGB_COMPOSE_G1_OVER_G0  0089h
%define COMPOSE_G0_DIRECT       0008h
%define G0_SEGMENT              0a000h

%if NEON4_P5_BPP == 4
%define SCREEN_WIDTH            640
%define SCREEN_HEIGHT           400
%define BYTES_PER_LINE          320
%define WORDS_PER_LINE          160
%define PAGE_A_SGP              0200000h
%define PAGE_A_SGP_LOW          0000h
%define PAGE_A_SGP_HIGH         0020h
%define PAGE_B_SGP              021f400h
%define PAGE_B_SGP_LOW          0f400h
%define PAGE_B_SGP_HIGH         0021h
%define PAGE_A_DSA              000000h
%define PAGE_B_DSA              01f400h
%elif NEON4_P5_BPP == 16
%define SCREEN_WIDTH            320
%define SCREEN_HEIGHT           200
%define BYTES_PER_LINE          640
%define WORDS_PER_LINE          320
%define PAGE_A_SGP              0200000h
%define PAGE_A_SGP_LOW          0000h
%define PAGE_A_SGP_HIGH         0020h
%define PAGE_B_SGP              021f400h
%define PAGE_B_SGP_LOW          0f400h
%define PAGE_B_SGP_HIGH         0021h
%define PAGE_A_DSA              000000h
%define PAGE_B_DSA              01f400h
%else
%define SCREEN_WIDTH            320
%define SCREEN_HEIGHT           200
%define BYTES_PER_LINE          320
%define WORDS_PER_LINE          160
%define PAGE_A_SGP              0220000h
%define PAGE_A_SGP_LOW          0000h
%define PAGE_A_SGP_HIGH         0022h
%define PAGE_B_SGP              022fa00h
%define PAGE_B_SGP_LOW          0fa00h
%define PAGE_B_SGP_HIGH         0022h
%define PAGE_A_DSA              020000h
%define PAGE_B_DSA              02fa00h
%endif
%define SCREEN_WORDS            (WORDS_PER_LINE * SCREEN_HEIGHT)
%define PAGE_BYTES              (BYTES_PER_LINE * SCREEN_HEIGHT)

%define SGP_END                 0001h
%define SGP_SET_WORK            0003h
%define SGP_SET_COLOR           0006h
%define SGP_CLS                 000ah
%define SGP_LINE                0009h
%define SGP_LINE_COPY           0005h
; The real VA BLTMODE direction meanings are the same as the validated
; NEON3/GLASS payload: HD=0400h and VD=0800h.  VAEG's internal enum uses
; opposite names for its legacy line model; the build script can select that
; model explicitly for emulator-only comparison, but hardware is the default.
%ifndef NEON4_SGP_REAL_DIRECTION
%define NEON4_SGP_REAL_DIRECTION 1
%endif
%if NEON4_SGP_REAL_DIRECTION
%define SGP_LINE_HD             0400h
%define SGP_LINE_VD             0800h
%else
%define SGP_LINE_HD             0800h
%define SGP_LINE_VD             0400h
%endif

%define P4_LIST_WORDS           4096
%define P4_TEST_X0              040
%define P4_TEST_Y0              040
%define P4_TEST_X1              280
%define P4_TEST_Y1              160
%define P4_TEST_FILL            01ch
%define P4_TEST_EDGE            0e3h

; ---------------------------------------------------------------------------
; Entry and stage dispatch.
; ---------------------------------------------------------------------------
start:
        cli
        push    cs
        pop     ds
        push    cs
        pop     es
        ; Keep the loader-provided SS:SP until the video BIOS transaction has
        ; completed.  The VA BIOS uses the caller stack internally; the
        ; loader stack is the only stack contract verified by the existing
        ; payloads.  A private E000h stack makes INT 8Fh hang in VAEG before
        ; returning from the mode call.
        cld
        sti

%if NEON4_STAGE == 8
        ; Take ownership of the VA text console before entering graphics.
        ; Text BIOS function 2Fh removes the resident soft-key producer and
        ; Screen Editor function 01h hides the system-line guide.  TEXT stays
        ; enabled for the NEON4 overlay.
        call    neon4_text_console_init
%endif

%if NEON4_STAGE == 1
        ; N4-1: entry/stack/segment smoke.  No graphics or BIOS call.
        jmp     return_to_loader
%else
        call    va_video_enter
        jc      stage_failure
        cld
        push    cs
        pop     ds
        push    cs
        pop     es

%if NEON4_STAGE == 8
        ; Establish the text overlay once, including a complete TVRAM clear,
        ; before the first graphics frame is built.  Later updates never turn
        ; the display off, which avoids the visible text blink.
        call    neon4_text_activate_overlay
%endif

%if NEON4_STAGE == 2
        ; The text checkpoint is written only after the graphics BIOS has
        ; established a valid VA memory map.  Pre-mode TVRAM writes are not
        ; safe on the 8bpp BIOS path.
%if NEON4_DIAG_MARKER
        call    diag_write_text_marker
%endif
        mov     al, 024h
        call    cpu_clear_page
        jmp     wait_for_escape
%elif NEON4_STAGE == 3
        call    cpu_colour_bars
        jmp     wait_for_escape
%elif NEON4_STAGE == 4
        call    cpu_clear_page_zero
        xor     bx, bx
.vblank_loop:
        call    wait_vblank_edge
        jc      stage_failure
        inc     bx
        mov     al, bl
        and     al, 01fh
        call    cpu_marker_band
        call    keyboard_escape
        jnc     .vblank_loop
        jmp     leave_and_return
%elif NEON4_STAGE == 5
        call    set_display_page_a
        mov     al, 024h
        call    sgp_clear_page
        jc      stage_failure
        jmp     wait_for_escape
%elif NEON4_STAGE == 6
        ; N4-6: render both hidden G1 pages with SGP, then exchange FB1 DSA
        ; at a VBLANK edge.  This follows the SGP256 two-page sequence.
        mov     al, 01ch
        call    sgp_clear_page
        jc      stage_failure
        mov     al, 0e3h
        call    sgp_clear_page_b
        jc      stage_failure
        call    wait_vblank_edge
        jc      stage_failure
        call    set_display_page_b
        jmp     wait_for_escape
%elif NEON4_STAGE == 7
        ; P4: run the same deterministic primitive scene through either the
        ; CPU reference writer or the SGP command-list writer.
        call    p4_run_test_scene
        jc      stage_failure
        jmp     wait_for_escape
%elif NEON4_STAGE == 8
        ; P5: render the selected NEON4 scene through the shared geometry and
        ; the VA SGP span/line backend.
        call    p5_run_scene
        jc      leave_and_return
        jmp     wait_for_escape
%else
%error "NEON4_STAGE must be 1..8"
%endif
%endif

stage_failure:
        ; Keep the failure path free of Text BIOS calls.  INT 83h is only
        ; safe while a text composition is active; a partial Graphics BIOS
        ; setup may leave the ROM in G0-only or an undefined composition.
        ; The red page is therefore the only P3 failure marker.
        push    cs
        pop     ds
        mov     al, 0e0h
        call    cpu_clear_page
wait_for_escape:
        call    keyboard_escape
        jnc     wait_for_escape
leave_and_return:
        call    va_video_leave
%if NEON4_STAGE == 8
        call    neon4_text_console_restore
%endif
return_to_loader:
        cli
        cmp     word [cs:0e006h], 5034h
        jne     .halt
        mov     ax, [cs:0e000h]
        mov     ss, ax
        mov     sp, [cs:0e002h]
        push    word [cs:0e004h]
        popf
        retf
.halt:
        sti
.halt_loop:
        hlt
        jmp     .halt_loop

; ---------------------------------------------------------------------------
; VA graphics setup.  The mode call and FB1 setup follow the SGP256 payload.
; ---------------------------------------------------------------------------
va_video_enter:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    ds
        push    es
        mov     ax, VIDEO_DATA_SEG
        mov     ds, ax
        mov     es, ax
        mov     bx, NEON4_MODE
        mov     cx, NEON4_PIXEL_ARGS
        xor     dx, dx
        xor     ax, ax
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
%if (NEON4_P5_BPP == 4) || (NEON4_P5_BPP == 16)
        ; G0 owns the complete 256 KiB GVRAM window.  The 4bpp profile uses
        ; two 640x400 pages; the 16bpp profile uses two 320x200 pages.
%if NEON4_P5_BPP == 16
        ; Direct-colour mode needs the explicit G0 register setup used by the
        ; 16bpp VA profile.  The 4bpp path keeps the BIOS mode registers from
        ; the validated SGP wireframe sequence.
        mov     dx, PORT_GRMODE
        mov     ax, 0b462h
        out     dx, ax
        mov     dx, PORT_GRRES
        mov     ax, 1313h
        out     dx, ax
%endif
%if NEON4_P5_BPP == 4
        ; Match the validated 640x400 4bpp sequence: temporarily disable
        ; graphics output, select the palette control bank, then restore the
        ; graphics register bank before defining FB0 and its window.
        mov     ax, 0b00h
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
        mov     ax, 0900h
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
        mov     ax, 0a00h
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
%endif
        push    cs
        pop     es
        mov     di, neon4_framebuffer_descriptor
        mov     ax, 0100h
        mov     cx, 1
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
        push    cs
        pop     es
        mov     di, neon4_window_descriptor
        mov     ax, 0200h
        mov     cx, 1
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
%if NEON4_P5_BPP == 4
        ; $DefBuf/$DefWin populate the validated 640x400 FB0 register set.
        ; Do not overwrite it with the direct-colour register convention.
%else
        call    configure_g0_framebuffer
%endif
        call    set_display_page_a
%if NEON4_P5_BPP == 4
        call    neon4_set_low_palette
        jc      .failed
        mov     ax, 0300h              ; $Compose: G0 only, palette colour.
        mov     cx, 0003h
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
%else
        mov     dx, PORT_COL_COMP
        xor     ax, ax
        out     dx, ax
        mov     dx, PORT_RGB_COMP
        mov     ax, COMPOSE_G0_DIRECT
        out     dx, ax
%endif
%else
%if NEON4_USE_DEFAULT_BUFFERS
        ; The proven 320x200 VA payload leaves the BIOS-created descriptors
        ; untouched and verifies them before drawing.  Keep that path as the
        ; default while the 8bpp descriptor contract is unverified.
%else
        ; Define the framebuffer surface and display window explicitly only
        ; for a separately calibrated mode.  This path is not used by the
        ; default P3 diagnostic build.
        push    cs
        pop     es
        mov     di, neon4_framebuffer_descriptor
        mov     ax, 0100h              ; $DefBuf, graphics screen 0.
        mov     cx, 1
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
        push    cs
        pop     es
        mov     di, neon4_window_descriptor
        mov     ax, 0200h              ; $DefWin, graphics screen 0.
        mov     cx, 1
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
%endif
%endif
%if NEON4_P5_BPP == 8
        mov     ax, 0b00h
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
        mov     ax, 0900h
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
        mov     ax, 0a00h
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
        ; Select the direct RGB composition slots used by the 8bpp G1-over-G0
        ; path.  The low nibble is the highest-priority slot: G1 (9) over G0
        ; (8).  This is the same composition contract as SGP256.
        mov     dx, PORT_COL_COMP
        xor     ax, ax
        out     dx, ax
        mov     dx, PORT_RGB_COMP
        mov     ax, RGB_COMPOSE_G1_OVER_G0
        out     dx, ax
        mov     ax, 0300h
        mov     cx, COMPOSE_G1_OVER_G0
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
        ; Keep G0 opaque and G1 index zero transparent, as in SGP256.
        mov     dx, PORT_G0_TRANSPARENCY
        xor     ax, ax
        out     dx, ax
        mov     dx, PORT_G1_TRANSPARENCY
        mov     ax, 0001h
        out     dx, ax
%if NEON4_DIRECT_REGS
        ; Optional experimental G0-direct calibration path.
        call    configure_rgb332_mode
%else
        call    configure_g1_framebuffer
        call    set_display_page_a
%endif
%endif
        mov     ax, 0b01h             ; Enable graphics output after setup.
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
        mov     dx, PORT_MEMORY_MAP
        mov     al, MEMORY_MAP_GVRAM
        out     dx, al
        mov     dx, PORT_GVRAM_WRITE_MODE
        mov     al, GVRAM_CPU_WRITE_MODE
        out     dx, al
        clc
        jmp     .done
.failed:
        mov     [cs:video_error_code], ax
        stc
.done:
        pop     es
        pop     ds
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; Configure a 320x200 16bpp G0 backing surface with a 400-line source and a
; 200-line display window.  DSA0 selects one of the two 128 KiB pages.
configure_g0_framebuffer:
        push    ax
        push    dx
        mov     dx, PORT_FB0_FBW
        mov     ax, BYTES_PER_LINE
        out     dx, ax
        mov     dx, PORT_FB0_FBL
        mov     ax, SCREEN_HEIGHT * 2
        out     dx, ax
        mov     dx, PORT_FB0_DOT
        xor     ax, ax
        out     dx, ax
        mov     dx, PORT_FB0_OFX
        out     dx, ax
        mov     dx, PORT_FB0_OFY
        out     dx, ax
        mov     dx, PORT_FB0_DSA_LOW
        out     dx, ax
        mov     dx, PORT_FB0_DSA_HIGH
        out     dx, ax
        mov     dx, PORT_FB0_DSH
        mov     ax, SCREEN_HEIGHT
        out     dx, ax
        mov     dx, PORT_FB0_DSP
        xor     ax, ax
        out     dx, ax
        pop     dx
        pop     ax
        ret

; Configure the G1 8bpp backing surface used by the SGP256 path.  FB1 is a
; 320-byte-pitch, 400-line surface with a 200-line display window; DSA1
; selects the upper or lower page.
configure_g1_framebuffer:
        push    ax
        push    dx
        mov     dx, PORT_FB1_FBW
        mov     ax, BYTES_PER_LINE
        out     dx, ax
        mov     dx, PORT_FB1_DOT
        xor     ax, ax
        out     dx, ax
        mov     dx, PORT_FB1_OFX
        out     dx, ax
        mov     dx, PORT_FB1_OFY
        out     dx, ax
        mov     dx, PORT_FB1_DSH
        mov     ax, SCREEN_HEIGHT
        out     dx, ax
        mov     dx, PORT_FB1_DSP
        xor     ax, ax
        out     dx, ax
        pop     dx
        pop     ax
        ret

; [NOT VERIFIED ON VA SILICON] Optional direct G0 RGB332 calibration path.
; The default P3 path uses the BIOS 8bpp + FB1 contract above.
configure_rgb332_mode:
        push    ax
        push    dx
        mov     dx, 0102h
        mov     ax, 0012h             ; G0: 320 dots, 8bpp; G1 unused.
        out     dx, ax
        mov     dx, 0106h
        xor     ax, ax                ; Disable palette-composed screens.
        out     dx, ax
        mov     dx, 0108h
        mov     ax, 0008h             ; Enable direct-color G0.
        out     dx, ax
        mov     dx, 0204h             ; FB0 FBW = 320 bytes/row.
        mov     ax, BYTES_PER_LINE
        out     dx, ax
        mov     dx, 0206h             ; FB0 FBL = 200 source rows - 1.
        mov     ax, SCREEN_HEIGHT - 1
        out     dx, ax
        mov     dx, 0208h             ; DOT = 0.
        xor     ax, ax
        out     dx, ax
        mov     dx, 020ah             ; OFX = 0.
        out     dx, ax
        mov     dx, 020ch             ; OFY = 0.
        out     dx, ax
        mov     dx, 020eh             ; DSA = page A, low word.
        out     dx, ax
        mov     dx, 0210h
        out     dx, ax
        mov     dx, 0212h             ; DSH = 200 rows.
        mov     ax, SCREEN_HEIGHT
        out     dx, ax
        mov     dx, 0216h             ; DSP = top of output field.
        xor     ax, ax
        out     dx, ax
        pop     dx
        pop     ax
        ret

va_video_leave:
        push    ax
        push    cx
        push    dx
        push    ds
        push    es
        mov     ax, VIDEO_DATA_SEG
        mov     ds, ax
        mov     es, ax
        mov     ax, 0300h
        mov     cx, 0031h
        int     VIDEO_BIOS_INT
        mov     ax, 0b01h
        int     VIDEO_BIOS_INT
        mov     dx, PORT_MEMORY_MAP
        mov     al, MEMORY_MAP_TVRAM
        out     dx, al
        pop     es
        pop     ds
        pop     dx
        pop     cx
        pop     ax
        ret

; ---------------------------------------------------------------------------
; CPU direct packed 8bpp writes.  One word contains two consecutive bytes.
; ---------------------------------------------------------------------------
cpu_clear_page:
        push    ax
        push    cx
        push    di
        push    es
        xor     ah, ah
        mov     ah, al
        push    ax
        mov     dx, PORT_MEMORY_MAP
        mov     al, MEMORY_MAP_GVRAM
        out     dx, al
        mov     dx, PORT_GVRAM_WRITE_MODE
        mov     al, GVRAM_CPU_WRITE_MODE
        out     dx, al
        mov     dx, G0_SEGMENT
        mov     es, dx
        mov     di, 0
        mov     cx, SCREEN_WORDS
        pop     ax
        rep     stosw
        pop     es
        pop     di
        pop     cx
        pop     ax
        ret

cpu_clear_page_zero:
        xor     al, al
        jmp     cpu_clear_page

; Fill 32 ten-pixel columns with distinct direct RGB332 bytes.
cpu_colour_bars:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    es
        mov     dx, PORT_MEMORY_MAP
        mov     al, MEMORY_MAP_GVRAM
        out     dx, al
        mov     dx, PORT_GVRAM_WRITE_MODE
        mov     al, GVRAM_CPU_WRITE_MODE
        out     dx, al
        mov     ax, G0_SEGMENT
        mov     es, ax
        xor     di, di
        xor     bx, bx
        mov     cx, SCREEN_HEIGHT
.row:
        push    cx
        mov     si, neon4_bar_values
        mov     bx, 32
.bar:
        lodsb
        mov     ah, al
        mov     dx, 5
        mov     cx, dx
        rep     stosw
        dec     bx
        jnz     .bar
        pop     cx
        loop    .row
        pop     es
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; Draw a four-row marker band.  AL selects one of 32 direct RGB332 values.
cpu_marker_band:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    es
        xor     ah, ah
        mov     si, ax
        mov     al, [cs:neon4_bar_values + si]
        mov     ah, al
        mov     dx, G0_SEGMENT
        mov     es, dx
        mov     di, 0
        mov     bx, 4
.line:
        mov     cx, 160
        rep     stosw
        dec     bx
        jnz     .line
        pop     es
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; ---------------------------------------------------------------------------
; SGP command-list path.
; ---------------------------------------------------------------------------
sgp_clear_page:
        mov     dx, PAGE_A_SGP_LOW
        mov     bx, PAGE_A_SGP_HIGH
        jmp     sgp_clear_with_base

sgp_clear_page_b:
        mov     dx, PAGE_B_SGP_LOW
        mov     bx, PAGE_B_SGP_HIGH
sgp_clear_with_base:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    es
        mov     [cs:sgp_colour_byte], al
        mov     [cs:sgp_destination_low], dx
        mov     [cs:sgp_destination_high], bx
        mov     di, sgp_command_list
        push    cs
        pop     es
        mov     ax, SGP_SET_WORK
        stosw
        mov     si, sgp_work_area
        call    physical_address_from_ds_si
        stosw
        mov     ax, dx
        stosw
        mov     ax, SGP_SET_COLOR
        stosw
        mov     al, [cs:sgp_colour_byte]
        mov     ah, al
        stosw
        mov     ax, SGP_CLS
        stosw
        mov     ax, [cs:sgp_destination_low]
        stosw
        mov     ax, [cs:sgp_destination_high]
        stosw
        mov     ax, SCREEN_WORDS
        stosw
        xor     ax, ax
        stosw
        mov     ax, SGP_END
        stosw
        call    run_sgp_command_list
        pop     es
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

run_sgp_command_list:
        push    ax
        push    dx
        call    wait_sgp_idle
        jc      .failed
        ; The VA BIOS reasserts CPU-data GVRAM write mode before each SGP kick.
        ; Keep the shared mode latch in the documented state on real hardware.
        mov     dx, PORT_GVRAM_WRITE_MODE
        mov     al, GVRAM_CPU_WRITE_MODE
        out     dx, al
        mov     dx, PORT_SGP_CONTROL
        xor     al, al
        out     dx, al
        mov     si, sgp_command_list
        call    physical_address_from_ds_si
        mov     bx, dx
        mov     dx, PORT_SGP_COMMAND
        out     dx, ax
        add     dx, 2
        mov     ax, bx
        out     dx, ax
        mov     dx, PORT_SGP_STATUS
        mov     al, 1
        out     dx, al
        call    wait_sgp_idle
        pop     dx
        pop     ax
        clc
        ret
.failed:
        pop     dx
        pop     ax
        stc
        ret

wait_sgp_idle:
        push    ax
        push    cx
        push    dx
        mov     dx, PORT_SGP_STATUS
        mov     cx, 0ffffh
.poll:
        in      al, dx
        test    al, SGP_BUSY
        jz      .ready
        loop    .poll
        stc
        jmp     .done
.ready:
        clc
.done:
        pop     dx
        pop     cx
        pop     ax
        ret

physical_address_from_ds_si:
        mov     ax, ds
        xor     dx, dx
        mov     cx, 4
.shift:
        shl     ax, 1
        rcl     dx, 1
        loop    .shift
        add     ax, si
        adc     dx, 0
        ret

; FB1 DSA is a pair of word ports.  Keep the low/high writes separate, as in
; the SGP256 payload; byte writes can hang real VA hardware.
set_display_page_a:
%if (NEON4_P5_BPP == 4) || (NEON4_P5_BPP == 16)
        mov     dx, PORT_FB0_DSA_LOW
        mov     ax, PAGE_A_DSA & 0ffffh
        out     dx, ax
        add     dx, 2
        mov     ax, PAGE_A_DSA >> 16
        out     dx, ax
%else
        mov     dx, PORT_FB1_DSA_LOW
        mov     ax, PAGE_A_DSA & 0ffffh
        out     dx, ax
        add     dx, 2
        mov     ax, PAGE_A_DSA >> 16
        out     dx, ax
%endif
        ret

set_display_page_b:
%if (NEON4_P5_BPP == 4) || (NEON4_P5_BPP == 16)
        mov     dx, PORT_FB0_DSA_LOW
        mov     ax, PAGE_B_DSA & 0ffffh
        out     dx, ax
        add     dx, 2
        mov     ax, PAGE_B_DSA >> 16
        out     dx, ax
%else
        mov     dx, PORT_FB1_DSA_LOW
        mov     ax, PAGE_B_DSA & 0ffffh
        out     dx, ax
        add     dx, 2
        mov     ax, PAGE_B_DSA >> 16
        out     dx, ax
%endif
        ret

; ---------------------------------------------------------------------------
; P4 deterministic primitive backend.
; The test scene deliberately uses the same logical shape in both builds:
; the CPU build writes packed 8bpp G0 pixels, while the SGP build emits
; SET_COLOR/CLS/LINE commands for G1.  The visible composition is selected
; accordingly so the two outputs can be compared without sharing a writer.
; ---------------------------------------------------------------------------
p4_run_test_scene:
%if NEON4_P4_BACKEND == 0
        call    p4_cpu_scene
%else
        call    p4_sgp_scene
%endif
        clc
        ret

p4_set_cpu_composition:
        push    ax
        push    dx
        mov     dx, PORT_COL_COMP
        xor     ax, ax
        out     dx, ax
        mov     dx, PORT_RGB_COMP
        mov     ax, 0008h
        out     dx, ax
        pop     dx
        pop     ax
        ret

p4_set_sgp_composition:
%if NEON4_P5_BPP == 4
        ; 4bpp G0 is palette-composed by $Compose during mode setup.
        ret
%else
        push    ax
        push    dx
        mov     dx, PORT_COL_COMP
        xor     ax, ax
        out     dx, ax
        mov     dx, PORT_RGB_COMP
%if NEON4_P5_BPP == 16
        mov     ax, COMPOSE_G0_DIRECT
%else
        mov     ax, RGB_COMPOSE_G1_OVER_G0
%endif
        out     dx, ax
        pop     dx
        pop     ax
        ret
%endif

p4_cpu_scene:
        call    p4_set_cpu_composition
        xor     al, al
        call    cpu_clear_page
        mov     byte [p4_draw_color], P4_TEST_FILL
        call    p4_cpu_fill_rect
        mov     byte [p4_draw_color], P4_TEST_EDGE
        mov     ax, P4_TEST_X0
        mov     bx, P4_TEST_Y0
        mov     cx, P4_TEST_X1
        mov     dx, P4_TEST_Y0
        call    p4_cpu_line
        mov     ax, P4_TEST_X1
        mov     bx, P4_TEST_Y0
        mov     cx, P4_TEST_X1
        mov     dx, P4_TEST_Y1
        call    p4_cpu_line
        mov     ax, P4_TEST_X1
        mov     bx, P4_TEST_Y1
        mov     cx, P4_TEST_X0
        mov     dx, P4_TEST_Y1
        call    p4_cpu_line
        mov     ax, P4_TEST_X0
        mov     bx, P4_TEST_Y1
        mov     cx, P4_TEST_X0
        mov     dx, P4_TEST_Y0
        call    p4_cpu_line
        mov     ax, P4_TEST_X0
        mov     bx, P4_TEST_Y0
        mov     cx, P4_TEST_X1
        mov     dx, P4_TEST_Y1
        call    p4_cpu_line
        mov     ax, P4_TEST_X1
        mov     bx, P4_TEST_Y0
        mov     cx, P4_TEST_X0
        mov     dx, P4_TEST_Y1
        jmp     p4_cpu_line

p4_cpu_fill_rect:
        push    ax
        push    bx
        push    cx
        push    dx
        mov     word [p4_rect_x0], P4_TEST_X0
        mov     word [p4_rect_y0], P4_TEST_Y0
        mov     word [p4_rect_x1], P4_TEST_X1
        mov     word [p4_rect_y1], P4_TEST_Y1
        mov     dx, [p4_rect_y0]
.row:
        mov     cx, [p4_rect_x0]
.pixel:
        mov     al, [p4_draw_color]
        call    p4_cpu_put_pixel
        inc     cx
        cmp     cx, [p4_rect_x1]
        jle     .pixel
        inc     dx
        cmp     dx, [p4_rect_y1]
        jle     .row
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; AL = colour, CX = physical X, DX = physical Y.
p4_cpu_put_pixel:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    es
        mov     [p4_pixel_color], al
        mov     si, cx
        mov     ax, dx
        mov     bx, BYTES_PER_LINE
        mul     bx
        and     cx, 0fffeh
        add     ax, cx
        mov     di, ax
        mov     ax, G0_SEGMENT
        mov     es, ax
        mov     ax, [es:di]
        test    si, 1
        jnz     .odd
        mov     al, [p4_pixel_color]
        jmp     .store
.odd:
        mov     ah, [p4_pixel_color]
.store:
        mov     [es:di], ax
        pop     es
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; AX=x0, BX=y0, CX=x1, DX=y1.  The color is p4_draw_color.
; The software reference follows the SGP LINE accumulator convention rather
; than an unrelated Bresenham tie rule, so CPU and SGP paths have identical
; endpoint ownership for the P4 comparison scene.
p4_cpu_line:
        mov     [p4_line_x0], ax
        mov     [p4_line_y0], bx
        mov     [p4_line_x1], cx
        mov     [p4_line_y1], dx
        mov     ax, [p4_line_x1]
        sub     ax, [p4_line_x0]
        jns     .x_positive
        neg     ax
        mov     word [p4_line_sx], -1
        jmp     .x_delta
.x_positive:
        mov     word [p4_line_sx], 1
.x_delta:
        mov     [p4_line_dx], ax
        mov     ax, [p4_line_y1]
        sub     ax, [p4_line_y0]
        jns     .y_positive
        neg     ax
        mov     word [p4_line_sy], -1
        jmp     .y_delta
.y_positive:
        mov     word [p4_line_sy], 1
.y_delta:
        mov     [p4_line_dy], ax
        mov     ax, [p4_line_dx]
        cmp     ax, [p4_line_dy]
        jb      .y_major

        ; X-major: denominator=width-1, numerator=height-1, and the
        ; accumulator starts at (denominator-1)/2 as in cmd_line().
        mov     [p4_line_denominator], ax
        mov     ax, [p4_line_dy]
        mov     [p4_line_numerator], ax
        mov     ax, [p4_line_denominator]
        or      ax, ax
        jz      .single_point
        dec     ax
        shr     ax, 1
        mov     [p4_line_count], ax
        mov     ax, [p4_line_denominator]
        inc     ax
        mov     [p4_line_steps], ax
.x_loop:
        mov     al, [p4_draw_color]
        mov     cx, [p4_line_x0]
        mov     dx, [p4_line_y0]
        call    p4_cpu_put_pixel
        dec     word [p4_line_steps]
        jz      .done
        mov     ax, [p4_line_x0]
        add     ax, [p4_line_sx]
        mov     [p4_line_x0], ax
        mov     ax, [p4_line_count]
        add     ax, [p4_line_numerator]
        cmp     ax, [p4_line_denominator]
        jb      .x_no_y
        sub     ax, [p4_line_denominator]
        mov     bx, [p4_line_y0]
        add     bx, [p4_line_sy]
        mov     [p4_line_y0], bx
.x_no_y:
        mov     [p4_line_count], ax
        jmp     .x_loop

.y_major:
        ; Y-major: denominator=height-1, numerator=width-1, and the
        ; accumulator starts at denominator/2 as in cmd_line().
        mov     [p4_line_denominator], ax
        mov     ax, [p4_line_dy]
        mov     [p4_line_denominator], ax
        mov     ax, [p4_line_dx]
        mov     [p4_line_numerator], ax
        mov     ax, [p4_line_denominator]
        or      ax, ax
        jz      .single_point
        shr     ax, 1
        mov     [p4_line_count], ax
        mov     ax, [p4_line_denominator]
        inc     ax
        mov     [p4_line_steps], ax
.y_loop:
        mov     al, [p4_draw_color]
        mov     cx, [p4_line_x0]
        mov     dx, [p4_line_y0]
        call    p4_cpu_put_pixel
        dec     word [p4_line_steps]
        jz      .done
        mov     ax, [p4_line_y0]
        add     ax, [p4_line_sy]
        mov     [p4_line_y0], ax
        mov     ax, [p4_line_count]
        add     ax, [p4_line_numerator]
        cmp     ax, [p4_line_denominator]
        jb      .y_no_x
        sub     ax, [p4_line_denominator]
        mov     bx, [p4_line_x0]
        add     bx, [p4_line_sx]
        mov     [p4_line_x0], bx
.y_no_x:
        mov     [p4_line_count], ax
        jmp     .y_loop

.single_point:
        mov     al, [p4_draw_color]
        mov     cx, [p4_line_x0]
        mov     dx, [p4_line_y0]
        call    p4_cpu_put_pixel
.done:
        ret

p4_sgp_scene:
        call    p4_set_sgp_composition
        xor     al, al
        call    sgp_clear_page
        jc      .done
        call    p4_build_sgp_scene
        call    run_sgp_command_list
.done:
        ret

p4_build_sgp_scene:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    es
        push    cs
        pop     es
        mov     di, sgp_command_list
        mov     ax, SGP_SET_WORK
        stosw
        mov     si, sgp_work_area
        call    physical_address_from_ds_si
        stosw
        mov     ax, dx
        stosw
        mov     ax, P4_TEST_FILL
        call    p4_sgp_emit_set_color
        mov     bx, P4_TEST_Y0
.fill_row:
        mov     ax, P4_TEST_X0
        mov     cx, (P4_TEST_X1-P4_TEST_X0+1)/2
        call    p4_sgp_emit_cls_span
        inc     bx
        cmp     bx, P4_TEST_Y1
        jle     .fill_row
        mov     ax, P4_TEST_EDGE
        call    p4_sgp_emit_set_color
        mov     ax, P4_TEST_X0
        mov     bx, P4_TEST_Y0
        mov     cx, P4_TEST_X1
        mov     dx, P4_TEST_Y0
        call    p4_sgp_emit_line
        mov     ax, P4_TEST_X1
        mov     bx, P4_TEST_Y0
        mov     cx, P4_TEST_X1
        mov     dx, P4_TEST_Y1
        call    p4_sgp_emit_line
        mov     ax, P4_TEST_X1
        mov     bx, P4_TEST_Y1
        mov     cx, P4_TEST_X0
        mov     dx, P4_TEST_Y1
        call    p4_sgp_emit_line
        mov     ax, P4_TEST_X0
        mov     bx, P4_TEST_Y1
        mov     cx, P4_TEST_X0
        mov     dx, P4_TEST_Y0
        call    p4_sgp_emit_line
        mov     ax, P4_TEST_X0
        mov     bx, P4_TEST_Y0
        mov     cx, P4_TEST_X1
        mov     dx, P4_TEST_Y1
        call    p4_sgp_emit_line
        mov     ax, P4_TEST_X1
        mov     bx, P4_TEST_Y0
        mov     cx, P4_TEST_X0
        mov     dx, P4_TEST_Y1
        call    p4_sgp_emit_line
        mov     ax, SGP_END
        stosw
        pop     es
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

p4_sgp_emit_set_color:
        push    bx
        mov     ah, al
        mov     bx, ax
        mov     ax, SGP_SET_COLOR
        stosw
        mov     ax, bx
        stosw
        pop     bx
        ret

; AX=x, BX=y, CX=word count.  The span is physical and page-A based.
p4_sgp_emit_cls_span:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        mov     [p4_span_x], ax
        mov     [p4_span_y], bx
        mov     [p4_span_words], cx
        mov     ax, bx
        mov     bx, BYTES_PER_LINE
        mul     bx
        add     ax, [p4_span_x]
        adc     dx, 0
        add     ax, PAGE_A_SGP_LOW
        adc     dx, PAGE_A_SGP_HIGH
        mov     si, ax
        mov     ax, SGP_CLS
        stosw
        mov     ax, si
        stosw
        mov     ax, dx
        stosw
        mov     ax, [p4_span_words]
        stosw
        xor     ax, ax
        stosw
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; AX=x0, BX=y0, CX=x1, DX=y1.  Endpoints are physical pixels.
p4_sgp_emit_line:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        mov     [p4_line_x0], ax
        mov     [p4_line_y0], bx
        mov     [p4_line_x1], cx
        mov     [p4_line_y1], dx
        ; LINE copy mode is the documented 8bpp-compatible copy variant used
        ; by the NEON3 backend.  Direction bits are added below.
        mov     bx, SGP_LINE_COPY
        mov     ax, [p4_line_x1]
        sub     ax, [p4_line_x0]
        jns     .x_ok
        neg     ax
        or      bx, SGP_LINE_HD
.x_ok:
        inc     ax
        mov     [p4_line_dx], ax
        mov     ax, [p4_line_y1]
        sub     ax, [p4_line_y0]
        jns     .y_ok
        neg     ax
        or      bx, SGP_LINE_VD
.y_ok:
        inc     ax
        mov     [p4_line_dy], ax
        mov     ax, [p4_line_y0]
        mov     si, BYTES_PER_LINE
        mul     si
        mov     si, [p4_line_x0]
        and     si, 0fffeh
        add     ax, si
        adc     dx, 0
        add     ax, PAGE_A_SGP_LOW
        adc     dx, PAGE_A_SGP_HIGH
        mov     si, ax
        mov     [p4_line_address_high], dx
        mov     ax, SGP_LINE
        stosw
        mov     ax, bx
        stosw
        mov     ax, [p4_line_x0]
        and     ax, 1
        shl     ax, 4
        ; Descriptor mode 2 is packed 8bpp; bit 4 selects the odd pixel
        ; within the two-pixel word.
        or      ax, 2
        stosw
        mov     ax, [p4_line_dx]
        stosw
        mov     ax, [p4_line_dy]
        stosw
        mov     ax, BYTES_PER_LINE
        stosw
        mov     ax, si
        stosw
        mov     ax, [p4_line_address_high]
        stosw
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

wait_vblank_edge:
        push    ax
        push    cx
        push    dx
        mov     dx, PORT_TSP_STATUS
        mov     cx, 0ffffh
.low:
        in      al, dx
        test    al, TSP_VBLANK
        jz      .low_seen
        loop    .low
        stc
        jmp     .done
.low_seen:
        mov     cx, 0ffffh
.high:
        in      al, dx
        test    al, TSP_VBLANK
        jnz     .edge
        loop    .high
        stc
        jmp     .done
.edge:
        clc
.done:
        pop     dx
        pop     cx
        pop     ax
        ret

keyboard_escape:
        ; Use the VA keyboard contract exercised by the existing SGP demo:
        ; AH=0Ah tests for a pending key and AH=09h consumes it.  AH=00h is
        ; the returned scan code for ESC.  The older AH=01h/AH=00h pair is
        ; not a non-blocking poll on the VA2 BIOS and can leave this stage
        ; waiting forever.
        mov     ah, 0ah
        int     KEYBOARD_BIOS_INT
        jc      .none
        mov     ah, 09h
        int     KEYBOARD_BIOS_INT
        cmp     ah, 00h
        jne     .none
        stc
        ret
.none:
        clc
        ret

%if NEON4_STAGE == 8
; Remove the inherited VA soft-key/function-key guide without disabling TEXT.
; INT 83h/AH=2Fh with AL=0 requests zero soft-key entries, while INT 94h
; (Screen Editor)/AH=01h with AL=FF hides the reserved system line.
neon4_text_console_init:
        pusha
        push    ds
        push    es
        mov     ah, 2fh
        xor     al, al
        int     TEXT_BIOS_INT
        mov     ah, 01h
        mov     al, 0ffh
        int     SCREEN_EDITOR_BIOS_INT
        mov     ah, 25h                ; Text BIOS: cursor display control
        xor     al, al                 ; hide cursor and cursor blink
        int     TEXT_BIOS_INT
        pop     es
        pop     ds
        popa
        ret

; Restore the caller's normal ten-entry soft-key/system-line guide on exit.
neon4_text_console_restore:
        pusha
        push    ds
        push    es
        mov     ah, 2fh
        mov     al, 0ah
        int     TEXT_BIOS_INT
        mov     ah, 01h
        mov     al, 0ah
        int     SCREEN_EDITOR_BIOS_INT
        mov     ah, 25h                ; restore the normal blinking cursor
        mov     al, 03h
        int     TEXT_BIOS_INT
        pop     es
        pop     ds
        popa
        ret

; Activate the live overlay once after graphics mode setup.  The complete
; TVRAM clear happens while the text-only composition is hidden; subsequent
; frame updates use the same Text BIOS writes without toggling ScnDsp.
neon4_text_activate_overlay:
        pusha
        push    ds
        push    es
        xor     ax, ax
        mov     ds, ax
        mov     es, ax
        mov     ax, 0b00h              ; $ScnDsp: display off
        int     VIDEO_BIOS_INT
        mov     ax, 0338h
        mov     ds, ax
        mov     es, ax
        mov     ax, 0300h              ; $Compose: text only
        mov     cx, 0001h
        int     VIDEO_BIOS_INT
        push    cs
        pop     ds
        push    cs
        pop     es
        mov     dx, PORT_MEMORY_MAP
        mov     al, MEMORY_MAP_TVRAM
        out     dx, al
        call    neon4_text_clear_console
        call    neon4_text_write_static_overlay
        call    neon4_text_update_overlay
        ; The first frame selects its scene after this bootstrap update.  Keep
        ; the title marked dirty so that scene 0 is written once at that
        ; first VBLANK, without repainting it on later frames.
        mov     byte [neon4_text_last_scene], 0ffh

        mov     dx, PORT_MEMORY_MAP
        mov     al, MEMORY_MAP_GVRAM
        out     dx, al
        mov     ax, 0338h
        mov     ds, ax
        mov     es, ax
        mov     ax, 0300h              ; $Compose: text above G0
        mov     cx, 0031h
        int     VIDEO_BIOS_INT
        mov     ax, 0338h
        mov     ds, ax
        mov     es, ax
        mov     ax, 0b01h              ; $ScnDsp: display on
        int     VIDEO_BIOS_INT
        mov     dx, PORT_MEMORY_MAP
        mov     al, MEMORY_MAP_GVRAM
        out     dx, al
        pop     es
        pop     ds
        popa
        ret

; Draw fields that never change during a run.  They are written once after
; the complete TVRAM clear so frame updates do not repaint the whole overlay.
neon4_text_write_static_overlay:
        pusha
        push    ds
        push    es
        push    cs
        pop     ds
        mov     dx, PORT_MEMORY_MAP
        mov     al, MEMORY_MAP_TVRAM
        out     dx, al

        mov     si, neon4_text_live_title
        xor     dx, dx
        call    neon4_text_puts_at
        mov     si, neon4_text_profile
        mov     dh, 1
        xor     dl, dl
        call    neon4_text_puts_at
        mov     si, neon4_text_frame
        mov     dh, 2
        xor     dl, dl
        call    neon4_text_puts_at
        mov     si, neon4_text_scene
        mov     dh, 3
        xor     dl, dl
        call    neon4_text_puts_at
        mov     si, neon4_text_local
        mov     dh, 4
        xor     dl, dl
        call    neon4_text_puts_at
        mov     si, neon4_text_limit
        mov     dh, 5
        xor     dl, dl
        call    neon4_text_puts_at
        mov     si, neon4_text_exit
        mov     dh, 6
        xor     dl, dl
        call    neon4_text_puts_at

        mov     dx, PORT_MEMORY_MAP
        mov     al, MEMORY_MAP_GVRAM
        out     dx, al
        pop     es
        pop     ds
        popa
        ret

; Update only the fixed-width live values.  The scene title row is cleared and
; rewritten only when the scene changes, preventing a visible repaint on every
; frame.
neon4_text_update_overlay:
        pusha
        push    ds
        push    es
        push    cs
        pop     ds
        mov     dx, PORT_MEMORY_MAP
        mov     al, MEMORY_MAP_TVRAM
        out     dx, al

        mov     ax, [render_frame_counter]
        mov     dh, 2
        call    neon4_text_hex_value_at
        mov     al, [scene_index]
        cmp     al, [neon4_text_last_scene]
        je      .scene_ready
        mov     [neon4_text_last_scene], al
        mov     si, neon4_text_blank_line
        mov     dh, 3
        xor     dl, dl
        call    neon4_text_puts_at
        mov     si, neon4_text_scene
        mov     dh, 3
        xor     dl, dl
        call    neon4_text_puts_at
        xor     bx, bx
        mov     bl, [scene_index]
        cmp     bl, NEON4_TEXT_SCENE_COUNT
        jae     .scene_ready
        shl     bx, 1
        mov     si, [scene_title_ptrs + bx]
        mov     dh, 3
        mov     dl, 18
        call    neon4_text_puts_at
.scene_ready:
        mov     ax, [scene_frame]
        mov     dh, 4
        call    neon4_text_hex_value_at

        mov     dx, PORT_MEMORY_MAP
        mov     al, MEMORY_MAP_GVRAM
        out     dx, al
        pop     es
        pop     ds
        popa
        ret

; Clear the complete TVRAM text surface once before the first overlay.  This
; removes stale caller/loader text while the display is still hidden; it is
; never repeated during frame updates.
neon4_text_clear_console:
        push    ax
        push    bx
        push    dx
        push    si
        xor     bx, bx
.row:
        mov     dh, bl
        xor     dl, dl
        mov     si, neon4_text_blank_line
        call    neon4_text_puts_at
        inc     bl
        cmp     bl, 25
        jb      .row
        pop     si
        pop     dx
        pop     bx
        pop     ax
        ret

; DS:SI = NUL-terminated string, DH = row, DL = column.
neon4_text_puts_at:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    ds
        push    es
        mov     bh, dh
        mov     bl, dl
        push    cs
        pop     ds
        push    cs
        pop     es
        cld
        mov     ah, 08h                ; set text cursor, DH=X and DL=Y
        mov     dh, bl
        mov     dl, bh
        push    si
        int     TEXT_BIOS_INT
        pop     si
        push    cs
        pop     ds
        push    cs
        pop     es
        mov     ah, 02h                ; write ASCIZ with current attribute
        mov     dx, 8000h
        push    si
        int     TEXT_BIOS_INT
        pop     si
        pop     es
        pop     ds
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; DS:SI = label, AX = value, DH = row.  Place the fixed-width value at col 24.
neon4_text_hex_at:
        push    ax
        push    dx
        push    si
        xor     dl, dl
        call    neon4_text_puts_at
        pop     si
        pop     dx
        pop     ax
        call    neon4_text_make_hex
        mov     si, neon4_text_hex_buffer
        mov     dl, 24
        call    neon4_text_puts_at
        ret

; AX = value, DH = row.  Write only the fixed-width value field at column 24.
neon4_text_hex_value_at:
        push    ax
        push    dx
        call    neon4_text_make_hex
        mov     si, neon4_text_hex_buffer
        mov     dl, 24
        call    neon4_text_puts_at
        pop     dx
        pop     ax
        ret

neon4_text_make_hex:
        push    ax
        push    bx
        push    cx
        push    dx
        push    di
        mov     bx, ax
        mov     di, neon4_text_hex_buffer
        mov     cx, 4
.digit:
        rol     bx, 4
        mov     dx, bx
        and     dx, 0fh
        mov     si, dx
        mov     al, [neon4_text_hex_digits + si]
        stosb
        loop    .digit
        xor     al, al
        stosb
        pop     di
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

%endif

; Put a checkpoint on the existing text plane before the first 8bpp mode
; transaction.  The text BIOS is not safe before a graphics mode has been
; established on the tested ROMs, so this diagnostic marker writes the known
; VA TVRAM character/attribute planes directly.  It is not a production text
; path and does not touch GVRAM.
diag_write_text_marker:
        push    ax
        push    bx
        push    dx
        push    si
        push    di
        push    ds
        push    es
        mov     dx, PORT_MEMORY_MAP
        mov     al, MEMORY_MAP_TVRAM
        out     dx, al
        push    cs
        pop     ds
        mov     ax, G0_SEGMENT
        mov     es, ax
        mov     si, diag_pre_mode_text
        mov     di, 0a00h             ; row 10, column 0 in the text plane
        mov     bx, 8a00h             ; matching attribute bytes
.character:
        lodsb
        test    al, al
        jz      .done
        xor     ah, ah
        stosw
        mov     byte [es:bx], 07h
        inc     bx
        jmp     .character
.done:
        ; Restore the graphics memory map before returning to the video setup.
        ; Leaving TVRAM selected would make the next stage depend on a
        ; diagnostic-only bank selection.
        mov     dx, PORT_MEMORY_MAP
        mov     al, MEMORY_MAP_GVRAM
        out     dx, al
        pop     es
        pop     ds
        pop     di
        pop     si
        pop     dx
        pop     bx
        pop     ax
        ret

align 2
sgp_command_list:
        times P4_LIST_WORDS dw 0
sgp_work_area:
        times 29 dw 0
sgp_destination_low dw 0
sgp_destination_high dw 0
sgp_colour_byte db 0
        align 2

; P4 diagnostic state.  These are scratch words only; the P4 path is entered
; after the normal BIOS mode setup and does not share state with the P3 probes.
p4_draw_color db 0
p4_pixel_color db 0
        align 2
p4_rect_x0 dw 0
p4_rect_y0 dw 0
p4_rect_x1 dw 0
p4_rect_y1 dw 0
p4_line_x0 dw 0
p4_line_y0 dw 0
p4_line_x1 dw 0
p4_line_y1 dw 0
p4_line_sx dw 0
p4_line_sy dw 0
p4_line_dx dw 0
p4_line_dy dw 0
p4_line_err dw 0
p4_line_count dw 0
p4_line_denominator dw 0
p4_line_numerator dw 0
p4_line_steps dw 0
p4_line_address_high dw 0
p4_span_x dw 0
p4_span_y dw 0
p4_span_words dw 0
        align 2

neon4_bar_values:
        db 00h, 20h, 40h, 60h, 80h, 0a0h, 0c0h, 0e0h
        db 04h, 24h, 44h, 64h, 84h, 0a4h, 0c4h, 0e4h
        db 10h, 30h, 50h, 70h, 90h, 0b0h, 0d0h, 0f0h
        db 1ch, 3ch, 5ch, 7ch, 9ch, 0bch, 0dch, 0fch

align 16
; $DefBuf descriptor: pixel size, width, height.
neon4_framebuffer_descriptor:
%if NEON4_P5_BPP == 4
        dw 4, SCREEN_WIDTH, SCREEN_HEIGHT * 2
%elif NEON4_P5_BPP == 16
        dw 16, SCREEN_WIDTH, SCREEN_HEIGHT * 2
%else
        dw 8, SCREEN_WIDTH, SCREEN_HEIGHT
%endif
align 16
; $DefWin descriptor: framebuffer number, screen Y, height, source X, Y.
neon4_window_descriptor:
        dw 0, 0, SCREEN_HEIGHT, 0, 0

diag_pre_mode_text db 'N4 ENTER OK', 0
%if NEON4_STAGE == 8
neon4_text_live_title db 'NEON4 // LIVE FRAME STATUS', 0
neon4_text_frame db 'FRAME (HEX):', 0
neon4_text_local db 'LOCAL FRAME (HEX):', 0
neon4_text_scene db 'SCENE TITLE:', 0
neon4_text_limit db 'TOTAL FRAMES (HEX):', 0
%if NEON4_P5_BPP == 4
neon4_text_profile db 'PROFILE: 640X400 / G0 / 4BPP', 0
%elif NEON4_P5_BPP == 16
neon4_text_profile db 'PROFILE: 320X200 / G0 / 16BPP', 0
%else
neon4_text_profile db 'PROFILE: 320X200 / G1 / 8BPP', 0
%endif
neon4_text_exit db 'ESC: PRESS ESC TO EXIT', 0
neon4_text_blank_line times 80 db ' '
                         db 0
neon4_text_hex_buffer times 5 db 0
neon4_text_hex_digits db '0123456789ABCDEF'
neon4_text_last_scene db 0ffh
%endif
video_error_code dw 0

; These words are intentionally reserved for the P3 command builder.  The
; loader writes its return continuation at CS:0e000..0e008.
%if ($ - $$) >= 0e000h
%error "NEON4 P3 payload overlaps the loader return reserve"
%endif

%if NEON4_STAGE == 8
; ---------------------------------------------------------------------------
; P5-1 scene integration.
; ---------------------------------------------------------------------------
; The original geometry remains in logical 640x400 coordinates.  These
; primitive entry points are the only place where it is reduced to the
; selected 320x200 packed/direct surface.  A span is represented by inclusive
; logical endpoints; the 8bpp SGP path rounds only the storage transaction to
; an even byte address, while the 16bpp path uses one word per pixel.
;
; The original low-colour geometry uses DI as a private per-helper flag.  The
; SGP list cursor therefore lives in p5_list_offset rather than DI; otherwise
; n4_story_raster_panel would redirect command words into the payload.

%include "config4_286.inc"
%include "data4_p0.inc"
%include "low4_data.inc"
%include "geom4_low.inc"
%include "scene4_256.inc"
%include "neon4_va_palette.inc"

; Load the original NEON4 low-colour hue/shade table into the VA's sixteen
; 12-bit palette entries.  The source bytes are G,R,B nibbles.  The VA
; palette word follows the documented 4096-colour layout: G[15:12],
; R[9:6], and B[4:1].
neon4_set_low_palette:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    ds
        push    cs
        pop     ds
        xor     bx, bx
        mov     si, low4_palette_grb
.entry:
        xor     ax, ax
        mov     al, [si]
        mov     cl, 12
        shl     ax, cl
        mov     dx, ax
        xor     ax, ax
        mov     al, [si + 1]
        mov     cl, 6
        shl     ax, cl
        or      dx, ax
        xor     ax, ax
        mov     al, [si + 2]
        shl     ax, 1
        or      dx, ax
        mov     cx, dx
        mov     ax, 0800h
        mov     al, bl
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
        push    cs
        pop     ds
        inc     bl
        add     si, 3
        cmp     bl, 16
        jb      .entry
        clc
        jmp     .done
.failed:
        stc
.done:
        pop     ds
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; config4_256.inc describes the original planar 16-colour surface.  The VA P5
; backend keeps the scene's logical SCREEN_W/SCREEN_H definitions, while the
; storage pitch follows the selected packed/direct colour mode.  The source
; include defines its own 80-byte planar pitch, so replace it explicitly here.
%undef BYTES_PER_LINE
%if NEON4_P5_BPP == 16
%define BYTES_PER_LINE 640
%else
%define BYTES_PER_LINE 320
%endif

p5_run_scene:
        call    p4_set_sgp_composition
        ; The direct-colour backend writes its RGB composition register above;
        ; reassert the VA text-above-G0 composition once before rendering.
        ; This is deliberately outside the frame loop, so it cannot blink.
        call    neon4_text_show_overlay
        ; Build on the hidden page and expose it only after the complete SGP
        ; batch has finished.  The page/descriptor path is selected by BPP.
        call    set_display_page_a
        mov     byte [p5_draw_page], 1
        call    p5_set_draw_page
        xor     al, al
        call    sgp_clear_page
        jc      .failed
        call    sgp_clear_page_b
        jc      .failed
%if NEON4_P5_BPP == 4
        mov     byte [video_400_mode], 1
%else
        mov     byte [video_400_mode], 0
%endif
        mov     byte [low_egc_available], 0
        mov     byte [low_dirty_span_enable], 0
        mov     byte [egc16_saved_page], 0
        mov     word [frame_counter], 0

.frame:
        call    wait_vblank_edge
        jc      .failed
        ; Update the live text fields immediately after the VBLANK edge.  The
        ; BIOS writes are grouped before the hidden graphics page is built,
        ; rather than landing after page presentation during active scanout.
        mov     ax, [frame_counter]
        mov     [render_frame_counter], ax
        call    select_scene
        call    neon4_text_update_overlay
        xor     al, al
        call    p5_clear_draw_page
        jc      .failed
        call    p5_start_batch
        call    render_scene
        call    p5_finish_batch
        jc      .failed
        call    p5_flip_draw_page
        jc      .failed
        call    keyboard_escape
        jc      .exit
        inc     word [frame_counter]
        cmp     word [frame_counter], TOTAL_FRAMES
        jb      .frame
        ; Both published profiles are continuously looping demos.  Keep the
        ; finite timeline for one pass, then restart it without leaving the
        ; payload; ESC remains the only normal exit.
        mov     word [frame_counter], 0
        jmp     .frame
.exit:
        stc
        ret
.failed:
        stc
        ret

; Reassert TEXT above G0 after a profile-specific direct-colour composition
; setup.  Unlike the old per-frame transition this does not blank the display.
neon4_text_show_overlay:
        pusha
        push    ds
        push    es
        mov     ax, 0338h
        mov     ds, ax
        mov     es, ax
        mov     ax, 0300h              ; $Compose: text above G0
        mov     cx, 0031h
        int     VIDEO_BIOS_INT
        mov     ax, 0338h
        mov     ds, ax
        mov     es, ax
        mov     ax, 0b01h              ; $ScnDsp: display on
        int     VIDEO_BIOS_INT
        pop     es
        pop     ds
        popa
        ret

; The source low-colour renderer treats this as a batch boundary.  SGP list
; submission is deliberately deferred until the list is full or the scene
; returns, so the geometry remains independent of the backend.
line_batch_begin:
        ret

set_access_page:
        ret

clear_graphics_frame16:
        xor     al, al
        jmp     sgp_clear_page

text_update:
low_dirty_span_frame_end:
low_dirty_span_record_rect:
low_raster_track_rect16:
grcg16_prepare_color:
egc16_enable_vram_copy:
egc16_disable_to_grcg:
        ret

pixel_set:
        ; SGP has no pixel opcode.  A one-pixel span is represented by the
        ; same general span writer; exact endpoint treatment is P5-2.
        call    p5_emit_span_from_ax_bx_cx
        ret

hline_set:
hline_set_fast:
hline_set_same_colour_fast:
hline_set16_same_fast:
hline_set_same_colour:
        call    p5_emit_span_from_ax_bx_cx
        ret

line_set:
line_set_same_colour:
        call    p5_emit_line_from_ax_bx_cx_dx
        ret

fill_rect:
        push    ax
        push    bx
        push    cx
        push    dx
        mov     [p5_rect_x0], ax
        mov     [p5_rect_y0], bx
        mov     [p5_rect_x1], cx
        mov     [p5_rect_y1], dx
        mov     bx, [p5_rect_y0]
.row:
        mov     ax, [p5_rect_x0]
        mov     cx, [p5_rect_x1]
        call    p5_emit_span_from_ax_bx_cx
        inc     bx
        cmp     bx, [p5_rect_y1]
        jle     .row
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

hline_set16_same:
        jmp     p5_emit_span_from_ax_bx_cx

; AX=x0, BX=y, CX=x1 in logical coordinates.  The geometry uses inclusive
; endpoints.  The 640x400 4bpp profile is mapped one-to-one; reduced
; profiles use floor(x/2), floor(y/2) at this primitive boundary.
p5_emit_span_from_ax_bx_cx:
        push    ax
        push    bx
        push    cx
        push    dx
        cmp     ax, cx
        jle     .ordered
        xchg    ax, cx
.ordered:
        cmp     cx, 0
        jl      .done
        cmp     ax, SCREEN_W-1
        jg      .done
        cmp     ax, 0
        jge     .x0_ok
        xor     ax, ax
.x0_ok:
        cmp     cx, SCREEN_W-1
        jle     .x1_ok
        mov     cx, SCREEN_W-1
.x1_ok:
%if NEON4_P5_BPP != 4
        sar     ax, 1
        sar     cx, 1
        sar     bx, 1
%endif
        call    p5_emit_span_physical
.done:
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; AX=physical x0, BX=physical y, CX=physical x1, inclusive.
p5_emit_span_physical:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        ; Preserve the caller's table/iterator pointer.  NEON4's CAT and
        ; raster span walkers keep their source cursor in DI; the SGP list
        ; encoder also uses DI internally.  The backend must not leak that
        ; implementation detail into the geometry layer.
        push    di
        cmp     ax, cx
        jle     .ordered
        xchg    ax, cx
.ordered:
%if NEON4_P5_BPP == 4
        ; Packed 4bpp has four logical pixels per word.  Write every word
        ; touched by this exact span through the same CPU RMW operation.  A
        ; partial endpoint must not be painted in a separate pass and a full
        ; interior word must not arrive later through SGP: that ordering made
        ; a four-pixel hole visible before the endpoint correction.  The
        ; complete span is now one general exact-write operation; SGP remains
        ; responsible for clear and line primitives.
        mov     [p5_span_x0], ax
        mov     [p5_span_y], bx
        mov     [p5_span_x1], cx
        call    p5_write_span_cpu_exact
        jmp     .span_done
%elif NEON4_P5_BPP == 16
        ; 16bpp pixels are one SGP word each; every physical X is already
        ; word-aligned after converting it to a byte address.
        sub     cx, ax
        inc     cx
        shl     ax, 1
        xor     dx, dx
%else
        ; CLS addresses are word-oriented in the packed 8bpp descriptor.  The
        ; first byte is aligned down and the count covers the resulting words.
        mov     dx, ax
        and     dx, 1
        and     ax, 0fffeh
        sub     cx, ax
        inc     cx
        inc     cx
        shr     cx, 1
%endif
        mov     di, [p5_list_offset]
        call    p5_emit_color_if_needed
        cmp     di, sgp_command_list + ((P4_LIST_WORDS-16)*2)
        jb      .space_ready
        call    p5_flush_batch
.space_ready:
        mov     si, ax
        mov     ax, bx
        mov     bx, BYTES_PER_LINE
        mul     bx
        add     ax, si
        adc     dx, 0
        add     ax, [p5_draw_sgp_low]
        adc     dx, [p5_draw_sgp_high]
        mov     si, ax
.emit_cls_words:
        mov     ax, SGP_CLS
        stosw
        mov     ax, si
        stosw
        mov     ax, dx
        stosw
        mov     ax, cx
        stosw
        xor     ax, ax
        stosw
        mov     [p5_list_offset], di
        jmp     .span_done

.span_done:
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; Write every packed word touched by the current exact span through CPU
; read-modify-write.  The first and last words use partial masks when needed;
; words wholly covered by the span use the same operation with all four pixel
; nibbles selected.  Thus no endpoint word is written in a later correction
; pass and no SGP command can expose a four-pixel intermediate hole.
p5_write_span_cpu_exact:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    bp
        push    es
        mov     ax, [p5_span_x0]
        shr     ax, 2
        mov     [p5_span_first_word], ax
        mov     si, ax
        mov     ax, [p5_span_x1]
        shr     ax, 2
        mov     [p5_span_last_word], ax
.word:
        mov     [p5_word_index], si
        mov     ax, si
        shl     ax, 2
        mov     [p5_word_low], ax
        add     ax, 3
        mov     [p5_word_high], ax
        cmp     si, [p5_span_first_word]
        jne     .left_ready
        mov     ax, [p5_span_x0]
        mov     [p5_word_low], ax
.left_ready:
        cmp     si, [p5_span_last_word]
        jne     .right_ready
        mov     ax, [p5_span_x1]
        mov     [p5_word_high], ax
.right_ready:
        call    p5_rmw_word
        cmp     si, [p5_span_last_word]
        jae     .done
        inc     si
        jmp     .word
.done:
        pop     es
        pop     bp
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; Read-modify-write one packed 4bpp word for the inclusive pixel range held
; in p5_word_low/high.  The x%4 ordering is the independently calibrated
; FB0 mapping used by the GLASS and NEON3 VA backends.  The CPU aperture
; segment follows the selected hidden page; page B is offset by 1f400h.  A
; 640x400 packed page is 128 KiB, so a single 16-bit offset cannot cover it:
; the upper half uses the next 4 KiB paragraph while the low offset wraps.
p5_rmw_word:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    bp
        push    es
        xor     dx, dx
        xor     bx, bx
        mov     al, [draw_color]
        and     al, 0fh
        xor     ah, ah
        mov     si, ax
        shl     ax, 4
        or      ax, si
        mov     si, ax
        shl     ax, 8
        or      si, ax
        mov     bp, [p5_word_low]
.pixel:
        mov     ax, bp
        and     ax, 3
        shl     ax, 1
        mov     di, ax
        mov     cx, [p5_packed_masks + di]
        or      dx, cx
        mov     cx, si
        and     cx, [p5_packed_masks + di]
        or      bx, cx
        inc     bp
        cmp     bp, [p5_word_high]
        jbe     .pixel
        mov     [p5_word_mask], dx
        mov     [p5_word_value], bx
        mov     ax, [p5_span_y]
        mov     cx, BYTES_PER_LINE
        mul     cx
        mov     cx, [p5_word_index]
        shl     cx, 1
        add     ax, cx
        adc     dx, 0
        mov     di, ax
        mov     ax, [p5_draw_cpu_segment]
        or      dx, dx
        jz      .segment_ready
        add     ax, 1000h
.segment_ready:
        mov     es, ax
        mov     ax, [es:di]
        mov     cx, [p5_word_mask]
        not     cx
        and     ax, cx
        or      ax, [p5_word_value]
        mov     [es:di], ax
        pop     es
        pop     bp
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; AX=x0, BX=y0, CX=x1, DX=y1 in logical coordinates.
p5_emit_line_from_ax_bx_cx_dx:
        push    ax
        push    bx
        push    cx
        push    dx
        cmp     ax, 0
        jl      .done
        cmp     cx, 0
        jl      .done
        cmp     ax, SCREEN_W-1
        jg      .done
        cmp     cx, SCREEN_W-1
        jg      .done
        cmp     bx, 0
        jl      .done
        cmp     dx, 0
        jl      .done
        cmp     bx, SCREEN_H-1
        jg      .done
        cmp     dx, SCREEN_H-1
        jg      .done
%if NEON4_P5_BPP != 4
        sar     ax, 1
        sar     cx, 1
        sar     bx, 1
        sar     dx, 1
%endif
        call    p5_emit_line_physical
.done:
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; AX=x0, BX=y0, CX=x1, DX=y1 in physical pixels.
p5_emit_line_physical:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    bp
        ; Preserve the caller's iterator pointer.  Line emission uses DI as
        ; the command-list cursor, but scene walkers may keep their data
        ; cursor there across successive primitives.
        push    di
        mov     [p5_line_x0], ax
        mov     [p5_line_y0], bx
        mov     [p5_line_x1], cx
        mov     [p5_line_y1], dx
        mov     bp, SGP_LINE_COPY
        mov     ax, cx
        sub     ax, [p5_line_x0]
        jns     .x_positive
        neg     ax
        or      bp, SGP_LINE_HD
.x_positive:
        inc     ax
        mov     [p5_line_width], ax
        mov     ax, dx
        sub     ax, [p5_line_y0]
        jns     .y_positive
        neg     ax
        or      bp, SGP_LINE_VD
.y_positive:
        inc     ax
        mov     [p5_line_height], ax
        mov     di, [p5_list_offset]
        call    p5_emit_color_if_needed
        cmp     di, sgp_command_list + ((P4_LIST_WORDS-20)*2)
        jb      .space_ready
        call    p5_flush_batch
.space_ready:
        mov     ax, [p5_line_y0]
        mov     bx, BYTES_PER_LINE
        mul     bx
        mov     bx, [p5_line_x0]
%if NEON4_P5_BPP == 4
        and     bx, 0fffch
        shr     bx, 1
%elif NEON4_P5_BPP == 16
        shl     bx, 1
%else
        and     bx, 0fffeh
%endif
        add     ax, bx
        adc     dx, 0
        add     ax, [p5_draw_sgp_low]
        adc     dx, [p5_draw_sgp_high]
        mov     si, ax
        mov     ax, SGP_LINE
        stosw
        mov     ax, bp
        stosw
%if NEON4_P5_BPP == 4
        mov     ax, [p5_line_x0]
        and     ax, 3
        shl     ax, 4
        or      ax, 1
%elif NEON4_P5_BPP == 16
        mov     ax, 3
%else
        mov     ax, [p5_line_x0]
        and     ax, 1
        shl     ax, 4
        or      ax, 2
%endif
        stosw
        mov     ax, [p5_line_width]
        stosw
        mov     ax, [p5_line_height]
        stosw
        mov     ax, BYTES_PER_LINE
        stosw
        mov     ax, si
        stosw
        mov     ax, dx
        stosw
        mov     [p5_list_offset], di
        pop     di
        pop     bp
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

p5_emit_color_if_needed:
        push    ax
        push    bx
        mov     di, [p5_list_offset]
        xor     bx, bx
        mov     bl, [draw_color]
%if NEON4_P5_BPP == 4
        and     bx, 000fh
        mov     ax, bx
        shl     ax, 4
        or      ax, bx
        mov     bx, ax
        shl     bx, 8
        or      ax, bx
%else
%if NEON4_P5_BPP == 16
        ; The direct-colour profile indexes the original PEGC palette directly.
        ; Its G6/R5/B5 word is precomputed from the source G/R/B triplets;
        ; RGB332 is retained only for the 8bpp profile.
        shl     bx, 1
        mov     ax, [n4_va_rgb565_from_pegc + bx]
%else
        mov     al, [n4_va_rgb332_from_pegc + bx]
        mov     ah, al
%endif
%endif
        cmp     ax, [p5_last_color]
        je      .done
        cmp     di, sgp_command_list + ((P4_LIST_WORDS-8)*2)
        jb      .space_ready
        call    p5_flush_batch
.space_ready:
        mov     [p5_last_color], ax
        mov     dx, ax
%if NEON4_P5_BPP == 8
        mov     dh, dl
%endif
        mov     ax, SGP_SET_COLOR
        stosw
        mov     ax, dx
        stosw
        mov     [p5_list_offset], di
.done:
        pop     bx
        pop     ax
        ret

p5_start_batch:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    es
        push    cs
        pop     es
        mov     di, sgp_command_list
        mov     word [p5_last_color], 0ffffh
        mov     ax, SGP_SET_WORK
        stosw
        mov     si, sgp_work_area
        call    physical_address_from_ds_si
        stosw
        mov     ax, dx
        stosw
        mov     [p5_list_offset], di
        pop     es
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

p5_finish_batch:
        call    p5_flush_batch
        ret

p5_set_draw_page:
        push    ax
        cmp     byte [p5_draw_page], 0
        jne     .page_b
        mov     ax, PAGE_A_SGP_LOW
        mov     [p5_draw_sgp_low], ax
        mov     ax, PAGE_A_SGP_HIGH
        mov     [p5_draw_sgp_high], ax
        mov     ax, PAGE_A_DSA & 0ffffh
        mov     [p5_draw_dsa_low], ax
        mov     ax, PAGE_A_DSA >> 16
        mov     [p5_draw_dsa_high], ax
        mov     ax, G0_SEGMENT
        mov     [p5_draw_cpu_segment], ax
        pop     ax
        ret
.page_b:
        mov     ax, PAGE_B_SGP_LOW
        mov     [p5_draw_sgp_low], ax
        mov     ax, PAGE_B_SGP_HIGH
        mov     [p5_draw_sgp_high], ax
        mov     ax, PAGE_B_DSA & 0ffffh
        mov     [p5_draw_dsa_low], ax
        mov     ax, PAGE_B_DSA >> 16
        mov     [p5_draw_dsa_high], ax
        mov     ax, G0_SEGMENT + ((PAGE_B_SGP - PAGE_A_SGP) >> 4)
        mov     [p5_draw_cpu_segment], ax
        pop     ax
        ret

p5_clear_draw_page:
        mov     dx, [p5_draw_sgp_low]
        mov     bx, [p5_draw_sgp_high]
        jmp     sgp_clear_with_base

p5_flip_draw_page:
        call    wait_vblank_edge
        jc      .failed
%if (NEON4_P5_BPP == 4) || (NEON4_P5_BPP == 16)
        mov     dx, PORT_FB0_DSA_LOW
%else
        mov     dx, PORT_FB1_DSA_LOW
%endif
        mov     ax, [p5_draw_dsa_low]
        out     dx, ax
        add     dx, 2
        mov     ax, [p5_draw_dsa_high]
        out     dx, ax
        xor     byte [p5_draw_page], 1
        call    p5_set_draw_page
        clc
        ret
.failed:
        stc
        ret

p5_flush_batch:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        mov     di, [p5_list_offset]
        mov     ax, SGP_END
        stosw
        call    run_sgp_command_list
        jc      .failed
        call    p5_start_batch
        clc
        jmp     .done
.failed:
        stc
.done:
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

align 2
p5_rect_x0 dw 0
p5_rect_y0 dw 0
p5_rect_x1 dw 0
p5_rect_y1 dw 0
p5_line_x0 dw 0
p5_line_y0 dw 0
p5_line_x1 dw 0
p5_line_y1 dw 0
p5_line_width dw 0
p5_line_height dw 0
p5_span_x0 dw 0
p5_span_x1 dw 0
p5_span_y dw 0
p5_span_first_word dw 0
p5_span_last_word dw 0
p5_word_index dw 0
p5_word_low dw 0
p5_word_high dw 0
p5_word_mask dw 0
p5_word_value dw 0
p5_last_color dw 0ffffh
p5_list_offset dw 0
p5_draw_page db 1
align 2
p5_draw_sgp_low dw PAGE_B_SGP_LOW
p5_draw_sgp_high dw PAGE_B_SGP_HIGH
p5_draw_dsa_low dw PAGE_B_DSA & 0ffffh
p5_draw_dsa_high dw PAGE_B_DSA >> 16
p5_draw_cpu_segment dw G0_SEGMENT + ((PAGE_B_SGP - PAGE_A_SGP) >> 4)

align 2
; Packed 4bpp FB0 word order: x%4 = 0,1,2,3.
p5_packed_masks dw 00f0h, 000fh, 0f000h, 0f00h

%if ($ - $$) >= 0e000h
%error "NEON4 P5-1 payload overlaps the loader return reserve"
%endif
%endif
