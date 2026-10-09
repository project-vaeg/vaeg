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
; 2. Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in the
;    documentation and/or other materials provided with the distribution.
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

; Static GLASS ORBIT scene data retained for the VA port.  P0 does not draw
; from these tables; no video or SGP dependency appears in this file.

align 2, db 0
render_frame_counter dw 0

glass_star_subtick   db 0
glass_star_scroll_x  dw 0
glass_star_plot_x    dw 0
glass_star_plot_y    dw 0
glass_star_twinkle   db 0

glass_sin_x          dw 0
glass_cos_x          dw 0
glass_sin_y          dw 0
glass_cos_y          dw 0
glass_tmp_x          dw 0
glass_tmp_y          dw 0
glass_tmp_z1         dw 0
glass_tmp_z2         dw 0
glass_den            dw 1
glass_accum          dd 0

glass_face_x0        dw 0
glass_face_y0        dw 0
glass_face_dx10      dw 0
glass_face_dy10      dw 0
glass_face_dx20      dw 0
glass_face_dy20      dw 0
glass_face_cross_lo  dw 0
glass_face_cross_hi  dw 0

glass_grid_endpoints dw 0,80,160,240,320,400,480,560,639
glass_grid_y         dw 270,294,322,354,389

glass_cube_vertices:
        dw -100,-100,-100
        dw  100,-100,-100
        dw  100, 100,-100
        dw -100, 100,-100
        dw -100,-100, 100
        dw  100,-100, 100
        dw  100, 100, 100
        dw -100, 100, 100

glass_cube_projected times 8*3 dw 0

; v0,v1,v2,v3,source face colour
glass_cube_faces:
        db 0,1,2,3,GLASS_FACE_0
        db 4,7,6,5,GLASS_FACE_1
        db 0,4,5,1,GLASS_FACE_2
        db 3,2,6,7,GLASS_FACE_3
        db 0,3,7,4,GLASS_FACE_4
        db 1,5,6,2,GLASS_FACE_5

; endpoint0,endpoint1,source edge colour
glass_cube_edges:
        db 0,1,GLASS_EDGE_A, 1,2,GLASS_EDGE_A, 2,3,GLASS_EDGE_A, 3,0,GLASS_EDGE_A
        db 4,5,GLASS_EDGE_B, 5,6,GLASS_EDGE_B, 6,7,GLASS_EDGE_B, 7,4,GLASS_EDGE_B
        db 0,4,GLASS_EDGE_C, 1,5,GLASS_EDGE_C, 2,6,GLASS_EDGE_C, 3,7,GLASS_EDGE_C

%macro glass_star 4
        dw %1,%2
        db %3,%4
%endmacro

glass_star_table:
        glass_star 130,176,0,50
        glass_star 239,106,0,5
        glass_star 465,33,0,251
        glass_star 119,170,0,36
        glass_star 323,153,0,189
        glass_star 556,98,0,250
        glass_star 24,36,0,92
        glass_star 283,135,0,50
        glass_star 68,41,0,82
        glass_star 391,41,0,242
        glass_star 310,56,0,225
        glass_star 475,112,0,248
        glass_star 44,120,0,19
        glass_star 21,176,0,210
        glass_star 163,56,0,180
        glass_star 630,64,0,213
        glass_star 485,208,0,75
        glass_star 252,157,0,185
        glass_star 136,99,0,140
        glass_star 486,13,0,33
        glass_star 599,89,0,99
        glass_star 174,171,0,255
        glass_star 520,67,0,135
        glass_star 168,43,0,15
        glass_star 500,40,0,230
        glass_star 147,164,0,156
        glass_star 312,50,0,132
        glass_star 164,159,0,17
        glass_star 491,57,0,49
        glass_star 255,56,0,228
        glass_star 635,18,0,67
        glass_star 249,218,0,89
        glass_star 114,167,0,28
        glass_star 166,217,0,57
        glass_star 572,114,0,92
        glass_star 165,166,0,195
        glass_star 292,179,0,16
        glass_star 513,172,0,193
        glass_star 326,183,0,16
        glass_star 564,7,0,172
        glass_star 248,198,0,21
        glass_star 20,20,0,25
        glass_star 258,18,0,253
        glass_star 206,90,0,71
        glass_star 35,9,0,46
        glass_star 43,151,0,61
        glass_star 195,81,0,18
        glass_star 478,13,0,210
        glass_star 435,158,0,1
        glass_star 389,25,0,203
        glass_star 81,155,0,54
        glass_star 94,140,0,190
        glass_star 238,106,0,17
        glass_star 192,79,0,232
        glass_star 576,118,0,136
        glass_star 288,204,0,211
        glass_star 606,215,0,107
        glass_star 355,142,0,168
        glass_star 94,111,0,91
        glass_star 564,48,0,90
        glass_star 332,40,0,189
        glass_star 328,117,0,78
        glass_star 625,90,0,22
        glass_star 334,45,0,248
        glass_star 548,117,0,190
        glass_star 416,10,0,13
        glass_star 346,162,0,97
        glass_star 262,202,0,186
        glass_star 451,137,0,20
        glass_star 564,4,0,227
        glass_star 218,172,0,199
        glass_star 505,229,0,25
        glass_star 351,86,0,136
        glass_star 415,17,0,238
        glass_star 541,133,0,84
        glass_star 559,236,0,22
        glass_star 184,12,0,106
        glass_star 214,139,0,235
        glass_star 238,183,0,109
        glass_star 161,203,0,114
        glass_star 552,149,0,222
        glass_star 236,208,0,105
        glass_star 167,227,0,184
        glass_star 424,94,0,225
        glass_star 161,117,0,87
        glass_star 233,50,0,42
        glass_star 68,37,0,42
        glass_star 394,12,0,76
        glass_star 371,109,0,167
        glass_star 34,144,0,22
        glass_star 315,144,0,174
        glass_star 415,74,0,62
        glass_star 39,226,0,102
        glass_star 293,83,0,76
        glass_star 548,7,0,176
        glass_star 123,70,0,12
        glass_star 550,153,0,138
        glass_star 54,229,0,49
        glass_star 367,30,0,255
        glass_star 569,224,0,49
        glass_star 131,92,0,157
        glass_star 445,237,0,24
        glass_star 84,48,0,170
        glass_star 410,96,0,156
        glass_star 302,211,0,174
        glass_star 522,185,0,162
        glass_star 173,19,0,3
        glass_star 624,30,0,109
        glass_star 524,139,0,229
        glass_star 279,208,0,113
        glass_star 430,100,0,85
        glass_star 99,112,0,72
        glass_star 591,166,0,11
        glass_star 235,96,0,232
        glass_star 137,55,0,217
        glass_star 317,44,0,69
        glass_star 222,180,0,86
        glass_star 561,161,0,21
        glass_star 221,10,0,201
        glass_star 224,34,0,199
        glass_star 237,41,0,176
        glass_star 521,50,0,17
        glass_star 30,222,0,202
        glass_star 333,16,0,193
        glass_star 535,22,0,184
        glass_star 465,173,0,235
        glass_star 283,77,0,110
        glass_star 255,236,0,39
        glass_star 571,223,0,211
        glass_star 68,237,0,168
        glass_star 234,126,0,191
        glass_star 40,57,0,196
        glass_star 421,31,0,154
        glass_star 469,35,0,83
        glass_star 50,171,0,229
        glass_star 95,92,0,31
        glass_star 206,219,0,198
        glass_star 504,208,0,94
        glass_star 483,5,0,23
        glass_star 238,115,0,251
        glass_star 140,195,0,0
        glass_star 506,206,0,159
        glass_star 347,83,0,78
        glass_star 228,208,0,64
        glass_star 13,198,0,195
        glass_star 420,234,0,149
        glass_star 614,215,0,147
        glass_star 48,179,0,178
        glass_star 551,203,0,116
        glass_star 437,154,0,186
        glass_star 41,141,0,105
        glass_star 39,48,0,77
        glass_star 477,207,0,112
        glass_star 489,4,0,224
        glass_star 134,107,0,246
        glass_star 549,24,0,102
        glass_star 428,201,0,186
        glass_star 56,56,0,17
        glass_star 350,138,0,59
        glass_star 458,224,0,101
        glass_star 322,237,0,180
        glass_star 477,203,0,172
        glass_star 339,72,0,230
        glass_star 262,73,0,29
        glass_star 135,158,0,75
        glass_star 458,215,0,164
        glass_star 341,93,0,176
        glass_star 26,20,0,207
        glass_star 246,157,0,47
        glass_star 275,175,0,39
        glass_star 104,27,0,13
        glass_star 554,163,0,69
        glass_star 620,64,0,239
        glass_star 472,51,0,5
        glass_star 360,224,0,5
        glass_star 446,55,0,103
        glass_star 459,27,0,133
        glass_star 17,151,0,31
        glass_star 365,108,0,172
        glass_star 511,169,0,247
        glass_star 82,96,0,184
        glass_star 582,143,0,67
        glass_star 138,157,0,232
        glass_star 373,68,0,185
        glass_star 241,129,0,92
        glass_star 460,223,0,193
        glass_star 388,187,0,192
        glass_star 311,4,0,121
        glass_star 546,171,0,194
        glass_star 431,236,0,179
        glass_star 478,134,1,83
        glass_star 125,101,1,9
        glass_star 402,113,1,183
        glass_star 524,184,1,159
        glass_star 538,122,1,78
        glass_star 499,66,1,135
        glass_star 626,211,1,200
        glass_star 354,221,1,19
        glass_star 535,78,1,154
        glass_star 418,208,1,254
        glass_star 166,36,1,252
        glass_star 256,52,1,45
        glass_star 528,199,1,142
        glass_star 27,19,1,187
        glass_star 530,136,1,103
        glass_star 433,221,1,198
        glass_star 82,127,1,71
        glass_star 293,208,1,175
        glass_star 459,18,1,126
        glass_star 82,179,1,63
        glass_star 419,37,1,74
        glass_star 357,10,1,15
        glass_star 621,65,1,77
        glass_star 400,174,1,201
        glass_star 577,72,1,92
        glass_star 490,219,1,190
        glass_star 597,176,1,97
        glass_star 99,51,1,175
        glass_star 310,125,1,61
        glass_star 159,71,1,81
        glass_star 531,117,1,135
        glass_star 33,192,1,45
        glass_star 506,198,1,195
        glass_star 0,155,1,229
        glass_star 389,157,1,152
        glass_star 148,44,1,51
        glass_star 25,20,1,210
        glass_star 291,235,1,1
        glass_star 147,133,1,76
        glass_star 357,109,1,127
        glass_star 346,227,1,252
        glass_star 291,110,1,146
        glass_star 298,101,1,104
        glass_star 124,25,1,156
        glass_star 292,222,1,96
        glass_star 362,229,1,211
        glass_star 127,73,1,135
        glass_star 38,142,1,97
        glass_star 46,175,1,223
        glass_star 312,20,1,1
        glass_star 243,192,1,1
        glass_star 175,46,1,232
        glass_star 435,43,2,30
        glass_star 360,189,2,51
        glass_star 188,2,2,67
        glass_star 38,60,2,12
        glass_star 366,80,2,195
        glass_star 458,13,2,251
        glass_star 511,82,2,249
        glass_star 533,101,2,128
        glass_star 538,155,2,35
        glass_star 391,69,2,81
        glass_star 569,118,2,91
        glass_star 161,6,2,199
        glass_star 56,145,2,81
        glass_star 28,124,2,60
