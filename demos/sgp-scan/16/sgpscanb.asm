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

bits 16
org 0x100

%define VIDEO_BIOS_INT       0x8f
%define EXT_GRAPHICS_BIOS_INT 0x87
%define KEYBOARD_BIOS_INT    0x82
%define MODE_640X400_G0      0xa000
%define PIXEL_SIZE_G0_1BPP   1
%define COMPOSE_G0_ONLY      3

%ifdef SGPSCAN_B1
%define LINE_X1 100
%define LINE_Y1 100
%define LINE_X2 140
%define LINE_Y2 100
%elifdef SGPSCAN_B2
%define LINE_X1 100
%define LINE_Y1 100
%define LINE_X2 100
%define LINE_Y2 140
%elifdef SGPSCAN_B3
%define LINE_X1 100
%define LINE_Y1 100
%define LINE_X2 140
%define LINE_Y2 140
%else
%define LINE_X1 160
%define LINE_Y1 80
%define LINE_X2 80
%define LINE_Y2 240
%define LINE2_X1 80
%define LINE2_Y1 240
%define LINE2_X2 240
%define LINE2_Y2 240
%define LINE3_X1 240
%define LINE3_Y1 240
%define LINE3_X2 160
%define LINE3_Y2 80
%endif

start:
    push cs
    pop ds
    push cs
    pop es
    ; Configure one visible 640x400, 1-bpp Graphic 0 framebuffer.
    mov bx, MODE_640X400_G0
    mov cx, PIXEL_SIZE_G0_1BPP
    mov dx, 0x0f00
    xor ax, ax
    int VIDEO_BIOS_INT
    test ax, ax
    jnz initialization_failed

    mov ax, 0x0b00
    int VIDEO_BIOS_INT
    mov ax, 0x0900
    int VIDEO_BIOS_INT
    mov ax, 0x0a00
    int VIDEO_BIOS_INT

    mov ax, 0x0100
    mov cx, 1
    mov di, video_framebuffer
    int VIDEO_BIOS_INT
    test ax, ax
    jnz initialization_failed

    mov ax, 0x0200
    mov cx, 1
    mov di, video_window
    int VIDEO_BIOS_INT
    test ax, ax
    jnz initialization_failed

    mov ax, 0x0300
    mov cx, COMPOSE_G0_ONLY
    int VIDEO_BIOS_INT
    test ax, ax
    jnz initialization_failed

    ; Use only the Extended Graphics BIOS for clear and lines.
    mov dx, graphics_framebuffer
    mov ah, 0x01
    int EXT_GRAPHICS_BIOS_INT
    ; SETFRAME has no defined AX status; do not treat the preserved AX as an error.

    mov bx, graphics_view
    xor ax, ax
    mov ah, 0x02
    int EXT_GRAPHICS_BIOS_INT
    test ax, ax
    jnz initialization_failed

    xor cx, cx
    mov ah, 0x10
    int EXT_GRAPHICS_BIOS_INT

    mov bx, line_top
    xor cx, cx
    xor dx, dx
    mov ax, 0x1100               ; AL=PSET, AH=LINE
    int EXT_GRAPHICS_BIOS_INT
%ifdef SGPSCAN_B1
    jmp .lines_done
%elifdef SGPSCAN_B2
    jmp .lines_done
%elifdef SGPSCAN_B3
    jmp .lines_done
%endif
    mov bx, line_bottom
    xor cx, cx
    xor dx, dx
    mov ax, 0x1100
    int EXT_GRAPHICS_BIOS_INT
    mov bx, line_right
    xor cx, cx
    xor dx, dx
    mov ax, 0x1100
    int EXT_GRAPHICS_BIOS_INT
.lines_done:

    mov ax, 0x0b01
    int VIDEO_BIOS_INT

    mov dx, message_done
    mov ah, 0x09
    int 0x21
wait_escape:
    ; Use the PC-88VA keyboard BIOS so ESC is recognized without DOS line
    ; input or an emulator-specific keyboard shortcut.
    mov ah, 0x0a
    int KEYBOARD_BIOS_INT
    jc wait_escape
    mov ah, 0x09
    int KEYBOARD_BIOS_INT
    cmp ah, 0x00
    jne wait_escape
    xor ax, ax
    int VIDEO_BIOS_INT
    mov ax, 0x4c00
    int 0x21

initialization_failed:
    mov dx, message_failed
    mov ah, 0x09
    int 0x21
    jmp wait_escape

message_done:
    db "BIOS LINE-only triangle; press ESC.$"
message_failed:
    db "Graphics BIOS initialization failed.$"

align 2, db 0

video_framebuffer:
    dw 1, 640, 400
video_window:
    dw 0, 0, 400, 0, 0

; Extended Graphics BIOS AH=01h descriptor: single-plane 1-bpp G0.
graphics_framebuffer:
    dw 1
    dw 0, 0xa000
    dw 80, 400

; Extended Graphics BIOS AH=02h descriptor: full 640x400 view.
graphics_view:
    dw 0, 0, 639, 399, 0, 0

; AH=11h single-line records: endpoints, style, frame color, fill color.
line_top:
    dw LINE_X1, LINE_Y1, LINE_X2, LINE_Y2, 0xffff, 1, 1
line_bottom:
%ifdef SGPSCAN_B1
    dw 0, 0, 0, 0, 0, 0, 0
%elifdef SGPSCAN_B2
    dw 0, 0, 0, 0, 0, 0, 0
%elifdef SGPSCAN_B3
    dw 0, 0, 0, 0, 0, 0, 0
%else
    dw LINE2_X1, LINE2_Y1, LINE2_X2, LINE2_Y2, 0xffff, 1, 1
%endif
line_right:
%ifdef SGPSCAN_B1
    dw 0, 0, 0, 0, 0, 0, 0
%elifdef SGPSCAN_B2
    dw 0, 0, 0, 0, 0, 0, 0
%elifdef SGPSCAN_B3
    dw 0, 0, 0, 0, 0, 0, 0
%else
    dw LINE3_X1, LINE3_Y1, LINE3_X2, LINE3_Y2, 0xffff, 1, 1
%endif
