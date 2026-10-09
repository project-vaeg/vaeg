; Copyright (c) 2026 Nakata Maho
; Ported By Maho Nakata.
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the BSD 2-Clause conditions apply.

; Per-frame renderer shared by NEON4_16 and NEON4286.
        mov     ax, [frame_counter]
        mov     [render_frame_counter], ax
        mov     ax, [render_frame_counter]
        call    select_scene
        mov     al, [draw_page]
        call    set_access_page
        call    clear_graphics_frame16
        call    render_scene
        call    low_dirty_span_frame_end
        call    text_update
