; GLASS ORBIT original work:
; Developed by ChatGPT Plus
; Supervised by SimK, Neko Project 21/W Developer
; Ported By Maho Nakata, 2026.
;
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
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
; IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
; OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
; IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
; INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
; BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
; ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
; THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

; PC-88VA YM2608/OPNA glue for GLASS ORBIT P6.
;
; The VA Sound BIOS documentation defines the OPNA register windows as
; 44h/45h (low bank) and 46h/47h (high bank).  The emulator's VA Sound Board
; II binds the same four ports.  Detection therefore requires both status
; windows to respond; a VA machine with only the three-channel YM2203 path is
; left in the graphics-only mode.  No alternate sound backend is probed.

%define GLASS_OPNA_PORT_LOW_ADDR       0x0044
%define GLASS_OPNA_PORT_LOW_DATA       0x0045
%define GLASS_OPNA_PORT_HIGH_ADDR      0x0046
%define GLASS_OPNA_PORT_HIGH_DATA      0x0047
%define GLASS_OPNA_STATUS_BUSY         0x0080
%define GLASS_OPNA_WAIT_LIMIT           0x4000
%define GLASS_OPNA_MUSIC_STEPS          0x0100
%define GLASS_OPNA_MUSIC_MASK           0x00ff
%define GLASS_OPNA_MUSIC_DIVIDER        6
%define GLASS_OPNA_HOLD                 0xfe
%define GLASS_OPNA_REST                 0xff

glass_opna_init:
        mov     byte [glass_opna_present], 0
        call    glass_opna_detect
        jc      .disabled
        mov     word [glass_opna_music_index], 0
        mov     byte [glass_opna_music_divider], GLASS_OPNA_MUSIC_DIVIDER
        call    glass_opna_mute_ssg
        xor     bx, bx
.setup:
        call    glass_opna_setup_channel
        inc     bl
        cmp     bl, 3
        jb      .setup
        mov     byte [glass_opna_present], 1
        call    glass_opna_music_step
        clc
        ret
.disabled:
        clc
        ret

; A missing or unmapped high-bank status port returns FFh on the VA I/O bus.
; This is a bounded, read-only probe and does not alter a chip register.
glass_opna_detect:
        push    ax
        push    dx
        mov     dx, GLASS_OPNA_PORT_LOW_ADDR
        in      al, dx
        cmp     al, 0xff
        je      .absent
        mov     dx, GLASS_OPNA_PORT_HIGH_ADDR
        in      al, dx
        cmp     al, 0xff
        je      .absent
        clc
        jmp     .done
.absent:
        stc
.done:
        pop     dx
        pop     ax
        ret

; Wait for the selected YM2608 bank to accept a write.  The timeout keeps a
; disabled or faulty audio board from hanging the graphics demo.
glass_opna_wait_low:
        push    ax
        push    cx
        push    dx
        mov     dx, GLASS_OPNA_PORT_LOW_ADDR
        mov     cx, GLASS_OPNA_WAIT_LIMIT
.poll:
        in      al, dx
        test    al, GLASS_OPNA_STATUS_BUSY
        jz      .ready
        loop    .poll
        stc
        jmp     .done
.ready:
        clc
.done:
        pop     dx
        pop     cx
        pop     ax
        ret

glass_opna_wait_high:
        push    ax
        push    cx
        push    dx
        mov     dx, GLASS_OPNA_PORT_HIGH_ADDR
        mov     cx, GLASS_OPNA_WAIT_LIMIT
.poll:
        in      al, dx
        test    al, GLASS_OPNA_STATUS_BUSY
        jz      .ready
        loop    .poll
        stc
        jmp     .done
.ready:
        clc
.done:
        pop     dx
        pop     cx
        pop     ax
        ret

; AL=register, AH=value.
glass_opna_write_low:
        push    ax
        push    bx
        push    dx
        call    glass_opna_wait_low
        jc      .done
        pop     dx
        pop     bx
        pop     ax
        push    ax
        mov     dx, GLASS_OPNA_PORT_LOW_ADDR
        out     dx, al
        inc     dx
        mov     al, ah
        out     dx, al
        pop     ax
        clc
        ret
.done:
        pop     dx
        pop     bx
        pop     ax
        ret

; AL=register, AH=value.
glass_opna_write_high:
        push    ax
        push    bx
        push    dx
        call    glass_opna_wait_high
        jc      .done
        pop     dx
        pop     bx
        pop     ax
        push    ax
        mov     dx, GLASS_OPNA_PORT_HIGH_ADDR
        out     dx, al
        inc     dx
        mov     al, ah
        out     dx, al
        pop     ax
        clc
        ret
.done:
        pop     dx
        pop     bx
        pop     ax
        ret

glass_opna_mute_ssg:
        mov     al, 07h
        mov     ah, 3fh
        call    glass_opna_write_low
        xor     ah, ah
        mov     al, 08h
        call    glass_opna_write_low
        mov     al, 09h
        call    glass_opna_write_low
        mov     al, 0ah
        jmp     glass_opna_write_low

; AL=NOTE(block,note), BL=SSG channel 0..2.  The GLASS score keeps these
; channels silent, but this complete OPNA primitive is retained for the
; standalone FM/SSG hardware check and future score extensions.
glass_opna_ssg_note_on:
        push    ax
        push    bx
        push    cx
        push    dx
        mov     [glass_opna_tmp_note], al
        mov     [glass_opna_tmp_channel], bl
        mov     dl, al
        and     dl, 0fh
        cmp     dl, 12
        jae     .done
        xor     bh, bh
        mov     bl, dl
        shl     bx, 1
        mov     ax, [glass_opna_ssg_period_c5+bx]
        mov     dl, [glass_opna_tmp_note]
        mov     dh, dl
        shr     dh, 4
        and     dh, 7
        cmp     dh, 4
        je      .period_ready
        ja      .higher
        mov     cl, 4
        sub     cl, dh
        shl     ax, cl
        cmp     ax, 0fffh
        jbe     .period_ready
        mov     ax, 0fffh
        jmp     .period_ready
.higher:
        mov     cl, dh
        sub     cl, 4
        shr     ax, cl
        or      ax, ax
        jnz     .period_ready
        inc     ax
.period_ready:
        mov     [glass_opna_tmp_period], ax
        xor     bx, bx
        mov     bl, [glass_opna_tmp_channel]
        shl     bx, 1
        mov     al, bl
        mov     ah, [glass_opna_tmp_period]
        call    glass_opna_write_low
        inc     al
        mov     ah, [glass_opna_tmp_period+1]
        and     ah, 0fh
        call    glass_opna_write_low
        shr     bx, 1
        mov     al, 07h
        mov     ah, [glass_opna_ssg_mixer_enable+bx]
        call    glass_opna_write_low
        mov     al, 08h
        add     al, [glass_opna_tmp_channel]
        mov     ah, 0ch
        call    glass_opna_write_low
.done:
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

glass_opna_ssg_key_off:
        push    ax
        mov     al, 08h
        add     al, bl
        xor     ah, ah
        call    glass_opna_write_low
        pop     ax
        ret

; Configure one of the three score FM channels from the original GLASS patch.
; YM2608 operator order is OP1, OP3, OP2, OP4, hence the offset table.
glass_opna_setup_channel:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        mov     [glass_opna_tmp_channel], bl
        xor     bh, bh
        mov     ax, bx
        shl     ax, 2
        mov     bx, ax
        xor     di, di
        mov     si, glass_opna_operator_offsets
.operator:
        mov     dl, [si]
        mov     al, 30h
        add     al, dl
        add     al, [glass_opna_tmp_channel]
        mov     ah, [glass_opna_mul+bx+di]
        call    glass_opna_write_low
        mov     al, 40h
        add     al, dl
        add     al, [glass_opna_tmp_channel]
        mov     ah, [glass_opna_tl+bx+di]
        call    glass_opna_write_low
        mov     al, 50h
        add     al, dl
        add     al, [glass_opna_tmp_channel]
        mov     ah, [glass_opna_ar+bx+di]
        call    glass_opna_write_low
        mov     al, 60h
        add     al, dl
        add     al, [glass_opna_tmp_channel]
        mov     ah, [glass_opna_dr+bx+di]
        call    glass_opna_write_low
        mov     al, 70h
        add     al, dl
        add     al, [glass_opna_tmp_channel]
        mov     ah, [glass_opna_sr+bx+di]
        call    glass_opna_write_low
        mov     al, 80h
        add     al, dl
        add     al, [glass_opna_tmp_channel]
        mov     ah, [glass_opna_slrr+bx+di]
        call    glass_opna_write_low
        inc     si
        inc     di
        cmp     di, 4
        jb      .operator
        xor     bh, bh
        mov     bl, [glass_opna_tmp_channel]
        mov     al, 0b0h
        add     al, bl
        mov     ah, [glass_opna_alg+bx]
        call    glass_opna_write_low
        mov     al, 0b4h
        add     al, bl
        mov     ah, [glass_opna_pan+bx]
        call    glass_opna_write_low
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

glass_opna_music_step:
        push    ax
        push    bx
        push    cx
        push    si
        mov     si, [glass_opna_music_index]
        xor     bx, bx
.channel:
        cmp     bl, 0
        je      .ch0
        cmp     bl, 1
        je      .ch1
        mov     al, [glass_opna_music_ch2+si]
        jmp     .apply
.ch0:
        mov     al, [glass_opna_music_ch0+si]
        jmp     .apply
.ch1:
        mov     al, [glass_opna_music_ch1+si]
.apply:
        mov     [glass_opna_tmp_channel], bl
        call    glass_opna_apply_step
        inc     bl
        cmp     bl, 3
        jb      .channel
        pop     si
        pop     cx
        pop     bx
        pop     ax
        ret

; AL=score token, BL=FM channel 0..2.
glass_opna_apply_step:
        cmp     al, GLASS_OPNA_HOLD
        je      .done
        cmp     al, GLASS_OPNA_REST
        jne     .note
        call    glass_opna_key_off
        ret
.note:
        call    glass_opna_key_off
        jmp     glass_opna_note_on
.done:
        ret

glass_opna_key_off:
        push    ax
        mov     al, 28h
        mov     ah, bl
        call    glass_opna_write_low
        pop     ax
        ret

; AL=NOTE(block,note), BL=FM channel.
glass_opna_note_on:
        push    ax
        push    bx
        push    dx
        mov     [glass_opna_tmp_note], al
        mov     dl, al
        and     dl, 0fh
        cmp     dl, 12
        jae     .done
        xor     bh, bh
        mov     bl, dl
        shl     bx, 1
        mov     bx, [glass_opna_fnum_table+bx]
        mov     [glass_opna_tmp_fnum], bx
        mov     dl, [glass_opna_tmp_note]
        mov     dh, dl
        shr     dh, 4
        and     dh, 7
        mov     al, 0a4h
        add     al, [glass_opna_tmp_channel]
        mov     ah, dh
        shl     ah, 3
        mov     dl, bh
        and     dl, 7
        or      ah, dl
        call    glass_opna_write_low
        mov     al, 0a0h
        add     al, [glass_opna_tmp_channel]
        mov     ah, [glass_opna_tmp_fnum]
        call    glass_opna_write_low
        mov     al, 28h
        mov     ah, 0f0h
        or      ah, [glass_opna_tmp_channel]
        call    glass_opna_write_low
.done:
        pop     dx
        pop     bx
        pop     ax
        ret

glass_opna_tick:
        cmp     byte [glass_opna_present], 1
        jne     .done
        dec     byte [glass_opna_music_divider]
        jnz     .done
        mov     byte [glass_opna_music_divider], GLASS_OPNA_MUSIC_DIVIDER
        call    glass_opna_music_step
        inc     word [glass_opna_music_index]
        and     word [glass_opna_music_index], GLASS_OPNA_MUSIC_MASK
.done:
        ret

glass_opna_shutdown:
        cmp     byte [glass_opna_present], 1
        jne     .done
        xor     bx, bx
.off:
        call    glass_opna_key_off
        inc     bl
        cmp     bl, 3
        jb      .off
        call    glass_opna_mute_ssg
        mov     byte [glass_opna_present], 0
.done:
        ret

glass_opna_present            db 0
glass_opna_music_divider       db 0
glass_opna_tmp_channel          db 0
glass_opna_tmp_note             db 0
glass_opna_music_index          dw 0
glass_opna_tmp_fnum             dw 0
glass_opna_tmp_period           dw 0

glass_opna_operator_offsets     db 0,8,4,12
glass_opna_mul                  db 01h,02h,04h,01h, 01h,02h,03h,01h, 01h,02h,03h,01h
glass_opna_tl                   db 28h,30h,38h,0Ah, 30h,2Ch,38h,12h, 38h,34h,3Ch,12h
glass_opna_ar                   db 1Fh,1Fh,1Fh,1Fh, 1Fh,1Fh,1Eh,1Fh, 1Fh,1Eh,1Eh,1Fh
glass_opna_dr                   db 08h,0Ah,0Ch,07h, 07h,09h,0Ah,06h, 09h,0Bh,0Dh,09h
glass_opna_sr                   db 04h,05h,06h,04h, 04h,05h,06h,03h, 05h,06h,07h,04h
glass_opna_slrr                 db 48h,59h,6Ah,48h, 6Ah,7Ah,7Ah,69h, 59h,6Ah,6Ah,58h
glass_opna_alg                  db 0Fh,07h,07h
glass_opna_pan                  db 0C0h,0C0h,0C0h
glass_opna_fnum_table           dw 026Ah,028Fh,02B6h,02DFh,030Bh,0339h
                                dw 036Ah,039Eh,03D5h,0410h,044Eh,048Fh
glass_opna_ssg_period_c5        dw 239,225,212,200,189,178,168,159,150,142,134,126
glass_opna_ssg_mixer_enable     db 3eh,3dh,3bh

%include "glass_opna_data.inc"
