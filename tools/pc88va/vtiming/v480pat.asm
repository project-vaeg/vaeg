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
; V480PAT.COM: PC-88VA display-timing test pattern for PC-Engine (V3 mode),
; written for vaeg M104; see tools/pc88va/vtiming/README.md. Assemble with
; NASM (8086/80186 code only): nasm -f bin -o V480PAT.COM v480pat.asm
;
;   V480PAT [lines] [S|T|U|R|D|I] [Hnn] [W] [N] [K] [Z] [Q|P]
;   V480PAT A|B|C|E|F
; lines: 1-480, default 480. Without S/T/U/D the pattern is left on the
; normal screen (400 lines visible), for VIEW480 or a capture.
; S: switch a 24.8 kHz display to N lines (VIEW480's SYNC; the frame grows
;    or shrinks with N).
; T: switch a 15.98 kHz display to N lines with the uPD72022 data-book
;    minimums for sprites: top 32, bottom >= 2, sync 4; the bottom grows to
;    keep a frame of at least 262 lines (240 lines: 278 lines, about
;    57.5 Hz). N 4-256, rounded down to even.
; U: as T without sprite headroom: top 16 (240 lines: 262 lines, 61 Hz).
; R: as T with top 37, the ROM's 15.98 kHz value: graphics then show all
;    lines (224 lines: 267-line frame, 59.9 Hz; 240: 283 lines, 56.5 Hz).
;    Run T and U with the monitor switch at 15 kHz.
; Hnn: as R with top blanking nn (1-63), to try values above 37.
; I: as R with the ROM's 15.73 kHz vector (C1 47 1C 00 9F 00 12 11 24 ...,
;    top 36, external sync) and port 100h RSM = 10 (interlaced mode 0),
;    the pairing the ROM uses for that family; Hnn after I changes the top.
; D: 320 x N (N 1-240) shown line-doubled on a 24.8 kHz display: graphics in
;    200-line mode with port 100h RSM = 01 (non-interlaced mode 1: each line
;    on an even/odd raster pair), the frame set to 2N rasters; above 400
;    rasters bottom 2 and sync 4 (480 rasters: about 48.6 Hz).
; Z: with D, RSM = 00 (non-interlaced mode 0: odd rasters blank) instead.
; Q: with S or D, bottom blanking 1 and sync 1 at any line count (as above
;    400 lines); P: with S or D, bottom blanking 2 and sync 4 (data-book
;    minimums).
;    With K they shorten the 24.8 kHz frame towards 60 Hz.
; W: 320 dots instead of 640 (with none, S, T or U; D is always 320).
; N: with S, T, U or D, narrow the TSP horizontal active period to 128 TCK
;    (HAD 159 -> 127; 256 dots at 320, 512 at 640) and add 16 TCK to each of
;    the left and right borders, keeping the line length. White marks show
;    the edges of that window (x = width/10 and width - width/10 - 1).
; K: with S or D, top blanking 17 instead of the ROM's 25 (480 lines with
;    S: 499-line frame, about 49.8 Hz).
; Any key restores the screen.
; A: sweep: run the 24.8 kHz list (sweep24 below) one entry at a time; B:
;    the same with the 15.98 kHz list (sweep15); C: the 15 kHz list of top
;    blankings above 37 and the 15.73 kHz family (sweepc); E: the 24.8 kHz
;    list of RSM = 00 doubling (sweepe); F: the 24.8 kHz list of frames
;    near 60 Hz (sweepf); G: the 24.8 kHz list of line-doubled 320-dot
;    frames near 60 Hz (sweepg); J: the 15 kHz list of 320-dot frames near
;    60 Hz (sweepj). Each entry is run as if
;    typed after V480PAT (its label shows it); any key goes on to the next
;    entry, ESC stops.
; Set up with the graphics BIOS (INT 8Fh); drawn directly into GVRAM with
; port 153h selecting GVRAM and port 580h in CPU-data write mode.
; Pattern: left half 40-line colour bands (1..14); right half sixteen
; colour bars (0..15); white lines every 100 lines; red lines at the
; 400-line edge (D: the 200-line edge); a white line at the last line.
; Label: the command line ("V480PAT" and its arguments, upper case) in white
; at the top left, 2x2-dot pixels on black, so photographs identify the run.
; Ruler: from y = 192 to the larger of the line count and 264, a bar at the
; left edge whose length is (y mod 8 + 1) steps and whose colour gives
; y div 8 (README.md has the table). It is drawn below the last line too,
; and GVRAM is cleared that far, so the last line the display reads can be
; read off the screen.
		cpu	186
		org	0100h
start:		mov	si,0081h
.blank:		lodsb
		cmp	al,' '
		je	.blank
		or	al,20h
		mov	bx,sweep24
		cmp	al,'a'
		je	sweep
		mov	bx,sweep15
		cmp	al,'b'
		je	sweep
		mov	bx,sweepc
		cmp	al,'c'
		je	sweep
		mov	bx,sweepe
		cmp	al,'e'
		je	sweep
		mov	bx,sweepf
		cmp	al,'f'
		je	sweep
		mov	bx,sweepg
		cmp	al,'g'
		je	sweep
		mov	bx,sweepj
		cmp	al,'j'
		je	sweep
parse:		mov	si,0081h
		xor	bx,bx
		xor	bp,bp
.skip:		lodsb
		cmp	al,' '
		je	.skip
		cmp	al,0dh
		je	.args
		cmp	al,'0'
		jb	.flag
		cmp	al,'9'
		ja	.flag
		sub	al,'0'
		xor	ah,ah
		xchg	ax,bx
		mov	cx,10
		mul	cx
		add	bx,ax
		jmp	.skip
.flag:		or	al,20h
		cmp	al,'s'
		jne	.t
		mov	bp,1
		jmp	.skip
.t:		cmp	al,'t'
		jne	.d
		mov	bp,2
		jmp	.skip
.d:		cmp	al,'d'
		jne	.u
		mov	bp,3
		jmp	.skip
.u:		cmp	al,'u'
		jne	.r
		mov	bp,2
		mov	byte [toplines],16
		jmp	.skip
.r:		cmp	al,'r'
		jne	.w
		mov	bp,2
		mov	byte [toplines],37
		jmp	.skip
.w:		cmp	al,'w'
		jne	.n
		mov	byte [narrow],1
		jmp	.skip
.n:		cmp	al,'n'
		jne	.k
		mov	byte [hnarrow],1
		jmp	.skip
.k:		cmp	al,'k'
		jne	.h
		mov	byte [ktop],1
		jmp	.skip
.h:		cmp	al,'h'			; Hnn: 15 kHz top blanking nn
		jne	.i
		mov	bp,2
		xor	cx,cx
.hd:		lodsb
		cmp	al,'0'
		jb	.he
		cmp	al,'9'
		ja	.he
		sub	al,'0'
		xchg	ax,cx
		mov	dl,10
		mul	dl
		add	al,cl
		mov	cx,ax
		jmp	.hd
.he:		dec	si
		and	cl,3fh
		mov	[toplines],cl
		jmp	.skip
.i:		cmp	al,'i'			; 15.73 kHz family
		jne	.z
		mov	bp,2
		mov	byte [interl],1
		mov	byte [toplines],36
		jmp	.skip
.z:		cmp	al,'z'			; D with RSM = 00
		jne	.q
		mov	byte [rsmzero],1
		jmp	.skip
.q:		cmp	al,'q'			; S: bottom 1, sync 1
		jne	.p
		mov	byte [shortv],1
		jmp	.skip
.p:		cmp	al,'p'			; S: bottom 2, sync 4
		jne	.skip
		mov	byte [shortv],2
		jmp	.skip
.args:		or	bx,bx
		jnz	.haven
		mov	bx,480
.haven:		cmp	bx,480
		ja	fail
		cmp	bp,2
		jne	.nmok
		and	bx,0fffeh		; VAD must be even
		cmp	bx,4
		jb	fail
		cmp	bx,256
		ja	fail
		mov	byte [gmode],02h	; T/U: 200-line (one raster per line)
.nmok:		cmp	bp,3
		jne	.nmok2
		cmp	bx,240
		ja	fail
		mov	byte [gmode],0ah	; D: 320 dots, 200-line (doubled)
		mov	word [scrw],320
		mov	word [lnbytes],160
		mov	word [edge],200
.nmok2:		cmp	byte [narrow],0
		je	.nmok3
		or	byte [gmode],08h	; W: graphics screen 0 at 320 dots
		mov	word [scrw],320
		mov	word [lnbytes],160
.nmok3:		mov	[lines],bx
		mov	[sflag],bp
		mov	ax,bx			; clear and ruler depth: lines, at least 264
		cmp	ax,264
		jae	.cl
		mov	ax,264
.cl:		mov	[clearlines],ax
		mov	[cliplim],ax
		mov	ax,[scrw]		; ruler step: 8 dots at 640, 4 at 320
		mov	bl,80
		div	bl
		xor	ah,ah
		mov	[rstep],ax

		mov	ah,00h			; screen mode: single plane, graphics on,
		mov	bh,0a0h			; 640 dots
		mov	bl,[gmode]		; 400 lines, or 200 lines for T
		mov	cx,0404h		; 4 bits/pixel
		xor	dx,dx
		int	8fh
		or	ax,ax
		jnz	fail
		mov	ax,[lines]
		mov	[desc+4],ax
		mov	ax,[scrw]
		mov	[desc+2],ax
		push	cs
		pop	es
		mov	di,desc
		mov	ax,0100h		; frame buffer 0 of screen 0
		mov	cx,1
		int	8fh
		or	ax,ax
		jnz	fail
		mov	ax,0700h		; its address
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
.cpu:		mov	[base],ax		; linear address of the frame buffer
		mov	[base+2],dx
		; it must lie inside the A0000h-DFFFFh GVRAM window
		mov	ax,[clearlines]
		mov	bx,[lnbytes]
		mul	bx
		add	ax,[base]
		adc	dx,[base+2]
		cmp	dx,000eh
		ja	fail
		jb	.fits
		or	ax,ax
		jnz	fail
.fits:		cli
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
		mov	byte [colour],0		; clear, below the last line too
		xor	si,si
		mov	ax,[clearlines]
		dec	ax
		xor	cx,cx
		mov	dx,[scrw]
		dec	dx
		call	box
		mov	ax,[lines]
		mov	[cliplim],ax
		; left half: 40-line bands
		xor	si,si			; y
		mov	byte [colour],1
.band:		cmp	si,[lines]
		jae	.bars
		mov	ax,si
		add	ax,39
		mov	cx,0
		mov	dx,[scrw]
		shr	dx,1
		dec	dx
		call	box
		add	si,40
		inc	byte [colour]
		cmp	byte [colour],15
		jb	.band
		mov	byte [colour],1
		jmp	.band
		; right half: sixteen vertical bars
.bars:		mov	byte [colour],0
		mov	cx,[scrw]
		shr	cx,1
.bar:		mov	dx,[scrw]		; bar width = width / 32
		shr	dx,5
		mov	[barw],dx
		add	dx,cx
		dec	dx
		xor	si,si
		mov	ax,[lines]
		dec	ax
		push	cx
		call	box
		pop	cx
		add	cx,[barw]
		inc	byte [colour]
		cmp	byte [colour],16
		jb	.bar
		; white lines every 100 lines, red at the 400-line (D: 200-line) edge
		mov	byte [colour],15
		xor	si,si
.hl:		cmp	si,[lines]
		jae	.edge
		call	hline
		add	si,100
		jmp	.hl
.edge:		mov	byte [colour],2
		mov	si,[edge]
		dec	si
		call	hline
		mov	si,[edge]
		call	hline
		mov	byte [colour],15
		mov	si,[lines]
		dec	si
		call	hline
		cmp	byte [hnarrow],0
		je	.nomark
		mov	ax,[scrw]		; window edge = width / 10, even
		xor	dx,dx
		mov	bx,10
		div	bx
		and	ax,0fffeh
		mov	[markx],ax
		mov	cx,ax			; left mark, 2 dots
		mov	dx,ax
		inc	dx
		xor	si,si
		mov	ax,[lines]
		dec	ax
		call	box
		mov	cx,[scrw]		; right mark, 2 dots ending at width-x-1
		sub	cx,[markx]
		sub	cx,2
		mov	dx,cx
		inc	dx
		xor	si,si
		mov	ax,[lines]
		dec	ax
		call	box
.nomark:
		; ruler, also below the last line
		mov	ax,[clearlines]
		mov	[cliplim],ax
		mov	si,192
.rl:		cmp	si,[clearlines]
		jae	.rd
		mov	byte [colour],0		; black background, 9 steps wide
		mov	ax,[rstep]
		mov	bx,9
		mul	bx
		mov	dx,ax
		dec	dx
		xor	cx,cx
		mov	ax,si
		call	box
		mov	bx,si			; colour from y div 8
		shr	bx,3
		and	bx,7
		mov	al,[rcol+bx]
		mov	[colour],al
		mov	ax,si			; length from y mod 8
		and	ax,7
		inc	ax
		mul	word [rstep]
		mov	dx,ax
		dec	dx
		xor	cx,cx
		mov	ax,si
		call	box
		inc	si
		jmp	.rl
.rd:
		call	label

		mov	al,[wmode]
		mov	dx,0580h
		out	dx,al
		mov	al,[bank]
		mov	dx,0153h
		out	dx,al
		sti
		mov	ax,0b01h		; graphics display on
		int	8fh

		cmp	word [sflag],0
		je	done
		mov	ah,16h
		int	84h
		cmp	word [sflag],2
		jne	.dd
		call	set_lines_15k
		jmp	.wait
.dd:		cmp	word [sflag],3
		jne	.s
		call	set_lines_dbl
		jmp	.wait
.s:		call	set_lines
.wait:
		mov	ah,0ch			; drop pending keys, wait for one
		int	82h
		mov	ah,00h
		int	82h
		mov	[lastkey],ax
		cmp	word [sflag],2
		jne	.r24
		cmp	byte [interl],0
		je	.r15
		mov	ax,[saved100]		; RSM back to its previous value
		mov	dx,0100h
		out	dx,ax
.r15:		call	restore_15k
		jmp	done
.r24:		cmp	word [sflag],3
		jne	.r24b
		mov	ax,[saved100]		; RSM back to its previous value
		mov	dx,0100h
		out	dx,ax
.r24b:		call	restore
done:		cmp	word [sweepp],0
		jne	sweep.next
		mov	ax,4c00h
		int	21h
fail:		cmp	word [sweepp],0
		jne	sweep.next
		mov	ax,4c01h
		int	21h

; sweep: BX = list of zero-terminated argument strings, ended by an empty one
sweep:		mov	[sweepp],bx
		push	cs
		pop	es
		cld
		mov	si,datastart		; keep the initial variables
		mov	di,datasave
		mov	cx,dataend-datastart
		rep	movsb
		jmp	.run
.next:		cmp	byte [lastkey],1bh	; ESC: stop
		je	.quit
.run:		mov	bx,[sweepp]
		cmp	byte [bx],0
		je	.quit
		push	cs
		pop	es
		cld
		mov	si,datasave		; fresh variables for each entry
		mov	di,datastart
		mov	cx,dataend-datastart
		rep	movsb
		mov	si,bx			; entry -> command tail at 0080h
		mov	di,0082h
		mov	byte [0081h],' '
		mov	cl,1
.cp:		lodsb
		or	al,al
		jz	.cpd
		stosb
		inc	cl
		jmp	.cp
.cpd:		mov	byte [di],0dh
		mov	[0080h],cl
		mov	[sweepp],si
		mov	word [lastkey],0
		jmp	parse
.quit:		mov	ax,4c00h
		int	21h

; horizontal white/colour line at y = SI, full width (skipped beyond N)
hline:		cmp	si,[lines]
		jae	.r
		mov	ax,si
		xor	cx,cx
		mov	dx,[scrw]
		dec	dx
		call	box
.r:		ret

; filled box x CX..DX (CX even, DX odd), y SI..AX, colour [colour]
box:		push	si
		push	ax
		mov	[x0],cx
		sub	dx,cx
		inc	dx
		shr	dx,1
		mov	[nbytes],dx
		mov	[y1],ax
		mov	al,[colour]
		mov	ah,al
		shl	ah,4
		or	al,ah
		mov	[fill],al
.row:		cmp	si,[y1]
		ja	.done
		cmp	si,[cliplim]
		jae	.done
		; linear = base + y * 320 + x0 / 2
		mov	ax,si
		mov	bx,[lnbytes]
		mul	bx
		mov	bx,[x0]
		shr	bx,1
		add	ax,bx
		adc	dx,0
		add	ax,[base]
		adc	dx,[base+2]
		mov	di,ax
		and	di,000fh
		shr	ax,4
		shl	dx,12
		or	ax,dx
		mov	es,ax
		mov	cx,[nbytes]
		mov	al,[fill]
		cld
		rep	stosb
		inc	si
		jmp	.row
.done:		push	cs
		pop	es
		pop	ax
		pop	si
		ret

; label: "V480PAT" and the command tail at the top left
label:		mov	ax,22			; clip to the drawn height
		cmp	ax,[cliplim]
		jbe	.c
		mov	ax,[cliplim]
.c:		push	word [cliplim]
		mov	[cliplim],ax
		mov	di,labtxt+7		; append the command tail, upper case
		mov	si,0081h
		xor	cx,cx
		mov	cl,[0080h]
		jcxz	.cpd
		mov	byte [di],' '
		inc	di
.cp:		lodsb
		cmp	al,0dh
		je	.cpd
		cmp	al,'a'
		jb	.up
		cmp	al,'z'
		ja	.up
		sub	al,20h
.up:		cmp	al,' '			; skip the leading blank(s)
		jne	.st
		cmp	byte [di-1],' '
		je	.nx
.st:		cmp	di,labtxt+labmax
		jae	.cpd
		mov	[di],al
		inc	di
.nx:		loop	.cp
.cpd:		mov	[labend],di
		mov	byte [colour],0		; black background
		mov	ax,di
		sub	ax,labtxt
		mov	bx,12
		mul	bx
		add	ax,7
		or	ax,1
		mov	dx,ax
		xor	cx,cx
		mov	si,2
		mov	ax,21
		call	box
		mov	byte [colour],7		; white glyphs
		mov	word [gx],4
		mov	bx,labtxt
.ch:		cmp	bx,[labend]
		jae	.done
		push	bx
		mov	al,[bx]
		call	glyph
		add	word [gx],12
		pop	bx
		inc	bx
		jmp	.ch
.done:		pop	word [cliplim]
		ret
; glyph AL at x = [gx], y = 4 (2x2-dot pixels)
glyph:		sub	al,'0'
		cmp	al,9
		jbe	.idx
		sub	al,'A'-'0'
		cmp	al,25
		ja	.r
		add	al,10
.idx:		xor	ah,ah
		mov	bx,7
		mul	bx
		add	ax,font5x7
		mov	[gptr],ax
		mov	word [gy],4
		mov	byte [grow],7
.row:		mov	bx,[gptr]
		mov	al,[bx]
		inc	word [gptr]
		mov	[gbits],al
		mov	cx,[gx]
		mov	byte [gcol],5
.col:		test	byte [gbits],10h
		jz	.skip
		push	cx
		mov	dx,cx
		inc	dx
		mov	si,[gy]
		mov	ax,si
		inc	ax
		call	box
		pop	cx
.skip:		shl	byte [gbits],1
		add	cx,2
		dec	byte [gcol]
		jnz	.col
		add	word [gy],2
		dec	byte [grow]
		jnz	.row
.r:		ret

; switch the display to [lines] lines (VIEW480's sequence)
set_lines:	mov	ax,0500h
		mov	cl,00h
		xor	bx,bx
		xor	dx,dx
		int	8fh
		mov	ax,0b01h
		int	8fh
		xor	ax,ax
		mov	dx,010ah
		out	dx,ax
		mov	ax,[lines]
		mov	dx,0206h
		out	dx,ax
		mov	dx,0212h
		out	dx,ax
		mov	byte [syncprm+10],al
		and	byte [syncprm+11],0bfh
		test	ah,01h
		jz	.lo
		or	byte [syncprm+11],40h
.lo:		cmp	word [lines],400
		jbe	.porch
		mov	byte [syncprm+12],01h
		mov	byte [syncprm+13],01h
.porch:		mov	si,syncprm
		call	ktop_si
		call	hnarrow_si
		call	shortv_si
		jmp	sync
; Q, P: bottom blanking and sync in the 24.8 kHz SYNC vector at SI
shortv_si:	cmp	byte [shortv],0
		je	.r
		mov	byte [si+12],01h	; Q: bottom 1, sync 1
		mov	byte [si+13],01h
		cmp	byte [shortv],1
		je	.r
		mov	byte [si+12],02h	; P: bottom 2, sync 4
		mov	byte [si+13],04h
.r:		ret
; K: top blanking 17 in the 24.8 kHz SYNC vector at SI
ktop_si:	cmp	byte [ktop],0
		je	.r
		mov	byte [si+8],17
.r:		ret
; 24.8 kHz, N lines doubled: frame of 2N rasters, RSM = 01
set_lines_dbl:	call	fb_lines
		mov	dx,0100h
		in	ax,dx
		mov	[saved100],ax
		and	al,3fh
		cmp	byte [rsmzero],0
		jne	.rsm
		or	al,40h			; non-interlaced mode 1
.rsm:		out	dx,ax
		mov	ax,[lines]
		shl	ax,1			; rasters
		mov	si,syncprm
		mov	[si+10],al
		and	byte [si+11],0bfh
		test	ah,01h
		jz	.lo
		or	byte [si+11],40h
.lo:		cmp	ax,400
		jbe	.porch
		mov	byte [si+12],02h	; bottom 2, sync 4: data-book minimums
		mov	byte [si+13],04h
.porch:		call	ktop_si
		call	hnarrow_si
		call	shortv_si
		jmp	sync
; 15.98 kHz: N lines in a 262-line frame
set_lines_15k:	call	fb_lines
		cmp	byte [interl],0
		je	.v
		mov	dx,0100h		; I: interlaced mode 0, 15.73 kHz vector
		in	ax,dx
		mov	[saved100],ax
		and	al,3fh
		or	al,80h
		out	dx,ax
		push	cs
		pop	es
		mov	si,sync1573
		mov	di,sync15
		mov	cx,14
		cld
		rep	movsb
.v:		mov	si,sync15
		mov	ax,[lines]
		mov	[si+10],al
		and	byte [si+11],0c0h	; BBR = 0
		test	ah,01h
		jz	.lo
		or	byte [si+11],40h
		jmp	.hi
.lo:		and	byte [si+11],0bfh
.hi:		mov	bl,[toplines]
		mov	[si+8],bl		; TBL (TBR = 0)
		mov	byte [si+13],4		; VS
		; bottom: at least 2, more to keep a 262-line frame
		mov	cx,262-4
		sub	cx,ax
		xor	bh,bh
		sub	cx,bx
		jge	.pos
		xor	cx,cx
.pos:		cmp	cx,2
		jae	.min
		mov	cx,2
.min:		cmp	cx,63
		jbe	.max
		mov	cx,63
.max:		mov	[si+12],cl
		call	hnarrow_si
		jmp	sync
; N: HAD 127, LBR and RBR + 16 in the SYNC vector at SI
hnarrow_si:	cmp	byte [hnarrow],0
		je	.r
		mov	byte [si+4],127
		add	byte [si+3],16
		add	byte [si+5],16
.r:		ret
; frame-buffer display set-up shared with VIEW480's sequence
fb_lines:	mov	ax,0500h
		mov	cl,00h
		xor	bx,bx
		xor	dx,dx
		int	8fh
		mov	ax,0b01h
		int	8fh
		xor	ax,ax
		mov	dx,010ah
		out	dx,ax
		mov	ax,[lines]
		mov	dx,0206h
		out	dx,ax
		mov	dx,0212h
		out	dx,ax
		ret
restore_15k:	mov	ax,0040h
		mov	es,ax
		mov	ax,word [es:0004h]
		mov	dx,010ah
		out	dx,ax
		mov	ax,200
		mov	dx,0206h
		out	dx,ax
		mov	dx,0212h
		out	dx,ax
		push	cs
		pop	es
		mov	si,sync15def
		call	sync
		jmp	dspon
restore:	mov	ax,0040h
		mov	es,ax
		mov	ax,word [es:0004h]
		mov	dx,010ah
		out	dx,ax
		mov	ax,400
		mov	dx,0206h
		out	dx,ax
		mov	dx,0212h
		out	dx,ax
		push	cs
		pop	es
		mov	si,syncdef
		call	sync
dspon:		cli
		mov	al,12h
		call	setcmd
		mov	al,7fh
		call	setprm
		mov	al,00h
		call	setprm
		mov	al,00h
		call	setprm
		sti
		ret
sync:		cli
		mov	al,10h
		call	setcmd
		mov	cx,14
		cld
.l:		lodsb
		call	setprm
		loop	.l
		sti
		ret
setcmd:		mov	ah,al
		mov	dx,0142h
.w:		in	al,dx
		test	al,05h
		jnz	.w
		mov	al,ah
		out	dx,al
		ret
setprm:		mov	ah,al
		mov	dx,0142h
.w:		in	al,dx
		test	al,01h
		jnz	.w
		mov	al,ah
		mov	dx,0146h
		out	dx,al
		ret

		align	2
sweepp		dw	0
lastkey		dw	0
; sweep lists: 24.8 kHz (monitor switch at 24 kHz) and 15.98 kHz
sweep24		db	'400 S',0,'408 S',0,'416 S',0,'420 S',0,'424 S',0
		db	'432 S',0,'440 S',0,'448 S',0,'456 S',0,'464 S',0
		db	'472 S',0,'476 S',0,'480 S',0,'440 S W',0
		db	'440 S N',0,'480 S W',0,'464 S K',0,'472 S K',0
		db	'480 S K',0,'200 D',0,'240 D',0,'232 D K',0
		db	'240 D K',0,0
sweep15		db	'200 R W',0,'208 R W',0,'216 R W',0,'224 R W',0
		db	'232 R W',0,'236 R W',0,'240 R W',0,'244 R W',0
		db	'248 R W',0,'224 R',0,'240 R',0,'240 R W N',0
		db	'240 T W',0,'240 U W',0,0
; C (15 kHz): top above 37, then the 15.73 kHz family
sweepc		db	'200 H37 W',0,'200 H41 W',0,'200 H45 W',0,'200 H53 W',0
		db	'224 H45 W',0,'240 H41 W',0,'240 H45 W',0
		db	'200 I W',0,'224 I W',0,'240 I W',0,'240 I W H37',0
		db	'240 I',0,0
; E (24 kHz): line doubling with RSM = 00
sweepe		db	'200 D',0,'200 D Z',0,'232 D Z',0,'240 D K Z',0,0
; F (24 kHz): frames near 60 Hz
sweepf		db	'400 S',0,'400 S Q',0,'400 S K Q',0,'400 S K P',0
		db	'400 S W K Q',0,'396 S K Q',0,'394 S K Q',0
		db	'392 S K Q',0,'392 S K P',0,'384 S K Q',0,0
; G (24 kHz): 320 x N line-doubled near 60 Hz
sweepg		db	'200 D',0,'200 D K Q',0,'200 D K P',0,'198 D K Q',0
		db	'197 D K Q',0,'196 D K Q',0,'192 D K Q',0,0
; J (15 kHz): 320 x N near 60 Hz
sweepj		db	'200 R W',0,'220 R W',0,'222 R W',0,'224 R W',0
		db	'224 H36 W',0,'224 R',0,0
		align	2
datastart:
desc		dw	4, 640, 480
base		dw	0, 0
x0		dw	0
y1		dw	0
nbytes		dw	0
fill		db	0
bank		db	0
wmode		db	0
lines		dw	480
sflag		dw	0
colour		db	0
gmode		db	0
narrow		db	0
toplines	db	32
hnarrow		db	0
ktop		db	0
interl		db	0
rsmzero		db	0
shortv		db	0
; ruler colours for y div 8 mod 8 = 0..7: white, red, yellow, green, cyan,
; blue, magenta, light grey in the default palette
rcol		db	7, 2, 6, 4, 5, 1, 3, 15
		align	2
markx		dw	0
clearlines	dw	264
cliplim		dw	264
rstep		dw	8
gx		dw	0
gy		dw	0
gptr		dw	0
labend		dw	0
grow		db	0
gcol		db	0
gbits		db	0
labmax		equ	26
labtxt		db	'V480PAT'
		times	labmax-7 db 0
; 5 x 7 font: digits then A-Z, one byte per row, bit 4 = leftmost dot
font5x7:
		db	00eh,011h,013h,015h,019h,011h,00eh	; 0
		db	004h,00ch,004h,004h,004h,004h,00eh	; 1
		db	00eh,011h,001h,002h,004h,008h,01fh	; 2
		db	01fh,002h,004h,002h,001h,011h,00eh	; 3
		db	002h,006h,00ah,012h,01fh,002h,002h	; 4
		db	01fh,010h,01eh,001h,001h,011h,00eh	; 5
		db	006h,008h,010h,01eh,011h,011h,00eh	; 6
		db	01fh,001h,002h,004h,008h,008h,008h	; 7
		db	00eh,011h,011h,00eh,011h,011h,00eh	; 8
		db	00eh,011h,011h,00fh,001h,002h,00ch	; 9
		db	00eh,011h,011h,01fh,011h,011h,011h	; A
		db	01eh,011h,011h,01eh,011h,011h,01eh	; B
		db	00eh,011h,010h,010h,010h,011h,00eh	; C
		db	01ch,012h,011h,011h,011h,012h,01ch	; D
		db	01fh,010h,010h,01eh,010h,010h,01fh	; E
		db	01fh,010h,010h,01eh,010h,010h,010h	; F
		db	00eh,011h,010h,017h,011h,011h,00fh	; G
		db	011h,011h,011h,01fh,011h,011h,011h	; H
		db	00eh,004h,004h,004h,004h,004h,00eh	; I
		db	007h,002h,002h,002h,002h,012h,00ch	; J
		db	011h,012h,014h,018h,014h,012h,011h	; K
		db	010h,010h,010h,010h,010h,010h,01fh	; L
		db	011h,01bh,015h,015h,011h,011h,011h	; M
		db	011h,011h,019h,015h,013h,011h,011h	; N
		db	00eh,011h,011h,011h,011h,011h,00eh	; O
		db	01eh,011h,011h,01eh,010h,010h,010h	; P
		db	00eh,011h,011h,011h,015h,012h,00dh	; Q
		db	01eh,011h,011h,01eh,014h,012h,011h	; R
		db	00fh,010h,010h,00eh,001h,001h,01eh	; S
		db	01fh,004h,004h,004h,004h,004h,004h	; T
		db	011h,011h,011h,011h,011h,011h,00eh	; U
		db	011h,011h,011h,011h,011h,00ah,004h	; V
		db	011h,011h,011h,015h,015h,015h,00ah	; W
		db	011h,011h,00ah,004h,00ah,011h,011h	; X
		db	011h,011h,011h,00ah,004h,004h,004h	; Y
		db	01fh,001h,002h,004h,008h,010h,01fh	; Z
		align	2
scrw		dw	640
lnbytes		dw	320
edge		dw	400
barw		dw	20
saved100	dw	0
syncprm		db	0c1h, 57h, 10h, 00h, 9fh, 00h, 10h, 0fh, 19h, 00h, 90h, 40h, 07h, 08h
; 15.98 kHz SYNC as the VA2 ROM programs it (200 lines in a 260-line frame)
sync15		db	0c1h, 57h, 1ch, 00h, 9fh, 00h, 10h, 0fh, 25h, 00h, 0c8h, 00h, 0fh, 08h
sync15def	db	0c1h, 57h, 1ch, 00h, 9fh, 00h, 10h, 0fh, 25h, 00h, 0c8h, 00h, 0fh, 08h
syncdef		db	0c1h, 57h, 10h, 00h, 9fh, 00h, 10h, 0fh, 19h, 00h, 90h, 40h, 07h, 08h
; 15.73 kHz 200-line SYNC from the PC-88VA material (pc88va-video-modes.md 3.2)
sync1573	db	0c1h, 47h, 1ch, 00h, 9fh, 00h, 12h, 11h, 24h, 00h, 0c8h, 00h, 17h, 04h
dataend:
datasave:					; copy of datastart-dataend (not in the file)
