; GLASS ORBIT original work:
; Developed by ChatGPT Plus
; Supervised by SimK, Neko Project 21/W Developer
; Ported By Maho Nakata, 2026.
;
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

; GLASS ORBIT P4-2 SGP fixed-frame renderer.
;
; The program shares only the preserved cube geometry/data with P4-1.  It
; constructs an SGP list in main RAM: SET WORK, SET COLOR, CLS, per-span CLS,
; per-edge LINE, and END.  It never uses the CPU G0 aperture before the SGP
; has completed.  After completion it applies exact packed-word endpoint RMW
; masks and then redraws the edge-only SGP list.  Evidence:
; docs/agents/reports/m97_p4_visual_holes.md.

cpu 286
bits 16
org 0

; Diagnostic build stages isolate the first list section that cannot complete.
; Stage 1 emits SET_WORK, SET_COLOR, full-page CLS, END.  Stage 2 adds face
; spans.  Stage 3 is the P4-2 candidate and adds the twelve cube-edge LINEs.
; Stage 4 emits only the cube-edge LINEs on a cleared page for registration
; diagnostics; stage 5 emits a direct one-pixel word-layout calibration strip.
; Neither diagnostic stage changes the production renderer.
; The selected stage changes the generated guest payload only; it does not
; alter VAEG's SGP behaviour.
%ifndef GLASS_P4_SGP_STAGE
%define GLASS_P4_SGP_STAGE 3
%endif
%ifndef GLASS_P4_SGP_AUDIT
%define GLASS_P4_SGP_AUDIT 0
%endif
%ifndef GLASS_P5
%define GLASS_P5 0
%endif
%if GLASS_P4_SGP_STAGE < 1 || GLASS_P4_SGP_STAGE > 5
%error "GLASS_P4_SGP_STAGE must be 1, 2, 3, 4, or 5"
%endif

%define VIDEO_BIOS_INT          0x8f
%define KEYBOARD_BIOS_INT       0x82
%define PORT_MEMORY_MAP         0x0153
%define PORT_GVRAM_WRITE_MODE   0x0580
%define PORT_SGP_COMMAND        0x0500
%define PORT_SGP_CONTROL        0x0504
%define PORT_SGP_STATUS         0x0506
%define MODE_G0_640X200_4BPP    0xa002
%define PIXEL_SIZE_G0_4BPP      0x0004
%define COMPOSE_G0_ONLY         0x0003
%define MEMORY_MAP_GVRAM_SINGLE 0x54
%define GVRAM_CPU_WRITE_MODE    0x10
%define G0_SGP_BASE             0x200000
%define G0_PAGE_WORDS           0x7d00
%define G0_WIDTH                640
%define G0_HEIGHT               200
%define G0_PITCH_BYTES          320
; The CPU aperture is one 64-KiB window; this diagnostic payload uses an
; otherwise uninteresting row inside that window.  The host verifier masks
; this range before running visible-pixel geometry checks.
%define G0_AUDIT_GVRAM_OFFSET   0x2000
%define SGP_BUSY                0x01
%define SGP_COMMAND_END         0x0001
%define SGP_COMMAND_SET_WORK    0x0003
%define SGP_COMMAND_SET_COLOR   0x0006
%define SGP_COMMAND_LINE        0x0009
%define SGP_COMMAND_CLS         0x000a
%define SGP_LINE_COPY           0x0005
%define SGP_LINE_HD             0x0400
%define SGP_LINE_VD             0x0800
%define LOADER_RETURN_SS        0xe000
%define LOADER_RETURN_SP        0xe002
%define LOADER_RETURN_FLAGS     0xe004
%define LOADER_RETURN_MAGIC     0xe006
%define LOADER_RETURN_SIGNATURE 0x5034

glass_p4_sgp_entry:
        cli
        mov     ax, cs
        mov     ds, ax
        mov     es, ax
        mov     ss, ax
        mov     sp, 0xf000
        cld
        sti

        mov     bx, MODE_G0_640X200_4BPP
        mov     cx, PIXEL_SIZE_G0_4BPP
        xor     dx, dx
        xor     ax, ax                 ; $ScnMode
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_p4_sgp_failed

        push    cs
        pop     es
        mov     ax, 0x0100             ; $DefBuf: G0 640x400 4bpp FB0.
        mov     cx, 1
        mov     di, glass_p4_sgp_framebuffer_descriptor
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_p4_sgp_failed

        push    cs
        pop     es
        mov     ax, 0x0200             ; $DefWin: first 200 source rows.
        mov     cx, 1
        mov     di, glass_p4_sgp_window_descriptor
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_p4_sgp_failed

        mov     ax, 0x0900             ; $PalCtl, palette mode 0.
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_p4_sgp_failed
        call    glass_p4_sgp_set_palette
        jc      glass_p4_sgp_failed

        mov     ax, 0x0300             ; $Compose: G0 only.
        mov     cx, COMPOSE_G0_ONLY
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_p4_sgp_failed
        mov     ax, 0x0b01             ; $ScnDsp: graphics on.
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_p4_sgp_failed

        mov     dx, PORT_MEMORY_MAP
        mov     al, MEMORY_MAP_GVRAM_SINGLE
        out     dx, al
        mov     dx, PORT_GVRAM_WRITE_MODE
        mov     al, GVRAM_CPU_WRITE_MODE
        out     dx, al

        ; P4 uses the visible FB0 source half.  P5 replaces these values with
        ; the hidden FB0 source half before each frame build.
        mov     word [glass_p4_sgp_target_base_low], G0_SGP_BASE & 0xffff
        mov     word [glass_p4_sgp_target_base_high], G0_SGP_BASE >> 16
        mov     word [glass_p4_sgp_target_cpu_segment], 0xa000

%if GLASS_P5
        call    glass_p5_run
        jmp     glass_p4_sgp_idle
%else
        mov     word [render_frame_counter], 0
        call    glass_compute_cube
        ; Diagnostic-only projection export for the independent host oracle.
        ; It is placed in an unused GVRAM page and does not participate in
        ; rendering or SGP command construction.
        mov     si, glass_cube_projected
        mov     ax, 0xa000
        mov     es, ax
        mov     di, 0xfc00
        mov     cx, 24
        rep     movsw
        push    cs
        pop     es
        call    glass_p4_sgp_build_list
        jc      glass_p4_sgp_failed
        ; Keep the generated-list size observable at a fixed pre-submit
        ; checkpoint.  It is diagnostic-only and does not participate in SGP
        ; execution.
times 0x0240 - ($ - $$) db 0x90
        mov     bx, [glass_p4_sgp_list_bytes]
glass_p4_sgp_build_ready:
        nop
        call    glass_p4_sgp_run_list
        jc      glass_p4_sgp_failed

        ; SGP has written only complete interior words.  Apply the recorded
        ; endpoint masks with CPU word read-modify-write, then redraw the
        ; intended outline list so edge colours remain on top of face coverage.
        call    glass_p4_sgp_apply_endpoint_spans
%if GLASS_P4_SGP_STAGE == 3 || GLASS_P4_SGP_STAGE == 4
        call    glass_p4_sgp_build_edge_list
        jc      glass_p4_sgp_failed
        call    glass_p4_sgp_run_list
        jc      glass_p4_sgp_failed
%endif

%if GLASS_P4_SGP_STAGE == 5
        ; Independent calibration writes.  This deliberately does not call
        ; glass_p4_sgp_pixel_masks: each x%4 case is an explicit literal so
        ; the captured raw words can calibrate that production helper.
        call    glass_p4_sgp_calibrate_word_layout
%endif

%if GLASS_P4_SGP_AUDIT
        call    glass_p4_sgp_export_span_audit
%endif

        ; Read only after END has made the SGP idle.  This checksum is a
        ; narrow stability witness; P4-2's host comparator owns the complete
        ; CPU-versus-SGP visible-pixel comparison.
        mov     ax, 0xa000
        mov     es, ax
        call    glass_p4_sgp_raw_checksum
        push    cs
        pop     es
        mov     ax, 0x4753             ; "GS" P4-2 SGP success marker.
        jmp     glass_p4_sgp_idle
%endif

glass_p4_sgp_failed:
        push    cs
        pop     es
        xor     bx, bx
        mov     ax, 0x47e2             ; debugger-visible P4-2 failure marker.

times 0x0280 - ($ - $$) db 0
glass_p4_sgp_idle:
        call    glass_p4_sgp_escape_pressed
        jc      glass_p4_sgp_exit
        hlt
        jmp     glass_p4_sgp_idle

glass_p4_sgp_escape_pressed:
        mov     ah, 0x01
        int     KEYBOARD_BIOS_INT
        jc      .none
        mov     ah, 0x00
        int     KEYBOARD_BIOS_INT
        cmp     bh, 0
        jne     .none
        cmp     bl, 0x1b
        je      .escape
.none:
        clc
        ret
.escape:
        stc
        ret

glass_p4_sgp_exit:
        cmp     word [cs:LOADER_RETURN_MAGIC], LOADER_RETURN_SIGNATURE
        jne     glass_p4_sgp_idle
        mov     ax, 0x0b00             ; $ScnDsp: graphics off.
        int     VIDEO_BIOS_INT
        mov     ax, 0x0300             ; $Compose: text only.
        mov     cx, 0x0001
        int     VIDEO_BIOS_INT
        cli
        mov     ax, [cs:LOADER_RETURN_SS]
        mov     ss, ax
        mov     sp, [cs:LOADER_RETURN_SP]
        push    word [cs:LOADER_RETURN_FLAGS]
        popf
        retf

glass_p4_sgp_set_palette:
        push    ax
        push    bx
        push    cx
        push    bp
        push    si
        xor     bx, bx
        mov     si, glass_p4_sgp_palette
        mov     bp, 16
.entry:
        mov     ax, 0x0800             ; $SetPal: AL=index, CX=value.
        mov     al, bl
        mov     cx, [si]
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
        inc     bl
        add     si, 2
        dec     bp
        jnz     .entry
        clc
        jmp     .done
.failed:
        stc
.done:
        pop     si
        pop     bp
        pop     cx
        pop     bx
        pop     ax
        ret

; Assemble the complete fixed-frame list before sending its physical address
; to the SGP.  Each word is capacity-checked to fail closed before a list
; overrun can alter adjacent payload data.
glass_p4_sgp_build_list:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    es
        push    ds
        pop     es
        mov     word [glass_p4_sgp_list_cursor], glass_p4_sgp_command_list
        mov     word [glass_p4_sgp_endpoint_cursor], glass_p4_sgp_endpoint_buffer
        mov     byte [glass_p4_sgp_list_overflow], 0
        mov     word [glass_p4_sgp_last_colour], 0xffff

        mov     ax, SGP_COMMAND_SET_WORK
        call    glass_p4_sgp_emit_word
        mov     si, glass_p4_sgp_work_area
        call    glass_p4_sgp_physical_address_from_ds_si
        call    glass_p4_sgp_emit_word
        mov     ax, dx
        call    glass_p4_sgp_emit_word

        xor     ax, ax
        call    glass_p4_sgp_emit_set_colour
        mov     ax, [glass_p4_sgp_target_base_low]
        mov     dx, [glass_p4_sgp_target_base_high]
        mov     cx, G0_PAGE_WORDS
        call    glass_p4_sgp_emit_cls

%if GLASS_P5
        call    glass_p5_draw_grid
%endif
%if GLASS_P4_SGP_STAGE == 2 || GLASS_P4_SGP_STAGE == 3
        call    glass_p4_sgp_draw_faces
%endif
%if GLASS_P4_SGP_STAGE == 3 || GLASS_P4_SGP_STAGE == 4
        call    glass_p4_sgp_draw_edges
%endif
        mov     ax, SGP_COMMAND_END
        call    glass_p4_sgp_emit_word

        cmp     byte [glass_p4_sgp_list_overflow], 0
        jne     .failed
        mov     ax, [glass_p4_sgp_list_cursor]
        sub     ax, glass_p4_sgp_command_list
        mov     [glass_p4_sgp_list_bytes], ax
        mov     si, glass_p4_sgp_command_list
        call    glass_p4_sgp_physical_address_from_ds_si
        mov     [glass_p4_sgp_command_address_low], ax
        mov     [glass_p4_sgp_command_address_high], dx
        clc
        jmp     .done
.failed:
        stc
.done:
        pop     es
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; Build a second list containing only the outlines.  Endpoint RMW is applied
; between the face list and this list, so exact face coverage cannot overwrite
; the red/green/white edge colours.
glass_p4_sgp_build_edge_list:
        push    ax
        push    si
        push    es
        push    ds
        pop     es
        mov     word [glass_p4_sgp_list_cursor], glass_p4_sgp_command_list
        mov     byte [glass_p4_sgp_list_overflow], 0
        mov     word [glass_p4_sgp_last_colour], 0xffff
        call    glass_p4_sgp_draw_edges
        mov     ax, SGP_COMMAND_END
        call    glass_p4_sgp_emit_word
        cmp     byte [glass_p4_sgp_list_overflow], 0
        jne     .failed
        mov     ax, [glass_p4_sgp_list_cursor]
        sub     ax, glass_p4_sgp_command_list
        mov     [glass_p4_sgp_list_bytes], ax
        mov     si, glass_p4_sgp_command_list
        call    glass_p4_sgp_physical_address_from_ds_si
        mov     [glass_p4_sgp_command_address_low], ax
        mov     [glass_p4_sgp_command_address_high], dx
        clc
        jmp     .done
.failed:
        stc
.done:
        pop     es
        pop     si
        pop     ax
        ret

; AL contains a palette index.  SET COLOR uses one packed copy in every
; 4-bpp nibble, so palette index 8 becomes 8888h.
glass_p4_sgp_emit_set_colour_index:
        push    bx
        and     ax, 0x000f
        mov     bx, ax
        shl     bx, 4
        or      ax, bx
        mov     bx, ax
        shl     bx, 8
        or      ax, bx
        pop     bx
        jmp     glass_p4_sgp_emit_set_colour

; AX is a complete packed color word.  Suppress adjacent identical commands.
glass_p4_sgp_emit_set_colour:
        cmp     ax, [glass_p4_sgp_last_colour]
        je      .done
        mov     [glass_p4_sgp_last_colour], ax
        push    ax
        mov     ax, SGP_COMMAND_SET_COLOR
        call    glass_p4_sgp_emit_word
        pop     ax
        call    glass_p4_sgp_emit_word
.done:
        ret

; DX:AX is an even physical word address and CX is a nonzero word count.
glass_p4_sgp_emit_cls:
        push    ax
        mov     ax, SGP_COMMAND_CLS
        call    glass_p4_sgp_emit_word
        pop     ax
        call    glass_p4_sgp_emit_word
        mov     ax, dx
        call    glass_p4_sgp_emit_word
        mov     ax, cx
        call    glass_p4_sgp_emit_word
        xor     ax, ax
        call    glass_p4_sgp_emit_word
        ret

; Build each camera-facing quad with one convex-polygon scan conversion.  The
; CPU performs only geometry and list construction; every visible span is an
; SGP CLS command.  The standalone triangle primitive remains available for
; independent QA, but is not used for P4 face rendering.
glass_p4_sgp_draw_faces:
        push    ax
        push    bx
        push    bp
        push    si
        push    di
        mov     si, glass_cube_faces
        mov     bp, 6
        mov     byte [glass_p4_sgp_face_id], 0
.face:
        call    glass_face_is_visible
        jnc     .next
        xor     bx, bx
        mov     bl, [si+4]
        mov     al, [glass_p4_sgp_face_colour_map + bx]
        mov     [glass_p4_sgp_draw_colour], al
        call    glass_p4_sgp_emit_set_colour_index

        mov     al, [si]
        mov     di, glass_p4_sgp_poly_v0
        call    glass_p4_sgp_load_vertex
        mov     al, [si+1]
        mov     di, glass_p4_sgp_poly_v1
        call    glass_p4_sgp_load_vertex
        mov     al, [si+2]
        mov     di, glass_p4_sgp_poly_v2
        call    glass_p4_sgp_load_vertex
        mov     al, [si+3]
        mov     di, glass_p4_sgp_poly_v3
        call    glass_p4_sgp_load_vertex
        sar     word [glass_p4_sgp_poly_v0+2], 1
        sar     word [glass_p4_sgp_poly_v1+2], 1
        sar     word [glass_p4_sgp_poly_v2+2], 1
        sar     word [glass_p4_sgp_poly_v3+2], 1
        push    si
        mov     si, glass_p4_sgp_poly_v0
        mov     cx, 4
        call    glass_p4_convex_fill_polygon
        pop     si
