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
; IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
; WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
; DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
; INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
; (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
; SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
; HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
; STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
; IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
; POSSIBILITY OF SUCH DAMAGE.

bits 16
org 0x100

start:
    push cs
    pop ds
    push cs
    pop es
    cld

    mov dx, message_title
    call print_string

    xor ax, ax
    mov ah, 0x30
    int 0x21
    mov [cs:dos_major], al
    mov [cs:dos_minor], ah

    mov ax, 0x352f
    int 0x21
    mov [cs:old_int2f], bx
    mov [cs:old_int2f + 2], es
    mov ax, es
    or ax, bx
    jz .no_original_int2f

    mov ax, 0x1100
    int 0x2f
    pushf
    pop bx
    mov [cs:before_ax], ax
    mov [cs:before_flags], bx
    push cs
    pop ds
    jmp .original_int2f_checked

.no_original_int2f:
    mov word [cs:before_ax], 0xffff
    mov word [cs:before_flags], 1

.original_int2f_checked:

    xor dx, dx
    mov ax, 0x5d06
    int 0x21
    pushf
    pop bx
    mov [cs:sda_ax], ax
    mov [cs:sda_flags], bx
    mov [cs:sda_segment], ds
    mov [cs:sda_offset], si
    push cs
    pop ds

    push ds
    push cs
    pop ds
    mov dx, int2f_handler
    mov ax, 0x252f
    int 0x21
    pop ds

    mov ax, 0x1100
    int 0x2f
    pushf
    pop bx
    mov [cs:hook_ax], ax
    mov [cs:hook_flags], bx
    push cs
    pop ds

    mov word [cs:redirect_calls], 0
    mov word [cs:last_redirect_ax], 0
    mov word [cs:last_redirect_stack], 0
    push cs
    pop es
    mov si, local_name_buffer
    mov di, remote_name_buffer
    xor bx, bx
    mov ax, 0x5f02
    int 0x21
    pushf
    pop bx
    mov [cs:result_5f02_ax], ax
    mov [cs:result_5f02_flags], bx
    mov ax, [cs:redirect_calls]
    mov [cs:result_5f02_calls], ax
    mov ax, [cs:last_redirect_ax]
    mov [cs:result_5f02_last], ax
    mov ax, [cs:last_redirect_stack]
    mov [cs:result_5f02_stack], ax
    push cs
    pop ds

    mov word [cs:redirect_calls], 0
    mov word [cs:last_redirect_ax], 0
    mov word [cs:last_redirect_stack], 0
    push cs
    pop ds
    push cs
    pop es
    mov si, source_drive
    mov di, destination_name
    xor cx, cx
    mov bx, 4
    mov ax, 0x5f03
    int 0x21
    pushf
    pop bx
    mov [cs:result_5f03_ax], ax
    mov [cs:result_5f03_flags], bx
    mov ax, [cs:redirect_calls]
    mov [cs:result_5f03_calls], ax
    mov ax, [cs:last_redirect_ax]
    mov [cs:result_5f03_last], ax
    mov ax, [cs:last_redirect_stack]
    mov [cs:result_5f03_stack], ax
    push cs
    pop ds

    push ds
    mov dx, [cs:old_int2f]
    mov ax, [cs:old_int2f + 2]
    mov ds, ax
    mov ax, 0x252f
    int 0x21
    pop ds
    push cs
    pop ds

    call print_results

    cmp word [hook_ax], 0x11ff
    jne hook_failed
    test word [hook_flags], 1
    jnz hook_failed
    mov ax, [result_5f02_calls]
    or ax, [result_5f03_calls]
    jz no_bridge
    cmp word [result_5f02_last], 0x111e
    je bridge_present
    cmp word [result_5f03_last], 0x111e
    je bridge_present

no_bridge:
    mov dx, message_no_bridge
    call print_string
    mov ax, 0x4c01
    int 0x21

bridge_present:
    mov dx, message_bridge
    call print_string
    mov ax, 0x4c00
    int 0x21

hook_failed:
    mov dx, message_hook_failed
    call print_string
    mov ax, 0x4c02
    int 0x21

int2f_handler:
    cmp ax, 0x1100
    je int2f_installation_check
    cmp ah, 0x11
    jne int2f_chain

    push bp
    mov bp, sp
    inc word [cs:redirect_calls]
    mov [cs:last_redirect_ax], ax
    mov ax, [ss:bp + 8]
    mov [cs:last_redirect_stack], ax
    mov ax, 1
    or word [ss:bp + 6], 1
    pop bp
    iret

int2f_installation_check:
    push bp
    mov bp, sp
    mov al, 0xff
    and word [ss:bp + 6], 0xfffe
    pop bp
    iret

int2f_chain:
    cmp word [cs:old_int2f + 2], 0
    jne .chain
    cmp word [cs:old_int2f], 0
    jne .chain
    iret
.chain:
    jmp far [cs:old_int2f]

print_results:
    mov dx, label_dos
    call print_string
    xor ax, ax
    mov al, [dos_major]
    call print_hex_byte
    mov dl, '.'
    call print_character
    mov al, [dos_minor]
    call print_hex_byte
    call print_newline

    mov dx, label_before
    mov ax, [before_ax]
    mov bx, [before_flags]
    call print_ax_cf
    call print_newline
    mov dx, label_hook
    mov ax, [hook_ax]
    mov bx, [hook_flags]
    call print_ax_cf
    call print_newline

    mov dx, label_sda
    mov ax, [sda_ax]
    mov bx, [sda_flags]
    call print_ax_cf
    mov dx, label_pointer
    call print_string
    mov ax, [sda_segment]
    call print_hex_word
    mov dl, ':'
    call print_character
    mov ax, [sda_offset]
    call print_hex_word
    call print_newline

    mov dx, label_5f02
    mov ax, [result_5f02_ax]
    mov bx, [result_5f02_flags]
    call print_ax_cf
    mov dx, label_calls
    call print_string
    mov ax, [result_5f02_calls]
    call print_hex_word
    mov dx, label_last
    call print_string
    mov ax, [result_5f02_last]
    call print_hex_word
    mov dx, label_stack
    call print_string
    mov ax, [result_5f02_stack]
    call print_hex_word
    call print_newline

    mov dx, label_5f03
    mov ax, [result_5f03_ax]
    mov bx, [result_5f03_flags]
    call print_ax_cf
    mov dx, label_calls
    call print_string
    mov ax, [result_5f03_calls]
    call print_hex_word
    mov dx, label_last
    call print_string
    mov ax, [result_5f03_last]
    call print_hex_word
    mov dx, label_stack
    call print_string
    mov ax, [result_5f03_stack]
    call print_hex_word
    call print_newline
    ret

print_ax_cf:
    push ax
    push bx
    call print_string
    pop bx
    pop ax
    call print_hex_word
    mov dx, label_cf
    call print_string
    mov ax, bx
    and ax, 1
    mov dl, '0'
    add dl, al
    call print_character
    ret

print_hex_word:
    push ax
    push bx
    push cx
    push dx
    mov bx, ax
    mov cx, 4
.digit:
    rol bx, 4
    mov dl, bl
    and dl, 0x0f
    cmp dl, 9
    jbe .decimal
    add dl, 'A' - 10
    jmp .emit
.decimal:
    add dl, '0'
.emit:
    call print_character
    loop .digit
    pop dx
    pop cx
    pop bx
    pop ax
    ret

print_hex_byte:
    push ax
    push bx
    push cx
    push dx
    mov bl, al
    mov cx, 2
.digit:
    rol bl, 4
    mov dl, bl
    and dl, 0x0f
    cmp dl, 9
    jbe .decimal
    add dl, 'A' - 10
    jmp .emit
.decimal:
    add dl, '0'
.emit:
    call print_character
    loop .digit
    pop dx
    pop cx
    pop bx
    pop ax
    ret

print_character:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    push ds
    push es
    mov [cs:character_buffer], dl
    push cs
    pop ds
    mov si, character_buffer
    mov ah, 0x02
    mov dx, 0x8000
    int 0x83
    pop es
    pop ds
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

print_string:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    push ds
    push es
    mov si, dx
    mov ah, 0x02
    mov dx, 0x8000
    int 0x83
    pop es
    pop ds
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

print_newline:
    push dx
    mov dx, newline
    call print_string
    pop dx
    ret

message_title db 'R2FPROBE 1 - PC-Engine DOS redirector bridge probe', 13, 10, 0
label_dos db 'DOS=', 0
label_before db 'INT2F/1100 before hook AX=', 0
label_hook db 'INT2F/1100 after hook AX=', 0
label_sda db 'INT21/5D06 AX=', 0
label_pointer db ' PTR=', 0
label_5f02 db 'INT21/5F02 AX=', 0
label_5f03 db 'INT21/5F03 AX=', 0
label_cf db ' CF=', 0
label_calls db ' CALLS=', 0
label_last db ' LAST=', 0
label_stack db ' STACK=', 0
newline db 13, 10, 0
message_no_bridge db 'RESULT=NO_DOS_REDIRECTOR_BRIDGE', 13, 10, 0
message_bridge db 'RESULT=DOS_REDIRECTOR_BRIDGE_PRESENT', 13, 10, 0
message_hook_failed db 'RESULT=PROBE_HOOK_FAILED', 13, 10, 0
character_buffer db 0, 0

source_drive db 'F:', 0
destination_name db '\\HOSTFS\ROOT', 0, 0
local_name_buffer times 16 db 0
remote_name_buffer times 128 db 0

old_int2f dd 0
dos_major db 0
dos_minor db 0
before_ax dw 0
before_flags dw 0
hook_ax dw 0
hook_flags dw 0
sda_ax dw 0
sda_flags dw 0
sda_segment dw 0
sda_offset dw 0
redirect_calls dw 0
last_redirect_ax dw 0
last_redirect_stack dw 0
result_5f02_ax dw 0
result_5f02_flags dw 0
result_5f02_calls dw 0
result_5f02_last dw 0
result_5f02_stack dw 0
result_5f03_ax dw 0
result_5f03_flags dw 0
result_5f03_calls dw 0
result_5f03_last dw 0
result_5f03_stack dw 0
