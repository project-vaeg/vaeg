#!/usr/bin/env python3
"""
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path


ASM_HEADER = """; Copyright (c) 2026 Nakata Maho
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
; PC-88VA backend for SQEMM 0.8. The surrounding SQEMM source is MIT
; licensed by its upstream author; see the upstream LICENSE file.

"""


OVERLAYS = {
    "max/defs/lotech.asm": """VA_PAGE_REGISTER_0 = 08E1h
VA_TARGET_REGISTER = 08E9h
PAGE_FRAME_COUNT = 4
MAX_PAGE_COUNT = 832
DEFAULT_CONVENTIONAL_PAGE_COUNT = 0
MAX_CONTEXT_COUNT = 0
CHIPSET_PAGE_FRAME_COUNT = 4
CHIPSET_DEFAULT_OFFSET = 0
""",
    "max/vars/lotech.asm": """chipset_page_lookup:
public chipset_page_lookup
  db 0C0h, 0C4h, 0C8h, 0CCh

_RESIDENT_VARIABLE_driver_local_page_cache:
  dw -1, -1, -1, -1

mappable_phys_page_struct:
public mappable_phys_page_struct_page_frame
mappable_phys_page_struct_page_frame:
  dw 0C000h, 0000h, 0C400h, 0001h, 0C800h, 0002h, 0CC00h, 0003h
""",
    "max/util/lotech.asm": """UTIL_get_page:
; Return the logical page mapped at physical page-frame index AX.
  push bx
  mov bx, ax
  shl bx, 1
  mov ax, word ptr cs:[bx + _RESIDENT_VARIABLE_driver_local_page_cache]
  pop bx
  ret

UTIL_set_page_reverse_arg:
; Write logical page AX to physical page-frame index DX.
  xchg ax, dx

UTIL_set_page:
public UTIL_set_page
; Write logical page DX to physical page-frame index AX. The VA board selects
; a 1MB target at 08E9h and a 16KB page at 08E1h/08E3h/08E5h/08E7h.
; Logical page -1 restores normal memory. Keep the two port writes atomic.
  push ax
  push bx
  push cx
  push dx
  push si
  pushf
  cli

  mov bx, ax
  shl bx, 1
  mov word ptr cs:[bx + _RESIDENT_VARIABLE_driver_local_page_cache], dx
  mov si, dx

  cmp si, -1
  jne VA_map_logical_page
  xor al, al
  mov dx, VA_TARGET_REGISTER
  out dx, al
  xor al, al
  jmp VA_write_page_register

VA_map_logical_page:
  mov ax, si
  mov cl, 6
  shr ax, cl
  inc al
  mov dx, VA_TARGET_REGISTER
  out dx, al

  mov ax, si
  and al, 03Fh
  shl al, 1
  shl al, 1

VA_write_page_register:
  mov dx, bx
  add dx, VA_PAGE_REGISTER_0
  out dx, al

  popf
  pop si
  pop dx
  pop cx
  pop bx
  pop ax
  ret

UTIL_unmap_all_pages:
  push ax
  push cx
  push dx
  xor ax, ax
  mov dx, -1
  mov cx, 4
VA_unmap_next_page:
  call UTIL_set_page
  inc ax
  loop VA_unmap_next_page
  pop dx
  pop cx
  pop ax
  ret
""",
    "max/init/lotech.asm": """; The PC-88VA page frame is fixed at C0000h-CCFFFh. Probe the target
; register to determine how many 1MB banks the emulator exposes. A read is
; zero for an installed target and FFh for an unavailable target.
  mov word ptr ds:[_RESIDENT_VARIABLE_page_frame_segment+1], 0C000h

  xor bx, bx
  mov cx, 13
  mov dx, VA_TARGET_REGISTER
VA_detect_next_megabyte:
  mov ax, bx
  inc al
  out dx, al
  in al, dx
  test al, al
  jne VA_detect_done
  inc bx
  loop VA_detect_next_megabyte

VA_detect_done:
  xor al, al
  out dx, al
  test bx, bx
  jnz VA_have_memory
  mov dx, OFFSET STRING_could_not_determine
  jmp DRIVER_NOT_INSTALLED

VA_have_memory:
  mov ax, bx
  mov cl, 6
  shl ax, cl
  mov word ptr ds:[_RESIDENT_VARIABLE_unallocated_page_count], ax
  mov word ptr ds:[_RESIDENT_VARIABLE_total_EMS_page_count+1], ax
  mov byte ptr ds:[_RESIDENT_VARIABLE_pageable_frame_count_1+1], PAGE_FRAME_COUNT

  mov di, OFFSET STRING_good_page_count_param_EDIT_OFFSET
  mov dx, OFFSET STRING_good_page_count_param
  call print_driver_param_4_char_int

  call UTIL_unmap_all_pages
""",
    "max/func05/lotech.asm": """; AL is the physical page-frame index and BX is the logical board page.
  push dx
  xor ah, ah
  mov dx, bx
  call UTIL_set_page
  pop dx
  pop bx
  xor ah, ah
  iret
""",
    "max/func15/lotech.asm": """FUNCTION_15_GET_PAGE_MAP:
  push ax
  push bx
  xor bx, bx
  mov ax, word ptr cs:[bx + _RESIDENT_VARIABLE_driver_local_page_cache]
  stosw
  add bx, 2
  mov ax, word ptr cs:[bx + _RESIDENT_VARIABLE_driver_local_page_cache]
  stosw
  add bx, 2
  mov ax, word ptr cs:[bx + _RESIDENT_VARIABLE_driver_local_page_cache]
  stosw
  add bx, 2
  mov ax, word ptr cs:[bx + _RESIDENT_VARIABLE_driver_local_page_cache]
  stosw
  sub di, 8
  pop bx
  pop ax
  ret

FUNCTION_15_SAVE_PAGE_MAP:
public FUNCTION_15_SAVE_PAGE_MAP
  push ax
  push cx
  push dx
  xor ax, ax
  mov cx, 4
VA_restore_next_page:
  lodsw
  xchg ax, dx
  mov ax, 4
  sub ax, cx
  call UTIL_set_page
  loop VA_restore_next_page
  sub si, 4
  pop dx
  pop cx
  pop ax
  ret
""",
    "max/func16/lotech.asm": """  xchg ax, bx
  pop ax

  cmp al, 2
  ja func_16_bad_subfunction
  cbw
  je func_16_sub_02
  test al, al
  push ax
  push dx
  push cx
  push si
  push bx

  lodsw
  xchg ax, cx
  jnz func_16_sub_01

func_16_sub_00:
  push di
  mov ax, cx
  stosw
  jcxz func_16_00_no_pages

func_16_sub_00_save_next_page_frame_register:
  lodsw
  call COMMON_util_get_physical_register_for_segment
  stosw
  mov bx, ax
  shl bx, 1
  mov ax, word ptr cs:[bx + _RESIDENT_VARIABLE_driver_local_page_cache]
  stosw
  loop func_16_sub_00_save_next_page_frame_register

func_16_00_no_pages:
  pop di
func_16_sub_00_done_recording_registers:
func_16_sub_01_done_recording_registers:
func_16_01_no_pages:
func_16_pop_and_return:
  pop bx
  pop si
  pop cx
  pop dx
  pop ax
  iret

func_16_sub_02:
  mov al, bl
  shl al, 1
  inc ax
  shl al, 1
  iret

func_16_sub_01:
func_16_sub_01_save_next_page_frame_register:
  jcxz func_16_01_no_pages
  lodsw
  mov bx, ax
  lodsw
  mov dx, ax
  mov ax, bx
  call UTIL_set_page
  loop func_16_sub_01_save_next_page_frame_register
  jmp func_16_sub_01_done_recording_registers

func_16_bad_subfunction:
  mov ah, 084h
  iret
""",
    "max/func17-0/lotech.asm": """func_1700_skip_logical_check:
  mov dx, bx
  lodsw
  cmp ax, PAGE_FRAME_COUNT
  jae func_1700_physical_page_too_high
  call UTIL_set_page
  loop func_1700_loop_next_page
  sti

func_1700_exit:
  xor ah, ah
func_1700_pop_and_exit:
  mov byte ptr cs:[_temp_byte], ah
  POPA_MACRO
  mov ah, byte ptr cs:[_temp_byte]
  iret

func_1700_logical_page_too_high:
  mov ah, 08Ah
  jmp func_1700_pop_and_exit
func_1700_physical_page_too_high:
  mov ah, 08Bh
  jmp func_1700_pop_and_exit
func_1700_handle_not_found:
  mov ah, 083h
  jmp func_1700_pop_and_exit
""",
    "max/func17-1/lotech.asm": """func_1701_skip_logical_check:
  mov dx, bx
  lodsw
  call COMMON_util_get_register_for_segment
  test ax, ax
  js func_1701_physical_page_too_high
  cmp ax, PAGE_FRAME_COUNT
  jae func_1701_physical_page_too_high
  call UTIL_set_page
  loop func_1701_loop_next_page
  sti

func_1701_exit:
  xor ah, ah
func_1701_pop_and_exit:
  mov byte ptr cs:[_temp_byte], ah
  POPA_MACRO
  mov ah, byte ptr cs:[_temp_byte]
  iret

func_1701_handle_not_found:
  mov ah, 083h
  jmp func_1701_pop_and_exit
func_1701_logical_page_too_high:
  mov ah, 08Ah
  jmp func_1701_pop_and_exit
