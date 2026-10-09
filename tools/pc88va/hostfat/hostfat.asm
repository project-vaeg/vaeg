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
; EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
; EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
; OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
; INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
; CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
; ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
; POSSIBILITY OF SUCH DAMAGE.

bits 16
cpu 8086
org 0

port_value      equ 0x07ed
port_command    equ 0x07ef

status_done     equ 0x0100
status_removable equ 0x0200
status_protected equ 0x8100
status_bad_command equ 0x8103
status_read_error equ 0x8108

packet_command  equ 0x02
packet_status   equ 0x03
packet_media    equ 0x0d
packet_transfer equ 0x0e
packet_count    equ 0x12
packet_sector   equ 0x14

device_header:
    dd 0xffffffff
    dw 0x2000
    dw strategy_entry
    dw interrupt_entry
    db 1
    dw device_name
    dw 0

device_name:
    db 'HOSTFAT', 0

strategy_entry:
    push ax
    push ds
    mov ax, cs
    mov ds, ax
    mov [active_packet], bx
    mov [active_packet + 2], es
    pop ds
    pop ax
    retf

interrupt_entry:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    push ds
    push es

    push cs
    pop ds
    les bx, [active_packet]
    mov al, [es:bx + packet_command]

    cmp al, 0x00
    jne dispatch_media_check
    jmp command_initialize
dispatch_media_check:
    cmp al, 0x01
    jne dispatch_build_bpb
    jmp command_media_check
dispatch_build_bpb:
    cmp al, 0x02
    jne dispatch_read
    jmp command_build_bpb
dispatch_read:
    cmp al, 0x04
    jne dispatch_write
    jmp command_read
dispatch_write:
    cmp al, 0x08
    jne dispatch_write_verify
    jmp command_write
dispatch_write_verify:
    cmp al, 0x09
    jne dispatch_open
    jmp command_write
dispatch_open:
    cmp al, 0x0d
    jne dispatch_close
    jmp command_noop
dispatch_close:
    cmp al, 0x0e
    jne dispatch_removable
    jmp command_noop
dispatch_removable:
    cmp al, 0x0f
    jne command_unknown
    jmp command_removable
command_unknown:
    mov ax, status_bad_command
    jmp finish_request

command_initialize:
    call return_resident_end
    call service_available
    test al, al
    jz initialize_unavailable

    mov byte [es:bx + packet_media], 1
    mov word [es:bx + packet_count], bpb_offset_list
    mov ax, cs
    mov word [es:bx + packet_sector], ax
    mov si, ready_message
    call display_message
    mov ax, status_done
    jmp finish_request

initialize_unavailable:
    mov byte [es:bx + packet_media], 0
    mov word [es:bx + packet_count], 0
    mov word [es:bx + packet_sector], 0
    mov si, unavailable_message
    call display_message
    mov ax, status_done
    jmp finish_request

command_media_check:
    mov byte [es:bx + packet_transfer], 1
    mov ax, status_done
    jmp finish_request

command_build_bpb:
    mov byte [es:bx + packet_media], 0xf0
    mov word [es:bx + packet_count], bios_parameter_block
    mov ax, cs
    mov word [es:bx + packet_sector], ax
    mov ax, status_done
    jmp finish_request

command_read:
    call perform_host_read
    test al, al
    jnz read_failed
    mov ax, status_done
    jmp finish_request

read_failed:
    mov word [es:bx + packet_count], 0
    mov ax, status_read_error
    jmp finish_request

command_write:
    mov ax, status_protected
    jmp finish_request

command_noop:
    mov ax, status_done
    jmp finish_request

command_removable:
    mov ax, status_removable

finish_request:
    mov [es:bx + packet_status], ax

    pop es
    pop ds
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    retf

return_resident_end:
    mov word [es:bx + packet_transfer], 0
    mov ax, cs
    add ax, resident_paragraphs
    mov word [es:bx + packet_transfer + 2], ax
    ret

service_available:
    mov dx, port_command
    mov si, probe_text
    mov cx, probe_text_length
    call send_bytes
    in al, dx
    mov ah, al
    in al, dx
    cmp ah, 'H'
    jne service_not_found
    cmp al, '1'
    jne service_not_found
    mov al, 1
    ret

service_not_found:
    xor al, al
    ret

perform_host_read:
    mov dx, port_value
    mov si, active_packet
    mov cx, 4
    call send_bytes
    mov dx, port_command
    mov si, read_text
    mov cx, read_text_length
    call send_bytes
    mov dx, port_value
    in al, dx
    ret

send_bytes:
    mov al, [si]
    out dx, al
    inc si
    loop send_bytes
    ret

display_message:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    push ds
    push es
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

active_packet:
    dw 0, 0

probe_text:
    db 'check_hostfat'
probe_text_length equ $ - probe_text

read_text:
    db 'read_hostfat1'
read_text_length equ $ - read_text

ready_message:
    db 0x0d, 0x0a, 'HOSTFAT read-only drive ready', 0x0d, 0x0a, 0

unavailable_message:
    db 0x0d, 0x0a
    db 'HOSTFAT unavailable (start vaeg with --hostfat-dir)'
    db 0x0d, 0x0a, 0

bpb_offset_list:
    dw bios_parameter_block

bios_parameter_block:
    dw 1024
    db 16
    dw 0
    db 2
    dw 128
    dw 65362
    db 0xf0
    dw 7
    db 0x90

align 16, db 0
resident_image_end:

resident_paragraphs equ (resident_image_end - $$ + 15) / 16
