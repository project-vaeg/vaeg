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

%define KEYBOARD_BIOS_INT       0x82
%define VIDEO_BIOS_INT          0x8f
%define VIDEO_BIOS_DATA_SEG     0x0338
%define VIDEO_MODE_OFFSET       0x000f
%define VIDEO_G0_BPP_OFFSET     0x0011
%define VIDEO_G1_BPP_OFFSET     0x0012

%define PORT_MEMORY_MAP         0x0153
%define PORT_GRRES              0x0102
%define PORT_GRMODE             0x0100
%define PORT_PALETTE_COMPOSE    0x0106
%define PORT_RGB_COMPOSE        0x0108
%define PORT_FB0_FBW            0x0204
%define PORT_FB0_FBL            0x0206
%define PORT_FB0_DOT            0x0208
%define PORT_FB0_OFX            0x020a
%define PORT_FB0_OFY            0x020c
%define PORT_FB0_DSA_LOW        0x020e
%define PORT_FB0_DSA_HIGH       0x0210
%define PORT_FB0_DSH            0x0212
%define PORT_FB0_DSP            0x0216
%define PORT_TSP_STATUS         0x0142
%define PORT_SGP_COMMAND        0x0500
%define PORT_SGP_CONTROL        0x0504
%define PORT_SGP_STATUS         0x0506
%define PORT_GVRAM_WRITE_MODE   0x0580

%define MEMORY_MAP_GVRAM_SINGLE 0x54
%define GVRAM_CPU_WRITE_MODE    0x10
%define MODE_320X200_G0_ONLY    0xa00e
%define PIXEL_SIZE_G0_16BPP     0x0010
%define COMPOSE_G0_DIRECT       0x0008
%define TSP_STATUS_VBLANK       0x40

; Two 320x200 pages are stacked vertically in the 320x400 16-bpp G0 source.
; SGP renders the hidden page and FB0 DSA selects the page shown by the TSP.
%define SCREEN_WIDTH            320
%define SCREEN_HEIGHT           200
%define SCREEN_PITCH            640
%define G0_PAGE_A_SGP_BASE      0x200000
%define G0_PAGE_B_SGP_BASE      0x21f400
%define G0_PAGE_A_DSA           0x000000
%define G0_PAGE_B_DSA           0x01f400
%define SCREEN_WORD_COUNT       0xfa00

%define SGP_COMMAND_END         0x0001
%define SGP_COMMAND_SET_WORK    0x0003
%define SGP_COMMAND_SET_SOURCE  0x0004
%define SGP_COMMAND_SET_DEST    0x0005
%define SGP_COMMAND_SET_COLOR   0x0006
%define SGP_COMMAND_BITBLT      0x0007
%define SGP_COMMAND_LINE        0x0009
%define SGP_COMMAND_CLS         0x000a
%define SGP_BITBLT_COPY_XPAR    0x0105
%define SGP_LINE_COPY           0x0005
%define SGP_LINE_HD             0x0400
%define SGP_LINE_VD             0x0800
%define SGP_BUSY                0x01

%define SPRITE_MIN_COUNT        1
%define SPRITE_MAX_COUNT        128
%define SPRITE_WIDTH            24
%define SPRITE_HEIGHT           24
%define SPRITE_PITCH            48
%define SPRITE_RECORD_SIZE      8
; VAEG direct-color uses 5-bit red, 6-bit green, and 5-bit blue.
%define GRID_COLOR              0xffff

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
    ; Render the hidden page, then exchange DSA only after SGP is idle.
    call wait_vblank_start
    jc animation_failed
    call update_sprites
    call build_frame_commands
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
    mov bx, MODE_320X200_G0_ONLY
    mov cx, PIXEL_SIZE_G0_16BPP
    xor dx, dx
    mov byte [video_mode_changed], 1
    xor ax, ax
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    mov dx, PORT_GRMODE
    mov ax, 0xb462
    out dx, ax
    mov dx, PORT_GRRES
    mov ax, 0x1313
    out dx, ax
    call define_g0_surface
    jc .failed
    call configure_g0_framebuffer
    jc .failed
    ; Keep page A displayed while the first complete frame is rendered on B.
    mov byte [draw_page_index], 1
    call select_draw_page
    xor ax, ax
    mov dx, PORT_PALETTE_COMPOSE
    out dx, ax
    mov ax, COMPOSE_G0_DIRECT
    mov dx, PORT_RGB_COMPOSE
    out dx, ax
    mov dx, PORT_MEMORY_MAP
    mov al, MEMORY_MAP_GVRAM_SINGLE
    out dx, al
    mov dx, PORT_GVRAM_WRITE_MODE
    mov al, GVRAM_CPU_WRITE_MODE
    out dx, al
    mov ax, 0x0b01
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    clc
    ret
.failed:
    stc
    ret

define_g0_surface:
    push es
    push ds
    pop es
    mov ax, 0x0100
    mov cx, 1
    mov di, g0_framebuffer_descriptor
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    mov ax, 0x0200
    mov cx, 1
    mov di, g0_window_descriptor
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    pop es
    clc
    ret
.failed:
    pop es
    stc
    ret

configure_g0_framebuffer:
    push ax
    push dx
    mov dx, PORT_FB0_FBW
    mov ax, SCREEN_PITCH
    out dx, ax
    mov dx, PORT_FB0_FBL
    mov ax, SCREEN_HEIGHT * 2
    out dx, ax
    mov dx, PORT_FB0_DOT
    xor ax, ax
    out dx, ax
    mov dx, PORT_FB0_OFX
    out dx, ax
    mov dx, PORT_FB0_OFY
    out dx, ax
    mov dx, PORT_FB0_DSA_LOW
    out dx, ax
    mov dx, PORT_FB0_DSA_HIGH
    out dx, ax
    mov dx, PORT_FB0_DSH
    mov ax, SCREEN_HEIGHT
    out dx, ax
    mov dx, PORT_FB0_DSP
    xor ax, ax
    out dx, ax
    pop dx
    pop ax
    clc
    ret

select_draw_page:
    cmp byte [draw_page_index], 0
    je .page_a
    mov word [draw_page_sgp_low], G0_PAGE_B_SGP_BASE & 0xffff
    mov word [draw_page_sgp_high], G0_PAGE_B_SGP_BASE >> 16
    mov word [draw_page_dsa_low], G0_PAGE_B_DSA & 0xffff
    mov word [draw_page_dsa_high], G0_PAGE_B_DSA >> 16
    ret
.page_a:
    mov word [draw_page_sgp_low], G0_PAGE_A_SGP_BASE & 0xffff
    mov word [draw_page_sgp_high], G0_PAGE_A_SGP_BASE >> 16
    mov word [draw_page_dsa_low], G0_PAGE_A_DSA & 0xffff
    mov word [draw_page_dsa_high], G0_PAGE_A_DSA >> 16
    ret

display_draw_page:
    mov dx, PORT_FB0_DSA_LOW
    mov ax, [draw_page_dsa_low]
    out dx, ax
    mov dx, PORT_FB0_DSA_HIGH
    mov ax, [draw_page_dsa_high]
    out dx, ax
    ret

update_sprites:
    push ax
    push bx
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
    pop bx
    pop ax
    ret

build_frame_commands:
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
    cmp byte [background_grid_enabled], 0
    je .sprites
    call emit_background_grid
.sprites:
    mov si, sprite_records
    mov cx, [active_sprite_count]
.sprite:
    push cx
    call emit_sprite
    pop cx
    add si, SPRITE_RECORD_SIZE
    loop .sprite

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
    mov ax, 3
    stosw
    mov ax, SPRITE_WIDTH
    stosw
    mov ax, SPRITE_HEIGHT
    stosw
    mov ax, SPRITE_PITCH
    stosw
    mov bx, [bp + 6]
    mov si, bx
    call physical_address_from_ds_si
    stosw
    mov ax, dx
    stosw

    mov ax, SGP_COMMAND_SET_DEST
    stosw
    mov ax, 3
    stosw
    mov ax, SPRITE_WIDTH
    stosw
    mov ax, SPRITE_HEIGHT
    stosw
    mov ax, SCREEN_PITCH
    stosw
    mov ax, [bp + 2]
    mov bx, [bp]
    mov dx, SCREEN_PITCH
    mul dx
    shl bx, 1
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

emit_background_grid:
    push ax
    push bx
    push cx
    mov ax, GRID_COLOR
    call emit_set_color
    xor bx, bx
    mov cx, 20
.vertical:
    mov [line_x1], bx
    mov [line_x2], bx
    mov word [line_y1], 0
    mov word [line_y2], SCREEN_HEIGHT - 1
    call emit_line
    add bx, 16
    loop .vertical
    xor bx, bx
    mov cx, 13
.horizontal:
    mov word [line_x1], 0
    mov word [line_x2], SCREEN_WIDTH - 1
    mov [line_y1], bx
    mov [line_y2], bx
    call emit_line
    add bx, 16
    loop .horizontal
    pop cx
    pop bx
    pop ax
    ret

emit_set_color:
    push ax
    mov ax, SGP_COMMAND_SET_COLOR
    stosw
    pop ax
    stosw
    ret

emit_line:
    push ax
    push bx
    push cx
    push dx
    mov ax, SGP_COMMAND_LINE
    stosw
    mov bx, SGP_LINE_COPY
    mov ax, [line_x2]
    sub ax, [line_x1]
    jns .x_positive
    neg ax
    or bx, SGP_LINE_HD
.x_positive:
    inc ax
    mov [line_width], ax
    mov ax, [line_y2]
    sub ax, [line_y1]
    jns .y_positive
    neg ax
    or bx, SGP_LINE_VD
.y_positive:
    inc ax
    mov [line_height], ax
    mov ax, bx
    stosw
    mov ax, 3
    stosw
    mov ax, [line_width]
    stosw
    mov ax, [line_height]
    stosw
    mov ax, SCREEN_PITCH
    stosw
    mov ax, [line_y1]
    mov dx, SCREEN_PITCH
    mul dx
    mov bx, [line_x1]
    shl bx, 1
    add ax, bx
    adc dx, 0
    add ax, [draw_page_sgp_low]
    adc dx, [draw_page_sgp_high]
    stosw
    mov ax, dx
    stosw
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
    cmp al, ' '
    je .toggle_grid
.none:
    clc
    ret
.increase_count:
    cmp word [active_sprite_count], SPRITE_MAX_COUNT
    jae .none
    inc word [active_sprite_count]
    clc
    ret
.decrease_count:
    cmp word [active_sprite_count], SPRITE_MIN_COUNT
    jbe .none
    dec word [active_sprite_count]
    clc
    ret
.toggle_grid:
    xor byte [background_grid_enabled], 1
    clc
    ret
.escape:
    stc
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
    test al, 0x10
    jz .bios_state
    mov byte [saved_single_plane], 1
    mov dx, PORT_GVRAM_WRITE_MODE
    in al, dx
    mov [saved_write_mode], al
.bios_state:
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
    je .write_mode
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
.write_mode:
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

message_start:
    db "SGP 65536-color double-buffered pseudo-sprite demo", 13, 10
    db "G0 direct-color 16-bpp; 16 HSV ray-traced spheres.", 13, 10
    db "UP/DOWN: count 1-128. SPACE: grid on/off. ESC exits.", 13, 10, "$"
message_done:
    db "Video state restored.", 13, 10, "$"
message_initialization_failed:
    db "Video or SGP initialization failed.", 13, 10, "$"
message_animation_failed:
    db "SGP synchronization failed.", 13, 10, "$"

align 2, db 0
g0_framebuffer_descriptor:
    dw 16, SCREEN_WIDTH, SCREEN_HEIGHT * 2
g0_window_descriptor:
    dw 0, 0, SCREEN_HEIGHT, 0, 0

align 2, db 0
sgp_command_list:
    times 4096 dw 0
sgp_work_area:
    times 29 dw 0

align 2, db 0
sprite_records:
%macro SPRITE_RECORD 5
    dw %1, %2
    db %3, %4
    dw %5
%endmacro
%include "sprite_records_128_random16.inc"
active_sprite_count:
    dw 1

align 2, db 0
%include "orb_hsv16_24.inc"

align 2, db 0
draw_page_sgp_low: dw 0
draw_page_sgp_high: dw 0
draw_page_dsa_low: dw 0
draw_page_dsa_high: dw 0
draw_page_index: db 0
align 2, db 0
sgp_command_address_low: dw 0
sgp_command_address_high: dw 0
sgp_work_address_low: dw 0
sgp_work_address_high: dw 0
saved_video_mode: dw 0
saved_memory_map: db 0
saved_write_mode: db 0
saved_single_plane: db 0
saved_g0_bpp: db 0
saved_g1_bpp: db 0
video_mode_changed: db 0
background_grid_enabled: db 0
line_x1: dw 0
line_y1: dw 0
line_x2: dw 0
line_y2: dw 0
line_width: dw 0
line_height: dw 0