glass_star_table_end:

%if glass_star_table_end - glass_star_table != GLASS_STAR_COUNT * 6
%error "GLASS ORBIT star table size mismatch"
%endif
%undef glass_star

align 2, db 0
sin_table:
        dw 0,402,804,1205,1606,2006,2404,2801
        dw 3196,3590,3981,4370,4756,5139,5520,5897
        dw 6270,6639,7005,7366,7723,8076,8423,8765
        dw 9102,9434,9760,10080,10394,10702,11003,11297
        dw 11585,11866,12140,12406,12665,12916,13160,13395
        dw 13623,13842,14053,14256,14449,14635,14811,14978
        dw 15137,15286,15426,15557,15679,15791,15893,15986
        dw 16069,16143,16207,16261,16305,16340,16364,16379
        dw 16384,16379,16364,16340,16305,16261,16207,16143
        dw 16069,15986,15893,15791,15679,15557,15426,15286
        dw 15137,14978,14811,14635,14449,14256,14053,13842
        dw 13623,13395,13160,12916,12665,12406,12140,11866
        dw 11585,11297,11003,10702,10394,10080,9760,9434
        dw 9102,8765,8423,8076,7723,7366,7005,6639
        dw 6270,5897,5520,5139,4756,4370,3981,3590
        dw 3196,2801,2404,2006,1606,1205,804,402
        dw 0,-402,-804,-1205,-1606,-2006,-2404,-2801
        dw -3196,-3590,-3981,-4370,-4756,-5139,-5520,-5897
        dw -6270,-6639,-7005,-7366,-7723,-8076,-8423,-8765
        dw -9102,-9434,-9760,-10080,-10394,-10702,-11003,-11297
        dw -11585,-11866,-12140,-12406,-12665,-12916,-13160,-13395
        dw -13623,-13842,-14053,-14256,-14449,-14635,-14811,-14978
        dw -15137,-15286,-15426,-15557,-15679,-15791,-15893,-15986
        dw -16069,-16143,-16207,-16261,-16305,-16340,-16364,-16379
        dw -16384,-16379,-16364,-16340,-16305,-16261,-16207,-16143
        dw -16069,-15986,-15893,-15791,-15679,-15557,-15426,-15286
        dw -15137,-14978,-14811,-14635,-14449,-14256,-14053,-13842
        dw -13623,-13395,-13160,-12916,-12665,-12406,-12140,-11866
        dw -11585,-11297,-11003,-10702,-10394,-10080,-9760,-9434
        dw -9102,-8765,-8423,-8076,-7723,-7366,-7005,-6639
        dw -6270,-5897,-5520,-5139,-4756,-4370,-3981,-3590
        dw -3196,-2801,-2404,-2006,-1606,-1205,-804,-402
sin_table_end:

%if sin_table_end - sin_table != 256 * 2
%error "GLASS ORBIT sine table size mismatch"
%endif
