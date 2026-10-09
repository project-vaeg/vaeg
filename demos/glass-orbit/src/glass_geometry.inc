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
; 2. Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in the
;    documentation and/or other materials provided with the distribution.
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

; This file carries only GLASS ORBIT geometry and fixed-point arithmetic.
; Drawing functions and all graphics primitives are deliberately absent.

; Original GLASS286 digital-colour indices.  They are data only in P0; no
; renderer or palette programming is imported by this extraction.
%define GLASS_STAR_DIM              1
%define GLASS_STAR_MED              5
%define GLASS_STAR_BRIGHT           7
%define GLASS_STAR_MED_DARK         1
%define GLASS_STAR_BRIGHT_DARK      5
%define GLASS_GRID_HORIZ            5
%define GLASS_GRID_DEPTH            1
%define GLASS_EDGE_A                2
%define GLASS_EDGE_B                4
%define GLASS_EDGE_C                7
%define GLASS_FACE_0                1
%define GLASS_FACE_1                2
%define GLASS_FACE_2                4
%define GLASS_FACE_3                5
%define GLASS_FACE_4                3
%define GLASS_FACE_5                6

GLASS_STAR_COUNT equ 256
GLASS_STAR_TICKS_PER_PIXEL equ 8

; DX:AX signed product -> AX = product >> 14, preserving only the low word.
; This is the original GLASS scene helper used by glass_compute_cube.
glass_q14_from_dxax:
        push    cx
        mov     cx, ax
        shr     cx, 14
        mov     ax, dx
        shl     ax, 2
        or      ax, cx
        pop     cx
        ret

; P0 reachability wrapper.  It deliberately computes geometry only; it does
; not render, select a video mode, access an I/O port, or prepare an SGP list.
glass_geometry_step:
        pusha
        call    glass_advance_scene_tick
        call    glass_compute_cube
        mov     si, glass_cube_faces
        call    glass_face_is_visible
        popa
        ret

glass_advance_scene_tick:
        inc     byte [glass_star_subtick]
        cmp     byte [glass_star_subtick], GLASS_STAR_TICKS_PER_PIXEL
        jb      .done
        mov     byte [glass_star_subtick], 0
        inc     word [glass_star_scroll_x]
        cmp     word [glass_star_scroll_x], 640
        jb      .done
        mov     word [glass_star_scroll_x], 0
.done:
        ret

; Integer cube rotation/projection retained without algorithmic changes from
; GLASS_SCENE.INC.  Coordinates remain in the original logical 640x400 space.
glass_compute_cube:
        pusha
        mov     ax, [render_frame_counter]
        mov     bx, ax
        shr     bx, 3
        sub     ax, bx
        add     al, 14
        call    get_sin
        mov     [glass_sin_x], ax
        mov     ax, [render_frame_counter]
        mov     bx, ax
        shr     bx, 3
        sub     ax, bx
        add     al, 14
        call    get_cos
        mov     [glass_cos_x], ax

        mov     ax, [render_frame_counter]
        mov     bx, ax
        shr     bx, 2
        add     ax, bx
        mov     bx, [render_frame_counter]
        shr     bx, 3
        add     ax, bx
        add     al, 24
        call    get_sin
        mov     [glass_sin_y], ax
        mov     ax, [render_frame_counter]
        mov     bx, ax
        shr     bx, 2
        add     ax, bx
        mov     bx, [render_frame_counter]
        shr     bx, 3
        add     ax, bx
        add     al, 24
        call    get_cos
        mov     [glass_cos_y], ax

        xor     si, si
        xor     di, di
        mov     bp, 8
.vertex_loop:
        ; x1 = (x*cosY + z*sinY) >> 14
        mov     ax, [glass_cube_vertices + si]
        imul    word [glass_cos_y]
        mov     [glass_accum], ax
        mov     [glass_accum+2], dx
        mov     ax, [glass_cube_vertices + si + 4]
        imul    word [glass_sin_y]
        add     [glass_accum], ax
        adc     [glass_accum+2], dx
        mov     ax, [glass_accum]
        mov     dx, [glass_accum+2]
        call    glass_q14_from_dxax
        mov     [glass_tmp_x], ax

        ; z1 = (-x*sinY + z*cosY) >> 14
        mov     ax, [glass_cube_vertices + si]
        neg     ax
        imul    word [glass_sin_y]
        mov     [glass_accum], ax
        mov     [glass_accum+2], dx
        mov     ax, [glass_cube_vertices + si + 4]
        imul    word [glass_cos_y]
        add     [glass_accum], ax
        adc     [glass_accum+2], dx
        mov     ax, [glass_accum]
        mov     dx, [glass_accum+2]
        call    glass_q14_from_dxax
        mov     [glass_tmp_z1], ax

        ; y2 = (y*cosX - z1*sinX) >> 14
        mov     ax, [glass_cube_vertices + si + 2]
        imul    word [glass_cos_x]
        mov     [glass_accum], ax
        mov     [glass_accum+2], dx
        mov     ax, [glass_tmp_z1]
        imul    word [glass_sin_x]
        sub     [glass_accum], ax
        sbb     [glass_accum+2], dx
        mov     ax, [glass_accum]
        mov     dx, [glass_accum+2]
        call    glass_q14_from_dxax
        mov     [glass_tmp_y], ax

        ; z2 = (y*sinX + z1*cosX) >> 14
        mov     ax, [glass_cube_vertices + si + 2]
        imul    word [glass_sin_x]
        mov     [glass_accum], ax
        mov     [glass_accum+2], dx
        mov     ax, [glass_tmp_z1]
        imul    word [glass_cos_x]
        add     [glass_accum], ax
        adc     [glass_accum+2], dx
        mov     ax, [glass_accum]
        mov     dx, [glass_accum+2]
        call    glass_q14_from_dxax
        mov     [glass_tmp_z2], ax

        mov     ax, [glass_tmp_z2]
        add     ax, 520
        mov     [glass_den], ax

        mov     ax, [glass_tmp_x]
        mov     bx, 340
        imul    bx
        idiv    word [glass_den]
        add     ax, 320
        mov     [glass_cube_projected + di], ax

        mov     ax, [glass_tmp_y]
        mov     bx, 340
        imul    bx
        idiv    word [glass_den]
        neg     ax
        add     ax, 150
        mov     [glass_cube_projected + di + 2], ax

        mov     ax, [glass_tmp_z2]
        mov     [glass_cube_projected + di + 4], ax
        add     si, 6
        add     di, 6
        dec     bp
        jnz     .vertex_loop
        popa
        ret

; SI -> face record.  CF=1 when the outward-wound projected face points
; toward the camera.  The routine is retained for later SGP face selection.
glass_face_is_visible:
        push    ax
        push    bx
        push    dx
        push    di

        xor     bx, bx
        mov     bl, [si]
        imul    bx, bx, 6
        mov     ax, [glass_cube_projected + bx]
        mov     [glass_face_x0], ax
        mov     ax, [glass_cube_projected + bx + 2]
        mov     [glass_face_y0], ax

        xor     bx, bx
        mov     bl, [si+1]
        imul    bx, bx, 6
        mov     ax, [glass_cube_projected + bx]
        sub     ax, [glass_face_x0]
        mov     [glass_face_dx10], ax
        mov     ax, [glass_cube_projected + bx + 2]
        sub     ax, [glass_face_y0]
        mov     [glass_face_dy10], ax

        xor     bx, bx
        mov     bl, [si+2]
        imul    bx, bx, 6
        mov     ax, [glass_cube_projected + bx]
        sub     ax, [glass_face_x0]
        mov     [glass_face_dx20], ax
        mov     ax, [glass_cube_projected + bx + 2]
        sub     ax, [glass_face_y0]
        mov     [glass_face_dy20], ax

        mov     ax, [glass_face_dx10]
        imul    word [glass_face_dy20]
        mov     [glass_face_cross_lo], ax
        mov     [glass_face_cross_hi], dx
        mov     ax, [glass_face_dy10]
        imul    word [glass_face_dx20]
        sub     [glass_face_cross_lo], ax
        sbb     [glass_face_cross_hi], dx

        test    word [glass_face_cross_hi], 8000h
        jz      .hidden
        stc
        jmp     .done
.hidden:
        clc
.done:
        pop     di
        pop     dx
        pop     bx
        pop     ax
        ret

; AL angle 0..255 -> AX signed sine, scale 16384. Preserves BX.
get_sin:
        push    bx
        xor     bx, bx
        mov     bl, al
        shl     bx, 1
        mov     ax, [sin_table + bx]
        pop     bx
        ret

; AL angle 0..255 -> AX signed cosine, scale 16384. Preserves BX.
get_cos:
        add     al, 64
        jmp     get_sin

; DX:AX is a signed 32-bit product. Return its low 16-bit value after
; arithmetic shift right 14.  Retained from VIDEO286.INC for later geometry.
math_q14_from_dxax:
        mov     cx, ax
        shr     cx, 14
        mov     ax, dx
        shl     ax, 2
        or      ax, cx
        ret

; DX:AX signed product -> AX=(product >> 8), low 16 bits.
math_q8_from_dxax:
        mov     cx, ax
        shr     cx, 8
        mov     ax, dx
        shl     ax, 8
        or      ax, cx
        ret
