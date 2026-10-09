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

; M102 CBPRB: one-by-one probe of every DD CB d xx and FD CB d xx opcode
; (d = 02h) in the uPD9002 Z80 emulation mode, with two input sets:
;   set 0: A=0Ah F=00h BC=0B0Ch DE=0D0Eh HL=4455h (IX+2)=(IY+2)=81h
;   set 1: A=0Ah F=D7h BC=0B0Ch DE=0D0Eh HL=4455h (IX+2)=(IY+2)=7Eh
; IX and IY both point 2 bytes below the probed byte. Writes CBPRB.TXT, one
; line per opcode and set:
;   pp CB 02 xx s A=xx F=xx BC=xxxx DE=xxxx HL=xxxx IX=xxxx IY=xxxx M=xx
; The console shows progress every 16 opcodes.
;
; No self-modifying code: every opcode has its own code path. F is set only
; through POP AF, and PUSH AF is the first instruction after the opcode.

	org	100h

start:	ld	sp,stack
	ld	de,title
	call	conmsg
	ld	de,fcbout
	call	fopen

; ---- DD CB 02 00
c_dd00:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,000h
	ld	(curop),a
	ld	a,0ddh
	call	progress
	xor	a
	ld	(curset),a
l_dd00:	di
	call	ldregs
	db	0ddh,0cbh,02h,000h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd00

; ---- DD CB 02 01
c_dd01:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,001h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd01:	di
	call	ldregs
	db	0ddh,0cbh,02h,001h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd01

; ---- DD CB 02 02
c_dd02:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,002h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd02:	di
	call	ldregs
	db	0ddh,0cbh,02h,002h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd02

; ---- DD CB 02 03
c_dd03:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,003h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd03:	di
	call	ldregs
	db	0ddh,0cbh,02h,003h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd03

; ---- DD CB 02 04
c_dd04:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,004h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd04:	di
	call	ldregs
	db	0ddh,0cbh,02h,004h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd04

; ---- DD CB 02 05
c_dd05:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,005h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd05:	di
	call	ldregs
	db	0ddh,0cbh,02h,005h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd05

; ---- DD CB 02 06
c_dd06:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,006h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd06:	di
	call	ldregs
	db	0ddh,0cbh,02h,006h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd06

; ---- DD CB 02 07
c_dd07:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,007h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd07:	di
	call	ldregs
	db	0ddh,0cbh,02h,007h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd07

; ---- DD CB 02 08
c_dd08:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,008h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd08:	di
	call	ldregs
	db	0ddh,0cbh,02h,008h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd08

; ---- DD CB 02 09
c_dd09:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,009h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd09:	di
	call	ldregs
	db	0ddh,0cbh,02h,009h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd09

; ---- DD CB 02 0A
c_dd0a:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,00ah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd0a:	di
	call	ldregs
	db	0ddh,0cbh,02h,00ah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd0a

; ---- DD CB 02 0B
c_dd0b:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,00bh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd0b:	di
	call	ldregs
	db	0ddh,0cbh,02h,00bh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd0b

; ---- DD CB 02 0C
c_dd0c:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,00ch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd0c:	di
	call	ldregs
	db	0ddh,0cbh,02h,00ch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd0c

; ---- DD CB 02 0D
c_dd0d:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,00dh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd0d:	di
	call	ldregs
	db	0ddh,0cbh,02h,00dh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd0d

; ---- DD CB 02 0E
c_dd0e:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,00eh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd0e:	di
	call	ldregs
	db	0ddh,0cbh,02h,00eh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd0e

; ---- DD CB 02 0F
c_dd0f:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,00fh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd0f:	di
	call	ldregs
	db	0ddh,0cbh,02h,00fh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd0f

; ---- DD CB 02 10
c_dd10:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,010h
	ld	(curop),a
	ld	a,0ddh
	call	progress
	xor	a
	ld	(curset),a
l_dd10:	di
	call	ldregs
	db	0ddh,0cbh,02h,010h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd10

; ---- DD CB 02 11
c_dd11:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,011h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd11:	di
	call	ldregs
	db	0ddh,0cbh,02h,011h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd11

; ---- DD CB 02 12
c_dd12:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,012h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd12:	di
	call	ldregs
	db	0ddh,0cbh,02h,012h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd12

; ---- DD CB 02 13
c_dd13:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,013h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd13:	di
	call	ldregs
	db	0ddh,0cbh,02h,013h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd13

; ---- DD CB 02 14
c_dd14:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,014h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd14:	di
	call	ldregs
	db	0ddh,0cbh,02h,014h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd14

; ---- DD CB 02 15
c_dd15:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,015h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd15:	di
	call	ldregs
	db	0ddh,0cbh,02h,015h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd15

; ---- DD CB 02 16
c_dd16:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,016h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd16:	di
	call	ldregs
	db	0ddh,0cbh,02h,016h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd16

; ---- DD CB 02 17
c_dd17:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,017h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd17:	di
	call	ldregs
	db	0ddh,0cbh,02h,017h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd17

; ---- DD CB 02 18
c_dd18:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,018h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd18:	di
	call	ldregs
	db	0ddh,0cbh,02h,018h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd18

; ---- DD CB 02 19
c_dd19:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,019h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd19:	di
	call	ldregs
	db	0ddh,0cbh,02h,019h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd19

; ---- DD CB 02 1A
c_dd1a:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,01ah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd1a:	di
	call	ldregs
	db	0ddh,0cbh,02h,01ah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd1a

; ---- DD CB 02 1B
c_dd1b:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,01bh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd1b:	di
	call	ldregs
	db	0ddh,0cbh,02h,01bh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd1b

; ---- DD CB 02 1C
c_dd1c:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,01ch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd1c:	di
	call	ldregs
	db	0ddh,0cbh,02h,01ch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd1c

; ---- DD CB 02 1D
c_dd1d:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,01dh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd1d:	di
	call	ldregs
	db	0ddh,0cbh,02h,01dh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd1d

; ---- DD CB 02 1E
c_dd1e:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,01eh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd1e:	di
	call	ldregs
	db	0ddh,0cbh,02h,01eh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd1e

; ---- DD CB 02 1F
c_dd1f:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,01fh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd1f:	di
	call	ldregs
	db	0ddh,0cbh,02h,01fh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd1f

; ---- DD CB 02 20
c_dd20:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,020h
	ld	(curop),a
	ld	a,0ddh
	call	progress
	xor	a
	ld	(curset),a
l_dd20:	di
	call	ldregs
	db	0ddh,0cbh,02h,020h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd20

; ---- DD CB 02 21
c_dd21:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,021h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd21:	di
	call	ldregs
	db	0ddh,0cbh,02h,021h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd21

; ---- DD CB 02 22
c_dd22:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,022h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd22:	di
	call	ldregs
	db	0ddh,0cbh,02h,022h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd22

; ---- DD CB 02 23
c_dd23:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,023h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd23:	di
	call	ldregs
	db	0ddh,0cbh,02h,023h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd23

; ---- DD CB 02 24
c_dd24:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,024h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd24:	di
	call	ldregs
	db	0ddh,0cbh,02h,024h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd24

; ---- DD CB 02 25
c_dd25:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,025h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd25:	di
	call	ldregs
	db	0ddh,0cbh,02h,025h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd25

; ---- DD CB 02 26
c_dd26:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,026h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd26:	di
	call	ldregs
	db	0ddh,0cbh,02h,026h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd26

; ---- DD CB 02 27
c_dd27:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,027h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd27:	di
	call	ldregs
	db	0ddh,0cbh,02h,027h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd27

; ---- DD CB 02 28
c_dd28:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,028h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd28:	di
	call	ldregs
	db	0ddh,0cbh,02h,028h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd28

; ---- DD CB 02 29
c_dd29:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,029h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd29:	di
	call	ldregs
	db	0ddh,0cbh,02h,029h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd29

; ---- DD CB 02 2A
c_dd2a:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,02ah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd2a:	di
	call	ldregs
	db	0ddh,0cbh,02h,02ah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd2a

; ---- DD CB 02 2B
c_dd2b:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,02bh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd2b:	di
	call	ldregs
	db	0ddh,0cbh,02h,02bh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd2b

; ---- DD CB 02 2C
c_dd2c:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,02ch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd2c:	di
	call	ldregs
	db	0ddh,0cbh,02h,02ch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd2c

; ---- DD CB 02 2D
c_dd2d:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,02dh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd2d:	di
	call	ldregs
	db	0ddh,0cbh,02h,02dh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd2d

; ---- DD CB 02 2E
c_dd2e:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,02eh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd2e:	di
	call	ldregs
	db	0ddh,0cbh,02h,02eh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd2e

; ---- DD CB 02 2F
c_dd2f:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,02fh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd2f:	di
	call	ldregs
	db	0ddh,0cbh,02h,02fh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd2f

; ---- DD CB 02 30
c_dd30:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,030h
	ld	(curop),a
	ld	a,0ddh
	call	progress
	xor	a
	ld	(curset),a
l_dd30:	di
	call	ldregs
	db	0ddh,0cbh,02h,030h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd30

; ---- DD CB 02 31
c_dd31:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,031h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd31:	di
	call	ldregs
	db	0ddh,0cbh,02h,031h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd31

; ---- DD CB 02 32
c_dd32:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,032h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd32:	di
	call	ldregs
	db	0ddh,0cbh,02h,032h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd32

; ---- DD CB 02 33
c_dd33:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,033h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd33:	di
	call	ldregs
	db	0ddh,0cbh,02h,033h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd33

; ---- DD CB 02 34
c_dd34:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,034h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd34:	di
	call	ldregs
	db	0ddh,0cbh,02h,034h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd34

; ---- DD CB 02 35
c_dd35:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,035h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd35:	di
	call	ldregs
	db	0ddh,0cbh,02h,035h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd35

; ---- DD CB 02 36
c_dd36:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,036h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd36:	di
	call	ldregs
	db	0ddh,0cbh,02h,036h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd36

; ---- DD CB 02 37
c_dd37:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,037h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd37:	di
	call	ldregs
	db	0ddh,0cbh,02h,037h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd37

; ---- DD CB 02 38
c_dd38:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,038h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd38:	di
	call	ldregs
	db	0ddh,0cbh,02h,038h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd38

; ---- DD CB 02 39
c_dd39:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,039h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd39:	di
	call	ldregs
	db	0ddh,0cbh,02h,039h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd39

; ---- DD CB 02 3A
c_dd3a:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,03ah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd3a:	di
	call	ldregs
	db	0ddh,0cbh,02h,03ah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd3a

; ---- DD CB 02 3B
c_dd3b:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,03bh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd3b:	di
	call	ldregs
	db	0ddh,0cbh,02h,03bh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd3b

; ---- DD CB 02 3C
c_dd3c:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,03ch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd3c:	di
	call	ldregs
	db	0ddh,0cbh,02h,03ch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd3c

; ---- DD CB 02 3D
c_dd3d:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,03dh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd3d:	di
	call	ldregs
	db	0ddh,0cbh,02h,03dh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd3d

; ---- DD CB 02 3E
c_dd3e:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,03eh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd3e:	di
	call	ldregs
	db	0ddh,0cbh,02h,03eh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd3e

