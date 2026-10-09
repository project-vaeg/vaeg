; Copyright (c) 2026 Nakata Maho
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
; 1. Redistributions of source code must retain the above copyright
;    notice, this list of conditions and the following disclaimer.
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
; USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
; ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
; THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

bits 16
org 0x100

%ifndef M7_VARIANT
%define M7_VARIANT              1
%endif

%ifndef M7_SCROLL
%define M7_SCROLL               0
%endif

%define KEYBOARD_BIOS_INT       0x82
%define CALENDAR_BIOS_INT       0x8c
%define VIDEO_BIOS_INT          0x8f
%define VIDEO_BIOS_DATA_SEG     0x0338
%define VIDEO_MODE_OFFSET       0x000f
%define VIDEO_G0_BPP_OFFSET     0x0011
%define VIDEO_G1_BPP_OFFSET     0x0012

%define PORT_MEMORY_MAP         0x0153
%define PORT_G0_TRANSPARENCY    0x0124
%define PORT_G1_TRANSPARENCY    0x0126
%define PORT_FB1_DSA_LOW        0x022e
%define PORT_FB1_DSA_HIGH        0x0230
%define PORT_TSP_STATUS         0x0142
%define PORT_SGP_COMMAND        0x0500
%define PORT_SGP_CONTROL        0x0504
%define PORT_SGP_STATUS         0x0506
%define PORT_GVRAM_WRITE_MODE   0x0580

%define MEMORY_MAP_GVRAM_SINGLE 0x54
%define GVRAM_CPU_WRITE_MODE    0x10

%define MODE_320X200_G0_G1      0xe00e
%define PIXEL_SIZE_G0_G1_4BPP   0x0404
%define COMPOSE_G1_OVER_G0      0x0034
%define TSP_STATUS_VBLANK       0x40

%define G0_SEGMENT              0xa000
%define SCREEN_WIDTH            320
%define SCREEN_HEIGHT           200
%define SCREEN_PITCH            160

%define SGP_COMMAND_END         0x0001
%define SGP_COMMAND_SET_WORK    0x0003
%define SGP_COMMAND_SET_SOURCE  0x0004
%define SGP_COMMAND_SET_DEST    0x0005
%define SGP_COMMAND_SET_COLOR   0x0006
%define SGP_COMMAND_BITBLT      0x0007
%define SGP_COMMAND_PATBLT      0x0008
%define SGP_COMMAND_CLS         0x000a
%define SGP_BITBLT_COPY_XPAR    0x0105
%define SGP_PATBLT_COPY         0x0005
%define SGP_BUSY                0x01

%define BALL_COUNT              16
%define BULLET_COUNT            32
%define SPRITE_INITIAL_COUNT    BALL_COUNT
%define SPRITE_MIN_COUNT        1
%define SPRITE_MAX_COUNT        256
%define BULLET_FIRST_INDEX      BALL_COUNT
%define SPRITE_WIDTH            24
%define SPRITE_HEIGHT           24
%define SPRITE_PITCH            12
%define SPRITE_BITMAP_BYTES     (SPRITE_PITCH * SPRITE_HEIGHT)
%define BULLET_WIDTH            8
%define BULLET_HEIGHT           8
%define BULLET_PITCH            4
%define BULLET_BITMAP_BYTES     (BULLET_PITCH * BULLET_HEIGHT)
%define FPS_GLYPH_COUNT         11
%define FPS_GLYPH_WIDTH         4
%define FPS_GLYPH_HEIGHT        7
%define FPS_GLYPH_PITCH         2
%define FPS_GLYPH_X             260
%define FPS_GLYPH_Y             4
%define FPS_GLYPH_ADVANCE       5
%define G1_PAGE_A_SGP_BASE      0x220000
%define G1_PAGE_B_SGP_BASE      0x227d00
%define G1_PAGE_A_DSA           0x020000
%define G1_PAGE_B_DSA           0x027d00
%define SCREEN_WORD_COUNT       0x3e80
%if M7_VARIANT >= 2
%define DIRTY_RECT_MAX_COUNT    (SPRITE_MAX_COUNT * 2 + 1)
%define SGP_COMMAND_WORD_COUNT  (13 + DIRTY_RECT_MAX_COUNT * 9 + (SPRITE_MAX_COUNT + FPS_GLYPH_COUNT) * 16)
%else
%define SGP_COMMAND_WORD_COUNT  (11 + (SPRITE_MAX_COUNT + FPS_GLYPH_COUNT) * 16)
%endif
%define FPS_SAMPLE_FRAMES       60
%define BALL_PIXEL_COUNT        (SPRITE_WIDTH * SPRITE_HEIGHT)
%define BALL_SOURCE_BYTES       SPRITE_BITMAP_BYTES
%define BULLET_PIXEL_COUNT      (BULLET_WIDTH * BULLET_HEIGHT)
%define BULLET_SOURCE_BYTES     BULLET_BITMAP_BYTES

%define RGB565(r,g,b)           ((((g) & 0x3f) << 10) | (((r) & 0x1f) << 5) | ((b) & 0x1f))
%define BALL_PAIR(a,b)          ((((a) & 0x0f) << 4) | ((b) & 0x0f))

; Pre-rendered 24x24 sphere raster: D=shadow, A=base hue,
; B=adjacent hue, F=highlight, and zero=transparent.
%macro DEFINE_HSV_BALL 3
%1:
    db BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 13), BALL_PAIR(13, 13), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(13, 13), BALL_PAIR(13, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(13, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%3, 13), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(13, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %3), BALL_PAIR(%3, 13), BALL_PAIR(0, 0), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(0, 13), BALL_PAIR(%2, %2), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%3, %3), BALL_PAIR(13, 0), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(0, %2), BALL_PAIR(%2, 15), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(15, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%3, %3), BALL_PAIR(13, 0), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(13, %2), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%3, %3), BALL_PAIR(%3, 13), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 13), BALL_PAIR(%2, %2), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%3, %3), BALL_PAIR(%3, 13), BALL_PAIR(13, 0)
    db BALL_PAIR(0, 13), BALL_PAIR(%2, %2), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%3, %3), BALL_PAIR(%3, 13), BALL_PAIR(13, 0)
    db BALL_PAIR(0, 13), BALL_PAIR(%2, %2), BALL_PAIR(%2, 15), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(15, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, 13), BALL_PAIR(13, 0)
    db BALL_PAIR(0, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, 15), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(15, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %3), BALL_PAIR(%3, %3), BALL_PAIR(13, 13), BALL_PAIR(13, 0)
    db BALL_PAIR(0, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, 15), BALL_PAIR(15, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(13, 13), BALL_PAIR(13, 0)
    db BALL_PAIR(0, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%3, %3), BALL_PAIR(%3, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 0)
    db BALL_PAIR(0, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 0)
    db BALL_PAIR(0, 13), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 0)
    db BALL_PAIR(0, 13), BALL_PAIR(%3, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 0)
    db BALL_PAIR(0, 13), BALL_PAIR(%3, %3), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(13, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(0, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 0), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(0, 13), BALL_PAIR(13, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 0), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(0, 0), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0)
%endmacro

%define SPRITE_X_OFFSET         0
%define SPRITE_Y_OFFSET         2
%define SPRITE_VX_OFFSET        4
%define SPRITE_VY_OFFSET        6
%define SPRITE_WIDTH_OFFSET     8
%define SPRITE_HEIGHT_OFFSET    10
%define SPRITE_PITCH_OFFSET     12
%define SPRITE_BITMAP_OFFSET    14
%define SPRITE_PRIORITY_OFFSET  16
%define SPRITE_RECORD_SIZE      18

start:
    push cs
    pop ds
    cld

    call save_video_state
%if M7_VARIANT >= 4
    call initialize_m7d_tables
%endif

    mov dx, message_start
    call print_string

    call initialize_video
    jc initialization_failed
    call initialize_fps_counter
    call initialize_m6_counters

%if M7_VARIANT >= 3
    mov byte [current_command_index], 0
    mov byte [build_command_index], 1
    call start_sgp_command_list
    jc animation_failed
    mov byte [pipeline_active], 1

pipeline_loop:
    call poll_keyboard
    jc animation_done

    push cs
    pop ds
    call update_sprite_positions

    ; Build the next list for the page currently on display.  Restore the
    ; current SGP page before the synchronized flip.
    xor byte [draw_page_index], 1
    call set_draw_page_base
    call build_sgp_frame_commands
    xor byte [draw_page_index], 1
    call set_draw_page_base

    call wait_sgp_idle
    jc animation_failed
    call flip_draw_page
    jc animation_failed
    call update_fps_counter
    call record_completed_frame

    mov al, [build_command_index]
    mov [current_command_index], al
    xor byte [build_command_index], 1
    call start_sgp_command_list
    jc animation_failed
    jmp pipeline_loop
%else
animation_loop:
    call poll_keyboard
    jc animation_done

    push cs
    pop ds
    call update_sprite_positions
%if M7_SCROLL
    call update_scroll_background
%endif
    call render_sprite_frame
    jc animation_failed
    call update_fps_counter
    call flip_draw_page
    jc animation_failed
    call record_completed_frame
    jmp animation_loop
%endif

animation_done:
%if M7_VARIANT >= 3
    cmp byte [pipeline_active], 0
    je .no_pipeline_wait
    call wait_sgp_idle
    mov byte [pipeline_active], 0
.no_pipeline_wait:
%endif
    call restore_video_state
    call print_m6_summary
    mov dx, message_done
    call print_string
    mov ax, 0x4c00
    int 0x21

initialization_failed:
    call restore_video_state
    mov dx, message_failed
    call print_string
    mov ax, 0x4c01
    int 0x21

animation_failed:
%if M7_VARIANT >= 3
    cmp byte [pipeline_active], 0
    je .no_pipeline_wait
    call wait_sgp_idle
    mov byte [pipeline_active], 0
.no_pipeline_wait:
%endif
    call restore_video_state
    call print_m6_summary
    mov dx, message_animation_failed
    call print_string
    mov ax, 0x4c02
    int 0x21

set_draw_page_base:
    push ax
    cmp byte [draw_page_index], 0
    jne .page_b

    mov ax, G1_PAGE_A_SGP_BASE & 0xffff
    mov [draw_page_sgp_low], ax
    mov ax, G1_PAGE_A_SGP_BASE >> 16
    mov [draw_page_sgp_high], ax
    mov ax, G1_PAGE_A_DSA & 0xffff
    mov [draw_page_dsa_low], ax
    mov ax, G1_PAGE_A_DSA >> 16
    mov [draw_page_dsa_high], ax
    pop ax
    ret

.page_b:
    mov ax, G1_PAGE_B_SGP_BASE & 0xffff
    mov [draw_page_sgp_low], ax
    mov ax, G1_PAGE_B_SGP_BASE >> 16
    mov [draw_page_sgp_high], ax
    mov ax, G1_PAGE_B_DSA & 0xffff
    mov [draw_page_dsa_low], ax
    mov ax, G1_PAGE_B_DSA >> 16
    mov [draw_page_dsa_high], ax
    pop ax
    ret

; DSA1 is a pair of word registers; byte writes can hang real hardware.
set_display_page_from_draw:
    push ax
    push dx
    mov dx, PORT_FB1_DSA_LOW
    mov ax, [draw_page_dsa_low]
    out dx, ax
    add dx, 2
    mov ax, [draw_page_dsa_high]
    out dx, ax
    pop dx
    pop ax
    ret

flip_draw_page:
    call wait_vblank_start
    jc .failed
    call set_display_page_from_draw
    inc word [m6_page_flips_lo]
    jnz .page_flip_count_ready
    inc word [m6_page_flips_hi]
.page_flip_count_ready:
    xor byte [draw_page_index], 1
    call set_draw_page_base
    clc
    ret

.failed:
    stc
    ret
initialize_video:
    mov bx, MODE_320X200_G0_G1
    mov cx, PIXEL_SIZE_G0_G1_4BPP
    xor dx, dx
    mov byte [video_mode_changed], 1
    xor ax, ax
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed

    mov ax, 0x0b00
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed

    mov ax, 0x0900
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed

    mov ax, 0x0a00
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed

    call initialize_sprite_palette
    jc .failed

    mov ax, 0x0300
    mov cx, COMPOSE_G1_OVER_G0
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed

    call verify_default_buffers
    jc .failed

    mov dx, PORT_G0_TRANSPARENCY
    xor ax, ax
    out dx, ax

    mov dx, PORT_G1_TRANSPARENCY
    mov ax, 0x0001
    out dx, ax

    mov dx, PORT_MEMORY_MAP
    mov al, MEMORY_MAP_GVRAM_SINGLE
    out dx, al

    mov dx, PORT_GVRAM_WRITE_MODE
    mov al, GVRAM_CPU_WRITE_MODE
    out dx, al

    push cs
    pop ds
    call draw_g0_checkerboard
    mov byte [draw_page_index], 0
%if M7_VARIANT >= 3
    mov byte [build_command_index], 0
    mov byte [current_command_index], 0
%endif
    call set_draw_page_base
    call render_sprite_frame
    jc .failed

    mov byte [draw_page_index], 1
%if M7_VARIANT >= 3
    mov byte [build_command_index], 1
    mov byte [current_command_index], 1
%endif
    call set_draw_page_base
    call render_sprite_frame
    jc .failed
%if M7_VARIANT >= 3
    mov byte [current_command_index], 0
    mov byte [build_command_index], 1
%endif

    call set_display_page_from_draw
    xor byte [draw_page_index], 1
    call set_draw_page_base

    mov ax, 0x0b01
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed

    clc
    ret

.failed:
    stc
    ret

initialize_sprite_palette:
    push bx
    push si
    xor bx, bx
    mov si, palette_values

.next_entry:
    mov ax, 0x0800
    mov al, bl
    mov cx, [si]
    push bx
    push si
    call set_palette_entry
    pop si
    pop bx
    jc .failed

    add si, 2
    inc bx
    cmp bx, 16
    jb .next_entry

    pop si
    pop bx
    clc
    ret

.failed:
    pop si
    pop bx
    stc
    ret

set_palette_entry:
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    clc
    ret

.failed:
    stc
    ret

verify_default_buffers:
    xor cx, cx
    mov ax, 0x0700
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    call verify_buffer_geometry
    jc .failed

    xor cx, cx
    mov ax, 0x0701
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    call verify_buffer_geometry
    ret

.failed:
    stc
    ret

verify_buffer_geometry:
    cmp word [es:di + 0], 4
    jne .failed
    cmp word [es:di + 2], SCREEN_WIDTH
    jne .failed
    cmp word [es:di + 4], SCREEN_HEIGHT
    jne .failed
    clc
    ret

.failed:
    stc
    ret

draw_g0_checkerboard:
    mov ax, G0_SEGMENT
    mov es, ax
    xor di, di
    xor bx, bx

.row:
    mov al, 0xdd
    test bl, 0x10
    jz .tile_row_ready
    mov al, 0xee

.tile_row_ready:
    mov cx, 20

.tile:
    mov ah, al
    push cx
    mov cx, 4
    rep stosw
    pop cx
    xor al, 0x33
    loop .tile

    inc bx
    cmp bx, SCREEN_HEIGHT
    jne .row
    ret

%if M7_SCROLL
; Update three independently phased checkerboard bands.  The phase counters
; advance at 3, 7, and 11 internal byte units, while the row writer rounds
; each visible source offset to the nearest eight-byte checker boundary. The
; G0 aperture is
; written with word transfers so the scroll background never uses byte writes
; to a hardware word port.
update_scroll_background:
    push ax
    push bx
    push cx
    push dx

    add word [scroll_phase_top], 3
    and word [scroll_phase_top], 0x000f
    add word [scroll_phase_middle], 7
    and word [scroll_phase_middle], 0x000f
    add word [scroll_phase_bottom], 11
    and word [scroll_phase_bottom], 0x000f

    xor bx, bx
    mov cx, 66
    mov dx, [scroll_phase_top]
    call draw_scroll_band

    mov bx, 66
    mov cx, 67
    mov dx, [scroll_phase_middle]
    call draw_scroll_band

    mov bx, 133
    mov cx, 67
    mov dx, [scroll_phase_bottom]
    call draw_scroll_band

    pop dx
    pop cx
    pop bx
    pop ax
    ret

; BX=start row, CX=row count, DX=unrounded phase.  The pattern source is
; copied to a temporary row with an offset rounded to the nearest checker
; boundary (eight bytes, or sixteen dots), then transferred to G0 as words.
draw_scroll_band:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp

    mov [scroll_work_y], bx
    mov [scroll_work_phase], dx
    mov [scroll_work_rows], cx
.row:
    mov bp, [scroll_work_y]
    test bp, 0x0010
    jz .dark_row
    mov si, scroll_pattern_light
    jmp .pattern_selected
.dark_row:
    mov si, scroll_pattern_dark
.pattern_selected:
    mov ax, [scroll_work_phase]
    add ax, 4
    and ax, 0x0008
    add si, ax

    push es
    push ds
    pop es
    mov di, scroll_row_buffer
    mov cx, SCREEN_PITCH
    rep movsb
    pop es

    mov ax, bp
    mov bx, SCREEN_PITCH
    mul bx
    mov di, ax
    mov si, scroll_row_buffer
    mov cx, SCREEN_PITCH / 2
    rep movsw

    inc word [scroll_work_y]
    dec word [scroll_work_rows]
    jnz .row

    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret
%endif


poll_keyboard:
    mov ah, 0x0a
    int KEYBOARD_BIOS_INT
    jc .no_key

    mov ah, 0x09
    int KEYBOARD_BIOS_INT
    cmp ah, 0x00
    je .escape
    cmp ah, 0x3a
    je .increase_count
    cmp ah, 0x3d
    je .decrease_count
    cmp al, '+'
    je .increase_count
    cmp al, '-'
    je .decrease_count

.no_key:
    clc
    ret

.increase_count:
    push cs
    pop ds
    cmp word [active_sprite_count], SPRITE_MAX_COUNT
    jae .no_key
    inc word [active_sprite_count]
    call format_fps_glyphs
    clc
    ret

.decrease_count:
    push cs
    pop ds
    cmp word [active_sprite_count], SPRITE_MIN_COUNT
    jbe .no_key
    dec word [active_sprite_count]
    call format_fps_glyphs
    clc
    ret

.escape:
    stc
    ret

initialize_fps_counter:
    push ax
    push bx
    push cx
    push dx
    push si

    push cs
    pop ds
    mov word [fps_frame_counter], 0
    mov word [fps_value], 0
    mov ah, 0x02
    int CALENDAR_BIOS_INT
    push cs
    pop ds
    mov [fps_last_second], dh
    call format_fps_glyphs

    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

update_fps_counter:
    push ax
    push bx
    push cx
    push dx
    push si

    push cs
    pop ds
    inc word [fps_frame_counter]
    cmp word [fps_frame_counter], FPS_SAMPLE_FRAMES
    jb .done

    ; Calendar BIOS is sampled once per 60 completed frames, never per frame.
    mov ah, 0x02
    int CALENDAR_BIOS_INT
    push cs
    pop ds
    mov al, dh
    sub al, [fps_last_second]
    jnc .delta_ready
    add al, 60
.delta_ready:
    xor ah, ah
    cmp ax, 0
    jne .seconds_ready
    mov ax, 1
.seconds_ready:
    mov [fps_last_second], dh
    mov bx, ax
    mov ax, FPS_SAMPLE_FRAMES
    xor dx, dx
    div bx
    mov [fps_value], ax
    mov word [fps_frame_counter], 0
    call format_fps_glyphs

.done:
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

initialize_m6_counters:
    xor ax, ax
    mov [m6_frames_lo], ax
    mov [m6_frames_hi], ax
    mov [m6_page_flips_lo], ax
    mov [m6_page_flips_hi], ax
    mov [m6_sgp_commands_frame], ax
    mov [m6_sgp_bitblts_frame], ax
    mov [m6_sgp_pixels_frame_lo], ax
    mov [m6_sgp_pixels_frame_hi], ax
    mov [m6_sgp_source_bytes_frame_lo], ax
    mov [m6_sgp_source_bytes_frame_hi], ax
    mov [m6_total_commands_lo], ax
    mov [m6_total_commands_hi], ax
    mov [m6_total_bitblts_lo], ax
    mov [m6_total_bitblts_hi], ax
    mov [m6_total_pixels_lo], ax
    mov [m6_total_pixels_hi], ax
    mov [m6_total_source_bytes_lo], ax
    mov [m6_total_source_bytes_hi], ax
    mov [m6_missed_vblank_lo], ax
    mov [m6_missed_vblank_hi], ax
%if M7_VARIANT >= 2
    mov [m7_dirty_rects_frame], ax
    mov [m7_dirty_pixels_frame_lo], ax
    mov [m7_dirty_pixels_frame_hi], ax
    mov [m7_full_clears_frame], ax
%endif
    ret

record_completed_frame:
    inc word [m6_frames_lo]
    jnz .ready
    inc word [m6_frames_hi]
.ready:
    ret

m6_accumulate_frame_counters:
    push ax
    push dx

    mov ax, [m6_sgp_commands_frame]
    add [m6_total_commands_lo], ax
    adc word [m6_total_commands_hi], 0

    mov ax, [m6_sgp_bitblts_frame]
    add [m6_total_bitblts_lo], ax
    adc word [m6_total_bitblts_hi], 0

    mov ax, [m6_sgp_pixels_frame_lo]
    mov dx, [m6_sgp_pixels_frame_hi]
    add [m6_total_pixels_lo], ax
    adc [m6_total_pixels_hi], dx

    mov ax, [m6_sgp_source_bytes_frame_lo]
    mov dx, [m6_sgp_source_bytes_frame_hi]
    add [m6_total_source_bytes_lo], ax
    adc [m6_total_source_bytes_hi], dx

    pop dx
    pop ax
    ret

print_m6_summary:
    push ax
    push dx

    mov dx, message_m6_summary
    call print_string
    mov dx, label_m6_frames
    call print_string
    mov ax, [m6_frames_lo]
    mov dx, [m6_frames_hi]
    call print_u32
    mov dx, label_m6_page_flips
    call print_string
    mov ax, [m6_page_flips_lo]
    mov dx, [m6_page_flips_hi]
    call print_u32
    mov dx, label_m6_last_commands
    call print_string
    mov ax, [m6_sgp_commands_frame]
    xor dx, dx
    call print_u32
    mov dx, label_m6_last_bitblts
    call print_string
    mov ax, [m6_sgp_bitblts_frame]
    xor dx, dx
    call print_u32
    mov dx, label_m6_last_pixels
    call print_string
    mov ax, [m6_sgp_pixels_frame_lo]
    mov dx, [m6_sgp_pixels_frame_hi]
    call print_u32
    mov dx, label_m6_last_bytes
    call print_string
    mov ax, [m6_sgp_source_bytes_frame_lo]
    mov dx, [m6_sgp_source_bytes_frame_hi]
    call print_u32
    mov dx, label_m6_total_commands
    call print_string
    mov ax, [m6_total_commands_lo]
    mov dx, [m6_total_commands_hi]
    call print_u32
    mov dx, label_m6_total_bitblts
    call print_string
    mov ax, [m6_total_bitblts_lo]
    mov dx, [m6_total_bitblts_hi]
    call print_u32
    mov dx, label_m6_total_pixels
    call print_string
    mov ax, [m6_total_pixels_lo]
    mov dx, [m6_total_pixels_hi]
    call print_u32
    mov dx, label_m6_total_bytes
    call print_string
    mov ax, [m6_total_source_bytes_lo]
    mov dx, [m6_total_source_bytes_hi]
    call print_u32
    mov dx, label_m6_active
    call print_string
    mov ax, [active_sprite_count]
    xor dx, dx
    call print_u32
    mov dx, label_m6_missed
    call print_string
    mov ax, [m6_missed_vblank_lo]
    mov dx, [m6_missed_vblank_hi]
    call print_u32
%if M7_VARIANT >= 2
    mov dx, label_m7_dirty_rects
    call print_string
    mov ax, [m7_dirty_rects_frame]
    xor dx, dx
    call print_u32
    mov dx, label_m7_dirty_pixels
    call print_string
    mov ax, [m7_dirty_pixels_frame_lo]
    mov dx, [m7_dirty_pixels_frame_hi]
    call print_u32
    mov dx, label_m7_full_clears
    call print_string
    mov ax, [m7_full_clears_frame]
    xor dx, dx
    call print_u32
%endif

    pop dx
    pop ax
    ret

print_u32:
    push ax
    push bx
    push cx
    push dx
    push si
    push di

    mov [m6_print_value_lo], ax
    mov [m6_print_value_hi], dx
    mov di, m6_number_buffer + 11
    mov byte [di], '$'
    dec di
    mov bx, 10
.next_digit:
    xor dx, dx
    mov ax, [m6_print_value_hi]
    div bx
    mov [m6_print_value_hi], ax
    mov ax, [m6_print_value_lo]
    div bx
    mov [m6_print_value_lo], ax
    add dl, '0'
    mov [di], dl
    dec di
    cmp word [m6_print_value_hi], 0
    jne .next_digit
    cmp word [m6_print_value_lo], 0
    jne .next_digit
    inc di
    mov dx, di
    mov ah, 0x09
    int 0x21

    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

format_fps_glyphs:
    push ax
    push bx
    push dx
    push si

    mov ax, [fps_value]
    cmp ax, 999
    jbe .value_ready
    mov ax, 999

.value_ready:
    xor dx, dx
    mov bx, 100
    div bx
    mov si, ax
    shl si, 1
    mov bx, [digit_glyph_table + si]
    mov [fps_glyph_pointers + 6], bx

    mov ax, dx
    xor dx, dx
    mov bx, 10
    div bx
    mov si, ax
    shl si, 1
    mov bx, [digit_glyph_table + si]
    mov [fps_glyph_pointers + 8], bx

    mov si, dx
    shl si, 1
    mov bx, [digit_glyph_table + si]
    mov [fps_glyph_pointers + 10], bx

    mov ax, [active_sprite_count]
    xor dx, dx
    mov bx, 1000
    div bx
    mov si, ax
    shl si, 1
    mov bx, [digit_glyph_table + si]
    mov [fps_glyph_pointers + 14], bx

    mov ax, dx
    xor dx, dx
    mov bx, 100
    div bx
    mov si, ax
    shl si, 1
    mov bx, [digit_glyph_table + si]
    mov [fps_glyph_pointers + 16], bx

    mov ax, dx
    xor dx, dx
    mov bx, 10
    div bx
    mov si, ax
    shl si, 1
    mov bx, [digit_glyph_table + si]
    mov [fps_glyph_pointers + 18], bx

    mov si, dx
    shl si, 1
    mov bx, [digit_glyph_table + si]
    mov [fps_glyph_pointers + 20], bx

    pop si
    pop dx
    pop bx
    pop ax
    ret

wait_vblank_start:
    mov dx, PORT_TSP_STATUS
    mov bx, 4

.wait_display_interval:
    mov cx, 0xffff

.poll_display_interval:
    in al, dx
    test al, TSP_STATUS_VBLANK
    jz .display_interval_seen
    loop .poll_display_interval
    dec bx
    jnz .wait_display_interval
    inc word [m6_missed_vblank_lo]
    jnz .display_timeout_count_ready
    inc word [m6_missed_vblank_hi]
.display_timeout_count_ready:
    stc
    ret

.display_interval_seen:
    mov bx, 4

.wait_vblank_interval:
    mov cx, 0xffff

.poll_vblank_interval:
    in al, dx
    test al, TSP_STATUS_VBLANK
    jnz .ready
    loop .poll_vblank_interval
    dec bx
    jnz .wait_vblank_interval
    inc word [m6_missed_vblank_lo]
    jnz .vblank_timeout_count_ready
    inc word [m6_missed_vblank_hi]
.vblank_timeout_count_ready:
    stc
    ret

.ready:
    clc
    ret

update_sprite_positions:
    push ax
    push cx
    push dx
    push si

    mov si, sprite_records
    mov cx, [active_sprite_count]

.next_sprite:
    mov ax, [si + SPRITE_X_OFFSET]
    add ax, [si + SPRITE_VX_OFFSET]
    test ax, 0x8000
    jz .check_x_maximum
    xor ax, ax
    neg word [si + SPRITE_VX_OFFSET]
    jmp .store_x

.check_x_maximum:
    mov dx, SCREEN_WIDTH
    sub dx, [si + SPRITE_WIDTH_OFFSET]
    cmp ax, dx
    jbe .store_x
    mov ax, dx
    neg word [si + SPRITE_VX_OFFSET]

.store_x:
    mov [si + SPRITE_X_OFFSET], ax

    mov ax, [si + SPRITE_Y_OFFSET]
    add ax, [si + SPRITE_VY_OFFSET]
    test ax, 0x8000
    jz .check_y_maximum
    xor ax, ax
    neg word [si + SPRITE_VY_OFFSET]
    jmp .store_y

.check_y_maximum:
    mov dx, SCREEN_HEIGHT
    sub dx, [si + SPRITE_HEIGHT_OFFSET]
    cmp ax, dx
    jbe .store_y
    mov ax, dx
    neg word [si + SPRITE_VY_OFFSET]

.store_y:
    mov [si + SPRITE_Y_OFFSET], ax
    add si, SPRITE_RECORD_SIZE
    loop .next_sprite

    pop si
    pop dx
    pop cx
    pop ax
    ret

; Compute logical transfer quantities once per frame from the fixed record order.
; Records are balls [0,16), bullets [16,48), then balls again.
update_frame_transfer_stats:
    push ax
    push bx
    push cx
    push dx

    mov ax, [active_sprite_count]
    xor dx, dx
    mov bx, ax
    cmp bx, BALL_COUNT
    jbe .balls_only
    mov bx, BALL_COUNT
.balls_only:
    mov cx, bx
    mov ax, BALL_PIXEL_COUNT
    mul cx
    mov [m6_sgp_pixels_frame_lo], ax
    mov [m6_sgp_pixels_frame_hi], dx
    mov ax, BALL_SOURCE_BYTES
    mul cx
    mov [m6_sgp_source_bytes_frame_lo], ax
    mov [m6_sgp_source_bytes_frame_hi], dx

    mov ax, [active_sprite_count]
    sub ax, BALL_COUNT
    jbe .glyphs
    mov bx, ax
    cmp bx, BULLET_COUNT
    jbe .bullet_count_ready
    mov bx, BULLET_COUNT
.bullet_count_ready:
    mov cx, bx
    mov ax, BULLET_PIXEL_COUNT
    mul cx
    add [m6_sgp_pixels_frame_lo], ax
    adc [m6_sgp_pixels_frame_hi], dx
    mov ax, BULLET_SOURCE_BYTES
    mul cx
    add [m6_sgp_source_bytes_frame_lo], ax
    adc [m6_sgp_source_bytes_frame_hi], dx

    mov ax, [active_sprite_count]
    sub ax, BALL_COUNT
    sub ax, BULLET_COUNT
    jbe .glyphs
    mov cx, ax
    mov ax, BALL_PIXEL_COUNT
    mul cx
    add [m6_sgp_pixels_frame_lo], ax
    adc [m6_sgp_pixels_frame_hi], dx
    mov ax, BALL_SOURCE_BYTES
    mul cx
    add [m6_sgp_source_bytes_frame_lo], ax
    adc [m6_sgp_source_bytes_frame_hi], dx

.glyphs:
    mov ax, FPS_GLYPH_COUNT
    add [m6_sgp_bitblts_frame], ax
    mov ax, FPS_GLYPH_COUNT * FPS_GLYPH_WIDTH * FPS_GLYPH_HEIGHT
    add [m6_sgp_pixels_frame_lo], ax
    adc word [m6_sgp_pixels_frame_hi], 0
    mov ax, FPS_GLYPH_COUNT * FPS_GLYPH_PITCH * FPS_GLYPH_HEIGHT
    add [m6_sgp_source_bytes_frame_lo], ax
    adc word [m6_sgp_source_bytes_frame_hi], 0

    pop dx
    pop cx
    pop bx
    pop ax
    ret

; M7b page-local dirty clear. The first pass records every old rectangle,
; the second pass records every current rectangle and updates that page cache.
; All current sprites are still redrawn in painter order after this clear.
%if M7_VARIANT >= 2
select_dirty_page:
    cmp byte [draw_page_index], 0
    jne .page_b
    mov word [dirty_page_old_x], page_last_x_a
    mov word [dirty_page_old_y], page_last_y_a
    mov word [dirty_page_new_x], page_last_x_a
    mov word [dirty_page_new_y], page_last_y_a
    mov word [dirty_page_active], page_last_active_a
    mov word [dirty_page_valid], page_last_valid_a
    ret
.page_b:
    mov word [dirty_page_old_x], page_last_x_b
    mov word [dirty_page_old_y], page_last_y_b
    mov word [dirty_page_new_x], page_last_x_b
    mov word [dirty_page_new_y], page_last_y_b
    mov word [dirty_page_active], page_last_active_b
    mov word [dirty_page_valid], page_last_valid_b
    ret

append_dirty_rect:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    mov si, [dirty_rect_count]
    cmp si, DIRTY_RECT_MAX_COUNT
    jae .done
    shl si, 1
    mov [dirty_rect_x + si], ax
    mov [dirty_rect_y + si], dx
    mov [dirty_rect_w + si], bx
    mov [dirty_rect_h + si], cx
    inc word [dirty_rect_count]
    mov ax, bx
    mul cx
    add [dirty_pixels_lo], ax
    adc [dirty_pixels_hi], dx
.done:
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

prepare_dirty_rects:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    call select_dirty_page
    xor ax, ax
    mov [dirty_rect_count], ax
    mov [dirty_pixels_lo], ax
    mov [dirty_pixels_hi], ax
    mov byte [dirty_full_clear], 0

    mov si, [dirty_page_valid]
    cmp byte [si], 0
    je .skip_old
    mov si, [dirty_page_old_x]
    mov di, [dirty_page_old_y]
    mov bp, sprite_records
    mov cx, [dirty_page_active]
.old_loop:
    jcxz .current
    mov ax, [si]
    mov dx, [di]
    mov bx, [bp + SPRITE_WIDTH_OFFSET]
    push cx
    mov cx, [bp + SPRITE_HEIGHT_OFFSET]
    call append_dirty_rect
    pop cx
    add si, 2
    add di, 2
    add bp, SPRITE_RECORD_SIZE
    loop .old_loop
    jmp .current

.skip_old:
    mov byte [dirty_full_clear], 1

.current:
    mov si, sprite_records
    mov di, [dirty_page_new_x]
    mov bp, [dirty_page_new_y]
    mov cx, [active_sprite_count]
.current_loop:
    jcxz .current_done
    mov ax, [si + SPRITE_X_OFFSET]
    mov dx, [si + SPRITE_Y_OFFSET]
    mov [di], ax
    mov [bp], dx
    mov bx, [si + SPRITE_WIDTH_OFFSET]
    push cx
    mov cx, [si + SPRITE_HEIGHT_OFFSET]
    call append_dirty_rect
    pop cx
    add si, SPRITE_RECORD_SIZE
    add di, 2
    add bp, 2
    loop .current_loop
.current_done:
    mov si, [dirty_page_active]
    mov ax, [active_sprite_count]
    mov [si], ax
    mov si, [dirty_page_valid]
    mov byte [si], 1

    mov ax, FPS_GLYPH_X
    mov dx, FPS_GLYPH_Y
    mov bx, FPS_GLYPH_COUNT * FPS_GLYPH_ADVANCE
    mov cx, FPS_GLYPH_HEIGHT
    call append_dirty_rect

    mov ax, [dirty_pixels_hi]
    cmp ax, 0
    jne .full
    cmp word [dirty_pixels_lo], SCREEN_WIDTH * SCREEN_HEIGHT
    jb .area_ready
.full:
    mov byte [dirty_full_clear], 1
.area_ready:
    mov ax, [dirty_rect_count]
    mov [m7_dirty_rects_frame], ax
    mov ax, [dirty_pixels_lo]
    mov [m7_dirty_pixels_frame_lo], ax
    mov ax, [dirty_pixels_hi]
    mov [m7_dirty_pixels_frame_hi], ax
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

emit_dirty_clear_commands:
    push ax
    push bx
    push cx
    push dx
    push si
    push bp
    mov ax, SGP_COMMAND_SET_SOURCE
    stosw
    mov ax, 0x0001
    stosw
    mov ax, 1
    stosw
    stosw
    mov ax, 2
    stosw
%if M7_VARIANT >= 4
    mov ax, [m7_zero_address_low]
    stosw
    mov ax, [m7_zero_address_high]
    stosw
%else
    mov si, sgp_zero_word
    call physical_address_from_ds_si
    stosw
    mov ax, dx
    stosw
%endif

    xor si, si
    mov cx, [dirty_rect_count]
.next_rect:
    jcxz .done
    mov ax, SGP_COMMAND_SET_DEST
    stosw
    mov ax, [dirty_rect_x + si]
    mov dx, ax
    and ax, 0x0003
    shl ax, 4
    or ax, 0x0001
    stosw
    mov ax, [dirty_rect_w + si]
    stosw
    mov ax, [dirty_rect_h + si]
    stosw
    mov ax, SCREEN_PITCH
    stosw
    mov ax, [dirty_rect_y + si]
    mov bx, SCREEN_PITCH
    mul bx
    mov bx, dx
    mov dx, [dirty_rect_x + si]
    and dx, 0xfffc
    shr dx, 1
    add ax, dx
    adc bx, 0
    add ax, [draw_page_sgp_low]
    adc bx, [draw_page_sgp_high]
    stosw
    mov ax, bx
    stosw
    mov ax, SGP_COMMAND_PATBLT
    stosw
    mov ax, SGP_PATBLT_COPY
    stosw
    add si, 2
    loop .next_rect
.done:
    pop bp
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret
%endif

select_build_buffer:
%if M7_VARIANT >= 3
    cmp byte [build_command_index], 0
    jne .buffer_b
    mov di, sgp_command_list_a
    mov [builder_command_base], di
    mov si, sgp_work_area_a
    mov [builder_work_area], si
    ret
.buffer_b:
    mov di, sgp_command_list_b
    mov [builder_command_base], di
    mov si, sgp_work_area_b
    mov [builder_work_area], si
    ret
%else
    mov di, sgp_command_list
    mov [builder_command_base], di
    mov si, sgp_work_area
    mov [builder_work_area], si
    ret
%endif

%if M7_VARIANT >= 4
; M7d startup pass: convert immutable main-RAM pointers once and build
; fixed SET_SOURCE/SET_DEST/BITBLT templates.  Frame emission patches only
; the destination start-dot and physical destination address.
initialize_m7d_tables:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    push ds
    pop es

    mov bx, sprite_records
    mov di, m7_sprite_source_low
    mov bp, m7_sprite_source_high
    mov cx, SPRITE_MAX_COUNT
.source_loop:
    push cx
    mov si, [bx + SPRITE_BITMAP_OFFSET]
    call physical_address_from_ds_si
    mov [di], ax
    mov [bp], dx
    pop cx
    add di, 2
    add bp, 2
    add bx, SPRITE_RECORD_SIZE
    loop .source_loop

    mov bx, fps_glyph_pointers
    mov di, m7_glyph_source_low
    mov bp, m7_glyph_source_high
    mov cx, FPS_GLYPH_COUNT
.glyph_source_loop:
    push cx
    mov si, [bx]
    call physical_address_from_ds_si
    mov [di], ax
    mov [bp], dx
    pop cx
    add bx, 2
    add di, 2
    add bp, 2
    loop .glyph_source_loop

    mov si, sgp_command_list_a
    call physical_address_from_ds_si
    mov [sgp_command_address_a_low], ax
    mov [sgp_command_address_a_high], dx
    mov si, sgp_command_list_b
    call physical_address_from_ds_si
    mov [sgp_command_address_b_low], ax
    mov [sgp_command_address_b_high], dx
    mov si, sgp_work_area_a
    call physical_address_from_ds_si
    mov [m7_work_address_low], ax
    mov [m7_work_address_high], dx
    mov si, sgp_zero_word
    call physical_address_from_ds_si
    mov [m7_zero_address_low], ax
    mov [m7_zero_address_high], dx

    mov bx, sprite_records
    mov bp, m7_sprite_source_low
    mov si, m7_sprite_source_high
    mov di, sprite_command_templates
    mov cx, SPRITE_MAX_COUNT
.template_loop:
    push cx
    mov ax, SGP_COMMAND_SET_SOURCE
    stosw
    mov ax, 0x0001
    stosw
    mov ax, [bx + SPRITE_WIDTH_OFFSET]
    stosw
    mov ax, [bx + SPRITE_HEIGHT_OFFSET]
    stosw
    mov ax, [bx + SPRITE_PITCH_OFFSET]
    stosw
    mov ax, [bp]
    stosw
    mov ax, [si]
    stosw

    mov ax, SGP_COMMAND_SET_DEST
    stosw
    xor ax, ax
    stosw
    mov ax, [bx + SPRITE_WIDTH_OFFSET]
    stosw
    mov ax, [bx + SPRITE_HEIGHT_OFFSET]
    stosw
    mov ax, SCREEN_PITCH
    stosw
    xor ax, ax
    stosw
    stosw

    mov ax, SGP_COMMAND_BITBLT
    stosw
    mov ax, SGP_BITBLT_COPY_XPAR
    stosw
    pop cx
    add bx, SPRITE_RECORD_SIZE
    add bp, 2
    add si, 2
    loop .template_loop

    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

emit_sprite_template_commands:
    push ax
    push bx
    push cx
    push dx
    push si
    push bp
    mov bx, sprite_records
    mov si, sprite_command_templates
    mov cx, [active_sprite_count]
.next_template:
    jcxz .done
    push cx
    mov cx, 16
    rep movsw
    pop cx

    mov ax, [bx + SPRITE_X_OFFSET]
    and ax, 0x0003
    shl ax, 4
    or ax, 0x0001
    mov [es:di - 16], ax

    mov ax, [bx + SPRITE_Y_OFFSET]
    shl ax, 1
    mov bp, ax
    mov ax, [y_offset_table + bp]
    xor bp, bp
    mov dx, [bx + SPRITE_X_OFFSET]
    and dx, 0xfffc
    shr dx, 1
    add ax, dx
    adc bp, 0
    add ax, [draw_page_sgp_low]
    adc bp, [draw_page_sgp_high]
    mov [es:di - 8], ax
    mov [es:di - 6], bp

    inc word [m6_sgp_bitblts_frame]
    add bx, SPRITE_RECORD_SIZE
    loop .next_template
.done:
    pop bp
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret
%endif

render_sprite_frame:
    call build_sgp_frame_commands
    call run_sgp_command_list
    ret

build_sgp_frame_commands:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    push es

    push ds
    pop es
    xor ax, ax
    mov [m6_sgp_bitblts_frame], ax
    mov [m6_sgp_pixels_frame_lo], ax
    mov [m6_sgp_pixels_frame_hi], ax
    mov [m6_sgp_source_bytes_frame_lo], ax
    mov [m6_sgp_source_bytes_frame_hi], ax
    call update_frame_transfer_stats
    call select_build_buffer

    mov ax, SGP_COMMAND_SET_WORK
    stosw
%if M7_VARIANT >= 4
    mov ax, [m7_work_address_low]
    stosw
    mov ax, [m7_work_address_high]
    stosw
%else
    mov si, [builder_work_area]
    call physical_address_from_ds_si
    stosw
    mov ax, dx
    stosw
%endif

    mov ax, SGP_COMMAND_SET_COLOR
    stosw
    xor ax, ax
    stosw

%if M7_VARIANT >= 2
    call prepare_dirty_rects
    cmp byte [dirty_full_clear], 0
    jne .full_clear
    call emit_dirty_clear_commands
    jmp .clear_done
.full_clear:
    inc word [m7_full_clears_frame]
%endif
    mov ax, SGP_COMMAND_CLS
    stosw
    mov ax, [draw_page_sgp_low]
    stosw
    mov ax, [draw_page_sgp_high]
    stosw
    mov ax, SCREEN_WORD_COUNT
    stosw
    xor ax, ax
    stosw
%if M7_VARIANT >= 2
.clear_done:
%endif

%if M7_VARIANT >= 4
    call emit_sprite_template_commands
%else
    mov si, sprite_records
    mov ax, [active_sprite_count]
    mov [m6_emit_remaining], ax

.emit_sprite:
    mov ax, SGP_COMMAND_SET_SOURCE
    stosw
    mov ax, 0x0001
    stosw
    mov ax, [si + SPRITE_WIDTH_OFFSET]
    stosw
    mov ax, [si + SPRITE_HEIGHT_OFFSET]
    stosw
    mov ax, [si + SPRITE_PITCH_OFFSET]
    stosw
    push si
    mov si, [si + SPRITE_BITMAP_OFFSET]
    call physical_address_from_ds_si
    pop si
    stosw
    mov ax, dx
    stosw

    mov ax, SGP_COMMAND_SET_DEST
    stosw
    mov ax, [si + SPRITE_X_OFFSET]
    and ax, 0x0003
    shl ax, 4
    or ax, 0x0001
    stosw
    mov ax, [si + SPRITE_WIDTH_OFFSET]
    stosw
    mov ax, [si + SPRITE_HEIGHT_OFFSET]
    stosw
    mov ax, SCREEN_PITCH
    stosw

    mov ax, [si + SPRITE_Y_OFFSET]
    mov bx, SCREEN_PITCH
    mul bx
    mov bx, [si + SPRITE_X_OFFSET]
    and bx, 0xfffc
    shr bx, 1
    add ax, bx
    adc dx, 0
    add ax, [draw_page_sgp_low]
    adc dx, [draw_page_sgp_high]
    stosw
    mov ax, dx
    stosw

    mov ax, SGP_COMMAND_BITBLT
    stosw
    mov ax, SGP_BITBLT_COPY_XPAR
    stosw
    inc word [m6_sgp_bitblts_frame]

    add si, SPRITE_RECORD_SIZE
    dec word [m6_emit_remaining]
    jz short .sprites_done
    jmp .emit_sprite
%endif

.sprites_done:
    call emit_fps_glyph_commands
    mov ax, SGP_COMMAND_END
    stosw

    mov ax, di
    sub ax, [builder_command_base]
    shr ax, 1
    mov [m6_sgp_commands_frame], ax
    call m6_accumulate_frame_counters

%if M7_VARIANT == 3
    mov si, [builder_command_base]
    call physical_address_from_ds_si
    cmp byte [build_command_index], 0
    jne .store_command_address_b
    mov [sgp_command_address_a_low], ax
    mov [sgp_command_address_a_high], dx
    jmp .command_address_ready
.store_command_address_b:
    mov [sgp_command_address_b_low], ax
    mov [sgp_command_address_b_high], dx
.command_address_ready:
%elif M7_VARIANT < 3
    mov si, sgp_command_list
    call physical_address_from_ds_si
    mov [sgp_command_address_low], ax
    mov [sgp_command_address_high], dx
%endif

    pop es
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

emit_fps_glyph_commands:
    push ax
    push bx
    push cx
    push dx
    push si
    push bp

    mov si, fps_glyph_pointers
    mov bx, FPS_GLYPH_X
    xor bp, bp
    mov ax, FPS_GLYPH_COUNT
    mov [m6_emit_remaining], ax

.next_glyph:
    mov ax, SGP_COMMAND_SET_SOURCE
    stosw
    mov ax, 0x0001
    stosw
    mov ax, FPS_GLYPH_WIDTH
    stosw
    mov ax, FPS_GLYPH_HEIGHT
    stosw
    mov ax, FPS_GLYPH_PITCH
    stosw
%if M7_VARIANT >= 4
    mov ax, [m7_glyph_source_low + bp]
    stosw
    mov ax, [m7_glyph_source_high + bp]
    stosw
%else
    push si
    mov si, [si]
    call physical_address_from_ds_si
    pop si
    stosw
    mov ax, dx
    stosw
%endif

    mov ax, SGP_COMMAND_SET_DEST
    stosw
    mov ax, bx
    and ax, 0x0003
    shl ax, 4
    or ax, 0x0001
    stosw
    mov ax, FPS_GLYPH_WIDTH
    stosw
    mov ax, FPS_GLYPH_HEIGHT
    stosw
    mov ax, SCREEN_PITCH
    stosw

    mov ax, FPS_GLYPH_Y * SCREEN_PITCH
    xor dx, dx
    mov cx, bx
    and cx, 0xfffc
    shr cx, 1
    add ax, cx
    adc dx, 0
    add ax, [draw_page_sgp_low]
    adc dx, [draw_page_sgp_high]
    stosw
    mov ax, dx
    stosw

    mov ax, SGP_COMMAND_BITBLT
    stosw
    mov ax, SGP_BITBLT_COPY_XPAR
    stosw
    inc word [m6_sgp_bitblts_frame]

    add si, 2
    add bp, 2
    add bx, FPS_GLYPH_ADVANCE
    dec word [m6_emit_remaining]
    jz short .glyphs_done
    jmp .next_glyph

.glyphs_done:
    pop bp
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

run_sgp_command_list:
%if M7_VARIANT >= 3
    mov al, [build_command_index]
    mov [current_command_index], al
    call start_sgp_command_list
    jc .failed
    call wait_sgp_idle
    ret
%else
    call wait_sgp_idle
    jc .failed
    call start_sgp_command_list
    jc .failed
    call wait_sgp_idle
    ret
%endif

.failed:
    stc
    ret

; The command pointer uses word writes at 0500h and 0502h.
; Control and start remain byte writes at 0504h and 0506h.
start_sgp_command_list:
    push ax
    push dx
    ; The VA BIOS reasserts CPU-data GVRAM write mode before each SGP kick.
    mov dx, PORT_GVRAM_WRITE_MODE
    mov al, GVRAM_CPU_WRITE_MODE
    out dx, al
    mov dx, PORT_SGP_COMMAND
%if M7_VARIANT >= 3
    cmp byte [current_command_index], 0
    jne .buffer_b
    mov ax, [sgp_command_address_a_low]
    out dx, ax
    add dx, 2
    mov ax, [sgp_command_address_a_high]
    jmp .address_ready
.buffer_b:
    mov ax, [sgp_command_address_b_low]
    out dx, ax
    add dx, 2
    mov ax, [sgp_command_address_b_high]
.address_ready:
%else
    mov ax, [sgp_command_address_low]
    out dx, ax
    add dx, 2
    mov ax, [sgp_command_address_high]
%endif
    out dx, ax

    mov dx, PORT_SGP_CONTROL
    xor al, al
    out dx, al

    mov dx, PORT_SGP_STATUS
    mov al, SGP_BUSY
    out dx, al
    pop dx
    pop ax
    clc
    ret

wait_sgp_idle:
    mov dx, PORT_SGP_STATUS
    mov cx, 0xffff

.poll:
    in al, dx
    test al, SGP_BUSY
    jz .ready
    loop .poll
    stc
    ret

.ready:
    clc
    ret

physical_address_from_ds_si:
    mov ax, ds
    xor dx, dx
    mov cx, 4

.shift_segment:
    shl ax, 1
    rcl dx, 1
    loop .shift_segment

    add ax, si
    adc dx, 0
    ret

save_video_state:
    mov dx, PORT_MEMORY_MAP
    in al, dx
    mov [saved_memory_map], al
    test al, 0x10
    jz .save_bios_state

    mov byte [saved_single_plane], 1
    mov dx, PORT_GVRAM_WRITE_MODE
    in al, dx
    mov [saved_write_mode], al

.save_bios_state:
    mov ax, VIDEO_BIOS_DATA_SEG
    mov es, ax
    mov ax, [es:VIDEO_MODE_OFFSET]
    mov [saved_video_mode], ax
    mov al, [es:VIDEO_G0_BPP_OFFSET]
    mov [saved_g0_bpp], al
    mov al, [es:VIDEO_G1_BPP_OFFSET]
    mov [saved_g1_bpp], al
    ret

restore_video_state:
    push cs
    pop ds

    mov dx, PORT_MEMORY_MAP
    mov al, [saved_memory_map]
    out dx, al

    cmp byte [video_mode_changed], 0
    je .restore_write_mode

    mov bx, [saved_video_mode]
    mov cl, [saved_g0_bpp]
    mov ch, [saved_g1_bpp]
    xor dx, dx
    xor ax, ax
    int VIDEO_BIOS_INT

    mov ax, 0x0a00
    int VIDEO_BIOS_INT

    mov dx, PORT_MEMORY_MAP
    mov al, [saved_memory_map]
    out dx, al

.restore_write_mode:
    cmp byte [saved_single_plane], 0
    je .done
    mov dx, PORT_GVRAM_WRITE_MODE
    mov al, [saved_write_mode]
    out dx, al

.done:
    ret

print_string:
    mov ah, 0x09
    int 0x21
    ret

saved_memory_map:
    db 0
saved_write_mode:
    db 0
saved_single_plane:
    db 0
video_mode_changed:
    db 0
saved_video_mode:
    dw 0
saved_g0_bpp:
    db 0
saved_g1_bpp:
    db 0
builder_command_base:
    dw 0
builder_work_area:
    dw 0
%if M7_VARIANT >= 3
sgp_command_address_a_low:
    dw 0
sgp_command_address_a_high:
    dw 0
sgp_command_address_b_low:
    dw 0
sgp_command_address_b_high:
    dw 0
current_command_index:
    db 0
build_command_index:
    db 0
pipeline_active:
    db 0
%else
sgp_command_address_low:
    dw 0
sgp_command_address_high:
    dw 0
%endif
active_sprite_count:
    dw SPRITE_INITIAL_COUNT
m6_emit_remaining:
    dw 0
fps_frame_counter:
    dw 0
fps_value:
    dw 0
fps_last_second:
    db 0
draw_page_index:
    db 1
draw_page_sgp_low:
    dw G1_PAGE_B_SGP_BASE & 0xffff
draw_page_sgp_high:
    dw G1_PAGE_B_SGP_BASE >> 16
draw_page_dsa_low:
    dw G1_PAGE_B_DSA & 0xffff
draw_page_dsa_high:
    dw G1_PAGE_B_DSA >> 16

%if M7_VARIANT >= 2
m7_dirty_rects_frame:
    dw 0
m7_dirty_pixels_frame_lo:
    dw 0
m7_dirty_pixels_frame_hi:
    dw 0
m7_full_clears_frame:
    dw 0
dirty_full_clear:
    db 0
dirty_rect_count:
    dw 0
dirty_pixels_lo:
    dw 0
dirty_pixels_hi:
    dw 0
dirty_page_old_x:
    dw 0
dirty_page_old_y:
    dw 0
dirty_page_new_x:
    dw 0
dirty_page_new_y:
    dw 0
dirty_page_active:
    dw 0
dirty_page_valid:
    dw 0
dirty_rect_x:
    times DIRTY_RECT_MAX_COUNT dw 0
dirty_rect_y:
    times DIRTY_RECT_MAX_COUNT dw 0
dirty_rect_w:
    times DIRTY_RECT_MAX_COUNT dw 0
dirty_rect_h:
    times DIRTY_RECT_MAX_COUNT dw 0
page_last_active_a:
    dw 0
page_last_valid_a:
    db 0
    db 0
page_last_x_a:
    times SPRITE_MAX_COUNT dw 0
page_last_y_a:
    times SPRITE_MAX_COUNT dw 0
page_last_active_b:
    dw 0
page_last_valid_b:
    db 0
    db 0
page_last_x_b:
    times SPRITE_MAX_COUNT dw 0
page_last_y_b:
    times SPRITE_MAX_COUNT dw 0
%endif

; Runtime diagnostics are 32-bit counters and wrap only after 0xffffffff.
m6_frames_lo:
    dw 0
m6_frames_hi:
    dw 0
m6_page_flips_lo:
    dw 0
m6_page_flips_hi:
    dw 0
m6_sgp_commands_frame:
    dw 0
m6_sgp_bitblts_frame:
    dw 0
m6_sgp_pixels_frame_lo:
    dw 0
m6_sgp_pixels_frame_hi:
    dw 0
m6_sgp_source_bytes_frame_lo:
    dw 0
m6_sgp_source_bytes_frame_hi:
    dw 0
m6_total_commands_lo:
    dw 0
m6_total_commands_hi:
    dw 0
m6_total_bitblts_lo:
    dw 0
m6_total_bitblts_hi:
    dw 0
m6_total_pixels_lo:
    dw 0
m6_total_pixels_hi:
    dw 0
m6_total_source_bytes_lo:
    dw 0
m6_total_source_bytes_hi:
    dw 0
m6_missed_vblank_lo:
    dw 0
m6_missed_vblank_hi:
    dw 0
m6_print_value_lo:
    dw 0
m6_print_value_hi:
    dw 0
m6_number_buffer:
    times 12 db 0

%if M7_SCROLL
align 2, db 0
scroll_phase_top:
    dw 0
scroll_phase_middle:
    dw 0
scroll_phase_bottom:
    dw 0
scroll_work_y:
    dw 0
scroll_work_phase:
    dw 0
scroll_work_rows:
    dw 0

; Each source row is 192 bytes so a rounded eight-byte phase can be applied
; without crossing the end of the pattern while copying a 160-byte screen
; row. The two rows are the alternating halves of a 16x16 checker tile.
scroll_pattern_dark:
%rep 6
    times 8 db 0xdd
    times 8 db 0xee
    times 8 db 0xdd
    times 8 db 0xee
%endrep
scroll_pattern_light:
%rep 6
    times 8 db 0xee
    times 8 db 0xdd
    times 8 db 0xee
    times 8 db 0xdd
%endrep
scroll_row_buffer:
    times SCREEN_PITCH db 0
%endif

message_start:
%if M7_SCROLL
    db "SGPD_7S: M7 scrolling Graphic 0 background", 13, 10
    db "Three bands use 3/7/11 phases aligned to checker boundaries.", 13, 10
    db "UP/+ adds a sprite (max 256), DOWN/- removes one, ESC exits.", 13, 10, "$"
%elif M7_VARIANT == 1
    db "SGPD_7A: M7a measurement-separated baseline", 13, 10
    db "UP/+ adds a sprite (max 256), DOWN/- removes one, ESC exits.", 13, 10, "$"
%elif M7_VARIANT == 2
    db "SGPD_7B: M7b page-local dirty clear", 13, 10
    db "UP/+ adds a sprite (max 256), DOWN/- removes one, ESC exits.", 13, 10, "$"
%elif M7_VARIANT == 3
    db "SGPD_7C: M7c CPU/SGP pipeline", 13, 10
    db "UP/+ adds a sprite (max 256), DOWN/- removes one, ESC exits.", 13, 10, "$"
%elif M7_VARIANT == 4
    db "SGPD_7D: M7d invariant-hoisted renderer", 13, 10
    db "UP/+ adds a sprite (max 256), DOWN/- removes one, ESC exits.", 13, 10, "$"
%else
    db "SGP final pseudo-sprite renderer", 13, 10
    db "UP/+ adds a sprite (max 256), DOWN/- removes one, ESC exits.", 13, 10, "$"
%endif
message_m6_summary:
    db 13, 10, "SGP counters (last frame and totals):", 13, 10, "$"
label_m6_frames:
    db "Frames: ", 13, 10, "$"
label_m6_page_flips:
    db "Page flips: ", 13, 10, "$"
label_m6_last_commands:
    db "Last command words: ", 13, 10, "$"
label_m6_last_bitblts:
    db "Last BITBLTs: ", 13, 10, "$"
label_m6_last_pixels:
    db "Last source pixels: ", 13, 10, "$"
label_m6_last_bytes:
    db "Last source bytes: ", 13, 10, "$"
label_m6_total_commands:
    db "Total command words: ", 13, 10, "$"
label_m6_total_bitblts:
    db "Total BITBLTs: ", 13, 10, "$"
label_m6_total_pixels:
    db "Total source pixels: ", 13, 10, "$"
label_m6_total_bytes:
    db "Total source bytes: ", 13, 10, "$"
label_m6_active:
    db "Active sprites: ", 13, 10, "$"
label_m6_missed:
    db "Missed VBLANK waits: ", 13, 10, "$"
%if M7_VARIANT >= 2
label_m7_dirty_rects:
    db "Dirty rectangles: ", 13, 10, "$"
label_m7_dirty_pixels:
    db "Dirty logical pixels: ", 13, 10, "$"
label_m7_full_clears:
    db "Full CLS clears: ", 13, 10, "$"
%endif
message_done:
    db "Video state restored.", 13, 10, "$"
message_failed:
    db "Video or SGP initialization failed.", 13, 10, "$"
message_animation_failed:
    db "Animation synchronization failed.", 13, 10, "$"

align 2, db 0
palette_values:
    dw RGB565(0, 0, 0)
    dw RGB565(29, 9, 4)
    dw RGB565(29, 34, 4)
    dw RGB565(29, 60, 4)
    dw RGB565(17, 60, 4)
    dw RGB565(4, 60, 4)
    dw RGB565(4, 60, 17)
    dw RGB565(4, 60, 29)
    dw RGB565(4, 34, 29)
    dw RGB565(4, 9, 29)
    dw RGB565(17, 9, 29)
    dw RGB565(29, 9, 29)
    dw RGB565(29, 9, 17)
    dw RGB565(3, 6, 3)
    dw RGB565(9, 18, 9)
    dw RGB565(31, 63, 31)

%if M7_VARIANT >= 4
align 2, db 0
y_offset_table:
%assign Y_OFFSET 0
%rep SCREEN_HEIGHT
    dw Y_OFFSET
%assign Y_OFFSET Y_OFFSET + SCREEN_PITCH
%endrep

align 2, db 0
m7_sprite_source_low:
    times SPRITE_MAX_COUNT dw 0
m7_sprite_source_high:
    times SPRITE_MAX_COUNT dw 0
m7_glyph_source_low:
    times FPS_GLYPH_COUNT dw 0
m7_glyph_source_high:
    times FPS_GLYPH_COUNT dw 0
m7_work_address_low:
    dw 0
m7_work_address_high:
    dw 0
m7_zero_address_low:
    dw 0
m7_zero_address_high:
    dw 0

align 2, db 0
sprite_command_templates:
    times SPRITE_MAX_COUNT * 16 dw 0
%endif

align 2, db 0
%if M7_VARIANT >= 3
sgp_command_list_a:
    times SGP_COMMAND_WORD_COUNT dw 0
sgp_command_list_b:
    times SGP_COMMAND_WORD_COUNT dw 0
%else
sgp_command_list:
    times SGP_COMMAND_WORD_COUNT dw 0
%endif

align 2, db 0
sprite_records:
    ; x, y, vx, vy, width, height, pitch, bitmap, painter priority
    dw 24, 92, 2, 0, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_00, 0
    dw 280, 92, -2, 0, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_01, 1
    dw 11, 13, 1, 1, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_02, 2
    dw 71, 21, -1, 1, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_03, 3
    dw 131, 31, 2, 1, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_04, 4
    dw 211, 19, -2, 1, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_05, 5
    dw 285, 45, -1, 2, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_06, 6
    dw 33, 131, 2, -1, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_07, 7
    dw 97, 168, 1, -2, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_08, 8
    dw 161, 141, -1, -1, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_09, 9
    dw 231, 155, -2, -1, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_10, 10
    dw 294, 176, -1, -2, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_11, 11
    dw 45, 59, 1, 2, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_12, 12
    dw 119, 105, -2, 1, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_13, 13
    dw 193, 63, 1, -2, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_14, 14
    dw 261, 119, -1, 1, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_15, 15
%assign BULLET_INDEX BULLET_FIRST_INDEX
%rep BULLET_COUNT
    dw ((BULLET_INDEX * 17) % (SCREEN_WIDTH - BULLET_WIDTH)), ((BULLET_INDEX * 29) % (SCREEN_HEIGHT - BULLET_HEIGHT)), ((BULLET_INDEX * 3) % 5) - 2, ((BULLET_INDEX * 5) % 5) - 2, BULLET_WIDTH, BULLET_HEIGHT, BULLET_PITCH, bullet_bitmap, BULLET_INDEX
%assign BULLET_INDEX BULLET_INDEX + 1
%endrep
%assign EXTRA_SPRITE_INDEX BULLET_FIRST_INDEX + BULLET_COUNT
%rep SPRITE_MAX_COUNT - (BULLET_FIRST_INDEX + BULLET_COUNT)
    dw ((EXTRA_SPRITE_INDEX * 37) % (SCREEN_WIDTH - SPRITE_WIDTH)), ((EXTRA_SPRITE_INDEX * 53) % (SCREEN_HEIGHT - SPRITE_HEIGHT)), ((EXTRA_SPRITE_INDEX * 3) % 5) - 2, ((EXTRA_SPRITE_INDEX * 5) % 7) - 3, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_00 + ((EXTRA_SPRITE_INDEX % 16) * SPRITE_BITMAP_BYTES), EXTRA_SPRITE_INDEX
%assign EXTRA_SPRITE_INDEX EXTRA_SPRITE_INDEX + 1
%endrep


align 2, db 0
%if M7_VARIANT >= 3
sgp_work_area_a:
    times 58 db 0
sgp_work_area_b:
    times 58 db 0
%else
sgp_work_area:
    times 58 db 0
%endif

align 2, db 0
sgp_zero_word:
    dw 0

align 2, db 0
DEFINE_HSV_BALL hsv_ball_00, 1, 1
DEFINE_HSV_BALL hsv_ball_01, 1, 2
DEFINE_HSV_BALL hsv_ball_02, 2, 2
DEFINE_HSV_BALL hsv_ball_03, 3, 3
DEFINE_HSV_BALL hsv_ball_04, 4, 4
DEFINE_HSV_BALL hsv_ball_05, 4, 5
DEFINE_HSV_BALL hsv_ball_06, 5, 5
DEFINE_HSV_BALL hsv_ball_07, 6, 6
DEFINE_HSV_BALL hsv_ball_08, 7, 7
DEFINE_HSV_BALL hsv_ball_09, 7, 8
DEFINE_HSV_BALL hsv_ball_10, 8, 8
DEFINE_HSV_BALL hsv_ball_11, 9, 9
DEFINE_HSV_BALL hsv_ball_12, 10, 10
DEFINE_HSV_BALL hsv_ball_13, 10, 11
DEFINE_HSV_BALL hsv_ball_14, 11, 11
DEFINE_HSV_BALL hsv_ball_15, 12, 12

; Small 8x8 4-bpp bullet used by the final stress prefix.
align 2, db 0
bullet_bitmap:
    db BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(0, 15), BALL_PAIR(15, 0), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 15), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(15, 0)
    db BALL_PAIR(15, 15), BALL_PAIR(15, 12), BALL_PAIR(12, 15), BALL_PAIR(15, 15)
    db BALL_PAIR(15, 12), BALL_PAIR(12, 12), BALL_PAIR(12, 12), BALL_PAIR(12, 15)
    db BALL_PAIR(15, 15), BALL_PAIR(15, 12), BALL_PAIR(12, 15), BALL_PAIR(15, 15)
    db BALL_PAIR(0, 15), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(15, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(0, 13), BALL_PAIR(13, 0), BALL_PAIR(0, 0)

align 2, db 0
fps_glyph_pointers:
    dw glyph_f, glyph_p, glyph_s, glyph_0, glyph_0, glyph_0, glyph_c, glyph_0, glyph_0, glyph_0, glyph_0
digit_glyph_table:
    dw glyph_0, glyph_1, glyph_2, glyph_3, glyph_4, glyph_5, glyph_6, glyph_7, glyph_8, glyph_9

glyph_f:
    db 0xff, 0xff, 0xf0, 0x00, 0xff, 0xf0, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00
glyph_p:
    db 0xff, 0xf0, 0xf0, 0x0f, 0xf0, 0x0f, 0xff, 0xf0, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00
glyph_s:
    db 0x0f, 0xff, 0xf0, 0x00, 0xf0, 0x00, 0x0f, 0xf0, 0x00, 0x0f, 0x00, 0x0f, 0xff, 0xf0
glyph_c:
    db 0xff, 0xf0, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xff, 0xf0
glyph_0:
    db 0x0f, 0xf0, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0x0f, 0xf0
glyph_1:
    db 0x00, 0xf0, 0x0f, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x0f, 0xff
glyph_2:
    db 0xff, 0xf0, 0x00, 0x0f, 0x00, 0x0f, 0x0f, 0xf0, 0xf0, 0x00, 0xf0, 0x00, 0xff, 0xff
glyph_3:
    db 0xff, 0xf0, 0x00, 0x0f, 0x00, 0x0f, 0x0f, 0xf0, 0x00, 0x0f, 0x00, 0x0f, 0xff, 0xf0
glyph_4:
    db 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xff, 0xff, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f
glyph_5:
    db 0xff, 0xff, 0xf0, 0x00, 0xf0, 0x00, 0xff, 0xf0, 0x00, 0x0f, 0x00, 0x0f, 0xff, 0xf0
glyph_6:
    db 0x0f, 0xff, 0xf0, 0x00, 0xf0, 0x00, 0xff, 0xf0, 0xf0, 0x0f, 0xf0, 0x0f, 0x0f, 0xf0
glyph_7:
    db 0xff, 0xff, 0x00, 0x0f, 0x00, 0xf0, 0x00, 0xf0, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00
glyph_8:
    db 0x0f, 0xf0, 0xf0, 0x0f, 0xf0, 0x0f, 0x0f, 0xf0, 0xf0, 0x0f, 0xf0, 0x0f, 0x0f, 0xf0
glyph_9:
    db 0x0f, 0xf0, 0xf0, 0x0f, 0xf0, 0x0f, 0x0f, 0xff, 0x00, 0x0f, 0x00, 0x0f, 0xff, 0xf0
