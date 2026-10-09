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
; WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
; ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
; POSSIBILITY OF SUCH DAMAGE.
;
; Fixed-seed LCG output. Initial top-left y values cover 0..199.
; Each record selects one of sixteen deterministic integer velocity vectors.
; The runtime clamps the 24-pixel sprite top to 0..176 before each draw.
    SPRITE_RECORD 43, 87, -1, 2, orb_hsv_00
    SPRITE_RECORD 87, 43, 0, -1, orb_hsv_01
    SPRITE_RECORD 0, 135, 0, -1, orb_hsv_02
    SPRITE_RECORD 43, 195, 0, 1, orb_hsv_03
    SPRITE_RECORD 217, 71, 1, -2, orb_hsv_04
    SPRITE_RECORD 8, 99, 1, -1, orb_hsv_05
    SPRITE_RECORD 116, 199, -1, -1, orb_hsv_06
    SPRITE_RECORD 213, 91, 1, -1, orb_hsv_07
    SPRITE_RECORD 235, 111, 0, 1, orb_hsv_08
    SPRITE_RECORD 42, 67, 2, 1, orb_hsv_09
    SPRITE_RECORD 265, 31, -1, -2, orb_hsv_10
    SPRITE_RECORD 241, 3, 1, 2, orb_hsv_11
    SPRITE_RECORD 43, 71, -2, 1, orb_hsv_12
    SPRITE_RECORD 270, 123, 1, -1, orb_hsv_13
    SPRITE_RECORD 203, 183, 0, 1, orb_hsv_14
    SPRITE_RECORD 29, 147, -1, 1, orb_hsv_15
    SPRITE_RECORD 61, 7, 0, -1, orb_hsv_00
    SPRITE_RECORD 157, 59, 2, 1, orb_hsv_01
    SPRITE_RECORD 80, 71, -1, 2, orb_hsv_02
    SPRITE_RECORD 250, 75, 2, 1, orb_hsv_03
    SPRITE_RECORD 211, 23, -2, 1, orb_hsv_04
    SPRITE_RECORD 71, 35, 1, 2, orb_hsv_05
    SPRITE_RECORD 141, 47, -1, 1, orb_hsv_06
    SPRITE_RECORD 18, 51, -1, 2, orb_hsv_07
    SPRITE_RECORD 42, 23, 1, 0, orb_hsv_08
    SPRITE_RECORD 2, 139, 0, -1, orb_hsv_09
    SPRITE_RECORD 178, 135, 0, 1, orb_hsv_10
    SPRITE_RECORD 268, 83, -1, 1, orb_hsv_11
    SPRITE_RECORD 31, 63, -1, 2, orb_hsv_12
    SPRITE_RECORD 202, 155, -1, -1, orb_hsv_13
    SPRITE_RECORD 134, 199, 2, -1, orb_hsv_14
    SPRITE_RECORD 214, 3, -1, 1, orb_hsv_15
    SPRITE_RECORD 247, 31, 0, 1, orb_hsv_00
    SPRITE_RECORD 201, 91, 0, 1, orb_hsv_01
    SPRITE_RECORD 46, 191, -2, 1, orb_hsv_02
    SPRITE_RECORD 262, 3, -1, 2, orb_hsv_03
    SPRITE_RECORD 246, 167, -1, 0, orb_hsv_04
    SPRITE_RECORD 29, 75, 1, 1, orb_hsv_05
    SPRITE_RECORD 55, 23, -2, 1, orb_hsv_06
    SPRITE_RECORD 226, 115, 1, -2, orb_hsv_07
    SPRITE_RECORD 214, 47, -1, 0, orb_hsv_08
    SPRITE_RECORD 44, 67, 1, -2, orb_hsv_09
    SPRITE_RECORD 234, 199, 0, 1, orb_hsv_10
    SPRITE_RECORD 59, 35, 1, -2, orb_hsv_11
    SPRITE_RECORD 177, 135, 0, 1, orb_hsv_12
    SPRITE_RECORD 257, 51, 1, -2, orb_hsv_13
    SPRITE_RECORD 222, 95, -1, -2, orb_hsv_14
    SPRITE_RECORD 287, 115, -1, -2, orb_hsv_15
    SPRITE_RECORD 229, 31, 2, -1, orb_hsv_00
    SPRITE_RECORD 160, 75, 1, 1, orb_hsv_01
    SPRITE_RECORD 240, 103, -1, -1, orb_hsv_02
    SPRITE_RECORD 31, 187, -1, -1, orb_hsv_03
    SPRITE_RECORD 178, 31, 2, -1, orb_hsv_04
    SPRITE_RECORD 51, 107, 1, 2, orb_hsv_05
    SPRITE_RECORD 119, 87, 1, 1, orb_hsv_06
    SPRITE_RECORD 114, 147, 1, -2, orb_hsv_07
    SPRITE_RECORD 241, 79, -2, -1, orb_hsv_08
    SPRITE_RECORD 50, 195, 1, -2, orb_hsv_09
    SPRITE_RECORD 225, 143, 2, -1, orb_hsv_10
    SPRITE_RECORD 113, 83, 1, 0, orb_hsv_11
    SPRITE_RECORD 278, 135, 1, -2, orb_hsv_12
    SPRITE_RECORD 236, 123, 0, -1, orb_hsv_13
    SPRITE_RECORD 281, 151, -2, -1, orb_hsv_14
    SPRITE_RECORD 266, 163, -2, -1, orb_hsv_15
    SPRITE_RECORD 44, 119, -1, 1, orb_hsv_00
    SPRITE_RECORD 63, 163, 1, -2, orb_hsv_01
    SPRITE_RECORD 201, 39, 2, -1, orb_hsv_02
    SPRITE_RECORD 100, 163, 2, 1, orb_hsv_03
    SPRITE_RECORD 260, 79, 1, 2, orb_hsv_04
    SPRITE_RECORD 291, 43, -1, 1, orb_hsv_05
    SPRITE_RECORD 203, 31, -1, 2, orb_hsv_06
    SPRITE_RECORD 259, 139, 1, 2, orb_hsv_07
    SPRITE_RECORD 192, 31, -1, 2, orb_hsv_08
    SPRITE_RECORD 208, 187, -1, 2, orb_hsv_09
    SPRITE_RECORD 43, 7, 2, -1, orb_hsv_10
    SPRITE_RECORD 35, 75, -2, -1, orb_hsv_11
    SPRITE_RECORD 49, 31, 2, 1, orb_hsv_12
    SPRITE_RECORD 52, 187, 1, 0, orb_hsv_13
    SPRITE_RECORD 134, 63, 1, 0, orb_hsv_14
    SPRITE_RECORD 269, 51, -1, 0, orb_hsv_15
    SPRITE_RECORD 205, 119, -1, 1, orb_hsv_00
    SPRITE_RECORD 245, 139, -1, -2, orb_hsv_01
    SPRITE_RECORD 174, 55, -1, 0, orb_hsv_02
    SPRITE_RECORD 15, 187, 1, -1, orb_hsv_03
    SPRITE_RECORD 239, 95, 0, -1, orb_hsv_04
    SPRITE_RECORD 112, 123, 1, -2, orb_hsv_05
    SPRITE_RECORD 174, 167, 1, -1, orb_hsv_06
    SPRITE_RECORD 229, 3, 1, 0, orb_hsv_07
    SPRITE_RECORD 54, 143, 0, 1, orb_hsv_08
    SPRITE_RECORD 200, 43, -2, 1, orb_hsv_09
    SPRITE_RECORD 80, 167, 2, -1, orb_hsv_10
    SPRITE_RECORD 9, 179, -1, 2, orb_hsv_11
    SPRITE_RECORD 184, 119, 1, 2, orb_hsv_12
    SPRITE_RECORD 118, 91, 0, 1, orb_hsv_13
    SPRITE_RECORD 286, 55, -2, -1, orb_hsv_14
    SPRITE_RECORD 23, 3, 2, 1, orb_hsv_15
    SPRITE_RECORD 58, 191, 1, -2, orb_hsv_00
    SPRITE_RECORD 44, 3, -1, 2, orb_hsv_01
    SPRITE_RECORD 195, 31, 0, -1, orb_hsv_02
    SPRITE_RECORD 28, 43, -2, -1, orb_hsv_03
    SPRITE_RECORD 180, 191, -2, 1, orb_hsv_04
    SPRITE_RECORD 74, 3, 1, -1, orb_hsv_05
    SPRITE_RECORD 193, 31, 1, 1, orb_hsv_06
    SPRITE_RECORD 195, 83, -1, -1, orb_hsv_07
    SPRITE_RECORD 96, 87, 1, -1, orb_hsv_08
    SPRITE_RECORD 14, 75, 0, 1, orb_hsv_09
    SPRITE_RECORD 222, 15, 1, 0, orb_hsv_10
    SPRITE_RECORD 240, 91, -1, 2, orb_hsv_11
    SPRITE_RECORD 289, 31, -2, -1, orb_hsv_12
    SPRITE_RECORD 135, 19, -2, -1, orb_hsv_13
    SPRITE_RECORD 166, 87, 1, 1, orb_hsv_14
    SPRITE_RECORD 246, 75, 0, 1, orb_hsv_15
    SPRITE_RECORD 19, 7, 1, 1, orb_hsv_00
    SPRITE_RECORD 292, 91, -1, 0, orb_hsv_01
    SPRITE_RECORD 206, 175, 0, 1, orb_hsv_02
    SPRITE_RECORD 182, 43, 1, -1, orb_hsv_03
    SPRITE_RECORD 30, 199, -2, 1, orb_hsv_04
    SPRITE_RECORD 231, 171, 0, 1, orb_hsv_05
    SPRITE_RECORD 133, 191, -1, 0, orb_hsv_06
    SPRITE_RECORD 131, 139, -2, 1, orb_hsv_07
    SPRITE_RECORD 105, 151, 1, 2, orb_hsv_08
    SPRITE_RECORD 229, 131, -1, 2, orb_hsv_09
    SPRITE_RECORD 261, 167, -1, 0, orb_hsv_10
    SPRITE_RECORD 27, 131, 1, 1, orb_hsv_11
    SPRITE_RECORD 70, 103, 2, -1, orb_hsv_12
    SPRITE_RECORD 122, 59, 0, 1, orb_hsv_13
    SPRITE_RECORD 182, 23, 0, -1, orb_hsv_14
    SPRITE_RECORD 65, 43, 0, 1, orb_hsv_15
; End of the deterministic 128-record table.
