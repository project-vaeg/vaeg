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

; M101 DAADUMP: exhaustive DAA/CPL/SCF/CCF dump for the uPD9002 Z80
; emulation mode. For each instruction, for fi = 0..63 and a = 00h..FFh:
; A = a, F = ftab[fi] (fi bits 5..0 spread over F bits 7, 6, 4, 2, 1, 0, i.e.
; S, Z, H, P/V, N, C; bits 5/3 stay clear), execute the instruction once and
; append A_out, F_out. Record offset = (fi*256 + a)*2; each file is 32768
; bytes. DAADUMP.TXT lists name, size and CRC-32 (EDB88320h, init and final
; XOR FFFFFFFFh) of each file; the same lines are printed on the console.
;
; Each instruction has its own code path. F is set only through
; PUSH BC / POP AF, captured with PUSH AF right after the instruction, under
; DI. Loop control branches only on Z from INC or CP.

	org	100h

start:	ld	sp,stack
	ld	de,title
	call	conmsg

; ---- DAA
	ld	de,m_daa
	call	conmsg
	ld	de,fcb_daa
	call	fopen
	call	crcini
	xor	a
	ld	(fi),a
daa_fi:	ld	a,(fi)
	ld	e,a
	ld	d,0
	ld	hl,ftab
	add	hl,de
	ld	a,(hl)
	ld	(fin),a
	xor	a
	ld	(av),a
daa_a:	ld	a,(fin)
	ld	c,a
	ld	a,(av)
	ld	b,a
	di
	push	bc
	pop	af
	daa			; under test
	push	af		; capture
	pop	bc
	ei
	ld	a,b
	call	bput
	ld	a,c
	call	bput
	ld	hl,av
	inc	(hl)
	jp	nz,daa_a
	ld	hl,fi
	inc	(hl)
	ld	a,(hl)
	cp	64
	jp	nz,daa_fi
	call	fclose
	ld	hl,crc_daa
	call	crcsave

; ---- CPL
	ld	de,m_cpl
	call	conmsg
	ld	de,fcb_cpl
	call	fopen
	call	crcini
	xor	a
	ld	(fi),a
cpl_fi:	ld	a,(fi)
	ld	e,a
	ld	d,0
	ld	hl,ftab
	add	hl,de
	ld	a,(hl)
	ld	(fin),a
	xor	a
	ld	(av),a
cpl_a:	ld	a,(fin)
	ld	c,a
	ld	a,(av)
	ld	b,a
	di
	push	bc
	pop	af
	cpl			; under test
	push	af		; capture
	pop	bc
	ei
	ld	a,b
	call	bput
	ld	a,c
	call	bput
	ld	hl,av
	inc	(hl)
	jp	nz,cpl_a
	ld	hl,fi
	inc	(hl)
	ld	a,(hl)
	cp	64
	jp	nz,cpl_fi
	call	fclose
	ld	hl,crc_cpl
	call	crcsave

; ---- SCF
	ld	de,m_scf
	call	conmsg
	ld	de,fcb_scf
	call	fopen
	call	crcini
	xor	a
	ld	(fi),a
scf_fi:	ld	a,(fi)
	ld	e,a
	ld	d,0
	ld	hl,ftab
	add	hl,de
	ld	a,(hl)
	ld	(fin),a
	xor	a
	ld	(av),a
scf_a:	ld	a,(fin)
	ld	c,a
	ld	a,(av)
	ld	b,a
	di
	push	bc
	pop	af
	scf			; under test
	push	af		; capture
	pop	bc
	ei
	ld	a,b
	call	bput
	ld	a,c
	call	bput
	ld	hl,av
	inc	(hl)
	jp	nz,scf_a
	ld	hl,fi
	inc	(hl)
	ld	a,(hl)
	cp	64
	jp	nz,scf_fi
	call	fclose
	ld	hl,crc_scf
	call	crcsave

; ---- CCF
	ld	de,m_ccf
	call	conmsg
	ld	de,fcb_ccf
	call	fopen
	call	crcini
	xor	a
	ld	(fi),a
ccf_fi:	ld	a,(fi)
	ld	e,a
	ld	d,0
	ld	hl,ftab
	add	hl,de
	ld	a,(hl)
	ld	(fin),a
	xor	a
	ld	(av),a
ccf_a:	ld	a,(fin)
	ld	c,a
	ld	a,(av)
	ld	b,a
	di
	push	bc
	pop	af
	ccf			; under test
	push	af		; capture
	pop	bc
	ei
	ld	a,b
	call	bput
	ld	a,c
	call	bput
	ld	hl,av
	inc	(hl)
	jp	nz,ccf_a
	ld	hl,fi
	inc	(hl)
	ld	a,(hl)
	cp	64
	jp	nz,ccf_fi
	call	fclose
	ld	hl,crc_ccf
	call	crcsave

; ---- summary
	ld	de,fcb_txt
	call	fopen
	ld	de,n_daa
	ld	hl,crc_daa
	call	sumline
	ld	de,n_cpl
	ld	hl,crc_cpl
	call	sumline
	ld	de,n_scf
	ld	hl,crc_scf
	call	sumline
	ld	de,n_ccf
	ld	hl,crc_ccf
	call	sumline
	call	fclose
	jp	0

; append a to the open file and to the running CRC-32
bput:	push	af
	push	de
	push	hl
	call	fputc
	ld	hl,crc
	xor	(hl)		; index = byte xor crc[0]
	ld	e,a
	ld	d,0
	ld	hl,ct0
	add	hl,de
	ld	a,(crc+1)
	xor	(hl)
	ld	(crc),a
	ld	hl,ct1
	add	hl,de
	ld	a,(crc+2)
	xor	(hl)
	ld	(crc+1),a
	ld	hl,ct2
	add	hl,de
	ld	a,(crc+3)
	xor	(hl)
	ld	(crc+2),a
	ld	hl,ct3
	add	hl,de
	ld	a,(hl)
	ld	(crc+3),a
	pop	hl
	pop	de
	pop	af
	ret

crcini:	ld	a,0ffh
	ld	(crc),a
	ld	(crc+1),a
	ld	(crc+2),a
	ld	(crc+3),a
	ret

; store the final CRC (crc xor FFFFFFFFh) at hl, least significant byte first
crcsave: ld	a,(crc)
	xor	0ffh
	ld	(hl),a
	inc	hl
	ld	a,(crc+1)
	xor	0ffh
	ld	(hl),a
	inc	hl
	ld	a,(crc+2)
	xor	0ffh
	ld	(hl),a
	inc	hl
	ld	a,(crc+3)
	xor	0ffh
	ld	(hl),a
	ret

; write "<name> 32768 <crc>" for the name at de and the CRC at hl
sumline: call	puts
	ld	de,s_size
	call	puts
	inc	hl
	inc	hl
	inc	hl
	ld	a,(hl)
	call	hex2
	dec	hl
	ld	a,(hl)
	call	hex2
	dec	hl
	ld	a,(hl)
	call	hex2
	dec	hl
	ld	a,(hl)
	call	hex2
	call	crlf
	ret

title:	db	'DAADUMP M101 DAA/CPL/SCF/CCF dump',13,10,'$'
m_daa:	db	'Writing DAA.BIN',13,10,'$'
m_cpl:	db	'Writing CPL.BIN',13,10,'$'
m_scf:	db	'Writing SCF.BIN',13,10,'$'
m_ccf:	db	'Writing CCF.BIN',13,10,'$'
n_daa:	db	'DAA.BIN$'
n_cpl:	db	'CPL.BIN$'
n_scf:	db	'SCF.BIN$'
n_ccf:	db	'CCF.BIN$'
s_size:	db	' 32768 $'

fcb_daa:	db	0,'DAA     BIN'
	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
fcb_cpl:	db	0,'CPL     BIN'
	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
