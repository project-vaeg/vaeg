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
; BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
; USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
; THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
; THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

; NEON3 P3 counter payload.
;
; This is deliberately a geometry-only harness.  It reuses the original
; NEON3286 projection/scene code, but replaces the PC-98 raster backend with
; counters.  No DOS service, GRCG write, or host-side drawing is used here.
; The next P3 step will connect these same primitive boundaries to the VA SGP
; backend and VA text overlay.

        cpu     286
        bits    16
        org     0

%include "../../neon3_1_5/98/CONFIG3_286.INC"
; DATA3_286.INC is the original shared state block.  Its audio fields are
; dormant in this P3 counter payload, but retain their documented sentinel
; constants so the unmodified data include remains assembleable.  No OPL
; routine or I/O path is linked or called.
%define OPL_PROBE_AUTO 0
%define OPL_DETECT_NONE 0

; P3-B SGP contract.  These values are taken from the already validated
; GLASS/Sprite VA payloads; they are not a new hardware guess.  The NEON
; logical scene remains 640x400 and is mapped to the selected physical G0
; window at the primitive boundary.
%define NEON_SGP_COMMAND_PORT      0500h
%define NEON_SGP_CONTROL_PORT      0504h
%define NEON_SGP_STATUS_PORT       0506h
%define NEON_MEMORY_MAP_PORT       0153h
%define NEON_GVRAM_WRITE_PORT      0580h
%define NEON_SGP_BUSY              01h
%define NEON_SGP_END               0001h
%define NEON_SGP_SET_WORK          0003h
%define NEON_SGP_SET_COLOR         0006h
%define NEON_SGP_LINE              0009h
%define NEON_SGP_CLS               000ah
%define NEON_SGP_LINE_COPY         0005h
; SGP LINE direction bits.  The validated GLASS VA backend uses the
; hardware BLTMODE meanings: HD=0400h and VD=0800h.  These are direction
; flags, not axis selectors; swapping them produces slope-dependent line
; corruption.
%define NEON_SGP_LINE_HD           0400h
%define NEON_SGP_LINE_VD           0800h
%define NEON_G0_PITCH_BYTES        320
%define NEON_G0_WORDS_PER_LINE     160
; FB0/G0 uses two contiguous packed-4bpp pages.  These values are the
; established VA descriptor/SGP contract documented in
; docs/port/neon3_va_design.md; the profile changes only the page stride.
%define NEON_G0_PAGE_A_SGP_BASE    00200000h
%define NEON_G0_PAGE_A_DSA         00000000h
%ifdef NEON_PROFILE_400
%define NEON_G0_PAGE_B_SGP_BASE    0021f400h
%define NEON_G0_PAGE_B_DSA         0001f400h
%else
%define NEON_G0_PAGE_B_SGP_BASE    0020fa00h
%define NEON_G0_PAGE_B_DSA         0000fa00h
%endif
%define NEON_FB0_DSA_LOW_PORT      020eh
%define NEON_FB0_DSA_HIGH_PORT     0210h
%define NEON_TSP_STATUS_PORT       0142h
%define NEON_TSP_VBLANK            40h
%define NEON_MEMORY_MAP_GVRAM      054h
%define NEON_MEMORY_MAP_TVRAM      041h
%define NEON_GVRAM_CPU_WRITE       010h
; The complete NEON route contains substantially denser late-scene spans than
; the P3 smoke frames.  Keep enough room for the worst command list while
; leaving the payload's F000h stack window separate from the data tail.
;
; The normal build keeps the command list in the payload segment.  The
; NEON_SGP_EXTERNAL_LIST experiment moves it to a caller-owned RAM segment so
; capacity experiments do not inflate the COM payload into the loader-return
; reserve.  The experiment is deliberately opt-in; the default layout is
; unchanged.
%ifndef NEON_SGP_LIST_CAPACITY
%define NEON_SGP_LIST_CAPACITY     20480
%endif
%ifdef NEON_SGP_EXTERNAL_LIST
%ifndef NEON_SGP_LIST_SEGMENT
%define NEON_SGP_LIST_SEGMENT       2000h
%endif
%define NEON_SGP_LIST_OFFSET        0000h
%else
%define NEON_SGP_LIST_SEGMENT       3000h
%endif
%if NEON_SGP_LIST_CAPACITY < 2
%error "NEON_SGP_LIST_CAPACITY must leave room for an END word"
%endif
%ifdef NEON_SGP_EXTERNAL_LIST
%if NEON_SGP_LIST_CAPACITY > 0fffeh
%error "NEON_SGP_LIST_CAPACITY exceeds a 16-bit command-list segment"
%endif
%ifdef NEON_SAFE_BIOS_STACK
%error "NEON_SGP_EXTERNAL_LIST conflicts with NEON_SAFE_BIOS_STACK at 2000h"
%endif
%endif
%ifndef NEON_SGP_TIMEOUT_OUTER
%define NEON_SGP_TIMEOUT_OUTER      0100h
%endif
%ifndef NEON_SGP_TIMEOUT_INNER
%define NEON_SGP_TIMEOUT_INNER      0ffffh
%endif
%define NEON_VIDEO_BIOS_INT         8fh
%define NEON_KEYBOARD_BIOS_INT      82h
%define NEON_TEXT_BIOS_INT          83h
%define NEON_SCREEN_EDITOR_BIOS_INT 94h
%ifndef NEON_VIDEO_MODE
%ifdef NEON_PROFILE_400
%define NEON_VIDEO_MODE             0a000h
%else
%define NEON_VIDEO_MODE             0a002h
%endif
%endif
%define NEON_LOADER_RETURN_SS       0e000h
%define NEON_LOADER_RETURN_SP       0e002h
%define NEON_LOADER_RETURN_FLAGS    0e004h
%define NEON_LOADER_RETURN_MAGIC    0e006h
%define NEON_LOADER_RETURN_LOADER_SEG 0e008h
%define NEON_LOADER_RETURN_SIGNATURE 5034h
%define NEON_BIOS_DESCRIPTOR_OFFSET 1970h
%ifndef NEON_FRAME_LIMIT
%define NEON_FRAME_LIMIT            TOTAL_FRAMES
%endif
%ifndef NEON_LOOP
%define NEON_LOOP                   1
%endif

; ---------------------------------------------------------------------------
; Entry and deterministic 6144-frame looping demo.
; ---------------------------------------------------------------------------
start:
        cli
        mov     ax, 0338h
        mov     ds, ax
        mov     es, ax
        mov     ax, cs
        mov     ds, ax
        mov     es, ax
%ifdef NEON_SAFE_BIOS_STACK
        mov     ax, 2000h
%endif
        mov     ss, ax
        ; Keep the BIOS graphics workspace away from the payload's mutable
        ; scene/command data.  This mirrors the proven VA demo entry stack;
        ; the local counter stack remains reserved for the later idle path.
        mov     sp, 0f000h
        cld
        sti

%ifdef NEON_PROFILE_400
        mov     byte [video_400_mode], 1
        mov     word [video_height], SCREEN_H
%else
        mov     byte [video_400_mode], 0
        mov     word [video_height], VIDEO_H
%endif
        mov     ax, [video_height]
        mov     bx, NEON_G0_WORDS_PER_LINE
        mul     bx
        mov     [neon_sgp_page_words], ax
        ; Take ownership of the VA text console before entering graphics.
        ; Text BIOS function 2Fh with AL=0 suppresses the resident soft-key
        ; producer.  Screen Editor function 01h with AL=FF then hides the
        ; complete system-line display.  Both are VA BIOS services; TEXT
        ; itself remains enabled.
        call    neon_text_console_init
        call    neon_va_enter
        jc      neon_counter_bios_failed
        cld                     ; VA BIOS may return with DF set on this ROM.
        ; $ScnMode may leave DS as the VA BIOS workspace segment.  The
        ; validated GLASS sequence leaves it untouched and re-establishes
        ; only ES for the descriptor lists.
        push    cs
        pop     es
        ; Keep FB0 page A visible while the first frame is built on hidden
        ; page B.  The first page selection is deliberately explicit: a
        ; loader or BIOS must not determine which G0 page is displayed.
        call    neon_set_display_page_a
        mov     byte [neon_draw_page_index], 1
        call    neon_select_draw_page
        ; Reset any SGP execution left active by the VA BIOS mode setup.
        ; The payload owns the first command-list submission and must not
        ; inherit a stale busy state.
        mov     dx, NEON_SGP_CONTROL_PORT
        mov     al, 02h
        out     dx, al
        xor     al, al
        out     dx, al
        call    neon_counter_reset

        xor     ax, ax
        mov     [frame_counter], ax
        mov     [render_frame_counter], ax
        mov     [city_camera_timeline_valid], al
        mov     [scene_index], al

        ; Establish the text surface once before the first frame.  This clears
        ; stale TVRAM and writes the static overlay; later frame updates never
        ; clear or recompose the visible display.
        push    cs
        pop     ds
        mov     dx, NEON_MEMORY_MAP_PORT
        mov     al, NEON_MEMORY_MAP_TVRAM
        out     dx, al
        call    neon_status_clear_rows
        call    neon_text_write_static_overlay

%ifdef NEON_INIT_ONLY
        ; Initialization-only probe used by the P3 gate to separate VA BIOS
        ; setup from the later SGP command-list path.
        call    neon_counter_prepare_idle
        jmp     start.idle
%endif

%ifdef NEON_STATUS_ONLY
        call    neon_counter_show_status
        call    neon_counter_prepare_idle
        jmp     start.idle
%endif
.frame_loop:
        mov     ax, [frame_counter]
        mov     [render_frame_counter], ax
        mov     byte [draw_color], 0
        call    neon_sgp_begin_frame
        jc      neon_counter_sgp_failed
%ifdef NEON_DEBUG_BEGIN_HALT
        mov     si, neon_sgp_command_list
        mov     ax, [si]
        mov     bx, [si+2]
        mov     cx, [si+4]
        mov     dx, [si+6]
.debug_begin_halt:
        jmp     .debug_begin_halt
%endif
%ifdef NEON_MINIMAL_SGP
        ; QA ladder: submit only SET_WORK + END.  This isolates the SGP
        ; transport/idle contract from scene geometry and VRAM work.
%elifdef NEON_SMOKE_SGP
        ; Deterministic P3 visual probe.  It intentionally exercises only
        ; the shared CLS/LINE emitters, so a blank capture is distinguishable
        ; from a scene-geometry failure.
        mov     byte [draw_color], 0
        call    clear_graphics_page
%ifdef NEON_DEBUG_CLEAR_HALT
        mov     si, neon_sgp_command_list
        mov     ax, [si]
        mov     bx, [si+2]
        mov     cx, [si+4]
        mov     dx, [si+6]
.debug_clear_halt:
        jmp     .debug_clear_halt
%endif
%ifdef NEON_SMOKE_CLS
        mov     byte [draw_color], 7
        mov     ax, 80
        mov     bx, 40
        mov     cx, 560
        mov     si, 160
        call    fill_rect
%ifdef NEON_DEBUG_FILL_HALT
        mov     si, neon_sgp_command_list
        mov     ax, [si]
        mov     bx, [si+2]
        mov     cx, [si+4]
        mov     dx, [si+6]
.debug_fill_halt:
        jmp     .debug_fill_halt
%endif
%else
        mov     byte [draw_color], 7
        mov     ax, 80
        mov     bx, 40
        mov     cx, 560
        mov     dx, 40
        call    line_set
%endif
        mov     ax, 80
        mov     bx, 40
        mov     cx, 80
        mov     dx, 160
        call    line_set
        mov     ax, 80
        mov     bx, 160
        mov     cx, 560
        mov     dx, 160
        call    line_set
        mov     ax, 560
        mov     bx, 40
        mov     cx, 560
        mov     dx, 160
        call    line_set
%ifdef NEON_DEBUG_AFTER_LINES_HALT
        mov     si, neon_sgp_command_list
        mov     ax, [si]
        mov     bx, [si+2]
        mov     cx, [si+4]
        mov     dx, [si+6]
.debug_after_lines_halt:
        jmp     .debug_after_lines_halt
%endif
%else
        call    city_camera_catch_up
        mov     ax, [render_frame_counter]
        call    select_scene
        call    clear_graphics_page
        call    render_scene
%endif
        call    neon_sgp_end_frame
        jc      neon_counter_sgp_failed
        ; Present only the completed hidden page.  The DSA1 pseudo-sprite
        ; path uses the same word-write/VBLANK contract; FB0 has its own
        ; descriptor ports at 020eh/0210h.
        call    neon_present_draw_page
        jc      neon_counter_sgp_failed
        call    neon_counter_record_frame
%ifndef NEON_SKIP_TEXT_OVERLAY
        ; The SGP list is idle here, so the text update describes the frame
        ; that has just finished rather than a frame that is still rendering.
        call    neon_counter_update_overlay
%endif
        inc     word [frame_counter]
        cmp     word [frame_counter], NEON_FRAME_LIMIT
        jb      .frame_loop

%if NEON_LOOP
        ; The release payload is a continuous demo.  Rebase the logical
        ; timeline at its end; select_scene will re-enter scene 0 on the next
        ; frame and the normal scene-change path will rebase persistent state.
        xor     ax, ax
        mov     [frame_counter], ax
        mov     byte [neon_text_last_scene], 0ffh
        jmp     .frame_loop
%else
%ifndef NEON_SKIP_STATUS
        call    neon_counter_show_status
%endif
        call    neon_counter_prepare_idle
%endif

        ; Keep the completed result visible until the operator presses ESC.
        ; Keyboard polling and return use VA BIOS services only; DOS INT 21h
        ; is deliberately not part of this payload.
neon_gate_ready:
start.idle:
%ifdef NEON_DEBUG_UNIQUE_IDLE_HALT
        mov     ax, [frame_counter]
        mov     bx, [neon_bios_failure_marker]
        mov     cx, [neon_sgp_idle_seen]
        mov     dx, NEON_FRAME_LIMIT
        jmp     neon_debug_unique_idle_halt
%endif
%ifdef NEON_HOLD_BEFORE_INPUT
.hold:
        hlt
        jmp     .hold
%else
        call    neon_escape_pressed
        jc      neon_counter_exit
        hlt
        jmp     start.idle
%endif

neon_counter_sgp_failed:
        cmp     word [neon_sgp_failure_marker], 0
        jne     .marker_ready
        mov     word [neon_sgp_failure_marker], 0x4e53
.marker_ready:
%ifndef NEON_SKIP_FAILURE_STATUS
        call    neon_counter_show_status
%endif
        call    neon_counter_prepare_idle
        jmp     start.idle

neon_counter_bios_failed:
        mov     word [neon_bios_failure_marker], 0x4249
%ifndef NEON_SKIP_FAILURE_STATUS
        call    neon_counter_show_status
%endif
        call    neon_counter_prepare_idle
        jmp     start.idle

; Keep the gate result in registers at the idle entry for debugger capture.
; AX=completed frames, BX=BIOS failure marker, CX=SGP idle marker, and
; DX=compiled frame limit.  This is an observation aid, not a drawing path.
neon_counter_prepare_idle:
        mov     ax, [frame_counter]
        mov     bx, [neon_bios_failure_marker]
        mov     cx, [neon_sgp_idle_seen]
        mov     dx, NEON_FRAME_LIMIT
        mov     si, [neon_bios_return_code]
%ifdef NEON_DEBUG_LIST
        mov     di, neon_sgp_command_list
        mov     ax, [es:di]
        mov     bx, [di+2]
        mov     cx, [di+4]
        mov     dx, [di+6]
        mov     bp, ds
        mov     si, [neon_sgp_list_cursor]
        mov     di, [neon_sgp_frame_active]
        ; The first-write probe is returned through the otherwise unused
        ; ES:DI/DS:SI slots in a debugger capture: SI=first AX, DI=first DS.
        mov     si, [neon_debug_first_ax]
        mov     di, [neon_debug_first_ds]
        mov     bp, [neon_debug_first_count]
        mov     ax, [neon_debug_bad_ax]
        mov     bx, [neon_debug_bad_cursor]
        mov     cx, [neon_debug_bad_ds]
        mov     dx, [neon_debug_bad_ds_count]
        mov     si, [neon_debug_bad_ret_ip]
        mov     di, [neon_debug_first_ax]
%endif
        ret

neon_counter_exit:
        call    neon_va_leave
        call    neon_text_console_restore
        cmp     word [cs:NEON_LOADER_RETURN_MAGIC], NEON_LOADER_RETURN_SIGNATURE
        jne     start.idle
        cli
        mov     ax, [cs:NEON_LOADER_RETURN_SS]
        mov     ss, ax
        mov     sp, [cs:NEON_LOADER_RETURN_SP]
        push    word [cs:NEON_LOADER_RETURN_FLAGS]
        popf
        retf

; Remove the resident soft-key/function-key guide without disabling TEXT.
; INT 83h/AH=2Fh stops the soft-key producer (AL=0 requests zero entries),
; while INT 94h/AH=01h with AL=FF hides the reserved system-line display.
neon_text_console_init:
        pusha
        push    ds
        push    es
        mov     ah, 2fh                ; Text BIOS: soft-key display control
        xor     al, al                 ; display zero function keys
        int     NEON_TEXT_BIOS_INT
        mov     ah, 01h                ; Screen Editor: system-line control
        mov     al, 0ffh               ; erase/hide the complete system line
        int     NEON_SCREEN_EDITOR_BIOS_INT
        mov     ah, 25h                ; Text BIOS: cursor display control
        xor     al, al                 ; hide cursor and cursor blink
        int     NEON_TEXT_BIOS_INT
        pop     es
        pop     ds
        popa
        ret

; Restore the normal ten-entry system-line/soft-key guide for the caller.
neon_text_console_restore:
        pusha
        push    ds
        push    es
        mov     ah, 2fh                ; Text BIOS: soft-key display control
        mov     al, 0ah                ; display the ten-key caller guide
        int     NEON_TEXT_BIOS_INT
        mov     ah, 01h                ; Screen Editor: system-line control
        mov     al, 0ah                ; display the ten-entry system line
        int     NEON_SCREEN_EDITOR_BIOS_INT
        mov     ah, 25h                ; Text BIOS: cursor display control
        mov     al, 03h                ; restore the normal blinking cursor
        int     NEON_TEXT_BIOS_INT
        pop     es
        pop     ds
        popa
        ret

; ---------------------------------------------------------------------------
; VA BIOS-only entry and a small text status overlay.
; ---------------------------------------------------------------------------
neon_va_enter:
        pusha
        mov     word [neon_bios_failure_marker], 0
        mov     word [neon_bios_return_code], 0
%ifdef NEON_PROFILE_400
        mov     bx, NEON_VIDEO_MODE
        ; $DefBuf describes one 640x400 FB0 window.  The second physical
        ; page is selected through FB0 DSA/SGP offsets; an 800-line descriptor
        ; is rejected by the VA BIOS even though the backing VRAM is present.
        mov     word [neon_framebuffer_descriptor+4], SCREEN_H
        mov     word [neon_window_descriptor+4], SCREEN_H
%else
        mov     bx, NEON_VIDEO_MODE
        mov     word [neon_framebuffer_descriptor+4], SCREEN_H
        mov     word [neon_window_descriptor+4], VIDEO_H
%endif
        mov     cx, 4
        xor     dx, dx
        xor     ax, ax
        mov     si, 0338h
        mov     ds, si
        mov     es, si
        int     NEON_VIDEO_BIOS_INT    ; VA $ScnMode
        ; Keep the BIOS-returned DS workspace segment; only ES is reset for
        ; the descriptor list, matching the validated GLASS sequence.
        push    cs
        pop     es
        or      ax, ax
        jnz     .fail_mode

        ; The VA descriptor ABI is ES:DI.  Keep the descriptor setup identical
        ; to the validated GLASS VA path: the ROM owns DS/ES while it handles
        ; the call, so only ES is re-established for each descriptor list.
        ; In particular, do not add a second descriptor or rewrite fields
        ; beyond the three words consumed by $DefBuf.
        push    cs
        pop     es
        mov     di, neon_framebuffer_descriptor
        mov     ax, 0100h              ; VA $DefBuf, descriptor 1
        mov     cx, 1
        int     NEON_VIDEO_BIOS_INT
        push    cs
        pop     es
        or      ax, ax
        jnz     .fail_buffer
        mov     ax, 0200h              ; VA $DefWin, descriptor 1
        mov     cx, 1
        mov     di, neon_window_descriptor
        int     NEON_VIDEO_BIOS_INT
        push    cs
        pop     es
        or      ax, ax
        jnz     .fail_window
        mov     ax, 0338h
        mov     ds, ax
        mov     es, ax
        mov     ax, 0900h              ; VA $PalCtl, palette mode 0
        int     NEON_VIDEO_BIOS_INT
        push    cs
        pop     ds
        push    cs
        pop     es
        or      ax, ax
        jnz     .fail_palette_control
        xor     bx, bx
        mov     si, neon_palette
.palette:
        mov     ax, 0800h              ; VA $SetPal, AL=index, CX=value
        mov     al, bl
        mov     cx, [cs:si]
        push    ax
        mov     ax, 0338h
        mov     ds, ax
        mov     es, ax
        pop     ax
        int     NEON_VIDEO_BIOS_INT
        push    cs
        pop     ds
        push    cs
        pop     es
        or      ax, ax
        jnz     .fail_palette_entry
        inc     bl
        add     si, 2
        cmp     bl, 16
        jb      .palette
        mov     ax, 0338h
        mov     ds, ax
        mov     es, ax
        ; Compose priority nibbles are documented as highest to lowest.
        ; 1 selects the text plane and 3 selects G0, so 0031h keeps the
        ; graphics scene visible with the VA text overlay above it.
        mov     ax, 0300h              ; VA $Compose, text above G0
        mov     cx, 0031h
        int     NEON_VIDEO_BIOS_INT
        push    cs
        pop     ds
        push    cs
        pop     es
        or      ax, ax
        jnz     .fail_compose
        mov     ax, 0338h
        mov     ds, ax
        mov     es, ax
        mov     ax, 0b01h              ; VA $ScnDsp, graphics on
        int     NEON_VIDEO_BIOS_INT
        push    cs
        pop     ds
        push    cs
        pop     es
        or      ax, ax
        jnz     .fail_display
        mov     dx, NEON_MEMORY_MAP_PORT
        mov     al, NEON_MEMORY_MAP_GVRAM
        out     dx, al
        mov     dx, NEON_GVRAM_WRITE_PORT
        mov     al, NEON_GVRAM_CPU_WRITE
        out     dx, al
        ; The text overlay uses the TVRAM aperture.  Restore the GVRAM
        ; single-plane map before the first SGP command-list submission;
        ; otherwise the SGP enable bit (GMSP) remains cleared and the list
        ; is accepted but never executed.
        mov     dx, NEON_MEMORY_MAP_PORT
        mov     al, NEON_MEMORY_MAP_GVRAM
        out     dx, al
        mov     dx, NEON_GVRAM_WRITE_PORT
        mov     al, NEON_GVRAM_CPU_WRITE
        out     dx, al
        popa
        clc
        ret

.fail_mode:
        mov     [neon_bios_return_code], ax
        mov     word [neon_bios_failure_marker], 4d53h
        jmp     .failed
.fail_buffer:
        mov     [neon_bios_return_code], ax
        mov     word [neon_bios_failure_marker], 4246h
        jmp     .failed
.fail_window:
        mov     [neon_bios_return_code], ax
        mov     word [neon_bios_failure_marker], 574eh
        jmp     .failed
.fail_palette_control:
        mov     [neon_bios_return_code], ax
        mov     word [neon_bios_failure_marker], 5043h
        jmp     .failed
.fail_palette_entry:
        mov     [neon_bios_return_code], ax
        mov     word [neon_bios_failure_marker], 5045h
        jmp     .failed
.fail_compose:
        mov     [neon_bios_return_code], ax
        mov     word [neon_bios_failure_marker], 434fh
        jmp     .failed
.fail_display:
        mov     [neon_bios_return_code], ax
        mov     word [neon_bios_failure_marker], 4453h
.failed:
        popa
        stc
        ret

neon_va_leave:
        mov     dx, NEON_MEMORY_MAP_PORT
        mov     al, NEON_MEMORY_MAP_TVRAM
        out     dx, al
        ; VA screen-control BIOS calls may use the caller's segment registers
        ; as scratch storage.  Keep the payload out of that work area and
        ; restore its data segments before returning to the loader.
        mov     ax, 0338h
        mov     ds, ax
        mov     es, ax
        mov     ax, 0300h              ; VA $Compose, text above G0
        mov     cx, 0031h
        int     NEON_VIDEO_BIOS_INT
        mov     ax, 0338h
        mov     ds, ax
        mov     es, ax
        mov     ax, 0b01h              ; VA $ScnDsp, display on
        int     NEON_VIDEO_BIOS_INT
        cld
        push    cs
        pop     ds
        push    cs
        pop     es
        ret

; Draw fields that never change during a run.  They are written once after
; the complete TVRAM clear so frame updates do not repaint the whole overlay.
neon_text_write_static_overlay:
        pusha
        push    ds
        push    es
        push    cs
        pop     ds
        mov     dx, NEON_MEMORY_MAP_PORT
        mov     al, NEON_MEMORY_MAP_TVRAM
        out     dx, al

        mov     si, neon_live_title
        xor     dx, dx
        call    neon_status_bios_puts_at
        mov     si, neon_live_frame
        mov     dh, 1
        xor     dl, dl
        call    neon_status_bios_puts_at
        mov     si, neon_live_local
        mov     dh, 2
        xor     dl, dl
        call    neon_status_bios_puts_at
        mov     si, neon_live_scene_title
        mov     dh, 3
        xor     dl, dl
        call    neon_status_bios_puts_at
        mov     si, neon_live_limit
        mov     dh, 4
        xor     dl, dl
        call    neon_status_bios_puts_at
        mov     si, neon_status_exit
        mov     dh, 14
        xor     dl, dl
        call    neon_status_bios_puts_at

        mov     dx, NEON_MEMORY_MAP_PORT
        mov     al, NEON_MEMORY_MAP_GVRAM
        out     dx, al
        pop     es
        pop     ds
        popa
        ret