.next:
        add     si, 5
        inc     byte [glass_p4_sgp_face_id]
        dec     bp
        jnz     .face
        pop     di
        pop     si
        pop     bp
        pop     bx
        pop     ax
        ret

; Emit all retained cube edges after the faces, preserving the source edge
; colors.  The SGP LINE command performs every GVRAM pixel update.
glass_p4_sgp_draw_edges:
        push    ax
        push    bx
        push    bp
        push    si
        mov     si, glass_cube_edges
        mov     bp, 12
.edge:
        mov     al, [si+2]
        mov     [glass_p4_sgp_draw_colour], al
        call    glass_p4_sgp_emit_set_colour_index
        xor     bx, bx
        mov     bl, [si]
        imul    bx, bx, 6
        mov     ax, [glass_cube_projected + bx]
        mov     [glass_p4_sgp_line_x0], ax
        mov     ax, [glass_cube_projected + bx + 2]
        mov     [glass_p4_sgp_line_y0], ax
        xor     bx, bx
        mov     bl, [si+1]
        imul    bx, bx, 6
        mov     ax, [glass_cube_projected + bx]
        mov     [glass_p4_sgp_line_x1], ax
        mov     ax, [glass_cube_projected + bx + 2]
        mov     [glass_p4_sgp_line_y1], ax
        call    glass_p4_sgp_emit_line
        add     si, 3
        dec     bp
        jnz     .edge
        pop     si
        pop     bp
        pop     bx
        pop     ax
        ret

; AL is a retained cube-vertex index and DI receives its logical x/y pair.
glass_p4_sgp_load_vertex:
        push    ax
        push    bx
        xor     bx, bx
        mov     bl, al
        imul    bx, bx, 6
        mov     ax, [glass_cube_projected + bx]
        mov     [di], ax
        mov     ax, [glass_cube_projected + bx + 2]
        mov     [di+2], ax
        pop     bx
        pop     ax
        ret

; Convert the unchanged logical Y coordinates exactly once, sort the triangle,
; then emit an inclusive span for each physical row.  The edge rule samples
; the row at y+1/2 and uses integer X edge ownership: ceil() for the left
; boundary and floor() for the right boundary.  Endpoint masks preserve this
; exact logical coverage while SGP handles only complete interior words.
glass_p4_sgp_fill_triangle:
        push    bp
        sar     word [glass_p4_sgp_tri_v0+2], 1
        sar     word [glass_p4_sgp_tri_v1+2], 1
        sar     word [glass_p4_sgp_tri_v2+2], 1
        mov     si, glass_p4_sgp_tri_v0
        mov     di, glass_p4_sgp_tri_v1
        call    glass_p4_sgp_swap_if_y_greater
        mov     si, glass_p4_sgp_tri_v1
        mov     di, glass_p4_sgp_tri_v2
        call    glass_p4_sgp_swap_if_y_greater
        mov     si, glass_p4_sgp_tri_v0
        mov     di, glass_p4_sgp_tri_v1
        call    glass_p4_sgp_swap_if_y_greater

        mov     ax, [glass_p4_sgp_tri_v0+2]
        cmp     ax, [glass_p4_sgp_tri_v2+2]
        je      .done
        mov     [glass_p4_sgp_scan_y], ax
.scan:
        mov     ax, [glass_p4_sgp_scan_y]
        cmp     ax, [glass_p4_sgp_tri_v2+2]
        jge     .done
        mov     bx, ax
        mov     si, glass_p4_sgp_tri_v0
        mov     di, glass_p4_sgp_tri_v2
        mov     bp, 1                 ; ceil(x_intersection - 1/2)
        call    glass_p4_sgp_interpolate_edge
        mov     [glass_p4_sgp_edge_a_ceil], ax
        xor     bp, bp                ; floor(x_intersection - 1/2)
        call    glass_p4_sgp_interpolate_edge
        mov     [glass_p4_sgp_edge_a_floor], ax

        mov     bx, [glass_p4_sgp_scan_y]
        cmp     bx, [glass_p4_sgp_tri_v1+2]
        jl      .upper
        mov     si, glass_p4_sgp_tri_v1
        mov     di, glass_p4_sgp_tri_v2
        jmp     .second_edge
.upper:
        mov     si, glass_p4_sgp_tri_v0
        mov     di, glass_p4_sgp_tri_v1
.second_edge:
        mov     bp, 1                 ; ceil(x_intersection - 1/2)
        call    glass_p4_sgp_interpolate_edge
        mov     [glass_p4_sgp_edge_b_ceil], ax
        xor     bp, bp                ; floor(x_intersection - 1/2)
        call    glass_p4_sgp_interpolate_edge
        mov     [glass_p4_sgp_edge_b_floor], ax
        mov     ax, [glass_p4_sgp_edge_a_ceil]
        cmp     ax, [glass_p4_sgp_edge_b_ceil]
        jle     .lower_ready
        mov     ax, [glass_p4_sgp_edge_b_ceil]
.lower_ready:
        mov     [glass_p4_sgp_span_x0], ax
        mov     ax, [glass_p4_sgp_edge_a_floor]
        cmp     ax, [glass_p4_sgp_edge_b_floor]
        jge     .upper_ready
        mov     ax, [glass_p4_sgp_edge_b_floor]
.upper_ready:
        mov     [glass_p4_sgp_span_x1], ax
        mov     ax, [glass_p4_sgp_span_x0]
        mov     bx, [glass_p4_sgp_span_x1]
        mov     cx, [glass_p4_sgp_scan_y]
        call    glass_p4_sgp_emit_span
        inc     word [glass_p4_sgp_scan_y]
        jmp     .scan
.done:
        pop     bp
        ret

; SI and DI are x/y pairs; BX is physical Y.  Return the x-boundary at the
; row sample y+1/2.  BP=1 returns ceil(x), BP=0 returns floor(x), using the
; deterministic integer-X edge ownership of the face rasterizer.  The
; denominator is positive because vertices were sorted by Y.  The caller
; never supplies a horizontal edge here.
glass_p4_sgp_interpolate_edge:
        push    bx
        push    cx
        push    dx
        mov     cx, [di+2]
        sub     cx, [si+2]
        jz      .degenerate
        mov     ax, bx
        sub     ax, [si+2]
        shl     ax, 1
        inc     ax
        mov     dx, [di]
        sub     dx, [si]
        imul    dx
        shl     cx, 1
        idiv    cx
        or      dx, dx
        jz      .add_origin
        test    dx, 8000h
        jnz     .negative_remainder
        cmp     bp, 0
        je      .add_origin
        inc     ax
        jmp     .add_origin
.negative_remainder:
        cmp     bp, 0
        jne     .add_origin
        dec     ax
.add_origin:
        add     ax, [si]
        jmp     .done
.degenerate:
        mov     ax, [si]
.done:
        pop     dx
        pop     cx
        pop     bx
        ret

glass_p4_sgp_swap_if_y_greater:
        push    ax
        mov     ax, [si+2]
        cmp     ax, [di+2]
        jle     .done
        xchg    ax, [di+2]
        mov     [si+2], ax
        mov     ax, [si]
        xchg    ax, [di]
        mov     [si], ax
.done:
        pop     ax
        ret

; AX/BX are inclusive logical X bounds and CX is a physical Y.  One packed
; 4bpp word contains four logical pixels.  Geometry is never rounded.  The
; word ownership partition is explicit: first_word=x0>>2,
; last_word=x1>>2, full_first=first_word+(x0&3!=0), and
; full_last=last_word-(x1&3!=3).  Only the complete [full_first,full_last]
; words are submitted to SGP.  Every other covered pixel is applied later by
; exact CPU word RMW, so a partial endpoint can never be written as a full
; SGP word, even transiently.  AX>BX is an empty raster span and must be
; discarded; swapping it would create a pixel outside the polygon.
glass_p4_sgp_emit_span:
        cmp     ax, bx
        jg      .done
        cmp     bx, 0
        jl      .done
        cmp     ax, G0_WIDTH - 1
        jg      .done
        cmp     ax, 0
        jge     .left_clipped
        xor     ax, ax
.left_clipped:
        cmp     bx, G0_WIDTH - 1
        jle     .right_clipped
        mov     bx, G0_WIDTH - 1
.right_clipped:
        mov     [glass_p4_sgp_exact_x0], ax
        mov     [glass_p4_sgp_exact_x1], bx
        mov     [glass_p4_sgp_exact_y], cx
        ; Partition by word indices, never by outward pixel-coordinate
        ; rounding.  A same-word or endpoint-only span has zero full words.
        mov     ax, [glass_p4_sgp_exact_x0]
        shr     ax, 2
        mov     [glass_p4_sgp_span_first_word], ax
        mov     bx, [glass_p4_sgp_exact_x1]
        shr     bx, 2
        mov     [glass_p4_sgp_span_last_word], bx
        mov     dx, [glass_p4_sgp_exact_x0]
        and     dx, 3
        mov     ax, [glass_p4_sgp_span_first_word]
        cmp     dx, 0
        je      .left_aligned
        inc     ax
.left_aligned:
        mov     [glass_p4_sgp_span_full_first], ax
        mov     dx, [glass_p4_sgp_exact_x1]
        and     dx, 3
        mov     bx, [glass_p4_sgp_span_last_word]
        cmp     dx, 3
        je      .right_aligned
        dec     bx
.right_aligned:
        mov     [glass_p4_sgp_span_full_last], bx
        cmp     ax, bx
        ja      .no_full_words
        sub     bx, ax
        inc     bx
        mov     [glass_p4_sgp_span_words], bx
        jmp     .record
.no_full_words:
        mov     word [glass_p4_sgp_span_words], 0
.record:
        call    glass_p4_sgp_record_span
        cmp     word [glass_p4_sgp_span_words], 0
        je      .done
        mov     ax, cx
        mov     bx, G0_PITCH_BYTES
        mul     bx
        mov     bx, [glass_p4_sgp_span_full_first]
        shl     bx, 1
        add     ax, bx
        adc     dx, 0
        add     ax, [glass_p4_sgp_target_base_low]
        adc     dx, [glass_p4_sgp_target_base_high]
        mov     cx, [glass_p4_sgp_span_words]
        call    glass_p4_sgp_emit_cls
.done:
        ret

; The shared convex scanner calls this backend callback with exact logical
; spans.  CPU and SGP therefore use identical edge and row coverage rules.
%define GLASS_P4_CONVEX_EMIT_SPAN glass_p4_sgp_emit_span
%include "glass_convex.inc"
%undef GLASS_P4_CONVEX_EMIT_SPAN

glass_p4_sgp_record_span:
        mov     di, [glass_p4_sgp_endpoint_cursor]
        cmp     di, glass_p4_sgp_endpoint_buffer_end - 16
        ja      .overflow
        mov     ax, [glass_p4_sgp_exact_x0]
        stosw
        mov     ax, [glass_p4_sgp_exact_x1]
        stosw
        mov     ax, [glass_p4_sgp_exact_y]
        stosw
        xor     ax, ax
        mov     al, [glass_p4_sgp_draw_colour]
        mov     ah, [glass_p4_sgp_face_id]
        stosw
        mov     ax, [glass_p4_sgp_span_first_word]
        stosw
        mov     ax, [glass_p4_sgp_span_last_word]
        stosw
        mov     ax, [glass_p4_sgp_span_full_first]
        stosw
        mov     ax, [glass_p4_sgp_span_words]
        stosw
        mov     [glass_p4_sgp_endpoint_cursor], di
        ret
.overflow:
        mov     byte [glass_p4_sgp_list_overflow], 1
        ret

; Apply only partial endpoint words as masked word RMW.  Complete interior
; words belong exclusively to the preceding SGP CLS range and are skipped
; here.  For a same-word span, full_count is zero and exactly one RMW is
; performed.  Pixels outside [x0,x1] are preserved in the endpoint word.
glass_p4_sgp_apply_endpoint_spans:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    bp
        push    es
        mov     ax, [glass_p4_sgp_target_cpu_segment]
        mov     es, ax
        mov     si, glass_p4_sgp_endpoint_buffer
.next:
        cmp     si, [glass_p4_sgp_endpoint_cursor]
        jae     .done
        mov     ax, [si]
        mov     [glass_p4_sgp_apply_x0], ax
        mov     ax, [si+2]
        mov     [glass_p4_sgp_apply_x1], ax
        mov     ax, [si+4]
        mov     [glass_p4_sgp_apply_y], ax
        mov     ax, [si+6]
        mov     [glass_p4_sgp_apply_colour], ax
        mov     ax, [si+12]
        mov     [glass_p4_sgp_apply_full_first], ax
        mov     ax, [si+14]
        mov     [glass_p4_sgp_apply_full_count], ax
        mov     ax, [glass_p4_sgp_apply_colour]
        and     ax, 0x000f
        mov     bx, ax
        shl     ax, 4
        or      ax, bx
        mov     bx, ax
        shl     bx, 8
        or      ax, bx
        mov     [glass_p4_sgp_apply_colour_word], ax
        mov     ax, [glass_p4_sgp_apply_x0]
        shr     ax, 2
        mov     [glass_p4_sgp_apply_word], ax
.word:
        mov     ax, [glass_p4_sgp_apply_word]
        cmp     word [glass_p4_sgp_apply_full_count], 0
        je      .write_word
        cmp     ax, [glass_p4_sgp_apply_full_first]
        jb      .write_word
        mov     bx, [glass_p4_sgp_apply_full_first]
        add     bx, [glass_p4_sgp_apply_full_count]
        cmp     ax, bx
        jb      .skip_word
.write_word:
        shl     ax, 2
        mov     [glass_p4_sgp_apply_word_x], ax
        mov     ax, [glass_p4_sgp_apply_word_x]
        cmp     ax, [glass_p4_sgp_apply_x0]
        jae     .low_ready
        mov     ax, [glass_p4_sgp_apply_x0]
.low_ready:
        mov     [glass_p4_sgp_apply_low], ax
        mov     ax, [glass_p4_sgp_apply_word_x]
        add     ax, 3
        cmp     ax, [glass_p4_sgp_apply_x1]
        jbe     .high_ready
        mov     ax, [glass_p4_sgp_apply_x1]
.high_ready:
        mov     [glass_p4_sgp_apply_high], ax
        xor     dx, dx
        xor     bx, bx
        mov     bp, [glass_p4_sgp_apply_low]
.pixel:
        mov     ax, bp
        and     ax, 3
        shl     ax, 1
        mov     di, ax
        mov     cx, [glass_p4_sgp_pixel_masks + di]
        or      dx, cx
        mov     ax, bp
        and     ax, 3
        shl     ax, 1
        mov     cx, [glass_p4_sgp_apply_colour_word]
        mov     di, ax
        and     cx, [glass_p4_sgp_pixel_masks + di]
        or      bx, cx
        inc     bp
        cmp     bp, [glass_p4_sgp_apply_high]
        jbe     .pixel
        mov     [glass_p4_sgp_apply_mask], dx
        mov     [glass_p4_sgp_apply_value], bx
        mov     ax, [glass_p4_sgp_apply_y]
        mov     cx, G0_PITCH_BYTES
        mul     cx
        mov     cx, [glass_p4_sgp_apply_word]
        shl     cx, 1
        add     ax, cx
        adc     dx, 0
        mov     di, ax
        mov     ax, [es:di]
        mov     cx, [glass_p4_sgp_apply_mask]
        not     cx
        and     ax, cx
        or      ax, [glass_p4_sgp_apply_value]
        mov     [es:di], ax
.skip_word:
        inc     word [glass_p4_sgp_apply_word]
        mov     ax, [glass_p4_sgp_apply_word]
        mov     cx, [glass_p4_sgp_apply_x1]
        shr     cx, 2
        cmp     ax, cx
        jbe     .word
        add     si, 16
        jmp     .next
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

; Export the exact span ownership audit into a gated diagnostic area of the
; CPU GVRAM aperture.  The default production payload does not call this
; routine.  The audit area is captured only by the audit run and is masked by
; the host visual checker; it is not part of the production rendering path.
glass_p4_sgp_export_span_audit:
        push    ax
        push    cx
        push    dx
        push    si
        push    di
        push    es
        mov     ax, [glass_p4_sgp_endpoint_cursor]
        sub     ax, glass_p4_sgp_endpoint_buffer
        mov     cx, 4
        shr     ax, cl
        mov     dx, ax
        mov     ax, 0xa000
        mov     es, ax
        mov     di, G0_AUDIT_GVRAM_OFFSET
        mov     ax, 0x5034             ; "P4" audit magic.
        stosw
        mov     ax, 1
        stosw
        mov     ax, 16                 ; bytes per span record.
        stosw
        mov     ax, dx
        stosw
        xor     ax, ax
        stosw
        stosw
        mov     si, glass_p4_sgp_endpoint_buffer
        mov     cx, dx
        shl     cx, 3                  ; eight words per 16-byte record.
        rep     movsw
        pop     es
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     ax
        ret

; Write x=0..15 as palette indices 1..15 at physical row 20.  The four
; literal masks are a diagnostic fixture, not a second production mask table.
; The page was cleared by the preceding SGP list, so raw-word deltas are
; directly observable in the capture.
glass_p4_sgp_calibrate_word_layout:
        push    ax
        push    bx
        push    cx
        push    dx
        push    di
        push    si
        push    es
        mov     ax, 0xa000
        mov     es, ax
        xor     bx, bx
.pixel:
        mov     ax, bx
        shr     ax, 2
        shl     ax, 1
        mov     di, ax
        add     di, 20 * G0_PITCH_BYTES
        mov     ax, bx
        and     ax, 3
        cmp     ax, 0
        jne     .x1
        mov     dx, 0x00f0
        jmp     .mask
.x1:
        cmp     ax, 1
        jne     .x2
        mov     dx, 0x000f
        jmp     .mask
.x2:
        cmp     ax, 2
        jne     .x3
        mov     dx, 0xf000
        jmp     .mask
.x3:
        mov     dx, 0x0f00
.mask:
        mov     ax, bx
        inc     ax
        cmp     ax, 16
        jne     .colour_ready
        mov     ax, 1
.colour_ready:
        mov     cx, ax
        shl     ax, 4
        or      ax, cx
        mov     cx, ax
        shl     cx, 8
        or      ax, cx
        and     ax, dx
        mov     cx, dx
        not     cx
        and     cx, [es:di]
        or      ax, cx
        mov     [es:di], ax
        inc     bx
        cmp     bx, 16
        jb      .pixel
        pop     es
        pop     si
        pop     di
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; Emit one documented SGP LINE.  The P4 fixed cube is wholly inside the
; physical 640x200 viewport; no implicit clipping is relied upon here.
glass_p4_sgp_emit_line:
        push    ax
        push    bx
        push    cx
        push    dx
        mov     ax, SGP_COMMAND_LINE
        call    glass_p4_sgp_emit_word
        mov     bx, SGP_LINE_COPY

        mov     ax, [glass_p4_sgp_line_x1]
        sub     ax, [glass_p4_sgp_line_x0]
        jns     .x_positive
        neg     ax
        or      bx, SGP_LINE_HD
.x_positive:
        inc     ax
        mov     [glass_p4_sgp_line_width], ax

        mov     ax, [glass_p4_sgp_line_y1]
        sar     ax, 1
        mov     [glass_p4_sgp_line_y1_physical], ax
        mov     cx, [glass_p4_sgp_line_y0]
        sar     cx, 1
        mov     [glass_p4_sgp_line_y0_physical], cx
        sub     ax, cx
        jns     .y_positive
        neg     ax
        or      bx, SGP_LINE_VD
.y_positive:
        inc     ax
        mov     [glass_p4_sgp_line_height], ax

        mov     ax, bx
        call    glass_p4_sgp_emit_word
        mov     ax, [glass_p4_sgp_line_x0]
        and     ax, 3
        shl     ax, 4
        or      ax, 1                 ; mode 1 = packed 4bpp.
        call    glass_p4_sgp_emit_word
        mov     ax, [glass_p4_sgp_line_width]
        call    glass_p4_sgp_emit_word
        mov     ax, [glass_p4_sgp_line_height]
        call    glass_p4_sgp_emit_word
        mov     ax, G0_PITCH_BYTES
        call    glass_p4_sgp_emit_word

        mov     ax, [glass_p4_sgp_line_y0_physical]
        mov     bx, G0_PITCH_BYTES
        mul     bx
        mov     bx, [glass_p4_sgp_line_x0]
        and     bx, 0xfffc
        shr     bx, 1
        add     ax, bx
        adc     dx, 0
        add     ax, [glass_p4_sgp_target_base_low]
        adc     dx, [glass_p4_sgp_target_base_high]
        call    glass_p4_sgp_emit_word
        mov     ax, dx
        call    glass_p4_sgp_emit_word
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; AX is one list word.  The main-RAM command cursor is deliberately separate
; from DI: triangle scan conversion uses DI for temporary vertex pointers.
glass_p4_sgp_emit_word:
        push    di
        mov     di, [glass_p4_sgp_list_cursor]
        cmp     di, glass_p4_sgp_command_list_end - 2
        ja      .overflow
        stosw
        mov     [glass_p4_sgp_list_cursor], di
        pop     di
        ret
.overflow:
        mov     byte [glass_p4_sgp_list_overflow], 1
        pop     di
        ret

glass_p4_sgp_run_list:
        call    glass_p4_sgp_wait_idle
        jc      .failed
        mov     dx, PORT_SGP_COMMAND
        mov     ax, [glass_p4_sgp_command_address_low]
        out     dx, ax
        add     dx, 2
        mov     ax, [glass_p4_sgp_command_address_high]
        out     dx, ax
        mov     dx, PORT_SGP_CONTROL
        xor     al, al
        out     dx, al
        mov     dx, PORT_SGP_STATUS
        mov     al, SGP_BUSY
        out     dx, al
        call    glass_p4_sgp_wait_idle
        ret
.failed:
        stc
        ret

glass_p4_sgp_wait_idle:
        mov     dx, PORT_SGP_STATUS
        ; A complete fixed frame contains a full-page clear plus many span
        ; commands.  Permit that finite list to drain before declaring an
        ; SGP timeout.  The outer count is a guest-side safety bound, not a
        ; timing model or a real-hardware performance assertion.
        mov     bx, 0x0100
.outer:
        mov     cx, 0xffff
.poll:
        in      al, dx
        test    al, SGP_BUSY
        jz      .ready
        loop    .poll
        dec     bx
        jnz     .outer
        stc
        ret
.ready:
        clc
        ret

; Convert DS:SI to the physical command-list address required by SGP.
glass_p4_sgp_physical_address_from_ds_si:
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

glass_p4_sgp_raw_checksum:
        push    ax
        push    cx
        push    di
        xor     bx, bx
        xor     di, di
        mov     cx, G0_PAGE_WORDS
.word:
        rol     bx, 1
        xor     bx, [es:di]
        add     di, 2
        loop    .word
        pop     di
        pop     cx
        pop     ax
        ret

%include "glass_geometry.inc"
%include "glass_data.inc"
%include "glass_scene.inc"

align 2, db 0
glass_p4_sgp_framebuffer_descriptor:
        dw 4, 640, 400
glass_p4_sgp_window_descriptor:
        dw 0, 0, 200, 0, 0

glass_p4_sgp_palette:
        dw 0x0000, 0x001f, 0x03e0, 0x03ff
        dw 0xfc00, 0xfc1f, 0xffe0, 0xffff
        dw 0x0015, 0x02a0, 0xac00, 0xac15
        dw 0x02b5, 0xaea0, 0x7def, 0x7fff

glass_p4_sgp_face_colour_map:
        db 0,8,9,12,10,11,13

glass_p4_sgp_draw_colour db 0
glass_p4_sgp_list_overflow db 0
align 2, db 0
glass_p4_sgp_last_colour       dw 0
glass_p4_sgp_tri_v0            dw 0,0
glass_p4_sgp_tri_v1            dw 0,0
glass_p4_sgp_tri_v2            dw 0,0
glass_p4_sgp_poly_v0           dw 0,0
glass_p4_sgp_poly_v1           dw 0,0
glass_p4_sgp_poly_v2           dw 0,0
glass_p4_sgp_poly_v3           dw 0,0
glass_p4_sgp_scan_y            dw 0
glass_p4_sgp_span_x0           dw 0
glass_p4_sgp_span_x1           dw 0
glass_p4_sgp_edge_a_ceil       dw 0
glass_p4_sgp_edge_a_floor      dw 0
glass_p4_sgp_edge_b_ceil       dw 0
glass_p4_sgp_edge_b_floor      dw 0
glass_p4_sgp_span_first_word   dw 0
glass_p4_sgp_span_last_word    dw 0
glass_p4_sgp_span_full_first   dw 0
glass_p4_sgp_span_full_last    dw 0
glass_p4_sgp_span_words        dw 0
glass_p4_sgp_exact_x0          dw 0
glass_p4_sgp_exact_x1          dw 0
glass_p4_sgp_exact_y           dw 0
glass_p4_sgp_face_id            db 0
align 2, db 0
glass_p4_sgp_line_x0           dw 0
glass_p4_sgp_line_y0           dw 0
glass_p4_sgp_line_x1           dw 0
glass_p4_sgp_line_y1           dw 0
glass_p4_sgp_line_y0_physical  dw 0
glass_p4_sgp_line_y1_physical  dw 0
glass_p4_sgp_line_width        dw 0
glass_p4_sgp_line_height       dw 0
glass_p4_sgp_command_address_low  dw 0
glass_p4_sgp_command_address_high dw 0
glass_p4_sgp_list_bytes           dw 0
glass_p4_sgp_list_cursor          dw 0
glass_p4_sgp_endpoint_cursor      dw 0
glass_p4_sgp_apply_x0             dw 0
glass_p4_sgp_apply_x1             dw 0
glass_p4_sgp_apply_y              dw 0
glass_p4_sgp_apply_colour         dw 0
glass_p4_sgp_apply_colour_word    dw 0
glass_p4_sgp_apply_word           dw 0
glass_p4_sgp_apply_word_x         dw 0
glass_p4_sgp_apply_low            dw 0
glass_p4_sgp_apply_high           dw 0
glass_p4_sgp_apply_mask           dw 0
glass_p4_sgp_apply_value          dw 0
glass_p4_sgp_apply_full_first      dw 0
glass_p4_sgp_apply_full_count      dw 0
glass_p4_sgp_target_base_low       dw 0
glass_p4_sgp_target_base_high      dw 0
glass_p4_sgp_target_cpu_segment    dw 0
; Packed 4bpp CPU-word order is byte-swapped relative to the logical pixel
; order: x%4=0,1,2,3 map to 00f0h, 000fh, f000h, 0f00h respectively.
glass_p4_sgp_pixel_masks           dw 0x00f0, 0x000f, 0xf000, 0x0f00

align 2, db 0
glass_p4_sgp_work_area:
        times 58 db 0
glass_p4_sgp_command_list:
        ; P2's conservative all-face budget is 32 KiB.  The P4 fixed frame
        ; uses less, but keeps the production candidate's fail-closed bound.
        times 32768 db 0
glass_p4_sgp_command_list_end:

align 2, db 0
glass_p4_sgp_endpoint_buffer:
        times 8192 db 0
glass_p4_sgp_endpoint_buffer_end:

%if ($ - $$) > LOADER_RETURN_SS
%error "GLASS ORBIT P4-2 payload overlaps the loader return reserve"
%endif
