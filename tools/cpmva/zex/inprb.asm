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

; M102 INPRB: documented I/O reads for the undefined ED 00-3F/74/75/77 value.
; On the real uPD9002 those opcodes write an input-dependent 16-bit value
; that so far correlates only with C (EDPRB: C=0Ch -> 2329h; EDPRB2 sets
; 0-2: C=30h/50h/80h -> 2800h/232Ah/A426h) and set F like IN r,(C).  This
; probe reads the candidate ports with the documented IN instructions only:
;   Pnn xx xx xx xx    four reads of IN A,(nn)
;   Cbbcc xx yy        IN A,(C) with BC=bbcc, then INC C and IN A,(C)
; Output goes to INPRB.TXT and the console.  z80asm 1.8 syntax; cpmio.asm
; is appended by zexbuild.py.

	org	100h

start:	ld	sp,stack
	ld	de,fcbout
	call	fopen
	ld	de,title
	call	conmsg

; ---- IN A,(nn): the ports named by C in the EDPRB/EDPRB2 records
	di
	in	a,(00ch)
	ld	(smp),a
	in	a,(00ch)
	ld	(smp+1),a
	in	a,(00ch)
	ld	(smp+2),a
	in	a,(00ch)
	ld	(smp+3),a
	ei
	ld	a,00ch
	call	rptp

	di
	in	a,(00dh)
	ld	(smp),a
	in	a,(00dh)
	ld	(smp+1),a
	in	a,(00dh)
	ld	(smp+2),a
	in	a,(00dh)
	ld	(smp+3),a
	ei
	ld	a,00dh
	call	rptp

	di
	in	a,(030h)
	ld	(smp),a
	in	a,(030h)
	ld	(smp+1),a
	in	a,(030h)
	ld	(smp+2),a
	in	a,(030h)
	ld	(smp+3),a
	ei
	ld	a,030h
	call	rptp

	di
	in	a,(031h)
	ld	(smp),a
	in	a,(031h)
	ld	(smp+1),a
	in	a,(031h)
	ld	(smp+2),a
	in	a,(031h)
	ld	(smp+3),a
	ei
	ld	a,031h
	call	rptp

	di
	in	a,(050h)
	ld	(smp),a
	in	a,(050h)
	ld	(smp+1),a
	in	a,(050h)
	ld	(smp+2),a
	in	a,(050h)
	ld	(smp+3),a
	ei
	ld	a,050h
	call	rptp

	di
	in	a,(051h)
	ld	(smp),a
	in	a,(051h)
	ld	(smp+1),a
	in	a,(051h)
	ld	(smp+2),a
	in	a,(051h)
	ld	(smp+3),a
	ei
	ld	a,051h
	call	rptp

	di
	in	a,(080h)
	ld	(smp),a
	in	a,(080h)
	ld	(smp+1),a
	in	a,(080h)
	ld	(smp+2),a
	in	a,(080h)
	ld	(smp+3),a
	ei
	ld	a,080h
	call	rptp

	di
	in	a,(081h)
	ld	(smp),a
	in	a,(081h)
	ld	(smp+1),a
	in	a,(081h)
	ld	(smp+2),a
	in	a,(081h)
	ld	(smp+3),a
	ei
	ld	a,081h
	call	rptp

; ---- IN A,(C) with the exact BC of the ED probes, then C+1
	di
	ld	bc,00b0ch	; EDPRB
	in	a,(c)
	ld	(smp),a
	inc	c
	in	a,(c)
	ld	(smp+1),a
	ei
	ld	hl,00b0ch
	call	rptc

	di
	ld	bc,00130h	; EDPRB2 set 0
	in	a,(c)
	ld	(smp),a
	inc	c
	in	a,(c)
	ld	(smp+1),a
	ei
	ld	hl,00130h
	call	rptc

	di
	ld	bc,00150h	; EDPRB2 set 1
	in	a,(c)
	ld	(smp),a
	inc	c
	in	a,(c)
	ld	(smp+1),a
	ei
	ld	hl,00150h
	call	rptc

	di
	ld	bc,00180h	; EDPRB2 set 2
	in	a,(c)
	ld	(smp),a
	inc	c
	in	a,(c)
	ld	(smp+1),a
	ei
	ld	hl,00180h
	call	rptc

	call	fclose
	ld	de,done
	call	conmsg
	jp	0

; print 'Pnn xx xx xx xx' from a=nn and smp
rptp:	push	af
	push	hl
	ld	l,a
	ld	a,'P'
	call	putc
	ld	a,l
	call	hex2
	ld	hl,smp
	ld	a,' '
	call	putc
	ld	a,(hl)
	call	hex2
	inc	hl
	ld	a,' '
	call	putc
	ld	a,(hl)
	call	hex2
	inc	hl
	ld	a,' '
	call	putc
	ld	a,(hl)
	call	hex2
	inc	hl
	ld	a,' '
	call	putc
	ld	a,(hl)
	call	hex2
	call	crlf
	pop	hl
	pop	af
	ret

; print 'Cbbcc xx yy' from hl=bbcc and smp
rptc:	push	af
	ld	a,'C'
	call	putc
	call	hex4
	ld	hl,smp
	ld	a,' '
	call	putc
	ld	a,(hl)
	call	hex2
	inc	hl
	ld	a,' '
	call	putc
	ld	a,(hl)
	call	hex2
	call	crlf
	pop	af
	ret

title:	db	'# INPRB M102 documented I/O reads',13,10,'$'
done:	db	'INPRB done',13,10,'$'
smp:	ds	4
fcbout:	db	0,'INPRB   TXT'
	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	ds	64
stack:
