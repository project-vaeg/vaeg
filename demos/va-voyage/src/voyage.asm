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
; MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
; IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
; SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
; WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
; OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
; OF THE POSSIBILITY OF SUCH DAMAGE.

; Independent V3 prototype. No demonstration-disk code/data is used.
; Video setup follows the validated NEON4 8bpp G1/loader contract.
; No DOS INT21, interrupt hooks, undocumented INT91 or sound high bank.
cpu 286
bits 16
org 0

%define PERIOD 900
%define ROWS 12
%define COLS 9
%define LIST_WORDS 4096
%define VIDEO_SEG 0338h
%define PAGE_BYTES 64000
%define PAGE_BASE 0220000h

start:
    push cs
    pop ds
    cld
    call save_video
    ; Hide the inherited guide while the text console is still active.
    mov ax, 2f00h
    int 83h
    mov ax, 01ffh
    int 94h
    push cs
    pop ds
    call video_enter
    jc exit_demo
    call clear_surfaces
    jc exit_demo
    call audio_init
    mov dx, 0142h
    in al, dx
    and al, 40h
    mov [blank_state], al
    mov byte [running], 1
    call music_service

next_frame:
    mov ax, [tick]
    mov [frame_tick], ax
    call build_frame
    call run_sgp
    jc exit_demo
    call wait_next_blank
    jc exit_demo
    call present_page
    xor byte [draw_page], 1
    inc word [frames_drawn]
frame_ready:
    nop                         ; stable local capture checkpoint
    call keyboard_escape
    jc exit_demo
.await_frame:
    mov cx, 0ffffh
.pace:
    call clock_poll
    call music_service
    mov ax, [tick]
    sub ax, [frame_tick]
    jns .delta
    add ax, PERIOD
.delta:
    cmp ax, 3
    jae next_frame
    loop .pace
    mov byte [wait_failed], 1
    jmp exit_demo

exit_demo:
    mov byte [running], 0
    cmp byte [sgp_owned], 0
    je .audio
    mov dx, 0504h
    mov al, 2                ; documented ABORT; no SGP completion IRQ.
    out dx, al
    xor al, al
    out dx, al
.audio:
    call audio_stop
    cmp byte [sgp_owned], 0
    je .video
    call clear_exit_vram
    ; If clearing timed out, abort again before changing video ownership.
    mov dx, 0504h
    mov al, 2
    out dx, al
    xor al, al
    out dx, al
.video:
    call video_leave
returned:
    ; Same continuation as the source-built NEON payload loader.
    cli
    cmp word [cs:0e006h], 5034h
    jne .no_loader
    mov ax, [cs:0e000h]
    mov ss, ax
    mov sp, [cs:0e002h]
    push word [cs:0e004h]
    popf
    retf
.no_loader:
    sti
.halt:
    hlt
    jmp .halt

; Poll edges, not loop iterations. Called during projection, SGP waits and
; OPN busy waits as well as idle pacing; never invokes the music service.
; Preserves flags, making it safe between arithmetic and conditional branches.
clock_poll:
    pushf
    push ax
    push dx
    cmp byte [cs:running], 0
    je .done
    mov dx, 0142h
    in al, dx
    and al, 40h
    cmp al, [cs:blank_state]
    je .done
    mov [cs:blank_state], al
    test al, al
    jz .done
    inc word [cs:tick]
    cmp word [cs:tick], PERIOD
    jb .done
    mov word [cs:tick], 0
    inc word [cs:loops_completed]
.done:
    pop dx
    pop ax
    popf
    ret

; A lost VBlank/status signal must not turn pacing into an infinite wait.
wait_next_blank:
    push bx
    push cx
    mov bx, [tick]
    mov cx, 0ffffh
.poll:
    call clock_poll
    call music_service
    cmp bx, [tick]
    jne .ready
    loop .poll
    mov byte [wait_failed], 1
    stc
    jmp .done
.ready:
    clc
.done:
    pop cx
    pop bx
    ret

keyboard_escape:
    push ds
    push es
    mov ah, 0ah
    int 82h
    jc .none
    mov ah, 09h
    int 82h
    cmp al, 1bh
    je .escape
    cmp ah, 0
    je .escape
.none:
    clc
    jmp .done
.escape:
    stc
.done:
    pop es
    pop ds
    ret

save_video:
    mov dx, 0153h
    in al, dx
    mov [saved_map], al
    mov ax, VIDEO_SEG
    mov es, ax
    mov ax, [es:000fh]
    mov [saved_mode], ax
    mov ax, [es:0011h]
    mov [saved_pixels], ax
    ret

video_enter:
    push ds
    push es
    mov ax, VIDEO_SEG
    mov ds, ax
    mov es, ax
    mov byte [cs:mode_changed], 1
    mov bx, 0e00eh
    mov cx, 0808h
    xor dx, dx
    xor ax, ax
    int 8fh
    test ax, ax
    jnz .failed
    mov ax, 0b00h
    int 8fh
    test ax, ax
    jnz .failed
    mov ax, 0900h
    int 8fh
    test ax, ax
    jnz .failed
    mov ax, 0a00h
    int 8fh
    test ax, ax
    jnz .failed
    mov dx, 0106h
    xor ax, ax
    out dx, ax
    mov dx, 0108h
    mov ax, 0089h             ; G1 over G0 direct colour.
    out dx, ax
    mov ax, 0300h
    mov cx, 0034h
    int 8fh
    test ax, ax
    jnz .failed
    mov dx, 0124h
    xor ax, ax
    out dx, ax
    mov dx, 0126h
    mov ax, 1
    out dx, ax
    ; BIOS default descriptors retained; FB1 backing is 320x400.
    mov dx, 0224h
    mov ax, 320
    out dx, ax
    mov dx, 0228h
    xor ax, ax
    out dx, ax
    add dx, 2
    out dx, ax
    add dx, 2
    out dx, ax
    mov dx, 0232h
    mov ax, 200
    out dx, ax
    mov dx, 0236h
    xor ax, ax
    out dx, ax
    mov dx, 022eh
    out dx, ax
    add dx, 2
    out dx, ax
    mov ax, 0b01h
    xor dx, dx
    int 8fh
    test ax, ax
    jnz .failed
    mov dx, 0153h
    mov al, 54h
    out dx, al
    mov dx, 0580h
    mov al, 10h
    out dx, al
    clc
    jmp .done
.failed:
    mov [cs:video_error], ax
    stc
.done:
    pop es
    pop ds
    ret

video_leave:
    push cs
    pop ds
    mov dx, 0153h
    mov al, [saved_map]
    out dx, al
    cmp byte [mode_changed], 0
    je .text
    push ds
    mov bx, [saved_mode]
    mov cx, [saved_pixels]
    mov ax, VIDEO_SEG
    mov ds, ax
    mov es, ax
    xor ax, ax
    xor dx, dx
    int 8fh
    pop ds
    test ax, ax
    jnz .failed
.text:
    ; Standard console composition, then restore the ten-entry guide.
    push ds
    mov ax, VIDEO_SEG
    mov ds, ax
    mov ax, 0300h
    mov cx, 0031h
    int 8fh
    mov ax, 0b01h
    xor dx, dx
    int 8fh
    pop ds
    mov dx, 0153h
    mov al, [saved_map]
    out dx, al
    mov ax, 2f0ah
    int 83h
    mov ax, 0100h
    int 94h
    push cs
    pop ds
    mov byte [video_restored], 1
    ret
.failed:
    mov [restore_error], ax
    ret

present_page:
    mov dx, 022eh             ; DSA registers are WORD ports.
    xor ax, ax
    cmp byte [draw_page], 0
    je .write
    mov ax, PAGE_BYTES
.write:
    out dx, ax
    add dx, 2
    xor ax, ax
    out dx, ax
    ret

begin_list:
    push cs
    pop es
    mov di, command_list
    mov ax, 3                 ; SET WORK is mandatory per submission.
    stosw
    mov si, work_area
    call physical_address
    stosw
    mov ax, dx
    stosw
    mov word [last_colour], 0ffffh
    ret