fcb_scf:	db	0,'SCF     BIN'
	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
fcb_ccf:	db	0,'CCF     BIN'
	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
fcb_txt:	db	0,'DAADUMP TXT'
	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
ftab:	db	000h,001h,002h,003h,004h,005h,006h,007h,010h,011h,012h,013h,014h,015h,016h,017h
	db	040h,041h,042h,043h,044h,045h,046h,047h,050h,051h,052h,053h,054h,055h,056h,057h
	db	080h,081h,082h,083h,084h,085h,086h,087h,090h,091h,092h,093h,094h,095h,096h,097h
	db	0c0h,0c1h,0c2h,0c3h,0c4h,0c5h,0c6h,0c7h,0d0h,0d1h,0d2h,0d3h,0d4h,0d5h,0d6h,0d7h
ct0:	db	000h,096h,02ch,0bah,019h,08fh,035h,0a3h,032h,0a4h,01eh,088h,02bh,0bdh,007h,091h
	db	064h,0f2h,048h,0deh,07dh,0ebh,051h,0c7h,056h,0c0h,07ah,0ech,04fh,0d9h,063h,0f5h
	db	0c8h,05eh,0e4h,072h,0d1h,047h,0fdh,06bh,0fah,06ch,0d6h,040h,0e3h,075h,0cfh,059h
	db	0ach,03ah,080h,016h,0b5h,023h,099h,00fh,09eh,008h,0b2h,024h,087h,011h,0abh,03dh
	db	090h,006h,0bch,02ah,089h,01fh,0a5h,033h,0a2h,034h,08eh,018h,0bbh,02dh,097h,001h
	db	0f4h,062h,0d8h,04eh,0edh,07bh,0c1h,057h,0c6h,050h,0eah,07ch,0dfh,049h,0f3h,065h
	db	058h,0ceh,074h,0e2h,041h,0d7h,06dh,0fbh,06ah,0fch,046h,0d0h,073h,0e5h,05fh,0c9h
	db	03ch,0aah,010h,086h,025h,0b3h,009h,09fh,00eh,098h,022h,0b4h,017h,081h,03bh,0adh
	db	020h,0b6h,00ch,09ah,039h,0afh,015h,083h,012h,084h,03eh,0a8h,00bh,09dh,027h,0b1h
	db	044h,0d2h,068h,0feh,05dh,0cbh,071h,0e7h,076h,0e0h,05ah,0cch,06fh,0f9h,043h,0d5h
	db	0e8h,07eh,0c4h,052h,0f1h,067h,0ddh,04bh,0dah,04ch,0f6h,060h,0c3h,055h,0efh,079h
	db	08ch,01ah,0a0h,036h,095h,003h,0b9h,02fh,0beh,028h,092h,004h,0a7h,031h,08bh,01dh
	db	0b0h,026h,09ch,00ah,0a9h,03fh,085h,013h,082h,014h,0aeh,038h,09bh,00dh,0b7h,021h
	db	0d4h,042h,0f8h,06eh,0cdh,05bh,0e1h,077h,0e6h,070h,0cah,05ch,0ffh,069h,0d3h,045h
	db	078h,0eeh,054h,0c2h,061h,0f7h,04dh,0dbh,04ah,0dch,066h,0f0h,053h,0c5h,07fh,0e9h
	db	01ch,08ah,030h,0a6h,005h,093h,029h,0bfh,02eh,0b8h,002h,094h,037h,0a1h,01bh,08dh
ct1:	db	000h,030h,061h,051h,0c4h,0f4h,0a5h,095h,088h,0b8h,0e9h,0d9h,04ch,07ch,02dh,01dh
	db	010h,020h,071h,041h,0d4h,0e4h,0b5h,085h,098h,0a8h,0f9h,0c9h,05ch,06ch,03dh,00dh
	db	020h,010h,041h,071h,0e4h,0d4h,085h,0b5h,0a8h,098h,0c9h,0f9h,06ch,05ch,00dh,03dh
	db	030h,000h,051h,061h,0f4h,0c4h,095h,0a5h,0b8h,088h,0d9h,0e9h,07ch,04ch,01dh,02dh
	db	041h,071h,020h,010h,085h,0b5h,0e4h,0d4h,0c9h,0f9h,0a8h,098h,00dh,03dh,06ch,05ch
	db	051h,061h,030h,000h,095h,0a5h,0f4h,0c4h,0d9h,0e9h,0b8h,088h,01dh,02dh,07ch,04ch
	db	061h,051h,000h,030h,0a5h,095h,0c4h,0f4h,0e9h,0d9h,088h,0b8h,02dh,01dh,04ch,07ch
	db	071h,041h,010h,020h,0b5h,085h,0d4h,0e4h,0f9h,0c9h,098h,0a8h,03dh,00dh,05ch,06ch
	db	083h,0b3h,0e2h,0d2h,047h,077h,026h,016h,00bh,03bh,06ah,05ah,0cfh,0ffh,0aeh,09eh
	db	093h,0a3h,0f2h,0c2h,057h,067h,036h,006h,01bh,02bh,07ah,04ah,0dfh,0efh,0beh,08eh
	db	0a3h,093h,0c2h,0f2h,067h,057h,006h,036h,02bh,01bh,04ah,07ah,0efh,0dfh,08eh,0beh
	db	0b3h,083h,0d2h,0e2h,077h,047h,016h,026h,03bh,00bh,05ah,06ah,0ffh,0cfh,09eh,0aeh
	db	0c2h,0f2h,0a3h,093h,006h,036h,067h,057h,04ah,07ah,02bh,01bh,08eh,0beh,0efh,0dfh
	db	0d2h,0e2h,0b3h,083h,016h,026h,077h,047h,05ah,06ah,03bh,00bh,09eh,0aeh,0ffh,0cfh
	db	0e2h,0d2h,083h,0b3h,026h,016h,047h,077h,06ah,05ah,00bh,03bh,0aeh,09eh,0cfh,0ffh
	db	0f2h,0c2h,093h,0a3h,036h,006h,057h,067h,07ah,04ah,01bh,02bh,0beh,08eh,0dfh,0efh
ct2:	db	000h,007h,00eh,009h,06dh,06ah,063h,064h,0dbh,0dch,0d5h,0d2h,0b6h,0b1h,0b8h,0bfh
	db	0b7h,0b0h,0b9h,0beh,0dah,0ddh,0d4h,0d3h,06ch,06bh,062h,065h,001h,006h,00fh,008h
	db	06eh,069h,060h,067h,003h,004h,00dh,00ah,0b5h,0b2h,0bbh,0bch,0d8h,0dfh,0d6h,0d1h
	db	0d9h,0deh,0d7h,0d0h,0b4h,0b3h,0bah,0bdh,002h,005h,00ch,00bh,06fh,068h,061h,066h
	db	0dch,0dbh,0d2h,0d5h,0b1h,0b6h,0bfh,0b8h,007h,000h,009h,00eh,06ah,06dh,064h,063h
	db	06bh,06ch,065h,062h,006h,001h,008h,00fh,0b0h,0b7h,0beh,0b9h,0ddh,0dah,0d3h,0d4h
	db	0b2h,0b5h,0bch,0bbh,0dfh,0d8h,0d1h,0d6h,069h,06eh,067h,060h,004h,003h,00ah,00dh
	db	005h,002h,00bh,00ch,068h,06fh,066h,061h,0deh,0d9h,0d0h,0d7h,0b3h,0b4h,0bdh,0bah
	db	0b8h,0bfh,0b6h,0b1h,0d5h,0d2h,0dbh,0dch,063h,064h,06dh,06ah,00eh,009h,000h,007h
	db	00fh,008h,001h,006h,062h,065h,06ch,06bh,0d4h,0d3h,0dah,0ddh,0b9h,0beh,0b7h,0b0h
	db	0d6h,0d1h,0d8h,0dfh,0bbh,0bch,0b5h,0b2h,00dh,00ah,003h,004h,060h,067h,06eh,069h
	db	061h,066h,06fh,068h,00ch,00bh,002h,005h,0bah,0bdh,0b4h,0b3h,0d7h,0d0h,0d9h,0deh
	db	064h,063h,06ah,06dh,009h,00eh,007h,000h,0bfh,0b8h,0b1h,0b6h,0d2h,0d5h,0dch,0dbh
	db	0d3h,0d4h,0ddh,0dah,0beh,0b9h,0b0h,0b7h,008h,00fh,006h,001h,065h,062h,06bh,06ch
	db	00ah,00dh,004h,003h,067h,060h,069h,06eh,0d1h,0d6h,0dfh,0d8h,0bch,0bbh,0b2h,0b5h
	db	0bdh,0bah,0b3h,0b4h,0d0h,0d7h,0deh,0d9h,066h,061h,068h,06fh,00bh,00ch,005h,002h
ct3:	db	000h,077h,0eeh,099h,007h,070h,0e9h,09eh,00eh,079h,0e0h,097h,009h,07eh,0e7h,090h
	db	01dh,06ah,0f3h,084h,01ah,06dh,0f4h,083h,013h,064h,0fdh,08ah,014h,063h,0fah,08dh
	db	03bh,04ch,0d5h,0a2h,03ch,04bh,0d2h,0a5h,035h,042h,0dbh,0ach,032h,045h,0dch,0abh
	db	026h,051h,0c8h,0bfh,021h,056h,0cfh,0b8h,028h,05fh,0c6h,0b1h,02fh,058h,0c1h,0b6h
	db	076h,001h,098h,0efh,071h,006h,09fh,0e8h,078h,00fh,096h,0e1h,07fh,008h,091h,0e6h
	db	06bh,01ch,085h,0f2h,06ch,01bh,082h,0f5h,065h,012h,08bh,0fch,062h,015h,08ch,0fbh
	db	04dh,03ah,0a3h,0d4h,04ah,03dh,0a4h,0d3h,043h,034h,0adh,0dah,044h,033h,0aah,0ddh
	db	050h,027h,0beh,0c9h,057h,020h,0b9h,0ceh,05eh,029h,0b0h,0c7h,059h,02eh,0b7h,0c0h
	db	0edh,09ah,003h,074h,0eah,09dh,004h,073h,0e3h,094h,00dh,07ah,0e4h,093h,00ah,07dh
	db	0f0h,087h,01eh,069h,0f7h,080h,019h,06eh,0feh,089h,010h,067h,0f9h,08eh,017h,060h
	db	0d6h,0a1h,038h,04fh,0d1h,0a6h,03fh,048h,0d8h,0afh,036h,041h,0dfh,0a8h,031h,046h
	db	0cbh,0bch,025h,052h,0cch,0bbh,022h,055h,0c5h,0b2h,02bh,05ch,0c2h,0b5h,02ch,05bh
	db	09bh,0ech,075h,002h,09ch,0ebh,072h,005h,095h,0e2h,07bh,00ch,092h,0e5h,07ch,00bh
	db	086h,0f1h,068h,01fh,081h,0f6h,06fh,018h,088h,0ffh,066h,011h,08fh,0f8h,061h,016h
	db	0a0h,0d7h,04eh,039h,0a7h,0d0h,049h,03eh,0aeh,0d9h,040h,037h,0a9h,0deh,047h,030h
	db	0bdh,0cah,053h,024h,0bah,0cdh,054h,023h,0b3h,0c4h,05dh,02ah,0b4h,0c3h,05ah,02dh
crc:	ds	4
crc_daa: ds	4
crc_cpl: ds	4
crc_scf: ds	4
crc_ccf: ds	4
fi:	db	0
fin:	db	0
av:	db	0
	ds	128
stack:
