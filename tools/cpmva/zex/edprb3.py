# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

"""Generate EDPRB3.COM: isolate the inputs of the undefined ED 00-3F value.

On the real uPD9002 the undefined ED 00-3F/74/75/77 opcodes write an
input-dependent 16-bit value to the pair selected by bits 5-4 and set F to
S/Z/P of its high byte.  EDPRB2 run 4 and INPRB run 6 showed that the value
does not depend on the code layout, is not memory at (BC) and is not an I/O
read of (C)/(C+1) or the 16-bit port BC.  The remaining candidate inputs are
A, F, B, C, the pointer registers and the sandbox memory contents.  EDPRB3
runs ED 20 (destination HL) with a baseline equal to EDPRB2's input set 0
and varies exactly one input per case, plus full replicas of sets 1 and 2
and single ED 00/ED 10 controls.  Each output line is one 128-byte record:

    Xii aaff bbbb dddd hhhh ssss w1w1 w2w2 w3w3 op -> rrrr gg

padded with spaces before CR LF.  The literal part names the case inputs
(AF, BC, DE, HL, SP, the words stored little-endian at 3404h/3C04h/5404h
and the second opcode byte); rrrr/gg are the destination pair and F after
the instruction.  EDPRB3.TXT is closed as a checkpoint after every record.
z80asm 1.8 syntax; cpmio.asm is appended by zexbuild.py.
"""

from __future__ import annotations

RECORD = 128
W1_ADDR, W2_ADDR, W3_ADDR = 0x3404, 0x3C04, 0x5404

BASE = {"af": 0x5AD7, "bc": 0x0130, "de": 0x3404, "hl": 0x3C04, "sp": 0x5404,
        "w1": 0x2524, "w2": 0x3534, "w3": 0x6564, "op": 0x20}
SET1 = {"af": 0xA500, "bc": 0x0150, "w1": 0x9D9C, "w2": 0xADAC, "w3": 0xDDDC}
SET2 = {"af": 0x3CC5, "bc": 0x0180, "w1": 0xDBDA, "w2": 0xCBCA, "w3": 0x9B9A}


def cases() -> list[dict]:
    result = [dict(BASE)]                                   # X00 baseline
    for a in (0xA5, 0x3C, 0x00, 0xFF):                      # X01-X04 vary A
        result.append(dict(BASE, af=(a << 8) | (BASE["af"] & 0xFF)))
    for f in (0x00, 0xC5, 0xFF):                            # X05-X07 vary F
        result.append(dict(BASE, af=(BASE["af"] & 0xFF00) | f))
    for b in (0x00, 0x0B, 0xFF):                            # X08-X0A vary B
        result.append(dict(BASE, bc=(b << 8) | (BASE["bc"] & 0xFF)))
    for c in (0x50, 0x80, 0x0C, 0x00, 0x31, 0xFF):          # X0B-X10 vary C
        result.append(dict(BASE, bc=(BASE["bc"] & 0xFF00) | c))
    result.append(dict(BASE, bc=0x0B0C))                    # X11 BC of EDPRB
    result.append(dict(BASE, de=0x3504))                    # X12 vary DE
    result.append(dict(BASE, hl=0x3D04))                    # X13 vary HL
    result.append(dict(BASE, sp=0x5504))                    # X14 vary SP
    result.append(dict(BASE, w1=0x9D9C))                    # X15 vary (3404)
    result.append(dict(BASE, w2=0xADAC))                    # X16 vary (3C04)
    result.append(dict(BASE, w3=0xBDBC))                    # X17 vary (5404)
    result.append(dict(BASE, **SET1))                       # X18 set 1 replica
    result.append(dict(BASE, **SET2))                       # X19 set 2 replica
    result.append(dict(BASE, op=0x10))                      # X1A ED 10 -> DE
    result.append(dict(BASE, op=0x00))                      # X1B ED 00 -> BC
    return result


def case_block(index: int, case: dict, last: bool) -> str:
    literal = (f"X{index:02X} {case['af']:04X} {case['bc']:04X} {case['de']:04X} "
               f"{case['hl']:04X} {case['sp']:04X} {case['w1']:04X} {case['w2']:04X} "
               f"{case['w3']:04X} {case['op']:02X} -> ")
    dest = {0x00: "\tld\th,b\n\tld\tl,c\n", 0x10: "\tex\tde,hl\n"}.get(case["op"], "")
    goto = "run_end" if last else f"q_{index + 1:02x}"
    return f"""
q_{index:02x}:	ld	de,t_{index:02x}
	call	puts
	ld	(mainsp),sp
	di
	ld	sp,0{case['sp']:04x}h
	ld	hl,0{case['w1']:04x}h
	ld	(0{W1_ADDR:04x}h),hl
	ld	hl,0{case['w2']:04x}h
	ld	(0{W2_ADDR:04x}h),hl
	ld	hl,0{case['w3']:04x}h
	ld	(0{W3_ADDR:04x}h),hl
	ld	bc,0{case['bc']:04x}h
	ld	de,0{case['de']:04x}h
	ld	hl,0{case['af']:04x}h
	push	hl
	pop	af
	ld	hl,0{case['hl']:04x}h
	db	0edh,0{case['op']:02x}h	; under test
{dest}	push	af
	pop	de		; e = F after
	ld	sp,(mainsp)
	ei
	call	hex4		; destination pair from hl
	ld	a,' '
	call	putc
	ld	a,e
	call	hex2
	call	recend
	jp	{goto}
t_{index:02x}:	db	'{literal}$'
"""


def source() -> str:
    all_cases = cases()
    blocks = "".join(case_block(i, c, i == len(all_cases) - 1)
                     for i, c in enumerate(all_cases))
    return f"""; M102 EDPRB3: generated by tools/cpmva/zex/edprb3.py; do not edit.
;
; Isolates the inputs of the 16-bit value written by the undefined
; ED 00-3F/74/75/77 opcodes; see the module docstring of edprb3.py.
; Writes EDPRB3.TXT in {RECORD}-byte records, closed after every record.

	org	100h

start:	ld	sp,stack
	ld	de,fcbout
	call	fopen
	ld	de,title
	call	conmsg
	jp	q_00

; pad the record with spaces, end it with CR LF, close as a checkpoint
recend:	push	af
	push	bc
	push	de
	push	hl
rpad:	ld	a,(fcnt)
	cp	2
	jp	z,rcr
	ld	a,' '
	call	fputc		; file only
	jp	rpad
rcr:	ld	a,13
	call	putc
	ld	a,10
	call	putc		; completes the record; fflush runs
	ld	de,(curfcb)
	ld	c,16		; close as a checkpoint; writing continues
	call	bdosv
	pop	hl
	pop	de
	pop	bc
	pop	af
	ret

run_end: call	fclose
	ld	de,done
	call	conmsg
	jp	0

title:	db	'# EDPRB3 M102 undefined ED value inputs',13,10,'$'
done:	db	'EDPRB3 done',13,10,'$'
mainsp:	dw	0
fcbout:	db	0,'EDPRB3  TXT'
	db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	ds	64
stack:
{blocks}"""
