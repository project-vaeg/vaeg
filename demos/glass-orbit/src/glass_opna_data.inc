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

; The original GLASS score stores a 256-step phrase twice in each 512-step
; channel. Keeping one phrase and masking the index at 256 is byte-for-byte
; equivalent while leaving room for the VA graphics payload.
glass_opna_music_ch0:
        db 57h,0FEh,60h,0FEh,63h,0FEh,67h,0FEh,6Ah,0FEh,67h,0FEh,63h,0FEh,62h,0FFh
        db 58h,0FEh,60h,0FEh,63h,0FEh,68h,0FEh,67h,0FEh,63h,0FEh,60h,0FEh,5Ah,0FFh
        db 57h,0FEh,5Ah,0FEh,63h,0FEh,67h,0FEh,65h,0FEh,63h,0FEh,5Ah,0FEh,57h,0FFh
        db 55h,0FEh,5Ah,0FEh,62h,0FEh,65h,0FEh,68h,0FEh,65h,0FEh,62h,0FEh,60h,0FFh
        db 60h,0FEh,62h,0FEh,63h,0FEh,67h,0FEh,6Ah,0FEh,67h,0FEh,65h,0FEh,63h,0FFh
        db 58h,0FEh,60h,0FEh,65h,0FEh,68h,0FEh,70h,0FEh,68h,0FEh,67h,0FEh,65h,0FFh
        db 63h,0FEh,60h,0FEh,58h,0FEh,60h,0FEh,63h,0FEh,67h,0FEh,65h,0FEh,63h,0FFh
        db 62h,0FEh,67h,0FEh,6Bh,0FEh,69h,0FEh,67h,0FEh,65h,0FEh,62h,0FEh,5Bh,0FFh
        db 60h,0FEh,0FEh,0FEh,63h,0FEh,0FEh,0FEh,68h,0FEh,0FEh,0FEh,67h,0FEh,0FEh,0FEh
        db 5Ah,0FEh,0FEh,0FEh,67h,0FEh,0FEh,0FEh,65h,0FEh,0FEh,0FEh,63h,0FEh,0FEh,0FEh
        db 60h,0FEh,0FEh,0FEh,65h,0FEh,0FEh,0FEh,68h,0FEh,0FEh,0FEh,67h,0FEh,0FEh,0FEh
        db 5Bh,0FEh,0FEh,0FEh,62h,0FEh,0FEh,0FEh,67h,0FEh,0FEh,0FEh,65h,0FEh,0FEh,0FEh
        db 63h,0FEh,67h,0FEh,70h,0FEh,6Ah,0FEh,67h,0FEh,63h,0FEh,62h,0FEh,60h,0FFh
        db 60h,0FEh,63h,0FEh,68h,0FEh,70h,0FEh,6Ah,0FEh,68h,0FEh,67h,0FEh,63h,0FFh
        db 62h,0FEh,67h,0FEh,6Bh,0FEh,72h,0FEh,70h,0FEh,6Bh,0FEh,67h,0FEh,65h,0FFh
        db 67h,0FEh,63h,0FEh,60h,0FEh,5Ah,0FEh,57h,0FEh,53h,0FEh,52h,0FEh,57h,0FEh
glass_opna_music_ch1:
        db 53h,0FEh,0FEh,0FEh,57h,0FEh,0FEh,0FEh,53h,0FEh,0FEh,0FEh,4Ah,0FEh,0FEh,0FEh
        db 53h,0FEh,0FEh,0FEh,50h,0FEh,0FEh,0FEh,53h,0FEh,0FEh,0FEh,4Ah,0FEh,0FEh,0FEh
        db 4Ah,0FEh,0FEh,0FEh,53h,0FEh,0FEh,0FEh,57h,0FEh,0FEh,0FEh,55h,0FEh,0FEh,0FEh
        db 52h,0FEh,0FEh,0FEh,55h,0FEh,0FEh,0FEh,52h,0FEh,0FEh,0FEh,48h,0FEh,0FEh,0FEh
        db 47h,0FEh,0FEh,0FEh,53h,0FEh,0FEh,0FEh,57h,0FEh,0FEh,0FEh,53h,0FEh,0FEh,0FEh
        db 50h,0FEh,0FEh,0FEh,58h,0FEh,0FEh,0FEh,50h,0FEh,0FEh,0FEh,48h,0FEh,0FEh,0FEh
        db 50h,0FEh,0FEh,0FEh,53h,0FEh,0FEh,0FEh,50h,0FEh,0FEh,0FEh,4Ah,0FEh,0FEh,0FEh
        db 4Bh,0FEh,0FEh,0FEh,52h,0FEh,0FEh,0FEh,57h,0FEh,0FEh,0FEh,52h,0FEh,0FEh,0FEh
        db 53h,0FEh,0FEh,0FEh,50h,0FEh,0FEh,0FEh,53h,0FEh,0FEh,0FEh,50h,0FEh,0FEh,0FEh
        db 53h,0FEh,0FEh,0FEh,4Ah,0FEh,0FEh,0FEh,53h,0FEh,0FEh,0FEh,4Ah,0FEh,0FEh,0FEh
        db 48h,0FEh,0FEh,0FEh,50h,0FEh,0FEh,0FEh,53h,0FEh,0FEh,0FEh,50h,0FEh,0FEh,0FEh
        db 52h,0FEh,0FEh,0FEh,4Bh,0FEh,0FEh,0FEh,52h,0FEh,0FEh,0FEh,4Bh,0FEh,0FEh,0FEh
        db 57h,0FEh,0FEh,0FEh,53h,0FEh,0FEh,0FEh,57h,0FEh,0FEh,0FEh,53h,0FEh,0FEh,0FEh
        db 53h,0FEh,0FEh,0FEh,50h,0FEh,0FEh,0FEh,53h,0FEh,0FEh,0FEh,50h,0FEh,0FEh,0FEh
        db 52h,0FEh,0FEh,0FEh,4Bh,0FEh,0FEh,0FEh,52h,0FEh,0FEh,0FEh,4Bh,0FEh,0FEh,0FEh
        db 53h,0FEh,0FEh,0FEh,4Ah,0FEh,0FEh,0FEh,47h,0FEh,0FEh,0FEh,50h,0FEh,0FEh,0FEh
glass_opna_music_ch2:
        db 30h,0FEh,0FEh,0FFh,37h,0FEh,0FEh,0FFh,30h,0FEh,0FEh,0FFh,37h,0FEh,0FEh,0FFh
        db 28h,0FEh,0FEh,0FFh,33h,0FEh,0FEh,0FFh,28h,0FEh,0FEh,0FFh,33h,0FEh,0FEh,0FFh
        db 33h,0FEh,0FEh,0FFh,3Ah,0FEh,0FEh,0FFh,33h,0FEh,0FEh,0FFh,3Ah,0FEh,0FEh,0FFh
        db 2Ah,0FEh,0FEh,0FFh,35h,0FEh,0FEh,0FFh,2Ah,0FEh,0FEh,0FFh,35h,0FEh,0FEh,0FFh
        db 30h,0FEh,0FEh,0FFh,37h,0FEh,0FEh,0FFh,30h,0FEh,0FEh,0FFh,37h,0FEh,0FEh,0FFh
        db 35h,0FEh,0FEh,0FFh,40h,0FEh,0FEh,0FFh,35h,0FEh,0FEh,0FFh,40h,0FEh,0FEh,0FFh
        db 28h,0FEh,0FEh,0FFh,33h,0FEh,0FEh,0FFh,28h,0FEh,0FEh,0FFh,33h,0FEh,0FEh,0FFh
        db 27h,0FEh,0FEh,0FFh,32h,0FEh,0FEh,0FFh,27h,0FEh,0FEh,0FFh,32h,0FEh,0FEh,0FFh
        db 28h,0FEh,0FEh,0FFh,33h,0FEh,0FEh,0FFh,28h,0FEh,0FEh,0FFh,33h,0FEh,0FEh,0FFh
        db 27h,0FEh,0FEh,0FFh,32h,0FEh,0FEh,0FFh,27h,0FEh,0FEh,0FFh,32h,0FEh,0FEh,0FFh
        db 35h,0FEh,0FEh,0FFh,40h,0FEh,0FEh,0FFh,35h,0FEh,0FEh,0FFh,40h,0FEh,0FEh,0FFh
        db 27h,0FEh,0FEh,0FFh,32h,0FEh,0FEh,0FFh,27h,0FEh,0FEh,0FFh,32h,0FEh,0FEh,0FFh
        db 30h,0FEh,0FEh,0FFh,37h,0FEh,0FEh,0FFh,30h,0FEh,0FEh,0FFh,37h,0FEh,0FEh,0FFh
        db 28h,0FEh,0FEh,0FFh,33h,0FEh,0FEh,0FFh,28h,0FEh,0FEh,0FFh,33h,0FEh,0FEh,0FFh
        db 27h,0FEh,0FEh,0FFh,32h,0FEh,0FEh,0FFh,27h,0FEh,0FEh,0FFh,32h,0FEh,0FEh,0FFh
        db 30h,0FEh,0FEh,0FFh,37h,0FEh,0FEh,0FFh,30h,0FEh,0FEh,0FFh,37h,0FEh,0FEh,0FFh
