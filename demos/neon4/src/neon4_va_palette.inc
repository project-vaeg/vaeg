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
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS
; OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
; OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
; IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
; INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
; BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
; AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
; OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
; USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

; NEON4 original PEGC palette conversion tables.
; Source: demos/neon4_1_0/VIDEO4_DATA.INC:pegc_palette_grb.
; Source order is GRB.  The 8bpp profile uses direct VA RGB332 bytes;
; the 16bpp profile uses the direct-colour table below and must not pass
; through RGB332 first.
align 2
n4_va_rgb332_from_pegc:
        db 00h, 00h, 00h, 24h, 24h, 24h, 25h, 49h, 49h, 49h, 49h, 49h, 6dh, 6dh, 6dh, 6dh
        db 92h, 92h, 92h, 92h, 0b6h, 0b6h, 0b6h, 0b6h, 0b6h, 0dah, 0dbh, 0dbh, 0dbh, 0ffh, 0ffh, 0ffh
        db 00h, 00h, 00h, 00h, 20h, 21h, 21h, 21h, 25h, 25h, 25h, 45h, 45h, 45h, 45h, 46h
        db 46h, 46h, 66h, 66h, 66h, 66h, 66h, 66h, 6ah, 8ah, 8bh, 8bh, 8bh, 8bh, 8bh, 8bh
        db 00h, 00h, 20h, 20h, 20h, 21h, 41h, 41h, 41h, 45h, 45h, 65h, 65h, 65h, 65h, 86h
        db 86h, 86h, 86h, 86h, 0a6h, 0a6h, 0a6h, 0a6h, 0a6h, 0c6h, 0c7h, 0cbh, 0cbh, 0ebh, 0ebh, 0ebh
        db 00h, 00h, 20h, 20h, 20h, 20h, 44h, 44h, 44h, 44h, 44h, 65h, 65h, 65h, 65h, 85h
        db 85h, 85h, 89h, 89h, 0a9h, 0a9h, 0a9h, 0a9h, 0a9h, 0c9h, 0c9h, 0c9h, 0c9h, 0e9h, 0edh, 0eeh
        db 00h, 00h, 24h, 24h, 24h, 24h, 48h, 48h, 48h, 48h, 48h, 6ch, 6ch, 6ch, 6ch, 90h
        db 90h, 90h, 90h, 90h, 0b4h, 0b5h, 0b5h, 0b5h, 0b5h, 0d9h, 0d9h, 0d9h, 0d9h, 0feh, 0feh, 0ffh
        db 00h, 00h, 04h, 04h, 04h, 04h, 28h, 28h, 28h, 28h, 28h, 2ch, 2ch, 2ch, 2ch, 30h
        db 50h, 50h, 50h, 50h, 54h, 54h, 54h, 54h, 54h, 58h, 78h, 78h, 78h, 7ch, 7ch, 7ch
        db 00h, 00h, 04h, 04h, 04h, 05h, 09h, 09h, 09h, 29h, 29h, 2dh, 2dh, 2dh, 2dh, 32h
        db 32h, 32h, 32h, 32h, 36h, 36h, 36h, 36h, 36h, 3ah, 3bh, 5bh, 5bh, 5fh, 5fh, 5fh
        db 00h, 00h, 24h, 24h, 25h, 25h, 49h, 49h, 49h, 49h, 49h, 6dh, 6dh, 6dh, 6dh, 92h
        db 92h, 92h, 92h, 92h, 0b6h, 0b6h, 0b6h, 0b6h, 0b6h, 0dah, 0dbh, 0dbh, 0dbh, 0ffh, 0ffh, 0ffh
n4_va_rgb332_from_pegc_end:
%if n4_va_rgb332_from_pegc_end - n4_va_rgb332_from_pegc != 256
%error "NEON4 VA palette table must contain 256 RGB332 entries"
%endif

; Direct VA colour table for NEON4_P5_BPP=16.
; Each source PEGC triplet is converted independently from 8-bit G/R/B
; to the VA direct-colour word layout G6[15:10], R5[9:5], B5[4:0].
; This preserves the original 256-entry palette selection without an
; intermediate RGB332 quantisation.
align 2
n4_va_rgb565_from_pegc:
        dw 0000h, 0821h, 1042h, 1863h, 2084h, 28A5h, 30C6h, 38E7h, 4108h, 4929h, 514Ah, 596Bh, 618Ch, 69ADh, 71CEh, 79EFh
        dw 8610h, 8E31h, 9652h, 9E73h, 0A694h, 0AEB5h, 0B6D6h, 0BEF7h, 0C718h, 0CF39h, 0D75Ah, 0DF7Bh, 0E79Ch, 0EFBDh, 0F7DEh, 0FFFFh
        dw 0001h, 0402h, 0823h, 1024h, 1425h, 1826h, 2047h, 2448h, 2849h, 2C6Ah, 346Bh, 386Ch, 3C6Dh, 448Eh, 488Fh, 4C90h
        dw 5490h, 58B1h, 5CB2h, 60B3h, 64D4h, 6CD5h, 70D6h, 74D7h, 7CF8h, 80F9h, 84FAh, 891Bh, 911Ch, 951Dh, 993Eh, 0A13Fh
        dw 0801h, 1002h, 1803h, 2024h, 2825h, 3026h, 3827h, 4048h, 4849h, 504Ah, 586Bh, 606Ch, 686Dh, 706Eh, 788Fh, 8090h
        dw 8490h, 8C91h, 9492h, 9CB3h, 0A4B4h, 0ACB5h, 0B4B6h, 0BCD7h, 0C4D8h, 0CCD9h, 0D4FAh, 0DCFBh, 0E4FCh, 0ECFDh, 0F51Eh, 0FD1Fh
        dw 0800h, 1000h, 1821h, 2021h, 2822h, 3043h, 3843h, 4064h, 4864h, 5064h, 5885h, 6085h, 6886h, 70A7h, 78A7h, 80C8h
        dw 84C8h, 8CC9h, 94E9h, 9CE9h, 0A50Ah, 0AD0Bh, 0B50Bh, 0BD2Ch, 0C52Ch, 0CD2Dh, 0D54Dh, 0DD4Dh, 0E56Eh, 0ED6Fh, 0F56Fh, 0FD90h
        dw 0820h, 1040h, 1860h, 2081h, 28A1h, 30C1h, 38E1h, 4102h, 4922h, 5142h, 5963h, 6183h, 69A3h, 71C3h, 79E4h, 8204h
        dw 8604h, 8E24h, 9644h, 9E65h, 0A685h, 0AEA5h, 0B6C5h, 0BEE6h, 0C706h, 0CF26h, 0D747h, 0DF67h, 0E787h, 0FFF6h, 0FFF9h, 0FFFCh
        dw 0020h, 0440h, 0860h, 0C80h, 0CA0h, 10C1h, 14E1h, 1901h, 1D21h, 2141h, 2561h, 2981h, 2DA1h, 31C2h, 35E2h, 3602h
        dw 3A02h, 3E22h, 4242h, 4662h, 4A83h, 4EA3h, 52C3h, 56E3h, 5703h, 5B23h, 5F43h, 6363h, 6784h, 6BA4h, 6FC4h, 73E4h
        dw 0021h, 0042h, 0463h, 0484h, 08A5h, 08C6h, 0CE7h, 0D08h, 1129h, 154Ah, 156Bh, 198Ch, 19ADh, 1DCEh, 1DEFh, 2210h
        dw 2210h, 2631h, 2652h, 2A73h, 2A94h, 2EB5h, 2ED6h, 32F7h, 3318h, 3739h, 375Ah, 3B7Bh, 3B9Ch, 3FBDh, 3FDEh, 43FFh
        dw 0821h, 1042h, 1863h, 2084h, 28A5h, 30C6h, 38E7h, 4108h, 4929h, 514Ah, 596Bh, 618Ch, 69ADh, 71CEh, 79EFh, 8210h
        dw 8611h, 8E32h, 9653h, 9E74h, 0A695h, 0AEB6h, 0B6D6h, 0BEF7h, 0C718h, 0CF39h, 0D75Ah, 0DF7Bh, 0E79Ch, 0EFBDh, 0F7DEh, 0FFFFh
n4_va_rgb565_from_pegc_end:
%if n4_va_rgb565_from_pegc_end - n4_va_rgb565_from_pegc != 512
%error "NEON4 VA direct palette table must contain 256 words"
%endif