; Refresh only fixed-width live fields after the completed page has been
; presented.  The caller reaches this point immediately after VBLANK; TEXT
; remains composed above G0, so no display or composition toggle is needed.
neon_counter_update_overlay:
        pusha
        push    ds
        push    es
        push    cs
        pop     ds
        mov     dx, NEON_MEMORY_MAP_PORT
        mov     al, NEON_MEMORY_MAP_TVRAM
        out     dx, al

        mov     ax, [render_frame_counter]
        call    neon_status_make_hex
        mov     si, neon_status_hex_buffer
        mov     dh, 1
        mov     dl, 24
        call    neon_status_bios_puts_at

        mov     ax, [scene_frame]
        call    neon_status_make_hex
        mov     si, neon_status_hex_buffer
        mov     dh, 2
        mov     dl, 24
        call    neon_status_bios_puts_at

        mov     al, [scene_index]
        cmp     al, [neon_text_last_scene]
        je      .scene_done
        mov     [neon_text_last_scene], al
        mov     si, neon_status_blank_line
        add     si, 18
        mov     dh, 3
        mov     dl, 18
        call    neon_status_bios_puts_at
        xor     bx, bx
        mov     bl, [scene_index]
        cmp     bl, SCENE_COUNT
        jae     .scene_done
        shl     bx, 1
        mov     si, [scene_title_ptrs + bx]
        mov     dh, 3
        mov     dl, 18
        call    neon_status_bios_puts_at
.scene_done:
        mov     dx, NEON_MEMORY_MAP_PORT
        mov     al, NEON_MEMORY_MAP_GVRAM
        out     dx, al
        pop     es
        pop     ds
        popa
        ret

; VA keyboard BIOS polling: AH=01h tests, AH=00h consumes one key.
; Return CF=1 only for the ESC key.
neon_escape_pressed:
        mov     ah, 01h
        int     NEON_KEYBOARD_BIOS_INT
        jc      .none
        mov     ah, 00h
        int     NEON_KEYBOARD_BIOS_INT
        cmp     bh, 0
        jne     .none
        cmp     bl, 1bh
        je      .escape
.none:
        clc
        ret
.escape:
        mov     byte [neon_escape_seen], 1
        stc
        ret

neon_counter_show_status:
        pusha
        push    ds
        push    cs
        pop     ds
        ; Text BIOS writes the frame buffer through A0000h.  The graphics
        ; path leaves bank 4 (GVRAM) selected, so select bank 1 (TVRAM)
        ; before invoking INT 83h.
        mov     dx, NEON_MEMORY_MAP_PORT
        mov     al, NEON_MEMORY_MAP_TVRAM
        out     dx, al
        cld
        push    cs
        pop     ds
        ; Write text while the graphics composition is still active.  The VA
        ; text BIOS uses the resident frame descriptor in this state; the
        ; composition switch is performed only after all character/attribute
        ; cells have been written (the same ordering as the standalone probe).
        mov     dx, NEON_MEMORY_MAP_PORT
        mov     al, NEON_MEMORY_MAP_TVRAM
        out     dx, al
        call    neon_status_clear_rows

        mov     si, neon_status_title
        mov     dh, 0
        xor     dl, dl
        call    neon_status_bios_puts_at
        mov     si, neon_status_profile
        mov     dh, 1
        xor     dl, dl
        call    neon_status_bios_puts_at

        mov     si, neon_status_line
        mov     ax, [neon_counter_max_line_calls]
        mov     dh, 2
        call    neon_status_bios_hex_at
        mov     si, neon_status_triangle
        mov     ax, [neon_counter_max_triangle_calls]
        mov     dh, 3
        call    neon_status_bios_hex_at
        mov     si, neon_status_span
        mov     ax, [neon_counter_max_triangle_spans]
        mov     dh, 4
        call    neon_status_bios_hex_at
        mov     si, neon_status_rect
        mov     ax, [neon_counter_max_fill_rect_rows]
        mov     dh, 5
        call    neon_status_bios_hex_at
        mov     si, neon_status_color
        mov     ax, [neon_counter_max_set_color]
        mov     dh, 6
        call    neon_status_bios_hex_at
        mov     si, neon_status_cls
        mov     ax, [neon_counter_max_cls]
        mov     dh, 7
        call    neon_status_bios_hex_at
        mov     si, neon_status_words
        mov     ax, [neon_counter_max_command_words]
        mov     dh, 8
        call    neon_status_bios_hex_at
        mov     si, neon_status_frames
        mov     ax, [frame_counter]
        mov     dh, 9
        call    neon_status_bios_hex_at
        mov     si, neon_status_limit
        mov     ax, NEON_FRAME_LIMIT
        mov     dh, 10
        call    neon_status_bios_hex_at
        mov     si, neon_status_bios
        mov     ax, [neon_bios_failure_marker]
        mov     dh, 11
        call    neon_status_bios_hex_at
        mov     si, neon_status_bios_rc
        mov     ax, [neon_bios_return_code]
        mov     dh, 12
        call    neon_status_bios_hex_at
        mov     si, neon_status_sgp
        mov     ax, [neon_sgp_failure_marker]
        or      ax, ax
        jnz     .sgp_status_ready
        mov     ax, [neon_sgp_idle_seen]
.sgp_status_ready:
        mov     dh, 13
        call    neon_status_bios_hex_at
        mov     si, neon_status_exit
        mov     dh, 14
        xor     dl, dl
        call    neon_status_bios_puts_at

        ; Now expose the text plane.  $TextInit is deliberately avoided: on
        ; some VA ROMs it overwrites the graphics-mode text frame descriptor.
        xor     ax, ax
        mov     ds, ax
        mov     es, ax
        mov     ax, 0b00h              ; VA $ScnDsp, graphics off
        int     NEON_VIDEO_BIOS_INT
        mov     ax, 0338h
        mov     ds, ax
        mov     es, ax
        mov     ax, 0300h              ; VA $Compose, text only
        mov     cx, 0001h
        int     NEON_VIDEO_BIOS_INT
        mov     ax, 0338h
        mov     ds, ax
        mov     es, ax
        mov     ax, 0b01h              ; VA $ScnDsp, display on
        int     NEON_VIDEO_BIOS_INT
        cld
        push    cs
        pop     ds
        push    cs
        pop     es
        pop     ds
        popa
        ret

; Clear the rows used by a diagnostic or live overlay.  The VA ROMs used by
; this payload have a working ASCIZ text path in graphics composition, while
; AH=17h/AL=02h can fail to return when issued during active G0 composition.
; Keep the clear operation on the known-good text BIOS path and cover the
; complete 80-column rows so stale loader text cannot remain visible.  The
; startup Text BIOS soft-key control and Screen Editor system-line control
; disable the resident guide first, so the text overlay owns all 25 rows.
neon_status_clear_rows:
        push    ax
        push    bx
        push    dx
        push    si
        xor     bx, bx
.row:
        mov     dh, bl
        xor     dl, dl
        mov     si, neon_status_blank_line
        call    neon_status_bios_puts_at
        inc     bl
        cmp     bl, 25
        jb      .row
        pop     si
        pop     dx
        pop     bx
        pop     ax
        ret

neon_status_bios_puts_at:
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
        ; Text BIOS coordinates are DH=X (column), DL=Y (row), while the
        ; payload API exposes DH=row and DL=column.
        mov     ah, 08h
        mov     dh, bl
        mov     dl, bh
        push    si
        int     NEON_TEXT_BIOS_INT
        pop     si
        push    cs
        pop     ds
        push    cs
        pop     es
        ; AH=02 writes an ASCIZ string using the current text attribute.
        ; Keep this sequence identical to the standalone VA BIOS probe;
        ; the AH=05 attribute service is intentionally not used here because
        ; it changes BIOS text state on some VA ROMs.
        mov     ah, 02h
        ; Attribute word used by the VA BIOS ASCIZ service.  8000h is the
        ; documented/default text attribute used by the working probe;
        ; 0007h selects an invisible attribute on the VA2 ROM.
        mov     dx, 8000h
        push    si
        int     NEON_TEXT_BIOS_INT
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

; DS:SI = label, AX = value, DH = text row.
neon_status_bios_hex_at:
        push    ax
        push    dx
        push    si
        xor     dl, dl
        call    neon_status_bios_puts_at
        pop     si
        pop     dx
        pop     ax
        call    neon_status_make_hex
        mov     si, neon_status_hex_buffer
        ; Place the value after the fixed-width label.
        mov     dl, 24
        call    neon_status_bios_puts_at
        ret

neon_status_make_hex:
        push    ax
        push    bx
        push    cx
        push    dx
        push    di
        mov     bx, ax
        mov     di, neon_status_hex_buffer
        mov     cx, 4
.digit:
        rol     bx, 4
        mov     dx, bx
        and     dx, 0fh
        mov     si, dx
        mov     al, [neon_hex_digits + si]
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

; ---------------------------------------------------------------------------
; Counter backend.  All entry points preserve the caller's registers because
; the original scene code expects the same preservation as the video backend.
; ---------------------------------------------------------------------------
neon_counter_reset:
        pusha
        mov     word [neon_counter_line_calls], 0
        mov     word [neon_counter_triangle_calls], 0
        mov     word [neon_counter_triangle_spans], 0
        mov     word [neon_counter_fill_rect_rows], 0
        mov     word [neon_counter_fill_rect_spans], 0
        mov     word [neon_counter_set_color], 0
        mov     word [neon_counter_cls], 0
        mov     word [neon_counter_end], 0
        mov     word [neon_counter_command_words], 0
        mov     word [neon_counter_max_line_calls], 0
        mov     word [neon_counter_max_triangle_calls], 0
        mov     word [neon_counter_max_triangle_spans], 0
        mov     word [neon_counter_max_fill_rect_rows], 0
        mov     word [neon_counter_max_fill_rect_spans], 0
        mov     word [neon_counter_max_set_color], 0
        mov     word [neon_counter_max_cls], 0
        mov     word [neon_counter_max_command_words], 0
        mov     byte [neon_counter_last_color], 0ffh
        mov     word [neon_counter_frame_index], 0
        mov     word [neon_sgp_failure_marker], 0
        mov     word [neon_sgp_idle_seen], 0
        mov     byte [neon_escape_seen], 0
        mov     byte [neon_text_last_scene], 0ffh
        popa
        ret

; Emitters use the same command-list word accounting as the VA SGP design:
; SET_COLOR=2, CLS=5, LINE=8, END=1.  SET_COLOR is suppressed when the
; requested colour is unchanged, matching the Glass SGP backend contract.
line_batch_begin:
        pusha
        mov     al, [draw_color]
        cmp     al, [neon_counter_last_color]
        je      .same_colour
        mov     [neon_counter_last_color], al
        inc     word [neon_counter_set_color]
        add     word [neon_counter_command_words], 2
        xor     ah, ah
        call    neon_sgp_emit_set_color_index
.same_colour:
        popa
        ret

line_set:
        call    line_batch_begin
        jmp     line_set_same_colour

line_set_same_colour:
        pusha
        inc     word [neon_counter_line_calls]
        add     word [neon_counter_command_words], 8
        call    neon_sgp_emit_line
        popa
        ret

hline_set:
        jmp     hline_set_same_colour

hline_set_same_colour:
        pusha
        call    line_batch_begin
        inc     word [neon_counter_triangle_spans]
        cmp     ax, cx
        jle     .ordered
        xchg    ax, cx
.ordered:
        call    neon_sgp_emit_span_interior
        popa
        ret

fill_rect:
        ; AX=x0, BX=y0, CX=x1, SI=y1 in the faithful 286 scene code.
        pusha
        call    line_batch_begin
        cmp     bx, si
        jle     .ordered
        xchg    bx, si
.ordered:
        cmp     si, 0
        jl      .done
        cmp     bx, [video_height]
        jge     .done
        cmp     bx, 0
        jge     .y0_ok
        xor     bx, bx
.y0_ok:
        mov     dx, [video_height]
        dec     dx
        cmp     si, dx
        jle     .y1_ok
        mov     si, dx
.y1_ok:
        sub     si, bx
        inc     si
        add     word [neon_counter_fill_rect_rows], si
        add     word [neon_counter_fill_rect_spans], si
        mov     [neon_sgp_rect_x0], ax
        mov     [neon_sgp_rect_x1], cx
        mov     [neon_sgp_rect_y], bx
.row:
        mov     ax, [neon_sgp_rect_x0]
        mov     bx, [neon_sgp_rect_y]
        mov     cx, [neon_sgp_rect_x1]
        call    neon_sgp_emit_span_interior
        inc     word [neon_sgp_rect_y]
        dec     si
        jnz     .row
.done:
        popa
        ret

; ---------------------------------------------------------------------------
; P3-B SGP command-list backend.  The geometry callbacks above remain shared
; with the counter build.  This stage emits complete CLS words only; exact
; partial-word endpoint RMW is intentionally a later increment.
; ---------------------------------------------------------------------------
neon_sgp_begin_frame:
        pusha
        cld
        ; Keep ES on the command-list segment for the complete frame.  DS
        ; remains the payload segment because all mutable state is there.
        mov     ax, NEON_SGP_LIST_SEGMENT
        mov     es, ax
        mov     word [neon_sgp_list_cursor], neon_sgp_command_list
        mov     byte [neon_sgp_list_overflow], 0
        mov     word [neon_sgp_last_color], 0ffffh
        mov     byte [neon_sgp_frame_active], 1
%ifdef NEON_STATIC_END
        mov     byte [es:neon_sgp_command_list], 1
        mov     byte [es:neon_sgp_command_list+1], 0
%endif
%ifndef NEON_NO_SET_WORK
        add     word [neon_counter_command_words], 3
        mov     ax, 3
        call    neon_sgp_reserve_words
        mov     ax, NEON_SGP_SET_WORK
        call    neon_sgp_emit_word
        mov     si, neon_sgp_work_area
        call    neon_sgp_physical_address_from_ds_si
        call    neon_sgp_emit_word
        mov     ax, dx
        call    neon_sgp_emit_word
%endif
        popa
        clc
        ret

neon_sgp_end_frame:
        pusha
        cld
        inc     word [neon_counter_end]
        inc     word [neon_counter_command_words]
%ifndef NEON_STATIC_END
        mov     ax, 1
        call    neon_sgp_reserve_words
        mov     ax, NEON_SGP_END
        call    neon_sgp_emit_word
%endif
        cmp     byte [neon_sgp_list_overflow], 0
        jne     .failed
        mov     si, neon_sgp_command_list
        call    neon_sgp_physical_address_from_es_si
        mov     [neon_sgp_command_address_low], ax
        mov     [neon_sgp_command_address_high], dx
%ifdef NEON_DEBUG_LIST
        mov     di, neon_sgp_command_list
        mov     ax, [es:di]
        mov     [neon_debug_word0_before_submit], ax
        mov     ax, [es:di+2]
        mov     [neon_debug_word1_before_submit], ax
        mov     ax, [es:di+4]
        mov     [neon_debug_word2_before_submit], ax
        mov     ax, [es:di+6]
        mov     [neon_debug_word3_before_submit], ax
%endif
        call    neon_sgp_run_list
        jc      .failed
        mov     byte [neon_sgp_frame_active], 0
        popa
        push    cs
        pop     es
        clc
        ret
.failed:
        mov     byte [neon_sgp_frame_active], 0
        popa
        push    cs
        pop     es
        stc
        ret

neon_sgp_emit_set_color_index:
        push    bx
        and     ax, 000fh
        mov     bx, ax
        shl     bx, 4
        or      ax, bx
        mov     bx, ax
        shl     bx, 8
        or      ax, bx
        pop     bx
        jmp     neon_sgp_emit_set_color_word

neon_sgp_emit_set_color_word:
        cmp     ax, [neon_sgp_last_color]
        je      .done
        push    ax
        mov     ax, 2
        call    neon_sgp_reserve_words
        pop     ax
        mov     [neon_sgp_last_color], ax
        push    ax
        mov     ax, NEON_SGP_SET_COLOR
        call    neon_sgp_emit_word
        pop     ax
        call    neon_sgp_emit_word
.done:
        ret

neon_sgp_emit_cls:
        or      cx, cx
        jz      .done
        push    ax
        mov     ax, 5
        call    neon_sgp_reserve_words
        pop     ax
        push    ax
        mov     ax, NEON_SGP_CLS
        call    neon_sgp_emit_word
        pop     ax
        call    neon_sgp_emit_word
        mov     ax, dx
        call    neon_sgp_emit_word
        mov     ax, cx
        call    neon_sgp_emit_word
        xor     ax, ax
        call    neon_sgp_emit_word
.done:
        ret

; Ensure that AX command words plus one END word fit in the current batch.
; A command is reserved as a whole by its caller, so a batch boundary never
; leaves a partially emitted LINE or CLS record for the SGP to decode.
neon_sgp_reserve_words:
        pusha
        mov     bx, ax
        shl     bx, 1
        add     bx, 2
        mov     di, [neon_sgp_list_cursor]
        mov     ax, neon_sgp_command_list_end
        sub     ax, di
        cmp     ax, bx
        jae     .enough
        call    neon_sgp_flush_partial
.enough:
        popa
        ret

; Finish the current command batch and start a fresh one for the same frame.
; The SGP is waited to idle before the cursor is rewound, so no command list
; storage is reused while the accelerator can still read it.
neon_sgp_flush_partial:
        pusha
        mov     ax, [neon_sgp_last_color]
        mov     [neon_sgp_resume_color], ax
        mov     di, [neon_sgp_list_cursor]
        cmp     di, neon_sgp_command_list
        je      .done
        mov     ax, NEON_SGP_END
        mov     [es:di], al
        mov     [es:di+1], ah
        add     di, 2
        mov     [neon_sgp_list_cursor], di
        mov     si, neon_sgp_command_list
        call    neon_sgp_physical_address_from_es_si
        mov     [neon_sgp_command_address_low], ax
        mov     [neon_sgp_command_address_high], dx
        inc     word [neon_counter_end]
        inc     word [neon_counter_command_words]
        call    neon_sgp_run_list
        jc      .failed

        mov     word [neon_sgp_list_cursor], neon_sgp_command_list
        mov     word [neon_sgp_last_color], 0ffffh
        add     word [neon_counter_command_words], 3
        mov     ax, NEON_SGP_SET_WORK
        call    neon_sgp_emit_word
        mov     si, neon_sgp_work_area
        call    neon_sgp_physical_address_from_ds_si
        call    neon_sgp_emit_word
        mov     ax, dx
        call    neon_sgp_emit_word
        mov     ax, [neon_sgp_resume_color]
        cmp     ax, 0ffffh
        je      .done
        add     word [neon_counter_command_words], 2
        call    neon_sgp_emit_set_color_word
.done:
        popa
        ret
.failed:
        mov     byte [neon_sgp_list_overflow], 1
        popa
        ret

; AX=x0, BX=y0, CX=x1, DX=y1 in logical 640x400 coordinates.
neon_sgp_emit_line:
        cmp     byte [neon_sgp_frame_active], 0
        je      .done
        mov     [neon_sgp_line_x0], ax
        mov     [neon_sgp_line_y0], bx
        mov     [neon_sgp_line_x1], cx
        mov     [neon_sgp_line_y1], dx
        mov     ax, [neon_sgp_line_x1]
        sub     ax, [neon_sgp_line_x0]
        jns     .x_positive
        neg     ax
        mov     bx, NEON_SGP_LINE_COPY | NEON_SGP_LINE_HD
        jmp     .x_ready
.x_positive:
        mov     bx, NEON_SGP_LINE_COPY
.x_ready:
        inc     ax
        mov     [neon_sgp_line_width], ax
        mov     ax, [neon_sgp_line_y0]
        call    neon_sgp_map_y
        mov     [neon_sgp_line_y0_physical], ax
        mov     ax, [neon_sgp_line_y1]
        call    neon_sgp_map_y
        mov     [neon_sgp_line_y1_physical], ax
        sub     ax, [neon_sgp_line_y0_physical]
        jns     .y_positive
        neg     ax
        or      bx, NEON_SGP_LINE_VD
.y_positive:
        inc     ax
        mov     [neon_sgp_line_height], ax
        mov     ax, 8
        call    neon_sgp_reserve_words
        mov     ax, NEON_SGP_LINE
        call    neon_sgp_emit_word
        mov     ax, bx
        call    neon_sgp_emit_word
        mov     ax, [neon_sgp_line_x0]
        and     ax, 3
        shl     ax, 4
        or      ax, 1
        call    neon_sgp_emit_word
        mov     ax, [neon_sgp_line_width]
        call    neon_sgp_emit_word
        mov     ax, [neon_sgp_line_height]
        call    neon_sgp_emit_word
        mov     ax, NEON_G0_PITCH_BYTES
        call    neon_sgp_emit_word
        mov     ax, [neon_sgp_line_y0_physical]
        mov     bx, NEON_G0_PITCH_BYTES
        mul     bx
        mov     bx, [neon_sgp_line_x0]
        and     bx, 0fffch
        shr     bx, 1
        add     ax, bx
        adc     dx, 0
        add     ax, [neon_g0_sgp_base_low]
        adc     dx, [neon_g0_sgp_base_high]
        call    neon_sgp_emit_word
        mov     ax, dx
        call    neon_sgp_emit_word
.done:
        ret

; AX=x0, BX=logical y, CX=x1.  Only complete 4-pixel words are submitted.
neon_sgp_emit_span_interior:
        cmp     byte [neon_sgp_frame_active], 0
        je      .done
        cmp     ax, cx
        jg      .done
        cmp     cx, 0
        jl      .done
        cmp     ax, SCREEN_W-1
        jg      .done
        cmp     ax, 0
        jge     .x0_ready
        xor     ax, ax
.x0_ready:
        cmp     cx, SCREEN_W-1
        jle     .x1_ready
        mov     cx, SCREEN_W-1
.x1_ready:
        mov     [neon_sgp_span_y], bx
        mov     dx, ax
        and     dx, 3
        shr     ax, 2
        or      dx, dx
        jz      .left_aligned
        inc     ax
.left_aligned:
        mov     [neon_sgp_span_first_word], ax
        mov     ax, cx
        mov     dx, ax
        and     dx, 3
        shr     ax, 2
        cmp     dx, 3
        je      .right_aligned
        dec     ax
.right_aligned:
        cmp     ax, [neon_sgp_span_first_word]
        jb      .done
        sub     ax, [neon_sgp_span_first_word]
        inc     ax
        mov     cx, ax
        mov     ax, [neon_sgp_span_y]
        call    neon_sgp_map_y
        mov     bx, NEON_G0_PITCH_BYTES
        mul     bx
        mov     bx, [neon_sgp_span_first_word]
        shl     bx, 1
        add     ax, bx
        adc     dx, 0
        add     ax, [neon_g0_sgp_base_low]
        adc     dx, [neon_g0_sgp_base_high]
        inc     word [neon_counter_cls]
        add     word [neon_counter_command_words], 5
        call    neon_sgp_emit_cls
.done:
        ret

neon_sgp_map_y:
        cmp     byte [video_400_mode], 0
        jne     .full
        sar     ax, 1
.full:
        ret

neon_sgp_emit_word:
        push    di
%ifdef NEON_DEBUG_LIST
        push    bp
        mov     bp, sp
        mov     bx, [ss:bp+4]
        mov     [neon_debug_low_ret_ip], bx
        pop     bp
        push    bx
        mov     bx, ds
        cmp     bx, 3000h
        je      .debug_ds_ok
        inc     word [neon_debug_bad_ds_count]
        cmp     word [neon_debug_bad_ds_count], 1
        jne     .debug_ds_restore
        mov     [neon_debug_bad_ds], bx
        mov     [neon_debug_bad_cursor], di
        mov     [neon_debug_bad_ax], ax
        mov     bx, [neon_debug_low_ret_ip]
        mov     [neon_debug_bad_ret_ip], bx
.debug_ds_restore:
        pop     bx
        jmp     .debug_ds_done
.debug_ds_ok:
        pop     bx
.debug_ds_done:
%endif
        mov     di, [neon_sgp_list_cursor]
%ifdef NEON_DEBUG_LIST
        cmp     di, neon_sgp_command_list
        jne     .debug_first_done
        inc     word [neon_debug_first_count]
        mov     [neon_debug_first_ax], ax
        push    bx
        mov     bx, ds
        mov     [neon_debug_first_ds], bx
        pop     bx
.debug_first_done:
        push    bx
        mov     bx, di
        sub     bx, neon_sgp_command_list
        cmp     bx, 3
        ja      .debug_low_done
        mov     [neon_debug_low_offset], bx
        mov     [neon_debug_low_ax], ax
        mov     [neon_debug_low_ds], ds
.debug_low_done:
        pop     bx
%endif
        cmp     di, neon_sgp_command_list_end - 2
        ja      .overflow
        ; Use the command-list segment explicitly.  Avoid string instructions
        ; and keep this path independent of any DF state leaked by a VA BIOS
        ; call on older ROMs.
        mov     [es:di], al
        mov     [es:di+1], ah
        add     di, 2
        mov     [neon_sgp_list_cursor], di
        pop     di
        ret
.overflow:
        mov     byte [neon_sgp_list_overflow], 1
        pop     di
        ret

neon_sgp_run_list:
%ifdef NEON_DEBUG_LIST
        mov     ax, [neon_debug_word0_before_submit]
        mov     bx, [neon_debug_word1_before_submit]
        mov     cx, [neon_debug_word2_before_submit]
        mov     dx, [neon_debug_word3_before_submit]
        mov     si, [neon_debug_first_ax]
        mov     di, [neon_debug_first_ds]
        mov     bp, [neon_debug_first_count]
        mov     bx, [neon_debug_low_ax]
        mov     cx, [neon_debug_low_offset]
        mov     dx, [neon_debug_low_ds]
        mov     es, [neon_debug_low_ret_ip]
        mov     ax, [neon_debug_bad_ax]
        mov     bx, [neon_debug_bad_cursor]
        mov     cx, [neon_debug_bad_ds]
        mov     dx, [neon_debug_bad_ds_count]
        mov     si, [neon_debug_bad_ret_ip]
        mov     di, [neon_debug_first_ax]
%endif
%ifdef NEON_DEBUG_LIST_HALT
        mov     si, [neon_sgp_command_address_low]
        ; The physical address is only an observation aid; the command list
        ; itself is addressed through ES in both default and external modes.
        mov     ax, [es:si]
        mov     bx, [es:si+2]
        mov     cx, [es:si+4]
        mov     dx, [es:si+6]
.debug_list_halt:
        jmp     .debug_list_halt
%endif
%ifndef NEON_SKIP_PREWAIT
        call    neon_sgp_wait_idle
        jc      .failed
%endif
        mov     dx, NEON_SGP_COMMAND_PORT
        mov     ax, [neon_sgp_command_address_low]
        out     dx, ax
        add     dx, 2
        mov     ax, [neon_sgp_command_address_high]
        out     dx, ax
        ; Keep the CPU-data GVRAM write-mode latch in the documented state.
        ; The memory-map selector (0153h) was set during VA mode setup; it is
        ; a different register from the write-mode port (0580h).
        mov     dx, NEON_GVRAM_WRITE_PORT
        mov     al, NEON_GVRAM_CPU_WRITE
        out     dx, al
        mov     dx, NEON_SGP_CONTROL_PORT
        xor     al, al
        out     dx, al
        mov     dx, NEON_SGP_STATUS_PORT
        mov     al, NEON_SGP_BUSY
        out     dx, al
%ifndef NEON_SKIP_POSTWAIT
        call    neon_sgp_wait_idle
        jc      .failed
%endif
        mov     word [neon_sgp_idle_seen], 1
        ret
.failed:
        mov     word [neon_sgp_failure_marker], 5349h
        stc
        ret

neon_sgp_wait_idle:
        mov     dx, NEON_SGP_STATUS_PORT
        mov     bx, NEON_SGP_TIMEOUT_OUTER
.outer:
        mov     cx, NEON_SGP_TIMEOUT_INNER
.poll:
        in      al, dx
        test    al, NEON_SGP_BUSY
        jz      .ready
        loop    .poll
        dec     bx
        jnz     .outer
        stc
        ret
.ready:
        clc
        ret

; Wait for one complete low-to-high VBLANK transition.  This is the same
; bounded TSP status protocol used by the validated hidden-page demo.  A
; timeout is reported to the caller rather than leaving the payload polling
; forever on a ROM/emulator with no VBLANK source.
neon_wait_vblank_start:
        mov     dx, NEON_TSP_STATUS_PORT
        mov     bx, 4
.wait_display_interval:
        mov     cx, 0ffffh
.poll_display_interval:
        in      al, dx
        test    al, NEON_TSP_VBLANK
        jz      .display_interval_seen
        loop    .poll_display_interval
        dec     bx
        jnz     .wait_display_interval
        stc
        ret
.display_interval_seen:
        mov     bx, 4
.wait_vblank_interval:
        mov     cx, 0ffffh
.poll_vblank_interval:
        in      al, dx
        test    al, NEON_TSP_VBLANK
        jnz     .ready
        loop    .poll_vblank_interval
        dec     bx
        jnz     .wait_vblank_interval
        stc
        ret
.ready:
        clc
        ret

; Select the hidden page used by the next SGP command list.  The SGP address
; and the FB0 DSA value are kept together so a profile can never submit a
; command list for one page and present another page by accident.
neon_select_draw_page:
        cmp     byte [neon_draw_page_index], 0
        jne     .page_b
        mov     ax, NEON_G0_PAGE_A_SGP_BASE & 0ffffh
        mov     [neon_g0_sgp_base_low], ax
        mov     ax, NEON_G0_PAGE_A_SGP_BASE >> 16
        mov     [neon_g0_sgp_base_high], ax
        mov     ax, NEON_G0_PAGE_A_DSA & 0ffffh
        mov     [neon_g0_draw_dsa_low], ax
        mov     ax, NEON_G0_PAGE_A_DSA >> 16
        mov     [neon_g0_draw_dsa_high], ax
        ret
.page_b:
        mov     ax, NEON_G0_PAGE_B_SGP_BASE & 0ffffh
        mov     [neon_g0_sgp_base_low], ax
        mov     ax, NEON_G0_PAGE_B_SGP_BASE >> 16
        mov     [neon_g0_sgp_base_high], ax
        mov     ax, NEON_G0_PAGE_B_DSA & 0ffffh
        mov     [neon_g0_draw_dsa_low], ax
        mov     ax, NEON_G0_PAGE_B_DSA >> 16
        mov     [neon_g0_draw_dsa_high], ax
        ret

; FB0 DSA is a pair of word registers.  Byte writes are not used here: the
; real VA contract and the existing G1 page-flip payload both require the
; low and high words to be written atomically at their respective ports.
neon_set_display_page_a:
        mov     dx, NEON_FB0_DSA_LOW_PORT
        mov     ax, NEON_G0_PAGE_A_DSA & 0ffffh
        out     dx, ax
        add     dx, 2
        mov     ax, NEON_G0_PAGE_A_DSA >> 16
        out     dx, ax
        ret

neon_set_display_page_from_draw:
        mov     dx, NEON_FB0_DSA_LOW_PORT
        mov     ax, [neon_g0_draw_dsa_low]
        out     dx, ax
        add     dx, 2
        mov     ax, [neon_g0_draw_dsa_high]
        out     dx, ax
        ret

; Rendered-page presentation boundary.  The command list and all CPU-side
; writes for the current page have completed before this function is called.
; After the VBLANK flip, the page variables are switched so the next frame
; targets the page that is no longer visible.
neon_present_draw_page:
        call    neon_wait_vblank_start
        jc      .failed
        call    neon_set_display_page_from_draw
        inc     word [neon_page_flip_count]
        xor     byte [neon_draw_page_index], 1
        call    neon_select_draw_page
        clc
        ret
.failed:
        mov     word [neon_sgp_failure_marker], 5642h
        stc
        ret

neon_sgp_physical_address_from_ds_si:
        push    cx
        mov     ax, ds
        xor     dx, dx
        mov     cx, 4
.shift:
        shl     ax, 1
        rcl     dx, 1
        loop    .shift
        add     ax, si
        adc     dx, 0
        pop     cx
        ret

; Convert ES:SI to the physical address consumed by the SGP command port.
; The normal build has ES=DS=3000h; the capacity experiment deliberately
; keeps the command list in a separate RAM segment.
neon_sgp_physical_address_from_es_si:
        push    cx
        push    es
        pop     ax
        xor     dx, dx
        mov     cx, 4
.shift:
        shl     ax, 1
        rcl     dx, 1
        loop    .shift
        add     ax, si
        adc     dx, 0
        pop     cx
        ret

; Water-raster pixels are deliberately not a separate SGP primitive in this
; counter build.  They are counted as no-op CPU pixels until the VA backend
; defines the corresponding documented operation.
pixel_set:
pixel_set_same_colour:
pixel_set_same_colour_fast:
        ret

grcg_prepare_color:
grcg_disable:
        ret

set_display_page:
set_access_page:
        ret

clear_graphics_page:
        pusha
        call    line_batch_begin
        inc     word [neon_counter_cls]
        add     word [neon_counter_command_words], 5
        mov     ax, [neon_g0_sgp_base_low]
        mov     dx, [neon_g0_sgp_base_high]
%ifdef NEON_MINIMAL_SGP
        mov     cx, 1
%else
        mov     cx, [neon_sgp_page_words]
%endif
        call    neon_sgp_emit_cls
        popa
        ret

; Count each accepted triangle at the scene primitive boundary.  The actual
; implementation is included under a private name so P3 can prove that the
; scene remains the original faithful geometry while the backend is replaced.
        %define city286f_fill_triangle neon_counter_triangle_impl
%include "../../neon3_1_5/98/CITY3D286_CORE.INC"
%include "../../neon3_1_5/98/CITY3D286_FAITHFUL.INC"
        %undef  city286f_fill_triangle

city286f_fill_triangle:
        pusha
        inc     word [neon_counter_triangle_calls]
        popa
        jmp     neon_counter_triangle_impl

%include "../../neon3_1_5/98/SCENE3_256.INC"

; Keep the raw payload entry at offset zero.  The original mutable state is
; data, not an entry stub, so it must follow the code for the local loader's
; fixed 3000:0000 transfer contract.
%include "../../neon3_1_5/98/DATA3_286.INC"

neon_counter_record_frame:
        pusha
        mov     ax, [neon_counter_line_calls]
        cmp     ax, [neon_counter_max_line_calls]
        jbe     .line_done
        mov     [neon_counter_max_line_calls], ax
.line_done:
        mov     ax, [neon_counter_triangle_calls]
        cmp     ax, [neon_counter_max_triangle_calls]
        jbe     .triangle_done
        mov     [neon_counter_max_triangle_calls], ax
.triangle_done:
        mov     ax, [neon_counter_triangle_spans]
        cmp     ax, [neon_counter_max_triangle_spans]
        jbe     .span_done
        mov     [neon_counter_max_triangle_spans], ax
.span_done:
        mov     ax, [neon_counter_fill_rect_rows]
        cmp     ax, [neon_counter_max_fill_rect_rows]
        jbe     .rows_done
        mov     [neon_counter_max_fill_rect_rows], ax
.rows_done:
        mov     ax, [neon_counter_fill_rect_spans]
        cmp     ax, [neon_counter_max_fill_rect_spans]
        jbe     .rect_span_done
        mov     [neon_counter_max_fill_rect_spans], ax
.rect_span_done:
        mov     ax, [neon_counter_set_color]
        cmp     ax, [neon_counter_max_set_color]
        jbe     .colour_done
        mov     [neon_counter_max_set_color], ax
.colour_done:
        mov     ax, [neon_counter_cls]
        cmp     ax, [neon_counter_max_cls]
        jbe     .cls_done
        mov     [neon_counter_max_cls], ax
.cls_done:
        mov     ax, [neon_counter_command_words]
        cmp     ax, [neon_counter_max_command_words]
        jbe     .commands_done
        mov     [neon_counter_max_command_words], ax
.commands_done:
        inc     word [neon_counter_frame_index]
        mov     word [neon_counter_line_calls], 0
        mov     word [neon_counter_triangle_calls], 0
        mov     word [neon_counter_triangle_spans], 0
        mov     word [neon_counter_fill_rect_rows], 0
        mov     word [neon_counter_fill_rect_spans], 0
        mov     word [neon_counter_set_color], 0
        mov     word [neon_counter_cls], 0
        mov     word [neon_counter_command_words], 0
        mov     byte [neon_counter_last_color], 0ffh
        popa
        ret

; QA-readable state.  The host runner will export these words through a VA
; text/trace layer in the next P3 step; they are intentionally plain 16-bit
; values so a VAEG debugger can inspect them without DOS services.
neon_counter_line_calls             dw 0
neon_counter_triangle_calls         dw 0
neon_counter_triangle_spans         dw 0
neon_counter_fill_rect_rows         dw 0
neon_counter_fill_rect_spans        dw 0
neon_counter_set_color              dw 0
neon_counter_cls                    dw 0
neon_counter_end                    dw 0
neon_counter_command_words          dw 0
neon_counter_max_line_calls         dw 0
neon_counter_max_triangle_calls     dw 0
neon_counter_max_triangle_spans     dw 0
neon_counter_max_fill_rect_rows     dw 0
neon_counter_max_fill_rect_spans    dw 0
neon_counter_max_set_color          dw 0
neon_counter_max_cls                dw 0
neon_counter_max_command_words      dw 0
neon_counter_frame_index            dw 0
neon_counter_last_color             db 0ffh
neon_text_last_scene                db 0ffh
        align 2
; This legacy diagnostic reserve has no runtime references; keep a small
; alignment cushion without letting the status-row cleanup cross the loader
; return reserve at E000h.
neon_counter_stack                 times 224 dw 0
neon_counter_stack_top:

align 2
neon_sgp_work_area:
        times 58 db 0
%ifdef NEON_SGP_EXTERNAL_LIST
neon_sgp_command_list equ NEON_SGP_LIST_OFFSET
neon_sgp_command_list_end equ neon_sgp_command_list + NEON_SGP_LIST_CAPACITY
%else
neon_sgp_command_list:
        times NEON_SGP_LIST_CAPACITY db 0
neon_sgp_command_list_end:
%endif

neon_sgp_list_cursor              dw 0
neon_sgp_command_address_low      dw 0
neon_sgp_command_address_high     dw 0
neon_sgp_last_color                dw 0ffffh
neon_sgp_page_words               dw 0
neon_sgp_frame_active              db 0
neon_sgp_list_overflow             db 0
neon_sgp_resume_color              dw 0ffffh
neon_draw_page_index               db 1
        align 2
neon_g0_sgp_base_low               dw NEON_G0_PAGE_B_SGP_BASE & 0ffffh
neon_g0_sgp_base_high              dw NEON_G0_PAGE_B_SGP_BASE >> 16
neon_g0_draw_dsa_low               dw NEON_G0_PAGE_B_DSA & 0ffffh
neon_g0_draw_dsa_high              dw NEON_G0_PAGE_B_DSA >> 16
neon_page_flip_count               dw 0
neon_sgp_failure_marker            dw 0
neon_bios_failure_marker            dw 0
neon_bios_return_code               dw 0
neon_sgp_idle_seen                  dw 0
neon_escape_seen                    db 0
neon_sgp_rect_x0                   dw 0
neon_sgp_rect_x1                   dw 0
neon_sgp_rect_y                    dw 0
neon_sgp_span_x0                   dw 0
neon_sgp_span_x1                   dw 0
neon_sgp_span_y                    dw 0
neon_sgp_span_first_word           dw 0
neon_sgp_span_last_word            dw 0
neon_sgp_line_x0                   dw 0
neon_sgp_line_y0                   dw 0
neon_sgp_line_x1                   dw 0
neon_sgp_line_y1                   dw 0
neon_sgp_line_y0_physical          dw 0
neon_sgp_line_y1_physical          dw 0
neon_sgp_line_width                dw 0
neon_sgp_line_height               dw 0
%ifdef NEON_DEBUG_LIST
neon_debug_word0_before_submit     dw 0
neon_debug_word1_before_submit     dw 0
neon_debug_word2_before_submit     dw 0
neon_debug_word3_before_submit     dw 0
neon_debug_first_ax                dw 0
neon_debug_first_ds                dw 0
neon_debug_first_count             dw 0
neon_debug_low_offset              dw 0
neon_debug_low_ax                  dw 0
neon_debug_low_ds                  dw 0
neon_debug_low_ret_ip               dw 0
neon_debug_bad_ds_count             dw 0
neon_debug_bad_ds                   dw 0
neon_debug_bad_cursor               dw 0
neon_debug_bad_ax                   dw 0
neon_debug_bad_ret_ip               dw 0
%endif

; Keep BIOS descriptor lists on a paragraph boundary.  The documented ABI
; describes ES:DI as a word list; paragraph alignment also matches the proven
; GLASS VA payload and avoids relying on an undocumented odd list placement.
align 16
neon_framebuffer_descriptor:
        dw 4, SCREEN_W, SCREEN_H
align 16
neon_window_descriptor:
        dw 0, 0, VIDEO_H, 0, 0

neon_palette:
        ; VA $SetPal accepts the 12-bit RGB value in the hardware layout:
        ; R[15:12], G[9:6], B[4:1].  These entries preserve the original
        ; NEON G,R,B nibbles from VIDEO3_286.INC after placing each channel
        ; in the documented VA position.  The BIOS/emulator expands this
        ; 12-bit value to the native RGB565 palette register.
        dw 0000h, 410eh, 4154h, 5198h
        dw 81d8h, 0a210h, 0b29ah, 0f3deh
        dw 43deh, 0f11eh, 0c318h, 0f3c8h
        dw 215eh, 0a30ch, 0f3d6h, 0f3deh

neon_hex_digits db '0123456789ABCDEF'
neon_scene_title db 'NEON3 VA SGP', 0
neon_scene_title_len equ $ - neon_scene_title
%ifdef NEON_PROFILE_400
neon_scene_profile db '640X400 16-COLOR', 0
%else
neon_scene_profile db '640X200 16-COLOR', 0
%endif
neon_scene_profile_len equ $ - neon_scene_profile
%ifdef NEON_DEBUG_UNIQUE_IDLE_HALT
align 16
neon_debug_unique_idle_halt:
        jmp     neon_debug_unique_idle_halt
%endif
neon_status_title db 'NEON3 P3 COUNTER', 0
neon_status_title_len equ $ - neon_status_title
neon_live_title db 'NEON3 // LIVE FRAME STATUS', 0
neon_live_frame db 'FRAME (HEX):', 0
neon_live_local db 'LOCAL FRAME (HEX):', 0
neon_live_scene_title db 'SCENE TITLE:', 0
neon_live_limit db 'TOTAL FRAMES (HEX):', 0
%ifdef NEON_PROFILE_400
neon_status_profile db 'PROFILE: 640X400 / G0 / 4BPP', 0
%else
neon_status_profile db 'PROFILE: 640X200 / G0 / 4BPP', 0
%endif
neon_status_profile_len equ $ - neon_status_profile
neon_status_line db 'MAX LINE:          ', 0
neon_status_line_len equ $ - neon_status_line
neon_status_triangle db 'MAX TRIANGLE:      ', 0
neon_status_triangle_len equ $ - neon_status_triangle
neon_status_span db 'MAX TRI SPAN:      ', 0
neon_status_span_len equ $ - neon_status_span
neon_status_rect db 'MAX RECT ROW:      ', 0
neon_status_rect_len equ $ - neon_status_rect
neon_status_color db 'MAX SET COLOR:     ', 0
neon_status_color_len equ $ - neon_status_color
neon_status_cls db 'MAX CLS:           ', 0
neon_status_cls_len equ $ - neon_status_cls
neon_status_words db 'MAX COMMAND WORDS: ', 0
neon_status_words_len equ $ - neon_status_words
neon_status_frames db 'FRAMES: ', 0
neon_status_frames_len equ $ - neon_status_frames
neon_status_limit db 'LIMIT: ', 0
neon_status_limit_len equ $ - neon_status_limit
neon_status_bios db 'BIOS: ', 0
neon_status_bios_len equ $ - neon_status_bios
neon_status_bios_rc db 'BIOS RC: ', 0
neon_status_bios_rc_len equ $ - neon_status_bios_rc
neon_status_sgp db 'SGP: ', 0
neon_status_sgp_len equ $ - neon_status_sgp
neon_status_exit db 'ESC: PRESS ESC TO EXIT', 0
neon_status_blank_line times 80 db ' '
                        db 0
neon_status_exit_len equ $ - neon_status_exit
neon_status_ok db 'OK', 0
neon_status_ok_len equ $ - neon_status_ok
neon_status_idle db 'IDLE', 0
neon_status_idle_len equ $ - neon_status_idle
neon_status_fail db 'FAIL', 0
neon_status_hex_buffer times 5 db 0
neon_status_fail_len equ $ - neon_status_fail

%if ($ - $$) > 0e000h
%error "NEON3 payload overlaps the loader return reserve"
%endif