func_1701_physical_page_too_high:
  mov ah, 08Bh
  jmp func_1701_pop_and_exit

_temp_byte:
  db 0
""",
}


PCENGINE_PRINTER = """PCENGINE_PRINT_STRING:
push  ax
push  bx
push  cx
push  dx
push  si
push  di
push  bp
push  ds
push  es
mov   si, dx
mov   ah, 02h
mov   dx, 8000h
int   083h
pop   es
pop   ds
pop   bp
pop   di
pop   si
pop   dx
pop   cx
pop   bx
pop   ax
ret

"""


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"SQEMM98_PREPARE_{label}: expected one match, found {count}")
    return text.replace(old, new, 1)


def normalize_wasm_syntax(root: Path) -> None:
    shift = re.compile(
        r"^(?P<indent>\s*)SHIFT_MACRO\s+(?!MACRO\b)"
        r"(?P<op>[A-Za-z]+)\s*,?\s+(?P<reg>[A-Za-z][A-Za-z0-9]*)"
        r"\s*,?\s+(?P<count>[0-9]+)(?P<tail>.*)$"
    )
    for path in root.rglob("*.asm"):
        lines = []
        for line in path.read_text(encoding="utf-8").replace("\\", "/").splitlines():
            match = shift.match(line)
            if match:
                line = (
                    f"{match.group('indent')}SHIFT_MACRO {match.group('op')}, "
                    f"{match.group('reg')}, {match.group('count')}{match.group('tail')}"
                )
            line = re.sub(r"\brep(\s+)cmpsb\b", r"repe\1cmpsb", line)
            line = re.sub(r"\bdb\s*,\s*0\b", "db 0", line)
            if re.match(r"^\s*dw\s+(128, 192, 256, 384|512, 640, 768, 896),\s*$", line):
                line = line.rstrip().removesuffix(",")
            lines.append(line)
        path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def prepare_top_level(root: Path) -> None:
    path = root / "sqemm.asm"
    text = path.read_text(encoding="utf-8")
    text = replace_once(
        text,
        "COMPILE_CHIPSET = SCAT_CHIPSET",
        "COMPILE_CHIPSET = LOTECH_BOARD",
        "TARGET",
    )
    macro = """SHIFT_MACRO MACRO instruction, register, count


IF COMPISA GE COMPILE_386
\t&instruction &register, &count

ELSEIF COMPISA GE COMPILE_186
\tIF COUNT GE 4
\t\t&instruction &register, &count
\tELSE
\t\tREPT &count
\t\t\t&instruction &register, 1
\t\tENDM
\tENDIF
ELSE
\tREPT &count
\t\t&instruction &register, 1
\tENDM
ENDIF

ENDM"""
    portable_macro = """SHIFT_MACRO MACRO instruction, register, count
\tREPT count
\t\tinstruction register, 1
\tENDM
ENDM"""
    text = replace_once(text, macro, portable_macro, "SHIFT_MACRO")
    path.write_text(text, encoding="utf-8")


def prepare_init_output(root: Path) -> None:
    path = root / "sqemmmax.asm"
    text = path.read_text(encoding="utf-8")
    text = replace_once(
        text,
        "'Testing Memory Page:    $'",
        "'Testing EMS memory...$'",
        "TEST_MESSAGE",
    )
    text = replace_once(
        text,
        "'SQEMM v 0.8 for Lo-tech EMS Board'",
        "'SQEMM98 MAX v0.8 for PC-88VA'",
        "HEADER",
    )
    text = replace_once(
        text,
        "'Could not determine page frame or port! SQEMM was not loaded.'",
        "'PC-88VA EMS board was not detected. SQEMM98 was not loaded.'",
        "DETECTION_MESSAGE",
    )
    text = replace_once(
        text,
        "'SQEMM successfully initialized.'",
        "'SQEMM98 successfully initialized.'",
        "SUCCESS_MESSAGE",
    )

    start = text.index("STRING_resident_driver_size")
    end = text.index("\nALIGN 2", start)
    strings = text[start:end]
    strings = re.sub(r",\s*(['\"])\$\1", ",0", strings)
    strings = re.sub(r"(['\"])([^'\"\n]*)\$\1", r"\1\2\1,0", strings)
    if "$" in strings:
        raise SystemExit("SQEMM98_PREPARE_STRING_TERMINATOR: DOS terminator remains")
    text = text[:start] + strings + text[end:]

    text = replace_once(
        text,
        "ALIGN 2\n\n_INIT_PARAM_PAGEFRAME_ARG:",
        "ALIGN 2\n\n" + PCENGINE_PRINTER + "_INIT_PARAM_PAGEFRAME_ARG:",
        "PRINTER_INSERT",
    )

    text = replace_once(
        text,
        "do_init:\n"
        "pop  ds\n"
        "pop  bx\n"
        "PUSHA_MACRO\n\n"
        "call  DRIVER_INIT\n\n"
        "POPA_MACRO\n"
        "retf",
        "do_init:\n"
        "pop  ds\n"
        "pop  bx\n"
        "push ds\n"
        "push es\n"
        "PUSHA_MACRO\n\n"
        "call  DRIVER_INIT\n\n"
        "POPA_MACRO\n"
        "pop  es\n"
        "pop  ds\n"
        "retf",
        "INIT_SEGMENT_REGISTERS",
    )

    text = replace_once(
        text,
        "EMS_DRIVER_CALL:\npush  bx",
        "EMS_DRIVER_CALL:\npushf\npush  bx",
        "SAVE_CALLER_FLAGS",
    )
    text, flag_return_count = re.subn(
        r"pop  ds\npop  bx\nretf",
        "pop  ds\npop  bx\npopf\nretf",
        text,
    )
    if flag_return_count != 2:
        raise SystemExit(
            "SQEMM98_PREPARE_RESTORE_CALLER_FLAGS: "
            f"expected two ordinary returns, found {flag_return_count}"
        )
    text = replace_once(
        text,
        "POPA_MACRO\npop  es\npop  ds\nretf",
        "POPA_MACRO\npop  es\npop  ds\npopf\nretf",
        "RESTORE_INIT_FLAGS",
    )

    text = replace_once(
        text,
        "call  process_command_line\n"
        'mov   ah, "Q" ; quiet mode?\n'
        "call  parse_driver_params\n\n"
        "jnc   quiet_mode_off\n\n\n"
        "mov   byte ptr ds:[print_driver_param], RET_OPCODE\n\n",
        "; PC-Engine request headers do not expose the DOS command-tail ABI.\n"
        "; Keep the PC-88VA port on its validated built-in defaults.\n\n",
        "PCENGINE_COMMAND_LINE",
    )

    dos_print = re.compile(
        r"^(?P<indent>[ \t]*)mov[ \t]+ah,[ \t]*(?:9|09h)[^\n]*\n"
        r"(?P=indent)int[ \t]+021h[ \t]*$",
        re.MULTILINE,
    )
    text, print_count = dos_print.subn(
        lambda match: f"{match.group('indent')}call  PCENGINE_PRINT_STRING", text
    )
    if print_count != 10:
        raise SystemExit(
            f"SQEMM98_PREPARE_PRINT_CALLS: expected 10 matches, found {print_count}"
        )

    text = replace_once(
        text,
        "   mov   ax, bx\n   call  print_ax_at_cursor\n\n",
        "",
        "PAGE_PROGRESS",
    )
    text = replace_once(
        text,
        "mov   ds, word ptr ds:[_RESIDENT_VARIABLE_page_frame_segment+1]\n\n"
        "xor   bx, bx",
        "mov   ds, word ptr ds:[_RESIDENT_VARIABLE_page_frame_segment+1]\n"
        "xor   di, di\n\n"
        "xor   bx, bx",
        "MEMORY_TEST_OFFSET",
    )
    cursor_start = text.index("memory_good:\n") + len("memory_good:\n")
    cursor_end = text.index("; now lets test all four pages...", cursor_start)
    cursor_block = text[cursor_start:cursor_end]
    if cursor_block.count("int   010h") != 2:
        raise SystemExit("SQEMM98_PREPARE_CURSOR_BLOCK: unexpected INT 10h block")
    text = text[:cursor_start] + "\n" + text[cursor_end:]

    function = re.compile(
        r"\nprint_ax_at_cursor:\n.*?(?=\n;;; END GENERIC INIT CODE)", re.DOTALL
    )
    text, function_count = function.subn("\n", text)
    if function_count != 1:
        raise SystemExit(
            f"SQEMM98_PREPARE_CURSOR_FUNCTION: expected one match, found {function_count}"
        )
    path.write_text(text, encoding="utf-8")


def write_overlays(root: Path) -> None:
    for relative, body in OVERLAYS.items():
        path = root / relative
        if not path.is_file():
            raise SystemExit(f"SQEMM98_PREPARE_OVERLAY_TARGET: missing {relative}")
        path.write_text(ASM_HEADER + body, encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Prepare pinned SQEMM 0.8 sources for the PC-88VA EMS board"
    )
    parser.add_argument("source", type=Path)
    args = parser.parse_args()
    root = args.source.resolve()
    if not (root / "sqemm.asm").is_file() or not (root / "sqemmmax.asm").is_file():
        raise SystemExit("SQEMM98_PREPARE_SOURCE: not an SQEMM source tree")

    normalize_wasm_syntax(root)
    prepare_top_level(root)
    prepare_init_output(root)
    write_overlays(root)
    print("SQEMM98_PREPARE_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
