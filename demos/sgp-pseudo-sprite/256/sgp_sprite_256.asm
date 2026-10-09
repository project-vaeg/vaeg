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
; OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
; OF THE POSSIBILITY OF SUCH DAMAGE.

bits 16
org 0x100

%ifndef SCROLL_BACKGROUND
%define SCROLL_BACKGROUND       0
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
%define PORT_PALETTE_COMPOSE    0x0106
%define PORT_RGB_COMPOSE        0x0108
%define PORT_FB1_FBW            0x0224
%define PORT_FB1_DOT            0x0228
%define PORT_FB1_OFX            0x022a
%define PORT_FB1_OFY            0x022c
%define PORT_FB1_DSA_LOW        0x022e
%define PORT_FB1_DSA_HIGH       0x0230
%define PORT_FB1_DSH            0x0232
%define PORT_FB1_DSP            0x0236
%define PORT_TSP_STATUS         0x0142
%define PORT_SGP_COMMAND        0x0500
%define PORT_SGP_CONTROL        0x0504
%define PORT_SGP_STATUS         0x0506
%define PORT_GVRAM_WRITE_MODE   0x0580

%define MEMORY_MAP_GVRAM_SINGLE 0x54
%define GVRAM_CPU_WRITE_MODE    0x10
%define MODE_320X200_G0_G1      0xe00e
%define PIXEL_SIZE_G0_G1_8BPP   0x0808
%define COMPOSE_G1_OVER_G0      0x0034
%define RGB_COMPOSE_G1_OVER_G0  0x0089
%define TSP_STATUS_VBLANK       0x40

; Graphic 0 is the 320x200 background. Graphic 1 owns a 320x400 backing
; surface containing two 320x200 8-bpp pages. Each page is 64,000 bytes and
; is selected through the FB1 DSA register pair; the display window remains
; 320x200.
%define G0_SEGMENT              0xa000
%define SCREEN_WIDTH            320
%define SCREEN_HEIGHT           200
%define SCREEN_PITCH            320
%define G1_PAGE_A_SGP_BASE      0x220000
%define G1_PAGE_B_SGP_BASE      0x22fa00
%define G1_PAGE_A_DSA           0x020000
%define G1_PAGE_B_DSA           0x02fa00
%define SCREEN_WORD_COUNT       0x7d00

%define SGP_COMMAND_END         0x0001
%define SGP_COMMAND_SET_WORK    0x0003
%define SGP_COMMAND_SET_SOURCE  0x0004
%define SGP_COMMAND_SET_DEST    0x0005
%define SGP_COMMAND_SET_COLOR   0x0006
%define SGP_COMMAND_BITBLT      0x0007
%define SGP_COMMAND_CLS         0x000a
%define SGP_BITBLT_COPY_XPAR    0x0105
%define SGP_BUSY                0x01

%define SPRITE_MIN_COUNT        1
%define SPRITE_INITIAL_COUNT    16
%define SPRITE_MAX_COUNT        128
%define SPRITE_WIDTH            24
%define SPRITE_HEIGHT           24
%define SPRITE_PITCH            24
%define SPRITE_BITMAP_BYTES     (SPRITE_PITCH * SPRITE_HEIGHT)
%define SPRITE_RECORD_SIZE      8

%define FPS_GLYPH_COUNT         11
%define FPS_GLYPH_WIDTH         4
%define FPS_GLYPH_HEIGHT        7
%define FPS_GLYPH_PITCH         4
%define FPS_GLYPH_X             248
%define FPS_GLYPH_Y             4
%define FPS_GLYPH_ADVANCE       5
%define FPS_SAMPLE_FRAMES       60
; The 8-bpp direct-color layout is GGG RRR BB.  Use all channel bits for
; status glyphs so the displayed FPS/C text is neutral white, not yellow.
%define GLYPH_COLOR             0xff

start:
    push cs
    pop ds
    cld
    call save_video_state
    mov dx, message_start
    call print_string
    call initialize_video
    jc initialization_failed

animation_loop:
    call poll_keyboard
    jc animation_done
    call wait_vblank_start
    jc animation_failed
%if SCROLL_BACKGROUND
    call update_scroll_background
%endif
    call update_sprites
    call update_fps_counter
    call build_sprite_commands
    call run_sgp_command_list
    jc animation_failed
    call display_draw_page
    xor byte [draw_page_index], 1
    call select_draw_page
    jmp animation_loop

animation_done:
    call restore_video_state
    mov dx, message_done
    call print_string
    mov ax, 0x4c00
    int 0x21

initialization_failed:
    call restore_video_state
    mov dx, message_initialization_failed
    call print_string
    mov ax, 0x4c01
    int 0x21

animation_failed:
    call restore_video_state
    mov dx, message_animation_failed
    call print_string
    mov ax, 0x4c02
    int 0x21

initialize_video:
    mov bx, MODE_320X200_G0_G1
    mov cx, PIXEL_SIZE_G0_G1_8BPP
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

    mov ax, 0x0300
    mov cx, COMPOSE_G1_OVER_G0
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed

    ; 8-bpp surfaces use the direct-color composition slots. The low
    ; nibble is the highest-priority slot: G1 (9) over G0 (8).
    xor ax, ax
    mov dx, PORT_PALETTE_COMPOSE
    out dx, ax
    mov ax, RGB_COMPOSE_G1_OVER_G0
    mov dx, PORT_RGB_COMPOSE
    out dx, ax

    mov dx, PORT_G0_TRANSPARENCY
    xor ax, ax
    out dx, ax

    mov dx, PORT_G1_TRANSPARENCY
    mov ax, 0x0001
    out dx, ax

    call configure_g1_framebuffer

    mov dx, PORT_MEMORY_MAP
    mov al, MEMORY_MAP_GVRAM_SINGLE
    out dx, al
    mov dx, PORT_GVRAM_WRITE_MODE
    mov al, GVRAM_CPU_WRITE_MODE
    out dx, al

    call draw_g0_checkerboard
    mov byte [draw_page_index], 0
    call select_draw_page
    call build_sprite_commands
    call run_sgp_command_list
    jc .failed
    call display_draw_page
    mov byte [draw_page_index], 1
    call select_draw_page
    call build_sprite_commands
    call run_sgp_command_list
    jc .failed
    mov ax, 0x0b01
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    clc
    ret
.failed:
    stc
    ret

; FB1 is a 320-byte-pitch, 400-line backing surface with a 200-line
; display window. DSA1 selects its upper or lower 200-line page; FBL is not
; used for FB1 vertical wraparound on the VA path.
configure_g1_framebuffer:
    push ax
    push dx
    mov dx, PORT_FB1_FBW
    mov ax, SCREEN_PITCH
    out dx, ax
    mov dx, PORT_FB1_DOT
    xor ax, ax
    out dx, ax
    mov dx, PORT_FB1_OFX
    out dx, ax
    mov dx, PORT_FB1_OFY
    out dx, ax
    mov dx, PORT_FB1_DSH
    mov ax, SCREEN_HEIGHT
    out dx, ax
    mov dx, PORT_FB1_DSP
    xor ax, ax
    out dx, ax
    pop dx
    pop ax
    ret

select_draw_page:
    cmp byte [draw_page_index], 0
    je .page_a
    mov word [draw_page_sgp_low], G1_PAGE_B_SGP_BASE & 0xffff
    mov word [draw_page_sgp_high], G1_PAGE_B_SGP_BASE >> 16
    mov word [draw_page_dsa_low], G1_PAGE_B_DSA & 0xffff
    mov word [draw_page_dsa_high], G1_PAGE_B_DSA >> 16
    ret
.page_a:
    mov word [draw_page_sgp_low], G1_PAGE_A_SGP_BASE & 0xffff
    mov word [draw_page_sgp_high], G1_PAGE_A_SGP_BASE >> 16
    mov word [draw_page_dsa_low], G1_PAGE_A_DSA & 0xffff
    mov word [draw_page_dsa_high], G1_PAGE_A_DSA >> 16
    ret

display_draw_page:
    mov dx, PORT_FB1_DSA_LOW
    mov ax, [draw_page_dsa_low]
    out dx, ax
    add dx, 2
    mov ax, [draw_page_dsa_high]
    out dx, ax
    ret

; Graphic 0 is a CPU-written 8-bpp checkerboard. It is deliberately kept
; independent from the SGP sprite list so the two graphics screens exercise
; the same G0/G1 composition path as the 16-color demo.
draw_g0_checkerboard:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    push es
    mov ax, G0_SEGMENT
    mov es, ax
    xor di, di
    xor bx, bx
.row:
    mov dx, checker_row_a
    mov bp, checker_row_b
    test bl, 0x10
    jz .phase_ready
    xchg dx, bp
.phase_ready:
    mov cx, 20
.tile:
    mov si, dx
    test cl, 1
    jz .tile_source_ready
    mov si, bp
.tile_source_ready:
    push cx
    mov cx, 16
    rep movsb
    pop cx
    dec cx
    jnz .tile
    inc bx
    cmp bx, SCREEN_HEIGHT
    jne .row
    pop es
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

%if SCROLL_BACKGROUND
; Move three independently phased 16-dot checker bands.  The internal
; phases advance by 3, 7, and 11 byte units, then round to a 16-byte tile
; boundary before copying each row.  G0 is CPU-visible memory, so the final
; row transfer uses word stores and never byte-accesses a hardware word port.
update_scroll_background:
    push ax
    push bx
    push cx
    push dx
    push es
    mov ax, G0_SEGMENT
    mov es, ax

    add word [scroll_phase_top], 3
    and word [scroll_phase_top], 0x001f
    add word [scroll_phase_middle], 7
    and word [scroll_phase_middle], 0x001f
    add word [scroll_phase_bottom], 11
    and word [scroll_phase_bottom], 0x001f

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

    pop es
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; BX=start row, CX=row count, DX=unrounded phase.  The source pattern is
; longer than one display row so an aligned 16-byte phase can be copied
; without wrapping inside the temporary row buffer.
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
    add ax, 8
    and ax, 0x0010
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

update_sprites:
    push ax
    push cx
    push si
    mov si, sprite_records
    mov cx, [active_sprite_count]
.next:
    mov al, [si + 4]
    cbw
    add ax, [si]
    cmp ax, SCREEN_WIDTH - SPRITE_WIDTH
    jns .x_high
    cmp ax, 0
    js .x_low
    mov [si], ax
    jmp .y_axis
.x_high:
    mov word [si], SCREEN_WIDTH - SPRITE_WIDTH
    neg byte [si + 4]
.x_low:
    cmp ax, 0
    jns .y_axis
    mov word [si], 0
    neg byte [si + 4]
.y_axis:
    mov al, [si + 5]
    cbw
    add ax, [si + 2]
    cmp ax, SCREEN_HEIGHT - SPRITE_HEIGHT
    jns .y_high
    cmp ax, 0
    js .y_low
    mov [si + 2], ax
    jmp .next_record
.y_high:
    mov word [si + 2], SCREEN_HEIGHT - SPRITE_HEIGHT
    neg byte [si + 5]
.y_low:
    cmp ax, 0
    jns .next_record
    mov word [si + 2], 0
    neg byte [si + 5]
.next_record:
    add si, SPRITE_RECORD_SIZE
    loop .next
    pop si
    pop cx
    pop ax
    ret

build_sprite_commands:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push es
    push ds
    pop es
    mov di, sgp_command_list

    mov ax, SGP_COMMAND_SET_WORK
    stosw
    mov si, sgp_work_area
    call physical_address_from_ds_si
    stosw
    mov ax, dx
    stosw

    mov ax, SGP_COMMAND_SET_COLOR
    stosw
    xor ax, ax
    stosw

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

    mov si, sprite_records
    mov cx, [active_sprite_count]
.sprite:
    push cx
    call emit_sprite
    pop cx
    add si, SPRITE_RECORD_SIZE
    loop .sprite

    call emit_status_glyphs

    mov ax, SGP_COMMAND_END
    stosw

    mov si, sgp_command_list
    call physical_address_from_ds_si
    mov [sgp_command_address_low], ax
    mov [sgp_command_address_high], dx
    pop es
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

emit_sprite:
    push ax
    push bx
    push cx
    push dx
    push si
    push bp
    mov bp, si
    mov ax, SGP_COMMAND_SET_SOURCE
    stosw
    mov ax, 2
    stosw
    mov ax, SPRITE_WIDTH
    stosw
    mov ax, SPRITE_HEIGHT
    stosw
    mov ax, SPRITE_PITCH
    stosw
    mov si, [bp + 6]
    call physical_address_from_ds_si
    stosw
    mov ax, dx
    stosw

    mov ax, SGP_COMMAND_SET_DEST
    stosw
    mov ax, 2
    stosw
    mov ax, SPRITE_WIDTH
    stosw
    mov ax, SPRITE_HEIGHT
    stosw
    mov ax, SCREEN_PITCH
    stosw
    mov ax, [bp + 2]
    mov dx, SCREEN_PITCH
    mul dx
    ; 8-bpp destination addresses are byte offsets.  Align the base to an
    ; even pixel and let the SET_DEST start-dot select the odd pixel.
    mov bx, [bp]
    and bx, 0xfffe
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
    pop bp
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

emit_status_glyphs:
    push ax
    push bx
    push cx
    push dx
    push si
    push bp
    mov si, fps_glyph_pointers
    mov bx, FPS_GLYPH_X
    mov cx, FPS_GLYPH_COUNT
.next:
    mov ax, SGP_COMMAND_SET_SOURCE
    stosw
    mov ax, 2
    stosw
    mov ax, FPS_GLYPH_WIDTH
    stosw
    mov ax, FPS_GLYPH_HEIGHT
    stosw
    mov ax, FPS_GLYPH_PITCH
    stosw
    push cx
    push si
    mov si, [si]
    call physical_address_from_ds_si
    pop si
    pop cx
    stosw
    mov ax, dx
    stosw

    mov ax, SGP_COMMAND_SET_DEST
    stosw
    mov ax, bx
    and ax, 1
    shl ax, 4
    or ax, 2
    stosw
    mov ax, FPS_GLYPH_WIDTH
    stosw
    mov ax, FPS_GLYPH_HEIGHT
    stosw
    mov ax, SCREEN_PITCH
    stosw
    mov ax, FPS_GLYPH_Y * SCREEN_PITCH
    xor dx, dx
    mov bp, bx
    and bp, 0xfffe
    add ax, bp
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
    add si, 2
    add bx, FPS_GLYPH_ADVANCE
    loop .next
    pop bp
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

poll_keyboard:
    mov ah, 0x0a
    int KEYBOARD_BIOS_INT
    jc .none
    mov ah, 0x09
    int KEYBOARD_BIOS_INT
    cmp ah, 0
    je .escape
    cmp ah, 0x48
    je .increase_count
    cmp ah, 0x50
    je .decrease_count
    cmp ah, 0x3a
    je .increase_count
    cmp ah, 0x3d
    je .decrease_count
    cmp al, '+'
    je .increase_count
    cmp al, '-'
    je .decrease_count
.none:
    clc
    ret
.increase_count:
    cmp word [active_sprite_count], SPRITE_MAX_COUNT
    jae .none
    inc word [active_sprite_count]
    call format_status_glyphs
    clc
    ret
.decrease_count:
    cmp word [active_sprite_count], SPRITE_MIN_COUNT
    jbe .none
    dec word [active_sprite_count]
    call format_status_glyphs
    clc
    ret
.escape:
    stc
    ret

initialize_fps_counter:
    mov word [fps_frame_counter], 0
    mov word [fps_value], 0
    mov ah, 0x02
    int CALENDAR_BIOS_INT
    mov [fps_last_second], dh
    call format_status_glyphs
    ret

update_fps_counter:
    inc word [fps_frame_counter]
    cmp word [fps_frame_counter], FPS_SAMPLE_FRAMES
    jb .done
    mov ah, 0x02
    int CALENDAR_BIOS_INT
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
    call format_status_glyphs
.done:
    ret

format_status_glyphs:
    push ax
    push bx
    push dx
    push si
    mov ax, [fps_value]
    cmp ax, 999
    jbe .fps_limit
    mov ax, 999
.fps_limit:
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
.wait_display:
    mov cx, 0xffff
.display_poll:
    in al, dx
    test al, TSP_STATUS_VBLANK
    jz .display_seen
    loop .display_poll
    dec bx
    jnz .wait_display
    stc
    ret
.display_seen:
    mov bx, 4
.wait_vblank:
    mov cx, 0xffff
.vblank_poll:
    in al, dx
    test al, TSP_STATUS_VBLANK
    jnz .ready
    loop .vblank_poll
    dec bx
    jnz .wait_vblank
    stc
    ret
.ready:
    clc
    ret

run_sgp_command_list:
    call wait_sgp_idle
    jc .failed
    mov dx, PORT_GVRAM_WRITE_MODE
    mov al, GVRAM_CPU_WRITE_MODE
    out dx, al
    mov dx, PORT_SGP_COMMAND
    mov ax, [sgp_command_address_low]
    out dx, ax
    add dx, 2
    mov ax, [sgp_command_address_high]
    out dx, ax
    mov dx, PORT_SGP_CONTROL
    xor al, al
    out dx, al
    mov dx, PORT_SGP_STATUS
    mov al, SGP_BUSY
    out dx, al
    call wait_sgp_idle
    ret
.failed:
    stc
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
.shift:
    shl ax, 1
    rcl dx, 1
    loop .shift
    add ax, si
    adc dx, 0
    ret

save_video_state:
    mov dx, PORT_MEMORY_MAP
    in al, dx
    mov [saved_memory_map], al
    mov dx, VIDEO_BIOS_DATA_SEG
    mov es, dx
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
    je .done
    mov bx, [saved_video_mode]
    mov cl, [saved_g0_bpp]
    mov ch, [saved_g1_bpp]
    xor dx, dx
    xor ax, ax
    int VIDEO_BIOS_INT
.done:
    ret

print_string:
    mov ah, 0x09
    int 0x21
    ret

message_start:
%if SCROLL_BACKGROUND
    db "SGP256T: 8-bpp pseudo-sprites with scrolling G0", 13, 10
    db "Three bands use 3/7/11 phases aligned to 16-dot checker tiles.", 13, 10
    db "UP/DOWN changes the ball count (1-128), ESC exits.", 13, 10, "$"
%else
    db "SGP 256-color double-buffered pseudo-sprite demo", 13, 10
    db "G0 VA 8-bpp direct GGGRRRBB ray-traced spheres; PATBLT checkerboard.", 13, 10
    db "UP/DOWN: count 1-128. ESC exits. FPS/C shown at top right.", 13, 10, "$"
%endif
message_done:
    db "Video state restored.", 13, 10, "$"
message_initialization_failed:
    db "Video or SGP initialization failed.", 13, 10, "$"
message_animation_failed:
    db "SGP synchronization failed.", 13, 10, "$"

align 2, db 0
sgp_command_list:
    times 4096 dw 0
sgp_work_area:
    times 29 dw 0

align 2, db 0
checker_row_a:
    times 8 db 0x00
    times 8 db 0x6d
checker_row_b:
    times 8 db 0x6d
    times 8 db 0x00

%if SCROLL_BACKGROUND
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

; Each source row is 352 bytes, allowing a 0- or 16-byte aligned phase to
; supply a complete 320-byte display row.  The two source rows are the
; alternating halves of the 16x16 checker tile.
scroll_pattern_dark:
%rep 11
    times 16 db 0x00
    times 16 db 0x6d
%endrep
scroll_pattern_light:
%rep 11
    times 16 db 0x6d
    times 16 db 0x00
%endrep
scroll_row_buffer:
    times SCREEN_PITCH db 0
%endif

align 2, db 0
; Retained only as a local data pattern for source-level reference; the
; active background writer uses checker_row_a/checker_row_b directly.
checker_pattern:
    db 0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38
    db 0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38
    db 0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38
    db 0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38
    db 0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38
    db 0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38
    db 0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38
    db 0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38
    db 0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18
    db 0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18
    db 0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18
    db 0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18
    db 0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18
    db 0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18
    db 0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18
    db 0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x38,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18

align 2, db 0
sprite_records:
%assign RECORD_INDEX 0
%macro SPRITE_RECORD 5
    dw %1, %2
    db %3, %4
    dw orb8_bitmaps + ((RECORD_INDEX % 16) * SPRITE_BITMAP_BYTES)
%assign RECORD_INDEX RECORD_INDEX + 1
%endmacro
%include "../65536/sprite_records_128_random16.inc"
active_sprite_count:
    dw SPRITE_INITIAL_COUNT

align 2, db 0
orb8_bitmaps:
%include "orb_raytrace8_24.inc"

align 2, db 0
fps_glyph_pointers:
    dw glyph_f, glyph_p, glyph_s, glyph_0, glyph_0, glyph_0
    dw glyph_c, glyph_0, glyph_0, glyph_0, glyph_0
digit_glyph_table:
    dw glyph_0, glyph_1, glyph_2, glyph_3, glyph_4, glyph_5
    dw glyph_6, glyph_7, glyph_8, glyph_9

%macro GLYPH_ROW 1
%if (%1 & 8) != 0
    db GLYPH_COLOR
%else
    db 0
%endif
%if (%1 & 4) != 0
    db GLYPH_COLOR
%else
    db 0
%endif
%if (%1 & 2) != 0
    db GLYPH_COLOR
%else
    db 0
%endif
%if (%1 & 1) != 0
    db GLYPH_COLOR
%else
    db 0
%endif
%endmacro
%macro GLYPH 7
    GLYPH_ROW %1
    GLYPH_ROW %2
    GLYPH_ROW %3
    GLYPH_ROW %4
    GLYPH_ROW %5
    GLYPH_ROW %6
    GLYPH_ROW %7
%endmacro

glyph_f: GLYPH 0xf, 0x8, 0x8, 0xe, 0x8, 0x8, 0x8
glyph_p: GLYPH 0xe, 0x9, 0x9, 0xe, 0x8, 0x8, 0x8
glyph_s: GLYPH 0x7, 0x8, 0x8, 0x6, 0x1, 0x1, 0xe
glyph_c: GLYPH 0x7, 0x8, 0x8, 0x8, 0x8, 0x8, 0x7
glyph_0: GLYPH 0x6, 0x9, 0xb, 0xd, 0x9, 0x9, 0x6
glyph_1: GLYPH 0x4, 0xc, 0x4, 0x4, 0x4, 0x4, 0xe
glyph_2: GLYPH 0xe, 0x1, 0x1, 0x6, 0x8, 0x8, 0xf
glyph_3: GLYPH 0xe, 0x1, 0x1, 0x6, 0x1, 0x1, 0xe
glyph_4: GLYPH 0x9, 0x9, 0x9, 0xf, 0x1, 0x1, 0x1
glyph_5: GLYPH 0xf, 0x8, 0x8, 0xe, 0x1, 0x1, 0xe
glyph_6: GLYPH 0x6, 0x8, 0x8, 0xe, 0x9, 0x9, 0x6
glyph_7: GLYPH 0xf, 0x1, 0x2, 0x2, 0x4, 0x4, 0x4
glyph_8: GLYPH 0x6, 0x9, 0x9, 0x6, 0x9, 0x9, 0x6
glyph_9: GLYPH 0x6, 0x9, 0x9, 0x7, 0x1, 0x1, 0x6

align 2, db 0
draw_page_sgp_low: dw 0
draw_page_sgp_high: dw 0
draw_page_dsa_low: dw 0
draw_page_dsa_high: dw 0
draw_page_index: db 0
align 2, db 0
sgp_command_address_low: dw 0
sgp_command_address_high: dw 0
fps_frame_counter: dw 0
fps_value: dw 0
fps_last_second: db 0
saved_video_mode: dw 0
saved_memory_map: db 0
saved_g0_bpp: db 0
saved_g1_bpp: db 0
video_mode_changed: db 0
