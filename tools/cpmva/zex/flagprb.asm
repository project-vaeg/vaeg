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

; M101 FLAGPRB: real-machine flag probes for the uPD9002 Z80 emulation mode.
; Writes FLAGPRB.TXT and echoes it to the console. One line per probe:
;   <id> IN A=xx F=xx BC=xxxx DE=xxxx HL=xxxx OUT A=xx F=xx BC=xxxx DE=xxxx HL=xxxx [MEM=xx]
; followed by '#' lines with the expected Zilog and uPD9002 values.
;
; Rules for every probe (no exceptions):
; - no self-modifying code; each probe has its own code path;
; - F is set only through PUSH BC / POP AF, with bits 5/3 clear except P7;
; - DI before the setup, PUSH AF immediately after the instruction(s) under
;   test, then the other registers are stored, then EI;
; - no branch depends on a flag produced by an instruction under test.

	org	100h

start:	ld	sp,stack
	ld	de,fcbout
	call	fopen
	ld	de,title
	call	conmsg		; console only

; ---- P0: XOR A (control)
	di
	ld	bc,05a00h		; A=5Ah F=00h
	push	bc
	pop	af
	ld	bc,0000h
	ld	de,0000h
	ld	hl,0000h
	xor	a		; under test
	push	af		; capture
	ld	(obc),bc
	ld	(ode),de
	ld	(ohl),hl
	pop	hl
	ld	(oaf),hl
	ei
	ld	ix,d_p0
	call	report

; ---- P1: AND 0Fh
	di
	ld	bc,0ff00h		; A=FFh F=00h
	push	bc
	pop	af
	ld	bc,0000h
	ld	de,0000h
	ld	hl,0000h
	and	0fh		; under test
	push	af		; capture
	ld	(obc),bc
	ld	(ode),de
	ld	(ohl),hl
	pop	hl
	ld	(oaf),hl
	ei
	ld	ix,d_p1
	call	report

; ---- P2: BIT 0,A
	di
	ld	bc,00100h		; A=01h F=00h
	push	bc
	pop	af
	ld	bc,0000h
	ld	de,0000h
	ld	hl,0000h
	bit	0,a		; under test
	push	af		; capture
	ld	(obc),bc
	ld	(ode),de
	ld	(ohl),hl
	pop	hl
	ld	(oaf),hl
	ei
	ld	ix,d_p2
	call	report

; ---- P3: RLCA
	di
	ld	bc,080d7h		; A=80h F=D7h
	push	bc
	pop	af
	ld	bc,0000h
	ld	de,0000h
	ld	hl,0000h
	rlca		; under test
	push	af		; capture
	ld	(obc),bc
	ld	(ode),de
	ld	(ohl),hl
	pop	hl
	ld	(oaf),hl
	ei
	ld	ix,d_p3
	call	report

; ---- P4a: LDI, BC=0001h
	ld	a,0
	ld	(src),a		; (HL) = 00h
	ld	a,0ffh
	ld	(dst),a		; destination starts as FFh
	di
	ld	bc,000d7h		; A=00h F=D7h
	push	bc
	pop	af
	ld	bc,0001h
	ld	de,dst
	ld	hl,src
	ldi		; under test
	push	af		; capture
	ld	(obc),bc
	ld	(ode),de
	ld	(ohl),hl
	pop	hl
	ld	(oaf),hl
	ei
	ld	a,(dst)
	ld	(omem),a
	ld	ix,d_p4a
	call	report

; ---- P4b: LDI, BC=0002h
	ld	a,0
	ld	(src),a		; (HL) = 00h
	ld	a,0ffh
	ld	(dst),a		; destination starts as FFh
	di
	ld	bc,000d7h		; A=00h F=D7h
	push	bc
	pop	af
	ld	bc,0002h
	ld	de,dst
	ld	hl,src
	ldi		; under test
	push	af		; capture
	ld	(obc),bc
	ld	(ode),de
	ld	(ohl),hl
	pop	hl
	ld	(oaf),hl
	ei
	ld	a,(dst)
	ld	(omem),a
	ld	ix,d_p4b
	call	report

; ---- P5a: ADD HL,BC
	di
	ld	bc,00000h		; A=00h F=00h
	push	bc
	pop	af
	ld	bc,0001h
	ld	de,0000h
	ld	hl,0fffh
	add	hl,bc		; under test
	push	af		; capture
	ld	(obc),bc
	ld	(ode),de
	ld	(ohl),hl
	pop	hl
	ld	(oaf),hl
	ei
	ld	ix,d_p5a
	call	report

; ---- P5b: ADD HL,BC
	di
	ld	bc,00010h		; A=00h F=10h
	push	bc
	pop	af
	ld	bc,0001h
	ld	de,0000h
	ld	hl,0001h
	add	hl,bc		; under test
	push	af		; capture
	ld	(obc),bc
	ld	(ode),de
	ld	(ohl),hl
	pop	hl
	ld	(oaf),hl
	ei
	ld	ix,d_p5b
	call	report

; ---- P6a: ADC HL,BC
	di
	ld	bc,00000h		; A=00h F=00h
	push	bc
	pop	af
	ld	bc,0001h
	ld	de,0000h
	ld	hl,000fh
	adc	hl,bc		; under test
	push	af		; capture
	ld	(obc),bc
	ld	(ode),de
	ld	(ohl),hl
	pop	hl
	ld	(oaf),hl
	ei
	ld	ix,d_p6a
	call	report

; ---- P6b: SBC HL,BC
	di
	ld	bc,00000h		; A=00h F=00h
	push	bc
	pop	af
	ld	bc,0001h
	ld	de,0000h
	ld	hl,0010h
	sbc	hl,bc		; under test
	push	af		; capture
	ld	(obc),bc
	ld	(ode),de
	ld	(ohl),hl
	pop	hl
	ld	(oaf),hl
	ei
	ld	ix,d_p6b
	call	report

; ---- P7a: PUSH BC/POP AF/PUSH AF/POP BC
	di
	ld	bc,00000h		; A=00h F=00h
	push	bc
	pop	af
	ld	bc,0ffffh
	ld	de,0000h
	ld	hl,0000h
	push	bc		; under test
	pop	af
	push	af
	pop	bc
	push	af		; capture
	ld	(obc),bc
	ld	(ode),de
	ld	(ohl),hl
	pop	hl
	ld	(oaf),hl
	ei
	ld	ix,d_p7a
	call	report

; ---- P7b: PUSH BC/POP AF/EX AF,AF' x2/PUSH AF/POP BC
	di
	ld	bc,00000h		; A=00h F=00h
	push	bc
	pop	af
	ld	bc,0ffffh
	ld	de,0000h
	ld	hl,0000h
	push	bc		; under test
	pop	af
	ex	af,af'
	ex	af,af'
	push	af
	pop	bc
	push	af		; capture
	ld	(obc),bc
	ld	(ode),de
	ld	(ohl),hl
	pop	hl
	ld	(oaf),hl
	ei
	ld	ix,d_p7b
	call	report

; ---- P7c: PUSH BC/POP AF/PUSH AF/POP BC
	di
	ld	bc,00000h		; A=00h F=00h
	push	bc
	pop	af
	ld	bc,0028h
	ld	de,0000h
	ld	hl,0000h
	push	bc		; under test
	pop	af
	push	af
	pop	bc
	push	af		; capture
	ld	(obc),bc
	ld	(ode),de
	ld	(ohl),hl
	pop	hl
	ld	(oaf),hl
	ei
	ld	ix,d_p7c
	call	report

; ---- P7d: PUSH BC/POP AF/INC DE/PUSH AF/POP BC
	di
	ld	bc,00000h		; A=00h F=00h
	push	bc
	pop	af
	ld	bc,0ffffh
	ld	de,0000h
	ld	hl,0000h
	push	bc		; under test
	pop	af
	inc	de
	push	af
	pop	bc
	push	af		; capture
	ld	(obc),bc
	ld	(ode),de
	ld	(ohl),hl
	pop	hl
	ld	(oaf),hl
	ei
	ld	ix,d_p7d
	call	report

; expected values
	ld	de,expect
	call	puts
	call	fclose
	jp	0

; print one probe line; ix points to the probe descriptor:
;   dw id string; db A, F; dw BC, DE, HL; db has_mem
report:	ld	e,(ix+0)
	ld	d,(ix+1)
	call	puts
	ld	de,s_in
	call	puts
	ld	a,(ix+2)
	call	hex2
	ld	de,s_f
	call	puts
	ld	a,(ix+3)
	call	hex2
	ld	de,s_bc
	call	puts
	ld	l,(ix+4)
	ld	h,(ix+5)
	call	hex4
	ld	de,s_de
	call	puts
	ld	l,(ix+6)
	ld	h,(ix+7)
	call	hex4
	ld	de,s_hl
	call	puts
	ld	l,(ix+8)
	ld	h,(ix+9)
	call	hex4
	ld	de,s_out
	call	puts
	ld	a,(oaf+1)
	call	hex2
	ld	de,s_f
	call	puts
	ld	a,(oaf)
	call	hex2
	ld	de,s_bc
	call	puts
	ld	hl,(obc)
	call	hex4
	ld	de,s_de
	call	puts
	ld	hl,(ode)
	call	hex4
	ld	de,s_hl
	call	puts
	ld	hl,(ohl)
	call	hex4
	ld	a,(ix+10)
	or	a
	jp	z,repend
	ld	de,s_mem
	call	puts
	ld	a,(omem)
	call	hex2
repend:	call	crlf
	ret

title:	db	'# FLAGPRB M101 uPD9002 flag probes',13,10,'$'
s_in:	db	' IN A=$'
s_f:	db	' F=$'
s_bc:	db	' BC=$'
s_de:	db	' DE=$'
s_hl:	db	' HL=$'
s_out:	db	' OUT A=$'
s_mem:	db	' MEM=$'

id_p0:	db	'P0$'
id_p1:	db	'P1$'
id_p2:	db	'P2$'
id_p3:	db	'P3$'
id_p4a:	db	'P4a$'
id_p4b:	db	'P4b$'
id_p5a:	db	'P5a$'
id_p5b:	db	'P5b$'
id_p6a:	db	'P6a$'
id_p6b:	db	'P6b$'
id_p7a:	db	'P7a$'
id_p7b:	db	'P7b$'
id_p7c:	db	'P7c$'
id_p7d:	db	'P7d$'
d_p0:	dw	id_p0
	db	05ah,000h
	dw	0000h,0000h,0000h
	db	0
d_p1:	dw	id_p1
	db	0ffh,000h
	dw	0000h,0000h,0000h
	db	0
d_p2:	dw	id_p2
	db	001h,000h
	dw	0000h,0000h,0000h
	db	0
d_p3:	dw	id_p3
	db	080h,0d7h
	dw	0000h,0000h,0000h
	db	0
d_p4a:	dw	id_p4a
	db	000h,0d7h
	dw	0001h,dst,src
	db	1
d_p4b:	dw	id_p4b
	db	000h,0d7h
	dw	0002h,dst,src
	db	1
d_p5a:	dw	id_p5a
	db	000h,000h
	dw	0001h,0000h,0fffh
	db	0
d_p5b:	dw	id_p5b
	db	000h,010h
	dw	0001h,0000h,0001h
	db	0
d_p6a:	dw	id_p6a
	db	000h,000h
	dw	0001h,0000h,000fh
	db	0
d_p6b:	dw	id_p6b
	db	000h,000h
	dw	0001h,0000h,0010h
	db	0
d_p7a:	dw	id_p7a
	db	000h,000h
	dw	0ffffh,0000h,0000h
	db	0
d_p7b:	dw	id_p7b
	db	000h,000h
	dw	0ffffh,0000h,0000h
	db	0
d_p7c:	dw	id_p7c
	db	000h,000h
	dw	0028h,0000h,0000h
	db	0
d_p7d:	dw	id_p7d
	db	000h,000h
	dw	0ffffh,0000h,0000h
	db	0
expect:
	db	'# P0 XOR A (control): ZILOG A=00 F=44; UPD9002 A=00 F=44',13,10
	db	'# P1 AND 0Fh: ZILOG A=0F F=1C; UPD9002 A=0F F=04',13,10
	db	'# P2 BIT 0,A: ZILOG F=10; UPD9002 F=00',13,10
	db	'# P3 RLCA: ZILOG A=01 F=C5; UPD9002 A=01 F=D7',13,10
	db	'# P4a LDI, BC=0001h: ZILOG F=C1 BC=0000; UPD9002 F=D3 BC=0000',13,10
	db	'# P4b LDI, BC=0002h: ZILOG F=C5 BC=0001; UPD9002 F=D7 BC=0001',13,10
	db	'# P5a ADD HL,BC: ZILOG HL=1000 F=10; UPD9002 HL=1000 F=00',13,10
	db	'# P5b ADD HL,BC: ZILOG HL=0002 F=00; UPD9002 HL=0002 F=10',13,10
	db	'# P6a ADC HL,BC: ZILOG HL=0010 F=00; UPD9002 HL=0010 F=10',13,10
	db	'# P6b SBC HL,BC: ZILOG HL=000F F=02; UPD9002 HL=000F F=12',13,10
	db	'# P7a PUSH BC/POP AF/PUSH AF/POP BC: ZILOG C=FF; UPD9002 C=D7',13,10
	db	'# P7b PUSH BC/POP AF/EX AF,AF',39,' x2/PUSH AF/POP BC: ZILOG C=FF; UPD9002 C=D7',13,10
	db	'# P7c PUSH BC/POP AF/PUSH AF/POP BC: ZILOG C=28; UPD9002 C=00',13,10
	db	'# P7d PUSH BC/POP AF/INC DE/PUSH AF/POP BC: ZILOG C=FF; UPD9002 C=D7',13,10
	db	'$'

fcbout:	db	0,'FLAGPRB TXT'
	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
oaf:	dw	0
obc:	dw	0
ode:	dw	0
ohl:	dw	0
omem:	db	0
src:	db	0
dst:	db	0
	ds	128
stack:
