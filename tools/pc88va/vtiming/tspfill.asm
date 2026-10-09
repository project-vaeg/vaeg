; Copyright (c) 2026 Nakata Maho
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
; 1. Redistributions of source code must retain the above copyright
;    notice, this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in the
;    documentation and/or other materials provided with the distribution.
;
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
; IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
; WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
; DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
; INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
; (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
; SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
; HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
; STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
; IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
; POSSIBILITY OF SUCH DAMAGE.
;
;
; TSPFILL.COM: fill TVRAM with known data under an undocumented TSP
; screen-table MODE, on a PC-88VA under PC-Engine (V3 mode), written for
; vaeg M104; see tools/pc88va/vtiming/README.md. Assemble with NASM
; (8086/80186 only): nasm -f bin -o TSPFILL.COM tspfill.asm
;
;   TSPFILL [mode]
;   TSPFILL A
;
; MODE values 8-14 (and 24-30) show a non-character pattern over the whole
; main split (TSPMODE, M104). To find what the TSP reads in those modes,
; TSPFILL sets split 0's MODE to mode (default 8) and fills the TVRAM text
; area from split 0's start address up to the screen table (7F00h), and
; the same range of the attribute area (+8000h), with known data. Each key
; steps to the next fill, ESC stops; "MODE nn Pn" is shown at the left of
; the function-key row. Fills, for a byte at TVRAM offset o (character and
; attribute areas alike unless noted):
;   P0 00h          P1 FFh
;   P2 characters FFh, attributes 00h
;   P3 characters 00h, attributes FFh
;   P4 o and FFh                    (byte ramp)
;   P5 (o shr 4) and FFh            (changes every 16 bytes)
;   P6 (o shr 8) and FFh            (changes every 256 bytes)
;   P7 nibbles 0,1,2,...,F repeated (high nibble first)
; The whole filled range and the MODE word are saved in a 64 KiB block
; (INT 21h function 48h) and restored on exit. Timing is not changed.
; A runs unattended for filming: MODE 8, 12, 14, 10 and 13 in turn, each
; with P0-P7 held for about 3 seconds (170 frames), then restores and
; exits by itself; any key stops it early.
		cpu	186
		org	0100h
TABLE		equ	7f00h		; screen table (TVRAM offset, PC-Engine)
NFILL		equ	8
start:		mov	sp,0fffeh
		mov	ah,4ah			; keep 64 KiB for this program
		push	cs
		pop	es
		mov	bx,1000h
		int	21h
		mov	ah,48h			; and get 64 KiB for the copy
		mov	bx,1000h
		int	21h
		jnc	.mem
		mov	ax,4c01h
		int	21h
.mem:		mov	[saveseg],ax
		mov	si,0081h
.blank:		lodsb
		cmp	al,' '
		je	.blank
		or	al,20h
		cmp	al,'a'
		jne	.num
		mov	byte [auto],1
.num:		mov	si,0081h
		xor	bx,bx
		mov	cl,0
.arg:		lodsb
		cmp	al,' '
		je	.arg
		cmp	al,'0'
		jb	.argd
		cmp	al,'9'
		ja	.argd
		mov	cl,1
		sub	al,'0'
		xor	ah,ah
		xchg	ax,bx
		mov	dx,10
		mul	dx
		add	bx,ax
		jmp	.arg
.argd:		or	cl,cl
		jnz	.given
		mov	bx,8
.given:		and	bx,1fh
		mov	[mode],bl

		cli
		mov	dx,0153h		; system memory area: TVRAM
		in	al,dx
		mov	[bank],al
		and	al,0f0h
		or	al,01h
		out	dx,al
		sti
		mov	ax,0a000h
		mov	es,ax
		mov	ax,[es:TABLE+10h]	; split 0 start
		mov	[rsa0],ax
		mov	ax,[es:TABLE+30h]	; split 1 start (function-key row)
		mov	[rsa1],ax
		mov	ax,[es:TABLE+0ah]
		mov	[save0],ax
		mov	cx,TABLE		; bytes to fill in each area
		sub	cx,[rsa0]
		jbe	restore_all.quit
		mov	[len],cx

		push	ds			; save characters, then attributes
		mov	si,[rsa0]
		mov	ax,[saveseg]
		mov	di,0
		push	es
		pop	ds
		mov	es,ax
		cld
		rep	movsb
		mov	cx,[cs:len]
		mov	si,[cs:rsa0]
		add	si,8000h
		rep	movsb
		pop	ds
		mov	ax,0a000h
		mov	es,ax

		cmp	byte [auto],0
		jne	autorun
		mov	ax,[save0]
		and	al,0e0h
		or	al,[mode]
		mov	[es:TABLE+0ah],ax
		mov	byte [fill],0
.step:		call	dofill
		call	label
		call	settle
		mov	ah,00h
		int	82h
		cmp	al,1bh
		je	.done
		inc	byte [fill]
		cmp	byte [fill],NFILL
		jb	.step
		mov	byte [fill],0
		jmp	.step

.done:
restore_all:	mov	ax,[save0]
		mov	[es:TABLE+0ah],ax
		push	ds			; restore both areas
		mov	cx,[len]
		mov	di,[rsa0]
		mov	ax,[saveseg]
		mov	ds,ax
		xor	si,si
		cld
		rep	movsb
		mov	cx,[cs:len]
		mov	di,[cs:rsa0]
		add	di,8000h
		rep	movsb
		pop	ds
.quit:		cli
		mov	al,[bank]
		mov	dx,0153h
		out	dx,al
		sti
		mov	es,[saveseg]
		mov	ah,49h
		int	21h
		mov	ax,4c00h
		int	21h

; A: every mode in automodes, P0-P7 each held for HOLD frames
HOLD		equ	170
autorun:	mov	si,automodes
.m:		lodsb
		or	al,al
		jz	.end
		mov	[mode],al
		push	si
		mov	ax,[save0]
		and	al,0e0h
		or	al,[mode]
		mov	[es:TABLE+0ah],ax
		mov	byte [fill],0
.f:		call	dofill
		call	label
		mov	cx,HOLD
		call	frames
		jc	.stop
		inc	byte [fill]
		cmp	byte [fill],NFILL
		jb	.f
		pop	si
		jmp	.m
.stop:		pop	si
.end:		mov	ah,0ch			; drop the key that stopped it
		int	82h
		jmp	restore_all

; wait CX frames (VRTC rising edges); CY = 1 if a key was pressed
frames:
.f:		mov	dx,0040h
.lo:		in	al,dx
		test	al,20h
		jnz	.lo
.hi:		in	al,dx
		test	al,20h
		jz	.hi
		push	cx
		mov	ah,01h
		int	82h
		pop	cx
		jnc	.key
		loop	.f
		clc
		ret
.key:		stc
		ret

; fill both areas with pattern [fill]
dofill:		mov	di,[rsa0]
		mov	cx,[len]
		call	fillarea
		mov	di,[rsa0]
		add	di,8000h
		mov	cx,[len]
		mov	byte [attr],1
		call	fillarea
		mov	byte [attr],0
		ret
; fill CX bytes at ES:DI; the value depends on the offset within the area
fillarea:	mov	bx,di
		and	bx,7fffh		; o: offset in the character area
.b:		mov	al,[fill]
		cmp	al,0
		jne	.p1
		xor	al,al
		jmp	.st
.p1:		cmp	al,1
		jne	.p2
		mov	al,0ffh
		jmp	.st
.p2:		cmp	al,2
		jne	.p3
		mov	al,[attr]
		dec	al			; characters FFh, attributes 00h
		jmp	.st
.p3:		cmp	al,3
		jne	.p4
		mov	al,[attr]
		neg	al			; characters 00h, attributes FFh
		jmp	.st
.p4:		cmp	al,4
		jne	.p5
		mov	al,bl
		jmp	.st
.p5:		cmp	al,5
		jne	.p6
		mov	ax,bx
		shr	ax,4
		jmp	.st
.p6:		cmp	al,6
		jne	.p7
		mov	al,bh
		jmp	.st
.p7:		mov	al,bl			; nibbles 2o, 2o+1
		shl	al,1
		and	al,0eh
		mov	ah,al
		shl	ah,4
		inc	al
		or	al,ah
.st:		stosb
		inc	bx
		loop	.b
		ret

; wait about 20 frames (VRTC rising edges, port 40h bit 5), then clear
; the keyboard queue so a held key steps only once
settle:		mov	cx,20
.f:		mov	dx,0040h
.lo:		in	al,dx
		test	al,20h
		jnz	.lo
.hi:		in	al,dx
		test	al,20h
		jz	.hi
		loop	.f
		mov	ah,0ch
		int	82h
		ret

; "MODE nn Pn" in the function-key row, white, mode-1 attribute
label:		mov	al,[mode]
		xor	ah,ah
		mov	bl,10
		div	bl
		add	ax,3030h
		mov	[labtxt+5],al
		mov	[labtxt+6],ah
		mov	al,[fill]
		add	al,'0'
		mov	[labtxt+9],al
		mov	si,[rsa1]
		mov	bx,labtxt
		mov	cx,LABLEN
.l:		mov	al,[bx]
		xor	ah,ah
		mov	[es:si],ax
		mov	word [es:si+8000h],00f0h
		inc	bx
		add	si,2
		loop	.l
		ret

labtxt		db	'MODE nn Pn  '
LABLEN		equ	$-labtxt
mode		db	0
bank		db	0
fill		db	0
attr		db	0
auto		db	0
automodes	db	8, 12, 14, 10, 13, 0
		align	2
rsa0		dw	0
rsa1		dw	0
save0		dw	0
len		dw	0
saveseg		dw	0