; ---- DD CB 02 3F
c_dd3f:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,03fh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd3f:	di
	call	ldregs
	db	0ddh,0cbh,02h,03fh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd3f

; ---- DD CB 02 40
c_dd40:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,040h
	ld	(curop),a
	ld	a,0ddh
	call	progress
	xor	a
	ld	(curset),a
l_dd40:	di
	call	ldregs
	db	0ddh,0cbh,02h,040h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd40

; ---- DD CB 02 41
c_dd41:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,041h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd41:	di
	call	ldregs
	db	0ddh,0cbh,02h,041h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd41

; ---- DD CB 02 42
c_dd42:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,042h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd42:	di
	call	ldregs
	db	0ddh,0cbh,02h,042h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd42

; ---- DD CB 02 43
c_dd43:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,043h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd43:	di
	call	ldregs
	db	0ddh,0cbh,02h,043h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd43

; ---- DD CB 02 44
c_dd44:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,044h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd44:	di
	call	ldregs
	db	0ddh,0cbh,02h,044h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd44

; ---- DD CB 02 45
c_dd45:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,045h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd45:	di
	call	ldregs
	db	0ddh,0cbh,02h,045h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd45

; ---- DD CB 02 46
c_dd46:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,046h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd46:	di
	call	ldregs
	db	0ddh,0cbh,02h,046h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd46

; ---- DD CB 02 47
c_dd47:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,047h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd47:	di
	call	ldregs
	db	0ddh,0cbh,02h,047h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd47

; ---- DD CB 02 48
c_dd48:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,048h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd48:	di
	call	ldregs
	db	0ddh,0cbh,02h,048h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd48

; ---- DD CB 02 49
c_dd49:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,049h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd49:	di
	call	ldregs
	db	0ddh,0cbh,02h,049h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd49

; ---- DD CB 02 4A
c_dd4a:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,04ah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd4a:	di
	call	ldregs
	db	0ddh,0cbh,02h,04ah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd4a

; ---- DD CB 02 4B
c_dd4b:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,04bh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd4b:	di
	call	ldregs
	db	0ddh,0cbh,02h,04bh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd4b

; ---- DD CB 02 4C
c_dd4c:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,04ch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd4c:	di
	call	ldregs
	db	0ddh,0cbh,02h,04ch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd4c

; ---- DD CB 02 4D
c_dd4d:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,04dh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd4d:	di
	call	ldregs
	db	0ddh,0cbh,02h,04dh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd4d

; ---- DD CB 02 4E
c_dd4e:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,04eh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd4e:	di
	call	ldregs
	db	0ddh,0cbh,02h,04eh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd4e

; ---- DD CB 02 4F
c_dd4f:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,04fh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd4f:	di
	call	ldregs
	db	0ddh,0cbh,02h,04fh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd4f

; ---- DD CB 02 50
c_dd50:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,050h
	ld	(curop),a
	ld	a,0ddh
	call	progress
	xor	a
	ld	(curset),a
l_dd50:	di
	call	ldregs
	db	0ddh,0cbh,02h,050h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd50

; ---- DD CB 02 51
c_dd51:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,051h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd51:	di
	call	ldregs
	db	0ddh,0cbh,02h,051h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd51

; ---- DD CB 02 52
c_dd52:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,052h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd52:	di
	call	ldregs
	db	0ddh,0cbh,02h,052h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd52

; ---- DD CB 02 53
c_dd53:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,053h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd53:	di
	call	ldregs
	db	0ddh,0cbh,02h,053h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd53

; ---- DD CB 02 54
c_dd54:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,054h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd54:	di
	call	ldregs
	db	0ddh,0cbh,02h,054h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd54

; ---- DD CB 02 55
c_dd55:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,055h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd55:	di
	call	ldregs
	db	0ddh,0cbh,02h,055h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd55

; ---- DD CB 02 56
c_dd56:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,056h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd56:	di
	call	ldregs
	db	0ddh,0cbh,02h,056h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd56

; ---- DD CB 02 57
c_dd57:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,057h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd57:	di
	call	ldregs
	db	0ddh,0cbh,02h,057h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd57

; ---- DD CB 02 58
c_dd58:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,058h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd58:	di
	call	ldregs
	db	0ddh,0cbh,02h,058h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd58

; ---- DD CB 02 59
c_dd59:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,059h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd59:	di
	call	ldregs
	db	0ddh,0cbh,02h,059h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd59

; ---- DD CB 02 5A
c_dd5a:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,05ah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd5a:	di
	call	ldregs
	db	0ddh,0cbh,02h,05ah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd5a

; ---- DD CB 02 5B
c_dd5b:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,05bh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd5b:	di
	call	ldregs
	db	0ddh,0cbh,02h,05bh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd5b

; ---- DD CB 02 5C
c_dd5c:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,05ch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd5c:	di
	call	ldregs
	db	0ddh,0cbh,02h,05ch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd5c

; ---- DD CB 02 5D
c_dd5d:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,05dh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd5d:	di
	call	ldregs
	db	0ddh,0cbh,02h,05dh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd5d

; ---- DD CB 02 5E
c_dd5e:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,05eh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd5e:	di
	call	ldregs
	db	0ddh,0cbh,02h,05eh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd5e

; ---- DD CB 02 5F
c_dd5f:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,05fh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd5f:	di
	call	ldregs
	db	0ddh,0cbh,02h,05fh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd5f

; ---- DD CB 02 60
c_dd60:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,060h
	ld	(curop),a
	ld	a,0ddh
	call	progress
	xor	a
	ld	(curset),a
l_dd60:	di
	call	ldregs
	db	0ddh,0cbh,02h,060h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd60

; ---- DD CB 02 61
c_dd61:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,061h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd61:	di
	call	ldregs
	db	0ddh,0cbh,02h,061h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd61

; ---- DD CB 02 62
c_dd62:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,062h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd62:	di
	call	ldregs
	db	0ddh,0cbh,02h,062h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd62

; ---- DD CB 02 63
c_dd63:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,063h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd63:	di
	call	ldregs
	db	0ddh,0cbh,02h,063h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd63

; ---- DD CB 02 64
c_dd64:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,064h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd64:	di
	call	ldregs
	db	0ddh,0cbh,02h,064h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd64

; ---- DD CB 02 65
c_dd65:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,065h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd65:	di
	call	ldregs
	db	0ddh,0cbh,02h,065h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd65

; ---- DD CB 02 66
c_dd66:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,066h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd66:	di
	call	ldregs
	db	0ddh,0cbh,02h,066h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd66

; ---- DD CB 02 67
c_dd67:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,067h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd67:	di
	call	ldregs
	db	0ddh,0cbh,02h,067h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd67

; ---- DD CB 02 68
c_dd68:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,068h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd68:	di
	call	ldregs
	db	0ddh,0cbh,02h,068h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd68

; ---- DD CB 02 69
c_dd69:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,069h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd69:	di
	call	ldregs
	db	0ddh,0cbh,02h,069h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd69

; ---- DD CB 02 6A
c_dd6a:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,06ah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd6a:	di
	call	ldregs
	db	0ddh,0cbh,02h,06ah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd6a

; ---- DD CB 02 6B
c_dd6b:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,06bh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd6b:	di
	call	ldregs
	db	0ddh,0cbh,02h,06bh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd6b

; ---- DD CB 02 6C
c_dd6c:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,06ch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd6c:	di
	call	ldregs
	db	0ddh,0cbh,02h,06ch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd6c

; ---- DD CB 02 6D
c_dd6d:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,06dh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd6d:	di
	call	ldregs
	db	0ddh,0cbh,02h,06dh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd6d

; ---- DD CB 02 6E
c_dd6e:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,06eh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd6e:	di
	call	ldregs
	db	0ddh,0cbh,02h,06eh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd6e

; ---- DD CB 02 6F
c_dd6f:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,06fh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd6f:	di
	call	ldregs
	db	0ddh,0cbh,02h,06fh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd6f

; ---- DD CB 02 70
c_dd70:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,070h
	ld	(curop),a
	ld	a,0ddh
	call	progress
	xor	a
	ld	(curset),a
l_dd70:	di
	call	ldregs
	db	0ddh,0cbh,02h,070h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd70

; ---- DD CB 02 71
c_dd71:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,071h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd71:	di
	call	ldregs
	db	0ddh,0cbh,02h,071h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd71

; ---- DD CB 02 72
c_dd72:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,072h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd72:	di
	call	ldregs
	db	0ddh,0cbh,02h,072h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd72

; ---- DD CB 02 73
c_dd73:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,073h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd73:	di
	call	ldregs
	db	0ddh,0cbh,02h,073h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd73

; ---- DD CB 02 74
c_dd74:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,074h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd74:	di
	call	ldregs
	db	0ddh,0cbh,02h,074h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd74

; ---- DD CB 02 75
c_dd75:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,075h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd75:	di
	call	ldregs
	db	0ddh,0cbh,02h,075h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd75

; ---- DD CB 02 76
c_dd76:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,076h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd76:	di
	call	ldregs
	db	0ddh,0cbh,02h,076h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd76

; ---- DD CB 02 77
c_dd77:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,077h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd77:	di
	call	ldregs
	db	0ddh,0cbh,02h,077h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd77

; ---- DD CB 02 78
c_dd78:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,078h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd78:	di
	call	ldregs
	db	0ddh,0cbh,02h,078h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd78

; ---- DD CB 02 79
c_dd79:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,079h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd79:	di
	call	ldregs
	db	0ddh,0cbh,02h,079h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd79

; ---- DD CB 02 7A
c_dd7a:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,07ah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd7a:	di
	call	ldregs
	db	0ddh,0cbh,02h,07ah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd7a

; ---- DD CB 02 7B
c_dd7b:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,07bh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd7b:	di
	call	ldregs
	db	0ddh,0cbh,02h,07bh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd7b

; ---- DD CB 02 7C
c_dd7c:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,07ch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd7c:	di
	call	ldregs
	db	0ddh,0cbh,02h,07ch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd7c

; ---- DD CB 02 7D
c_dd7d:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,07dh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd7d:	di
	call	ldregs
	db	0ddh,0cbh,02h,07dh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd7d

; ---- DD CB 02 7E
c_dd7e:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,07eh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd7e:	di
	call	ldregs
	db	0ddh,0cbh,02h,07eh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd7e

; ---- DD CB 02 7F
c_dd7f:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,07fh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd7f:	di
	call	ldregs
	db	0ddh,0cbh,02h,07fh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd7f

; ---- DD CB 02 80
c_dd80:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,080h
	ld	(curop),a
	ld	a,0ddh
	call	progress
	xor	a
	ld	(curset),a
l_dd80:	di
	call	ldregs
	db	0ddh,0cbh,02h,080h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd80

; ---- DD CB 02 81
c_dd81:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,081h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd81:	di
	call	ldregs
	db	0ddh,0cbh,02h,081h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd81

; ---- DD CB 02 82
c_dd82:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,082h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd82:	di
	call	ldregs
	db	0ddh,0cbh,02h,082h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd82

; ---- DD CB 02 83
c_dd83:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,083h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd83:	di
	call	ldregs
	db	0ddh,0cbh,02h,083h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd83

; ---- DD CB 02 84
c_dd84:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,084h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd84:	di
	call	ldregs
	db	0ddh,0cbh,02h,084h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd84

; ---- DD CB 02 85
c_dd85:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,085h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd85:	di
	call	ldregs
	db	0ddh,0cbh,02h,085h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd85

; ---- DD CB 02 86
c_dd86:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,086h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd86:	di
	call	ldregs
	db	0ddh,0cbh,02h,086h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd86

; ---- DD CB 02 87
c_dd87:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,087h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd87:	di
	call	ldregs
	db	0ddh,0cbh,02h,087h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd87

; ---- DD CB 02 88
c_dd88:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,088h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd88:	di
	call	ldregs
	db	0ddh,0cbh,02h,088h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd88

; ---- DD CB 02 89
c_dd89:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,089h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd89:	di
	call	ldregs
	db	0ddh,0cbh,02h,089h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd89

; ---- DD CB 02 8A
c_dd8a:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,08ah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd8a:	di
	call	ldregs
	db	0ddh,0cbh,02h,08ah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd8a

; ---- DD CB 02 8B
c_dd8b:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,08bh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd8b:	di
	call	ldregs
	db	0ddh,0cbh,02h,08bh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd8b

; ---- DD CB 02 8C
c_dd8c:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,08ch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd8c:	di
	call	ldregs
	db	0ddh,0cbh,02h,08ch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd8c

; ---- DD CB 02 8D
c_dd8d:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,08dh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd8d:	di
	call	ldregs
	db	0ddh,0cbh,02h,08dh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd8d

; ---- DD CB 02 8E
c_dd8e:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,08eh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd8e:	di
	call	ldregs
	db	0ddh,0cbh,02h,08eh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd8e

; ---- DD CB 02 8F
c_dd8f:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,08fh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd8f:	di
	call	ldregs
	db	0ddh,0cbh,02h,08fh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd8f

; ---- DD CB 02 90
c_dd90:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,090h
	ld	(curop),a
	ld	a,0ddh
	call	progress
	xor	a
	ld	(curset),a
l_dd90:	di
	call	ldregs
	db	0ddh,0cbh,02h,090h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd90

; ---- DD CB 02 91
c_dd91:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,091h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd91:	di
	call	ldregs
	db	0ddh,0cbh,02h,091h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd91

; ---- DD CB 02 92
c_dd92:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,092h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd92:	di
	call	ldregs
	db	0ddh,0cbh,02h,092h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd92

; ---- DD CB 02 93
c_dd93:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,093h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd93:	di
	call	ldregs
	db	0ddh,0cbh,02h,093h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd93

; ---- DD CB 02 94
c_dd94:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,094h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd94:	di
	call	ldregs
	db	0ddh,0cbh,02h,094h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd94

; ---- DD CB 02 95
c_dd95:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,095h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd95:	di
	call	ldregs
	db	0ddh,0cbh,02h,095h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd95

; ---- DD CB 02 96
c_dd96:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,096h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd96:	di
	call	ldregs
	db	0ddh,0cbh,02h,096h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd96

; ---- DD CB 02 97
c_dd97:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,097h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd97:	di
	call	ldregs
	db	0ddh,0cbh,02h,097h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd97

; ---- DD CB 02 98
c_dd98:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,098h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd98:	di
	call	ldregs
	db	0ddh,0cbh,02h,098h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd98

; ---- DD CB 02 99
c_dd99:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,099h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd99:	di
	call	ldregs
	db	0ddh,0cbh,02h,099h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd99

; ---- DD CB 02 9A
c_dd9a:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,09ah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd9a:	di
	call	ldregs
	db	0ddh,0cbh,02h,09ah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd9a

; ---- DD CB 02 9B
c_dd9b:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,09bh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd9b:	di
	call	ldregs
	db	0ddh,0cbh,02h,09bh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd9b

; ---- DD CB 02 9C
c_dd9c:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,09ch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd9c:	di
	call	ldregs
	db	0ddh,0cbh,02h,09ch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd9c

; ---- DD CB 02 9D
c_dd9d:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,09dh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd9d:	di
	call	ldregs
	db	0ddh,0cbh,02h,09dh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd9d

; ---- DD CB 02 9E
c_dd9e:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,09eh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd9e:	di
	call	ldregs
	db	0ddh,0cbh,02h,09eh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd9e

; ---- DD CB 02 9F
c_dd9f:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,09fh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dd9f:	di
	call	ldregs
	db	0ddh,0cbh,02h,09fh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dd9f

; ---- DD CB 02 A0
c_dda0:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0a0h
	ld	(curop),a
	ld	a,0ddh
	call	progress
	xor	a
	ld	(curset),a
l_dda0:	di
	call	ldregs
	db	0ddh,0cbh,02h,0a0h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dda0

; ---- DD CB 02 A1
c_dda1:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0a1h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dda1:	di
	call	ldregs
	db	0ddh,0cbh,02h,0a1h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dda1

; ---- DD CB 02 A2
c_dda2:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0a2h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dda2:	di
	call	ldregs
	db	0ddh,0cbh,02h,0a2h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dda2

; ---- DD CB 02 A3
c_dda3:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0a3h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dda3:	di
	call	ldregs
	db	0ddh,0cbh,02h,0a3h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dda3

; ---- DD CB 02 A4
c_dda4:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0a4h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dda4:	di
	call	ldregs
	db	0ddh,0cbh,02h,0a4h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dda4

; ---- DD CB 02 A5
c_dda5:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0a5h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dda5:	di
	call	ldregs
	db	0ddh,0cbh,02h,0a5h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dda5

; ---- DD CB 02 A6
c_dda6:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0a6h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dda6:	di
	call	ldregs
	db	0ddh,0cbh,02h,0a6h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dda6

; ---- DD CB 02 A7
c_dda7:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0a7h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dda7:	di
	call	ldregs
	db	0ddh,0cbh,02h,0a7h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dda7

; ---- DD CB 02 A8
c_dda8:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0a8h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dda8:	di
	call	ldregs
	db	0ddh,0cbh,02h,0a8h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dda8

; ---- DD CB 02 A9
c_dda9:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0a9h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dda9:	di
	call	ldregs
	db	0ddh,0cbh,02h,0a9h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dda9

; ---- DD CB 02 AA
c_ddaa:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0aah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddaa:	di
	call	ldregs
	db	0ddh,0cbh,02h,0aah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddaa

; ---- DD CB 02 AB
c_ddab:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0abh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddab:	di
	call	ldregs
	db	0ddh,0cbh,02h,0abh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddab

; ---- DD CB 02 AC
c_ddac:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0ach
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddac:	di
	call	ldregs
	db	0ddh,0cbh,02h,0ach	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddac

; ---- DD CB 02 AD
c_ddad:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0adh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddad:	di
	call	ldregs
	db	0ddh,0cbh,02h,0adh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddad

; ---- DD CB 02 AE
c_ddae:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0aeh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddae:	di
	call	ldregs
	db	0ddh,0cbh,02h,0aeh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddae

; ---- DD CB 02 AF
c_ddaf:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0afh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddaf:	di
	call	ldregs
	db	0ddh,0cbh,02h,0afh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddaf

; ---- DD CB 02 B0
c_ddb0:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0b0h
	ld	(curop),a
	ld	a,0ddh
	call	progress
	xor	a
	ld	(curset),a
l_ddb0:	di
	call	ldregs
	db	0ddh,0cbh,02h,0b0h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddb0

; ---- DD CB 02 B1
c_ddb1:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0b1h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddb1:	di
	call	ldregs
	db	0ddh,0cbh,02h,0b1h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddb1

; ---- DD CB 02 B2
c_ddb2:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0b2h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddb2:	di
	call	ldregs
	db	0ddh,0cbh,02h,0b2h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddb2

; ---- DD CB 02 B3
c_ddb3:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0b3h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddb3:	di
	call	ldregs
	db	0ddh,0cbh,02h,0b3h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddb3

; ---- DD CB 02 B4
c_ddb4:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0b4h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddb4:	di
	call	ldregs
	db	0ddh,0cbh,02h,0b4h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddb4

; ---- DD CB 02 B5
c_ddb5:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0b5h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddb5:	di
	call	ldregs
	db	0ddh,0cbh,02h,0b5h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddb5

; ---- DD CB 02 B6
c_ddb6:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0b6h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddb6:	di
	call	ldregs
	db	0ddh,0cbh,02h,0b6h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddb6

; ---- DD CB 02 B7
c_ddb7:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0b7h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddb7:	di
	call	ldregs
	db	0ddh,0cbh,02h,0b7h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddb7

; ---- DD CB 02 B8
c_ddb8:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0b8h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddb8:	di
	call	ldregs
	db	0ddh,0cbh,02h,0b8h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddb8

; ---- DD CB 02 B9
c_ddb9:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0b9h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddb9:	di
	call	ldregs
	db	0ddh,0cbh,02h,0b9h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddb9

; ---- DD CB 02 BA
c_ddba:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0bah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddba:	di
	call	ldregs
	db	0ddh,0cbh,02h,0bah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddba

; ---- DD CB 02 BB
c_ddbb:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0bbh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddbb:	di
	call	ldregs
	db	0ddh,0cbh,02h,0bbh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddbb

; ---- DD CB 02 BC
c_ddbc:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0bch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddbc:	di
	call	ldregs
	db	0ddh,0cbh,02h,0bch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddbc

; ---- DD CB 02 BD
c_ddbd:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0bdh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddbd:	di
	call	ldregs
	db	0ddh,0cbh,02h,0bdh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddbd

; ---- DD CB 02 BE
c_ddbe:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0beh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddbe:	di
	call	ldregs
	db	0ddh,0cbh,02h,0beh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddbe

; ---- DD CB 02 BF
c_ddbf:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0bfh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddbf:	di
	call	ldregs
	db	0ddh,0cbh,02h,0bfh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddbf

; ---- DD CB 02 C0
c_ddc0:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0c0h
	ld	(curop),a
	ld	a,0ddh
	call	progress
	xor	a
	ld	(curset),a
l_ddc0:	di
	call	ldregs
	db	0ddh,0cbh,02h,0c0h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddc0

; ---- DD CB 02 C1
c_ddc1:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0c1h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddc1:	di
	call	ldregs
	db	0ddh,0cbh,02h,0c1h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddc1

; ---- DD CB 02 C2
c_ddc2:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0c2h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddc2:	di
	call	ldregs
	db	0ddh,0cbh,02h,0c2h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddc2

; ---- DD CB 02 C3
c_ddc3:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0c3h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddc3:	di
	call	ldregs
	db	0ddh,0cbh,02h,0c3h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddc3

; ---- DD CB 02 C4
c_ddc4:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0c4h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddc4:	di
	call	ldregs
	db	0ddh,0cbh,02h,0c4h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddc4

; ---- DD CB 02 C5
c_ddc5:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0c5h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddc5:	di
	call	ldregs
	db	0ddh,0cbh,02h,0c5h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddc5

; ---- DD CB 02 C6
c_ddc6:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0c6h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddc6:	di
	call	ldregs
	db	0ddh,0cbh,02h,0c6h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddc6

; ---- DD CB 02 C7
c_ddc7:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0c7h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddc7:	di
	call	ldregs
	db	0ddh,0cbh,02h,0c7h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddc7

; ---- DD CB 02 C8
c_ddc8:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0c8h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddc8:	di
	call	ldregs
	db	0ddh,0cbh,02h,0c8h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddc8

; ---- DD CB 02 C9
c_ddc9:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0c9h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddc9:	di
	call	ldregs
	db	0ddh,0cbh,02h,0c9h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddc9

; ---- DD CB 02 CA
c_ddca:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0cah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddca:	di
	call	ldregs
	db	0ddh,0cbh,02h,0cah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddca

; ---- DD CB 02 CB
c_ddcb:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0cbh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddcb:	di
	call	ldregs
	db	0ddh,0cbh,02h,0cbh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddcb

; ---- DD CB 02 CC
c_ddcc:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0cch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddcc:	di
	call	ldregs
	db	0ddh,0cbh,02h,0cch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddcc

; ---- DD CB 02 CD
c_ddcd:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0cdh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddcd:	di
	call	ldregs
	db	0ddh,0cbh,02h,0cdh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddcd

; ---- DD CB 02 CE
c_ddce:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0ceh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddce:	di
	call	ldregs
	db	0ddh,0cbh,02h,0ceh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddce

; ---- DD CB 02 CF
c_ddcf:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0cfh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddcf:	di
	call	ldregs
	db	0ddh,0cbh,02h,0cfh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddcf

; ---- DD CB 02 D0
c_ddd0:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0d0h
	ld	(curop),a
	ld	a,0ddh
	call	progress
	xor	a
	ld	(curset),a
l_ddd0:	di
	call	ldregs
	db	0ddh,0cbh,02h,0d0h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddd0

; ---- DD CB 02 D1
c_ddd1:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0d1h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddd1:	di
	call	ldregs
	db	0ddh,0cbh,02h,0d1h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddd1

; ---- DD CB 02 D2
c_ddd2:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0d2h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddd2:	di
	call	ldregs
	db	0ddh,0cbh,02h,0d2h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddd2

; ---- DD CB 02 D3
c_ddd3:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0d3h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddd3:	di
	call	ldregs
	db	0ddh,0cbh,02h,0d3h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddd3

; ---- DD CB 02 D4
c_ddd4:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0d4h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddd4:	di
	call	ldregs
	db	0ddh,0cbh,02h,0d4h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddd4

; ---- DD CB 02 D5
c_ddd5:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0d5h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddd5:	di
	call	ldregs
	db	0ddh,0cbh,02h,0d5h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddd5

; ---- DD CB 02 D6
c_ddd6:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0d6h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddd6:	di
	call	ldregs
	db	0ddh,0cbh,02h,0d6h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddd6

; ---- DD CB 02 D7
c_ddd7:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0d7h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddd7:	di
	call	ldregs
	db	0ddh,0cbh,02h,0d7h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddd7

; ---- DD CB 02 D8
c_ddd8:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0d8h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddd8:	di
	call	ldregs
	db	0ddh,0cbh,02h,0d8h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddd8

; ---- DD CB 02 D9
c_ddd9:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0d9h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddd9:	di
	call	ldregs
	db	0ddh,0cbh,02h,0d9h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddd9

; ---- DD CB 02 DA
c_ddda:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0dah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddda:	di
	call	ldregs
	db	0ddh,0cbh,02h,0dah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddda

; ---- DD CB 02 DB
c_dddb:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0dbh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dddb:	di
	call	ldregs
	db	0ddh,0cbh,02h,0dbh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dddb

; ---- DD CB 02 DC
c_dddc:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0dch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dddc:	di
	call	ldregs
	db	0ddh,0cbh,02h,0dch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dddc

; ---- DD CB 02 DD
c_dddd:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0ddh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dddd:	di
	call	ldregs
	db	0ddh,0cbh,02h,0ddh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dddd

; ---- DD CB 02 DE
c_ddde:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0deh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddde:	di
	call	ldregs
	db	0ddh,0cbh,02h,0deh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddde

; ---- DD CB 02 DF
c_dddf:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0dfh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dddf:	di
	call	ldregs
	db	0ddh,0cbh,02h,0dfh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dddf

; ---- DD CB 02 E0
c_dde0:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0e0h
	ld	(curop),a
	ld	a,0ddh
	call	progress
	xor	a
	ld	(curset),a
l_dde0:	di
	call	ldregs
	db	0ddh,0cbh,02h,0e0h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dde0

; ---- DD CB 02 E1
c_dde1:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0e1h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dde1:	di
	call	ldregs
	db	0ddh,0cbh,02h,0e1h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dde1

; ---- DD CB 02 E2
c_dde2:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0e2h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dde2:	di
	call	ldregs
	db	0ddh,0cbh,02h,0e2h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dde2

; ---- DD CB 02 E3
c_dde3:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0e3h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dde3:	di
	call	ldregs
	db	0ddh,0cbh,02h,0e3h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dde3

; ---- DD CB 02 E4
c_dde4:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0e4h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dde4:	di
	call	ldregs
	db	0ddh,0cbh,02h,0e4h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dde4

; ---- DD CB 02 E5
c_dde5:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0e5h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dde5:	di
	call	ldregs
	db	0ddh,0cbh,02h,0e5h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dde5

; ---- DD CB 02 E6
c_dde6:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0e6h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dde6:	di
	call	ldregs
	db	0ddh,0cbh,02h,0e6h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dde6

; ---- DD CB 02 E7
c_dde7:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0e7h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dde7:	di
	call	ldregs
	db	0ddh,0cbh,02h,0e7h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dde7

; ---- DD CB 02 E8
c_dde8:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0e8h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dde8:	di
	call	ldregs
	db	0ddh,0cbh,02h,0e8h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dde8

; ---- DD CB 02 E9
c_dde9:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0e9h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dde9:	di
	call	ldregs
	db	0ddh,0cbh,02h,0e9h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dde9

; ---- DD CB 02 EA
c_ddea:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0eah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddea:	di
	call	ldregs
	db	0ddh,0cbh,02h,0eah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddea

; ---- DD CB 02 EB
c_ddeb:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0ebh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddeb:	di
	call	ldregs
	db	0ddh,0cbh,02h,0ebh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddeb

; ---- DD CB 02 EC
c_ddec:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0ech
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddec:	di
	call	ldregs
	db	0ddh,0cbh,02h,0ech	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddec

; ---- DD CB 02 ED
c_dded:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0edh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_dded:	di
	call	ldregs
	db	0ddh,0cbh,02h,0edh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_dded

; ---- DD CB 02 EE
c_ddee:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0eeh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddee:	di
	call	ldregs
	db	0ddh,0cbh,02h,0eeh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddee

; ---- DD CB 02 EF
c_ddef:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0efh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddef:	di
	call	ldregs
	db	0ddh,0cbh,02h,0efh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddef

; ---- DD CB 02 F0
c_ddf0:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0f0h
	ld	(curop),a
	ld	a,0ddh
	call	progress
	xor	a
	ld	(curset),a
l_ddf0:	di
	call	ldregs
	db	0ddh,0cbh,02h,0f0h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddf0

; ---- DD CB 02 F1
c_ddf1:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0f1h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddf1:	di
	call	ldregs
	db	0ddh,0cbh,02h,0f1h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddf1

; ---- DD CB 02 F2
c_ddf2:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0f2h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddf2:	di
	call	ldregs
	db	0ddh,0cbh,02h,0f2h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddf2

; ---- DD CB 02 F3
c_ddf3:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0f3h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddf3:	di
	call	ldregs
	db	0ddh,0cbh,02h,0f3h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddf3

; ---- DD CB 02 F4
c_ddf4:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0f4h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddf4:	di
	call	ldregs
	db	0ddh,0cbh,02h,0f4h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddf4

; ---- DD CB 02 F5
c_ddf5:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0f5h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddf5:	di
	call	ldregs
	db	0ddh,0cbh,02h,0f5h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddf5

; ---- DD CB 02 F6
c_ddf6:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0f6h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddf6:	di
	call	ldregs
	db	0ddh,0cbh,02h,0f6h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddf6

; ---- DD CB 02 F7
c_ddf7:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0f7h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddf7:	di
	call	ldregs
	db	0ddh,0cbh,02h,0f7h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddf7

; ---- DD CB 02 F8
c_ddf8:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0f8h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddf8:	di
	call	ldregs
	db	0ddh,0cbh,02h,0f8h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddf8

; ---- DD CB 02 F9
c_ddf9:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0f9h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddf9:	di
	call	ldregs
	db	0ddh,0cbh,02h,0f9h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddf9

; ---- DD CB 02 FA
c_ddfa:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0fah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddfa:	di
	call	ldregs
	db	0ddh,0cbh,02h,0fah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddfa

; ---- DD CB 02 FB
c_ddfb:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0fbh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddfb:	di
	call	ldregs
	db	0ddh,0cbh,02h,0fbh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddfb

; ---- DD CB 02 FC
c_ddfc:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0fch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddfc:	di
	call	ldregs
	db	0ddh,0cbh,02h,0fch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddfc

; ---- DD CB 02 FD
c_ddfd:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0fdh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddfd:	di
	call	ldregs
	db	0ddh,0cbh,02h,0fdh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddfd

; ---- DD CB 02 FE
c_ddfe:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0feh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddfe:	di
	call	ldregs
	db	0ddh,0cbh,02h,0feh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddfe

; ---- DD CB 02 FF
c_ddff:	ld	a,0ddh
	ld	(curpfx),a
	ld	a,0ffh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_ddff:	di
	call	ldregs
	db	0ddh,0cbh,02h,0ffh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_ddff

; ---- FD CB 02 00
c_fd00:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,000h
	ld	(curop),a
	ld	a,0fdh
	call	progress
	xor	a
	ld	(curset),a
l_fd00:	di
	call	ldregs
	db	0fdh,0cbh,02h,000h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd00

; ---- FD CB 02 01
c_fd01:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,001h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd01:	di
	call	ldregs
	db	0fdh,0cbh,02h,001h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd01

; ---- FD CB 02 02
c_fd02:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,002h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd02:	di
	call	ldregs
	db	0fdh,0cbh,02h,002h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd02

; ---- FD CB 02 03
c_fd03:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,003h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd03:	di
	call	ldregs
	db	0fdh,0cbh,02h,003h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd03

; ---- FD CB 02 04
c_fd04:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,004h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd04:	di
	call	ldregs
	db	0fdh,0cbh,02h,004h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd04

; ---- FD CB 02 05
c_fd05:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,005h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd05:	di
	call	ldregs
	db	0fdh,0cbh,02h,005h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd05

; ---- FD CB 02 06
c_fd06:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,006h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd06:	di
	call	ldregs
	db	0fdh,0cbh,02h,006h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd06

; ---- FD CB 02 07
c_fd07:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,007h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd07:	di
	call	ldregs
	db	0fdh,0cbh,02h,007h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd07

; ---- FD CB 02 08
c_fd08:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,008h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd08:	di
	call	ldregs
	db	0fdh,0cbh,02h,008h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd08

; ---- FD CB 02 09
c_fd09:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,009h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd09:	di
	call	ldregs
	db	0fdh,0cbh,02h,009h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd09

; ---- FD CB 02 0A
c_fd0a:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,00ah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd0a:	di
	call	ldregs
	db	0fdh,0cbh,02h,00ah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd0a

; ---- FD CB 02 0B
c_fd0b:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,00bh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd0b:	di
	call	ldregs
	db	0fdh,0cbh,02h,00bh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd0b

; ---- FD CB 02 0C
c_fd0c:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,00ch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd0c:	di
	call	ldregs
	db	0fdh,0cbh,02h,00ch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd0c

; ---- FD CB 02 0D
c_fd0d:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,00dh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd0d:	di
	call	ldregs
	db	0fdh,0cbh,02h,00dh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd0d

; ---- FD CB 02 0E
c_fd0e:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,00eh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd0e:	di
	call	ldregs
	db	0fdh,0cbh,02h,00eh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd0e

; ---- FD CB 02 0F
c_fd0f:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,00fh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd0f:	di
	call	ldregs
	db	0fdh,0cbh,02h,00fh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd0f

; ---- FD CB 02 10
c_fd10:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,010h
	ld	(curop),a
	ld	a,0fdh
	call	progress
	xor	a
	ld	(curset),a
l_fd10:	di
	call	ldregs
	db	0fdh,0cbh,02h,010h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd10

; ---- FD CB 02 11
c_fd11:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,011h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd11:	di
	call	ldregs
	db	0fdh,0cbh,02h,011h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd11

; ---- FD CB 02 12
c_fd12:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,012h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd12:	di
	call	ldregs
	db	0fdh,0cbh,02h,012h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd12

; ---- FD CB 02 13
c_fd13:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,013h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd13:	di
	call	ldregs
	db	0fdh,0cbh,02h,013h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd13

; ---- FD CB 02 14
c_fd14:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,014h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd14:	di
	call	ldregs
	db	0fdh,0cbh,02h,014h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd14

; ---- FD CB 02 15
c_fd15:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,015h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd15:	di
	call	ldregs
	db	0fdh,0cbh,02h,015h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd15

; ---- FD CB 02 16
c_fd16:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,016h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd16:	di
	call	ldregs
	db	0fdh,0cbh,02h,016h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd16

; ---- FD CB 02 17
c_fd17:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,017h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd17:	di
	call	ldregs
	db	0fdh,0cbh,02h,017h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd17

; ---- FD CB 02 18
c_fd18:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,018h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd18:	di
	call	ldregs
	db	0fdh,0cbh,02h,018h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd18

; ---- FD CB 02 19
c_fd19:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,019h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd19:	di
	call	ldregs
	db	0fdh,0cbh,02h,019h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd19

; ---- FD CB 02 1A
c_fd1a:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,01ah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd1a:	di
	call	ldregs
	db	0fdh,0cbh,02h,01ah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd1a

; ---- FD CB 02 1B
c_fd1b:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,01bh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd1b:	di
	call	ldregs
	db	0fdh,0cbh,02h,01bh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd1b

; ---- FD CB 02 1C
c_fd1c:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,01ch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd1c:	di
	call	ldregs
	db	0fdh,0cbh,02h,01ch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd1c

; ---- FD CB 02 1D
c_fd1d:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,01dh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd1d:	di
	call	ldregs
	db	0fdh,0cbh,02h,01dh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd1d

; ---- FD CB 02 1E
c_fd1e:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,01eh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd1e:	di
	call	ldregs
	db	0fdh,0cbh,02h,01eh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd1e

; ---- FD CB 02 1F
c_fd1f:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,01fh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd1f:	di
	call	ldregs
	db	0fdh,0cbh,02h,01fh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd1f

; ---- FD CB 02 20
c_fd20:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,020h
	ld	(curop),a
	ld	a,0fdh
	call	progress
	xor	a
	ld	(curset),a
l_fd20:	di
	call	ldregs
	db	0fdh,0cbh,02h,020h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd20

; ---- FD CB 02 21
c_fd21:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,021h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd21:	di
	call	ldregs
	db	0fdh,0cbh,02h,021h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd21

; ---- FD CB 02 22
c_fd22:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,022h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd22:	di
	call	ldregs
	db	0fdh,0cbh,02h,022h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd22

; ---- FD CB 02 23
c_fd23:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,023h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd23:	di
	call	ldregs
	db	0fdh,0cbh,02h,023h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd23

; ---- FD CB 02 24
c_fd24:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,024h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd24:	di
	call	ldregs
	db	0fdh,0cbh,02h,024h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd24

; ---- FD CB 02 25
c_fd25:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,025h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd25:	di
	call	ldregs
	db	0fdh,0cbh,02h,025h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd25

; ---- FD CB 02 26
c_fd26:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,026h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd26:	di
	call	ldregs
	db	0fdh,0cbh,02h,026h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd26

; ---- FD CB 02 27
c_fd27:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,027h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd27:	di
	call	ldregs
	db	0fdh,0cbh,02h,027h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd27

; ---- FD CB 02 28
c_fd28:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,028h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd28:	di
	call	ldregs
	db	0fdh,0cbh,02h,028h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd28

; ---- FD CB 02 29
c_fd29:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,029h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd29:	di
	call	ldregs
	db	0fdh,0cbh,02h,029h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd29

; ---- FD CB 02 2A
c_fd2a:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,02ah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd2a:	di
	call	ldregs
	db	0fdh,0cbh,02h,02ah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd2a

; ---- FD CB 02 2B
c_fd2b:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,02bh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd2b:	di
	call	ldregs
	db	0fdh,0cbh,02h,02bh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd2b

; ---- FD CB 02 2C
c_fd2c:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,02ch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd2c:	di
	call	ldregs
	db	0fdh,0cbh,02h,02ch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd2c

; ---- FD CB 02 2D
c_fd2d:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,02dh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd2d:	di
	call	ldregs
	db	0fdh,0cbh,02h,02dh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd2d

; ---- FD CB 02 2E
c_fd2e:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,02eh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd2e:	di
	call	ldregs
	db	0fdh,0cbh,02h,02eh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd2e

; ---- FD CB 02 2F
c_fd2f:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,02fh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd2f:	di
	call	ldregs
	db	0fdh,0cbh,02h,02fh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd2f

; ---- FD CB 02 30
c_fd30:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,030h
	ld	(curop),a
	ld	a,0fdh
	call	progress
	xor	a
	ld	(curset),a
l_fd30:	di
	call	ldregs
	db	0fdh,0cbh,02h,030h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd30

; ---- FD CB 02 31
c_fd31:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,031h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd31:	di
	call	ldregs
	db	0fdh,0cbh,02h,031h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd31

; ---- FD CB 02 32
c_fd32:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,032h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd32:	di
	call	ldregs
	db	0fdh,0cbh,02h,032h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd32

; ---- FD CB 02 33
c_fd33:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,033h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd33:	di
	call	ldregs
	db	0fdh,0cbh,02h,033h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd33

; ---- FD CB 02 34
c_fd34:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,034h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd34:	di
	call	ldregs
	db	0fdh,0cbh,02h,034h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd34

; ---- FD CB 02 35
c_fd35:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,035h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd35:	di
	call	ldregs
	db	0fdh,0cbh,02h,035h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd35

; ---- FD CB 02 36
c_fd36:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,036h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd36:	di
	call	ldregs
	db	0fdh,0cbh,02h,036h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd36

; ---- FD CB 02 37
c_fd37:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,037h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd37:	di
	call	ldregs
	db	0fdh,0cbh,02h,037h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd37

; ---- FD CB 02 38
c_fd38:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,038h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd38:	di
	call	ldregs
	db	0fdh,0cbh,02h,038h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd38

; ---- FD CB 02 39
c_fd39:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,039h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd39:	di
	call	ldregs
	db	0fdh,0cbh,02h,039h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd39

; ---- FD CB 02 3A
c_fd3a:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,03ah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd3a:	di
	call	ldregs
	db	0fdh,0cbh,02h,03ah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd3a

; ---- FD CB 02 3B
c_fd3b:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,03bh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd3b:	di
	call	ldregs
	db	0fdh,0cbh,02h,03bh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd3b

; ---- FD CB 02 3C
c_fd3c:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,03ch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd3c:	di
	call	ldregs
	db	0fdh,0cbh,02h,03ch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd3c

; ---- FD CB 02 3D
c_fd3d:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,03dh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd3d:	di
	call	ldregs
	db	0fdh,0cbh,02h,03dh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd3d

; ---- FD CB 02 3E
c_fd3e:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,03eh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd3e:	di
	call	ldregs
	db	0fdh,0cbh,02h,03eh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd3e

; ---- FD CB 02 3F
c_fd3f:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,03fh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd3f:	di
	call	ldregs
	db	0fdh,0cbh,02h,03fh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd3f

; ---- FD CB 02 40
c_fd40:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,040h
	ld	(curop),a
	ld	a,0fdh
	call	progress
	xor	a
	ld	(curset),a
l_fd40:	di
	call	ldregs
	db	0fdh,0cbh,02h,040h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd40

; ---- FD CB 02 41
c_fd41:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,041h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd41:	di
	call	ldregs
	db	0fdh,0cbh,02h,041h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd41

; ---- FD CB 02 42
c_fd42:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,042h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd42:	di
	call	ldregs
	db	0fdh,0cbh,02h,042h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd42

; ---- FD CB 02 43
c_fd43:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,043h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd43:	di
	call	ldregs
	db	0fdh,0cbh,02h,043h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd43

; ---- FD CB 02 44
c_fd44:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,044h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd44:	di
	call	ldregs
	db	0fdh,0cbh,02h,044h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd44

; ---- FD CB 02 45
c_fd45:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,045h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd45:	di
	call	ldregs
	db	0fdh,0cbh,02h,045h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd45

; ---- FD CB 02 46
c_fd46:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,046h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd46:	di
	call	ldregs
	db	0fdh,0cbh,02h,046h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd46

; ---- FD CB 02 47
c_fd47:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,047h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd47:	di
	call	ldregs
	db	0fdh,0cbh,02h,047h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd47

; ---- FD CB 02 48
c_fd48:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,048h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd48:	di
	call	ldregs
	db	0fdh,0cbh,02h,048h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd48

; ---- FD CB 02 49
c_fd49:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,049h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd49:	di
	call	ldregs
	db	0fdh,0cbh,02h,049h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd49

; ---- FD CB 02 4A
c_fd4a:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,04ah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd4a:	di
	call	ldregs
	db	0fdh,0cbh,02h,04ah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd4a

; ---- FD CB 02 4B
c_fd4b:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,04bh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd4b:	di
	call	ldregs
	db	0fdh,0cbh,02h,04bh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd4b

; ---- FD CB 02 4C
c_fd4c:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,04ch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd4c:	di
	call	ldregs
	db	0fdh,0cbh,02h,04ch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd4c

; ---- FD CB 02 4D
c_fd4d:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,04dh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd4d:	di
	call	ldregs
	db	0fdh,0cbh,02h,04dh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd4d

; ---- FD CB 02 4E
c_fd4e:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,04eh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd4e:	di
	call	ldregs
	db	0fdh,0cbh,02h,04eh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd4e

; ---- FD CB 02 4F
c_fd4f:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,04fh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd4f:	di
	call	ldregs
	db	0fdh,0cbh,02h,04fh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd4f

; ---- FD CB 02 50
c_fd50:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,050h
	ld	(curop),a
	ld	a,0fdh
	call	progress
	xor	a
	ld	(curset),a
l_fd50:	di
	call	ldregs
	db	0fdh,0cbh,02h,050h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd50

; ---- FD CB 02 51
c_fd51:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,051h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd51:	di
	call	ldregs
	db	0fdh,0cbh,02h,051h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd51

; ---- FD CB 02 52
c_fd52:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,052h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd52:	di
	call	ldregs
	db	0fdh,0cbh,02h,052h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd52

; ---- FD CB 02 53
c_fd53:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,053h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd53:	di
	call	ldregs
	db	0fdh,0cbh,02h,053h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd53

; ---- FD CB 02 54
c_fd54:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,054h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd54:	di
	call	ldregs
	db	0fdh,0cbh,02h,054h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd54

; ---- FD CB 02 55
c_fd55:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,055h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd55:	di
	call	ldregs
	db	0fdh,0cbh,02h,055h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd55

; ---- FD CB 02 56
c_fd56:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,056h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd56:	di
	call	ldregs
	db	0fdh,0cbh,02h,056h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd56

; ---- FD CB 02 57
c_fd57:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,057h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd57:	di
	call	ldregs
	db	0fdh,0cbh,02h,057h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd57

; ---- FD CB 02 58
c_fd58:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,058h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd58:	di
	call	ldregs
	db	0fdh,0cbh,02h,058h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd58

; ---- FD CB 02 59
c_fd59:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,059h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd59:	di
	call	ldregs
	db	0fdh,0cbh,02h,059h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd59

; ---- FD CB 02 5A
c_fd5a:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,05ah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd5a:	di
	call	ldregs
	db	0fdh,0cbh,02h,05ah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd5a

; ---- FD CB 02 5B
c_fd5b:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,05bh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd5b:	di
	call	ldregs
	db	0fdh,0cbh,02h,05bh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd5b

; ---- FD CB 02 5C
c_fd5c:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,05ch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd5c:	di
	call	ldregs
	db	0fdh,0cbh,02h,05ch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd5c

; ---- FD CB 02 5D
c_fd5d:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,05dh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd5d:	di
	call	ldregs
	db	0fdh,0cbh,02h,05dh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd5d

; ---- FD CB 02 5E
c_fd5e:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,05eh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd5e:	di
	call	ldregs
	db	0fdh,0cbh,02h,05eh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd5e

; ---- FD CB 02 5F
c_fd5f:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,05fh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd5f:	di
	call	ldregs
	db	0fdh,0cbh,02h,05fh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd5f

; ---- FD CB 02 60
c_fd60:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,060h
	ld	(curop),a
	ld	a,0fdh
	call	progress
	xor	a
	ld	(curset),a
l_fd60:	di
	call	ldregs
	db	0fdh,0cbh,02h,060h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd60

; ---- FD CB 02 61
c_fd61:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,061h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd61:	di
	call	ldregs
	db	0fdh,0cbh,02h,061h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd61

; ---- FD CB 02 62
c_fd62:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,062h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd62:	di
	call	ldregs
	db	0fdh,0cbh,02h,062h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd62

; ---- FD CB 02 63
c_fd63:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,063h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd63:	di
	call	ldregs
	db	0fdh,0cbh,02h,063h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd63

; ---- FD CB 02 64
c_fd64:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,064h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd64:	di
	call	ldregs
	db	0fdh,0cbh,02h,064h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd64

; ---- FD CB 02 65
c_fd65:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,065h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd65:	di
	call	ldregs
	db	0fdh,0cbh,02h,065h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd65

; ---- FD CB 02 66
c_fd66:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,066h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd66:	di
	call	ldregs
	db	0fdh,0cbh,02h,066h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd66

; ---- FD CB 02 67
c_fd67:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,067h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd67:	di
	call	ldregs
	db	0fdh,0cbh,02h,067h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd67

; ---- FD CB 02 68
c_fd68:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,068h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd68:	di
	call	ldregs
	db	0fdh,0cbh,02h,068h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd68

; ---- FD CB 02 69
c_fd69:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,069h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd69:	di
	call	ldregs
	db	0fdh,0cbh,02h,069h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd69

; ---- FD CB 02 6A
c_fd6a:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,06ah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd6a:	di
	call	ldregs
	db	0fdh,0cbh,02h,06ah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd6a

; ---- FD CB 02 6B
c_fd6b:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,06bh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd6b:	di
	call	ldregs
	db	0fdh,0cbh,02h,06bh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd6b

; ---- FD CB 02 6C
c_fd6c:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,06ch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd6c:	di
	call	ldregs
	db	0fdh,0cbh,02h,06ch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd6c

; ---- FD CB 02 6D
c_fd6d:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,06dh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd6d:	di
	call	ldregs
	db	0fdh,0cbh,02h,06dh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd6d

; ---- FD CB 02 6E
c_fd6e:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,06eh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd6e:	di
	call	ldregs
	db	0fdh,0cbh,02h,06eh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd6e

; ---- FD CB 02 6F
c_fd6f:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,06fh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd6f:	di
	call	ldregs
	db	0fdh,0cbh,02h,06fh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd6f

; ---- FD CB 02 70
c_fd70:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,070h
	ld	(curop),a
	ld	a,0fdh
	call	progress
	xor	a
	ld	(curset),a
l_fd70:	di
	call	ldregs
	db	0fdh,0cbh,02h,070h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd70

; ---- FD CB 02 71
c_fd71:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,071h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd71:	di
	call	ldregs
	db	0fdh,0cbh,02h,071h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd71

; ---- FD CB 02 72
c_fd72:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,072h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd72:	di
	call	ldregs
	db	0fdh,0cbh,02h,072h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd72

; ---- FD CB 02 73
c_fd73:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,073h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd73:	di
	call	ldregs
	db	0fdh,0cbh,02h,073h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd73

; ---- FD CB 02 74
c_fd74:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,074h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd74:	di
	call	ldregs
	db	0fdh,0cbh,02h,074h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd74

; ---- FD CB 02 75
c_fd75:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,075h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd75:	di
	call	ldregs
	db	0fdh,0cbh,02h,075h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd75

; ---- FD CB 02 76
c_fd76:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,076h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd76:	di
	call	ldregs
	db	0fdh,0cbh,02h,076h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd76

; ---- FD CB 02 77
c_fd77:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,077h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd77:	di
	call	ldregs
	db	0fdh,0cbh,02h,077h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd77

; ---- FD CB 02 78
c_fd78:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,078h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd78:	di
	call	ldregs
	db	0fdh,0cbh,02h,078h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd78

; ---- FD CB 02 79
c_fd79:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,079h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd79:	di
	call	ldregs
	db	0fdh,0cbh,02h,079h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd79

; ---- FD CB 02 7A
c_fd7a:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,07ah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd7a:	di
	call	ldregs
	db	0fdh,0cbh,02h,07ah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd7a

; ---- FD CB 02 7B
c_fd7b:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,07bh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd7b:	di
	call	ldregs
	db	0fdh,0cbh,02h,07bh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd7b

; ---- FD CB 02 7C
c_fd7c:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,07ch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd7c:	di
	call	ldregs
	db	0fdh,0cbh,02h,07ch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd7c

; ---- FD CB 02 7D
c_fd7d:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,07dh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd7d:	di
	call	ldregs
	db	0fdh,0cbh,02h,07dh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd7d

; ---- FD CB 02 7E
c_fd7e:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,07eh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd7e:	di
	call	ldregs
	db	0fdh,0cbh,02h,07eh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd7e

; ---- FD CB 02 7F
c_fd7f:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,07fh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd7f:	di
	call	ldregs
	db	0fdh,0cbh,02h,07fh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd7f

; ---- FD CB 02 80
c_fd80:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,080h
	ld	(curop),a
	ld	a,0fdh
	call	progress
	xor	a
	ld	(curset),a
l_fd80:	di
	call	ldregs
	db	0fdh,0cbh,02h,080h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd80

; ---- FD CB 02 81
c_fd81:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,081h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd81:	di
	call	ldregs
	db	0fdh,0cbh,02h,081h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd81

; ---- FD CB 02 82
c_fd82:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,082h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd82:	di
	call	ldregs
	db	0fdh,0cbh,02h,082h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd82

; ---- FD CB 02 83
c_fd83:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,083h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd83:	di
	call	ldregs
	db	0fdh,0cbh,02h,083h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd83

; ---- FD CB 02 84
c_fd84:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,084h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd84:	di
	call	ldregs
	db	0fdh,0cbh,02h,084h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd84

; ---- FD CB 02 85
c_fd85:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,085h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd85:	di
	call	ldregs
	db	0fdh,0cbh,02h,085h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd85

; ---- FD CB 02 86
c_fd86:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,086h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd86:	di
	call	ldregs
	db	0fdh,0cbh,02h,086h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd86

; ---- FD CB 02 87
c_fd87:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,087h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd87:	di
	call	ldregs
	db	0fdh,0cbh,02h,087h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd87

; ---- FD CB 02 88
c_fd88:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,088h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd88:	di
	call	ldregs
	db	0fdh,0cbh,02h,088h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd88

; ---- FD CB 02 89
c_fd89:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,089h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd89:	di
	call	ldregs
	db	0fdh,0cbh,02h,089h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd89

; ---- FD CB 02 8A
c_fd8a:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,08ah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd8a:	di
	call	ldregs
	db	0fdh,0cbh,02h,08ah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd8a

; ---- FD CB 02 8B
c_fd8b:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,08bh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd8b:	di
	call	ldregs
	db	0fdh,0cbh,02h,08bh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd8b

; ---- FD CB 02 8C
c_fd8c:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,08ch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd8c:	di
	call	ldregs
	db	0fdh,0cbh,02h,08ch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd8c

; ---- FD CB 02 8D
c_fd8d:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,08dh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd8d:	di
	call	ldregs
	db	0fdh,0cbh,02h,08dh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd8d

; ---- FD CB 02 8E
c_fd8e:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,08eh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd8e:	di
	call	ldregs
	db	0fdh,0cbh,02h,08eh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd8e

; ---- FD CB 02 8F
c_fd8f:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,08fh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd8f:	di
	call	ldregs
	db	0fdh,0cbh,02h,08fh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd8f

; ---- FD CB 02 90
c_fd90:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,090h
	ld	(curop),a
	ld	a,0fdh
	call	progress
	xor	a
	ld	(curset),a
l_fd90:	di
	call	ldregs
	db	0fdh,0cbh,02h,090h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd90

; ---- FD CB 02 91
c_fd91:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,091h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd91:	di
	call	ldregs
	db	0fdh,0cbh,02h,091h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd91

; ---- FD CB 02 92
c_fd92:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,092h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd92:	di
	call	ldregs
	db	0fdh,0cbh,02h,092h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd92

; ---- FD CB 02 93
c_fd93:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,093h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd93:	di
	call	ldregs
	db	0fdh,0cbh,02h,093h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd93

; ---- FD CB 02 94
c_fd94:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,094h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd94:	di
	call	ldregs
	db	0fdh,0cbh,02h,094h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd94

; ---- FD CB 02 95
c_fd95:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,095h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd95:	di
	call	ldregs
	db	0fdh,0cbh,02h,095h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd95

; ---- FD CB 02 96
c_fd96:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,096h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd96:	di
	call	ldregs
	db	0fdh,0cbh,02h,096h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd96

; ---- FD CB 02 97
c_fd97:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,097h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd97:	di
	call	ldregs
	db	0fdh,0cbh,02h,097h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd97

; ---- FD CB 02 98
c_fd98:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,098h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd98:	di
	call	ldregs
	db	0fdh,0cbh,02h,098h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd98

; ---- FD CB 02 99
c_fd99:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,099h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd99:	di
	call	ldregs
	db	0fdh,0cbh,02h,099h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd99

; ---- FD CB 02 9A
c_fd9a:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,09ah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd9a:	di
	call	ldregs
	db	0fdh,0cbh,02h,09ah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd9a

; ---- FD CB 02 9B
c_fd9b:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,09bh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd9b:	di
	call	ldregs
	db	0fdh,0cbh,02h,09bh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd9b

; ---- FD CB 02 9C
c_fd9c:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,09ch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd9c:	di
	call	ldregs
	db	0fdh,0cbh,02h,09ch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd9c

; ---- FD CB 02 9D
c_fd9d:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,09dh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd9d:	di
	call	ldregs
	db	0fdh,0cbh,02h,09dh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd9d

; ---- FD CB 02 9E
c_fd9e:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,09eh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd9e:	di
	call	ldregs
	db	0fdh,0cbh,02h,09eh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd9e

; ---- FD CB 02 9F
c_fd9f:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,09fh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fd9f:	di
	call	ldregs
	db	0fdh,0cbh,02h,09fh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fd9f

; ---- FD CB 02 A0
c_fda0:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0a0h
	ld	(curop),a
	ld	a,0fdh
	call	progress
	xor	a
	ld	(curset),a
l_fda0:	di
	call	ldregs
	db	0fdh,0cbh,02h,0a0h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fda0

; ---- FD CB 02 A1
c_fda1:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0a1h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fda1:	di
	call	ldregs
	db	0fdh,0cbh,02h,0a1h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fda1

; ---- FD CB 02 A2
c_fda2:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0a2h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fda2:	di
	call	ldregs
	db	0fdh,0cbh,02h,0a2h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fda2

; ---- FD CB 02 A3
c_fda3:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0a3h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fda3:	di
	call	ldregs
	db	0fdh,0cbh,02h,0a3h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fda3

; ---- FD CB 02 A4
c_fda4:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0a4h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fda4:	di
	call	ldregs
	db	0fdh,0cbh,02h,0a4h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fda4

; ---- FD CB 02 A5
c_fda5:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0a5h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fda5:	di
	call	ldregs
	db	0fdh,0cbh,02h,0a5h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fda5

; ---- FD CB 02 A6
c_fda6:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0a6h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fda6:	di
	call	ldregs
	db	0fdh,0cbh,02h,0a6h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fda6

; ---- FD CB 02 A7
c_fda7:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0a7h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fda7:	di
	call	ldregs
	db	0fdh,0cbh,02h,0a7h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fda7

; ---- FD CB 02 A8
c_fda8:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0a8h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fda8:	di
	call	ldregs
	db	0fdh,0cbh,02h,0a8h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fda8

; ---- FD CB 02 A9
c_fda9:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0a9h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fda9:	di
	call	ldregs
	db	0fdh,0cbh,02h,0a9h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fda9

; ---- FD CB 02 AA
c_fdaa:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0aah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdaa:	di
	call	ldregs
	db	0fdh,0cbh,02h,0aah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdaa

; ---- FD CB 02 AB
c_fdab:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0abh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdab:	di
	call	ldregs
	db	0fdh,0cbh,02h,0abh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdab

; ---- FD CB 02 AC
c_fdac:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0ach
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdac:	di
	call	ldregs
	db	0fdh,0cbh,02h,0ach	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdac

; ---- FD CB 02 AD
c_fdad:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0adh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdad:	di
	call	ldregs
	db	0fdh,0cbh,02h,0adh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdad

; ---- FD CB 02 AE
c_fdae:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0aeh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdae:	di
	call	ldregs
	db	0fdh,0cbh,02h,0aeh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdae

; ---- FD CB 02 AF
c_fdaf:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0afh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdaf:	di
	call	ldregs
	db	0fdh,0cbh,02h,0afh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdaf

; ---- FD CB 02 B0
c_fdb0:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0b0h
	ld	(curop),a
	ld	a,0fdh
	call	progress
	xor	a
	ld	(curset),a
l_fdb0:	di
	call	ldregs
	db	0fdh,0cbh,02h,0b0h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdb0

; ---- FD CB 02 B1
c_fdb1:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0b1h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdb1:	di
	call	ldregs
	db	0fdh,0cbh,02h,0b1h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdb1

; ---- FD CB 02 B2
c_fdb2:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0b2h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdb2:	di
	call	ldregs
	db	0fdh,0cbh,02h,0b2h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdb2

; ---- FD CB 02 B3
c_fdb3:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0b3h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdb3:	di
	call	ldregs
	db	0fdh,0cbh,02h,0b3h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdb3

; ---- FD CB 02 B4
c_fdb4:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0b4h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdb4:	di
	call	ldregs
	db	0fdh,0cbh,02h,0b4h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdb4

; ---- FD CB 02 B5
c_fdb5:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0b5h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdb5:	di
	call	ldregs
	db	0fdh,0cbh,02h,0b5h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdb5

; ---- FD CB 02 B6
c_fdb6:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0b6h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdb6:	di
	call	ldregs
	db	0fdh,0cbh,02h,0b6h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdb6

; ---- FD CB 02 B7
c_fdb7:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0b7h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdb7:	di
	call	ldregs
	db	0fdh,0cbh,02h,0b7h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdb7

; ---- FD CB 02 B8
c_fdb8:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0b8h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdb8:	di
	call	ldregs
	db	0fdh,0cbh,02h,0b8h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdb8

; ---- FD CB 02 B9
c_fdb9:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0b9h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdb9:	di
	call	ldregs
	db	0fdh,0cbh,02h,0b9h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdb9

; ---- FD CB 02 BA
c_fdba:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0bah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdba:	di
	call	ldregs
	db	0fdh,0cbh,02h,0bah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdba

; ---- FD CB 02 BB
c_fdbb:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0bbh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdbb:	di
	call	ldregs
	db	0fdh,0cbh,02h,0bbh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdbb

; ---- FD CB 02 BC
c_fdbc:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0bch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdbc:	di
	call	ldregs
	db	0fdh,0cbh,02h,0bch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdbc

; ---- FD CB 02 BD
c_fdbd:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0bdh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdbd:	di
	call	ldregs
	db	0fdh,0cbh,02h,0bdh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdbd

; ---- FD CB 02 BE
c_fdbe:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0beh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdbe:	di
	call	ldregs
	db	0fdh,0cbh,02h,0beh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdbe

; ---- FD CB 02 BF
c_fdbf:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0bfh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdbf:	di
	call	ldregs
	db	0fdh,0cbh,02h,0bfh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdbf

; ---- FD CB 02 C0
c_fdc0:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0c0h
	ld	(curop),a
	ld	a,0fdh
	call	progress
	xor	a
	ld	(curset),a
l_fdc0:	di
	call	ldregs
	db	0fdh,0cbh,02h,0c0h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdc0

; ---- FD CB 02 C1
c_fdc1:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0c1h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdc1:	di
	call	ldregs
	db	0fdh,0cbh,02h,0c1h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdc1

; ---- FD CB 02 C2
c_fdc2:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0c2h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdc2:	di
	call	ldregs
	db	0fdh,0cbh,02h,0c2h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdc2

; ---- FD CB 02 C3
c_fdc3:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0c3h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdc3:	di
	call	ldregs
	db	0fdh,0cbh,02h,0c3h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdc3

; ---- FD CB 02 C4
c_fdc4:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0c4h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdc4:	di
	call	ldregs
	db	0fdh,0cbh,02h,0c4h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdc4

; ---- FD CB 02 C5
c_fdc5:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0c5h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdc5:	di
	call	ldregs
	db	0fdh,0cbh,02h,0c5h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdc5

; ---- FD CB 02 C6
c_fdc6:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0c6h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdc6:	di
	call	ldregs
	db	0fdh,0cbh,02h,0c6h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdc6

; ---- FD CB 02 C7
c_fdc7:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0c7h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdc7:	di
	call	ldregs
	db	0fdh,0cbh,02h,0c7h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdc7

; ---- FD CB 02 C8
c_fdc8:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0c8h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdc8:	di
	call	ldregs
	db	0fdh,0cbh,02h,0c8h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdc8

; ---- FD CB 02 C9
c_fdc9:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0c9h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdc9:	di
	call	ldregs
	db	0fdh,0cbh,02h,0c9h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdc9

; ---- FD CB 02 CA
c_fdca:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0cah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdca:	di
	call	ldregs
	db	0fdh,0cbh,02h,0cah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdca

; ---- FD CB 02 CB
c_fdcb:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0cbh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdcb:	di
	call	ldregs
	db	0fdh,0cbh,02h,0cbh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdcb

; ---- FD CB 02 CC
c_fdcc:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0cch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdcc:	di
	call	ldregs
	db	0fdh,0cbh,02h,0cch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdcc

; ---- FD CB 02 CD
c_fdcd:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0cdh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdcd:	di
	call	ldregs
	db	0fdh,0cbh,02h,0cdh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdcd

; ---- FD CB 02 CE
c_fdce:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0ceh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdce:	di
	call	ldregs
	db	0fdh,0cbh,02h,0ceh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdce

; ---- FD CB 02 CF
c_fdcf:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0cfh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdcf:	di
	call	ldregs
	db	0fdh,0cbh,02h,0cfh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdcf

; ---- FD CB 02 D0
c_fdd0:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0d0h
	ld	(curop),a
	ld	a,0fdh
	call	progress
	xor	a
	ld	(curset),a
l_fdd0:	di
	call	ldregs
	db	0fdh,0cbh,02h,0d0h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdd0

; ---- FD CB 02 D1
c_fdd1:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0d1h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdd1:	di
	call	ldregs
	db	0fdh,0cbh,02h,0d1h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdd1

; ---- FD CB 02 D2
c_fdd2:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0d2h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdd2:	di
	call	ldregs
	db	0fdh,0cbh,02h,0d2h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdd2

; ---- FD CB 02 D3
c_fdd3:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0d3h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdd3:	di
	call	ldregs
	db	0fdh,0cbh,02h,0d3h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdd3

