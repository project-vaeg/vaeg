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
; G160.COM: a 160 x 100 picture with a colour per dot on a PC-88VA under
; PC-Engine (V3 mode), written for vaeg M104; see
; tools/pc88va/vtiming/README.md. Assemble with NASM (8086/80186 only):
; nasm -f bin -o G160.COM g160.asm
;
;   G160 [16|8|4|A]
;
; Graphics screen 0 becomes single-plane 320 x 200 with 16, 8 or 4 bits per
; pixel (default 16), 320-dot mode and 200-line mode, and port 0100h
; RSM = 01 so that at 24.8 kHz each line is shown on two rasters (measured
; on a PC-88VA2 in M104). The hardware thus enlarges 2 x 2; the CPU writes
; each picture dot as 2 x 2 pixels, so one dot is 4 x 4 screen dots.
; The SGP is not used: its BitBlt copies blocks and cannot enlarge.
; The picture is redrawn completely every frame with a moving colour
; offset, as fast as the CPU allows, until a key is pressed; then the
; number of redraws and the start and end clock values (hexadecimal bytes,
; hh:mm:ss) are shown in the function-key row
; (written to TVRAM) until a second key, and the screen is restored.
; PC-Engine's INT 21h provides only file, memory and process functions
; (technical manual chapter 7), so the time comes from the calendar clock
; BIOS (INT 8Ch function 02h, whole seconds) and the text goes to TVRAM;
; run for ten seconds or more for a usable rate. The values are shown raw
; because a computed elapsed time was wrong on the PC-88VA2. Colour formats as modelled in vaeg: 16 bits G6 R5 B5, 8 bits
; G3 R3 B2, 4 bits palette index.
; A runs 16, 8 and 4 bits one after another (two keys each).
		cpu	186
		org	0100h
start:		mov	si,0081h
		xor	bx,bx
.arg:		lodsb
		cmp	al,' '
		je	.arg
		mov	ah,al
		or	ah,20h
		cmp	ah,'a'
		je	sweep
		cmp	al,'0'
		jb	.argd
		cmp	al,'9'
		ja	.argd
		sub	al,'0'
		xor	ah,ah
		xchg	ax,bx
		mov	cx,10
		mul	cx
		add	bx,ax
		jmp	.arg
.argd:		cmp	bx,8
		je	.b8
		cmp	bx,4
		je	.b4
		mov	bx,16
		mov	word [dotbytes],4	; bytes per picture dot in a GVRAM line
		jmp	.bd
.b8:		mov	word [dotbytes],2
		mov	word [white],0ffh
		mov	word [red],1ch
		jmp	.bd
.b4:		mov	word [dotbytes],1
		mov	word [white],7
		mov	word [red],2
.bd:		mov	[bpp],bx
		mov	ax,[dotbytes]
		mov	cx,160
		mul	cx
		mov	[linebytes],ax		; bytes per GVRAM line (320 pixels)

		mov	ah,00h			; screen mode: single plane, graphics on,
		mov	bx,0a00ah		; G0 320 dots, 200 lines
		mov	cl,[bpp]
		mov	ch,cl
		xor	dx,dx
		int	8fh
		or	ax,ax
		jnz	fail
		mov	ax,[bpp]
		mov	[desc],ax
		push	cs
		pop	es
		mov	di,desc
		mov	ax,0100h		; frame buffer 0 of screen 0: 320 x 200
		mov	cx,1
		int	8fh
		or	ax,ax
		jnz	fail
		mov	ax,0700h
		mov	cl,0
		int	8fh
		or	ax,ax
		jnz	fail
		mov	ax,[es:di+6]
		mov	dx,[es:di+8]
		push	cs
		pop	es
		cmp	dx,000ah
		jae	.cpu
		add	dx,000ah		; GVRAM-relative: GVRAM starts at A0000h
.cpu:		mov	[base],ax
		mov	[base+2],dx
		mov	ax,0b01h		; graphics display on
		int	8fh
		mov	dx,0100h		; RSM = 01: line doubling at 24.8 kHz
		in	ax,dx
		mov	[saved100],ax
		and	al,3fh
		or	al,40h
		out	dx,ax

		mov	ah,0ch			; drop keys typed so far
		int	82h
		mov	ah,02h			; start time: calendar clock BIOS
		int	8ch
		mov	[t0],cx
		mov	[t0+2],dx
		cli
		mov	dx,0153h		; system memory area: GVRAM
		in	al,dx
		mov	[bank],al
		and	al,0f0h
		or	al,04h
		out	dx,al
		mov	dx,0580h		; single-plane write mode: CPU data
		in	al,dx
		mov	[wmode],al
		mov	al,10h
		out	dx,al
		sti

.frame:		call	draw
		inc	word [frames]
		inc	byte [phase]
		mov	ah,01h			; key waiting? (CY = 1: none)
		int	82h
		jc	.frame
		mov	ah,00h			; take it
		int	82h

		mov	ah,02h
		int	8ch
		mov	[t1],cx
		mov	[t1+2],dx
		cli
		mov	al,[wmode]
		mov	dx,0580h
		out	dx,al
		mov	al,[bank]
		mov	dx,0153h
		out	dx,al
		sti
		call	report			; build the result line
		call	show			; show it, wait for a key, restore TVRAM
		mov	ax,[saved100]
		mov	dx,0100h
		out	dx,ax
		mov	ax,0b00h		; graphics display off
		int	8fh
		cmp	word [sweepp],0
		jne	sweep.next
		mov	ax,4c00h
		int	21h
fail:		mov	ax,4c01h
		int	21h

; A: run 16, 8 and 4 bits in turn, each with fresh variables
sweep:		push	cs
		pop	es
		cld
		mov	si,datastart
		mov	di,datasave
		mov	cx,dataend-datastart
		rep	movsb
		mov	word [sweepp],depths
.next:		mov	si,[sweepp]
		lodsw
		or	ax,ax
		jz	.quit
		mov	[sweepp],si
		push	ax
		push	cs
		pop	es
		cld
		mov	si,datasave
		mov	di,datastart
		mov	cx,dataend-datastart
		rep	movsb
		pop	bx
		jmp	start.argd
.quit:		mov	ax,4c00h
		int	21h

; draw the whole picture once
draw:		xor	si,si			; picture row y (0-99)
.row:		mov	ax,si			; GVRAM line 2y: linear = base + 2y * linebytes
		shl	ax,1
		mul	word [linebytes]
		add	ax,[base]
		adc	dx,[base+2]
		mov	di,ax
		and	di,000fh
		shr	ax,4
		shl	dx,12
		or	ax,dx
		mov	es,ax
		mov	[rowseg],ax
		mov	[rowoff],di
		xor	bx,bx			; picture column x (0-159)
.dot:		call	colour			; AX = colour of dot (BX, SI)
		cmp	word [dotbytes],2
		je	.w8
		jb	.w4
		stosw				; 16 bits: two pixels
		stosw
		jmp	.nx
.w8:		mov	ah,al			; 8 bits: two pixels
		stosw
		jmp	.nx
.w4:		and	al,0fh			; 4 bits: two pixels in one byte
		mov	ah,al
		shl	ah,4
		or	al,ah
		stosb
.nx:		inc	bx
		cmp	bx,160
		jb	.dot
		; copy GVRAM line 2y to line 2y + 1
		push	ds
		push	si
		mov	cx,[linebytes]
		mov	si,[rowoff]
		mov	ax,[rowseg]
		mov	ds,ax
		shr	cx,1
		rep	movsw
		pop	si
		pop	ds
		inc	si
		cmp	si,100
		jb	.row
		push	cs
		pop	es
		ret

; AX = colour of picture dot x = BX, y = SI, for the current phase.
; A diagonal gradient that moves with the phase, and a 32 x 32 block of
; single-dot checks at the top left (alternating two colours) so that the
; per-dot resolution is visible.
colour:		cmp	bx,32
		jae	.grad
		cmp	si,32
		jae	.grad
		mov	ax,bx
		xor	ax,si
		test	al,1
		jz	.c0
		mov	ax,[white]
		ret
.c0:		mov	ax,[red]
		ret
.grad:		cmp	word [dotbytes],2
		je	.g8
		jb	.g4
		mov	ax,bx			; 16 bits: G6 R5 B5
		add	al,[phase]
		shr	ax,3			; red 0-31 across x
		and	ax,1fh
		shl	ax,5
		mov	dx,si
		add	dl,[phase]
		shl	dx,10			; green from y (6 bits)
		and	dx,0fc00h
		or	ax,dx
		mov	dx,bx
		add	dx,si
		shr	dx,3
		and	dx,1fh			; blue from x + y
		or	ax,dx
		ret
.g8:		mov	ax,bx			; 8 bits: G3 R3 B2
		add	al,[phase]
		shr	ax,2
		and	ax,1ch			; red 3 bits
		mov	dx,si
		add	dl,[phase]
		shl	dx,2
		and	dx,0e0h			; green 3 bits
		or	ax,dx
		mov	dx,bx
		add	dx,si
		shr	dx,5
		and	dx,3			; blue 2 bits
		or	ax,dx
		ret
.g4:		mov	ax,bx			; 4 bits: palette index bands
		add	ax,si
		add	al,[phase]
		shr	ax,3
		and	ax,0fh
		ret

; build "G160 nn-bit N redraws S.CC s" in outbuf
report:		mov	di,outbuf
		mov	si,msg1
		call	puts
		mov	ax,[bpp]
		call	pdec
		mov	si,msg2
		call	puts
		mov	ax,[frames]
		call	pdec
		mov	si,msg3
		call	puts
		mov	si,t0			; raw start and end clock values,
		call	ptime			; as returned by INT 8Ch function 02h
		mov	al,'-'
		stosb
		mov	si,t1
		call	ptime
		mov	[outend],di
		ret
; "hh:mm:ss" from a saved CX:DX pair at SI (CH hour, CL minute, DH second),
; each byte as two hexadecimal digits so that BCD and binary are both visible
ptime:		mov	al,[si+1]
		call	phex
		mov	al,':'
		stosb
		mov	al,[si]
		call	phex
		mov	al,':'
		stosb
		mov	al,[si+3]
		jmp	phex
phex:		push	ax
		shr	al,4
		call	.n
		pop	ax
		and	al,0fh
.n:		add	al,'0'
		cmp	al,'9'
		jbe	.s
		add	al,7
.s:		stosb
		ret
; copy the zero-terminated string at SI to DI
puts:		lodsb
		or	al,al
		jz	.r
		stosb
		jmp	puts
.r:		ret
; AX in decimal at DI
pdec:		mov	bx,10
		xor	cx,cx
.d:		xor	dx,dx
		div	bx
		push	dx
		inc	cx
		or	ax,ax
		jnz	.d
.p:		pop	ax
		add	al,'0'
		stosb
		loop	.p
		ret
; write outbuf into the function-key row (screen split 1), wait for a key,
; restore the cells
show:		cli
		mov	dx,0153h		; system memory area: TVRAM
		in	al,dx
		mov	[bank],al
		and	al,0f0h
		or	al,01h
		out	dx,al
		sti
		mov	ax,0a000h
		mov	es,ax
		mov	si,[es:7f30h]		; split 1 source address
		mov	[rsa1],si
		mov	bx,outbuf
		mov	di,savebuf
.w:		cmp	bx,outbuf+40
		jae	.wd
		mov	ax,[es:si]
		mov	[di],ax
		mov	ax,[es:si+8000h]
		mov	[di+2],ax
		add	di,4
		xor	ax,ax
		mov	al,' '
		cmp	bx,[outend]
		jae	.sp
		mov	al,[bx]
.sp:		mov	[es:si],ax
		mov	word [es:si+8000h],00f0h
		inc	bx
		add	si,2
		jmp	.w
.wd:		mov	ah,0ch
		int	82h
		mov	ah,00h
		int	82h
		mov	si,[rsa1]
		mov	di,savebuf
		mov	cx,40
.r:		mov	ax,[di]
		mov	[es:si],ax
		mov	ax,[di+2]
		mov	[es:si+8000h],ax
		add	di,4
		add	si,2
		loop	.r
		cli
		mov	al,[bank]
		mov	dx,0153h
		out	dx,al
		sti
		push	cs
		pop	es
		ret

msg1		db	'G160 ',0
msg2		db	'-bit ',0
msg3		db	' redraws ',0
		align	2
sweepp		dw	0
depths		dw	16, 8, 4, 0
datastart:
desc		dw	16, 320, 200
bpp		dw	16
dotbytes	dw	4
linebytes	dw	640
base		dw	0, 0
rowseg		dw	0
rowoff		dw	0
frames		dw	0
saved100	dw	0
t0		dw	0, 0
t1		dw	0, 0
white		dw	0ffffh
red		dw	03e0h
phase		db	0
bank		db	0
wmode		db	0
		align	2
rsa1		dw	0
outend		dw	0
dataend:
outbuf		times	48 db 0
datasave	times	dataend-datastart db 0
savebuf:
