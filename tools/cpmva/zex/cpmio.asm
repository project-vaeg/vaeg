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

; M101 CP/M 2.2 console and sequential-file output for the probe programs.
; z80asm 1.8 syntax; appended to each probe program by zexbuild.py.
;
; Rules shared with the probes (see docs/agents/tasks/M101_*.md):
; - only BDOS 2, 9, 16, 19, 21, 22 and 26 are used, always through CALL 5;
; - control flow branches only on Z, C or S produced by INC, DEC, CP, OR or
;   XOR; hex digits come from a table, not from compare-and-adjust code;
; - one file is open at a time; records are 128 bytes and a text file's last
;   record is padded with 1Ah.

bdosv:	equ	5

; open (create) the file whose FCB is at de; stop the program on failure
fopen:	push	af
	push	bc
	push	de
	push	hl
	ld	(curfcb),de
	ld	c,19		; delete any old file
	call	bdosv
	ld	hl,(curfcb)
	ld	de,12
	add	hl,de
	ld	(hl),0		; ex
	inc	hl
	inc	hl
	ld	(hl),0		; s2
	ld	hl,(curfcb)
	ld	de,32
	add	hl,de
	ld	(hl),0		; cr
	ld	de,(curfcb)
	ld	c,22		; make file
	call	bdosv
	inc	a		; 0ffh means no directory space
	jp	z,fmkfail
	ld	hl,fbuf
	ld	(fptr),hl
	ld	a,128
	ld	(fcnt),a
	pop	hl
	pop	de
	pop	bc
	pop	af
	ret
fmkfail: ld	de,fmkmsg
	ld	c,9
	call	bdosv
	jp	0

; append the byte in a to the open file
fputc:	push	af
	push	hl
	ld	hl,(fptr)
	ld	(hl),a
	inc	hl
	ld	(fptr),hl
	ld	hl,fcnt
	dec	(hl)
	call	z,fflush
	pop	hl
	pop	af
	ret

; write the record buffer and reset it; stop the program on failure
fflush:	push	af
	push	bc
	push	de
	push	hl
	ld	de,fbuf
	ld	c,26		; set dma address
	call	bdosv
	ld	de,(curfcb)
	ld	c,21		; write sequential
	call	bdosv
	or	a
	jp	nz,fwrfail
	ld	hl,fbuf
	ld	(fptr),hl
	ld	a,128
	ld	(fcnt),a
	pop	hl
	pop	de
	pop	bc
	pop	af
	ret
fwrfail: ld	de,fwrmsg
	ld	c,9
	call	bdosv
	jp	0

; pad the last record with 1ah, write it, and close the file
fclose:	push	af
	push	bc
	push	de
	push	hl
	ld	a,(fcnt)
	cp	128
	jp	z,fcls
fpad:	ld	a,01ah
	call	fputc
	ld	a,(fcnt)
	cp	128
	jp	nz,fpad
fcls:	ld	de,(curfcb)
	ld	c,16		; close file
	call	bdosv
	pop	hl
	pop	de
	pop	bc
	pop	af
	ret

; console only: write the '$'-terminated string at de
conmsg:	push	af
	push	bc
	push	de
	push	hl
	ld	c,9
	call	bdosv
	pop	hl
	pop	de
	pop	bc
	pop	af
	ret

; write the character in a to the console and to the open file
putc:	push	af
	push	bc
	push	de
	push	hl
	call	fputc
	ld	e,a
	ld	c,2
	call	bdosv
	pop	hl
	pop	de
	pop	bc
	pop	af
	ret

; write the '$'-terminated string at de to the console and the file
puts:	push	af
	push	de
putslp:	ld	a,(de)
	cp	'$'
	jp	z,putsx
	call	putc
	inc	de
	jp	putslp
putsx:	pop	de
	pop	af
	ret

; write the low nibble of a as one upper-case hex digit
hexdig:	push	af
	push	de
	push	hl
	and	0fh
	ld	e,a
	ld	d,0
	ld	hl,hextab
	add	hl,de
	ld	a,(hl)
	call	putc
	pop	hl
	pop	de
	pop	af
	ret

; write a as two hex digits
hex2:	push	af
	rrca
	rrca
	rrca
	rrca
	call	hexdig
	pop	af
	call	hexdig
	ret

; write hl as four hex digits
hex4:	push	af
	ld	a,h
	call	hex2
	ld	a,l
	call	hex2
	pop	af
	ret

crlf:	push	af
	ld	a,13
	call	putc
	ld	a,10
	call	putc
	pop	af
	ret

hextab:	db	'0123456789ABCDEF'
fmkmsg:	db	13,10,'Cannot create the output file',13,10,'$'
fwrmsg:	db	13,10,'Write error on the output file',13,10,'$'
curfcb:	dw	0
fptr:	dw	fbuf
fcnt:	db	128
fbuf:	ds	128
