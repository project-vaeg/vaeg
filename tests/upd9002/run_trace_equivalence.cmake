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
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

if(NOT DEFINED VAEG_EXECUTABLE OR NOT DEFINED TRACE_GOLDEN)
    message(FATAL_ERROR "VAEG_EXECUTABLE and TRACE_GOLDEN are required")
endif()

function(run_trace output_variable)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env
            SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
            "${VAEG_EXECUTABLE}" --selftest --trace-cpu 8
        RESULT_VARIABLE result
        OUTPUT_VARIABLE stdout_text
        ERROR_VARIABLE stderr_text)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "trace-enabled selftest failed (${result}):\n${stderr_text}")
    endif()
    # MSYS2's C runtime may translate stderr lines to CRLF.  Canonical trace
    # comparison is line-oriented, so make the captured stream platform
    # independent before extracting records and checkpoints.
    string(REPLACE "\r" "" stderr_text "${stderr_text}")
    string(REGEX MATCHALL
        "(upd9002-trace-v1|begin step=[^\n]*|event step=[^\n]*|end step=[^\n]*)"
        trace_lines "${stderr_text}")
    if(NOT trace_lines)
        message(FATAL_ERROR "trace-enabled run emitted no canonical trace")
    endif()
    list(JOIN trace_lines "\n" trace_text)
    set(${output_variable} "${trace_text}\n" PARENT_SCOPE)
    string(REGEX REPLACE
        "upd9002-trace-v1\n|begin step=[^\n]*\n|event step=[^\n]*\n|end step=[^\n]*\n"
        "" checkpoint_text "${stderr_text}")
    # Selftest media names include the process ID to avoid collisions.  That
    # runtime-only value must not make deterministic checkpoint comparison
    # fail between otherwise identical runs.
    string(REGEX REPLACE
        "vaeg-selftest-[0-9]+"
        "vaeg-selftest-PID"
        checkpoint_text "${checkpoint_text}")
    # macOS SDL startup logs include wall-clock time and process/thread IDs.
    # They are host diagnostics, not part of the CPU checkpoint contract, so
    # remove only the known renderer-switch records before comparing runs.
    string(REGEX REPLACE
        "[^\n]*INFO: Renderer switch: native window detached; new SDL window=[0-9]+\n"
        ""
        checkpoint_text "${checkpoint_text}")
    set(${output_variable}_checkpoints "${checkpoint_text}" PARENT_SCOPE)
endfunction()

run_trace(first)
run_trace(second)
if(NOT first STREQUAL second)
    string(LENGTH first first_length)
    string(LENGTH second second_length)
    message(FATAL_ERROR
        "two identical trace-enabled runs differ "
        "(first_length=${first_length} second_length=${second_length})")
endif()
if(NOT first_checkpoints STREQUAL second_checkpoints)
    string(LENGTH first_checkpoints first_checkpoint_length)
    string(LENGTH second_checkpoints second_checkpoint_length)
    message(FATAL_ERROR
        "two trace-enabled checkpoint streams differ "
        "(first_length=${first_checkpoint_length} "
        "second_length=${second_checkpoint_length})")
endif()
foreach(origin cpu dma device)
    if(NOT first MATCHES "origin=${origin}")
        message(FATAL_ERROR "canonical trace lacks ${origin} origin")
    endif()
endforeach()
file(READ "${TRACE_GOLDEN}" golden)
set(m60a_saved_flags_lines
    "event step=00000004 seq=00000004 origin=cpu kind=memory-write address=00007ffe value=0000f002 width=02"
    "event step=00000005 seq=00000004 origin=cpu kind=memory-write address=00007ffe value=0000f002 width=02")
set(m42_compatible_trace "${first}")
foreach(current_line IN LISTS m60a_saved_flags_lines)
    string(REGEX MATCHALL "${current_line}" matches "${m42_compatible_trace}")
    list(LENGTH matches match_count)
    if(NOT match_count EQUAL 1)
        message(FATAL_ERROR
            "approved M60a saved-FLAGS trace transition is absent or duplicated")
    endif()
    string(REPLACE "value=0000f002" "value=00000002"
        baseline_line "${current_line}")
    string(REPLACE "${current_line}" "${baseline_line}"
        m42_compatible_trace "${m42_compatible_trace}")
endforeach()
set(m64_div_flags_lines
    "end step=00000000 ax=0001 bx=0000 cx=0001 dx=007f si=0000 di=0000 bp=0000 sp=8000 es=0000 cs=0000 ss=0000 ds=0000 ip=2002 flags=f093 esbase=00000000 csbase=00000000 ssbase=00000000 dsbase=00000000 consumed=0000000e remain=000003da"
    "end step=00000001 ax=0001 bx=0000 cx=0001 dx=007f si=0000 di=0000 bp=0000 sp=8000 es=0000 cs=0000 ss=0000 ds=0000 ip=2002 flags=f093 esbase=00000000 csbase=00000000 ssbase=00000000 dsbase=00000000 consumed=0000000e remain=000003da"
    "end step=00000002 ax=0205 bx=0003 cx=0001 dx=007f si=0000 di=0000 bp=0000 sp=8000 es=0000 cs=0000 ss=0000 ds=0000 ip=2002 flags=f093 esbase=00000000 csbase=00000000 ssbase=00000000 dsbase=00000000 consumed=0000000e remain=000003da"
    "end step=00000003 ax=0205 bx=0003 cx=0001 dx=007f si=0000 di=0000 bp=0000 sp=8000 es=0000 cs=0000 ss=0000 ds=0000 ip=2002 flags=f093 esbase=00000000 csbase=00000000 ssbase=00000000 dsbase=00000000 consumed=0000000e remain=000003da")
foreach(current_line IN LISTS m64_div_flags_lines)
    string(REGEX MATCHALL "${current_line}" matches "${m42_compatible_trace}")
    list(LENGTH matches match_count)
    if(NOT match_count EQUAL 1)
        message(FATAL_ERROR
            "approved M64 DIV/IDIV FLAGS trace transition is absent or duplicated")
    endif()
    string(REPLACE "flags=f093" "flags=f002"
        baseline_line "${current_line}")
    string(REPLACE "${current_line}" "${baseline_line}"
        m42_compatible_trace "${m42_compatible_trace}")
endforeach()
if(NOT m42_compatible_trace STREQUAL golden)
    message(FATAL_ERROR
        "canonical trace differs outside the approved M60a saved-FLAGS and M64 DIV/IDIV transitions")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
        "${VAEG_EXECUTABLE}" --selftest
    RESULT_VARIABLE untraced_result
    OUTPUT_QUIET ERROR_VARIABLE untraced_error)
if(NOT untraced_result EQUAL 0)
    message(FATAL_ERROR "trace-disabled equivalence selftest failed: ${untraced_error}")
endif()
string(REPLACE "\r" "" untraced_error "${untraced_error}")
string(REGEX REPLACE
    "vaeg-selftest-[0-9]+"
    "vaeg-selftest-PID"
    untraced_error "${untraced_error}")
string(REGEX REPLACE
    "[^\n]*INFO: Renderer switch: native window detached; new SDL window=[0-9]+\n"
    ""
    untraced_error "${untraced_error}")
if(NOT first_checkpoints STREQUAL untraced_error)
    message(FATAL_ERROR
        "trace-enabled and trace-disabled final checkpoint streams differ")
endif()

message(STATUS "uPD9002 trace determinism, origin schema, M42 baseline plus exact M60a/M64 transitions, and complete on/off checkpoint equivalence passed")