; ---- FD CB 02 D4
c_fdd4:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0d4h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdd4:	di
	call	ldregs
	db	0fdh,0cbh,02h,0d4h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdd4

; ---- FD CB 02 D5
c_fdd5:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0d5h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdd5:	di
	call	ldregs
	db	0fdh,0cbh,02h,0d5h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdd5

; ---- FD CB 02 D6
c_fdd6:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0d6h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdd6:	di
	call	ldregs
	db	0fdh,0cbh,02h,0d6h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdd6

; ---- FD CB 02 D7
c_fdd7:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0d7h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdd7:	di
	call	ldregs
	db	0fdh,0cbh,02h,0d7h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdd7

; ---- FD CB 02 D8
c_fdd8:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0d8h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdd8:	di
	call	ldregs
	db	0fdh,0cbh,02h,0d8h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdd8

; ---- FD CB 02 D9
c_fdd9:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0d9h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdd9:	di
	call	ldregs
	db	0fdh,0cbh,02h,0d9h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdd9

; ---- FD CB 02 DA
c_fdda:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0dah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdda:	di
	call	ldregs
	db	0fdh,0cbh,02h,0dah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdda

; ---- FD CB 02 DB
c_fddb:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0dbh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fddb:	di
	call	ldregs
	db	0fdh,0cbh,02h,0dbh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fddb

; ---- FD CB 02 DC
c_fddc:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0dch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fddc:	di
	call	ldregs
	db	0fdh,0cbh,02h,0dch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fddc

; ---- FD CB 02 DD
c_fddd:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0ddh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fddd:	di
	call	ldregs
	db	0fdh,0cbh,02h,0ddh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fddd

; ---- FD CB 02 DE
c_fdde:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0deh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdde:	di
	call	ldregs
	db	0fdh,0cbh,02h,0deh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdde

; ---- FD CB 02 DF
c_fddf:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0dfh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fddf:	di
	call	ldregs
	db	0fdh,0cbh,02h,0dfh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fddf

; ---- FD CB 02 E0
c_fde0:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0e0h
	ld	(curop),a
	ld	a,0fdh
	call	progress
	xor	a
	ld	(curset),a
l_fde0:	di
	call	ldregs
	db	0fdh,0cbh,02h,0e0h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fde0

; ---- FD CB 02 E1
c_fde1:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0e1h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fde1:	di
	call	ldregs
	db	0fdh,0cbh,02h,0e1h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fde1

; ---- FD CB 02 E2
c_fde2:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0e2h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fde2:	di
	call	ldregs
	db	0fdh,0cbh,02h,0e2h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fde2

; ---- FD CB 02 E3
c_fde3:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0e3h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fde3:	di
	call	ldregs
	db	0fdh,0cbh,02h,0e3h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fde3

; ---- FD CB 02 E4
c_fde4:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0e4h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fde4:	di
	call	ldregs
	db	0fdh,0cbh,02h,0e4h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fde4

; ---- FD CB 02 E5
c_fde5:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0e5h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fde5:	di
	call	ldregs
	db	0fdh,0cbh,02h,0e5h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fde5

; ---- FD CB 02 E6
c_fde6:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0e6h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fde6:	di
	call	ldregs
	db	0fdh,0cbh,02h,0e6h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fde6

; ---- FD CB 02 E7
c_fde7:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0e7h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fde7:	di
	call	ldregs
	db	0fdh,0cbh,02h,0e7h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fde7

; ---- FD CB 02 E8
c_fde8:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0e8h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fde8:	di
	call	ldregs
	db	0fdh,0cbh,02h,0e8h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fde8

; ---- FD CB 02 E9
c_fde9:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0e9h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fde9:	di
	call	ldregs
	db	0fdh,0cbh,02h,0e9h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fde9

; ---- FD CB 02 EA
c_fdea:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0eah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdea:	di
	call	ldregs
	db	0fdh,0cbh,02h,0eah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdea

; ---- FD CB 02 EB
c_fdeb:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0ebh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdeb:	di
	call	ldregs
	db	0fdh,0cbh,02h,0ebh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdeb

; ---- FD CB 02 EC
c_fdec:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0ech
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdec:	di
	call	ldregs
	db	0fdh,0cbh,02h,0ech	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdec

; ---- FD CB 02 ED
c_fded:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0edh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fded:	di
	call	ldregs
	db	0fdh,0cbh,02h,0edh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fded

; ---- FD CB 02 EE
c_fdee:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0eeh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdee:	di
	call	ldregs
	db	0fdh,0cbh,02h,0eeh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdee

; ---- FD CB 02 EF
c_fdef:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0efh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdef:	di
	call	ldregs
	db	0fdh,0cbh,02h,0efh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdef

; ---- FD CB 02 F0
c_fdf0:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0f0h
	ld	(curop),a
	ld	a,0fdh
	call	progress
	xor	a
	ld	(curset),a
l_fdf0:	di
	call	ldregs
	db	0fdh,0cbh,02h,0f0h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdf0

; ---- FD CB 02 F1
c_fdf1:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0f1h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdf1:	di
	call	ldregs
	db	0fdh,0cbh,02h,0f1h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdf1

; ---- FD CB 02 F2
c_fdf2:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0f2h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdf2:	di
	call	ldregs
	db	0fdh,0cbh,02h,0f2h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdf2

; ---- FD CB 02 F3
c_fdf3:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0f3h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdf3:	di
	call	ldregs
	db	0fdh,0cbh,02h,0f3h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdf3

; ---- FD CB 02 F4
c_fdf4:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0f4h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdf4:	di
	call	ldregs
	db	0fdh,0cbh,02h,0f4h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdf4

; ---- FD CB 02 F5
c_fdf5:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0f5h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdf5:	di
	call	ldregs
	db	0fdh,0cbh,02h,0f5h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdf5

; ---- FD CB 02 F6
c_fdf6:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0f6h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdf6:	di
	call	ldregs
	db	0fdh,0cbh,02h,0f6h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdf6

; ---- FD CB 02 F7
c_fdf7:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0f7h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdf7:	di
	call	ldregs
	db	0fdh,0cbh,02h,0f7h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdf7

; ---- FD CB 02 F8
c_fdf8:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0f8h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdf8:	di
	call	ldregs
	db	0fdh,0cbh,02h,0f8h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdf8

; ---- FD CB 02 F9
c_fdf9:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0f9h
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdf9:	di
	call	ldregs
	db	0fdh,0cbh,02h,0f9h	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdf9

; ---- FD CB 02 FA
c_fdfa:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0fah
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdfa:	di
	call	ldregs
	db	0fdh,0cbh,02h,0fah	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdfa

; ---- FD CB 02 FB
c_fdfb:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0fbh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdfb:	di
	call	ldregs
	db	0fdh,0cbh,02h,0fbh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdfb

; ---- FD CB 02 FC
c_fdfc:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0fch
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdfc:	di
	call	ldregs
	db	0fdh,0cbh,02h,0fch	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdfc

; ---- FD CB 02 FD
c_fdfd:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0fdh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdfd:	di
	call	ldregs
	db	0fdh,0cbh,02h,0fdh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdfd

; ---- FD CB 02 FE
c_fdfe:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0feh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdfe:	di
	call	ldregs
	db	0fdh,0cbh,02h,0feh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdfe

; ---- FD CB 02 FF
c_fdff:	ld	a,0fdh
	ld	(curpfx),a
	ld	a,0ffh
	ld	(curop),a
	xor	a
	ld	(curset),a
l_fdff:	di
	call	ldregs
	db	0fdh,0cbh,02h,0ffh	; under test
	call	capt
	ei
	call	report
	ld	hl,curset
	inc	(hl)
	ld	a,(hl)
	cp	2
	jp	nz,l_fdff

	call	fclose
	jp	0

; load the registers and the probed byte for set (curset); returns with them
; loaded (RET does not change flags)
ldregs:	ld	a,(curset)
	or	a
	ld	a,081h
	ld	hl,00a00h	; A=0Ah F=00h
	jp	z,ldr1
	ld	a,07eh
	ld	hl,00ad7h	; A=0Ah F=D7h
ldr1:	ld	(probe),a
	push	hl
	pop	af
	ld	bc,0b0ch
	ld	de,0d0eh
	ld	hl,4455h
	ld	ix,probe-2
	ld	iy,probe-2
	ret

; capture right after the opcode: AF first
capt:	push	af
	ld	(obc),bc
	ld	(ode),de
	ld	(ohl),hl
	ld	(oix),ix
	ld	(oiy),iy
	pop	hl
	ld	(oaf),hl
	ld	a,(probe)
	ld	(omem),a
	ret

; one result line to the file
report:	ld	a,(curpfx)
	call	fhex2
	ld	de,s_cb
	call	fputs
	ld	a,(curop)
	call	fhex2
	ld	a,' '
	call	fputc
	ld	a,(curset)
	add	a,'0'
	call	fputc
	ld	de,s_a
	call	fputs
	ld	a,(oaf+1)
	call	fhex2
	ld	de,s_f
	call	fputs
	ld	a,(oaf)
	call	fhex2
	ld	de,s_bc
	call	fputs
	ld	hl,(obc)
	call	fhex4
	ld	de,s_de
	call	fputs
	ld	hl,(ode)
	call	fhex4
	ld	de,s_hl
	call	fputs
	ld	hl,(ohl)
	call	fhex4
	ld	de,s_ix
	call	fputs
	ld	hl,(oix)
	call	fhex4
	ld	de,s_iy
	call	fputs
	ld	hl,(oiy)
	call	fhex4
	ld	de,s_m
	call	fputs
	ld	a,(omem)
	call	fhex2
	ld	a,13
	call	fputc
	ld	a,10
	jp	fputc

; console progress: "pp CB 02 xx" for a = prefix, (curop) = opcode
progress: push	af
	call	conhex
	ld	de,s_cb
	call	conmsg
	ld	a,(curop)
	call	conhex
	ld	e,13
	ld	c,2
	call	bdosv
	ld	e,10
	ld	c,2
	call	bdosv
	pop	af
	ret

fputs:	ld	a,(de)
	cp	'$'
	ret	z
	call	fputc
	inc	de
	jp	fputs

fhex2:	push	af
	rrca
	rrca
	rrca
	rrca
	call	hexch
	call	fputc
	pop	af
	call	hexch
	jp	fputc
fhex4:	ld	a,h
	call	fhex2
	ld	a,l
	jp	fhex2

; low nibble of a -> ASCII hex digit in a (table lookup)
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

title:	db	'CBPRB M102 DD/FD CB opcode probe',13,10,'$'
s_cb:	db	' CB 02 $'
s_a:	db	' A=$'
s_f:	db	' F=$'
s_bc:	db	' BC=$'
s_de:	db	' DE=$'
s_hl:	db	' HL=$'
s_ix:	db	' IX=$'
s_iy:	db	' IY=$'
s_m:	db	' M=$'
fcbout:	db	0,'CBPRB   TXT'
	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
curpfx:	db	0
curop:	db	0
curset:	db	0
oaf:	dw	0
obc:	dw	0
ode:	dw	0
ohl:	dw	0
oix:	dw	0
oiy:	dw	0
omem:	db	0
	ds	4
probe:	db	0
	ds	4
	ds	128
stack:
