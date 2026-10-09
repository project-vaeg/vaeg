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

%ifndef CASE_ID
%error "CASE_ID=1..8 is required"
%endif

%if CASE_ID = 1
%define CASE_NAME "r001"
%define CASE_AX 0x0011
%macro EMIT_CASE 0
    db 0x0f, 0x10, 0xc0
%endmacro
%elif CASE_ID = 2
%define CASE_NAME "r002"
%define CASE_AX 0x0011
%macro EMIT_CASE 0
    db 0xf2, 0x0f, 0x10, 0xc0
%endmacro
%elif CASE_ID = 3
%define CASE_NAME "r003"
%define CASE_AX 0x0011
%macro EMIT_CASE 0
    db 0xf3, 0x0f, 0x10, 0xc0
%endmacro
%elif CASE_ID = 4
%define CASE_NAME "r004"
%define CASE_AX 0x0000
%macro EMIT_CASE 0
    db 0x0f, 0x01, 0xf0
%endmacro
%elif CASE_ID = 5
%define CASE_NAME "r005"
%define CASE_AX 0x0000
%macro EMIT_CASE 0
    db 0xf2, 0x0f, 0x01, 0xf0
%endmacro
%elif CASE_ID = 6
%define CASE_NAME "r006"
%define CASE_AX 0x0000
%macro EMIT_CASE 0
    db 0xf3, 0x0f, 0x01, 0xf0
%endmacro
%elif CASE_ID = 7
%define CASE_NAME "r007"
%define CASE_AX 0x0000
%macro EMIT_CASE 0
    db 0xf2, 0x0f, 0x00, 0xc0
%endmacro
%elif CASE_ID = 8
%define CASE_NAME "r008"
%define CASE_AX 0x0000
%macro EMIT_CASE 0
    db 0xf2, 0x0f, 0x01, 0x06
    dw guard
%endmacro
%else
%error "CASE_ID must be in the range 1..8"
%endif

    jmp start

case_name db CASE_NAME, 0
before_head db "VAEG_REP0F,v=1,event=before,case=", 0
bytes_key db ",bytes=", 0
before_ax_key db ",ax=", 0
before_mid db ",bx=2233,cx=0001,dx=4455,si=6677,di=8899,bp=aabb"
           db ",sp=", 0
before_ip_key db ",ip=", 0
before_flags db ",flags=0302,guard=", 0
after_head db "VAEG_REP0F,v=1,event=after,case=", 0
trap_key db ",completion=1,trap=", 0
ip_key db ",ip=", 0
cs_key db ",cs=", 0
flags_key db ",flags=", 0
ax_key db ",ax=", 0
bx_key db ",bx=", 0
cx_key db ",cx=", 0
dx_key db ",dx=", 0
si_key db ",si=", 0
di_key db ",di=", 0
bp_key db ",bp=", 0
sp_key db ",sp=", 0
ds_key db ",ds=", 0
es_key db ",es=", 0
ss_key db ",ss=", 0
guard_key db ",guard=", 0
newline db 13, 10, 0
hex_digits db "0123456789abcdef"

old_int1_off dw 0
old_int1_seg dw 0
old_int6_off dw 0
old_int6_seg dw 0
result_trap db 0
result_ax dw 0
result_bx dw 0
result_cx dw 0
result_dx dw 0
result_si dw 0
result_di dw 0
result_bp dw 0
result_sp dw 0
result_ds dw 0
result_es dw 0
result_ss dw 0
result_ip dw 0
result_cs dw 0
result_flags dw 0
guard times 12 db 0xa5

print_z:
    lodsb
    test al, al
    jz .done
    mov dl, al
    mov ah, 0x02
    int 0x21
    jmp print_z
.done:
    ret

print_nibble:
    and al, 0x0f
    push bx
    mov bx, hex_digits
    xlat
    pop bx
    mov dl, al
    mov ah, 0x02
    int 0x21
    ret

print_byte:
    push cx
    push ax
    mov cl, 4
    shr al, cl
    call print_nibble
    pop ax
    call print_nibble
    pop cx
    ret

print_word:
    push ax
    mov al, ah
    call print_byte
    pop ax
    call print_byte
    ret

print_guard:
    push si
    push cx
    mov si, guard
    mov cx, 12
.next:
    lodsb
    call print_byte
    loop .next
    pop cx
    pop si
    ret

print_before:
    mov si, before_head
    call print_z
    mov si, case_name
    call print_z
    mov si, bytes_key
    call print_z
    mov si, case_instruction
    mov cx, case_end - case_instruction
.byte:
    lodsb
    call print_byte
    loop .byte
    mov si, before_ax_key
    call print_z
    mov ax, CASE_AX
    call print_word
    mov si, before_mid
    call print_z
    mov ax, stack_top
    call print_word
    mov si, before_ip_key
    call print_z
    mov ax, case_instruction
    call print_word
    mov si, before_flags
    call print_z
    call print_guard
    mov si, newline
    call print_z
    ret

print_field_word:
    push ax
    call print_z
    pop ax
    call print_word
    ret

print_after:
    mov si, after_head
    call print_z
    mov si, case_name
    call print_z
    mov si, trap_key
    call print_z
    mov al, [result_trap]
    call print_byte
    mov si, ip_key
    mov ax, [result_ip]
    call print_field_word
    mov si, cs_key
    mov ax, [result_cs]
    call print_field_word
    mov si, flags_key
    mov ax, [result_flags]
    call print_field_word
    mov si, ax_key
    mov ax, [result_ax]
    call print_field_word
    mov si, bx_key
    mov ax, [result_bx]
    call print_field_word
    mov si, cx_key
    mov ax, [result_cx]
    call print_field_word
    mov si, dx_key
    mov ax, [result_dx]
    call print_field_word
    mov si, si_key
    mov ax, [result_si]
    call print_field_word
    mov si, di_key
    mov ax, [result_di]
    call print_field_word
    mov si, bp_key
    mov ax, [result_bp]
    call print_field_word
    mov si, sp_key
    mov ax, [result_sp]
    call print_field_word
    mov si, ds_key
    mov ax, [result_ds]
    call print_field_word
    mov si, es_key
    mov ax, [result_es]
    call print_field_word
    mov si, ss_key
    mov ax, [result_ss]
    call print_field_word
    mov si, guard_key
    call print_z
    call print_guard
    mov si, newline
    call print_z
    ret

int1_handler:
    mov byte [cs:result_trap], 1
    jmp short trap_common

int6_handler:
    mov byte [cs:result_trap], 6

trap_common:
    mov [cs:result_ax], ax
    mov [cs:result_bx], bx
    mov [cs:result_cx], cx
    mov [cs:result_dx], dx
    mov [cs:result_si], si
    mov [cs:result_di], di
    mov [cs:result_bp], bp
    mov ax, sp
    add ax, 6
    mov [cs:result_sp], ax
    mov ax, ds
    mov [cs:result_ds], ax
    mov ax, es
    mov [cs:result_es], ax
    mov ax, ss
    mov [cs:result_ss], ax
    mov bp, sp
    mov ax, [ss:bp]
    mov [cs:result_ip], ax
    mov ax, [ss:bp + 2]
    mov [cs:result_cs], ax
    mov ax, [ss:bp + 4]
    mov [cs:result_flags], ax
    mov word [ss:bp], after_case
    push cs
    pop ax
    mov [ss:bp + 2], ax
    and word [ss:bp + 4], 0xfeff
    iret

restore_vectors:
    mov dx, [old_int1_off]
    mov ax, [old_int1_seg]
    mov ds, ax
    mov ax, 0x2501
    int 0x21
    push cs
    pop ds
    mov dx, [old_int6_off]
    mov ax, [old_int6_seg]
    mov ds, ax
    mov ax, 0x2506
    int 0x21
    push cs
    pop ds
    ret

start:
    push cs
    pop ax
    cli
    mov ss, ax
    mov sp, stack_top
    sti
    push cs
    pop ds
    push cs
    pop es

    call print_before

    mov ax, 0x3501
    int 0x21
    mov [old_int1_off], bx
    mov [old_int1_seg], es
    mov ax, 0x3506
    int 0x21
    mov [old_int6_off], bx
    mov [old_int6_seg], es
    mov dx, int1_handler
    mov ax, 0x2501
    int 0x21
    mov dx, int6_handler
    mov ax, 0x2506
    int 0x21
    push cs
    pop es

    mov ax, 0x0302
    push ax
    mov ax, CASE_AX
    mov bx, 0x2233
    mov cx, 0x0001
    mov dx, 0x4455
    mov si, 0x6677
    mov di, 0x8899
    mov bp, 0xaabb
    popf

case_instruction:
    EMIT_CASE
case_end:

    ; TF or INT 6 must redirect execution here. Falling through is deliberately
    ; treated the same way so the result remains recoverable on unusual cores.
after_case:
    push cs
    pop ds
    push cs
    pop es
    call restore_vectors
    call print_after
    mov ax, 0x4c00
    int 0x21

align 2
stack_space times 1024 db 0
stack_top:
