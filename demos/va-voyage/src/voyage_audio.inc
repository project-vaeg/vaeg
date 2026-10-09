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

%ifndef VOYAGE_AUDIO
%define VOYAGE_AUDIO 1
%endif
%if VOYAGE_AUDIO != 0 && VOYAGE_AUDIO != 1
%error "VOYAGE_AUDIO must be 0 or 1"
%endif

; Exclusive low-bank chip ownership. Three algorithm-4 FM voices, plus SSG
; noise percussion. No OPNA upper bank/ADPCM, IRQ hook or timer-driven music.
; Normal BIOS FM prescaling is assumed; no prescaler writes are performed.
audio_init:
%if VOYAGE_AUDIO == 0
    ret
%else
    pusha
    mov byte [audio_present], 1
    call opn_ready
    jc .done
    ; Read SSG mixer before replacing it: retain the I/O direction bits.
    mov dx, 44h
    mov al, 7
    out dx, al
    call opn_ready
    jc .done
    mov dx, 45h
    in al, dx
    and al, 0c0h
    mov [mixer_directions], al
    mov ax, 3027h            ; stop/acknowledge both timers; no timer IRQs.
    call opn_write
    xor bx, bx
.channel:
    mov al, 0b0h
    add al, bl
    mov ah, 14h             ; feedback 2, algorithm 4: two FM pairs.
    call opn_write
    mov si, fm_parameters
    mov cx, (fm_parameters_end-fm_parameters)/2
.operator:
    lodsw
    cmp bl, 2
    jne .write
    cmp al, 40h
    jb .write
    cmp al, 50h
    jae .attack
    add ah, 18             ; softer, less modulated pad.
    jmp .write
.attack:
    cmp al, 60h
    jae .write
    mov ah, 12h            ; slower pad attack.
.write:
    add al, bl
    call opn_write
    loop .operator
    mov al, 28h
    mov ah, bl
    call opn_write
    inc bx
    cmp bx, 3
    jb .channel
    mov al, 7
    mov ah, [mixer_directions]
    or ah, 1fh             ; noise C only; preserve I/O directions.
    call opn_write
    mov ax, 0008h
    call opn_write
    mov ax, 0009h
    call opn_write
    mov ax, 000ah
    call opn_write
.done:
    popa
    ret
%endif

; Shared calibrated 900-tick, eight-bar / 128-step prototype timeline.
; This never calls itself: opn_ready only samples clock edges.
music_service:
    pushf
    pusha
    cmp byte [running], 0
    je .done
    mov ax, [tick]
    cmp ax, [music_tick]
    je .done
    mov dx, ax
    sub dx, [music_tick]
    cmp word [music_tick], 0ffffh
    je .first
    test dx, dx
    jns .delta
    add dx, PERIOD
.delta:
    ; Visual pulse exists even in silent mode. Account for elapsed ticks,
    ; not the number of service calls made by a slow or fast renderer.
    xor bx, bx
    mov bl, [pulse]
    sub bx, dx
    jns .pulse_ok
    xor bx, bx
.pulse_ok:
    mov [pulse], bl
    xor bx, bx
    mov bl, [noise_volume]
    shl dx, 1
    sub bx, dx
    jns .volume_ok
    xor bx, bx
.volume_ok:
    mov [noise_volume], bl
    mov ah, bl
    mov al, 0ah
    call opn_write
.first:
    mov ax, [tick]
    mov [music_tick], ax
    mov bx, 128
    mul bx
    mov bx, PERIOD
    div bx
    cmp ax, [music_step]
    je .done
    mov [music_step], ax
    mov bp, ax
    mov si, ax
    shl si, 1
    mov ax, [bass_score+si]
    cmp ax, 0ffffh
    je .lead
    xor bx, bx
    call play_note
.lead:
    mov ax, [lead_score+si]
    mov bx, 1
    call play_note
    test bp, 15
    jnz .percussion
    mov ax, [pad_score+si]
    mov bx, 2
    call play_note
.percussion:
    test bp, 3
    jnz .hat
    mov byte [pulse], 20
    mov byte [noise_volume], 15
    mov ax, 1806h           ; low noise thump.
    call opn_write
    jmp .volume
.hat:
    test bp, 1
    jz .done
    mov byte [noise_volume], 12
    mov ax, 0306h           ; short bright noise hat.
    call opn_write
.volume:
    mov al, 0ah
    mov ah, [noise_volume]
    call opn_write
.done:
    popa
    popf
    ret

; BX = low-bank voice 0..2, AX = packed block/F-number.
play_note:
    push ax
    push cx
    mov cx, ax
    mov al, 28h
    mov ah, bl
    call opn_write
    mov ah, ch
    mov al, 0a4h
    add al, bl
    call opn_write
    mov ah, cl
    mov al, 0a0h
    add al, bl
    call opn_write
    mov al, 28h
    mov ah, 0f0h
    or ah, bl
    call opn_write
    pop cx
    pop ax
    ret

audio_stop:
    pusha
    xor bx, bx
.channel:
    mov al, 28h
    mov ah, bl
    call opn_write
    inc bx
    cmp bx, 3
    jb .channel
    mov ax, 0008h
    call opn_write
    mov ax, 0009h
    call opn_write
    mov ax, 000ah
    call opn_write
    mov al, 7
    mov ah, [mixer_directions]
    or ah, 3fh
    call opn_write
    mov ax, 3027h
    call opn_write
    popa
    ret

; AL=register, AH=value. Each address AND data write waits for BUSY to
; clear. FFh means absent; a stuck BUSY flag disables further sound writes.
; This is a bounded fail-silent path, not a probe of another chip/backend.
opn_write:
    cmp byte [audio_present], 0
    je .disabled
    push ax
    push cx
    push dx
    call opn_ready
    jc .done
    mov dx, 44h
    out dx, al
    call opn_ready
    jc .done
    mov dx, 45h
    mov al, ah
    out dx, al
.done:
    pop dx
    pop cx
    pop ax
    ret
.disabled:
    stc
    ret

opn_ready:
    push ax
    push cx
    push dx
    mov cx, 4000h
.poll:
    call clock_poll
    mov dx, 44h
    in al, dx
    cmp al, 0ffh
    je .failed
    test al, 80h
    jz .ready
    loop .poll
.failed:
    mov byte [audio_present], 0
    stc
    jmp .done
.ready:
    clc
.done:
    pop dx
    pop cx
    pop ax
    ret

fm_parameters:
    db 30h,02h,34h,01h,38h,01h,3ch,01h
    db 40h,32h,44h,28h,48h,24h,4ch,1ch
    db 50h,1fh,54h,1fh,58h,1fh,5ch,1fh
    db 60h,08h,64h,08h,68h,08h,6ch,08h
    db 70h,04h,74h,04h,78h,04h,7ch,04h
    db 80h,5ah,84h,5ah,88h,5ah,8ch,5ah
    db 90h,00h,94h,00h,98h,00h,9ch,00h
fm_parameters_end:
audio_present: db 0
mixer_directions: db 0
noise_volume: db 0
pulse: db 0
align 2, db 0
music_tick: dw 0ffffh
music_step: dw 0ffffh
