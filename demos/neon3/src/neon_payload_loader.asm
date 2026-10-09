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

; Local validation loader for a raw NEON3 VA payload.  It uses the same
; PC-Engine loader continuation contract as the validated Glass payload, but
; is a local bootable-disk test artifact and is not a distributable disk.

        cpu     286
        bits    16
        org     100h

%ifndef NEON_PAYLOAD_FILE
%error "NEON_PAYLOAD_FILE must name the raw payload"
%endif

%define PAYLOAD_SEGMENT          3000h
%define LOADER_RETURN_SS        0e000h
%define LOADER_RETURN_SP        0e002h
%define LOADER_RETURN_FLAGS     0e004h
%define LOADER_RETURN_MAGIC     0e006h
%define LOADER_RETURN_LOADER_SEG 0e008h
%define LOADER_RETURN_SIGNATURE 5034h
%define BIOS_DESCRIPTOR_OFFSET  1970h

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
        mov     ax, cs
        mov     [es:LOADER_RETURN_LOADER_SEG], ax
        mov     di, BIOS_DESCRIPTOR_OFFSET
        mov     word [ds:di], 4
        mov     word [ds:di+2], 640
        mov     word [ds:di+4], 400
        mov     word [ds:di+6], 0
        mov     word [ds:di+8], 0
        mov     word [ds:di+10], 200
        mov     word [ds:di+12], 0
        mov     word [ds:di+14], 0
        mov     word [ds:di+16], 0
        mov     word [ds:di+18], 0
        push    word PAYLOAD_SEGMENT
        push    word 0
        retf

loader_return:
        ret

payload:
        incbin NEON_PAYLOAD_FILE
payload_end:

%if ($ - $$) > 0ff00h
%error "NEON3 validation loader exceeds the DOS COM size limit"
%endif
