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
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
; WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
; MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
; EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
; SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
; WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
; OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
; ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

; Generic convex-polygon scan conversion shared by the P4 CPU and SGP
; payloads. The caller supplies DS:SI -> count packed x/y vertices, with
; physical 640x200 Y coordinates already prepared. The callback receives one
; exact inclusive span in AX=x0, BX=x1, CX=y. The callback is the only
; backend-specific operation; this file contains all polygon geometry.
%ifndef GLASS_P4_CONVEX_EMIT_SPAN
%error "GLASS_P4_CONVEX_EMIT_SPAN must name the backend span writer"
%endif

glass_p4_convex_fill_polygon:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    bp
        mov     [glass_p4_convex_base], si
        mov     [glass_p4_convex_count], cx
        cmp     cx, 3
        jb      .done

        ; Find the vertical extent without sorting or changing the caller's
        ; vertex order. Horizontal edges are handled by the edge activation
        ; rule below and are never counted as intersections.
        mov     di, si
        mov     ax, [di+2]
        mov     [glass_p4_convex_min_y], ax
        mov     [glass_p4_convex_max_y], ax
        mov     bx, 1
.find_extent:
        cmp     bx, [glass_p4_convex_count]
        jae     .clip_extent
        mov     di, bx
        shl     di, 2
        add     di, [glass_p4_convex_base]
        mov     ax, [di+2]
        cmp     ax, [glass_p4_convex_min_y]
        jge     .not_lower
        mov     [glass_p4_convex_min_y], ax
.not_lower:
        cmp     ax, [glass_p4_convex_max_y]
        jle     .not_upper
        mov     [glass_p4_convex_max_y], ax
.not_upper:
        inc     bx
        jmp     .find_extent

.clip_extent:
        mov     ax, [glass_p4_convex_min_y]
        cmp     ax, G0_HEIGHT
        jge     .done
        mov     ax, [glass_p4_convex_max_y]
        cmp     ax, 0
        jle     .done
        mov     ax, [glass_p4_convex_min_y]
        cmp     ax, 0
        jge     .scan_start_ready
        xor     ax, ax
.scan_start_ready:
        mov     [glass_p4_convex_scan_y], ax
        mov     ax, [glass_p4_convex_max_y]
        cmp     ax, G0_HEIGHT
        jle     .scan_end_ready
        mov     ax, G0_HEIGHT
.scan_end_ready:
        mov     [glass_p4_convex_scan_end], ax

.scan_row:
        mov     ax, [glass_p4_convex_scan_y]
        cmp     ax, [glass_p4_convex_scan_end]
        jge     .done
        mov     byte [glass_p4_convex_edge_count], 0
        mov     word [glass_p4_convex_edge_index], 0
        mov     si, [glass_p4_convex_base]

.edge:
        mov     di, si
        add     di, 4
        mov     ax, [glass_p4_convex_edge_index]
        inc     ax
        cmp     ax, [glass_p4_convex_count]
        jb      .second_ready
        mov     di, [glass_p4_convex_base]
.second_ready:
        mov     [glass_p4_convex_next_ptr], di

        ; Activate an edge for min_y <= sample_y < max_y. This half-open
        ; rule counts every vertex exactly once and ignores horizontal edges.
        mov     ax, [si+2]
        cmp     ax, [di+2]
        je      .next_edge
        jl      .first_is_low
        mov     dx, [glass_p4_convex_scan_y]
        cmp     dx, [di+2]
        jl      .next_edge
        cmp     dx, ax
        jge     .next_edge
        xchg    si, di
        jmp     .ordered_edge
.first_is_low:
        mov     dx, [glass_p4_convex_scan_y]
        cmp     dx, ax
        jl      .next_edge
        cmp     dx, [di+2]
        jge     .next_edge
.ordered_edge:
        mov     bx, [glass_p4_convex_scan_y]
        mov     bp, 1                 ; ceil(x at y+1/2)
        call    glass_p4_convex_interpolate_edge
        mov     [glass_p4_convex_current_ceil], ax
        xor     bp, bp                ; floor(x at y+1/2)
        call    glass_p4_convex_interpolate_edge
        mov     [glass_p4_convex_current_floor], ax

        cmp     byte [glass_p4_convex_edge_count], 0
        jne     .update_extent
        mov     ax, [glass_p4_convex_current_ceil]
        mov     [glass_p4_convex_left], ax
        mov     ax, [glass_p4_convex_current_floor]
        mov     [glass_p4_convex_right], ax
        jmp     .edge_counted
.update_extent:
        mov     ax, [glass_p4_convex_current_ceil]
        cmp     ax, [glass_p4_convex_left]
        jge     .left_ready
        mov     [glass_p4_convex_left], ax
.left_ready:
        mov     ax, [glass_p4_convex_current_floor]
        cmp     ax, [glass_p4_convex_right]
        jle     .edge_counted
        mov     [glass_p4_convex_right], ax
.edge_counted:
        inc     byte [glass_p4_convex_edge_count]

.next_edge:
        mov     si, [glass_p4_convex_next_ptr]
        inc     word [glass_p4_convex_edge_index]
        mov     ax, [glass_p4_convex_edge_index]
        cmp     ax, [glass_p4_convex_count]
        jb      .edge

        cmp     byte [glass_p4_convex_edge_count], 2
        jb      .advance_row
        mov     ax, [glass_p4_convex_left]
        mov     bx, [glass_p4_convex_right]
        mov     cx, [glass_p4_convex_scan_y]
        call    GLASS_P4_CONVEX_EMIT_SPAN
.advance_row:
        inc     word [glass_p4_convex_scan_y]
        jmp     .scan_row

.done:
        pop     bp
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; SI and DI are ordered low-Y/high-Y x/y pairs. BX is physical Y. BP=1
; returns ceil(x) and BP=0 returns floor(x) for the y+1/2 sample.
glass_p4_convex_interpolate_edge:
        push    bx
        push    cx
        push    dx
        mov     cx, [di+2]
        sub     cx, [si+2]
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
        pop     dx
        pop     cx
        pop     bx
        ret

align 2, db 0
glass_p4_convex_base          dw 0
glass_p4_convex_count         dw 0
glass_p4_convex_min_y         dw 0
glass_p4_convex_max_y         dw 0
glass_p4_convex_scan_y        dw 0
glass_p4_convex_scan_end      dw 0
glass_p4_convex_edge_index    dw 0
glass_p4_convex_next_ptr      dw 0
glass_p4_convex_left          dw 0
glass_p4_convex_right         dw 0
glass_p4_convex_current_ceil  dw 0
glass_p4_convex_current_floor dw 0
glass_p4_convex_edge_count    db 0