end_list:
    mov ax, 1
    stosw
    mov [list_end], di
    mov si, command_list
    call physical_address
    mov [list_address], ax
    mov [list_address+2], dx
    ret

; Clear both G0 and G1 backing surfaces on entry, not just the first page.
clear_surfaces:
    call begin_list
    xor ax, ax
    call colour
    mov ax, 0ah
    stosw
    xor ax, ax
    stosw
    mov ax, 0020h
    stosw
    mov ax, 64000
    stosw
    xor ax, ax
    stosw
    mov ax, 0ah
    stosw
    xor ax, ax
    stosw
    mov ax, 0022h
    stosw
    mov ax, 64000
    stosw
    xor ax, ax
    stosw
    call end_list
    call run_sgp
    ret

; Restoring the console mode reinterprets 8bpp VRAM as its original format.
; Compose 0031h includes G0: leaving our page data produces ghost geometry.
; Clear all 256 KiB while our original packed setup still owns the SGP.
clear_exit_vram:
    call begin_list
    xor ax, ax
    call colour
    mov bx, 20h
.surface:
    mov ax, 0ah
    stosw
    xor ax, ax
    stosw
    mov ax, bx
    stosw
    mov ax, 8000h
    stosw
    xor ax, ax
    stosw
    inc bx
    cmp bx, 24h
    jb .surface
    call end_list
    call run_sgp
    ret

build_frame:
    ; Freeze projection phase at this frame's timestamp; music stays live.
    mov ax, [frame_tick]
    mov bx, 256
    mul bx
    mov bx, PERIOD
    div bx
    mov [scene_phase], al
    mov al, [pulse]
    mov [frame_pulse], al
    xor ax, ax
    mov al, [scene_phase]
    mov bx, ax
    shl bx, 1
    mov ax, [sine_table+bx]
    mov cl, 3
    sar ax, cl
    mov [bank], ax
    xor ax, ax
    mov al, [scene_phase]
    shl al, 1
    xor ah, ah
    mov bx, ax
    shl bx, 1
    mov ax, [sine_table+bx]
    mov cl, 4
    sar ax, cl
    mov [heave], ax
    mov al, [scene_phase]
    xor ah, ah
    mov cl, 3
    shl ax, cl
    mov [forward_phase], ax
    call project_mesh
    call begin_list
    xor ax, ax
    call colour
    ; Only the hidden 320x200 G1 page is cleared each frame.
    mov ax, 0ah
    stosw
    call page_address
    stosw
    mov ax, dx
    stosw
    mov ax, PAGE_BYTES/2
    stosw
    xor ax, ax
    stosw
    call emit_stars
    mov byte [row_index], 0
    mov si, mesh
.row:
    mov byte [col_index], 0
    ; Colour follows projected distance, avoiding a colour jump on wrap.
    mov bx, [si+2]
    sub bx, 50
    shr bx, 1
    and bx, 7
    mov al, [sea_colours+bx]
    cmp byte [frame_pulse], 10
    jb .dim
    or al, 20h
.dim:
    mov ah, al
    call colour
.point:
    mov ax, [si]
    mov [x1], ax
    mov ax, [si+2]
    mov [y1], ax
    cmp byte [col_index], COLS-1
    je .vertical
    mov ax, [si+4]
    mov [x2], ax
    mov ax, [si+6]
    mov [y2], ax
    call line
.vertical:
    cmp byte [row_index], ROWS-1
    je .advance
    mov ax, [si+COLS*4]
    mov [x2], ax
    mov ax, [si+COLS*4+2]
    mov [y2], ax
    call line
.advance:
    add si, 4
    call clock_poll
    inc byte [col_index]
    cmp byte [col_index], COLS
    jb .point
    call music_service
    inc byte [row_index]
    cmp byte [row_index], ROWS
    jb .row
    call emit_title
    call end_list
    ret

project_mesh:
    mov di, mesh
    mov byte [row_index], 0
.row:
    ; Q8 depth decreases toward the camera; reciprocal gives Q7 scale.
    xor ax, ax
    mov al, [row_index]
    shl ax, 1
    shl ax, 1
    mov bx, 50
    sub bx, ax
    mov cl, 8
    shl bx, cl
    mov ax, [forward_phase]
    xor ah, ah
    shl ax, 1
    shl ax, 1
    sub bx, ax
    mov dx, 000ch             ; 819200 / depth_q8
    mov ax, 8000h
    div bx
    cmp ax, 448
    jbe .scale_ok
    mov ax, 448
.scale_ok:
    mov [scale], ax
    mov byte [col_index], 0
.point:
    xor bx, bx
    mov bl, [col_index]
    shl bx, 1
    mov ax, [world_columns+bx]
    imul word [scale]         ; |world|<=40; |product|<=17920.
    mov cl, 7
    sar ax, cl
    mov [projected_x], ax
    ; Travelling waves: world-row identity survives forward strip wrapping.
    mov al, [row_index]
    sub al, [forward_phase+1]
    mov cl, 5
    shl al, cl
    mov bl, [scene_phase]
    add al, bl
    add al, bl
    add al, bl
    add al, bl
    add al, bl
    mov bl, [col_index]
    mov ah, 13
    ; 13*col without corrupting the wave accumulator.
    mov [wave_phase], al
    mov al, bl
    mul ah
    add al, [wave_phase]
    xor ah, ah
    mov bx, ax
    shl bx, 1
    mov ax, [sine_table+bx]
    ; Divide amplitude before multiplication to stay in signed 16 bits.
    mov cl, 2
    sar ax, cl
    imul word [scale]
    mov cl, 10
    sar ax, cl
    mov bx, [scale]
    shr bx, 1
    shr bx, 1
    add ax, bx
    add ax, 58
    add ax, [heave]
    mov [projected_y], ax
    ; Small-angle bank around the horizon; signed Q7/Q6 transforms.
    mov ax, [projected_x]
    imul word [bank]
    mov cl, 7
    sar ax, cl
    add ax, [projected_y]
    call clamp_y
    mov [di+2], ax
    mov ax, [projected_y]
    sub ax, 58
    imul word [bank]
    mov cl, 6
    sar ax, cl
    add ax, [projected_x]
    add ax, 160
    call clamp_x
    mov [di], ax
    add di, 4
    call clock_poll
    inc byte [col_index]
    cmp byte [col_index], COLS
    jb .point
    call music_service
    inc byte [row_index]
    cmp byte [row_index], ROWS
    jb .row
    ret

clamp_x:
    cmp ax, 0
    jge .upper
    xor ax, ax
.upper:
    cmp ax, 319
    jle .done
    mov ax, 319
.done:
    ret
clamp_y:
    cmp ax, 0
    jge .upper
    xor ax, ax
.upper:
    cmp ax, 199
    jle .done
    mov ax, 199
.done:
    ret

emit_stars:
    mov ax, 7373h
    call colour
    mov si, stars
    mov bp, 14
.next:
    lodsw
    mov [x1], ax
    inc ax
    mov [x2], ax
    lodsw
    mov [y1], ax
    mov [y2], ax
    call line
    dec bp
    jnz .next
    ret

emit_title:
    mov ax, 0f7f7h
    call colour
    mov si, title_lines
    mov bp, 9
.next:
    lodsw
    mov [x1], ax
    lodsw
    mov [y1], ax
    lodsw
    mov [x2], ax
    lodsw
    mov [y2], ax
    call line
    dec bp
    jnz .next
    ret

colour:
    cmp ax, [last_colour]
    je .done
    mov [last_colour], ax
    push ax
    mov ax, 6
    stosw
    pop ax
    stosw
.done:
    ret

; Documented SGP LINE descriptor: XY direction bits 0400h/0800h.
; All endpoints are within the backing/display rectangle. This prototype
; clamps projected vertices at the border rather than geometric line clipping.
line:
    push ax
    push bx
    push cx
    push dx
    mov ax, 9
    stosw
    mov bx, 5
    mov ax, [x2]
    sub ax, [x1]
    jns .positive_x
    neg ax
    or bx, 0400h
.positive_x:
    inc ax
    mov [line_width], ax
    mov ax, [y2]
    sub ax, [y1]
    jns .positive_y
    neg ax
    or bx, 0800h
.positive_y:
    inc ax
    mov cx, ax
    mov ax, bx
    stosw
    mov ax, [x1]
    and ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    or ax, 2
    stosw
    mov ax, [line_width]
    stosw
    mov ax, cx
    stosw
    mov ax, 320
    stosw
    mov ax, [y1]
    mov bx, 320
    mul bx
    mov bx, [x1]
    and bx, 0fffeh
    add ax, bx
    adc dx, 0
    push ax
    push dx
    call page_address
    pop cx
    pop bx
    add ax, bx
    adc dx, cx
    stosw
    mov ax, dx
    stosw
    pop dx
    pop cx
    pop bx
    pop ax
    ret

page_address:
    mov ax, PAGE_BASE & 0ffffh
    mov dx, PAGE_BASE >> 16
    cmp byte [draw_page], 0
    je .done
    add ax, PAGE_BYTES
    adc dx, 0
.done:
    ret

physical_address:
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

run_sgp:
    ; A static maximum is also checked by the generator/test/build contract.
    cmp word [list_end], command_list + LIST_WORDS*2
    ja .failed
    call wait_sgp
    jc .failed
    mov byte [sgp_owned], 1
    mov dx, 0580h
    mov al, 10h
    out dx, al
    mov dx, 0500h
    mov ax, [list_address]
    out dx, ax
    add dx, 2
    mov ax, [list_address+2]
    out dx, ax
    mov dx, 0504h
    xor al, al
    out dx, al
    mov dx, 0506h
    mov al, 1
    out dx, al
    call wait_sgp
    ret
.failed:
    mov byte [wait_failed], 1
    stc
    ret

wait_sgp:
    push cx
    push dx
    mov cx, 0ffffh
.poll:
    call clock_poll
    call music_service
    mov dx, 0506h
    in al, dx
    test al, 1
    jz .ready
    loop .poll
    mov byte [wait_failed], 1
    stc
    jmp .done
.ready:
    clc
.done:
    pop dx
    pop cx
    ret

%include "voyage_audio.inc"
%include "voyage_tables.inc"

align 2, db 0
world_columns: dw -40,-30,-20,-10,0,10,20,30,40
sea_colours: db 0bh,0fh,2fh,4fh,6fh,8fh,0afh,0cfh
; Original single-stroke VA title and frame, deliberately not a sprite yet.
title_lines:
    dw 131,17,142,37, 142,37,153,17
    dw 161,37,171,17, 171,17,181,37, 165,29,177,29
    dw 118,10,194,10, 118,44,194,44, 118,10,118,44, 194,10,194,44
stars:
    dw 16,12, 44,31, 61,8, 86,23, 102,42, 27,49, 73,52
    dw 209,9, 239,32, 260,15, 283,42, 304,6, 220,50, 310,53

running: db 0
blank_state: db 0
scene_phase: db 0
wave_phase: db 0
frame_pulse: db 0
sgp_owned: db 0
row_index: db 0
col_index: db 0
draw_page: db 1
mode_changed: db 0
video_restored: db 0
wait_failed: db 0
saved_map: db 0
align 2, db 0
tick: dw 0
frame_tick: dw 0
frames_drawn: dw 0
loops_completed: dw 0
forward_phase: dw 0
scale: dw 0
bank: dw 0
heave: dw 0
projected_x: dw 0
projected_y: dw 0
saved_mode: dw 0
saved_pixels: dw 0
video_error: dw 0
restore_error: dw 0
last_colour: dw 0
list_address: dd 0
list_end: dw 0
x1: dw 0
y1: dw 0
x2: dw 0
y2: dw 0
line_width: dw 0
mesh: times ROWS*COLS*2 dw 0
work_area: times 64 dw 0
command_list: times LIST_WORDS dw 0
payload_end:
%if ($-$$) >= 0d000h
%error "VOYAGE payload overlaps loader reserve/stack margin"
%endif
