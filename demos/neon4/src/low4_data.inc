; Copyright (c) 2026 Nakata Maho
; Ported By Maho Nakata.
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the BSD 2-Clause conditions apply.

; ---------------------------------------------------------------------------
; LOW4_DATA.INC - state private to NEON4 analog 16-colour / 286 renderers
; ---------------------------------------------------------------------------
%ifdef NEON4_286
video_400_mode          db 0
video_height            dw 200
video_last_line         dw 199
video_clear_words       dw 8000
%else
video_400_mode          db 1
video_height            dw 400
video_last_line         dw 399
video_clear_words       dw 16000
%endif
egc16_saved_page        db 0
grcg16_mode_cache       db 0
; RASTER TRANSFER EGC/VRAM-cache accelerator state.
; The functional probe is authoritative; failure selects the GRCG fallback.
low_egc_available        db 0
low_egc_probe_ok         db 0
low_egc_batch_active     db 0
low_egc_asset_valid      db 0          ; bit0 page0, bit1 page1
low_egc_saved_draw_color db 0
low_egc_cache_count      db 0
low_egc_cat_physical     db 0
low_raster_egc_used      db 0          ; last frame that issued a cached raster blit via EGC
low_surface_page_valid    db 0,0        ; SURFACE WAVE fixed-envelope page clear state
low_clear_scene_stamp      db 0FFh,0FFh  ; scene last rendered on each physical page
low_egc_shift_word       dw 0
low_egc_cache_row_base   dw 0
low_egc_dest_y           dw 0

; 0.47 ordinary-scene scanline dirty history.  X is stored in native GRCG
; byte units (eight pixels), so two 400-line pages need only 1600 bytes.
; In 286/200 mode only entries 0..199 of each 400-entry page are used.
low_dirty_span_enable      db 0
        align 2
low_dirty_span_base        dw 0
low_dirty_span_current_y0  dw 07FFFh
low_dirty_span_current_y1  dw -1
low_dirty_span_y0          dw 07FFFh,07FFFh
low_dirty_span_y1          dw -1,-1
low_dirty_span_min         times 800 db 0FFh
low_dirty_span_max         times 800 db 0


; 0.43 byte-aligned conservative graphics envelopes for ordinary low-colour
; scenes.  Scene changes still receive one full-page clear per physical page;
; repeated frames clear only these regions.  Entries are x0,y0,x1,y1.
low_scene_clear_rects:
        dw 88,15,551,385       ; 0 SIGNAL SEED
        dw 56,5,583,395        ; 1 FACET ASSEMBLY
        dw 48,25,607,390       ; 2 MATERIAL ASSEMBLY
        dw 88,15,551,385       ; 3 MORPH GATE
        dw 0,0,639,399         ; 4 special seven-rectangle path
        dw 40,44,599,350       ; 5 special SURFACE envelope
        dw 0,15,639,390        ; 6 GRID ARRIVAL
        dw 40,20,607,380       ; 7 SOLID FINALE

; Analog 16-colour palette, G/R/B nibbles.
;
; The low-colour renderer maps authored 256-colour indices by their high nibble.
; Therefore 2/3 represent the two halves of the 256 blue ramp (32..63), 4/5
; cyan (64..95), 6/7 green (96..127), 8/9 yellow (128..159), 10/11 red
; (160..191), and 12/13 magenta (192..223).  This table now follows those
; original hue families instead of assigning unrelated colours.  Slots 14/15
; stand in for the rotating 224..255 spectrum and deliberately use two vivid,
; contrasting spectrum colours so the finale remains colourful.
low4_palette_grb:
        db 0,0,0       ;  0 black / background
        db 11,11,11    ;  1 bright neutral (256 grayscale 16..31)
        db 2,1,4       ;  2 dark blue
        db 7,3,11      ;  3 bright blue
        db 4,1,4       ;  4 dark cyan
        db 11,3,11     ;  5 bright cyan
        db 4,1,2       ;  6 dark green
        db 11,4,6      ;  7 bright green
        db 4,4,1       ;  8 dark yellow / olive
        db 12,12,4     ;  9 bright yellow
        db 2,4,0       ; 10 dark red
        db 5,11,1      ; 11 bright red / orange
        db 1,4,4       ; 12 dark magenta
        db 3,11,11     ; 13 bright magenta
        db 15,2,15     ; 14 vivid cyan (spectrum substitute)
        db 2,15,2      ; 15 vivid red  (opposite spectrum substitute)


; ---------------------------------------------------------------------------
; Low-colour face separation.
;
; Faces are graph-coloured per solid so any two REAL surfaces sharing an edge
; use different colour classes.  Each class is then rendered with a two-colour
; scanline dither.  The theme is selected from n4_geo_colour_base, but unlike
; the old high-nibble quantiser the actual 16-colour pairs are chosen for strong
; separation first.  The two base triangles of the pyramid deliberately share
; one class because together they are one square surface.
; ---------------------------------------------------------------------------
low_face_class_ptr       dw low_face_class_cube
low_face_dither_enable   db 0
low_face_dither_phase    db 0
; 0.50 preconverted 4-bit face colours.  Triangle/quad scanlines alternate
; these bytes directly instead of remapping the same physical colours per row.
low_fill_colour16_a      db 0
low_fill_colour16_b      db 0
; 0.45: low-colour quads are scan-converted as one convex polygon instead of
; two triangles.  Four perimeter DDAs feed one left/right span per scanline, so
; there is no authored/internal diagonal whose rounding can leak through the
; two-colour row pattern.
align 2
low_quad_x               dw 0,0,0,0
low_quad_y               dw 0,0,0,0
low_quad_edge0           times 5 dw 0
low_quad_edge1           times 5 dw 0
low_quad_edge2           times 5 dw 0
low_quad_edge3           times 5 dw 0
; Per edge: minY,maxY.
low_quad_edge_range      dw 0,0, 0,0, 0,0, 0,0
low_quad_min_y           dw 0
low_quad_max_y           dw 0
low_quad_cur_y           dw 0
low_quad_span_l          dw 0
low_quad_span_r          dw 0

low_face_class_cube:     db 0,0,1,1,2,2
low_face_class_octa:     db 0,1,0,1,1,0,1,0
low_face_class_tetra:    db 0,1,2,3
low_face_class_pyramid:  db 0,0,1,2,1,2
low_face_class_icosa:
        db 0,1,0,1,2, 0,1,0,0,1, 0,1,2,0,1, 2,2,1,2,2

; 8 themes x 4 face classes x (A,B).  Entries are direct analog-16 indices.
; Adjacent classes never share an identical pair; several themes intentionally
; cross hue families because geometric readability has priority over 256-colour
; palette fidelity in these editions.
low_face_theme_pairs:
        ; theme 0: neutral/fallback
        db 1,3,   5,7,   9,11,  13,15
        ; theme 1: blue object
        db 3,5,   13,3,  1,5,   9,3
        ; theme 2: cyan object
        db 5,14,  3,5,   7,5,   13,14
        ; theme 3: green object
        db 7,5,   9,7,   14,7,  3,5
        ; theme 4: yellow object
        db 9,11,  7,9,   13,9,  1,11
        ; theme 5: red object
        db 11,15, 9,11,  13,15, 1,11
        ; theme 6: magenta object
        db 13,15, 3,13,  11,13, 5,14
        ; theme 7: spectrum/finale object
        db 14,5,  15,11, 9,7,   13,3

; SURFACE WAVE uses direct low-colour strip palettes so neighbouring strips do
; not collapse into the same high-nibble colour after quantisation.
low_ribbon_palette_top:    db 11,13,9,15
low_ribbon_palette_bottom: db 5,3,14,7

; Eight corona blades: all neighbours, including blade 7 -> blade 0, differ.
low_finale_blade_colours:  db 14,9,15,5,13,7,11,3

; Primitive work state.
low_line_x0 dw 0
low_line_y0 dw 0
low_line_x1 dw 0
low_line_y1 dw 0
low_line_dx dw 0
low_line_dy dw 0
low_line_sx dw 0
low_line_sy dw 0
low_line_err dw 0
low_line_e2 dw 0
low_line_mask db 0
low_hx0 dw 0
low_hx1 dw 0
low_hstart db 0
low_hend db 0

; Exact scan-converter DDA states.  Each state is x,q,remainder,accumulator,dy.
low_dda_a:              times 5 dw 0
low_dda_b:              times 5 dw 0
low_dda_b_lower         db 0
align 2
low_last_physical_y     dw 08000h

; Address/mask lookup tables used by the hot line/span paths.
low_pixel_mask_table:   db 080h,040h,020h,010h,08h,04h,02h,01h
align 2
low_row_offset_table:
%assign N4_LOW_ROW_I 0
%rep 400
        dw N4_LOW_ROW_I*80
%assign N4_LOW_ROW_I N4_LOW_ROW_I+1
%endrep
%undef N4_LOW_ROW_I

; Triangle / polygon work state.
low_tx0 dw 0
low_ty0 dw 0
low_tx1 dw 0
low_ty1 dw 0
low_tx2 dw 0
low_ty2 dw 0
low_tcury dw 0
low_edge_x0 dw 0
low_edge_y0 dw 0
low_edge_x1 dw 0
low_edge_y1 dw 0
low_interp_y dw 0
low_ix_a dw 0
low_ix_b dw 0
low_tmp_x dw 0
low_tmp_y dw 0
low_tmp_x2 dw 0
low_tmp_y2 dw 0
low_cx dw 0
low_cy dw 0
low_size dw 0
low_phase db 0
low_base_color db 0
low_radius dw 0
low_radius2 dw 0
low_angle db 0
low_card_x dw 0
low_card_y dw 0
; RASTER TRANSFER page-local clear cache.  A page is marked valid only after
; it has been fully cleared once inside scene 4; later scene-4 frames erase only
; the seven byte-aligned object bounds previously drawn on that physical page.
low_raster_page_valid db 0,0
low_raster_rect_count db 0,0
align 2
; Seven previous object bounds per physical page: 3 raster bands + 4 cards.
; X values are byte-aligned at record time so the erase path needs no masks.
low_raster_rect_x0    times 14 dw 0
low_raster_rect_y0    times 14 dw 0
low_raster_rect_x1    times 14 dw 0
low_raster_rect_y1    times 14 dw 0
low_raster_dirty_x0   dw 0
low_raster_dirty_y0   dw 0
low_raster_dirty_x1   dw 0
low_raster_dirty_y1   dw 0
low_cat_bg_color      db 0
low_cat_level         db 0
low_ribbon_base dw 0
low_ribbon_thick dw 0
low_ribbon_bias db 0
low_string_channel db 0
low_string_note db 0FFh
low_string_energy db 0
low_string_base_y dw 0
low_string_prev_x dw 0
low_string_prev_y dw 0
low_q14_div dw 16384
low_three dw 3
low_sixteen dw 16

; ---------------------------------------------------------------------------
; RASTER TRANSFER CAT hot-path table (0.32).
;
; DATA4.INC keeps the canonical row RLE for the 256-colour asset builder.
; The low-colour renderer uses this derived table instead: for every logical row,
; spans are grouped by source luminance level 0..3.  Spans within one row never
; overlap, so their colour order is irrelevant, while keeping rows in original
; order preserves the exact 286 400->200 overwrite semantics.
;
; 48 rows * 4 group counts.  Span pairs follow row0 level0..3, row1 level0..3...
; Physical palette values are still read from n4_cat48_palette at runtime.
; ---------------------------------------------------------------------------
low_cat48_group_counts:
        db 0,0,2,4,1,3,3,2,4,5,5,5,4,3,5,3
        db 4,6,6,8,3,3,5,9,3,3,2,6,0,4,1,2
        db 2,2,3,4,1,4,2,3,3,1,5,3,3,2,4,2
        db 0,4,4,2,2,2,0,5,3,1,2,5,4,0,1,8
        db 3,2,1,5,2,3,0,5,2,0,2,5,1,2,1,1
        db 2,1,5,4,5,5,3,2,1,2,1,4,2,1,4,3
        db 2,2,3,3,2,4,2,1,1,2,1,4,2,3,2,3
        db 2,4,4,3,4,6,6,6,2,6,5,5,3,3,6,1
        db 3,3,6,3,3,3,3,3,4,2,3,5,2,3,2,2
        db 4,3,2,3,1,2,2,5,3,3,6,4,6,4,8,8
        db 6,5,5,8,5,2,7,7,6,4,5,5,6,4,6,8
        db 4,5,7,3,6,7,7,2,3,5,2,4,0,0,3,5
low_cat48_group_spans:
        db 8,10,37,39,7,7,11,11,36,36,40,40,7,7,6,6
        db 8,12,35,40,13,13,34,34,41,41,14,14,33,33,6,6
        db 14,14,33,33,41,41,5,5,13,13,15,15,32,32,34,34
        db 7,7,12,12,35,35,40,40,42,42,8,8,10,11,16,16
        db 31,31,36,36,5,5,16,16,31,31,42,42,15,15,30,30
        db 32,32,6,6,14,14,17,17,33,33,41,41,4,4,18,18
        db 29,29,5,5,18,18,29,29,42,42,10,11,17,17,19,19
        db 28,28,30,30,36,37,12,12,16,16,20,23,25,27,31,31
        db 35,35,4,4,6,6,9,9,24,24,34,34,38,38,41,41
        db 43,43,5,5,34,34,42,42,12,13,20,27,35,35,11,11
        db 19,19,28,28,33,33,36,36,4,4,6,6,10,10,14,14
        db 18,18,29,29,37,37,41,41,43,43,14,14,33,33,42,42
        db 5,5,13,13,34,34,32,32,41,41,4,4,6,6,12,12
        db 15,15,22,25,35,35,5,5,14,15,32,33,41,42,6,6
        db 12,13,34,35,15,15,32,32,5,6,41,41,14,14,33,33
        db 42,42,12,13,16,16,31,31,34,35,15,15,6,6,14,14
        db 32,33,41,41,5,5,42,42,12,13,34,35,40,40,6,6
        db 14,14,33,33,41,41,5,5,13,13,15,15,32,32,34,34
        db 7,7,40,40,42,42,6,6,13,13,34,34,33,33,41,41
        db 12,12,14,14,35,35,40,40,5,5,7,7,6,6,12,12
        db 35,35,41,41,7,7,13,13,34,34,40,40,20,20,27,27
        db 20,20,27,27,6,7,40,41,19,19,21,21,26,26,28,28
        db 38,39,7,7,20,20,27,27,40,40,6,6,41,41,19,19
        db 21,21,26,26,28,28,37,39,7,7,20,20,27,27,40,40
        db 39,39,6,6,8,8,19,19,21,21,26,26,28,28,37,38
        db 41,41,20,20,27,27,40,40,7,7,39,39,8,8,19,19
        db 21,21,26,26,28,28,37,38,27,27,39,39,7,8,20,20
        db 40,40,19,19,21,21,26,26,28,28,36,38,8,8,39,39
        db 38,38,40,40,7,7,9,9,20,20,27,27,36,37,39,39
        db 8,8,38,38,9,9,36,37,8,9,38,39,37,37,4,7
        db 10,13,23,24,34,36,40,43,14,16,22,22,25,25,30,33
        db 9,10,14,16,22,25,31,32,37,38,4,8,11,13,30,30
        db 33,36,39,42,17,17,21,21,43,43,26,26,29,29,22,25
        db 9,10,37,38,35,36,11,17,21,21,26,26,30,34,10,10
        db 37,37,23,24,9,9,25,25,35,36,38,38,11,12,22,22
        db 32,34,10,10,36,37,11,17,29,35,7,9,18,18,38,40
        db 4,6,28,28,41,43,11,11,36,36,4,10,12,15,31,35
        db 37,43,16,18,29,30,3,3,35,35,11,12,36,36,33,34
        db 4,6,13,13,30,32,41,43,12,12,35,35,13,13,15,17
        db 30,34,14,14,29,29,11,11,28,28,36,36,11,14,32,35
        db 10,10,15,16,30,31,36,37,9,9,17,17,29,29,38,38
        db 8,8,26,28,39,39,8,8,14,14,33,33,39,39,6,7
        db 9,9,15,15,32,32,38,38,40,40,10,11,13,13,28,31
        db 34,34,36,37,41,41,5,5,12,12,16,16,26,27,35,35
        db 42,42,16,16,31,31,5,6,15,15,17,17,29,30,32,32
        db 41,41,7,7,18,19,26,28,40,40,42,42,8,8,14,14
        db 20,25,33,33,39,39,17,20,27,30,37,37,26,26,36,36
        db 38,38,16,16,21,21,25,25,31,31,35,35,39,39,22,24
        db 30,30,35,35,39,39,17,17,29,29,38,38,16,16,26,28
        db 31,31,34,34,36,36,40,40,18,21,23,25,37,37,16,16
        db 31,31,34,34,30,30,35,35,39,40,15,15,29,29,32,32
        db 17,17,27,28,38,38,15,15,32,32,34,34,40,40,31,31
        db 39,39,16,16,29,30,35,35,14,14,27,28,33,33,38,38
        db 41,41,32,32,39,39,14,15,33,35,40,40,30,31,38,38
        db 29,29,36,37,14,14,33,33,35,35,38,39,32,32,34,34
        db 36,37,31,31,40,40,13,13,15,15,30,30,33,34,14,14
        db 35,38,13,13,32,32,5,6,28,28,31,31,39,39,41,42
        db 4,6,33,33,41,43,7,7,14,14,40,40,3,3,8,8
        db 13,13,32,32,34,34,44,44,19,20,28,28,31,31,39,39
        db 3,3,8,8,14,14,33,33,39,39,44,44,7,7,9,9
        db 38,38,40,40,4,4,10,10,13,13,19,19,32,32,34,34
        db 37,37,41,43,2,2,5,6,11,12,20,20,27,28,31,31
        db 35,36,45,45,3,3,10,11,13,14,33,33,36,37,44,44
        db 9,9,12,12,32,32,34,35,38,38,4,4,19,20,27,27
        db 39,39,43,43,2,2,8,8,15,15,21,21,28,28,31,31
        db 41,42,45,45,4,4,14,14,20,20,27,27,43,43,32,33
        db 44,44,3,3,5,5,15,15,19,19,21,21,26,26,41,42
        db 6,6,11,13,22,22,28,28,31,31,34,36,40,40,5,5
        db 15,15,21,21,26,26,32,32,42,42,4,4,20,20,41,41
        db 43,43,6,6,14,14,27,27,31,31,33,33,7,7,19,19
        db 22,23,30,30,40,40,6,6,15,15,21,21,26,26,32,32
        db 41,41,5,5,7,7,40,40,42,42,20,20,22,22,25,25
        db 30,31,33,33,39,39,8,9,14,14,16,16,19,19,23,23
        db 27,27,29,29,36,38,7,8,21,21,32,32,39,40,15,16
        db 22,23,25,26,31,31,38,38,6,6,9,9,20,20,24,24
        db 29,30,37,37,41,41,10,10,28,28,33,36,10,10,16,16
        db 21,22,25,26,31,31,37,38,9,9,11,11,23,24,29,30
        db 32,32,36,36,39,39,8,8,12,12,15,15,17,17,20,20
        db 27,28,33,35,13,14,40,40,12,17,20,20,27,35,11,11
        db 18,19,21,21,26,26,36,36,10,10,37,37,9,9,22,22
        db 25,25,38,38,14,19,28,29,31,33,12,13,20,20,27,27
        db 30,30,34,35

%ifdef NEON4_286
; ---------------------------------------------------------------------------
; 286 / 200-line final CAT ownership maps (0.32).
;
; Symbols: 0..3 = n4_cat48_palette luminance levels, 4 = box background,
;          5 = box edge.
; Because logical Y is halved at the final primitive, the exact final card
; differs depending on destination Y parity.  These tables pre-compose the old
; background -> frame -> logical row overlay order into physical rows.  The
; renderer still maps symbol colours at runtime, so user palette edits remain
; effective.
; ---------------------------------------------------------------------------
low_cat200_even_counts:
        db 1,3,3,2,0,3,4,5,7,6,5,2,3,5,7,9,6,2
        db 0,4,1,4,6,2,1,4,2,5,5,2,3,2,5,3,5,2
        db 2,4,2,5,7,2,4,0,1,8,5,2,2,3,0,5,5,2
        db 1,2,2,4,5,2,5,5,3,2,4,2,2,1,4,4,4,2
        db 2,4,2,2,3,2,2,3,2,5,5,2,4,6,7,6,3,2
        db 3,7,8,4,4,2,3,3,5,4,4,2,2,3,3,3,3,2
        db 1,2,3,6,5,2,6,4,8,8,5,2,7,4,8,10,6,2
        db 6,4,6,8,7,2,7,7,9,2,3,2,0,4,5,9,0,3
low_cat200_even_spans:
        db 7,7,6,6,8,12,35,40,13,13,34,34,41,41,14,14
        db 33,33,0,5,15,32,42,47,5,5,16,16,31,31,42,42
        db 13,13,15,15,30,30,32,32,34,34,6,7,12,12,14,14
        db 17,17,33,33,35,35,40,41,4,4,8,8,10,11,18,18
        db 29,29,36,36,1,3,9,9,19,28,37,39,43,46,0,0
        db 47,47,5,5,34,34,42,42,12,13,17,17,20,27,30,30
        db 35,35,11,11,16,16,19,19,28,28,31,31,33,33,36,36
        db 4,4,6,6,9,10,14,14,18,18,29,29,37,38,41,41
        db 43,43,1,3,7,8,15,15,32,32,39,40,44,46,0,0
        db 47,47,5,5,14,15,32,33,41,42,6,6,4,4,12,13
        db 22,25,34,35,1,3,7,11,16,21,26,31,36,40,43,46
        db 0,0,47,47,15,15,6,6,14,14,32,33,41,41,5,5
        db 42,42,12,13,16,16,31,31,34,35,40,40,1,4,7,11
        db 17,30,36,39,43,46,0,0,47,47,6,6,13,13,34,34
        db 33,33,41,41,12,12,14,15,32,32,35,35,40,40,5,5
        db 7,7,42,42,1,4,8,11,16,31,36,39,43,46,0,0
        db 47,47,20,20,27,27,6,7,12,12,35,35,40,41,13,13
        db 34,34,19,19,21,21,26,26,28,28,38,39,1,5,8,11
        db 14,18,22,25,29,33,36,37,42,46,0,0,47,47,7,7
        db 20,20,27,27,40,40,39,39,6,6,8,8,19,19,21,21
        db 26,26,28,28,37,38,41,41,1,5,9,18,22,25,29,36
        db 42,46,0,0,47,47,27,27,39,39,7,8,20,20,40,40
        db 19,19,21,21,26,26,28,28,36,38,1,6,9,18,22,25
        db 29,35,41,46,0,0,47,47,39,39,8,8,38,38,9,9
        db 40,40,7,7,20,20,27,27,36,37,1,6,10,19,21,26
        db 28,35,41,46,0,0,47,47,9,10,14,16,22,25,31,32
        db 37,38,4,8,11,13,30,30,33,36,39,42,17,17,21,21
        db 43,43,26,26,29,29,1,3,18,20,27,28,44,46,0,0
        db 47,47,10,10,37,37,23,24,9,9,25,25,35,36,38,38
        db 11,17,21,22,26,26,30,34,1,8,18,20,27,29,39,46
        db 0,0,47,47,11,11,36,36,4,10,12,15,31,35,37,43
        db 16,18,29,30,3,3,28,28,1,2,19,27,44,46,0,0
        db 47,47,12,12,35,35,13,13,15,17,30,34,14,14,29,29
        db 4,6,11,11,28,28,36,36,41,43,1,3,7,10,18,27
        db 37,40,44,46,0,0,47,47,8,8,14,14,33,33,39,39
        db 6,7,9,9,15,15,32,32,38,38,40,40,10,11,13,13
        db 17,17,28,31,34,34,36,37,41,41,5,5,12,12,16,16
        db 26,27,35,35,42,42,1,4,18,25,43,46,0,0,47,47
        db 17,20,27,30,37,37,5,6,15,15,26,26,32,32,36,36
        db 38,38,41,41,7,7,16,16,21,21,25,25,31,31,35,35
        db 39,40,42,42,8,8,14,14,22,24,33,33,1,4,9,13
        db 34,34,43,46,0,0,47,47,16,16,31,31,34,34,30,30
        db 35,35,39,40,15,15,26,26,29,29,32,32,36,36,17,21
        db 23,25,27,28,37,38,1,14,22,22,33,33,41,46,0,0
        db 47,47,32,32,39,39,14,15,33,35,40,40,16,16,30,31
        db 38,38,27,29,36,37,41,41,1,13,17,26,42,46,0,0
        db 47,47,33,34,14,14,35,38,13,13,32,32,40,40,5,6
        db 15,15,28,28,30,31,39,39,41,42,1,4,7,12,16,27
        db 29,29,43,46,0,0,47,47,3,3,8,8,14,14,33,33
        db 39,39,44,44,7,7,9,9,38,38,40,40,4,4,10,10
        db 13,13,19,19,32,32,34,34,37,37,41,43,2,2,5,6
        db 11,12,20,20,27,28,31,31,35,36,45,45,1,1,15,18
        db 21,26,29,30,46,46,0,0,47,47,4,4,10,10,14,14
        db 20,20,27,27,37,37,43,43,9,9,32,33,38,38,44,44
        db 3,3,5,5,15,15,19,19,21,21,26,26,39,39,41,42
        db 2,2,6,6,8,8,11,13,22,22,28,28,31,31,34,36
        db 40,40,45,45,1,1,7,7,16,18,23,25,29,30,46,46
        db 0,0,47,47,6,6,15,15,21,21,26,26,32,32,41,41
        db 4,5,7,7,40,40,42,43,20,20,22,22,25,25,30,31
        db 33,33,39,39,8,9,14,14,16,16,19,19,23,23,27,27
        db 29,29,36,38,1,3,10,13,17,18,24,24,28,28,34,35
        db 44,46,0,0,47,47,7,7,10,10,16,16,21,22,25,26
        db 31,31,37,38,9,9,11,11,23,24,29,30,32,32,36,36
        db 39,39,6,6,8,8,12,12,15,15,17,17,20,20,27,28
        db 33,35,41,41,13,14,40,40,1,5,18,19,42,46,0,0
        db 47,47,11,11,21,21,26,26,36,36,10,10,14,19,28,29
        db 31,33,37,37,9,9,12,13,20,20,22,22,25,25,27,27
        db 30,30,34,35,38,38,0,8,23,24,39,47
low_cat200_odd_counts:
        db 0,0,2,4,0,3,4,7,5,5,3,2,4,8,8,8,5,2
        db 3,5,6,11,6,2,2,2,3,4,5,2,3,1,5,5,5,2
        db 0,5,4,3,7,2,3,1,2,5,5,2,3,2,1,7,5,2
        db 2,0,2,5,5,2,2,1,5,4,4,2,1,2,2,4,4,2
        db 2,3,4,4,4,2,1,3,3,4,3,2,2,4,4,3,3,2
        db 2,8,9,7,2,2,3,3,6,2,3,2,4,2,3,6,4,2
        db 4,3,2,3,3,2,3,4,6,4,6,2,6,7,5,9,5,2
        db 6,4,6,8,7,2,4,7,7,5,4,2,3,7,3,5,2,2
        db 0,0,3,5,0,3
low_cat200_odd_spans:
        db 8,10,37,39,7,7,11,11,36,36,40,40,0,6,12,35
        db 41,47,6,6,14,14,33,33,41,41,5,5,9,9,13,13
        db 15,15,32,32,34,34,37,39,7,7,12,12,35,35,40,40
        db 42,42,8,8,10,11,16,16,31,31,36,36,1,4,17,30
        db 43,46,0,0,47,47,5,5,18,18,29,29,42,42,10,11
        db 15,15,17,17,19,19,28,28,30,30,32,32,36,37,12,12
        db 14,14,16,16,20,23,25,27,31,31,33,33,35,35,4,4
        db 6,6,9,9,24,24,34,34,38,38,41,41,43,43,1,3
        db 7,8,13,13,39,40,44,46,0,0,47,47,14,14,33,33
        db 42,42,5,5,13,13,20,21,26,27,34,34,11,11,19,19
        db 28,28,32,32,36,36,41,41,4,4,6,6,10,10,12,12
        db 15,15,18,18,22,25,29,29,35,35,37,37,43,43,1,3
        db 7,9,16,17,30,31,38,40,44,46,0,0,47,47,15,15
        db 32,32,5,6,41,41,14,14,33,33,42,42,12,13,16,16
        db 31,31,34,35,1,4,7,11,17,30,36,40,43,46,0,0
        db 47,47,6,6,14,14,33,33,41,41,5,5,13,13,15,15
        db 32,32,34,34,7,7,12,12,35,35,40,40,42,42,1,4
        db 8,11,16,31,36,39,43,46,0,0,47,47,6,6,12,12
        db 33,33,35,35,41,41,7,7,13,14,34,34,40,40,5,5
        db 20,20,27,27,1,4,8,11,15,19,21,26,28,32,36,39
        db 42,46,0,0,47,47,7,7,20,20,27,27,40,40,6,6
        db 41,41,19,19,21,21,26,26,28,28,37,39,1,5,8,18
        db 22,25,29,36,42,46,0,0,47,47,20,20,27,27,40,40
        db 7,7,39,39,8,8,6,6,19,19,21,21,26,26,28,28
        db 37,38,41,41,1,5,9,18,22,25,29,36,42,46,0,0
        db 47,47,8,8,39,39,38,38,40,40,7,7,9,9,19,21
        db 26,28,36,37,1,6,10,18,22,25,29,35,41,46,0,0
        db 47,47,8,9,38,39,37,37,4,7,10,13,23,24,34,36
        db 40,43,14,16,22,22,25,25,30,33,1,3,17,21,26,29
        db 44,46,0,0,47,47,22,25,4,10,37,42,35,36,43,43
        db 11,17,21,21,26,26,29,34,1,3,18,20,27,28,44,46
        db 0,0,47,47,10,10,36,37,11,17,23,24,29,35,7,9
        db 18,18,25,25,38,40,4,6,22,22,28,28,41,43,1,3
        db 19,21,26,27,44,46,0,0,47,47,35,35,7,12,14,15
        db 36,40,16,18,29,29,33,34,3,6,13,13,30,32,41,43
        db 1,2,19,28,44,46,0,0,47,47,11,14,32,35,10,10
        db 15,16,30,31,36,37,9,9,17,17,29,29,38,38,8,8
        db 26,28,39,39,1,7,18,25,40,46,0,0,47,47,16,16
        db 31,31,5,6,9,9,15,15,17,17,29,30,32,32,38,38
        db 41,41,7,7,10,11,13,13,18,19,26,28,34,34,36,37
        db 40,40,42,42,8,8,12,12,14,14,20,25,33,33,35,35
        db 39,39,1,4,43,46,0,0,47,47,30,30,35,35,39,39
        db 17,17,29,29,38,38,16,16,26,28,31,31,34,34,36,36
        db 40,40,18,25,37,37,1,15,32,33,41,46,0,0,47,47
        db 15,15,32,32,34,34,40,40,31,31,39,39,16,16,29,30
        db 35,35,14,14,17,17,27,28,33,33,38,38,41,41,1,13
        db 18,26,36,37,42,46,0,0,47,47,14,14,33,33,35,35
        db 38,39,32,32,34,34,36,37,31,31,40,40,13,13,15,15
        db 29,30,1,12,16,28,41,46,0,0,47,47,4,6,33,33
        db 41,43,7,7,14,14,35,38,40,40,3,3,8,8,13,13
        db 32,32,34,34,44,44,19,20,28,28,31,31,39,39,1,2
        db 9,12,15,18,21,27,29,30,45,46,0,0,47,47,3,3
        db 10,11,13,14,33,33,36,37,44,44,7,7,9,9,12,12
        db 32,32,34,35,38,38,40,40,4,4,19,20,27,27,39,39
        db 43,43,2,2,5,6,8,8,15,15,21,21,28,28,31,31
        db 41,42,45,45,1,1,16,18,22,26,29,30,46,46,0,0
        db 47,47,5,5,15,15,21,21,26,26,32,32,42,42,4,4
        db 20,20,41,41,43,44,3,3,6,6,14,14,27,27,31,31
        db 33,33,7,7,11,13,19,19,22,23,28,28,30,30,34,36
        db 40,40,1,2,8,10,16,18,24,25,29,29,37,39,45,46
        db 0,0,47,47,7,8,21,21,32,32,39,40,5,5,15,16
        db 22,23,25,26,31,31,38,38,42,42,6,6,9,9,20,20
        db 24,24,29,30,37,37,41,41,10,10,14,14,19,19,27,28
        db 33,36,1,4,11,13,17,18,43,46,0,0,47,47,12,17
        db 20,20,27,35,11,11,18,19,21,21,23,24,26,26,36,36
        db 39,39,8,8,10,10,37,37,9,9,22,22,25,25,38,38
        db 40,40,1,7,41,46,0,0,47,47,14,19,28,29,31,33
        db 12,13,20,20,27,27,30,30,34,35,0,11,21,26,36,47

%endif

low_text_top       db ' NEON RELAY 4 :: GEOMETRIC SOLID                               FRAME=',0
low_text_video16       db 'GRCG ANALOG 16-COLOR / 640x400 / PAGE FLIP',0
low_text_video16_egc   db 'GRCG+EGC ANALOG 16-COLOR / 640x400 / PAGE FLIP',0
low_text_video200      db 'GRCG ANALOG 16-COLOR / 640x200 / LOGICAL 400->200',0
low_text_video200_egc  db 'GRCG+EGC ANALOG 16-COLOR / 640x200 / LOGICAL 400->200',0
low_text_video400      db 'GRCG ANALOG 16-COLOR / 640x400 / 80286',0
low_text_video400_egc  db 'GRCG+EGC ANALOG 16-COLOR / 640x400 / 80286',0
low_text_cpu386    db 'CPU: 386+ / 256-SCENE GEOMETRY PORT',0
low_text_cpu286    db 'CPU: 80286 / 256-SCENE GEOMETRY / 200 OR 400 LINES',0
low_qx0 dw 0
low_qy0 dw 0
low_qx1 dw 0
low_qy1 dw 0
low_qx2 dw 0
low_qy2 dw 0
low_qx3 dw 0
low_qy3 dw 0
low_cube_fx0 dw 0
low_cube_fy0 dw 0
low_cube_fx1 dw 0
low_cube_fy1 dw 0
low_cube_bx0 dw 0
low_cube_by0 dw 0
low_cube_bx1 dw 0
low_cube_by1 dw 0


; 0.27 faithful 3-D / raster work state
low_geo_xr dw 0
low_geo_zr dw 0
low_geo_yr dw 0
low_geo_z  dw 0
low_geo_recip dw 0
low_cross1_lo dw 0
low_cross1_hi dw 0
low_wire_pass db 0
align 2
low_wire_x0 dw 0
low_wire_y0 dw 0
low_wire_x1 dw 0
low_wire_y1 dw 0
low_wire_z0 dw 0
low_wire_z1 dw 0
low_wire_mx dw 0
low_wire_my dw 0
low_raster_x dw 0
low_raster_phase db 0
