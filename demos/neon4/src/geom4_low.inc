; Copyright (c) 2026 Nakata Maho
; Ported By Maho Nakata.
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the BSD 2-Clause conditions apply.

; ===========================================================================
; GEOM4_LOW.INC - NEON4 faithful analog-16 / 80286 renderer
; ---------------------------------------------------------------------------
; 0.28 policy: preserve the 256-derived composition; optimise arithmetic and VRAM hot paths.
; The same vertex tables, object centres, rotation angles, perspective depths,
; chapter timing, raster/card paths, ribbons, checker floor and finale motion
; are used by the 16-colour and 286 builds.  Colour is quantised only at the
; final primitive.  The 286 /200 mode additionally allows one fill span per physical Y.
; ===========================================================================

; 0.44 hot-path inline trig.  Same Q14 table/result as city_get_sin/cos,
; but avoids CALL/RET in scene motion code.  BX is preserved exactly as before.
%macro N4_SIN_INLINE 0
        push    bx
        xor     bx, bx
        mov     bl, al
        shl     bx, 1
        mov     ax, [sin_table + bx]
        pop     bx
%endmacro
%macro N4_COS_INLINE 0
        add     al, 64
        N4_SIN_INLINE
%endmacro

; Exact low-16 result macros used inside the vertex hot loop.  These are the
; same operations as low_q14_mul/low_mul_shr8 without per-product CALL/RET.
%macro LOW_Q14_MUL_HOT 0
        imul    bx
        shl     dx, 1
        shl     dx, 1
        shr     ax, 14
        or      ax, dx
%endmacro
%macro LOW_MUL_SHR8_HOT 0
        imul    bx
        shl     dx, 8
        shr     ax, 8
        or      ax, dx
%endmacro

city_get_sin:
        push    bx
        xor     bx, bx
        mov     bl, al
        shl     bx, 1
        mov     ax, [sin_table + bx]
        pop     bx
        ret
city_get_cos:
        add     al, 64
        jmp     city_get_sin

; signed AX * signed BX >> 14 -> AX, valid on 80286.
; 0.43: the caller only consumes AX, so build result bits 0..15 directly from
; DX:AX.  This removes the old CX save/restore and the unused sign-extension
; work from every rotation multiply.
low_q14_mul:
        imul    bx                      ; DX:AX = signed 16x16 product
        shl     dx, 1
        shl     dx, 1                   ; product bits 16..29 -> result bits 2..15
        shr     ax, 14                  ; product bits 14..15 -> result bits 0..1
        or      ax, dx
        ret

; signed AX * positive BX >> 8 -> AX.  Used by the 0.43 reciprocal-LUT
; perspective projector.  As above, only the low 16 bits of the shifted signed
; product are needed, so no 32-bit instruction is required.
low_mul_shr8:
        imul    bx                      ; DX:AX
        shl     dx, 8
        shr     ax, 8
        or      ax, dx
        ret

; Convert authored 0..255 physical colour to the matching low-colour hue/shade
; bucket.  The VA stage keeps the authored index so its packed RGB332 and
; direct G6/R5/B5 backends can convert the original 256-entry PEGC palette
; without collapsing it to a low-colour bucket.
; AL-only hot conversion for callers that do not need low_phys_to_16's
; historical AH/CX side effects.  80286 supports immediate shift counts.
%if (NEON4_STAGE == 8) && (NEON4_P5_BPP != 4)
%macro LOW_PHYS_AL_TO_16_HOT 0
        ; VA direct-colour path: preserve the source palette index.
%endmacro
%else
%macro LOW_PHYS_AL_TO_16_HOT 0
        cmp     al, 16
        jb      %%ready
        shr     al, 4
        and     al, 0Fh
%%ready:
%endmacro
%endif

low_phys_to_16:
%if (NEON4_STAGE == 8) && (NEON4_P5_BPP != 4)
        ret
%else
        push    cx
        cmp     al, 16
        jb      .done
        mov     ah, al
        mov     cl, 4
        shr     al, cl
        and     al, 0Fh
.done:
        pop     cx
        ret
%endif

low_use_color:
%if NEON4_STAGE == 8
        mov     [draw_color], al
%else
        and     al, 0Fh
        mov     [draw_color], al
%endif
        ; Tail-call the batch setup; its RET returns directly to our caller.
        jmp     line_batch_begin
low_use_phys_color:
        call    low_phys_to_16
        jmp     low_use_color

; ---------------------------------------------------------------------------
; 80286-safe triangle scan conversion.  It uses the same n4_tri_* vertices as
; the 256 renderer.  Fill mode 4 applies the identical +/-16px raster shear.
; ---------------------------------------------------------------------------
low_interp_x:
        push    bx
        push    cx
        push    dx
        mov     ax, [low_edge_y1]
        sub     ax, [low_edge_y0]
        or      ax, ax
        jnz     .normal
        mov     ax, [low_edge_x0]
        jmp     .done
.normal:
        mov     cx, ax
        mov     ax, [low_edge_x1]
        sub     ax, [low_edge_x0]
        mov     bx, [low_interp_y]
        sub     bx, [low_edge_y0]
        imul    bx
        idiv    cx
        add     ax, [low_edge_x0]
.done:
        pop     dx
        pop     cx
        pop     bx
        ret

low_swap_v01:
        mov     ax, [low_tx0]
        xchg    ax, [low_tx1]
        mov     [low_tx0], ax
        mov     ax, [low_ty0]
        xchg    ax, [low_ty1]
        mov     [low_ty0], ax
        ret
low_swap_v12:
        mov     ax, [low_tx1]
        xchg    ax, [low_tx2]
        mov     [low_tx1], ax
        mov     ax, [low_ty1]
        xchg    ax, [low_ty2]
        mov     [low_ty1], ax
        ret

low_dda_init:
        ; SI -> five-word state: current X, quotient, remainder, accumulator, DY.
        ; AX=x0 BX=y0 CX=x1 DX=y1.  Sorted triangle edges always have DY >= 0.
        push    ax
        push    bx
        push    cx
        push    dx
        push    bp
        mov     [si+0], ax
        mov     bp, dx
        sub     bp, bx
        or      bp, bp
        jnz     .normal
        mov     word [si+2], 0
        mov     word [si+4], 0
        mov     word [si+6], 0
        mov     word [si+8], 0
        jmp     .done
.normal:
        sub     cx, ax
        mov     ax, cx
        cwd
        idiv    bp
        mov     [si+2], ax
        mov     [si+4], dx
        mov     word [si+6], 0
        mov     [si+8], bp
.done:
        pop     bp
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; 0.50 hot DDA step.  This is instruction-for-instruction equivalent to the
; former low_dda_step subroutine, including AX/CX/DX preservation, but expands
; directly inside scanline loops so 286-class CPUs avoid CALL/RET twice per row.
%macro LOW_DDA_STEP_HOT 0
        push    ax
        push    cx
        push    dx
        mov     ax, [si+0]
        add     ax, [si+2]
        mov     dx, [si+6]
        add     dx, [si+4]
        mov     [si+6], dx
        mov     cx, [si+4]
        or      cx, cx
        jz      %%store_x
        js      %%negative_rem
        mov     cx, [si+8]
        cmp     dx, cx
        jl      %%store_x
        inc     ax
        sub     dx, cx
        mov     [si+6], dx
        jmp     %%store_x
%%negative_rem:
        mov     cx, [si+8]
        neg     cx
        cmp     dx, cx
        jg      %%store_x
        dec     ax
        mov     cx, [si+8]
        add     dx, cx
        mov     [si+6], dx
%%store_x:
        mov     [si+0], ax
        pop     dx
        pop     cx
        pop     ax
%endmacro

low_fill_triangle:
        pusha
        call    line_batch_begin
        mov     ax, [low_ty0]
        cmp     ax, [low_ty1]
        jle     .s01
        call    low_swap_v01
.s01:
        mov     ax, [low_ty1]
        cmp     ax, [low_ty2]
        jle     .s12
        call    low_swap_v12
.s12:
        mov     ax, [low_ty0]
        cmp     ax, [low_ty1]
        jle     .sorted
        call    low_swap_v01
.sorted:
        ; Long edge 0->2.
        mov     ax, [low_tx0]
        mov     bx, [low_ty0]
        mov     cx, [low_tx2]
        mov     dx, [low_ty2]
        mov     si, low_dda_a
        call    low_dda_init

        ; Short edge is 0->1 until y1, then 1->2.  The old converter switches
        ; to the lower edge at y == y1; keep that exact ownership rule.
        mov     ax, [low_ty0]
        cmp     ax, [low_ty1]
        jl      .init_upper_b
        mov     byte [low_dda_b_lower], 1
        mov     ax, [low_tx1]
        mov     bx, [low_ty1]
        mov     cx, [low_tx2]
        mov     dx, [low_ty2]
        jmp     .init_b
.init_upper_b:
        mov     byte [low_dda_b_lower], 0
        mov     ax, [low_tx0]
        mov     bx, [low_ty0]
        mov     cx, [low_tx1]
        mov     dx, [low_ty1]
.init_b:
        mov     si, low_dda_b
        call    low_dda_init

        mov     ax, [low_ty0]
        mov     [low_tcury], ax
%ifdef N4_286_200_SCANLINE_THIN
        mov     word [low_last_physical_y], 08000h
%endif
.yloop:
        mov     ax, [low_tcury]
        cmp     ax, [low_ty2]
        jg      .done
        cmp     ax, 0
        jl      .next_y
        cmp     ax, SCREEN_H-1
        jg      .done

%ifdef N4_286_200_SCANLINE_THIN
        ; 0.28 / 200-line optimisation: only one filled logical scanline is
        ; emitted for each physical scanline.  DDA geometry still advances at
        ; every logical Y, so 400-line mode and all outline/wire motion remain
        ; unchanged.  /400 bypasses this test completely.
        cmp     byte [video_400_mode], 0
        jne     .scanline_ready
        mov     dx, ax
        sar     dx, 1
        cmp     dx, [low_last_physical_y]
        je      .next_y
        mov     [low_last_physical_y], dx
.scanline_ready:
%endif
        mov     ax, [low_dda_a+0]
        mov     cx, [low_dda_b+0]

        cmp     byte [n4_fill_mode], 4
        jne     .no_shear
        mov     bx, [city_global_frame]
        shl     bx, 1
        mov     si, [low_tcury]
        mov     dx, si
        shl     si, 1
        add     si, dx                      ; y*3
        add     bx, si
        xor     dx, dx
        mov     dl, [n4_poly_raster_phase_bias]
        add     bx, dx
        and     bx, 00FFh
        shl     bx, 1
        mov     dx, [sin_table + bx]
        sar     dx, N4_POLY_RASTER_SHIFT
        add     ax, dx
        add     cx, dx
        dec     ax
        inc     cx
.no_shear:
        ; Optional two-colour face dither.  This runs independently of fill
        ; mode 4, so raster-sheared faces keep their geometry while still
        ; retaining low-colour surface separation.
        cmp     byte [low_face_dither_enable], 0
        je      .row_colour_ready
        push    ax
        push    cx
        mov     dx, [low_tcury]
%ifdef NEON4_286
        cmp     byte [video_400_mode], 0
        jne     .dither_y_ready
        sar     dx, 1
.dither_y_ready:
%endif
        xor     dl, [low_face_dither_phase]
        test    dl, 1
        jnz     .row_colour_b
        mov     al, [low_fill_colour16_a]
        jmp     .row_set_colour
.row_colour_b:
        mov     al, [low_fill_colour16_b]
.row_set_colour:
        mov     [draw_color], al
        call    line_batch_begin
        pop     cx
        pop     ax
.row_colour_ready:
        mov     bx, [low_tcury]
        call    hline_set16_same_fast

.next_y:
        mov     ax, [low_tcury]
        cmp     ax, [low_ty2]
        jge     .done

        mov     si, low_dda_a
        LOW_DDA_STEP_HOT
        mov     si, low_dda_b
        LOW_DDA_STEP_HOT
        inc     word [low_tcury]

        cmp     byte [low_dda_b_lower], 0
        jne     .yloop
        mov     ax, [low_tcury]
        cmp     ax, [low_ty1]
        jne     .yloop
        ; Reinitialise edge B exactly at vertex 1.  The just-completed upper
        ; edge already arrived at x1, and the lower edge begins from the same x.
        mov     byte [low_dda_b_lower], 1
        mov     ax, [low_tx1]
        mov     bx, [low_ty1]
        mov     cx, [low_tx2]
        mov     dx, [low_ty2]
        mov     si, low_dda_b
        call    low_dda_init
        jmp     .yloop
.done:
        popa
        ret

; ---------------------------------------------------------------------------
; 0.45 low-colour convex-quad scan converter.
;
; Cube/morph quads used to be split into v0-v1-v2 and v0-v2-v3.  Even with a
; dither-matched repair line, the two independent integer DDAs can disagree by
; a pixel NEXT TO the authored diagonal on occasional projected square faces.
; This path walks the four real perimeter edges once and emits one union span
; per Y.  Therefore the internal diagonal does not exist at all.
;
; The four vertices in low_quad_x/low_quad_y must be convex and cyclic, which
; is true for the visible cube/morph faces after the existing back-face test.
; ---------------------------------------------------------------------------
low_quad_edge_init:
        ; AX=x0 BX=y0 CX=x1 DX=y1, SI=DDA state, DI=minY/maxY pair.
        ; low_dda_init expects non-negative DY, so orient every perimeter edge
        ; from top to bottom.  Horizontal edges are retained as zero-DY states
        ; and skipped by the scan loop; their adjacent edges own that row.
        cmp     bx, dx
        jle     .ordered
        xchg    ax, cx
        xchg    bx, dx
.ordered:
        mov     [di+0], bx
        mov     [di+2], dx
        ; Tail-call: low_dda_init preserves the same caller-visible registers.
        jmp     low_dda_init

low_fill_quad_convex:
        pusha
        ; 0.50: cache both 4-bit dither shades once per quad instead of
        ; remapping the same two physical palette values on every scanline.
        mov     al, [n4_fill_colour_a]
        LOW_PHYS_AL_TO_16_HOT
        mov     [low_fill_colour16_a], al
        mov     [draw_color], al
        mov     al, [n4_fill_colour_b]
        LOW_PHYS_AL_TO_16_HOT
        mov     [low_fill_colour16_b], al
        call    line_batch_begin

        ; Overall Y envelope.
        mov     ax, [low_quad_y+0]
        mov     [low_quad_min_y], ax
        mov     [low_quad_max_y], ax
        mov     si, 2
.find_y:
        cmp     si, 8
        jae     .y_ready
        mov     ax, [low_quad_y+si]
        cmp     ax, [low_quad_min_y]
        jge     .not_min
        mov     [low_quad_min_y], ax
.not_min:
        cmp     ax, [low_quad_max_y]
        jle     .not_max
        mov     [low_quad_max_y], ax
.not_max:
        add     si, 2
        jmp     .find_y
.y_ready:

        ; Edge 0: v0 -> v1.
        mov     ax, [low_quad_x+0]
        mov     bx, [low_quad_y+0]
        mov     cx, [low_quad_x+2]
        mov     dx, [low_quad_y+2]
        mov     si, low_quad_edge0
        mov     di, low_quad_edge_range+0
        call    low_quad_edge_init
        ; Edge 1: v1 -> v2.
        mov     ax, [low_quad_x+2]
        mov     bx, [low_quad_y+2]
        mov     cx, [low_quad_x+4]
        mov     dx, [low_quad_y+4]
        mov     si, low_quad_edge1
        mov     di, low_quad_edge_range+4
        call    low_quad_edge_init
        ; Edge 2: v2 -> v3.
        mov     ax, [low_quad_x+4]
        mov     bx, [low_quad_y+4]
        mov     cx, [low_quad_x+6]
        mov     dx, [low_quad_y+6]
        mov     si, low_quad_edge2
        mov     di, low_quad_edge_range+8
        call    low_quad_edge_init
        ; Edge 3: v3 -> v0.
        mov     ax, [low_quad_x+6]
        mov     bx, [low_quad_y+6]
        mov     cx, [low_quad_x+0]
        mov     dx, [low_quad_y+0]
        mov     si, low_quad_edge3
        mov     di, low_quad_edge_range+12
        call    low_quad_edge_init

        mov     ax, [low_quad_min_y]
        mov     [low_quad_cur_y], ax
%ifdef N4_286_200_SCANLINE_THIN
        mov     word [low_last_physical_y], 08000h
%endif
.yloop:
        mov     word [low_quad_span_l], 07FFFh
        mov     word [low_quad_span_r], 08000h
        mov     dx, [low_quad_cur_y]

        ; Collect edge 0 intersection and advance it for the next logical row.
        mov     ax, [low_quad_edge_range+0]
        cmp     ax, [low_quad_edge_range+2]
        je      .e0_done
        cmp     dx, ax
        jl      .e0_done
        cmp     dx, [low_quad_edge_range+2]
        jg      .e0_done
        mov     ax, [low_quad_edge0+0]
        cmp     ax, [low_quad_span_l]
        jge     .e0_not_l
        mov     [low_quad_span_l], ax
.e0_not_l:
        cmp     ax, [low_quad_span_r]
        jle     .e0_not_r
        mov     [low_quad_span_r], ax
.e0_not_r:
        cmp     dx, [low_quad_edge_range+2]
        jge     .e0_done
        mov     si, low_quad_edge0
        LOW_DDA_STEP_HOT
.e0_done:

        ; Edge 1.
        mov     dx, [low_quad_cur_y]
        mov     ax, [low_quad_edge_range+4]
        cmp     ax, [low_quad_edge_range+6]
        je      .e1_done
        cmp     dx, ax
        jl      .e1_done
        cmp     dx, [low_quad_edge_range+6]
        jg      .e1_done
        mov     ax, [low_quad_edge1+0]
        cmp     ax, [low_quad_span_l]
        jge     .e1_not_l
        mov     [low_quad_span_l], ax
.e1_not_l:
        cmp     ax, [low_quad_span_r]
        jle     .e1_not_r
        mov     [low_quad_span_r], ax
.e1_not_r:
        cmp     dx, [low_quad_edge_range+6]
        jge     .e1_done
        mov     si, low_quad_edge1
        LOW_DDA_STEP_HOT
.e1_done:

        ; Edge 2.
        mov     dx, [low_quad_cur_y]
        mov     ax, [low_quad_edge_range+8]
        cmp     ax, [low_quad_edge_range+10]
        je      .e2_done
        cmp     dx, ax
        jl      .e2_done
        cmp     dx, [low_quad_edge_range+10]
        jg      .e2_done
        mov     ax, [low_quad_edge2+0]
        cmp     ax, [low_quad_span_l]
        jge     .e2_not_l
        mov     [low_quad_span_l], ax
.e2_not_l:
        cmp     ax, [low_quad_span_r]
        jle     .e2_not_r
        mov     [low_quad_span_r], ax
.e2_not_r:
        cmp     dx, [low_quad_edge_range+10]
        jge     .e2_done
        mov     si, low_quad_edge2
        LOW_DDA_STEP_HOT
.e2_done:

        ; Edge 3.
        mov     dx, [low_quad_cur_y]
        mov     ax, [low_quad_edge_range+12]
        cmp     ax, [low_quad_edge_range+14]
        je      .edges_done
        cmp     dx, ax
        jl      .edges_done
        cmp     dx, [low_quad_edge_range+14]
        jg      .edges_done
        mov     ax, [low_quad_edge3+0]
        cmp     ax, [low_quad_span_l]
        jge     .e3_not_l
        mov     [low_quad_span_l], ax
.e3_not_l:
        cmp     ax, [low_quad_span_r]
        jle     .e3_not_r
        mov     [low_quad_span_r], ax
.e3_not_r:
        cmp     dx, [low_quad_edge_range+14]
        jge     .edges_done
        mov     si, low_quad_edge3
        LOW_DDA_STEP_HOT
.edges_done:

        ; No non-horizontal perimeter intersection on this row.
        cmp     word [low_quad_span_l], 07FFFh
        je      .next_y

        mov     dx, [low_quad_cur_y]
        cmp     dx, 0
        jl      .next_y
        cmp     dx, SCREEN_H-1
        jg      .done

%ifdef N4_286_200_SCANLINE_THIN
        cmp     byte [video_400_mode], 0
        jne     .scanline_ready
        mov     ax, dx
        sar     ax, 1
        cmp     ax, [low_last_physical_y]
        je      .next_y
        mov     [low_last_physical_y], ax
.scanline_ready:
%endif

        mov     ax, [low_quad_span_l]
        mov     cx, [low_quad_span_r]

        ; Same per-row raster shear as the triangle converter.
        cmp     byte [n4_fill_mode], 4
        jne     .no_shear
        mov     bx, [city_global_frame]
        shl     bx, 1
        mov     si, [low_quad_cur_y]
        mov     dx, si
        shl     si, 1
        add     si, dx                      ; y*3
        add     bx, si
        xor     dx, dx
        mov     dl, [n4_poly_raster_phase_bias]
        add     bx, dx
        and     bx, 00FFh
        shl     bx, 1
        mov     dx, [sin_table + bx]
        sar     dx, N4_POLY_RASTER_SHIFT
        add     ax, dx
        add     cx, dx
        dec     ax
        inc     cx
.no_shear:

        ; One absolute-Y dither decision for the whole quad span.  Unlike the
        ; old two-triangle path, no second rasterizer can restart or round a row
        ; independently on the other side of the face.
        cmp     byte [low_face_dither_enable], 0
        je      .row_colour_ready
        push    ax
        push    cx
        mov     dx, [low_quad_cur_y]
%ifdef NEON4_286
        cmp     byte [video_400_mode], 0
        jne     .dither_y_ready
        sar     dx, 1
.dither_y_ready:
%endif
        xor     dl, [low_face_dither_phase]
        test    dl, 1
        jnz     .row_colour_b
        mov     al, [low_fill_colour16_a]
        jmp     .row_set_colour
.row_colour_b:
        mov     al, [low_fill_colour16_b]
.row_set_colour:
        mov     [draw_color], al
        call    line_batch_begin
        pop     cx
        pop     ax
.row_colour_ready:
        mov     bx, [low_quad_cur_y]
        call    hline_set16_same_fast

.next_y:
        mov     ax, [low_quad_cur_y]
        cmp     ax, [low_quad_max_y]
        jge     .done
        inc     word [low_quad_cur_y]
        jmp     .yloop
.done:
        popa
        ret

n4_fill_triangle:
        pusha
        ; 0.50: convert the two authored physical shades once per face.
        mov     al, [n4_fill_colour_a]
        LOW_PHYS_AL_TO_16_HOT
        mov     [low_fill_colour16_a], al
        mov     [draw_color], al
        mov     al, [n4_fill_colour_b]
        LOW_PHYS_AL_TO_16_HOT
        mov     [low_fill_colour16_b], al
        mov     ax, [n4_tri_x0]
        mov     [low_tx0], ax
        mov     ax, [n4_tri_y0]
        mov     [low_ty0], ax
        mov     ax, [n4_tri_x1]
        mov     [low_tx1], ax
        mov     ax, [n4_tri_y1]
        mov     [low_ty1], ax
        mov     ax, [n4_tri_x2]
        mov     [low_tx2], ax
        mov     ax, [n4_tri_y2]
        mov     [low_ty2], ax
        call    low_fill_triangle
        popa
        ret

; Low-colour page clearing is handled in VIDEO4_LOW.INC.  0.43 gives ordinary
; scenes a conservative page-local envelope clear after the first full clear;
; RASTER TRANSFER and SURFACE WAVE retain their specialised erase paths.
n4_geo_frame_begin:
n4_geo_frame_end:
n4_geo_set_dirty_rect:
        ret

; ---------------------------------------------------------------------------
; Faithful 3-D transform using the SAME source vertex tables as GEOM4_256.
; Rotation remains 286-safe signed 16x16->32 arithmetic.  0.43 also shares the
; 8-Z-unit reciprocal projection LUT, removing the two per-vertex IDIVs without
; reducing vertex count or animation precision.
; ---------------------------------------------------------------------------
n4_geo_set_angles:
        pusha
        mov     [n4_geo_angle_y], al
        mov     [n4_geo_angle_x], ah
        ; 0.43: this routine already saves BX, so read the four sin-table values
        ; directly instead of nesting four city_get_sin/cos call/save pairs.
        xor     bx, bx
        mov     bl, [n4_geo_angle_y]
        shl     bx, 1
        mov     ax, [sin_table + bx]
        mov     [n4_geo_sin_y], ax
        xor     bx, bx
        mov     bl, [n4_geo_angle_y]
        add     bl, 64
        shl     bx, 1
        mov     ax, [sin_table + bx]
        mov     [n4_geo_cos_y], ax
        xor     bx, bx
        mov     bl, [n4_geo_angle_x]
        shl     bx, 1
        mov     ax, [sin_table + bx]
        mov     [n4_geo_sin_x], ax
        xor     bx, bx
        mov     bl, [n4_geo_angle_x]
        add     bl, 64
        shl     bx, 1
        mov     ax, [sin_table + bx]
        mov     [n4_geo_cos_x], ax
        popa
        ret

n4_geo_transform_vertices:
        pusha
        mov     [n4_geo_vertex_count], cx
        xor     di, di
.vertex:
        cmp     di, [n4_geo_vertex_count]
        jae     .done

        ; xr = x*cy + z*sy
        mov     ax, [si]
        mov     bx, [n4_geo_cos_y]
        LOW_Q14_MUL_HOT
        mov     [low_geo_xr], ax
        mov     ax, [si+4]
        mov     bx, [n4_geo_sin_y]
        LOW_Q14_MUL_HOT
        add     [low_geo_xr], ax

        ; zr = z*cy - x*sy
        mov     ax, [si+4]
        mov     bx, [n4_geo_cos_y]
        LOW_Q14_MUL_HOT
        mov     [low_geo_zr], ax
        mov     ax, [si]
        mov     bx, [n4_geo_sin_y]
        LOW_Q14_MUL_HOT
        sub     [low_geo_zr], ax

        ; yr = y*cx - zr*sx
        mov     ax, [si+2]
        mov     bx, [n4_geo_cos_x]
        LOW_Q14_MUL_HOT
        mov     [low_geo_yr], ax
        mov     ax, [low_geo_zr]
        mov     bx, [n4_geo_sin_x]
        LOW_Q14_MUL_HOT
        sub     [low_geo_yr], ax

        ; z = y*sx + zr*cx + object depth
        mov     ax, [si+2]
        mov     bx, [n4_geo_sin_x]
        LOW_Q14_MUL_HOT
        mov     [low_geo_z], ax
        mov     ax, [low_geo_zr]
        mov     bx, [n4_geo_cos_x]
        LOW_Q14_MUL_HOT
        add     [low_geo_z], ax
        mov     ax, [n4_geo_object_depth]
        add     [low_geo_z], ax
        cmp     word [low_geo_z], 32
        jge     .z_near_ok
        mov     word [low_geo_z], 32
.z_near_ok:
        cmp     word [low_geo_z], 4095
        jle     .z_far_ok
        mov     word [low_geo_z], 4095
.z_far_ok:

        mov     bx, di
        shl     bx, 1
        mov     ax, [low_geo_z]
        mov     [n4_geo_depth_z + bx], ax

        ; 0.43 perspective projection: use the same 8-Z-unit reciprocal LUT as
        ; the 256-colour renderer.  This removes two IDIV instructions per
        ; transformed vertex, a major win on 286-class CPUs.
        mov     bx, [low_geo_z]
        shr     bx, N4_GEO_RECIP_SHIFT
        cmp     bx, 1
        jae     .ri_low_ok
        mov     bx, 1
.ri_low_ok:
        cmp     bx, 511
        jbe     .ri_ready
        mov     bx, 511
.ri_ready:
        shl     bx, 1
        mov     bx, [n4_geo_recip_lut + bx]
        mov     [low_geo_recip], bx

        ; sx = center_x + xr * reciprocal >> 8
        mov     ax, [low_geo_xr]
        LOW_MUL_SHR8_HOT
        add     ax, [n4_geo_center_x]
        mov     bx, di
        shl     bx, 1
        mov     [n4_geo_screen_x + bx], ax

        ; sy = center_y - yr * reciprocal >> 8
        mov     ax, [low_geo_yr]
        mov     bx, [low_geo_recip]
        LOW_MUL_SHR8_HOT
        neg     ax
        add     ax, [n4_geo_center_y]
        mov     bx, di
        shl     bx, 1
        mov     [n4_geo_screen_y + bx], ax

        add     si, 6
        inc     di
        jmp     .vertex
.done:
        popa
        ret

; Return AL=1 for positive/front-facing cross product, AL=0 otherwise.
low_face_is_front:
        push    bx
        push    cx
        push    dx
        mov     ax, [n4_tri_x1]
        sub     ax, [n4_tri_x0]
        mov     bx, [n4_tri_y2]
        sub     bx, [n4_tri_y0]
        imul    bx
        mov     [low_cross1_lo], ax
        mov     [low_cross1_hi], dx
        mov     ax, [n4_tri_x2]
        sub     ax, [n4_tri_x0]
        mov     bx, [n4_tri_y1]
        sub     bx, [n4_tri_y0]
        imul    bx
        sub     [low_cross1_lo], ax
        sbb     [low_cross1_hi], dx
        ; Keep the complete positive projected area for the direct-colour
        ; profile.  The original 256-colour renderer derives its ramp shade
        ; from this value; the 16-colour profile continues to use its graph
        ; colour classes below.
        mov     ax, [low_cross1_lo]
        mov     [n4_geo_face_cross], ax
        mov     ax, [low_cross1_hi]
        mov     [n4_geo_face_cross+2], ax
        mov     ax, [low_cross1_hi]
        or      ax, ax
        js      .no
        jnz     .yes
        cmp     word [low_cross1_lo], N4_GEO_MIN_CROSS
        jbe     .no
.yes:
        mov     al, 1
        jmp     .ret
.no:
        xor     al, al
.ret:
        pop     dx
        pop     cx
        pop     bx
        ret

; Select a graph-coloured face pair.  AL=colour A, AH=colour B.
; n4_geo_colour_base chooses the theme; BP chooses the shape-specific face
; class through low_face_class_ptr.  Class tables are authored so adjacent real
; surfaces never use the same class.
low_face_colour:
        push    bx
        push    cx
        push    dx
        push    si

%if (NEON4_STAGE == 8) && (NEON4_P5_BPP != 4)
        ; Direct-colour profiles retain the original NEON4 256-entry palette
        ; index and shade calculation.  The projected face area is positive
        ; here and is converted exactly as n4_geo_colour_from_cross in the
        ; original 256-colour source: shade = clamp(cross >> 11 + 8, 8, 31),
        ; A = base + shade, B = A - 3.  p5_emit_color_if_needed later maps
        ; those source indices independently to RGB332 or G6/R5/B5.
        mov     ax, [n4_geo_face_cross+2]
        or      ax, ax
        jnz     .direct_shade_max
        mov     dx, [n4_geo_face_cross]
        mov     cl, 11
        shr     dx, cl
        add     dx, 8
        cmp     dx, 31
        jbe     .direct_shade_ready
.direct_shade_max:
        mov     dx, 31
.direct_shade_ready:
        add     dl, [n4_geo_colour_base]
        mov     al, dl
        mov     ah, dl
        sub     ah, 3
        jmp     .done
%endif

        mov     si, [low_face_class_ptr]
        mov     bx, bp
        mov     dl, [si+bx]
        and     dl, 3                       ; class 0..3

        xor     ax, ax
        mov     al, [n4_geo_colour_base]
        cmp     al, 16
        jb      .theme_zero
        mov     cl, 5                       ; 32-colour authored bank -> theme 1..7
        shr     al, cl
        cmp     al, 7
        jbe     .theme_ready
        mov     al, 7
        jmp     .theme_ready
.theme_zero:
        xor     al, al
.theme_ready:
        xor     ah, ah
        mov     bx, ax
        shl     bx, 1                       ; *2
        shl     bx, 1                       ; *4
        shl     bx, 1                       ; *8 bytes/theme
        xor     ax, ax
        mov     al, dl
        shl     ax, 1                       ; pair offset
        add     bx, ax
        mov     al, [low_face_theme_pairs+bx]
        mov     ah, [low_face_theme_pairs+bx+1]

.done:
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret

n4_geo_draw_tri_faces:
        pusha
        mov     byte [low_face_dither_enable], 0
        mov     [n4_geo_face_count], cx
        xor     bp, bp
.face:
        cmp     bp, [n4_geo_face_count]
        jae     .done
        xor     bx, bx
        mov     bl, [si]
        shl     bx, 1
        mov     ax, [n4_geo_screen_x+bx]
        mov     [n4_tri_x0], ax
        mov     ax, [n4_geo_screen_y+bx]
        mov     [n4_tri_y0], ax
        xor     bx, bx
        mov     bl, [si+1]
        shl     bx, 1
        mov     ax, [n4_geo_screen_x+bx]
        mov     [n4_tri_x1], ax
        mov     ax, [n4_geo_screen_y+bx]
        mov     [n4_tri_y1], ax
        xor     bx, bx
        mov     bl, [si+2]
        shl     bx, 1
        mov     ax, [n4_geo_screen_x+bx]
        mov     [n4_tri_x2], ax
        mov     ax, [n4_geo_screen_y+bx]
        mov     [n4_tri_y2], ax
        call    low_face_is_front
        or      al, al
        jz      .next
        call    low_face_colour
        mov     [n4_fill_colour_a], al
        mov     [n4_fill_colour_b], ah
        mov     ax, bp
        and     al, 1
        mov     [low_face_dither_phase], al
%if (NEON4_STAGE == 8) && (NEON4_P5_BPP != 4)
        ; The original 256-colour path writes one shaded palette index per
        ; face.  Dither is a low-colour-only presentation rule.
        mov     byte [low_face_dither_enable], 0
%else
        mov     byte [low_face_dither_enable], 1
%endif
        mov     al, [n4_geo_fill_mode]
        mov     [n4_fill_mode], al
        call    n4_fill_triangle
.next:
        add     si, 4
        inc     bp
        jmp     .face
.done:
        mov     byte [low_face_dither_enable], 0
        popa
        ret

n4_geo_draw_quad_faces:
        pusha
        mov     byte [low_face_dither_enable], 0
        mov     [n4_geo_face_count], cx
        xor     bp, bp
.face:
        cmp     bp, [n4_geo_face_count]
        jae     .done

        ; Load the cyclic quad once.  v0/v1/v2 are also mirrored into the
        ; existing triangle scratch solely for the established back-face test.
        xor     bx, bx
        mov     bl, [si]
        shl     bx, 1
        mov     ax, [n4_geo_screen_x+bx]
        mov     [n4_tri_x0], ax
        mov     [low_quad_x+0], ax
        mov     ax, [n4_geo_screen_y+bx]
        mov     [n4_tri_y0], ax
        mov     [low_quad_y+0], ax

        xor     bx, bx
        mov     bl, [si+1]
        shl     bx, 1
        mov     ax, [n4_geo_screen_x+bx]
        mov     [n4_tri_x1], ax
        mov     [low_quad_x+2], ax
        mov     ax, [n4_geo_screen_y+bx]
        mov     [n4_tri_y1], ax
        mov     [low_quad_y+2], ax

        xor     bx, bx
        mov     bl, [si+2]
        shl     bx, 1
        mov     ax, [n4_geo_screen_x+bx]
        mov     [n4_tri_x2], ax
        mov     [low_quad_x+4], ax
        mov     ax, [n4_geo_screen_y+bx]
        mov     [n4_tri_y2], ax
        mov     [low_quad_y+4], ax

        xor     bx, bx
        mov     bl, [si+3]
        shl     bx, 1
        mov     ax, [n4_geo_screen_x+bx]
        mov     [low_quad_x+6], ax
        mov     ax, [n4_geo_screen_y+bx]
        mov     [low_quad_y+6], ax

        call    low_face_is_front
        or      al, al
        jz      .next
        call    low_face_colour
        mov     [n4_fill_colour_a], al
        mov     [n4_fill_colour_b], ah
        mov     ax, bp
        and     al, 1
        mov     [low_face_dither_phase], al
%if (NEON4_STAGE == 8) && (NEON4_P5_BPP != 4)
        ; The original 256-colour path writes one shaded palette index per
        ; face.  Dither is a low-colour-only presentation rule.
        mov     byte [low_face_dither_enable], 0
%else
        mov     byte [low_face_dither_enable], 1
%endif
        mov     al, [n4_geo_fill_mode]
        mov     [n4_fill_mode], al

        ; 0.45: one perimeter-based convex-quad fill.  No v0-v2 split, no
        ; diagonal repair pass, and one dither phase decision per scanline.
        call    low_fill_quad_convex
.next:
        add     si, 4
        inc     bp
        jmp     .face
.done:
        mov     byte [low_face_dither_enable], 0
        popa
        ret

n4_geo_wire_line_safe:
        cmp     ax, 0
        jl      .done
        cmp     ax, SCREEN_W-1
        jg      .done
        cmp     cx, 0
        jl      .done
        cmp     cx, SCREEN_W-1
        jg      .done
        cmp     bx, 0
        jl      .done
        cmp     bx, SCREEN_H-1
        jg      .done
        cmp     dx, 0
        jl      .done
        cmp     dx, SCREEN_H-1
        jg      .done
        ; Tail-call the line writer; avoid one nested CALL/RET per cage edge.
        jmp     line_set_same_colour
.done:
        ret

; Exact far/solid/near cage ordering from 256 version, using projected midpoint
; for an edge that crosses the object-centre depth.
n4_geo_draw_cube_cage_pass:
        pusha
        mov     [low_wire_pass], al
        mov     al, [draw_color]
        call    low_phys_to_16
        mov     [draw_color], al
        mov     si, n4_geo_cube_cage_vertices
        mov     cx, 8
        call    n4_geo_transform_vertices
        call    line_batch_begin
        mov     si, n4_geo_cube_edges
        mov     bp, 12
.edge:
        or      bp, bp
        jz      .done
        xor     bx, bx
        mov     bl, [si]
        shl     bx, 1
        mov     ax, [n4_geo_screen_x+bx]
        mov     [low_wire_x0], ax
        mov     ax, [n4_geo_screen_y+bx]
        mov     [low_wire_y0], ax
        mov     ax, [n4_geo_depth_z+bx]
        mov     [low_wire_z0], ax
        xor     bx, bx
        mov     bl, [si+1]
        shl     bx, 1
        mov     ax, [n4_geo_screen_x+bx]
        mov     [low_wire_x1], ax
        mov     ax, [n4_geo_screen_y+bx]
        mov     [low_wire_y1], ax
        mov     ax, [n4_geo_depth_z+bx]
        mov     [low_wire_z1], ax

        mov     ax, [low_wire_z0]
        cmp     ax, [n4_geo_object_depth]
        jl      .z0_near
        ; z0 far
        mov     ax, [low_wire_z1]
        cmp     ax, [n4_geo_object_depth]
        jl      .split_far_near
        cmp     byte [low_wire_pass], 0
        jne     .next
        jmp     .full
.z0_near:
        mov     ax, [low_wire_z1]
        cmp     ax, [n4_geo_object_depth]
        jge     .split_near_far
        cmp     byte [low_wire_pass], 1
        jne     .next
        jmp     .full
.split_far_near:
        mov     ax, [low_wire_x0]
        add     ax, [low_wire_x1]
        sar     ax, 1
        mov     [low_wire_mx], ax
        mov     ax, [low_wire_y0]
        add     ax, [low_wire_y1]
        sar     ax, 1
        mov     [low_wire_my], ax
        cmp     byte [low_wire_pass], 0
        jne     .mid_to_1
        mov     ax, [low_wire_x0]
        mov     bx, [low_wire_y0]
        mov     cx, [low_wire_mx]
        mov     dx, [low_wire_my]
        call    n4_geo_wire_line_safe
        jmp     .next
.split_near_far:
        mov     ax, [low_wire_x0]
        add     ax, [low_wire_x1]
        sar     ax, 1
        mov     [low_wire_mx], ax
        mov     ax, [low_wire_y0]
        add     ax, [low_wire_y1]
        sar     ax, 1
        mov     [low_wire_my], ax
        cmp     byte [low_wire_pass], 1
        jne     .mid_to_1
        mov     ax, [low_wire_x0]
        mov     bx, [low_wire_y0]
        mov     cx, [low_wire_mx]
        mov     dx, [low_wire_my]
        call    n4_geo_wire_line_safe
        jmp     .next
.mid_to_1:
        mov     ax, [low_wire_mx]
        mov     bx, [low_wire_my]
        mov     cx, [low_wire_x1]
        mov     dx, [low_wire_y1]
        call    n4_geo_wire_line_safe
        jmp     .next
.full:
        mov     ax, [low_wire_x0]
        mov     bx, [low_wire_y0]
        mov     cx, [low_wire_x1]
        mov     dx, [low_wire_y1]
        call    n4_geo_wire_line_safe
.next:
        add     si, 2
        dec     bp
        jmp     .edge
.done:
        popa
        ret

n4_geo_draw_cube:
        push    si
        push    cx
        mov     word [low_face_class_ptr], low_face_class_cube
        mov     si, n4_geo_cube_vertices
        mov     cx, 8
        call    n4_geo_transform_vertices
        mov     si, n4_geo_cube_quads
        mov     cx, 6
        call    n4_geo_draw_quad_faces
        pop     cx
        pop     si
        ret
n4_geo_draw_octa:
        push    si
        push    cx
        mov     word [low_face_class_ptr], low_face_class_octa
        mov     si, n4_geo_octa_vertices
        mov     cx, 6
        call    n4_geo_transform_vertices
        mov     si, n4_geo_octa_faces
        mov     cx, 8
        call    n4_geo_draw_tri_faces
        pop     cx
        pop     si
        ret
n4_geo_draw_tetra:
        push    si
        push    cx
        mov     word [low_face_class_ptr], low_face_class_tetra
        mov     si, n4_geo_tetra_vertices
        mov     cx, 4
        call    n4_geo_transform_vertices
        mov     si, n4_geo_tetra_faces
        mov     cx, 4
        call    n4_geo_draw_tri_faces
        pop     cx
        pop     si
        ret
n4_geo_draw_pyramid:
        push    si
        push    cx
        mov     word [low_face_class_ptr], low_face_class_pyramid
        mov     si, n4_geo_pyramid_vertices
        mov     cx, 5
        call    n4_geo_transform_vertices
        mov     si, n4_geo_pyramid_faces
        mov     cx, 6
        call    n4_geo_draw_tri_faces
        pop     cx
        pop     si
        ret
n4_geo_draw_icosa:
        push    si
        push    cx
        mov     word [low_face_class_ptr], low_face_class_icosa
        mov     si, n4_geo_icosa_vertices
        mov     cx, 12
        call    n4_geo_transform_vertices
        mov     si, n4_geo_icosa_faces
        mov     cx, 20
        call    n4_geo_draw_tri_faces
        pop     cx
        pop     si
        ret

; ---------------------------------------------------------------------------
; Low-colour emulation of the 256-version raster/BitBlt assets.
; Geometry, nominal positions and per-scanline warp phase are identical.
; ---------------------------------------------------------------------------
%define N4_RASTER_WAVE_SHIFT 9
%define N4_RASTER_PHASE_STEP 5

; The low-colour page tail is exactly large enough for both immutable assets:
; 48x48 CAT = 288 bytes/plane and 96x40 raster = 480 bytes/plane.  Build them
; once on each graphics page with GRCG, then RASTER TRANSFER can use the EGC
; shifter for true VRAM-to-VRAM copies.  If the startup EGC probe failed, the
; existing span renderer below remains the exact fallback.
n4_blit_ensure_asset:
        pusha
        cmp     byte [low_egc_available], 0
        je      .done
        xor     ax, ax
        mov     al, [egc16_saved_page]
        and     al, 1
        mov     cl, al
        mov     bl, 1
        shl     bl, cl
        test    [low_egc_asset_valid], bl
        jnz     .done
        call    low_egc_build_assets_current_page
        xor     ax, ax
        mov     al, [egc16_saved_page]
        and     al, 1
        mov     cl, al
        mov     bl, 1
        shl     bl, cl
        or      [low_egc_asset_valid], bl
.done:
        popa
        ret

; Enter EGC only once for the whole scene batch.
; 0.34 no longer performs the old black guard-word authoring here: the revised
; row blitters consume their retained tail with a destination-only STOSW, so a
; GRCG colour setup and VRAM write on every RASTER TRANSFER frame are avoided.
n4_blit_batch_begin:
        pusha
        mov     byte [low_raster_egc_used], 0
        cmp     byte [low_egc_batch_active], 0
        jne     .already_active
        cmp     byte [low_egc_available], 0
        je      .done
        mov     al, [draw_color]
        mov     [low_egc_saved_draw_color], al
        call    egc16_enable_vram_copy
        mov     byte [low_egc_batch_active], 1
.already_active:
        mov     byte [low_raster_egc_used], 1
.done:
        popa
        ret

n4_blit_batch_end:
        pusha
        cmp     byte [low_egc_batch_active], 0
        je      .done
        call    egc16_disable_to_grcg
        mov     al, [low_egc_saved_draw_color]
        mov     [draw_color], al
        mov     byte [low_egc_batch_active], 0
.done:
        popa
        ret

; BX=packed planar row base, AX=x0, CX=x1 (inclusive), ES=A800h.
; Current GRCG RMW colour is already prepared.  This private builder bypasses
; visible-screen clipping so it can author the off-screen 32000..32767 tail.
low_egc_cache_span_same:
        pusha
        cmp     ax, cx
        jle     .ordered
        xchg    ax, cx
.ordered:
        mov     bp, cx                    ; preserve x1 before CL becomes a shift count
        mov     di, bx
        mov     si, ax
        shr     si, 3
        add     di, si
        mov     dx, ax
        and     dx, 7
        mov     cl, dl
        mov     al, 0FFh
        shr     al, cl
        mov     [low_hstart], al
        mov     ax, bp
        mov     dx, ax
        shr     bp, 3
        and     dx, 7
        mov     cl, 7
        sub     cl, dl
        mov     al, 0FFh
        shl     al, cl
        mov     [low_hend], al
        mov     ax, si
        cmp     bp, ax
        jne     .multiple
        mov     al, [low_hstart]
        and     al, [low_hend]
        mov     [es:di], al
        jmp     .done
.multiple:
        mov     al, [low_hstart]
        stosb
        mov     cx, bp
        sub     cx, si
        dec     cx
        jcxz    .last
        mov     al, 0FFh
        rep     stosb
.last:
        mov     al, [low_hend]
        mov     [es:di], al
.done:
        popa
        ret

; Build the page-local CAT and raster sources once.  This path is intentionally
; simple rather than fast: it runs only once per page and mirrors the authored
; ordering of the old per-frame GRCG renderer exactly.
low_egc_build_assets_current_page:
        pusha
        push    es
        cld
        mov     al, [draw_color]
        mov     [low_egc_saved_draw_color], al
        mov     ax, VRAM_B_SEG
        mov     es, ax

        ; Black the exact 768-byte cache tail and the one-word flush guard.
        mov     byte [draw_color], 0
        call    grcg16_prepare_color
        mov     di, LOW_EGC_CAT_BASE
        mov     ax, 0FFFFh
        mov     cx, (LOW_EGC_CAT_BYTES+LOW_EGC_RASTER_BYTES)/2
        rep     stosw
        mov     word [es:LOW_EGC_GUARD_OFFSET], 0FFFFh

        ; CAT background: 48 complete rows.
        mov     al, [n4_cat48_box_fill]
        call    low_phys_to_16
        call    low_use_color
        mov     bx, LOW_EGC_CAT_BASE
        mov     bp, 48
.cat_bg:
        xor     ax, ax
        mov     cx, 47
        call    low_egc_cache_span_same
        add     bx, LOW_EGC_CAT_ROW_BYTES
        dec     bp
        jnz     .cat_bg

        ; CAT frame, then image overlay, matching low_draw_cat48 ordering.
        mov     al, [n4_cat48_box_edge]
        call    low_phys_to_16
        call    low_use_color
        mov     bx, LOW_EGC_CAT_BASE
        xor     ax, ax
        mov     cx, 47
        call    low_egc_cache_span_same
        mov     bx, LOW_EGC_CAT_BASE + (47*LOW_EGC_CAT_ROW_BYTES)
        call    low_egc_cache_span_same
        mov     bx, LOW_EGC_CAT_BASE
        mov     bp, 48
.cat_sides:
        xor     ax, ax
        xor     cx, cx
        call    low_egc_cache_span_same
        mov     ax, 47
        mov     cx, 47
        call    low_egc_cache_span_same
        add     bx, LOW_EGC_CAT_ROW_BYTES
        dec     bp
        jnz     .cat_sides

        mov     si, n4_cat48_row_counts
        mov     di, n4_cat48_row_spans
        mov     word [low_egc_cache_row_base], LOW_EGC_CAT_BASE
        mov     bp, 48
.cat_row:
        mov     al, [si]
        inc     si
        mov     [low_egc_cache_count], al
.cat_span:
        cmp     byte [low_egc_cache_count], 0
        je      .cat_next_row
        xor     ax, ax
        mov     al, [di]
        mov     [low_tmp_x], ax
        xor     ax, ax
        mov     al, [di+1]
        mov     [low_tmp_x2], ax
        xor     bx, bx
        mov     bl, [di+2]
        add     di, 3
        mov     al, [n4_cat48_palette + bx]
        call    low_phys_to_16
        call    low_use_color
        mov     bx, [low_egc_cache_row_base]
        mov     ax, [low_tmp_x]
        mov     cx, [low_tmp_x2]
        call    low_egc_cache_span_same
        dec     byte [low_egc_cache_count]
        jmp     .cat_span
.cat_next_row:
        add     word [low_egc_cache_row_base], LOW_EGC_CAT_ROW_BYTES
        dec     bp
        jnz     .cat_row

        ; 96x40 raster source.  Warp is not baked in; each complete cached row
        ; is shifted horizontally by EGC at display time.
        mov     word [low_egc_cache_row_base], LOW_EGC_RASTER_BASE
        xor     bp, bp
.raster_row:
        cmp     bp, 40
        jae     .raster_done
        mov     al, 107
        test    bp, 4
        jz      .rbase
        mov     al, 116
.rbase:
        call    low_use_phys_color
        mov     bx, [low_egc_cache_row_base]
        xor     ax, ax
        mov     cx, 95
        call    low_egc_cache_span_same

        cmp     bp, 2
        jb      .rrail
        cmp     bp, 38
        jae     .rrail
        mov     al, 150
        call    low_use_phys_color
        mov     bx, [low_egc_cache_row_base]
        mov     ax, 7
        mov     cx, 10
        call    low_egc_cache_span_same
        mov     ax, 27
        mov     cx, 30
        call    low_egc_cache_span_same
        mov     ax, 47
        mov     cx, 50
        call    low_egc_cache_span_same
        mov     ax, 67
        mov     cx, 70
        call    low_egc_cache_span_same
        mov     ax, 85
        mov     cx, 88
        call    low_egc_cache_span_same
        jmp     .rtaper
.rrail:
        mov     al, 127
        call    low_use_phys_color
        mov     bx, [low_egc_cache_row_base]
        xor     ax, ax
        mov     cx, 95
        call    low_egc_cache_span_same
.rtaper:
        cmp     bp, 6
        jb      .rsync
        cmp     bp, 34
        jae     .rsync
        mov     ax, bp
        sub     ax, 20
        jns     .rabs
        neg     ax
.rabs:
        shr     ax, 1
        mov     dx, 30
        sub     dx, ax
        jbe     .rsync
        mov     al, 180
        test    bp, 4
        jz      .rtcol
        mov     al, 188
.rtcol:
        call    low_use_phys_color
        mov     bx, [low_egc_cache_row_base]
        mov     ax, 48
        sub     ax, dx
        mov     cx, 48
        add     cx, dx
        dec     cx
        call    low_egc_cache_span_same
.rsync:
        cmp     bp, 10
        je      .rsync_row
        cmp     bp, 20
        je      .rsync_row
        cmp     bp, 29
        jne     .rnext
.rsync_row:
        mov     al, 127
        call    low_use_phys_color
        mov     bx, [low_egc_cache_row_base]
        mov     ax, 4
        mov     cx, 91
        call    low_egc_cache_span_same
.rnext:
        add     word [low_egc_cache_row_base], LOW_EGC_RASTER_ROW_BYTES
        inc     bp
        jmp     .raster_row

.raster_done:
%ifdef NEON4_286
        cmp     byte [video_400_mode], 0
        jne     .restore
        call    low_egc_build_cat200_variants
%endif
.restore:
        mov     al, [low_egc_saved_draw_color]
        mov     [draw_color], al
        mov     byte [grcg_color_cache], 0FFh
        call    grcg16_prepare_color
        pop     es
        popa
        ret

; 286 /200 exact CAT caches.  The 0.32 tables already encode the final physical
; result after background fill, four frame lines, and image-overlay ordering.
; Author both destination-Y parities once so the hot path only needs 24/25 EGC
; row copies per card.
%ifdef NEON4_286
low_egc_build_cat200_variants:
        pusha
        cmp     byte [video_400_mode], 0
        jne     .done

        mov     al, [n4_cat48_box_fill]
        call    low_phys_to_16
        call    low_use_color
        mov     bx, LOW_EGC_CAT200_EVEN_BASE
        mov     bp, LOW_EGC_CAT200_EVEN_ROWS
.even_bg:
        xor     ax, ax
        mov     cx, 47
        call    low_egc_cache_span_same
        add     bx, LOW_EGC_CAT_ROW_BYTES
        dec     bp
        jnz     .even_bg
        mov     bx, LOW_EGC_CAT200_ODD_BASE
        mov     bp, LOW_EGC_CAT200_ODD_ROWS
.odd_bg:
        xor     ax, ax
        mov     cx, 47
        call    low_egc_cache_span_same
        add     bx, LOW_EGC_CAT_ROW_BYTES
        dec     bp
        jnz     .odd_bg

        mov     si, low_cat200_even_counts
        mov     di, low_cat200_even_spans
        mov     bx, LOW_EGC_CAT200_EVEN_BASE
        mov     bp, LOW_EGC_CAT200_EVEN_ROWS
        call    low_egc_build_cat200_table
        mov     si, low_cat200_odd_counts
        mov     di, low_cat200_odd_spans
        mov     bx, LOW_EGC_CAT200_ODD_BASE
        mov     bp, LOW_EGC_CAT200_ODD_ROWS
        call    low_egc_build_cat200_table
.done:
        popa
        ret

; SI=count table, DI=span pairs, BX=cache base, BP=physical row count.
low_egc_build_cat200_table:
        pusha
        mov     [low_egc_cache_row_base], bx
.row:
        or      bp, bp
        jz      .done
        mov     byte [low_cat_level], 0
.symbol:
        xor     ax, ax
        mov     al, [si]
        inc     si
        mov     [low_egc_cache_count], al
        or      al, al
        jz      .next_symbol

        mov     bl, [low_cat_level]
        cmp     bl, 4
        je      .skip_background
        cmp     bl, 5
        je      .edge
        xor     bh, bh
        mov     al, [n4_cat48_palette + bx]
        jmp     .map
.edge:
        mov     al, [n4_cat48_box_edge]
.map:
        call    low_phys_to_16
        call    low_use_color
.draw_span:
        cmp     byte [low_egc_cache_count], 0
        je      .next_symbol
        xor     ax, ax
        mov     al, [di]
        xor     cx, cx
        mov     cl, [di+1]
        add     di, 2
        mov     bx, [low_egc_cache_row_base]
        call    low_egc_cache_span_same
        dec     byte [low_egc_cache_count]
        jmp     .draw_span

.skip_background:
        xor     ax, ax
        mov     al, [low_egc_cache_count]
        shl     ax, 1
        add     di, ax
        mov     byte [low_egc_cache_count], 0
.next_symbol:
        inc     byte [low_cat_level]
        cmp     byte [low_cat_level], 6
        jb      .symbol
        add     word [low_egc_cache_row_base], LOW_EGC_CAT_ROW_BYTES
        dec     bp
        jmp     .row
.done:
        popa
        ret
%endif

; AX=x, BX=logical y.  400-line modes copy the canonical 48 logical rows.
; NEON4286 /200 selects a precomposed even/odd 24/25-row physical cache so the
; original fill -> frame -> image order is preserved exactly.
;
; 0.34: every destination row is a completely independent EGC transfer.  Both
; LENGTH and SHIFT are rewritten before the row, which resets remain/stack and
; prevents the retained tail of one row from leaking into the next.  DS:SI and
; ES:DI both point at the current VRAM page, so REP MOVSW gives the intended
; bus sequence: source VRAM read -> EGC shifter -> destination VRAM write.
; An unaligned 48-pixel row has one retained tail word.  EGC VRAM-source mode
; advances the shifter on a source read, so one black guard read prepares that
; tail before the final STOSW consumes it.
low_egc_blit_tile48:
        pusha
        push    ds
        push    es
        cld
        mov     [cs:low_card_x], ax
        mov     [cs:low_egc_dest_y], bx
        mov     dx, ax
        and     dx, 15
        shl     dx, 4                     ; EGC dstbit is SHIFT bits 4..7
        mov     [cs:low_egc_shift_word], dx
        mov     ax, VRAM_B_SEG
        mov     ds, ax
        mov     es, ax
        mov     byte [cs:low_egc_cat_physical], 0
%ifdef NEON4_286
        cmp     byte [cs:video_400_mode], 0
        jne     .logical_cache
        mov     byte [cs:low_egc_cat_physical], 1
        mov     ax, [cs:low_egc_dest_y]
        test    al, 1
        jnz     .odd_cache
        mov     si, LOW_EGC_CAT200_EVEN_BASE
        mov     bp, LOW_EGC_CAT200_EVEN_ROWS
        jmp     .physical_cache_ready
.odd_cache:
        mov     si, LOW_EGC_CAT200_ODD_BASE
        mov     bp, LOW_EGC_CAT200_ODD_ROWS
.physical_cache_ready:
        sar     ax, 1
        mov     [cs:low_egc_dest_y], ax
        jmp     .row
.logical_cache:
%endif
        mov     si, LOW_EGC_CAT_BASE
        mov     bp, 48
.row:
        ; LENGTH/SHIFT are deliberately both written per row.  This is slightly
        ; more I/O than 0.33 but still tiny compared with rebuilding hundreds of
        ; GRCG spans, and it guarantees a fresh EGC shifter state.
        mov     dx, EGC_LENGTH_PORT
        mov     ax, 47
        out     dx, ax
        mov     dx, EGC_SHIFT_PORT
        mov     ax, [cs:low_egc_shift_word]
        out     dx, ax
        mov     bx, [cs:low_egc_dest_y]
%ifdef NEON4_286
        cmp     byte [cs:low_egc_cat_physical], 0
        jne     .physical_y
        cmp     byte [cs:video_400_mode], 0
        jne     .physical_y
        sar     bx, 1
.physical_y:
%endif
        mov     di, bx
        shl     di, 6
        mov     dx, bx
        shl     dx, 4
        add     di, dx
        mov     ax, [cs:low_card_x]
        mov     dx, ax
        shr     dx, 4
        shl     dx, 1
        add     di, dx
        mov     cx, 3
        rep     movsw                      ; 48 source pixels / three EGC words
        test    word [cs:low_egc_shift_word], 0F0h
        jz      .no_tail
        mov     ax, [ds:LOW_EGC_GUARD_OFFSET] ; source read prepares retained tail
        stosw                              ; CPU value ignored: source is VRAM
.no_tail:
        inc     word [cs:low_egc_dest_y]
        dec     bp
        jnz     .row
        pop     es
        pop     ds
        popa
        ret

; AX=nominal X, BX=logical Y.  Copy all 40 cached rows with the original
; per-row sine shift.  /200 deliberately issues both logical rows that map to
; one physical line, preserving the old union/overwrite result at row edges.
; As with CAT, 0.34 restarts LENGTH+SHIFT for every row and uses REP MOVSW;
; unaligned rows use one black guard source read plus a final tail write.
low_egc_raster_blit96x40:
        pusha
        push    ds
        push    es
        cld
        mov     [cs:low_card_x], ax
        mov     [cs:low_card_y], bx
        mov     ax, [cs:city_global_frame]
        shl     ax, 1
        add     al, [cs:n4_raster_phase_bias]
        mov     [cs:low_raster_phase], al
        mov     ax, VRAM_B_SEG
        mov     ds, ax
        mov     es, ax
        mov     si, LOW_EGC_RASTER_BASE
        xor     bp, bp
.row:
        cmp     bp, 40
        jae     .done
        xor     bx, bx
        mov     bl, [cs:low_raster_phase]
        shl     bx, 1
        mov     ax, [cs:sin_table + bx]
        sar     ax, N4_RASTER_WAVE_SHIFT
        add     ax, [cs:low_card_x]
        mov     [cs:low_raster_x], ax
        mov     dx, ax
        and     dx, 15
        shl     dx, 4
        mov     [cs:low_egc_shift_word], dx

        mov     dx, EGC_LENGTH_PORT
        mov     ax, 95
        out     dx, ax
        mov     dx, EGC_SHIFT_PORT
        mov     ax, [cs:low_egc_shift_word]
        out     dx, ax

        mov     bx, [cs:low_card_y]
        add     bx, bp
%ifdef NEON4_286
        cmp     byte [cs:video_400_mode], 0
        jne     .physical_y
        sar     bx, 1
.physical_y:
%endif
        mov     di, bx
        shl     di, 6
        mov     dx, bx
        shl     dx, 4
        add     di, dx
        mov     ax, [cs:low_raster_x]
        mov     dx, ax
        shr     dx, 4
        shl     dx, 1
        add     di, dx
        mov     cx, 6
        rep     movsw                      ; 96 source pixels / six EGC words
        test    word [cs:low_egc_shift_word], 0F0h
        jz      .no_tail
        mov     ax, [ds:LOW_EGC_GUARD_OFFSET] ; prepare final shifted pixels
        stosw
.no_tail:
        add     byte [cs:low_raster_phase], N4_RASTER_PHASE_STEP
        inc     bp
        jmp     .row
.done:
        pop     es
        pop     ds
        popa
        ret

; AX=nominal X, BX=Y.  Draw the 96x40 procedural source with the same row warp.
n4_raster_blit96x40:
        cmp     byte [low_egc_batch_active], 0
        jne     low_egc_raster_blit96x40
        pusha
        mov     [low_card_x], ax
        mov     [low_card_y], bx
        mov     ax, [city_global_frame]
        shl     ax, 1
        add     al, [n4_raster_phase_bias]
        mov     [low_raster_phase], al
        xor     bp, bp
.row:
        cmp     bp, 40
        jae     .done
        xor     bx, bx
        mov     bl, [low_raster_phase]
        shl     bx, 1
        mov     ax, [sin_table+bx]
        sar     ax, N4_RASTER_WAVE_SHIFT
        add     ax, [low_card_x]
        mov     [low_raster_x], ax
        mov     bx, [low_card_y]
        add     bx, bp

        ; alternating base field, as in n4_raster_build_asset
        mov     al, 107
        test    bp, 4
        jz      .base_col
        mov     al, 116
.base_col:
        call    low_use_phys_color
        mov     ax, [low_raster_x]
        mov     cx, ax
        add     cx, 95
        call    hline_set_same_colour

        ; top/bottom rails
        cmp     bp, 2
        jb      .rail
        cmp     bp, 38
        jae     .rail
        jmp     .markers
.rail:
        mov     al, 127
        call    low_use_phys_color
        mov     ax, [low_raster_x]
        mov     cx, ax
        add     cx, 95
        call    hline_set_same_colour
        jmp     .taper
.markers:
        mov     al, 150
        call    low_use_phys_color
        mov     ax, [low_raster_x]
        add     ax, 7
        mov     cx, ax
        add     cx, 3
        call    hline_set_same_colour
        mov     ax, [low_raster_x]
        add     ax, 27
        mov     cx, ax
        add     cx, 3
        call    hline_set_same_colour
        mov     ax, [low_raster_x]
        add     ax, 47
        mov     cx, ax
        add     cx, 3
        call    hline_set_same_colour
        mov     ax, [low_raster_x]
        add     ax, 67
        mov     cx, ax
        add     cx, 3
        call    hline_set_same_colour
        mov     ax, [low_raster_x]
        add     ax, 85
        mov     cx, ax
        add     cx, 3
        call    hline_set_same_colour
.taper:
        cmp     bp, 6
        jb      .sync
        cmp     bp, 34
        jae     .sync
        mov     ax, bp
        sub     ax, 20
        jns     .absok
        neg     ax
.absok:
        shr     ax, 1
        mov     dx, 30
        sub     dx, ax
        jbe     .sync
        mov     al, 180
        test    bp, 4
        jz      .tcol
        mov     al, 188
.tcol:
        call    low_use_phys_color
        mov     ax, [low_raster_x]
        add     ax, 48
        sub     ax, dx
        mov     cx, [low_raster_x]
        add     cx, 48
        add     cx, dx
        dec     cx
        call    hline_set_same_colour
.sync:
        cmp     bp, 10
        je      .syncrow
        cmp     bp, 20
        je      .syncrow
        cmp     bp, 29
        jne     .next
.syncrow:
        mov     al, 127
        call    low_use_phys_color
        mov     ax, [low_raster_x]
        add     ax, 4
        mov     cx, [low_raster_x]
        add     cx, 91
        call    hline_set_same_colour
.next:
        add     byte [low_raster_phase], N4_RASTER_PHASE_STEP
        inc     bp
        jmp     .row
.done:
        popa
        ret

; AX=x BX=y
n4_blit_tile48:
        cmp     byte [low_egc_batch_active], 0
        jne     low_egc_blit_tile48
        jmp     low_draw_cat48

; User CAT card is pixel/span-faithful; only palette indices are quantised.
; 0.32 RASTER TRANSFER hot path:
;   * background/frame are still emitted exactly as before;
;   * authored image spans remain in original logical-row order (important for
;     286 400->200 mapping);
;   * inside each row, non-overlapping spans are grouped by source luminance so
;     GRCG tile colour is prepared at most four times instead of once per span;
;   * in 400-line modes, background-equivalent inner-row writes are skipped;
;   * 286/200 uses a pre-composed physical-row ownership table instead.
low_draw_cat48:
        pusha
        mov     [low_card_x], ax
        mov     [low_card_y], bx

        ; Background colour is needed both for the rectangle and for the
        ; redundant-span test below.  Compare after 256->16 quantisation, not
        ; before it: e.g. physical 84 and 95 currently both become colour 5.
        mov     al, [n4_cat48_box_fill]
        call    low_phys_to_16
        mov     [low_cat_bg_color], al
        call    low_use_color
        mov     ax, [low_card_x]
        mov     cx, ax
        add     cx, 47
        mov     bx, [low_card_y]
        mov     si, bx
        add     si, 47
        call    fill_rect

%ifdef NEON4_286
        ; 200-line mode has a dedicated physical-row composite.  It folds the
        ; old frame + two-logical-rows-per-physical-row overwrite sequence into
        ; one final set of spans, preserving the exact result while avoiding
        ; hundreds of intermediate hline writes.
        cmp     byte [video_400_mode], 0
        jne     .draw_400_overlay
        call    low_draw_cat48_200_overlay
        popa
        ret
.draw_400_overlay:
%endif

        mov     al, [n4_cat48_box_edge]
        call    low_use_phys_color
        mov     ax, [low_card_x]
        mov     bx, [low_card_y]
        mov     cx, ax
        add     cx, 47
        mov     dx, bx
        call    line_set_same_colour
        mov     dx, [low_card_y]
        add     dx, 47
        call    line_set_same_colour
        mov     ax, [low_card_x]
        mov     bx, [low_card_y]
        mov     cx, ax
        mov     dx, bx
        add     dx, 47
        call    line_set_same_colour
        mov     ax, [low_card_x]
        add     ax, 47
        mov     cx, ax
        mov     bx, [low_card_y]
        mov     dx, bx
        add     dx, 47
        call    line_set_same_colour

        mov     si, low_cat48_group_counts
        mov     di, low_cat48_group_spans
        xor     bp, bp
.row:
        cmp     bp, 48
        jae     .done

        ; Keep the physical/logical row ordering identical to the old RLE walk.
        mov     ax, [low_card_y]
        add     ax, bp
        mov     [low_tmp_y2], ax
        mov     byte [low_cat_level], 0
.level:
        xor     dx, dx
        mov     dl, [si]
        inc     si
        or      dl, dl
        jz      .next_level

        xor     bx, bx
        mov     bl, [low_cat_level]
        mov     al, [n4_cat48_palette + bx]
        call    low_phys_to_16
        ; In 400-line modes rows are independent.  Rows 0 and 47 are covered by
        ; the horizontal frame before the image overlay, so background-colour
        ; source spans there still have to overwrite that frame.
        cmp     bp, 0
        je      .draw_group
        cmp     bp, 47
        je      .draw_group
        cmp     al, [low_cat_bg_color]
        je      .skip_group
.draw_group:
        call    low_use_color

.span:
        xor     ax, ax
        mov     al, [di]
        inc     di
        add     ax, [low_card_x]
        mov     [low_tmp_x], ax
        xor     ax, ax
        mov     al, [di]
        inc     di
        add     ax, [low_card_x]
        mov     cx, ax
        mov     ax, [low_tmp_x]
        mov     bx, [low_tmp_y2]
        call    hline_set_same_colour
        dec     dl
        jnz     .span
        jmp     .next_level

.skip_group:
        ; The final card background is already this exact analog-16 colour.
        ; Advance over x0/x1 pairs without touching VRAM.
        xor     ax, ax
        mov     al, dl
        shl     ax, 1
        add     di, ax

.next_level:
        inc     byte [low_cat_level]
        cmp     byte [low_cat_level], 4
        jb      .level
        inc     bp
        jmp     .row
.done:
        popa
        ret

; 286 200-line CAT overlay after the background rectangle has already been
; filled.  AX/BX inputs are kept in low_card_x/low_card_y by low_draw_cat48.
; Final symbol maps include the frame and image overwrite order.
%ifdef NEON4_286
low_draw_cat48_200_overlay:
        pusha

        mov     ax, [low_card_y]
        test    al, 1
        jnz     .odd
        mov     si, low_cat200_even_counts
        mov     di, low_cat200_even_spans
        mov     bp, 24
        jmp     .table_ready
.odd:
        mov     si, low_cat200_odd_counts
        mov     di, low_cat200_odd_spans
        mov     bp, 25
.table_ready:
        ; hline_set16_same performs the final /2 conversion.  Use the even
        ; logical coordinate corresponding to floor(card_y/2) as row zero.
        mov     ax, [low_card_y]
        and     ax, 0FFFEh
        mov     [low_tmp_y2], ax
.row:
        or      bp, bp
        jz      .done
        mov     byte [low_cat_level], 0   ; symbol 0..5
.symbol:
        xor     dx, dx
        mov     dl, [si]
        inc     si
        or      dl, dl
        jz      .next_symbol

        xor     bx, bx
        mov     bl, [low_cat_level]
        cmp     bl, 4
        je      .skip_symbol              ; background already filled
        cmp     bl, 5
        je      .edge_colour
        mov     al, [n4_cat48_palette + bx]
        jmp     .map_colour
.edge_colour:
        mov     al, [n4_cat48_box_edge]
.map_colour:
        call    low_phys_to_16
        cmp     al, [low_cat_bg_color]
        je      .skip_symbol
        call    low_use_color

.span:
        xor     ax, ax
        mov     al, [di]
        inc     di
        add     ax, [low_card_x]
        mov     [low_tmp_x], ax
        xor     ax, ax
        mov     al, [di]
        inc     di
        add     ax, [low_card_x]
        mov     cx, ax                    ; x1
        mov     ax, [low_tmp_x]           ; x0
        mov     bx, [low_tmp_y2]
        call    hline_set_same_colour
        dec     dl
        jnz     .span
        jmp     .next_symbol

.skip_symbol:
        xor     ax, ax
        mov     al, dl
        shl     ax, 1
        add     di, ax

.next_symbol:
        inc     byte [low_cat_level]
        cmp     byte [low_cat_level], 6
        jb      .symbol
        add     word [low_tmp_y2], 2
        dec     bp
        jmp     .row
.done:
        popa
        ret
%endif

; Shared visual-story raster motif.
; AX=nominal X, BX=Y, DL=phase bias.  The 96x40 source is generated once in
; page-tail VRAM and thereafter scanline-shift copied.  Keeping this recurring
; signal in the polygon chapters ties the eight technical studies together.
; ---------------------------------------------------------------------------
n4_story_raster_panel:
        pusha
        mov     [n4_story_raster_x], ax
        mov     [n4_story_raster_y], bx
        mov     [n4_raster_phase_bias], dl
        call    n4_blit_ensure_asset
        xor     di, di
        cmp     byte [low_egc_batch_active], 0
        jne     .batch_ready
        mov     di, 1                       ; this helper owns the batch
        call    n4_blit_batch_begin
.batch_ready:
        mov     ax, [n4_story_raster_x]
        sub     ax, 32
        mov     cx, [n4_story_raster_x]
        add     cx, 127                    ; conservative +/-32px warp envelope
        mov     bx, [n4_story_raster_y]
        mov     dx, bx
        add     dx, 39
        cmp     byte [low_dirty_span_enable], 0
        je      .story_dirty_done
        call    low_dirty_span_record_rect
.story_dirty_done:
        mov     ax, [n4_story_raster_x]
        mov     bx, [n4_story_raster_y]
        call    n4_raster_blit96x40
        or      di, di
        jz      .done
        call    n4_blit_batch_end
.done:
        popa
        ret

; ---------------------------------------------------------------------------
; Scene 0 - SIGNAL SEED.
; A compact raster signal appears first, then six coloured facets orbit around
; a tetrahedral seed.  Everything remains comfortably inside the viewport: the
; opening is energetic without immediately filling the user's entire field of
; view.  This signal becomes the connective visual motif of later chapters.
; ---------------------------------------------------------------------------
scene4_solid_primitives:
        pusha
        mov     ax, 90
        mov     bx, 15
        mov     cx, 550
        mov     dx, 385
        ; 0.50: low-colour n4_geo_set_dirty_rect is a no-op; CALL removed

        mov     ax, [scene_frame]
        shl     ax, 1
        mov     [n4_motion_phase], ax

        ; Raster seed behind the geometry, gently drifting around the centre.
        mov     ax, [n4_motion_phase]
        add     al, 29
        N4_SIN_INLINE
        sar     ax, 9                       ; +/-32
        add     ax, 240
        mov     [n4_tmp_x], ax
        mov     ax, [n4_motion_phase]
        add     al, 101
        N4_COS_INLINE
        sar     ax, 10                      ; +/-16
        add     ax, 164
        mov     bx, ax
        mov     ax, [n4_tmp_x]
        mov     dl, 0
        call    n4_story_raster_panel


        ; Keep the seed restrained: no large petals/fins, just the central body.
        mov     ax, [n4_motion_phase]
        N4_SIN_INLINE
        sar     ax, 10                      ; +/-16
        add     ax, 320
        mov     [n4_fan_cx], ax
        mov     ax, [n4_motion_phase]
        add     al, 53
        N4_COS_INLINE
        sar     ax, 11                      ; +/-8
        add     ax, 200
        mov     [n4_fan_cy], ax

.centre:
        mov     ax, [n4_fan_cx]
        mov     [n4_geo_center_x], ax
        mov     ax, [n4_fan_cy]
        mov     [n4_geo_center_y], ax
        mov     word [n4_geo_object_depth], 1000
        mov     ax, [n4_motion_phase]
        mov     ah, al
        add     ah, 47
        call    n4_geo_set_angles
        mov     byte [n4_geo_colour_base], 224
        mov     byte [n4_geo_fill_mode], 0
        call    n4_geo_draw_tetra
        popa
        ret

; AL=angle. Uses n4_fan_cx/cy/radius -> n4_tmp_x/y.
n4_geo_polar_point:
        pusha
        mov     [n4_tmp_angle], al
        ; 0.43 direct lookup avoids two nested trig calls for every corona point.
        xor     bx, bx
        mov     bl, [n4_tmp_angle]
        add     bl, 64
        shl     bx, 1
        mov     ax, [sin_table + bx]
        mov     bx, [n4_fan_radius]
        LOW_Q14_MUL_HOT
        add     ax, [n4_fan_cx]
        mov     [n4_tmp_x], ax
        xor     bx, bx
        mov     bl, [n4_tmp_angle]
        shl     bx, 1
        mov     ax, [sin_table + bx]
        mov     bx, [n4_fan_radius]
        LOW_Q14_MUL_HOT
        mov     bx, [n4_fan_cy]
        sub     bx, ax
        mov     [n4_tmp_y], bx
        popa
        ret

; ---------------------------------------------------------------------------
; Scene 1 - FACET AWAKENING.
; The seed evolves through cube/tetra/octa/icosa forms while the same raster
; carrier follows below it.  Motion remains >=2x the early prototype, but the
; object centre and zoom are bounded so its silhouette stays readable.
; ---------------------------------------------------------------------------
scene4_facet_rotation:
        pusha
        mov     ax, 60
        mov     bx, 5
        mov     cx, 580
        mov     dx, 395
        ; 0.50: low-colour n4_geo_set_dirty_rect is a no-op; CALL removed
        mov     ax, [scene_frame]
        shl     ax, 1
        mov     [n4_motion_phase], ax

        N4_SIN_INLINE
        sar     ax, 8                       ; +/-64
        add     ax, 320
        mov     [n4_geo_center_x], ax
        mov     ax, [n4_motion_phase]
        add     al, 67
        N4_COS_INLINE
        sar     ax, 11                      ; +/-8
        add     ax, 200
        mov     [n4_geo_center_y], ax
        mov     ax, [n4_motion_phase]
        add     al, 111
        N4_SIN_INLINE
        test    ax, ax
        jns     .zoom_abs
        neg     ax
.zoom_abs:
        sar     ax, 8                       ; 0..64
        mov     bx, 1000
        sub     bx, ax                      ; 936..1000; cube/tetra remain on-screen
        mov     [n4_geo_object_depth], bx

        ; Carrier follows the object instead of becoming a separate spectacle.
        mov     ax, [n4_geo_center_x]
        sub     ax, 48
        mov     bx, [n4_geo_center_y]
        add     bx, 72
        mov     dl, 37
        call    n4_story_raster_panel

        ; One coherent transform is shared by the solid and its 18%-larger
        ; wire cage.  Far cage edges are drawn first; near cage edges are drawn
        ; after the solid, giving a true "solid inside a wireframe" impression.
        mov     ax, [n4_motion_phase]
        mov     ah, al
        shr     ah, 1
        add     ah, 29
        call    n4_geo_set_angles
        ; 0.46: keep the construction cage throughout FACET ASSEMBLY.
        ; It remains ordered as far wire -> solid -> near wire.
        mov     byte [draw_color], 20        ; dim far half
        xor     ax, ax                       ; pass 0 = far edges
        call    n4_geo_draw_cube_cage_pass
        mov     byte [n4_geo_fill_mode], 0
        mov     ax, [scene_frame]
        shr     ax, 7
        and     al, 3
        cmp     al, 0
        jne     .not_cube
        mov     byte [n4_geo_colour_base], 32
        call    n4_geo_draw_cube
        jmp     .solid_done
.not_cube:
        cmp     al, 1
        jne     .not_tetra
        mov     byte [n4_geo_colour_base], 96
        call    n4_geo_draw_tetra
        jmp     .solid_done
.not_tetra:
        cmp     al, 2
        jne     .icosa
        mov     byte [n4_geo_colour_base], 160
        call    n4_geo_draw_octa
        jmp     .solid_done
.icosa:
        mov     byte [n4_geo_colour_base], 192
        call    n4_geo_draw_icosa
.solid_done:
        mov     byte [draw_color], 29        ; brighter near half
        mov     ax, 1                        ; pass 1 = near edges
        call    n4_geo_draw_cube_cage_pass
        popa
        ret

; ---------------------------------------------------------------------------
; Scene 2 - MATERIAL ASSEMBLY.
; Three solids gather around the signal.  The centre cube uses polygon raster
; shear (fill mode 4), proving that raster-scan techniques are not isolated to
; the BitBlt chapter.  Satellites stay smaller and frame the subject rather than
; competing with it.
; ---------------------------------------------------------------------------
scene4_pattern_cube:
        pusha
        mov     ax, 55
        mov     bx, 25
        mov     cx, 605
        mov     dx, 390
        ; 0.50: low-colour n4_geo_set_dirty_rect is a no-op; CALL removed
        mov     ax, [scene_frame]
        shl     ax, 1
        mov     [n4_motion_phase], ax

        ; Two pieces of the carrier converge behind the assembly.
        ; Keep the VRAM-copy engine active across both adjacent panels.
        call    n4_blit_ensure_asset
        call    n4_blit_batch_begin
        mov     ax, [n4_motion_phase]
        add     al, 33
        N4_SIN_INLINE
        sar     ax, 10
        add     ax, 170
        mov     bx, 132
        mov     dl, 53
        call    n4_story_raster_panel
        mov     ax, [n4_motion_phase]
        add     al, 155
        N4_COS_INLINE
        sar     ax, 10
        add     ax, 360
        mov     bx, 228
        mov     dl, 149
        call    n4_story_raster_panel
        call    n4_blit_batch_end

        ; Centre cube: controlled motion, rich 32-shade ramp, scanline shear.
        mov     ax, [n4_motion_phase]
        N4_SIN_INLINE
        sar     ax, 10
        add     ax, 320
        mov     [n4_geo_center_x], ax
        mov     ax, [n4_motion_phase]
        add     al, 59
        N4_COS_INLINE
        sar     ax, 11
        add     ax, 200
        mov     [n4_geo_center_y], ax
        mov     word [n4_geo_object_depth], 1100
        mov     ax, [n4_motion_phase]
        mov     ah, al
        add     ah, 33
        call    n4_geo_set_angles
        mov     byte [n4_geo_colour_base], 64
        mov     byte [n4_geo_fill_mode], 4
        mov     byte [n4_poly_raster_phase_bias], 0
        call    n4_geo_draw_cube

        ; Left octahedron.
        mov     ax, [n4_motion_phase]
        add     al, 83
        N4_SIN_INLINE
        sar     ax, 11
        add     ax, 158
        mov     [n4_geo_center_x], ax
        mov     ax, [n4_motion_phase]
        add     al, 13
        N4_COS_INLINE
        sar     ax, 12
        add     ax, 118
        mov     [n4_geo_center_y], ax
        mov     word [n4_geo_object_depth], 980
        mov     ax, [n4_motion_phase]
        add     al, 41
        mov     ah, al
        add     ah, 71
        call    n4_geo_set_angles
        mov     byte [n4_geo_colour_base], 128
        mov     byte [n4_geo_fill_mode], 1
        call    n4_geo_draw_octa

        ; Right pyramid.
        mov     ax, [n4_motion_phase]
        add     al, 173
        N4_SIN_INLINE
        sar     ax, 11
        add     ax, 470
        mov     [n4_geo_center_x], ax
        mov     ax, [n4_motion_phase]
        add     al, 103
        N4_COS_INLINE
        sar     ax, 12
        add     ax, 265
        mov     [n4_geo_center_y], ax
        mov     word [n4_geo_object_depth], 1200
        mov     ax, [n4_motion_phase]
        add     al, 97
        mov     ah, al
        add     ah, 37
        call    n4_geo_set_angles
        mov     byte [n4_geo_colour_base], 192
        mov     byte [n4_geo_fill_mode], 2
        call    n4_geo_draw_pyramid
        popa
        ret

; ---------------------------------------------------------------------------
; Scene 3 - MORPH GATE.
; The assembled cube is scanned by the carrier and morphs into a convex tapered
; solid.  During the middle half of the chapter the polygon spans themselves
; undergo a restrained +/-16px raster shear.  Quad-level culling remains in
; charge, so the hidden-face fix from 0.6 is preserved.
; ---------------------------------------------------------------------------
scene4_morphing_solid:
        pusha
        mov     ax, 90
        mov     bx, 15
        mov     cx, 550
        mov     dx, 385
        ; 0.50: low-colour n4_geo_set_dirty_rect is a no-op; CALL removed
        mov     ax, [scene_frame]
        shl     ax, 1
        mov     [n4_motion_phase], ax

        mov     ax, [n4_motion_phase]
        add     al, 77
        N4_SIN_INLINE
        sar     ax, 9
        add     ax, 272
        mov     [n4_tmp_x], ax
        mov     ax, [n4_motion_phase]
        add     al, 19
        N4_COS_INLINE
        sar     ax, 11
        add     ax, 178
        mov     bx, ax
        mov     ax, [n4_tmp_x]
        mov     dl, 91
        call    n4_story_raster_panel

        mov     ax, [n4_motion_phase]
        N4_SIN_INLINE
        test    ax, ax
        jns     .morph_pos
        neg     ax
.morph_pos:
        shr     ax, 6
        cmp     ax, 256
        jbe     .morph_t_ok
        mov     ax, 256
.morph_t_ok:
        mov     [n4_morph_t], ax

        ; 0.44: the morph target differs from the cube only at vertices 2,3,6,7.
        ; Those coordinates move by only +/-80 in X/Z and +40 in Y.  Build the
        ; complete eight-vertex work set from two positive offsets instead of
        ; performing 24 signed multiplies every frame.  Negative branches use
        ; the same positive magnitude; the maximum difference from arithmetic
        ; signed rounding is one sub-pixel source unit and is not visible after
        ; projection.
        mov     dx, ax                      ; t = 0..256
        shl     ax, 2
        add     ax, dx                      ; 5*t
        mov     dx, ax
        shr     ax, 4                       ; off80 = floor(80*t/256)
        mov     [n4_tmp_x], ax
        mov     ax, dx
        shr     ax, 5                       ; off40 = floor(40*t/256)
        mov     [n4_tmp_y], ax

        ; v0 / v1 unchanged
        mov     word [n4_geo_work_vertices+0],  -220
        mov     word [n4_geo_work_vertices+2],  -220
        mov     word [n4_geo_work_vertices+4],  -220
        mov     word [n4_geo_work_vertices+6],   220
        mov     word [n4_geo_work_vertices+8],  -220
        mov     word [n4_geo_work_vertices+10], -220

        ; v2 = (220-off80, 220+off40, -220+off80)
        mov     ax, 220
        sub     ax, [n4_tmp_x]
        mov     [n4_geo_work_vertices+12], ax
        mov     ax, 220
        add     ax, [n4_tmp_y]
        mov     [n4_geo_work_vertices+14], ax
        mov     ax, -220
        add     ax, [n4_tmp_x]
        mov     [n4_geo_work_vertices+16], ax

        ; v3 = (-220+off80, 220+off40, -220+off80)
        mov     ax, -220
        add     ax, [n4_tmp_x]
        mov     [n4_geo_work_vertices+18], ax
        mov     ax, 220
        add     ax, [n4_tmp_y]
        mov     [n4_geo_work_vertices+20], ax
        mov     ax, -220
        add     ax, [n4_tmp_x]
        mov     [n4_geo_work_vertices+22], ax

        ; v4 / v5 unchanged
        mov     word [n4_geo_work_vertices+24], -220
        mov     word [n4_geo_work_vertices+26], -220
        mov     word [n4_geo_work_vertices+28],  220
        mov     word [n4_geo_work_vertices+30],  220
        mov     word [n4_geo_work_vertices+32], -220
        mov     word [n4_geo_work_vertices+34],  220

        ; v6 = (220-off80, 220+off40, 220-off80)
        mov     ax, 220
        sub     ax, [n4_tmp_x]
        mov     [n4_geo_work_vertices+36], ax
        mov     ax, 220
        add     ax, [n4_tmp_y]
        mov     [n4_geo_work_vertices+38], ax
        mov     ax, 220
        sub     ax, [n4_tmp_x]
        mov     [n4_geo_work_vertices+40], ax

        ; v7 = (-220+off80, 220+off40, 220-off80)
        mov     ax, -220
        add     ax, [n4_tmp_x]
        mov     [n4_geo_work_vertices+42], ax
        mov     ax, 220
        add     ax, [n4_tmp_y]
        mov     [n4_geo_work_vertices+44], ax
        mov     ax, 220
        sub     ax, [n4_tmp_x]
        mov     [n4_geo_work_vertices+46], ax

        mov     ax, [n4_motion_phase]
        add     al, 37
        N4_SIN_INLINE
        sar     ax, 10
        add     ax, 320
        mov     [n4_geo_center_x], ax
        mov     ax, [n4_motion_phase]
        add     al, 121
        N4_COS_INLINE
        sar     ax, 11
        add     ax, 200
        mov     [n4_geo_center_y], ax
        mov     ax, [n4_motion_phase]
        add     al, 79
        N4_SIN_INLINE
        test    ax, ax
        jns     .depth_abs
        neg     ax
.depth_abs:
        sar     ax, 9                       ; modest 0..32 zoom component
        mov     bx, 1020
        sub     bx, ax                      ; 988..1020, safe for full morph radius
        mov     [n4_geo_object_depth], bx
        mov     ax, [n4_motion_phase]
        mov     ah, al
        add     ah, 51
        call    n4_geo_set_angles
        mov     byte [n4_geo_colour_base], 96
        mov     byte [n4_geo_fill_mode], 0
        cmp     word [scene_frame], N4_MORPH_RASTER_START
        jb      .morph_mode_ready
        cmp     word [scene_frame], N4_MORPH_RASTER_END
        jae     .morph_mode_ready
        mov     byte [n4_geo_fill_mode], 4
        mov     byte [n4_poly_raster_phase_bias], 64
.morph_mode_ready:
        mov     si, n4_geo_work_vertices
        mov     cx, 8
        call    n4_geo_transform_vertices
        mov     word [low_face_class_ptr], low_face_class_cube
        mov     si, n4_geo_cube_quads
        mov     cx, 6
        call    n4_geo_draw_quad_faces
        popa
        ret

; ---------------------------------------------------------------------------
; Scene 4 - RASTER TRANSFER.
; The solid has dissolved into VRAM-resident imagery.  Three restrained warped
; strips form a vertical progression and four cards move between them.  This is
; the dedicated PEGC shift/BitBlt chapter, but unlike 0.6 it leaves clear space
; around each element so the hardware effect is easy to read.
; ---------------------------------------------------------------------------
scene4_shift_blitter:
        pusha
        ; The seven object bounds are recorded as they are drawn.  On the next
        ; reuse of this physical page only those old bounds are erased.
        call    n4_blit_ensure_asset
        call    n4_blit_batch_begin
        mov     ax, [scene_frame]
        shl     ax, 1
        mov     [n4_motion_phase], ax

        mov     ax, [n4_motion_phase]
        N4_SIN_INLINE
        sar     ax, 9
        add     ax, 150
        mov     bx, 82
        mov     byte [n4_raster_phase_bias], 0
        push    ax
        push    bx
        mov     cx, ax
        sub     ax, 32
        add     cx, 127
        mov     dx, bx
        add     dx, 39
        call    low_raster_track_rect16
        pop     bx
        pop     ax
        call    n4_raster_blit96x40

        mov     ax, [n4_motion_phase]
        add     al, 85
        N4_SIN_INLINE
        sar     ax, 9
        add     ax, 272
        mov     bx, 176
        mov     byte [n4_raster_phase_bias], 85
        push    ax
        push    bx
        mov     cx, ax
        sub     ax, 32
        add     cx, 127
        mov     dx, bx
        add     dx, 39
        call    low_raster_track_rect16
        pop     bx
        pop     ax
        call    n4_raster_blit96x40

        mov     ax, [n4_motion_phase]
        add     al, 170
        N4_SIN_INLINE
        sar     ax, 9
        add     ax, 394
        mov     bx, 270
        mov     byte [n4_raster_phase_bias], 170
        push    ax
        push    bx
        mov     cx, ax
        sub     ax, 32
        add     cx, 127
        mov     dx, bx
        add     dx, 39
        call    low_raster_track_rect16
        pop     bx
        pop     ax
        call    n4_raster_blit96x40

        xor     bp, bp
.cards:
        cmp     bp, 4
        jae     .finish
        mov     si, bp

        ; Lissajous-like motion so the cards feel transferred between signal
        ; bands instead of merely vibrating in place.
        mov     ax, [n4_motion_phase]
        xor     dx, dx
        mov     dl, [n4_blit_phase_seed + si]
        add     ax, dx
        N4_SIN_INLINE
        sar     ax, 7                       ; +/-128
        add     ax, 272
        mov     [n4_tmp_x], ax
        mov     ax, [n4_motion_phase]
        add     ax, [n4_motion_phase]
        add     ax, [n4_motion_phase]       ; 3x phase for the minor axis
        xor     dx, dx
        mov     dl, [n4_blit_phase_seed + si]
        add     ax, dx
        add     al, 37
        N4_SIN_INLINE
        sar     ax, 10                      ; +/-16
        add     [n4_tmp_x], ax

        mov     ax, [n4_motion_phase]
        add     ax, [n4_motion_phase]       ; 2x phase
        xor     dx, dx
        mov     dl, [n4_blit_phase_seed + si]
        add     ax, dx
        add     al, 64
        N4_COS_INLINE
        sar     ax, 8                       ; +/-32
        add     ax, 170
        mov     [n4_tmp_y], ax
        mov     ax, [n4_motion_phase]
        mov     dx, [n4_motion_phase]
        shl     dx, 2                       ; 4x phase
        add     ax, dx                      ; 5x total phase
        xor     dx, dx
        mov     dl, [n4_blit_phase_seed + si]
        add     ax, dx
        add     al, 13
        N4_SIN_INLINE
        sar     ax, 10                      ; +/-16
        add     ax, [n4_tmp_y]
        mov     bx, ax
        mov     ax, [n4_tmp_x]
        push    ax
        push    bx
        mov     cx, ax
        add     cx, 47
        mov     dx, bx
        add     dx, 47
        call    low_raster_track_rect16
        pop     bx
        pop     ax
        call    n4_blit_tile48
        inc     bp
        jmp     .cards
.finish:
        call    n4_blit_batch_end
        popa
        ret

; ---------------------------------------------------------------------------
; BGM-driven scanline string.
; AL=FM channel 0..2, BX=base Y, DL=physical palette colour.
; The current score pitch sets spatial/temporal vibration frequency and the
; common sequencer's 12-frame excitation envelope sets amplitude.  0.12 boosts
; that visual amplitude about 3x.  0.41 samples the same fixed-end envelope at
; odd points and renders short horizontal markers instead of a connected line.
; ---------------------------------------------------------------------------
n4_draw_music_string:
        pusha
        mov     [low_string_channel], al
        mov     [low_string_base_y], bx
        xor     ah, ah
        mov     si, ax
        mov     al, [music_visual_note + si]
        mov     [low_string_note], al
        mov     al, [music_visual_energy + si]
        mov     [low_string_energy], al
        mov     al, dl
        call    low_phys_to_16
        mov     [draw_color], al
        call    line_batch_begin

        ; 0.41: replace the old 32 connected segments with sixteen independent
        ; short horizontal markers.  The hot loop now performs half as many
        ; sine/envelope calculations and never invokes the arbitrary-slope line
        ; rasteriser.  Odd samples 1,3,...31 retain the original spatial shape.
        mov     word [n4_string_phase], 0
        mov     word [n4_string_phase_step], 0
        xor     ax, ax
        mov     al, [low_string_energy]
        mov     dx, 3
        mul     dx
        mov     [n4_string_energy3], ax
        cmp     byte [low_string_note], 0FFh
        je      .phase_ready
        cmp     byte [low_string_energy], 0
        je      .phase_ready
        xor     dx, dx
        mov     dl, [low_string_note]
        and     dx, 000Fh
        mov     cx, dx
        shr     cx, 1
        add     cx, 6
        mov     ax, [city_global_frame]
        imul    cx
        mov     [n4_string_phase], ax
        xor     dx, dx
        mov     dl, [low_string_note]
        and     dx, 000Fh
        mov     cx, dx
        shl     cx, 1
        add     cx, 12
        mov     [n4_string_phase_step], cx
        add     [n4_string_phase], cx       ; sample 1
.phase_ready:
        mov     word [n4_tmp_x], 95
        mov     bp, 1
.marker:
        cmp     bp, 33
        jae     .done
        mov     ax, [low_string_base_y]
        mov     [n4_tmp_y], ax
        cmp     byte [low_string_note], 0FFh
        je      .draw_marker
        cmp     byte [low_string_energy], 0
        je      .draw_marker

        mov     ax, [n4_string_phase]
        xor     bx, bx
        mov     bl, al
        shl     bx, 1
        mov     ax, [sin_table + bx]
        mov     bx, ax

        xor     ax, ax
        mov     si, bp
        mov     al, [n4_string_envelope + si]
        mul     word [n4_string_energy3]
        mov     cx, ax
        mov     ax, bx
        imul    cx                           ; signed 16x16 -> DX:AX
        mov     ax, dx                       ; exact signed product >>18
        sar     ax, 1
        sar     ax, 1
        add     ax, [low_string_base_y]
        mov     [n4_tmp_y], ax

.draw_marker:
        mov     ax, [n4_tmp_x]
        sub     ax, 6
        mov     cx, [n4_tmp_x]
        add     cx, 6
        mov     bx, [n4_tmp_y]
        push    bp
        call    hline_set16_same_fast
        pop     bp
        add     word [n4_tmp_x], 30
        mov     ax, [n4_string_phase_step]
        shl     ax, 1                       ; skip one old sample each marker
        add     [n4_string_phase], ax
        add     bp, 2
        jmp     .marker
.done:
        popa
        ret

; ---------------------------------------------------------------------------
; Scene 5 - SURFACE WAVE.
; Raster data becomes continuous polygonal surface.  Two separated ribbons use
; the full 256-colour ramps without touching the viewport edges.  The first
; ribbon also uses polygon raster-shear, linking the BitBlt effect directly to
; scan-converted geometry.
; ---------------------------------------------------------------------------
; ---------------------------------------------------------------------------
; 0.38 SURFACE WAVE specialised ribbon scan converter (286-safe).
;
; The two generic triangles of each fixed-thickness strip used up to six IDIV
; edge initialisations and emitted almost every horizontal row twice.  This
; fixed-quad path uses two DDA edge initialisations total and emits each logical
; row once.  /200 keeps the existing one-span-per-physical-Y policy.
; With the authored phase step of 13, adjacent sampled Y values differ by at
; most 21 pixels (sin table Q14 >> 8), safely below the 28-pixel thickness;
; therefore the full-width middle section always exists.
; ---------------------------------------------------------------------------
n4_fill_ribbon_quad:
        pusha
        mov     al, [n4_fill_colour_a]
        LOW_PHYS_AL_TO_16_HOT
        mov     [draw_color], al
        call    line_batch_begin
        mov     word [low_last_physical_y], 08000h

        mov     ax, [n4_tri_y1]
        sub     ax, [n4_tri_y0]
        jg      .positive
        jl      .negative

.flat:
        mov     bx, [n4_tri_y0]
        sub     bx, [n4_ribbon_thickness]
        mov     dx, [n4_tri_y0]
.flat_loop:
        cmp     bx, dx
        jg      .done
        mov     ax, [n4_tri_x0]
        mov     cx, [n4_tri_x1]
        call    n4_ribbon_emit_span
        inc     bx
        jmp     .flat_loop

.positive:
        ; Top edge DDA: left-top -> right-top.
        mov     ax, [n4_tri_x0]
        mov     bx, [n4_tri_y0]
        sub     bx, [n4_ribbon_thickness]
        mov     cx, [n4_tri_x1]
        mov     dx, [n4_tri_y1]
        sub     dx, [n4_ribbon_thickness]
        mov     si, low_dda_a
        call    low_dda_init
        mov     bx, [n4_tri_y0]
        sub     bx, [n4_ribbon_thickness]
        mov     dx, [n4_tri_y1]
        sub     dx, [n4_ribbon_thickness]
.pos_top:
        cmp     bx, dx
        jge     .pos_middle_setup
        mov     ax, [n4_tri_x0]
        mov     cx, [low_dda_a+0]
        call    n4_ribbon_emit_span
        mov     si, low_dda_a
        LOW_DDA_STEP_HOT
        inc     bx
        jmp     .pos_top

.pos_middle_setup:
        mov     bx, dx
        mov     dx, [n4_tri_y0]
.pos_middle:
        cmp     bx, dx
        jg      .pos_bottom_setup
        mov     ax, [n4_tri_x0]
        mov     cx, [n4_tri_x1]
        call    n4_ribbon_emit_span
        inc     bx
        jmp     .pos_middle

.pos_bottom_setup:
        mov     ax, [n4_tri_x0]
        mov     bx, [n4_tri_y0]
        mov     cx, [n4_tri_x1]
        mov     dx, [n4_tri_y1]
        mov     si, low_dda_a
        call    low_dda_init
        mov     si, low_dda_a
        LOW_DDA_STEP_HOT
        mov     bx, [n4_tri_y0]
        inc     bx
        mov     dx, [n4_tri_y1]
.pos_bottom:
        cmp     bx, dx
        jg      .done
        mov     ax, [low_dda_a+0]
        mov     cx, [n4_tri_x1]
        call    n4_ribbon_emit_span
        mov     si, low_dda_a
        LOW_DDA_STEP_HOT
        inc     bx
        jmp     .pos_bottom

.negative:
        ; Top edge DDA in increasing Y: right-top -> left-top.
        mov     ax, [n4_tri_x1]
        mov     bx, [n4_tri_y1]
        sub     bx, [n4_ribbon_thickness]
        mov     cx, [n4_tri_x0]
        mov     dx, [n4_tri_y0]
        sub     dx, [n4_ribbon_thickness]
        mov     si, low_dda_a
        call    low_dda_init
        mov     bx, [n4_tri_y1]
        sub     bx, [n4_ribbon_thickness]
        mov     dx, [n4_tri_y0]
        sub     dx, [n4_ribbon_thickness]
.neg_top:
        cmp     bx, dx
        jge     .neg_middle_setup
        mov     ax, [low_dda_a+0]
        mov     cx, [n4_tri_x1]
        call    n4_ribbon_emit_span
        mov     si, low_dda_a
        LOW_DDA_STEP_HOT
        inc     bx
        jmp     .neg_top

.neg_middle_setup:
        mov     bx, dx
        mov     dx, [n4_tri_y1]
.neg_middle:
        cmp     bx, dx
        jg      .neg_bottom_setup
        mov     ax, [n4_tri_x0]
        mov     cx, [n4_tri_x1]
        call    n4_ribbon_emit_span
        inc     bx
        jmp     .neg_middle

.neg_bottom_setup:
        mov     ax, [n4_tri_x1]
        mov     bx, [n4_tri_y1]
        mov     cx, [n4_tri_x0]
        mov     dx, [n4_tri_y0]
        mov     si, low_dda_a
        call    low_dda_init
        mov     si, low_dda_a
        LOW_DDA_STEP_HOT
        mov     bx, [n4_tri_y1]
        inc     bx
        mov     dx, [n4_tri_y0]
.neg_bottom:
        cmp     bx, dx
        jg      .done
        mov     ax, [n4_tri_x0]
        mov     cx, [low_dda_a+0]
        call    n4_ribbon_emit_span
        mov     si, low_dda_a
        LOW_DDA_STEP_HOT
        inc     bx
        jmp     .neg_bottom

.done:
        popa
        ret

; AX=x0, CX=x1, BX=logical y.  GRCG colour is already prepared.
n4_ribbon_emit_span:
        ; 0.43: preserve only the loop registers needed after the clobbering
        ; GRCG span writer instead of PUSHA/POPA on every ribbon scanline.
        push    bx
        push    dx
%ifdef N4_286_200_SCANLINE_THIN
        cmp     byte [video_400_mode], 0
        jne     .logical_y
        mov     dx, bx
        sar     dx, 1
        cmp     dx, [low_last_physical_y]
        je      .done
        mov     [low_last_physical_y], dx
.logical_y:
%endif
        cmp     byte [n4_fill_mode], 4
        jne     .no_shear
        mov     dx, [city_global_frame]
        shl     dx, 1
        mov     si, bx
        mov     di, si
        shl     si, 1
        add     si, di                      ; y*3
        add     dx, si
        push    ax
        xor     ax, ax
        mov     al, [n4_poly_raster_phase_bias]
        mov     si, ax
        pop     ax
        add     dx, si
        and     dx, 00FFh
        mov     si, dx
        shl     si, 1
        mov     dx, [sin_table + si]
        sar     dx, N4_POLY_RASTER_SHIFT
        add     ax, dx
        add     cx, dx
        dec     ax
        inc     cx
.no_shear:
        call    hline_set16_same_fast
.done:
        pop     dx
        pop     bx
        ret

scene4_ribbon_wave:
        pusha
        mov     ax, 40
        mov     bx, 35
        mov     cx, 600
        mov     dx, 350
        ; 0.50: low-colour n4_geo_set_dirty_rect is a no-op; CALL removed
        mov     word [n4_ribbon_base_y], 139
        mov     word [n4_ribbon_thickness], 28
        mov     byte [n4_ribbon_phase_bias], 0
        mov     byte [n4_ribbon_colour_base], 160
        mov     byte [n4_ribbon_fill_mode], 4
        mov     byte [n4_poly_raster_phase_bias], 16
        call    n4_draw_ribbon_layer
        mov     word [n4_ribbon_base_y], 269
        mov     word [n4_ribbon_thickness], 28
        mov     byte [n4_ribbon_phase_bias], 119
        mov     byte [n4_ribbon_colour_base], 192
        mov     byte [n4_ribbon_fill_mode], 0
        call    n4_draw_ribbon_layer

        ; Three fine scanlines behave like strings excited directly by FM0..2.
        ; FM0 is deliberately brightest; harmony/bass strings sit farther back.
        mov     al, 0
        mov     bx, 82
        mov     dl, 95
        call    n4_draw_music_string
        mov     al, 1
        mov     bx, 200
        mov     dl, 151
        call    n4_draw_music_string
        mov     al, 2
        mov     bx, 312                     ; +/-36 remains inside authored envelope
        mov     dl, 215
        call    n4_draw_music_string
        popa
        ret

n4_draw_ribbon_layer:
        pusha

        ; 0.38: adjacent strips share one sine sample.  Carry that boundary
        ; forward and advance the phase by 16 instead of recalculating both
        ; endpoints (and their multiplications) for every strip.
        mov     ax, [city_global_frame]
        imul    ax, ax, 3
        xor     dx, dx
        mov     dl, [n4_ribbon_phase_bias]
        add     ax, dx
        mov     [n4_motion_phase], ax
        xor     bx, bx
        mov     bl, al
        shl     bx, 1
        mov     ax, [sin_table + bx]
        sar     ax, 8
        add     ax, [n4_ribbon_base_y]
        mov     [n4_tri_y0], ax

        mov     word [n4_tmp_x], 60
        xor     bp, bp
.ribbon:
        cmp     bp, 10
        jae     .done
        mov     ax, [n4_tmp_x]
        mov     [n4_tri_x0], ax
        add     ax, 52
        mov     [n4_tri_x1], ax
        mov     [n4_tmp_x], ax

        mov     ax, [n4_motion_phase]
        add     ax, 16
        xor     bx, bx
        mov     bl, al
        shl     bx, 1
        mov     ax, [sin_table + bx]
        sar     ax, 8
        add     ax, [n4_ribbon_base_y]
        mov     [n4_tri_y1], ax

%if (NEON4_STAGE == 8) && (NEON4_P5_BPP != 4)
        ; Direct-colour profiles use the original 256-colour ribbon ramp.
        ; The low-colour edition keeps its authored four-slot palette below.
        mov     ax, bp
        shl     ax, 1
        mov     bx, [city_global_frame]
        shr     bx, 3
        add     ax, bx
        and     al, 63
        cmp     al, 31
        jbe     .ribbon_level_ready
        mov     ah, 63
        sub     ah, al
        mov     al, ah
.ribbon_level_ready:
        mov     ah, al
        add     al, [n4_ribbon_colour_base]
        mov     [n4_fill_colour_a], al
        mov     al, ah
        cmp     al, 3
        jae     .ribbon_b_sub
        xor     al, al
        jmp     .ribbon_b_ready
.ribbon_b_sub:
        sub     al, 3
.ribbon_b_ready:
        add     al, [n4_ribbon_colour_base]
        mov     [n4_fill_colour_b], al
%else
        ; 16-colour/286 editions: use four direct colours per ribbon.
        mov     bx, bp
        and     bx, 3
        cmp     byte [n4_ribbon_colour_base], 192
        jae     .ribbon_bottom_palette
        mov     al, [low_ribbon_palette_top+bx]
        jmp     .ribbon_colour_ready
.ribbon_bottom_palette:
        mov     al, [low_ribbon_palette_bottom+bx]
.ribbon_colour_ready:
        mov     [n4_fill_colour_a], al
        mov     [n4_fill_colour_b], al
%endif
        mov     al, [n4_ribbon_fill_mode]
        mov     [n4_fill_mode], al
        call    n4_fill_ribbon_quad

        mov     ax, [n4_tri_y1]
        mov     [n4_tri_y0], ax
        add     word [n4_motion_phase], 16
        inc     bp
        jmp     .ribbon
.done:
        popa
        ret


; ---------------------------------------------------------------------------
; 0.43 286-safe GRID ARRIVAL checker trapezoid filler.
; Two edge DDAs replace the previous two generic triangles.  /200 still emits
; only one span per physical scanline while both DDAs advance at logical Y.
n4_fill_checker_trapezoid:
        pusha
        mov     al, [n4_fill_colour_a]
        LOW_PHYS_AL_TO_16_HOT
        mov     [draw_color], al
        call    line_batch_begin

        mov     ax, [n4_tri_x0]
        mov     bx, [n4_obj_y0]
        mov     cx, [n4_tri_x2]
        mov     dx, [n4_obj_y1]
        mov     si, low_dda_a
        call    low_dda_init
        mov     ax, [n4_tri_x1]
        mov     bx, [n4_obj_y0]
        mov     cx, [n4_side_x]
        mov     dx, [n4_obj_y1]
        mov     si, low_dda_b
        call    low_dda_init
        mov     ax, [n4_obj_y0]
        mov     [low_tcury], ax
%ifdef N4_286_200_SCANLINE_THIN
        mov     word [low_last_physical_y], 08000h
%endif
.loop:
        mov     ax, [low_tcury]
        cmp     ax, [n4_obj_y1]
        jg      .done
%ifdef N4_286_200_SCANLINE_THIN
        cmp     byte [video_400_mode], 0
        jne     .emit
        mov     dx, ax
        sar     dx, 1
        cmp     dx, [low_last_physical_y]
        je      .advance
        mov     [low_last_physical_y], dx
.emit:
%endif
        mov     ax, [low_dda_a+0]
        mov     cx, [low_dda_b+0]
        mov     bx, [low_tcury]
        call    hline_set16_same_fast
.advance:
        mov     ax, [low_tcury]
        cmp     ax, [n4_obj_y1]
        jge     .done
        mov     si, low_dda_a
        LOW_DDA_STEP_HOT
        mov     si, low_dda_b
        LOW_DDA_STEP_HOT
        inc     word [low_tcury]
        jmp     .loop
.done:
        popa
        ret

; ---------------------------------------------------------------------------
; Scene 6 - GRID ARRIVAL.
; The lower ribbon settles into a perspective checker floor a few rows at a
; time, and a solid is reconstructed above it.  The first reconstructed form
; still carries a mild polygon raster shear; later forms stabilise completely.
; ---------------------------------------------------------------------------
scene4_checker_plane:
        pusha
        mov     ax, 5
        mov     bx, 15
        mov     cx, 635
        mov     dx, 390
        ; 0.50: low-colour n4_geo_set_dirty_rect is a no-op; CALL removed
        mov     ax, [scene_frame]
        shl     ax, 1
        mov     [n4_motion_phase], ax

        mov     ax, [n4_motion_phase]
        add     al, 17
        N4_SIN_INLINE
        sar     ax, 9
        add     ax, 260
        mov     bx, 92
        mov     dl, 201
        call    n4_story_raster_panel

        mov     ax, [n4_motion_phase]
        N4_SIN_INLINE
        sar     ax, 10                      ; +/-16 vanishing-point drift
        add     ax, 320
        mov     [n4_checker_center_x], ax
        mov     ax, [n4_motion_phase]
        add     al, 61
        N4_COS_INLINE
        sar     ax, 12                      ; +/-4 vertical drift
        mov     [n4_checker_y_offset], ax

        mov     ax, [scene_frame]
        shr     ax, 5
        inc     ax
        cmp     ax, 6
        jbe     .row_limit_ok
        mov     ax, 6
.row_limit_ok:
        mov     [n4_checker_row_limit], al

        ; Precompute the two frame-only checker phases once.  The old inner
        ; loop shifted city_global_frame for every visible tile.
        mov     ax, [city_global_frame]
        mov     dx, ax
        shr     ax, 4
        and     al, 1
        mov     [n4_tmp_angle], al
        mov     ax, dx
        shr     ax, 3
        mov     [n4_tmp_x], ax

        xor     bp, bp
.row:
        xor     ax, ax
        mov     al, [n4_checker_row_limit]
        cmp     bp, ax
        jae     .object
        mov     si, bp
        shl     si, 1
        mov     ax, [n4_checker_y + si]
        add     ax, [n4_checker_y_offset]
        mov     [n4_obj_y0], ax
        mov     ax, [n4_checker_y + si + 2]
        add     ax, [n4_checker_y_offset]
        mov     [n4_obj_y1], ax
        mov     ax, [n4_checker_halfw + si]
        mov     [n4_obj_x0], ax
        shr     ax, 2
        mov     [n4_obj_z0], ax             ; top boundary step
        mov     ax, [n4_checker_halfw + si + 2]
        mov     [n4_obj_x1], ax
        shr     ax, 2
        mov     [n4_obj_z1], ax             ; bottom boundary step

        ; Start at column boundary -4.  All authored half-widths are multiples
        ; of four, so repeated addition is bit-exact to (column-4)*halfw/4.
        mov     ax, [n4_checker_center_x]
        sub     ax, [n4_obj_x0]
        mov     [n4_tri_x0], ax             ; current top-left
        mov     ax, [n4_checker_center_x]
        sub     ax, [n4_obj_x1]
        mov     [n4_tri_x2], ax             ; current bottom-left

        ; Row colour is constant across all four visible checker tiles.
        mov     ax, bp
        shl     ax, 2
        add     ax, [n4_tmp_x]
        and     al, 31
        add     al, 224
        mov     [n4_fill_colour_a], al

        xor     di, di
.col:
        cmp     di, 8
        jae     .nextrow
        mov     ax, [n4_tri_x0]
        add     ax, [n4_obj_z0]
        mov     [n4_tri_x1], ax             ; top-right
        mov     ax, [n4_tri_x2]
        add     ax, [n4_obj_z1]
        mov     [n4_side_x], ax             ; bottom-right

        mov     ax, di
        add     ax, bp
        add     al, [n4_tmp_angle]
        test    al, 1
        jnz     .skip_tile
        call    n4_fill_checker_trapezoid
.skip_tile:
        mov     ax, [n4_tri_x1]
        mov     [n4_tri_x0], ax
        mov     ax, [n4_side_x]
        mov     [n4_tri_x2], ax
        inc     di
        jmp     .col
.nextrow:
        inc     bp
        jmp     .row

.object:
        mov     ax, [n4_motion_phase]
        add     al, 97
        N4_SIN_INLINE
        sar     ax, 9                       ; +/-32
        add     ax, 320
        mov     [n4_geo_center_x], ax
        mov     ax, [n4_motion_phase]
        add     al, 33
        N4_COS_INLINE
        sar     ax, 11
        add     ax, 175
        mov     [n4_geo_center_y], ax
        mov     ax, [n4_motion_phase]
        add     al, 151
        N4_SIN_INLINE
        test    ax, ax
        jns     .obj_depth_abs
        neg     ax
.obj_depth_abs:
        sar     ax, 9
        mov     bx, 1050
        sub     bx, ax                      ; all stage solids fit without top clipping
        mov     [n4_geo_object_depth], bx
        mov     ax, [n4_motion_phase]
        mov     ah, al
        add     ah, 39
        call    n4_geo_set_angles
        mov     byte [n4_geo_fill_mode], 0
        mov     ax, [scene_frame]
        shr     ax, 7
        and     al, 3
        cmp     al, 0
        jne     .stage_octa
        mov     byte [n4_geo_colour_base], 32
        mov     byte [n4_geo_fill_mode], 4
        mov     byte [n4_poly_raster_phase_bias], 211
        call    n4_geo_draw_icosa
        jmp     .done
.stage_octa:
        cmp     al, 1
        jne     .stage_pyr
        mov     byte [n4_geo_colour_base], 128
        call    n4_geo_draw_octa
        jmp     .done
.stage_pyr:
        cmp     al, 2
        jne     .stage_tetra
        mov     byte [n4_geo_colour_base], 160
        call    n4_geo_draw_pyramid
        jmp     .done
.stage_tetra:
        mov     byte [n4_geo_colour_base], 64
        call    n4_geo_draw_tetra
.done:
        popa
        ret

; ---------------------------------------------------------------------------
; Scene 7 - SOLID FINALE.
; The signal converges on three restrained material samples, then forms a bright
; central polyhedron surrounded by an eight-blade colour corona.  The corona is
; deliberately contained (175px radius): it reads as a climax without crushing
; the frame or touching the hardware edges.
; ---------------------------------------------------------------------------
scene4_material_finale:
        pusha
        ; 0.43: once the shutter reaches 200px from both edges the graphics
        ; page is fully black.  Do not build hidden corona/solids behind it.
        cmp     word [scene_frame], N4_FINAL_FADE_START + 50
        jb      .visible_finale
        call    n4_finale_black_shutter
        popa
        ret
.visible_finale:
        mov     ax, 40
        mov     bx, 20
        mov     cx, 600
        mov     dx, 380
        ; 0.50: low-colour n4_geo_set_dirty_rect is a no-op; CALL removed
        mov     ax, [scene_frame]
        shl     ax, 1
        mov     [n4_motion_phase], ax
        cmp     word [scene_frame], N4_FINALE_BUILD_START
        jae     .finale

        ; Two carrier fragments visibly converge toward the centre.
        ; These panels are adjacent, so one EGC/PEGC batch covers both.
        call    n4_blit_ensure_asset
        call    n4_blit_batch_begin
        mov     ax, [scene_frame]
        shr     ax, 2
        add     ax, 128
        mov     bx, 112
        mov     dl, 31
        call    n4_story_raster_panel
        mov     ax, 416
        mov     dx, [scene_frame]
        shr     dx, 2
        sub     ax, dx
        mov     bx, 244
        mov     dl, 159
        call    n4_story_raster_panel
        call    n4_blit_batch_end

        mov     word [n4_geo_object_depth], 980
        mov     ax, [n4_motion_phase]
        mov     ah, al
        add     ah, 23
        call    n4_geo_set_angles

        mov     ax, [n4_motion_phase]
        N4_SIN_INLINE
        sar     ax, 12
        add     ax, 165
        mov     [n4_geo_center_x], ax
        mov     word [n4_geo_center_y], 194
        mov     byte [n4_geo_colour_base], 32
        mov     byte [n4_geo_fill_mode], 0
        call    n4_geo_draw_tetra

        mov     ax, [n4_motion_phase]
        add     al, 85
        N4_SIN_INLINE
        sar     ax, 12
        add     ax, 320
        mov     [n4_geo_center_x], ax
        mov     byte [n4_geo_colour_base], 128
        mov     byte [n4_geo_fill_mode], 1
        call    n4_geo_draw_cube

        mov     ax, [n4_motion_phase]
        add     al, 170
        N4_SIN_INLINE
        sar     ax, 12
        add     ax, 475
        mov     [n4_geo_center_x], ax
        mov     byte [n4_geo_colour_base], 192
        mov     byte [n4_geo_fill_mode], 2
        call    n4_geo_draw_icosa
        jmp     .done

.finale:
        ; Last trace of the carrier passes through the corona before becoming solid.
        cmp     word [scene_frame], N4_FINALE_CARRIER_END
        jae     .storm_setup
        mov     ax, [n4_motion_phase]
        add     al, 47
        N4_SIN_INLINE
        sar     ax, 10
        add     ax, 272
        mov     bx, 180
        mov     dl, 223
        call    n4_story_raster_panel
.storm_setup:
        xor     bp, bp
.storm:
        cmp     bp, 8
        jae     .central
        mov     ax, bp
        shl     ax, 5                       ; 32 phase steps = eight blades
        add     ax, [n4_motion_phase]
        mov     [n4_fan_phase], al
        ; 0.44: the finale phase and all blade offsets are even.  Index the
        ; pre-scaled polar tables (128 entries/radius) instead of doing six
        ; trig-radius multiplies for each blade.
        xor     si, si
        mov     al, [n4_fan_phase]
        xor     ah, ah
        shr     ax, 1
        mov     si, ax
        shl     si, 2
        mov     ax, [n4_polar175_even_xy + si]
        add     ax, 320
        mov     [n4_tri_x0], ax
        mov     ax, [n4_polar175_even_xy + si + 2]
        add     ax, 200
        mov     [n4_tri_y0], ax

        mov     al, [n4_fan_phase]
        add     al, 12
        xor     ah, ah
        shr     ax, 1
        mov     si, ax
        shl     si, 2
        mov     ax, [n4_polar175_even_xy + si]
        add     ax, 320
        mov     [n4_tri_x1], ax
        mov     ax, [n4_polar175_even_xy + si + 2]
        add     ax, 200
        mov     [n4_tri_y1], ax

        mov     al, [n4_fan_phase]
        add     al, 6
        xor     ah, ah
        shr     ax, 1
        mov     si, ax
        shl     si, 2
        mov     ax, [n4_polar92_even_xy + si]
        add     ax, 320
        mov     [n4_tri_x2], ax
        mov     ax, [n4_polar92_even_xy + si + 2]
        add     ax, 200
        mov     [n4_tri_y2], ax
%if (NEON4_STAGE == 8) && (NEON4_P5_BPP != 4)
        ; Direct-colour profiles use the original 256-colour finale ramp.
        mov     bx, bp
        mov     ax, [city_global_frame]
        shr     ax, 3
        add     bx, ax
        and     bx, 15
        mov     al, [n4_finale_colours + bx]
%else
        mov     bx, bp
        and     bx, 7
        mov     al, [low_finale_blade_colours + bx]
%endif
        mov     [n4_fill_colour_a], al
        mov     byte [n4_fill_mode], 0
        call    n4_fill_triangle
        inc     bp
        jmp     .storm

.central:
        mov     ax, [n4_motion_phase]
        add     al, 41
        N4_SIN_INLINE
        sar     ax, 11
        add     ax, 320
        mov     [n4_geo_center_x], ax
        mov     ax, [n4_motion_phase]
        add     al, 109
        N4_COS_INLINE
        sar     ax, 12
        add     ax, 200
        mov     [n4_geo_center_y], ax
        mov     ax, [scene_frame]
        sub     ax, N4_FINALE_BUILD_START
        shr     ax, 1
        mov     bx, 930
        sub     bx, ax
        cmp     bx, 740
        jge     .depth_ok
        mov     bx, 740
.depth_ok:
        mov     [n4_geo_object_depth], bx
        mov     ax, [n4_motion_phase]
        mov     ah, al
        add     ah, 61
        call    n4_geo_set_angles
        mov     byte [n4_geo_fill_mode], 0
        test    byte [scene_frame], 40h
        jz      .final_mode_ready
        mov     byte [n4_geo_fill_mode], 4
        mov     byte [n4_poly_raster_phase_bias], 240
.final_mode_ready:
        mov     ax, [scene_frame]
        shr     ax, 6
        test    al, 1
        jnz     .final_icosa
        mov     byte [n4_geo_colour_base], 64
        call    n4_geo_draw_octa
        jmp     .done
.final_icosa:
        mov     byte [n4_geo_colour_base], 224
        call    n4_geo_draw_icosa
.done:
        cmp     word [scene_frame], N4_FINAL_FADE_START
        jb      .return
        call    n4_finale_black_shutter
.return:
        popa
        ret

n4_finale_black_shutter:
        pusha
        mov     ax, [scene_frame]
        sub     ax, N4_FINAL_FADE_START
        shl     ax, 2                       ; 0..252 pixels from each edge
        cmp     ax, 200
        jbe     .height_ok
        mov     ax, 200
.height_ok:
        mov     dx, ax
        or      dx, dx
        jz      .done
        mov     byte [draw_color], 0
        ; top band y=0..height-1
        mov     si, dx
        dec     si
        xor     ax, ax
        xor     bx, bx
        mov     cx, SCREEN_W-1
        call    fill_rect
        ; bottom band y=400-height..399
        mov     bx, SCREEN_H
        sub     bx, dx
        mov     si, SCREEN_H-1
        xor     ax, ax
        mov     cx, SCREEN_W-1
        call    fill_rect
.done:
        popa
        ret
