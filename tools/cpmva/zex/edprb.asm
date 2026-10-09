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
; EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
; WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
; OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
; OF THE POSSIBILITY OF SUCH DAMAGE.

; M102 EDPRB: one-by-one probe of undefined ED opcodes (and the NEG and
; RETN duplicates) in the uPD9002 Z80 emulation mode.
;
; usage: EDPRB [start]   start = hex opcode (default 00). Opcodes below the
; start are skipped; the two documented controls (ED 44 NEG, ED 45 RETN) run
; only when start is 00. The output file is EDxx.TXT, xx = start.
;
; Each probe prints "ED xx " on the console first, so that a hang shows the
; opcode. It then sets fixed registers, a sandbox stack whose top word is the
; address of ret_trap, and sandbox memory at HL, IX, IY and around SP, runs
; ED xx once under DI, and captures everything on the way out: fall-through
; (NEXT) or a pop of the sandbox return address (RET). One result is exactly
; one 128-byte record, and the file is closed after every record so that the
; results survive a hang.
;
; Excluded: ED ED (CALLN), ED FD (RETEM), and the IM duplicates ED 4E, 66,
; 6E, 76 and 7E (the interrupt mode cannot be read back).
;
; No self-modifying code: every opcode has its own code path.

	org	100h

start:	ld	sp,stack
	call	parse
	ld	de,title
	call	conmsg
	ld	de,fcbout
	call	fopen

; ---- ED 44
p_c44:	ld	a,(startop)
	or	a
	jp	nz,k_c44
	ld	a,044h
	call	prep
	ld	hl,k_c44	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,044h	; under test
	jp	fall
k_c44:

; ---- ED 45
p_c45:	ld	a,(startop)
	or	a
	jp	nz,k_c45
	ld	a,045h
	call	prep
	ld	hl,k_c45	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,045h	; under test
	jp	fall
k_c45:

; ---- ED 00
p_00:	ld	a,(startop)
	or	a
	jp	nz,k_00
	ld	a,000h
	call	prep
	ld	hl,k_00	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,000h	; under test
	jp	fall
k_00:

; ---- ED 01
p_01:	ld	a,(startop)
	cp	002h
	jp	nc,k_01
	ld	a,001h
	call	prep
	ld	hl,k_01	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,001h	; under test
	jp	fall
k_01:

; ---- ED 02
p_02:	ld	a,(startop)
	cp	003h
	jp	nc,k_02
	ld	a,002h
	call	prep
	ld	hl,k_02	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,002h	; under test
	jp	fall
k_02:

; ---- ED 03
p_03:	ld	a,(startop)
	cp	004h
	jp	nc,k_03
	ld	a,003h
	call	prep
	ld	hl,k_03	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,003h	; under test
	jp	fall
k_03:

; ---- ED 04
p_04:	ld	a,(startop)
	cp	005h
	jp	nc,k_04
	ld	a,004h
	call	prep
	ld	hl,k_04	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,004h	; under test
	jp	fall
k_04:

; ---- ED 05
p_05:	ld	a,(startop)
	cp	006h
	jp	nc,k_05
	ld	a,005h
	call	prep
	ld	hl,k_05	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,005h	; under test
	jp	fall
k_05:

; ---- ED 06
p_06:	ld	a,(startop)
	cp	007h
	jp	nc,k_06
	ld	a,006h
	call	prep
	ld	hl,k_06	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,006h	; under test
	jp	fall
k_06:

; ---- ED 07
p_07:	ld	a,(startop)
	cp	008h
	jp	nc,k_07
	ld	a,007h
	call	prep
	ld	hl,k_07	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,007h	; under test
	jp	fall
k_07:

; ---- ED 08
p_08:	ld	a,(startop)
	cp	009h
	jp	nc,k_08
	ld	a,008h
	call	prep
	ld	hl,k_08	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,008h	; under test
	jp	fall
k_08:

; ---- ED 09
p_09:	ld	a,(startop)
	cp	00ah
	jp	nc,k_09
	ld	a,009h
	call	prep
	ld	hl,k_09	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,009h	; under test
	jp	fall
k_09:

; ---- ED 0A
p_0a:	ld	a,(startop)
	cp	00bh
	jp	nc,k_0a
	ld	a,00ah
	call	prep
	ld	hl,k_0a	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,00ah	; under test
	jp	fall
k_0a:

; ---- ED 0B
p_0b:	ld	a,(startop)
	cp	00ch
	jp	nc,k_0b
	ld	a,00bh
	call	prep
	ld	hl,k_0b	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,00bh	; under test
	jp	fall
k_0b:

; ---- ED 0C
p_0c:	ld	a,(startop)
	cp	00dh
	jp	nc,k_0c
	ld	a,00ch
	call	prep
	ld	hl,k_0c	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,00ch	; under test
	jp	fall
k_0c:

; ---- ED 0D
p_0d:	ld	a,(startop)
	cp	00eh
	jp	nc,k_0d
	ld	a,00dh
	call	prep
	ld	hl,k_0d	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,00dh	; under test
	jp	fall
k_0d:

; ---- ED 0E
p_0e:	ld	a,(startop)
	cp	00fh
	jp	nc,k_0e
	ld	a,00eh
	call	prep
	ld	hl,k_0e	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,00eh	; under test
	jp	fall
k_0e:

; ---- ED 0F
p_0f:	ld	a,(startop)
	cp	010h
	jp	nc,k_0f
	ld	a,00fh
	call	prep
	ld	hl,k_0f	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,00fh	; under test
	jp	fall
k_0f:

; ---- ED 10
p_10:	ld	a,(startop)
	cp	011h
	jp	nc,k_10
	ld	a,010h
	call	prep
	ld	hl,k_10	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,010h	; under test
	jp	fall
k_10:

; ---- ED 11
p_11:	ld	a,(startop)
	cp	012h
	jp	nc,k_11
	ld	a,011h
	call	prep
	ld	hl,k_11	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,011h	; under test
	jp	fall
k_11:

; ---- ED 12
p_12:	ld	a,(startop)
	cp	013h
	jp	nc,k_12
	ld	a,012h
	call	prep
	ld	hl,k_12	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,012h	; under test
	jp	fall
k_12:

; ---- ED 13
p_13:	ld	a,(startop)
	cp	014h
	jp	nc,k_13
	ld	a,013h
	call	prep
	ld	hl,k_13	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,013h	; under test
	jp	fall
k_13:

; ---- ED 14
p_14:	ld	a,(startop)
	cp	015h
	jp	nc,k_14
	ld	a,014h
	call	prep
	ld	hl,k_14	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,014h	; under test
	jp	fall
k_14:

; ---- ED 15
p_15:	ld	a,(startop)
	cp	016h
	jp	nc,k_15
	ld	a,015h
	call	prep
	ld	hl,k_15	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,015h	; under test
	jp	fall
k_15:

; ---- ED 16
p_16:	ld	a,(startop)
	cp	017h
	jp	nc,k_16
	ld	a,016h
	call	prep
	ld	hl,k_16	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,016h	; under test
	jp	fall
k_16:

; ---- ED 17
p_17:	ld	a,(startop)
	cp	018h
	jp	nc,k_17
	ld	a,017h
	call	prep
	ld	hl,k_17	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,017h	; under test
	jp	fall
k_17:

; ---- ED 18
p_18:	ld	a,(startop)
	cp	019h
	jp	nc,k_18
	ld	a,018h
	call	prep
	ld	hl,k_18	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,018h	; under test
	jp	fall
k_18:

; ---- ED 19
p_19:	ld	a,(startop)
	cp	01ah
	jp	nc,k_19
	ld	a,019h
	call	prep
	ld	hl,k_19	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,019h	; under test
	jp	fall
k_19:

; ---- ED 1A
p_1a:	ld	a,(startop)
	cp	01bh
	jp	nc,k_1a
	ld	a,01ah
	call	prep
	ld	hl,k_1a	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,01ah	; under test
	jp	fall
k_1a:

; ---- ED 1B
p_1b:	ld	a,(startop)
	cp	01ch
	jp	nc,k_1b
	ld	a,01bh
	call	prep
	ld	hl,k_1b	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,01bh	; under test
	jp	fall
k_1b:

; ---- ED 1C
p_1c:	ld	a,(startop)
	cp	01dh
	jp	nc,k_1c
	ld	a,01ch
	call	prep
	ld	hl,k_1c	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,01ch	; under test
	jp	fall
k_1c:

; ---- ED 1D
p_1d:	ld	a,(startop)
	cp	01eh
	jp	nc,k_1d
	ld	a,01dh
	call	prep
	ld	hl,k_1d	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,01dh	; under test
	jp	fall
k_1d:

; ---- ED 1E
p_1e:	ld	a,(startop)
	cp	01fh
	jp	nc,k_1e
	ld	a,01eh
	call	prep
	ld	hl,k_1e	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,01eh	; under test
	jp	fall
k_1e:

; ---- ED 1F
p_1f:	ld	a,(startop)
	cp	020h
	jp	nc,k_1f
	ld	a,01fh
	call	prep
	ld	hl,k_1f	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,01fh	; under test
	jp	fall
k_1f:

; ---- ED 20
p_20:	ld	a,(startop)
	cp	021h
	jp	nc,k_20
	ld	a,020h
	call	prep
	ld	hl,k_20	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,020h	; under test
	jp	fall
k_20:

; ---- ED 21
p_21:	ld	a,(startop)
	cp	022h
	jp	nc,k_21
	ld	a,021h
	call	prep
	ld	hl,k_21	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,021h	; under test
	jp	fall
k_21:

; ---- ED 22
p_22:	ld	a,(startop)
	cp	023h
	jp	nc,k_22
	ld	a,022h
	call	prep
	ld	hl,k_22	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,022h	; under test
	jp	fall
k_22:

; ---- ED 23
p_23:	ld	a,(startop)
	cp	024h
	jp	nc,k_23
	ld	a,023h
	call	prep
	ld	hl,k_23	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,023h	; under test
	jp	fall
k_23:

; ---- ED 24
p_24:	ld	a,(startop)
	cp	025h
	jp	nc,k_24
	ld	a,024h
	call	prep
	ld	hl,k_24	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,024h	; under test
	jp	fall
k_24:

; ---- ED 25
p_25:	ld	a,(startop)
	cp	026h
	jp	nc,k_25
	ld	a,025h
	call	prep
	ld	hl,k_25	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,025h	; under test
	jp	fall
k_25:

; ---- ED 26
p_26:	ld	a,(startop)
	cp	027h
	jp	nc,k_26
	ld	a,026h
	call	prep
	ld	hl,k_26	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,026h	; under test
	jp	fall
k_26:

; ---- ED 27
p_27:	ld	a,(startop)
	cp	028h
	jp	nc,k_27
	ld	a,027h
	call	prep
	ld	hl,k_27	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,027h	; under test
	jp	fall
k_27:

; ---- ED 28
p_28:	ld	a,(startop)
	cp	029h
	jp	nc,k_28
	ld	a,028h
	call	prep
	ld	hl,k_28	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,028h	; under test
	jp	fall
k_28:

; ---- ED 29
p_29:	ld	a,(startop)
	cp	02ah
	jp	nc,k_29
	ld	a,029h
	call	prep
	ld	hl,k_29	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,029h	; under test
	jp	fall
k_29:

; ---- ED 2A
p_2a:	ld	a,(startop)
	cp	02bh
	jp	nc,k_2a
	ld	a,02ah
	call	prep
	ld	hl,k_2a	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,02ah	; under test
	jp	fall
k_2a:

; ---- ED 2B
p_2b:	ld	a,(startop)
	cp	02ch
	jp	nc,k_2b
	ld	a,02bh
	call	prep
	ld	hl,k_2b	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,02bh	; under test
	jp	fall
k_2b:

; ---- ED 2C
p_2c:	ld	a,(startop)
	cp	02dh
	jp	nc,k_2c
	ld	a,02ch
	call	prep
	ld	hl,k_2c	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,02ch	; under test
	jp	fall
k_2c:

; ---- ED 2D
p_2d:	ld	a,(startop)
	cp	02eh
	jp	nc,k_2d
	ld	a,02dh
	call	prep
	ld	hl,k_2d	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,02dh	; under test
	jp	fall
k_2d:

; ---- ED 2E
p_2e:	ld	a,(startop)
	cp	02fh
	jp	nc,k_2e
	ld	a,02eh
	call	prep
	ld	hl,k_2e	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,02eh	; under test
	jp	fall
k_2e:

; ---- ED 2F
p_2f:	ld	a,(startop)
	cp	030h
	jp	nc,k_2f
	ld	a,02fh
	call	prep
	ld	hl,k_2f	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,02fh	; under test
	jp	fall
k_2f:

; ---- ED 30
p_30:	ld	a,(startop)
	cp	031h
	jp	nc,k_30
	ld	a,030h
	call	prep
	ld	hl,k_30	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,030h	; under test
	jp	fall
k_30:

; ---- ED 31
p_31:	ld	a,(startop)
	cp	032h
	jp	nc,k_31
	ld	a,031h
	call	prep
	ld	hl,k_31	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,031h	; under test
	jp	fall
k_31:

; ---- ED 32
p_32:	ld	a,(startop)
	cp	033h
	jp	nc,k_32
	ld	a,032h
	call	prep
	ld	hl,k_32	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,032h	; under test
	jp	fall
k_32:

; ---- ED 33
p_33:	ld	a,(startop)
	cp	034h
	jp	nc,k_33
	ld	a,033h
	call	prep
	ld	hl,k_33	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,033h	; under test
	jp	fall
k_33:

; ---- ED 34
p_34:	ld	a,(startop)
	cp	035h
	jp	nc,k_34
	ld	a,034h
	call	prep
	ld	hl,k_34	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,034h	; under test
	jp	fall
k_34:

; ---- ED 35
p_35:	ld	a,(startop)
	cp	036h
	jp	nc,k_35
	ld	a,035h
	call	prep
	ld	hl,k_35	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,035h	; under test
	jp	fall
k_35:

; ---- ED 36
p_36:	ld	a,(startop)
	cp	037h
	jp	nc,k_36
	ld	a,036h
	call	prep
	ld	hl,k_36	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,036h	; under test
	jp	fall
k_36:

; ---- ED 37
p_37:	ld	a,(startop)
	cp	038h
	jp	nc,k_37
	ld	a,037h
	call	prep
	ld	hl,k_37	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,037h	; under test
	jp	fall
k_37:

; ---- ED 38
p_38:	ld	a,(startop)
	cp	039h
	jp	nc,k_38
	ld	a,038h
	call	prep
	ld	hl,k_38	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,038h	; under test
	jp	fall
k_38:

; ---- ED 39
p_39:	ld	a,(startop)
	cp	03ah
	jp	nc,k_39
	ld	a,039h
	call	prep
	ld	hl,k_39	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,039h	; under test
	jp	fall
k_39:

; ---- ED 3A
p_3a:	ld	a,(startop)
	cp	03bh
	jp	nc,k_3a
	ld	a,03ah
	call	prep
	ld	hl,k_3a	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,03ah	; under test
	jp	fall
k_3a:

; ---- ED 3B
p_3b:	ld	a,(startop)
	cp	03ch
	jp	nc,k_3b
	ld	a,03bh
	call	prep
	ld	hl,k_3b	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,03bh	; under test
	jp	fall
k_3b:

; ---- ED 3C
p_3c:	ld	a,(startop)
	cp	03dh
	jp	nc,k_3c
	ld	a,03ch
	call	prep
	ld	hl,k_3c	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,03ch	; under test
	jp	fall
k_3c:

; ---- ED 3D
p_3d:	ld	a,(startop)
	cp	03eh
	jp	nc,k_3d
	ld	a,03dh
	call	prep
	ld	hl,k_3d	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,03dh	; under test
	jp	fall
k_3d:

; ---- ED 3E
p_3e:	ld	a,(startop)
	cp	03fh
	jp	nc,k_3e
	ld	a,03eh
	call	prep
	ld	hl,k_3e	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,03eh	; under test
	jp	fall
k_3e:

; ---- ED 3F
p_3f:	ld	a,(startop)
	cp	040h
	jp	nc,k_3f
	ld	a,03fh
	call	prep
	ld	hl,k_3f	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,03fh	; under test
	jp	fall
k_3f:

; ---- ED 4C
p_4c:	ld	a,(startop)
	cp	04dh
	jp	nc,k_4c
	ld	a,04ch
	call	prep
	ld	hl,k_4c	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,04ch	; under test
	jp	fall
k_4c:

; ---- ED 54
p_54:	ld	a,(startop)
	cp	055h
	jp	nc,k_54
	ld	a,054h
	call	prep
	ld	hl,k_54	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,054h	; under test
	jp	fall
k_54:

; ---- ED 55
p_55:	ld	a,(startop)
	cp	056h
	jp	nc,k_55
	ld	a,055h
	call	prep
	ld	hl,k_55	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,055h	; under test
	jp	fall
k_55:

; ---- ED 5C
p_5c:	ld	a,(startop)
	cp	05dh
	jp	nc,k_5c
	ld	a,05ch
	call	prep
	ld	hl,k_5c	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,05ch	; under test
	jp	fall
k_5c:

; ---- ED 5D
p_5d:	ld	a,(startop)
	cp	05eh
	jp	nc,k_5d
	ld	a,05dh
	call	prep
	ld	hl,k_5d	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,05dh	; under test
	jp	fall
k_5d:

; ---- ED 64
p_64:	ld	a,(startop)
	cp	065h
	jp	nc,k_64
	ld	a,064h
	call	prep
	ld	hl,k_64	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,064h	; under test
	jp	fall
k_64:

; ---- ED 65
p_65:	ld	a,(startop)
	cp	066h
	jp	nc,k_65
	ld	a,065h
	call	prep
	ld	hl,k_65	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,065h	; under test
	jp	fall
k_65:

; ---- ED 6C
p_6c:	ld	a,(startop)
	cp	06dh
	jp	nc,k_6c
	ld	a,06ch
	call	prep
	ld	hl,k_6c	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,06ch	; under test
	jp	fall
k_6c:

; ---- ED 6D
p_6d:	ld	a,(startop)
	cp	06eh
	jp	nc,k_6d
	ld	a,06dh
	call	prep
	ld	hl,k_6d	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,06dh	; under test
	jp	fall
k_6d:

; ---- ED 74
p_74:	ld	a,(startop)
	cp	075h
	jp	nc,k_74
	ld	a,074h
	call	prep
	ld	hl,k_74	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,074h	; under test
	jp	fall
k_74:

; ---- ED 75
p_75:	ld	a,(startop)
	cp	076h
	jp	nc,k_75
	ld	a,075h
	call	prep
	ld	hl,k_75	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,075h	; under test
	jp	fall
k_75:

; ---- ED 77
p_77:	ld	a,(startop)
	cp	078h
	jp	nc,k_77
	ld	a,077h
	call	prep
	ld	hl,k_77	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,077h	; under test
	jp	fall
k_77:

; ---- ED 7C
p_7c:	ld	a,(startop)
	cp	07dh
	jp	nc,k_7c
	ld	a,07ch
	call	prep
	ld	hl,k_7c	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,07ch	; under test
	jp	fall
k_7c:

; ---- ED 7D
p_7d:	ld	a,(startop)
	cp	07eh
	jp	nc,k_7d
	ld	a,07dh
	call	prep
	ld	hl,k_7d	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,07dh	; under test
	jp	fall
k_7d:

; ---- ED 7F
p_7f:	ld	a,(startop)
	cp	080h
	jp	nc,k_7f
	ld	a,07fh
	call	prep
	ld	hl,k_7f	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,07fh	; under test
	jp	fall
k_7f:

; ---- ED 80
p_80:	ld	a,(startop)
	cp	081h
	jp	nc,k_80
	ld	a,080h
	call	prep
	ld	hl,k_80	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,080h	; under test
	jp	fall
k_80:

; ---- ED 81
p_81:	ld	a,(startop)
	cp	082h
	jp	nc,k_81
	ld	a,081h
	call	prep
	ld	hl,k_81	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,081h	; under test
	jp	fall
k_81:

; ---- ED 82
p_82:	ld	a,(startop)
	cp	083h
	jp	nc,k_82
	ld	a,082h
	call	prep
	ld	hl,k_82	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,082h	; under test
	jp	fall
k_82:

; ---- ED 83
p_83:	ld	a,(startop)
	cp	084h
	jp	nc,k_83
	ld	a,083h
	call	prep
	ld	hl,k_83	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,083h	; under test
	jp	fall
k_83:

; ---- ED 84
p_84:	ld	a,(startop)
	cp	085h
	jp	nc,k_84
	ld	a,084h
	call	prep
	ld	hl,k_84	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,084h	; under test
	jp	fall
k_84:

; ---- ED 85
p_85:	ld	a,(startop)
	cp	086h
	jp	nc,k_85
	ld	a,085h
	call	prep
	ld	hl,k_85	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,085h	; under test
	jp	fall
k_85:

; ---- ED 86
p_86:	ld	a,(startop)
	cp	087h
	jp	nc,k_86
	ld	a,086h
	call	prep
	ld	hl,k_86	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,086h	; under test
	jp	fall
k_86:

; ---- ED 87
p_87:	ld	a,(startop)
	cp	088h
	jp	nc,k_87
	ld	a,087h
	call	prep
	ld	hl,k_87	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,087h	; under test
	jp	fall
k_87:

; ---- ED 88
p_88:	ld	a,(startop)
	cp	089h
	jp	nc,k_88
	ld	a,088h
	call	prep
	ld	hl,k_88	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,088h	; under test
	jp	fall
k_88:

; ---- ED 89
p_89:	ld	a,(startop)
	cp	08ah
	jp	nc,k_89
	ld	a,089h
	call	prep
	ld	hl,k_89	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,089h	; under test
	jp	fall
k_89:

; ---- ED 8A
p_8a:	ld	a,(startop)
	cp	08bh
	jp	nc,k_8a
	ld	a,08ah
	call	prep
	ld	hl,k_8a	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,08ah	; under test
	jp	fall
k_8a:

; ---- ED 8B
p_8b:	ld	a,(startop)
	cp	08ch
	jp	nc,k_8b
	ld	a,08bh
	call	prep
	ld	hl,k_8b	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,08bh	; under test
	jp	fall
k_8b:

; ---- ED 8C
p_8c:	ld	a,(startop)
	cp	08dh
	jp	nc,k_8c
	ld	a,08ch
	call	prep
	ld	hl,k_8c	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,08ch	; under test
	jp	fall
k_8c:

; ---- ED 8D
p_8d:	ld	a,(startop)
	cp	08eh
	jp	nc,k_8d
	ld	a,08dh
	call	prep
	ld	hl,k_8d	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,08dh	; under test
	jp	fall
k_8d:

; ---- ED 8E
p_8e:	ld	a,(startop)
	cp	08fh
	jp	nc,k_8e
	ld	a,08eh
	call	prep
	ld	hl,k_8e	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,08eh	; under test
	jp	fall
k_8e:

; ---- ED 8F
p_8f:	ld	a,(startop)
	cp	090h
	jp	nc,k_8f
	ld	a,08fh
	call	prep
	ld	hl,k_8f	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,08fh	; under test
	jp	fall
k_8f:

; ---- ED 90
p_90:	ld	a,(startop)
	cp	091h
	jp	nc,k_90
	ld	a,090h
	call	prep
	ld	hl,k_90	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,090h	; under test
	jp	fall
k_90:

; ---- ED 91
p_91:	ld	a,(startop)
	cp	092h
	jp	nc,k_91
	ld	a,091h
	call	prep
	ld	hl,k_91	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,091h	; under test
	jp	fall
k_91:

; ---- ED 92
p_92:	ld	a,(startop)
	cp	093h
	jp	nc,k_92
	ld	a,092h
	call	prep
	ld	hl,k_92	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,092h	; under test
	jp	fall
k_92:

; ---- ED 93
p_93:	ld	a,(startop)
	cp	094h
	jp	nc,k_93
	ld	a,093h
	call	prep
	ld	hl,k_93	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,093h	; under test
	jp	fall
k_93:

; ---- ED 94
p_94:	ld	a,(startop)
	cp	095h
	jp	nc,k_94
	ld	a,094h
	call	prep
	ld	hl,k_94	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,094h	; under test
	jp	fall
k_94:

; ---- ED 95
p_95:	ld	a,(startop)
	cp	096h
	jp	nc,k_95
	ld	a,095h
	call	prep
	ld	hl,k_95	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,095h	; under test
	jp	fall
k_95:

; ---- ED 96
p_96:	ld	a,(startop)
	cp	097h
	jp	nc,k_96
	ld	a,096h
	call	prep
	ld	hl,k_96	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,096h	; under test
	jp	fall
k_96:

; ---- ED 97
p_97:	ld	a,(startop)
	cp	098h
	jp	nc,k_97
	ld	a,097h
	call	prep
	ld	hl,k_97	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,097h	; under test
	jp	fall
k_97:

; ---- ED 98
p_98:	ld	a,(startop)
	cp	099h
	jp	nc,k_98
	ld	a,098h
	call	prep
	ld	hl,k_98	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,098h	; under test
	jp	fall
k_98:

; ---- ED 99
p_99:	ld	a,(startop)
	cp	09ah
	jp	nc,k_99
	ld	a,099h
	call	prep
	ld	hl,k_99	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,099h	; under test
	jp	fall
k_99:

; ---- ED 9A
p_9a:	ld	a,(startop)
	cp	09bh
	jp	nc,k_9a
	ld	a,09ah
	call	prep
	ld	hl,k_9a	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,09ah	; under test
	jp	fall
k_9a:

; ---- ED 9B
p_9b:	ld	a,(startop)
	cp	09ch
	jp	nc,k_9b
	ld	a,09bh
	call	prep
	ld	hl,k_9b	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,09bh	; under test
	jp	fall
k_9b:

; ---- ED 9C
p_9c:	ld	a,(startop)
	cp	09dh
	jp	nc,k_9c
	ld	a,09ch
	call	prep
	ld	hl,k_9c	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,09ch	; under test
	jp	fall
k_9c:

; ---- ED 9D
p_9d:	ld	a,(startop)
	cp	09eh
	jp	nc,k_9d
	ld	a,09dh
	call	prep
	ld	hl,k_9d	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,09dh	; under test
	jp	fall
k_9d:

; ---- ED 9E
p_9e:	ld	a,(startop)
	cp	09fh
	jp	nc,k_9e
	ld	a,09eh
	call	prep
	ld	hl,k_9e	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,09eh	; under test
	jp	fall
k_9e:

; ---- ED 9F
p_9f:	ld	a,(startop)
	cp	0a0h
	jp	nc,k_9f
	ld	a,09fh
	call	prep
	ld	hl,k_9f	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,09fh	; under test
	jp	fall
k_9f:

; ---- ED A4
p_a4:	ld	a,(startop)
	cp	0a5h
	jp	nc,k_a4
	ld	a,0a4h
	call	prep
	ld	hl,k_a4	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0a4h	; under test
	jp	fall
k_a4:

; ---- ED A5
p_a5:	ld	a,(startop)
	cp	0a6h
	jp	nc,k_a5
	ld	a,0a5h
	call	prep
	ld	hl,k_a5	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0a5h	; under test
	jp	fall
k_a5:

; ---- ED A6
p_a6:	ld	a,(startop)
	cp	0a7h
	jp	nc,k_a6
	ld	a,0a6h
	call	prep
	ld	hl,k_a6	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0a6h	; under test
	jp	fall
k_a6:

; ---- ED A7
p_a7:	ld	a,(startop)
	cp	0a8h
	jp	nc,k_a7
	ld	a,0a7h
	call	prep
	ld	hl,k_a7	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0a7h	; under test
	jp	fall
k_a7:

; ---- ED AC
p_ac:	ld	a,(startop)
	cp	0adh
	jp	nc,k_ac
	ld	a,0ach
	call	prep
	ld	hl,k_ac	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0ach	; under test
	jp	fall
k_ac:

; ---- ED AD
p_ad:	ld	a,(startop)
	cp	0aeh
	jp	nc,k_ad
	ld	a,0adh
	call	prep
	ld	hl,k_ad	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0adh	; under test
	jp	fall
k_ad:

; ---- ED AE
p_ae:	ld	a,(startop)
	cp	0afh
	jp	nc,k_ae
	ld	a,0aeh
	call	prep
	ld	hl,k_ae	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0aeh	; under test
	jp	fall
k_ae:

; ---- ED AF
p_af:	ld	a,(startop)
	cp	0b0h
	jp	nc,k_af
	ld	a,0afh
	call	prep
	ld	hl,k_af	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0afh	; under test
	jp	fall
k_af:

; ---- ED B4
p_b4:	ld	a,(startop)
	cp	0b5h
	jp	nc,k_b4
	ld	a,0b4h
	call	prep
	ld	hl,k_b4	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0b4h	; under test
	jp	fall
k_b4:

; ---- ED B5
p_b5:	ld	a,(startop)
	cp	0b6h
	jp	nc,k_b5
	ld	a,0b5h
	call	prep
	ld	hl,k_b5	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0b5h	; under test
	jp	fall
k_b5:

; ---- ED B6
p_b6:	ld	a,(startop)
	cp	0b7h
	jp	nc,k_b6
	ld	a,0b6h
	call	prep
	ld	hl,k_b6	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0b6h	; under test
	jp	fall
k_b6:

; ---- ED B7
p_b7:	ld	a,(startop)
	cp	0b8h
	jp	nc,k_b7
	ld	a,0b7h
	call	prep
	ld	hl,k_b7	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0b7h	; under test
	jp	fall
k_b7:

; ---- ED BC
p_bc:	ld	a,(startop)
	cp	0bdh
	jp	nc,k_bc
	ld	a,0bch
	call	prep
	ld	hl,k_bc	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0bch	; under test
	jp	fall
k_bc:

; ---- ED BD
p_bd:	ld	a,(startop)
	cp	0beh
	jp	nc,k_bd
	ld	a,0bdh
	call	prep
	ld	hl,k_bd	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0bdh	; under test
	jp	fall
k_bd:

; ---- ED BE
p_be:	ld	a,(startop)
	cp	0bfh
	jp	nc,k_be
	ld	a,0beh
	call	prep
	ld	hl,k_be	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0beh	; under test
	jp	fall
k_be:

; ---- ED BF
p_bf:	ld	a,(startop)
	cp	0c0h
	jp	nc,k_bf
	ld	a,0bfh
	call	prep
	ld	hl,k_bf	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0bfh	; under test
	jp	fall
k_bf:

; ---- ED C0
p_c0:	ld	a,(startop)
	cp	0c1h
	jp	nc,k_c0
	ld	a,0c0h
	call	prep
	ld	hl,k_c0	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0c0h	; under test
	jp	fall
k_c0:

; ---- ED C1
p_c1:	ld	a,(startop)
	cp	0c2h
	jp	nc,k_c1
	ld	a,0c1h
	call	prep
	ld	hl,k_c1	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0c1h	; under test
	jp	fall
k_c1:

; ---- ED C2
p_c2:	ld	a,(startop)
	cp	0c3h
	jp	nc,k_c2
	ld	a,0c2h
	call	prep
	ld	hl,k_c2	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0c2h	; under test
	jp	fall
k_c2:

; ---- ED C3
p_c3:	ld	a,(startop)
	cp	0c4h
	jp	nc,k_c3
	ld	a,0c3h
	call	prep
	ld	hl,k_c3	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0c3h	; under test
	jp	fall
k_c3:

; ---- ED C4
p_c4:	ld	a,(startop)
	cp	0c5h
	jp	nc,k_c4
	ld	a,0c4h
	call	prep
	ld	hl,k_c4	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0c4h	; under test
	jp	fall
k_c4:

; ---- ED C5
p_c5:	ld	a,(startop)
	cp	0c6h
	jp	nc,k_c5
	ld	a,0c5h
	call	prep
	ld	hl,k_c5	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0c5h	; under test
	jp	fall
k_c5:

; ---- ED C6
p_c6:	ld	a,(startop)
	cp	0c7h
	jp	nc,k_c6
	ld	a,0c6h
	call	prep
	ld	hl,k_c6	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0c6h	; under test
	jp	fall
k_c6:

; ---- ED C7
p_c7:	ld	a,(startop)
	cp	0c8h
	jp	nc,k_c7
	ld	a,0c7h
	call	prep
	ld	hl,k_c7	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0c7h	; under test
	jp	fall
k_c7:

; ---- ED C8
p_c8:	ld	a,(startop)
	cp	0c9h
	jp	nc,k_c8
	ld	a,0c8h
	call	prep
	ld	hl,k_c8	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0c8h	; under test
	jp	fall
k_c8:

; ---- ED C9
p_c9:	ld	a,(startop)
	cp	0cah
	jp	nc,k_c9
	ld	a,0c9h
	call	prep
	ld	hl,k_c9	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0c9h	; under test
	jp	fall
k_c9:

; ---- ED CA
p_ca:	ld	a,(startop)
	cp	0cbh
	jp	nc,k_ca
	ld	a,0cah
	call	prep
	ld	hl,k_ca	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0cah	; under test
	jp	fall
k_ca:

; ---- ED CB
p_cb:	ld	a,(startop)
	cp	0cch
	jp	nc,k_cb
	ld	a,0cbh
	call	prep
	ld	hl,k_cb	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0cbh	; under test
	jp	fall
k_cb:

; ---- ED CC
p_cc:	ld	a,(startop)
	cp	0cdh
	jp	nc,k_cc
	ld	a,0cch
	call	prep
	ld	hl,k_cc	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0cch	; under test
	jp	fall
k_cc:

; ---- ED CD
p_cd:	ld	a,(startop)
	cp	0ceh
	jp	nc,k_cd
	ld	a,0cdh
	call	prep
	ld	hl,k_cd	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0cdh	; under test
	jp	fall
k_cd:

; ---- ED CE
p_ce:	ld	a,(startop)
	cp	0cfh
	jp	nc,k_ce
	ld	a,0ceh
	call	prep
	ld	hl,k_ce	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0ceh	; under test
	jp	fall
k_ce:

; ---- ED CF
p_cf:	ld	a,(startop)
	cp	0d0h
	jp	nc,k_cf
	ld	a,0cfh
	call	prep
	ld	hl,k_cf	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0cfh	; under test
	jp	fall
k_cf:

; ---- ED D0
p_d0:	ld	a,(startop)
	cp	0d1h
	jp	nc,k_d0
	ld	a,0d0h
	call	prep
	ld	hl,k_d0	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0d0h	; under test
	jp	fall
k_d0:

; ---- ED D1
p_d1:	ld	a,(startop)
	cp	0d2h
	jp	nc,k_d1
	ld	a,0d1h
	call	prep
	ld	hl,k_d1	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0d1h	; under test
	jp	fall
k_d1:

; ---- ED D2
p_d2:	ld	a,(startop)
	cp	0d3h
	jp	nc,k_d2
	ld	a,0d2h
	call	prep
	ld	hl,k_d2	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0d2h	; under test
	jp	fall
k_d2:

; ---- ED D3
p_d3:	ld	a,(startop)
	cp	0d4h
	jp	nc,k_d3
	ld	a,0d3h
	call	prep
	ld	hl,k_d3	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0d3h	; under test
	jp	fall
k_d3:

; ---- ED D4
p_d4:	ld	a,(startop)
	cp	0d5h
	jp	nc,k_d4
	ld	a,0d4h
	call	prep
	ld	hl,k_d4	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0d4h	; under test
	jp	fall
k_d4:

; ---- ED D5
p_d5:	ld	a,(startop)
	cp	0d6h
	jp	nc,k_d5
	ld	a,0d5h
	call	prep
	ld	hl,k_d5	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0d5h	; under test
	jp	fall
k_d5:

; ---- ED D6
p_d6:	ld	a,(startop)
	cp	0d7h
	jp	nc,k_d6
	ld	a,0d6h
	call	prep
	ld	hl,k_d6	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0d6h	; under test
	jp	fall
k_d6:

; ---- ED D7
p_d7:	ld	a,(startop)
	cp	0d8h
	jp	nc,k_d7
	ld	a,0d7h
	call	prep
	ld	hl,k_d7	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0d7h	; under test
	jp	fall
k_d7:

; ---- ED D8
p_d8:	ld	a,(startop)
	cp	0d9h
	jp	nc,k_d8
	ld	a,0d8h
	call	prep
	ld	hl,k_d8	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0d8h	; under test
	jp	fall
k_d8:

; ---- ED D9
p_d9:	ld	a,(startop)
	cp	0dah
	jp	nc,k_d9
	ld	a,0d9h
	call	prep
	ld	hl,k_d9	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0d9h	; under test
	jp	fall
k_d9:

; ---- ED DA
p_da:	ld	a,(startop)
	cp	0dbh
	jp	nc,k_da
	ld	a,0dah
	call	prep
	ld	hl,k_da	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0dah	; under test
	jp	fall
k_da:

; ---- ED DB
p_db:	ld	a,(startop)
	cp	0dch
	jp	nc,k_db
	ld	a,0dbh
	call	prep
	ld	hl,k_db	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0dbh	; under test
	jp	fall
k_db:

; ---- ED DC
p_dc:	ld	a,(startop)
	cp	0ddh
	jp	nc,k_dc
	ld	a,0dch
	call	prep
	ld	hl,k_dc	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0dch	; under test
	jp	fall
k_dc:

; ---- ED DD
p_dd:	ld	a,(startop)
	cp	0deh
	jp	nc,k_dd
	ld	a,0ddh
	call	prep
	ld	hl,k_dd	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0ddh	; under test
	jp	fall
k_dd:

; ---- ED DE
p_de:	ld	a,(startop)
	cp	0dfh
	jp	nc,k_de
	ld	a,0deh
	call	prep
	ld	hl,k_de	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0deh	; under test
	jp	fall
k_de:

; ---- ED DF
p_df:	ld	a,(startop)
	cp	0e0h
	jp	nc,k_df
	ld	a,0dfh
	call	prep
	ld	hl,k_df	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0dfh	; under test
	jp	fall
k_df:

; ---- ED E0
p_e0:	ld	a,(startop)
	cp	0e1h
	jp	nc,k_e0
	ld	a,0e0h
	call	prep
	ld	hl,k_e0	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0e0h	; under test
	jp	fall
k_e0:

; ---- ED E1
p_e1:	ld	a,(startop)
	cp	0e2h
	jp	nc,k_e1
	ld	a,0e1h
	call	prep
	ld	hl,k_e1	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0e1h	; under test
	jp	fall
k_e1:

; ---- ED E2
p_e2:	ld	a,(startop)
	cp	0e3h
	jp	nc,k_e2
	ld	a,0e2h
	call	prep
	ld	hl,k_e2	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0e2h	; under test
	jp	fall
k_e2:

; ---- ED E3
p_e3:	ld	a,(startop)
	cp	0e4h
	jp	nc,k_e3
	ld	a,0e3h
	call	prep
	ld	hl,k_e3	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0e3h	; under test
	jp	fall
k_e3:

; ---- ED E4
p_e4:	ld	a,(startop)
	cp	0e5h
	jp	nc,k_e4
	ld	a,0e4h
	call	prep
	ld	hl,k_e4	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0e4h	; under test
	jp	fall
k_e4:

; ---- ED E5
p_e5:	ld	a,(startop)
	cp	0e6h
	jp	nc,k_e5
	ld	a,0e5h
	call	prep
	ld	hl,k_e5	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0e5h	; under test
	jp	fall
k_e5:

; ---- ED E6
p_e6:	ld	a,(startop)
	cp	0e7h
	jp	nc,k_e6
	ld	a,0e6h
	call	prep
	ld	hl,k_e6	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0e6h	; under test
	jp	fall
k_e6:

; ---- ED E7
p_e7:	ld	a,(startop)
	cp	0e8h
	jp	nc,k_e7
	ld	a,0e7h
	call	prep
	ld	hl,k_e7	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0e7h	; under test
	jp	fall
k_e7:

; ---- ED E8
p_e8:	ld	a,(startop)
	cp	0e9h
	jp	nc,k_e8
	ld	a,0e8h
	call	prep
	ld	hl,k_e8	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0e8h	; under test
	jp	fall
k_e8:

; ---- ED E9
p_e9:	ld	a,(startop)
	cp	0eah
	jp	nc,k_e9
	ld	a,0e9h
	call	prep
	ld	hl,k_e9	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0e9h	; under test
	jp	fall
k_e9:

; ---- ED EA
p_ea:	ld	a,(startop)
	cp	0ebh
	jp	nc,k_ea
	ld	a,0eah
	call	prep
	ld	hl,k_ea	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0eah	; under test
	jp	fall
k_ea:

; ---- ED EB
p_eb:	ld	a,(startop)
	cp	0ech
	jp	nc,k_eb
	ld	a,0ebh
	call	prep
	ld	hl,k_eb	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0ebh	; under test
	jp	fall
k_eb:

; ---- ED EC
p_ec:	ld	a,(startop)
	cp	0edh
	jp	nc,k_ec
	ld	a,0ech
	call	prep
	ld	hl,k_ec	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0ech	; under test
	jp	fall
k_ec:

; ---- ED EE
p_ee:	ld	a,(startop)
	cp	0efh
	jp	nc,k_ee
	ld	a,0eeh
	call	prep
	ld	hl,k_ee	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0eeh	; under test
	jp	fall
k_ee:

; ---- ED EF
p_ef:	ld	a,(startop)
	cp	0f0h
	jp	nc,k_ef
	ld	a,0efh
	call	prep
	ld	hl,k_ef	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0efh	; under test
	jp	fall
k_ef:

; ---- ED F0
p_f0:	ld	a,(startop)
	cp	0f1h
	jp	nc,k_f0
	ld	a,0f0h
	call	prep
	ld	hl,k_f0	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0f0h	; under test
	jp	fall
k_f0:

; ---- ED F1
p_f1:	ld	a,(startop)
	cp	0f2h
	jp	nc,k_f1
	ld	a,0f1h
	call	prep
	ld	hl,k_f1	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0f1h	; under test
	jp	fall
k_f1:

; ---- ED F2
p_f2:	ld	a,(startop)
	cp	0f3h
	jp	nc,k_f2
	ld	a,0f2h
	call	prep
	ld	hl,k_f2	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0f2h	; under test
	jp	fall
k_f2:

; ---- ED F3
p_f3:	ld	a,(startop)
	cp	0f4h
	jp	nc,k_f3
	ld	a,0f3h
	call	prep
	ld	hl,k_f3	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0f3h	; under test
	jp	fall
k_f3:

; ---- ED F4
p_f4:	ld	a,(startop)
	cp	0f5h
	jp	nc,k_f4
	ld	a,0f4h
	call	prep
	ld	hl,k_f4	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0f4h	; under test
	jp	fall
k_f4:

; ---- ED F5
p_f5:	ld	a,(startop)
	cp	0f6h
	jp	nc,k_f5
	ld	a,0f5h
	call	prep
	ld	hl,k_f5	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0f5h	; under test
	jp	fall
k_f5:

; ---- ED F6
p_f6:	ld	a,(startop)
	cp	0f7h
	jp	nc,k_f6
	ld	a,0f6h
	call	prep
	ld	hl,k_f6	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0f6h	; under test
	jp	fall
k_f6:

; ---- ED F7
p_f7:	ld	a,(startop)
	cp	0f8h
	jp	nc,k_f7
	ld	a,0f7h
	call	prep
	ld	hl,k_f7	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0f7h	; under test
	jp	fall
k_f7:

; ---- ED F8
p_f8:	ld	a,(startop)
	cp	0f9h
	jp	nc,k_f8
	ld	a,0f8h
	call	prep
	ld	hl,k_f8	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0f8h	; under test
	jp	fall
k_f8:

; ---- ED F9
p_f9:	ld	a,(startop)
	cp	0fah
	jp	nc,k_f9
	ld	a,0f9h
	call	prep
	ld	hl,k_f9	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0f9h	; under test
	jp	fall
k_f9:

; ---- ED FA
p_fa:	ld	a,(startop)
	cp	0fbh
	jp	nc,k_fa
	ld	a,0fah
	call	prep
	ld	hl,k_fa	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0fah	; under test
	jp	fall
k_fa:

; ---- ED FB
p_fb:	ld	a,(startop)
	cp	0fch
	jp	nc,k_fb
	ld	a,0fbh
	call	prep
	ld	hl,k_fb	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0fbh	; under test
	jp	fall
k_fb:

; ---- ED FC
p_fc:	ld	a,(startop)
	cp	0fdh
	jp	nc,k_fc
	ld	a,0fch
	call	prep
	ld	hl,k_fc	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0fch	; under test
	jp	fall
k_fc:

; ---- ED FE
p_fe:	ld	a,(startop)
	cp	0ffh
	jp	nc,k_fe
	ld	a,0feh
	call	prep
	ld	hl,k_fe	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0feh	; under test
	jp	fall
k_fe:

; ---- ED FF
p_ff:	ld	a,0ffh
	call	prep
	ld	hl,k_ff	; report returns here
	push	hl
	di
	ld	(mainsp),sp
	ld	sp,tstk
	ld	bc,05ad7h	; A=5Ah F=D7h
	push	bc
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,bufh
	ld	ix,bufx
	ld	iy,bufy
	db	0edh,0ffh	; under test
	jp	fall
k_ff:

	call	fclose
	jp	0

; fall-through exit: capture before anything else changes
fall:	ld	(osp),sp
	ld	(ohl),hl
	ld	sp,capstk
	push	af
	xor	a
	jp	capture
; return exit: the sandbox return address was popped
rtrap:	ld	(osp),sp
	ld	(ohl),hl
	ld	sp,capstk
	push	af
	ld	a,1
capture: ld	(oexit),a
	pop	hl		; AF
	ld	(oaf),hl
	ld	(obc),bc
	ld	(ode),de
	ld	(oix),ix
	ld	(oiy),iy
	ld	sp,(mainsp)
	ei
	jp	report

; prepare one probe: a = opcode. Print "ED xx " on the console, reset the
; sandbox memory and the sandbox stack.
prep:	ld	(curop),a
	ld	de,s_ed
	call	conmsg
	ld	a,(curop)
	call	conhex
	ld	e,' '
	ld	c,2
	call	bdosv
	ld	hl,pattern
	ld	de,bufh
	ld	bc,4
	ldir
	ld	hl,pattern
	ld	de,bufx
	ld	bc,4
	ldir
	ld	hl,pattern
	ld	de,bufy
	ld	bc,4
	ldir
	ld	hl,stkpat
	ld	de,tstk-4
	ld	bc,8
	ldir
	ret

