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

; M102 EDPRB4: sweep C through 00h-FFh for the undefined ED 20.
; EDPRB3 (run 7) showed that the 16-bit value written by the undefined
; ED 00-3F/74/75/77 depends on C alone, wraps modulo 256 and looks like a
; word read of one hidden byte-addressed page.  This program runs ED 20
; (destination HL) with the EDPRB3 baseline inputs for every C and records
; one 16-byte line per C in EDPRB4.TXT:
;
;     Ycc vvvv ff...CR LF      (vvvv = HL after, ff = F after, '.' = space)
;
; The file is closed as a checkpoint after every eight lines (one CP/M
; record).  z80asm 1.8 syntax; cpmio.asm is appended by zexbuild.py.

	org	100h

start:	ld	sp,stack
	ld	de,fcbout
	call	fopen
	ld	de,title
	call	conmsg
	ld	a,0
	ld	(ccur),a

loop:	ld	a,'Y'
	call	putc
	ld	a,(ccur)
	call	hex2
	ld	a,' '
	call	putc
	ld	(mainsp),sp
	di
	ld	sp,5404h	; EDPRB3 baseline sandbox stack
	ld	hl,2524h
	ld	(3404h),hl
	ld	hl,3534h
	ld	(3c04h),hl
	ld	hl,6564h
	ld	(5404h),hl
	ld	a,(ccur)
	ld	c,a
	ld	b,1
	ld	de,3404h
	ld	hl,5ad7h
	push	hl
	pop	af
	ld	hl,3c04h
	db	0edh,020h	; under test
	push	af
	pop	de		; e = F after
	ld	sp,(mainsp)
	ei
	call	hex4		; value from hl
	ld	a,' '
	call	putc
	ld	a,e
	call	hex2
	ld	a,' '
	call	fputc		; pad the 16-byte line, file only
	ld	a,' '
	call	fputc
	ld	a,' '
	call	fputc
	call	crlf
	ld	a,(ccur)
	and	7
	cp	7		; checkpoint after every full record
	jp	nz,next
	ld	de,(curfcb)
	ld	c,16		; close as a checkpoint; writing continues
	call	bdosv
next:	ld	a,(ccur)
	inc	a
	ld	(ccur),a
	or	a
	jp	nz,loop

	call	fclose
	ld	de,done
	call	conmsg
	jp	0

title:	db	'# EDPRB4 M102 undefined ED C sweep',13,10,'$'
done:	db	'EDPRB4 done',13,10,'$'
ccur:	db	0
mainsp:	dw	0
fcbout:	db	0,'EDPRB4  TXT'
	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	ds	64
stack:
