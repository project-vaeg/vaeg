; Copyright (c) 2026 Nakata Maho
; Ported By Maho Nakata.
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the BSD 2-Clause conditions apply.

; ---------------------------------------------------------------------------
; SCENE4_256.INC - NEON 4 / GEOMETRIC SOLID chapter selection
; ---------------------------------------------------------------------------

select_scene:
        pusha
        mov     [city_global_frame], ax
        mov     [scene_frame], ax
        xor     bx, bx
        mov     si, scene_length_table
.select_loop:
        cmp     bx, SCENE_COUNT-1
        jae     .selected
        cmp     ax, [si]
        jb      .selected
        sub     ax, [si]
        mov     [scene_frame], ax
        add     si, 2
        inc     bx
        jmp     .select_loop
.selected:
        cmp     bl, [scene_index]
        je      .state_ready
        ; NEON4 camera motion is analytically derived from logical time.  Reset
        ; the old persistent smoother at chapter boundaries so AFS cannot carry
        ; an interior-space camera into the next material study.
        mov     byte [city_camera_initialized], 0
.state_ready:
        mov     [scene_index], bl
        popa
        ret

render_scene:
        pusha
        xor     bx, bx
        mov     bl, [scene_index]
        shl     bx, 1
        call    word [scene_routines + bx]
        popa
        ret

; Kept for scheduler compatibility.  Every NEON4 camera is a direct function of
; scene_frame, so skipped renders need no invisible camera replay.
city_camera_catch_up:
        ret

scene_length_table:
        dw      N4_SCENE0_FRAMES,N4_SCENE1_FRAMES,N4_SCENE2_FRAMES,N4_SCENE3_FRAMES
        dw      N4_SCENE4_FRAMES,N4_SCENE5_FRAMES,N4_SCENE6_FRAMES,N4_SCENE7_FRAMES

scene_routines:
        dw      scene4_solid_primitives
        dw      scene4_facet_rotation
        dw      scene4_pattern_cube
        dw      scene4_morphing_solid
        dw      scene4_shift_blitter
        dw      scene4_ribbon_wave
        dw      scene4_checker_plane
        dw      scene4_material_finale