; write the result line: "ED xx " to the file only (the console already has
; it), the rest to both; pad the file record to 126 bytes, then CR LF, and
; close the file as a checkpoint
report:	xor	a
	ld	(lcnt),a
	ld	a,'E'
	call	fonly
	ld	a,'D'
	call	fonly
	ld	a,' '
	call	fonly
	ld	a,(curop)
	call	fhex2
	ld	a,' '
	call	fonly
	ld	de,s_next
	ld	a,(oexit)
	or	a
	jp	z,rep1
	ld	de,s_ret
rep1:	call	lputs
	ld	de,s_a
	call	lputs
	ld	a,(oaf+1)
	call	lhex2
	ld	de,s_f
	call	lputs
	ld	a,(oaf)
	call	lhex2
	ld	de,s_bc
	call	lputs
	ld	hl,(obc)
	call	lhex4
	ld	de,s_de
	call	lputs
	ld	hl,(ode)
	call	lhex4
	ld	de,s_hl
	call	lputs
	ld	hl,(ohl)
	call	lhex4
	ld	de,s_ix
	call	lputs
	ld	hl,(oix)
	call	lhex4
	ld	de,s_iy
	call	lputs
	ld	hl,(oiy)
	call	lhex4
	ld	de,s_sp
	call	lputs
	ld	hl,(osp)
	call	lhex4
	ld	de,s_m
	call	lputs
	ld	hl,bufh
	ld	b,4
	call	lbytes
	ld	de,s_x
	call	lputs
	ld	hl,bufx
	ld	b,4
	call	lbytes
	ld	de,s_y
	call	lputs
	ld	hl,bufy
	ld	b,4
	call	lbytes
	ld	de,s_s
	call	lputs
	ld	hl,tstk-4
	ld	b,8
	call	lbytes
	ld	e,13
	ld	c,2
	call	bdosv
	ld	e,10
	ld	c,2
	call	bdosv
rpad:	ld	a,(lcnt)
	cp	126
	jp	z,rend
	ld	a,' '
	call	fonly
	jp	rpad
rend:	ld	a,13
	call	fonly
	ld	a,10
	call	fonly		; completes the 128-byte record
	ld	de,(curfcb)
	ld	c,16		; close as a checkpoint; writing continues
	call	bdosv
	ret

; file only, counted
fonly:	push	hl
	call	fputc
	ld	hl,lcnt
	inc	(hl)
	pop	hl
	ret

; console and file, counted
lput:	call	putc
	push	hl
	ld	hl,lcnt
	inc	(hl)
	pop	hl
	ret

lputs:	ld	a,(de)
	cp	'$'
	ret	z
	call	lput
	inc	de
	jp	lputs

; a as two hex digits: fhex2 to the file only, lhex2 to both
fhex2:	push	af
	rrca
	rrca
	rrca
	rrca
	call	hexch
	call	fonly
	pop	af
	call	hexch
	jp	fonly
lhex2:	push	af
	rrca
	rrca
	rrca
	rrca
	call	hexch
	call	lput
	pop	af
	call	hexch
	jp	lput
lhex4:	ld	a,h
	call	lhex2
	ld	a,l
	jp	lhex2
; b bytes at hl as hex
lbytes:	ld	a,(hl)
	call	lhex2
	inc	hl
	dec	b
	jp	nz,lbytes
	ret

; low nibble of a -> ASCII hex digit in a (table, no compare-and-adjust)
hexch:	push	de
	push	hl
	and	0fh
	ld	e,a
	ld	d,0
	ld	hl,hextab
	add	hl,de
	ld	a,(hl)
	pop	hl
	pop	de
	ret

; console only: a as two hex digits
conhex:	push	af
	rrca
	rrca
	rrca
	rrca
	call	hexch
	ld	e,a
	ld	c,2
	call	bdosv
	pop	af
	call	hexch
	ld	e,a
	ld	c,2
	call	bdosv
	ret

; parse the optional hex start opcode from the command tail and name the
; output file EDxx.TXT
parse:	xor	a
	ld	(startop),a
	ld	hl,80h
	ld	b,(hl)		; tail length
	inc	hl
	inc	b
pskip:	dec	b
	jp	z,pname
	ld	a,(hl)
	inc	hl
	cp	' '
	jp	z,pskip
	dec	hl
	inc	b
	ld	c,0		; value
	ld	d,2		; at most two digits
pdig:	dec	b
	jp	z,pdone
	ld	a,(hl)
	inc	hl
	cp	'0'
	jp	c,pdone
	cp	'9'+1
	jp	c,pnum
	cp	'A'
	jp	c,pdone
	cp	'F'+1
	jp	nc,pdone
	sub	'A'-10
	jp	pacc
pnum:	sub	'0'
pacc:	ld	e,a
	ld	a,c
	rlca
	rlca
	rlca
	rlca
	and	0f0h
	or	e
	ld	c,a
	dec	d
	jp	nz,pdig
pdone:	ld	a,c
	ld	(startop),a
pname:	ld	a,(startop)
	push	af
	rrca
	rrca
	rrca
	rrca
	call	hexch
	ld	(fcbout+3),a
	pop	af
	call	hexch
	ld	(fcbout+4),a
	ret

title:	db	'EDPRB M102 undefined ED opcode probe',13,10,'$'
s_ed:	db	'ED $'
s_next:	db	'NEXT$'
s_ret:	db	'RET $'
s_a:	db	' A=$'
s_f:	db	' F=$'
s_bc:	db	' BC=$'
s_de:	db	' DE=$'
s_hl:	db	' HL=$'
s_ix:	db	' IX=$'
s_iy:	db	' IY=$'
s_sp:	db	' SP=$'
s_m:	db	' M=$'
s_x:	db	' X=$'
s_y:	db	' Y=$'
s_s:	db	' S=$'
pattern: db	11h,22h,33h,44h
stkpat:	db	0a1h,0a2h,0a3h,0a4h
	dw	rtrap
	db	0a7h,0a8h
fcbout:	db	0,'ED00    TXT'
	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
startop: db	0
curop:	db	0
oexit:	db	0
lcnt:	db	0
mainsp:	dw	0
osp:	dw	0
oaf:	dw	0
obc:	dw	0
ode:	dw	0
ohl:	dw	0
oix:	dw	0
oiy:	dw	0
bufh:	ds	4
bufx:	ds	4
bufy:	ds	4
	ds	32
tstk:	ds	32		; sandbox stack: tstk-4..tstk+3 is recorded
	ds	16
capstk:
	ds	128
stack:
