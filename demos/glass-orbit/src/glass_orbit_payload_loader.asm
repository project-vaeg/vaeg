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
; BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
; USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
; THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
; THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

; Local PC-Engine loader for the final SGP verification payload.  It copies the
; source-built image to the same private main-RAM segment used by the corrected
; P4-1 loader and records a Keyboard-BIOS return continuation there.

cpu 286
bits 16
org 0x100

%ifndef GLASS_PAYLOAD_FILE
%error "GLASS_PAYLOAD_FILE must name the raw SGP payload"
%endif

%define LOADER_RETURN_SS        0xe000
%define LOADER_RETURN_SP        0xe002
%define LOADER_RETURN_FLAGS     0xe004
%define LOADER_RETURN_MAGIC     0xe006
%define LOADER_RETURN_SIGNATURE 0x5034
%define PAYLOAD_SEGMENT          0x3000

start:
        pushf
        pop     bx
        cli
        push    cs
        pop     ds
        mov     ax, PAYLOAD_SEGMENT
        mov     es, ax
        mov     si, payload
        xor     di, di
        mov     cx, (payload_end - payload + 1) / 2
        cld
        rep     movsw
        push    cs
        push    word loader_return
        mov     ax, ss
        mov     [es:LOADER_RETURN_SS], ax
        mov     dx, sp
        mov     [es:LOADER_RETURN_SP], dx
        mov     [es:LOADER_RETURN_FLAGS], bx
        mov     word [es:LOADER_RETURN_MAGIC], LOADER_RETURN_SIGNATURE
        push    word PAYLOAD_SEGMENT
        push    word 0x0000
        retf

loader_return:
        ret

payload:
        incbin GLASS_PAYLOAD_FILE
payload_end:

%if ($ - $$) > 0xff00
%error "GLASS ORBIT SGP loader exceeds the DOS COM size limit"
%endif
