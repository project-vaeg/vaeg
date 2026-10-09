; Copyright (c) 2026 Nakata Maho
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
; 1. Redistributions of source code must retain the above copyright notice,
;    this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright notice,
;    this list of conditions and the following disclaimer in the documentation
;    and/or other materials provided with the distribution.
;
; VA2 /87 seam probe.  Assemble as a flat 16-bit binary and load it at
; 7FF0:0000 after reserving the machine-language area with CLEAR.
; The two stores deliberately use the 8087 short-real and long-real forms.

bits 16
org 0

start:
    fninit
    fld1
    fld1
    fadd st0, st1
    fstp dword [cs:0x0020]
    fld1
    fld1
    fadd st0, st1
    fstp qword [cs:0x0030]
    fwait
    retf
