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
; BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
; ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
; THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

; P5 scene connection.  Faces, edges, and grid lines are emitted through the
; existing SGP list builder.  Stars are the one deliberate CPU-pixel path in
; the approved design because SGP has no single-pixel operation.

%define PORT_TSP_STATUS_P5       0x0142
%define TSP_STATUS_VBLANK_P5     0x0040
%define PAGE_A_SOURCE_Y_P5       0
%define PAGE_B_SOURCE_Y_P5       200
%define PAGE_A_BASE_P5           0x200000
%define PAGE_B_BASE_P5           0x20fa00
%define PAGE_A_CPU_SEGMENT_P5    0xa000
%define PAGE_B_CPU_SEGMENT_P5    0xafa0

glass_p5_run:
        mov     word [render_frame_counter], 0
        mov     word [glass_star_scroll_x], 0
        mov     byte [glass_star_subtick], 0
        mov     byte [glass_p5_display_page], 0
        mov     byte [glass_p5_target_page], 1
        mov     byte [glass_p5_frame_ready], 0
        mov     byte [glass_p5_geometry_complete], 0
        mov     byte [glass_p5_stars_complete], 0
        mov     byte [glass_p5_sgp_idle], 0
        mov     word [glass_p5_presented_frame], 0
        mov     ax, 0x0b00             ; $ScnDsp: graphics off.
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
        call    glass_p5_select_display_page
        jc      .failed
%if GLASS_P5
        ; Keep graphics usable when the YM2608 high bank is absent.  The
        ; probe is read-only and bounded; a detected OPNA starts the song.
        call    glass_opna_init
%endif
.frame:
        ; A target half is not presentable while it is being rebuilt.
        mov     byte [glass_p5_frame_ready], 0
        mov     byte [glass_p5_geometry_complete], 0
        mov     byte [glass_p5_stars_complete], 0
        mov     byte [glass_p5_sgp_idle], 0
        call    glass_p5_select_target_storage
        call    glass_compute_cube
        call    glass_p4_sgp_build_list
        jc      .failed
        call    glass_p4_sgp_run_list
        jc      .failed
        mov     byte [glass_p5_sgp_idle], 1
        call    glass_p4_sgp_apply_endpoint_spans
        mov     byte [glass_p5_geometry_complete], 1
        call    glass_p4_sgp_build_edge_list
        jc      .failed
        call    glass_p4_sgp_run_list
        jc      .failed
        mov     byte [glass_p5_sgp_idle], 1
        call    glass_p5_draw_stars
        mov     byte [glass_p5_stars_complete], 1
        call    glass_advance_scene_tick
%if GLASS_P5
        call    glass_opna_tick
%endif
        inc     word [render_frame_counter]
        ; FRAME_READY is set only after both SGP lists have completed and the
        ; CPU star pass has finished.  The display selector below is guarded
        ; by this state; an incomplete target half can never be presented.
        cmp     byte [glass_p5_geometry_complete], 1
        jne     .failed
        cmp     byte [glass_p5_stars_complete], 1
        jne     .failed
        cmp     byte [glass_p5_sgp_idle], 1
        jne     .failed
        mov     byte [glass_p5_frame_ready], 1
        call    glass_p5_wait_vblank
        jc      .failed
        cmp     byte [glass_p5_frame_ready], 1
        jne     .failed
        mov     al, [glass_p5_target_page]
        mov     [glass_p5_display_page], al
        call    glass_p5_select_display_page
        jc      .failed
        inc     word [glass_p5_presented_frame]
        mov     byte [glass_p5_frame_ready], 0
        xor     byte [glass_p5_target_page], 1
        cmp     word [render_frame_counter], 1
        jne     .display_ready
        mov     ax, 0x0b01             ; $ScnDsp: first complete frame on.
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
.display_ready:
        call    glass_p4_sgp_escape_pressed
        jc      .exit
        jmp     .frame
.exit:
%if GLASS_P5
        call    glass_opna_shutdown
%endif
        jmp     glass_p4_sgp_exit
.failed:
%if GLASS_P5
        call    glass_opna_shutdown
%endif
        push    cs
        pop     es
        xor     bx, bx
        mov     ax, 0x47e5             ; debugger-visible P5 failure marker.
        ret

; Select the source-window half with the VA Graphics BIOS $RollTo service.
; This is the coupled FB0 OFY/DSA operation proved by GA-6; no raw DSA write
; is used for presentation.
glass_p5_select_display_page:
        push    ax
        push    bx
        push    cx
        push    dx
        xor     bx, bx
        xor     cx, cx
        xor     dx, dx
        cmp     byte [glass_p5_display_page], 0
        je      .source_ready
        mov     dx, PAGE_B_SOURCE_Y_P5
.source_ready:
        mov     ax, 0x0500             ; $RollTo, G0, FB0.
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
        clc
        jmp     .done
.failed:
        stc
.done:
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; Point the shared exact-span backend at the hidden FB0 source half.  CPU
; endpoint RMW and stars use the corresponding A0000h aperture segment.
glass_p5_select_target_storage:
        cmp     byte [glass_p5_target_page], 0
        je      .page_a
        mov     word [glass_p4_sgp_target_base_low], PAGE_B_BASE_P5 & 0xffff
        mov     word [glass_p4_sgp_target_base_high], PAGE_B_BASE_P5 >> 16
        mov     word [glass_p4_sgp_target_cpu_segment], PAGE_B_CPU_SEGMENT_P5
        ret
.page_a:
        mov     word [glass_p4_sgp_target_base_low], PAGE_A_BASE_P5 & 0xffff
        mov     word [glass_p4_sgp_target_base_high], PAGE_A_BASE_P5 >> 16
        mov     word [glass_p4_sgp_target_cpu_segment], PAGE_A_CPU_SEGMENT_P5
        ret

; Emit the original perspective grid as 15 SGP LINE commands.  Coordinates
; stay in the source logical 640x400 space; the common LINE emitter performs
; the single physical-Y conversion.
glass_p5_draw_grid:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    bp

        mov     al, GLASS_GRID_HORIZ
        call    glass_p5_set_grid_colour
        xor     ax, ax
        mov     bx, 250
        mov     cx, 639
        mov     dx, 250
        call    glass_p5_emit_line

        mov     si, glass_grid_endpoints
        mov     bp, 9
.rays:
        mov     ax, 320
        mov     bx, 399
        mov     cx, [si]
        mov     dx, 250
        call    glass_p5_emit_line
        add     si, 2
        dec     bp
        jnz     .rays

        mov     al, GLASS_GRID_DEPTH
        call    glass_p5_set_grid_colour
        mov     si, glass_grid_y
        mov     bp, 5
.horizontal:
        xor     ax, ax
        mov     bx, [si]
        mov     cx, 639
        mov     dx, bx
        call    glass_p5_emit_line
        add     si, 2
        dec     bp
        jnz     .horizontal

        pop     bp
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

glass_p5_set_grid_colour:
        xor     ah, ah
        mov     [glass_p4_sgp_draw_colour], al
        jmp     glass_p4_sgp_emit_set_colour_index

; AX/BX/CX/DX are logical x0/y0/x1/y1.  Adapt the scene-facing order to the
; existing SGP LINE emitter without introducing another rasterizer.
glass_p5_emit_line:
        mov     [glass_p4_sgp_line_x0], ax
        mov     [glass_p4_sgp_line_y0], bx
        mov     [glass_p4_sgp_line_x1], cx
        mov     [glass_p4_sgp_line_y1], dx
        jmp     glass_p4_sgp_emit_line

; Plot one packed 4bpp pixel through the documented CPU GVRAM aperture.
; Input: AX=physical x, BX=physical y, DL=palette index.
glass_p5_plot_pixel:
        cmp     ax, 0
        jl      .done
        cmp     ax, G0_WIDTH - 1
        ja      .done
        cmp     bx, 0
        jl      .done
        cmp     bx, G0_HEIGHT - 1
        ja      .done
        mov     [glass_p5_pixel_x], ax
        mov     [glass_p5_pixel_colour], dl
        push    ax
        push    bx
        push    cx
        push    dx
        push    di
        push    es
        mov     ax, [glass_p4_sgp_target_cpu_segment]
        mov     es, ax
        mov     ax, bx
        mov     cx, G0_PITCH_BYTES
        mul     cx
        mov     di, ax
        mov     ax, [glass_p5_pixel_x]
        mov     bx, ax
        shr     ax, 2
        shl     ax, 1
        add     di, ax
        and     bx, 3
        shl     bx, 1
        mov     cx, [glass_p4_sgp_pixel_masks + bx]
        mov     bx, cx
        mov     ax, [es:di]
        not     cx
        and     ax, cx
        mov     al, [glass_p5_pixel_colour]
        xor     ah, ah
        mov     cx, ax
        shl     ax, 4
        or      ax, cx
        mov     cx, ax
        shl     cx, 8
        or      ax, cx
        and     ax, bx
        or      ax, [es:di]
        mov     [es:di], ax
        pop     es
        pop     di
        pop     dx
        pop     cx
        pop     bx
        pop     ax
.done:
        ret

align 2, db 0
glass_p5_pixel_x dw 0
glass_p5_pixel_colour db 0
glass_p5_display_page db 0
glass_p5_target_page db 1
glass_p5_frame_ready db 0
glass_p5_geometry_complete db 0
glass_p5_stars_complete db 0
glass_p5_sgp_idle db 0
align 2, db 0
glass_p5_presented_frame dw 0

; Draw the retained 256-star table.  The source scene is logical 640x400;
; stars are restricted to the upper sky and therefore use physical y=y/2.
glass_p5_draw_stars:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    bp
        mov     si, glass_star_table
        mov     bp, GLASS_STAR_COUNT
.star:
        mov     ax, [si]
        sub     ax, [glass_star_scroll_x]
        jns     .x_ready
        add     ax, 640
.x_ready:
        mov     [glass_star_plot_x], ax
        mov     bx, [si+2]
        cmp     bx, 240
        jae     .next
        sar     bx, 1
        mov     dl, [si+4]
        cmp     dl, 0
        je      .dim
        cmp     dl, 1
        je      .medium
        mov     dl, GLASS_STAR_BRIGHT
        call    glass_p5_plot_pixel
        mov     ax, [glass_star_plot_x]
        dec     ax
        call    glass_p5_plot_pixel
        mov     ax, [glass_star_plot_x]
        inc     ax
        call    glass_p5_plot_pixel
        jmp     .next
.dim:
        mov     dl, GLASS_STAR_DIM
        call    glass_p5_plot_pixel
        jmp     .next
.medium:
        mov     dl, GLASS_STAR_MED
        call    glass_p5_plot_pixel
.next:
        add     si, 6
        dec     bp
        jnz     .star
        pop     bp
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; Wait for one observed display-to-blank transition.  The bound is only a
; fail-closed emulator/test guard and makes no timing claim about hardware.
glass_p5_wait_vblank:
        push    ax
        push    bx
        push    cx
        push    dx
        mov     dx, PORT_TSP_STATUS_P5
        mov     bx, 4
.wait_low:
        mov     cx, 0xffff
.low:
        in      al, dx
        test    al, TSP_STATUS_VBLANK_P5
        jz      .low_seen
        loop    .low
        dec     bx
        jnz     .wait_low
        stc
        jmp     .done
.low_seen:
        mov     bx, 4
.wait_high:
        mov     cx, 0xffff
.high:
        in      al, dx
        test    al, TSP_STATUS_VBLANK_P5
        jnz     .ready
        loop    .high
        dec     bx
        jnz     .wait_high
        stc
        jmp     .done
.ready:
        clc
.done:
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
