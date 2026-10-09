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
; G256.COM: a 256 x 192 window in 320 x 200 graphics on a PC-88VA under
; PC-Engine (V3 mode), written for vaeg M104; see
; tools/pc88va/vtiming/README.md. Assemble with NASM (8086/80186 only):
; nasm -f bin -o G256.COM g256.asm
;
;   G256 [16|8|4|A]
;
; The VA has no 256-dot graphics mode and the TSP horizontal active period
; does not clip graphics (measured in M104), so a 256 x 192 picture is a
; window in single-plane 320 x 200 graphics: x 32-287, y 4-195, black
; outside. 192 lines fit the native 200-line mode, so no SYNC change is
; needed: at 15.98 kHz each line is one raster, at 24.8 kHz port 0100h
; RSM = 01 shows it on two rasters. Pixels are 1:1 with GVRAM (16, 8 or 4
; bits; default 16). The window has a white one-dot border, a grey grid
; every 32 dots, one-dot red/white checks in its top-left 32 x 32 cell and
; a gradient elsewhere; line 1 carries one white dot per bit of depth
; (16, 8 or 4 dots at the left). Any key restores the screen; PC-Engine's
; INT 21h has no console output, so nothing is printed.
; A shows 16, 8 and 4 bits one after another, a key each.
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
		jmp	.bd
.b8:		mov	word [white],0ffh
		mov	word [grey],092h
		mov	word [red],01ch
		jmp	.bd
.b4:		mov	word [white],7
		mov	word [grey],15
		mov	word [red],2
.bd:		mov	[bpp],bx

		mov	ah,00h			; single plane, graphics on, G0 320 dots,
		mov	bx,0a00ah		; 200 lines
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
		mov	ax,0b01h		; graphics display on, before port 0100h
		int	8fh			; (the BIOS call rewrites it)
		mov	dx,0100h		; RSM = 01 (line doubling at 24.8 kHz)
		in	ax,dx
		mov	[saved100],ax
		and	al,3fh
		or	al,40h
		out	dx,ax
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
		call	draw
		call	label
		mov	ah,0ch			; drop pending keys, wait for one
		int	82h
		mov	ah,00h
		int	82h
		cli
		mov	al,[wmode]
		mov	dx,0580h
		out	dx,al
		mov	al,[bank]
		mov	dx,0153h
		out	dx,al
		sti
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

; draw the whole 320 x 200 screen, one GVRAM line at a time
draw:		xor	si,si			; y
.row:		mov	ax,si			; line start = base + y * linebytes
		mov	cx,[bpp]
		imul	cx,cx,40		; bytes per line = bpp * 40
		mul	cx
		add	ax,[base]
		adc	dx,[base+2]
		mov	di,ax
		and	di,000fh
		shr	ax,4
		shl	dx,12
		or	ax,dx
		mov	es,ax
		xor	bx,bx			; x
.px:		call	colour
		cmp	word [bpp],8
		je	.p8
		jb	.p4
		stosw
		inc	bx
		jmp	.nx
.p8:		stosb
		inc	bx
		jmp	.nx
.p4:		mov	cl,al			; two pixels, left in the high nibble
		inc	bx
		push	cx
		call	colour
		pop	cx
		and	al,0fh
		shl	cl,4
		or	al,cl
		stosb
		inc	bx
.nx:		cmp	bx,320
		jb	.px
		inc	si
		cmp	si,200
		jb	.row
		push	cs
		pop	es
		ret

; AX = colour of GVRAM pixel x = BX, y = SI
colour:		cmp	bx,32
		jb	.blk
		cmp	bx,288
		jae	.blk
		cmp	si,4
		jb	.blk
		cmp	si,196
		jae	.blk
		mov	cx,bx			; window coordinates CX, DX
		sub	cx,32
		mov	dx,si
		sub	dx,4
		or	cx,cx
		jz	.wht
		cmp	cx,255
		je	.wht
		or	dx,dx
		jz	.wht
		cmp	dx,191
		je	.wht
		test	cl,1fh
		jz	.gry
		test	dl,1fh
		jz	.gry
		cmp	cx,32
		jae	.grad
		cmp	dx,32
		jae	.grad
		mov	ax,cx
		xor	ax,dx
		test	al,1
		jz	.red
.wht:		mov	ax,[white]
		ret
.red:		mov	ax,[red]
		ret
.gry:		mov	ax,[grey]
		ret
.blk:		xor	ax,ax
		ret
.grad:		cmp	word [bpp],8
		je	.g8
		jb	.g4
		mov	ax,cx			; 16 bits: G6 R5 B5
		shr	ax,3			; red 0-31 across
		shl	ax,5
		mov	cx,dx
		mov	dx,0
		push	ax
		mov	ax,cx
		mov	cl,3
		div	cl			; green = y / 3 (0-63)
		xor	ah,ah
		shl	ax,10
		pop	cx
		or	ax,cx
		or	ax,10h			; some blue
		ret
.g8:		mov	ax,cx			; 8 bits: G3 R3 B2
		shr	ax,3
		and	ax,1ch			; red = x / 32
		push	ax
		mov	ax,dx
		mov	cl,24
		div	cl			; green = y / 24 (0-7)
		xor	ah,ah
		shl	ax,5
		pop	cx
		or	ax,cx
		or	ax,2
		ret
.g4:		mov	ax,cx			; 4 bits: 16 x 16 colour blocks
		shr	ax,4
		shr	dx,4
		xor	ax,dx
		and	ax,0fh
		ret

; mark the bit depth with that many white dots on line 1 (x 2, 4, 6, ...)
label:		mov	si,1
		xor	cx,cx
.d:		cmp	cx,[bpp]
		jae	.r
		mov	bx,cx
		shl	bx,1
		add	bx,2
		push	cx
		call	dot
		pop	cx
		inc	cx
		jmp	.d
.r:		ret
; white dot at x = BX, y = SI
dot:		mov	ax,si
		mov	cx,[bpp]
		imul	cx,cx,40
		mul	cx
		push	dx
		push	ax
		mov	ax,bx
		cmp	word [bpp],8
		je	.a8
		jb	.a4
		shl	ax,1
		jmp	.aa
.a4:		shr	ax,1
.a8:
.aa:		pop	cx
		pop	dx
		add	ax,cx
		adc	dx,0
		add	ax,[base]
		adc	dx,[base+2]
		mov	di,ax
		and	di,000fh
		shr	ax,4
		shl	dx,12
		or	ax,dx
		mov	es,ax
		mov	ax,[white]
		cmp	word [bpp],8
		je	.w8
		jb	.w4
		stosw
		jmp	.e
.w8:		stosb
		jmp	.e
.w4:		mov	ah,[es:di]
		test	bl,1
		jnz	.lo
		and	ah,0fh
		shl	al,4
		or	al,ah
		stosb
		jmp	.e
.lo:		and	ah,0f0h
		or	al,ah
		stosb
.e:		push	cs
		pop	es
		ret

		align	2
sweepp		dw	0
depths		dw	16, 8, 4, 0
datastart:
desc		dw	16, 320, 200
bpp		dw	16
base		dw	0, 0
saved100	dw	0
white		dw	0ffffh
grey		dw	08210h
red		dw	03e0h
bank		db	0
wmode		db	0
dataend:
datasave:
