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

bits 16
org 0x100

%define VIDEO_BIOS_INT          0x8f
%define KEYBOARD_BIOS_INT       0x82
%define VIDEO_BIOS_DATA_SEG     0x0338
%define VIDEO_MODE_OFFSET       0x000f
%define VIDEO_G0_BPP_OFFSET     0x0011
%define VIDEO_G1_BPP_OFFSET     0x0012

%define PORT_MEMORY_MAP         0x0153
%define PORT_GVRAM_WRITE_MODE   0x0580
%define PORT_SGP_COMMAND        0x0500
%define PORT_SGP_CONTROL        0x0504
%define PORT_SGP_STATUS         0x0506
%define PORT_FB0_DSA_LOW        0x020e
%define PORT_SGP_BUSY           0x01
%define MEMORY_MAP_GVRAM_SINGLE 0x54
%define GVRAM_CPU_WRITE_MODE    0x10

%define MODE_640X400_G0         0xa000
%define PIXEL_SIZE_G0_4BPP      4
%define COMPOSE_G0_ONLY         3
%define SCREEN_PITCH            320
%define G0_SGP_BASE             0x200000
%define PAGE_WORD_COUNT         0xfa00

%define SGP_COMMAND_END         0x0001
%define SGP_COMMAND_SET_WORK    0x0003
%define SGP_COMMAND_SET_SOURCE  0x0004
%define SGP_COMMAND_SET_DEST    0x0005
%define SGP_COMMAND_SET_COLOR   0x0006
%define SGP_COMMAND_PATBLT      0x0008
%define SGP_COMMAND_LINE        0x0009
%define SGP_COMMAND_CLS         0x000a
%define SGP_COMMAND_SCAN_RIGHT  0x000b
%define SGP_COMMAND_SCAN_LEFT   0x000c
%define SGP_LINE_COPY           0x0005
%define SGP_PATBLT_COPY         0x0005
%define SCAN_BOUNDARY_COLOR     0xffff
%define SCAN_FILL_COLOR         0x7777

%define TEST_LEFT_X             100
%define TEST_RIGHT_X            200
%define TEST_START_X            150

start:
    push cs
    pop ds
    push cs
    pop es
    cld

    call save_video_state
    ; Video BIOS descriptor pointers are ES:DI. save_video_state uses ES for
    ; its data segment, so restore ES before passing descriptors.
    push ds
    pop es
    call initialize_video
    jc initialization_failed
    call build_command_list
    call run_sgp
    jc sgp_failed

    mov dx, message_done
    mov ah, 0x09
    int 0x21
wait_escape:
    ; Use the VA keyboard BIOS, not DOS line input, for deterministic ESC exit.
    mov ah, 0x0a
    int KEYBOARD_BIOS_INT
    jc wait_escape
    mov ah, 0x09
    int KEYBOARD_BIOS_INT
    cmp ah, 0
    jne wait_escape
    call restore_video_state
    mov ax, 0x4c00
    int 0x21

initialization_failed:
    mov dx, message_init_failed
    mov ah, 0x09
    int 0x21
    call restore_video_state
    mov ax, 0x4c01
    int 0x21

sgp_failed:
    mov dx, message_sgp_failed
    mov ah, 0x09
    int 0x21
    call restore_video_state
    mov ax, 0x4c02
    int 0x21

message_done:
    db "SGP SCAN LEFT/RIGHT sanity; press ESC.$"
message_init_failed:
    db "Video initialization failed.$"
message_sgp_failed:
    db "SGP did not become idle.$"

; Configure one 640x400, 4-bpp single-plane Graphic 0 surface.
initialize_video:
    mov bx, MODE_640X400_G0
    mov cx, PIXEL_SIZE_G0_4BPP
    xor dx, dx
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

    mov ax, 0x0100
    mov cx, 1
    mov di, framebuffer_descriptor
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    mov ax, 0x0200
    mov cx, 1
    mov di, window_descriptor
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed

    mov ax, 0x080f
    mov cx, 0xffff
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    mov ax, 0x0807
    ; RGB565(20,20,20), matching the emulator's VA palette field order.
    mov cx, 0x5294
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    mov ax, 0x0300
    mov cx, COMPOSE_G0_ONLY
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed

    mov dx, PORT_MEMORY_MAP
    mov al, MEMORY_MAP_GVRAM_SINGLE
    out dx, al
    mov dx, PORT_GVRAM_WRITE_MODE
    mov al, GVRAM_CPU_WRITE_MODE
    out dx, al
    mov dx, PORT_FB0_DSA_LOW
    xor ax, ax
    out dx, ax
    add dx, 2
    out dx, ax
    mov ax, 0x0b01
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    clc
    ret
.failed:
    stc
    ret

build_command_list:
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

    ; SET WORK is required before any drawing command.
    mov ax, SGP_COMMAND_SET_WORK
    stosw
    mov si, sgp_work_area
    call physical_address_from_ds_si
    stosw
    mov ax, dx
    stosw

    ; Clear one displayed page.
    mov ax, SGP_COMMAND_SET_COLOR
    stosw
    xor ax, ax
    stosw
    mov ax, SGP_COMMAND_CLS
    stosw
    mov ax, G0_SGP_BASE & 0xffff
    stosw
    mov ax, G0_SGP_BASE >> 16
    stosw
    mov ax, PAGE_WORD_COUNT
    stosw
    xor ax, ax
    stosw

    ; White is both the independent boundary color and the scan target.
    mov ax, SGP_COMMAND_SET_COLOR
    stosw
    mov ax, SCAN_BOUNDARY_COLOR
    stosw

    ; Independent source image: vertical boundaries, made only by LINE.
    mov word [line_x1], TEST_LEFT_X
    mov word [line_y1], 80
    mov word [line_x2], TEST_LEFT_X
    mov word [line_y2], 159
    call emit_line
    mov word [line_x1], TEST_RIGHT_X
    mov word [line_y1], 80
    mov word [line_x2], TEST_RIGHT_X
    mov word [line_y2], 159
    call emit_line

    ; A separate four-boundary region proves nearest-boundary selection.
    mov word [line_x1], 80
    mov word [line_y1], 170
    mov word [line_x2], 80
    mov word [line_y2], 199
    call emit_line
    mov word [line_x1], 100
    mov word [line_y1], 170
    mov word [line_x2], 100
    mov word [line_y2], 199
    call emit_line
    mov word [line_x1], 200
    mov word [line_y1], 170
    mov word [line_x2], 200
    mov word [line_y2], 199
    call emit_line
    mov word [line_x1], 220
    mov word [line_y1], 170
    mov word [line_x2], 220
    mov word [line_y2], 199
    call emit_line

    ; The adjacent-boundary region exercises one-step/off-by-one behavior.
    mov word [line_x1], 149
    mov word [line_y1], 210
    mov word [line_x2], 149
    mov word [line_y2], 239
    call emit_line
    mov word [line_x1], 151
    mov word [line_y1], 210
    mov word [line_x2], 151
    mov word [line_y2], 239
    call emit_line

    ; PATBLT uses a 1x1, 1bpp all-one source, independent of SCAN.
    mov ax, SGP_COMMAND_SET_SOURCE
    stosw
    xor ax, ax
    stosw
    mov ax, 1
    stosw
    stosw
    mov ax, 4
    stosw
    mov si, fill_pattern
    call physical_address_from_ds_si
    stosw
    mov ax, dx
    stosw

    ; Three separated bands between x=100 and x=200.
    mov ax, 90
    mov cx, 10
.band_a:
    push cx
    call emit_scan_band
    inc ax
    pop cx
    loop .band_a

    mov ax, 110
    mov cx, 10
.band_a2:
    push cx
    call emit_scan_band
    inc ax
    pop cx
    loop .band_a2

    mov ax, 130
    mov cx, 10
.band_a3:
    push cx
    call emit_scan_band
    inc ax
    pop cx
    loop .band_a3

    ; Nearest-boundary band: x=100 and x=200 must win over x=80/x=220.
    mov ax, 175
    mov cx, 10
.band_b:
    push cx
    call emit_scan_band
    inc ax
    pop cx
    loop .band_b

    ; Adjacent-boundary band: the only interior pixel is x=150.
    mov ax, 215
    mov cx, 10
.band_c:
    push cx
    call emit_scan_band
    inc ax
    pop cx
    loop .band_c

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

; Emit one row of two scans and two PATBLTs. AX is the row.
emit_scan_band:
    push ax
    mov [band_y], ax

    mov ax, SGP_COMMAND_SET_COLOR
    stosw
    mov ax, SCAN_BOUNDARY_COLOR
    stosw

    ; SCAN_RIGHT starts at x=150 and searches through x=200 inclusive.
    mov word [dest_x], TEST_START_X
    mov ax, [band_y]
    mov [dest_y], ax
    mov word [dest_width], 51
    call emit_set_destination
    mov ax, SGP_COMMAND_SCAN_RIGHT
    stosw
    ; Keep the boundary color for SCAN, but use a distinct gray fill color.
    mov ax, SGP_COMMAND_SET_COLOR
    stosw
    mov ax, SCAN_FILL_COLOR
    stosw
    mov ax, SGP_COMMAND_PATBLT
    stosw
    mov ax, SGP_PATBLT_COPY
    stosw

    ; SCAN_LEFT records the interior's left edge and width.
    mov ax, SGP_COMMAND_SET_COLOR
    stosw
    mov ax, SCAN_BOUNDARY_COLOR
    stosw
    mov word [dest_x], TEST_START_X
    mov ax, [band_y]
    mov [dest_y], ax
    mov word [dest_width], 51
    call emit_set_destination
    mov ax, SGP_COMMAND_SCAN_LEFT
    stosw
    mov ax, SGP_COMMAND_SET_COLOR
    stosw
    mov ax, SCAN_FILL_COLOR
    stosw
    mov ax, SGP_COMMAND_PATBLT
    stosw
    mov ax, SGP_PATBLT_COPY
    stosw
    pop ax
    ret

emit_set_destination:
    push ax
    push bx
    push dx
    mov ax, SGP_COMMAND_SET_DEST
    stosw
    mov ax, [dest_x]
    and ax, 3
    shl ax, 4
    or ax, 1
    stosw
    mov ax, [dest_width]
    stosw
    mov ax, 1
    stosw
    mov ax, SCREEN_PITCH
    stosw
    mov ax, [dest_y]
    mov bx, SCREEN_PITCH
    mul bx
    mov bx, [dest_x]
    and bx, 0xfffc
    shr bx, 1
    add ax, bx
    adc dx, 0
    add ax, G0_SGP_BASE & 0xffff
    adc dx, G0_SGP_BASE >> 16
    stosw
    mov ax, dx
    stosw
    pop dx
    pop bx
    pop ax
    ret

emit_line:
    push ax
    push bx
    push dx
    mov ax, SGP_COMMAND_LINE
    stosw
    mov bx, SGP_LINE_COPY
    mov ax, [line_x2]
    sub ax, [line_x1]
    jns .x_positive
    neg ax
    or bx, 0x0400
.x_positive:
    inc ax
    mov [line_width], ax
    mov ax, [line_y2]
    sub ax, [line_y1]
    jns .y_positive
    neg ax
    or bx, 0x0800
.y_positive:
    inc ax
    mov [line_height], ax
    mov ax, bx
    stosw
    mov ax, [line_x1]
    and ax, 3
    shl ax, 4
    or ax, 1
    stosw
    mov ax, [line_width]
    stosw
    mov ax, [line_height]
    stosw
    mov ax, SCREEN_PITCH
    stosw
    mov ax, [line_y1]
    mov bx, SCREEN_PITCH
    mul bx
    mov bx, [line_x1]
    and bx, 0xfffc
    shr bx, 1
    add ax, bx
    adc dx, 0
    add ax, G0_SGP_BASE & 0xffff
    adc dx, G0_SGP_BASE >> 16
    stosw
    mov ax, dx
    stosw
    pop dx
    pop bx
    pop ax
    ret

run_sgp:
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
    mov al, PORT_SGP_BUSY
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
    test al, PORT_SGP_BUSY
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
    mov bx, [saved_video_mode]
    mov cl, [saved_g0_bpp]
    mov ch, [saved_g1_bpp]
    xor dx, dx
    xor ax, ax
    int VIDEO_BIOS_INT
    ret

align 2, db 0
framebuffer_descriptor:
    dw 4, 640, 400
window_descriptor:
    dw 0, 0, 400, 0, 0

align 2, db 0
fill_pattern:
    dw 0xffff
sgp_command_list:
    times 8192 dw 0
sgp_work_area:
    times 58 db 0
sgp_command_address_low: dw 0
sgp_command_address_high: dw 0
saved_video_mode: dw 0
saved_g0_bpp: db 0
saved_g1_bpp: db 0
band_y: dw 0
dest_x: dw 0
dest_y: dw 0
dest_width: dw 0
line_x1: dw 0
line_y1: dw 0
line_x2: dw 0
line_y2: dw 0
line_width: dw 0
line_height: dw 0
