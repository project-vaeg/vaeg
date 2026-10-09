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

if(NOT DEFINED VAEG_EXECUTABLE)
    message(FATAL_ERROR "VAEG_EXECUTABLE is required")
endif()

set(work "${CMAKE_CURRENT_BINARY_DIR}/m74-debug-harness-romless")
set(output "${work}/output")
file(REMOVE_RECURSE "${work}")
file(MAKE_DIRECTORY "${output}")
file(WRITE "${work}/case.debug"
    "debug-script 1\n"
    "limit-frame 4\n"
    "counter reset-vector f000:fff0\n"
    "wait-pc f000:fff0 1\n"
    "trace reset-event 1\n"
    "capture reset-event registers tvram screen\n"
    "exit\n")

execute_process(
    COMMAND "${VAEG_EXECUTABLE}" --smoke --model va
        --debug-script "${work}/case.debug" --debug-output-dir "${output}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout_text
    ERROR_VARIABLE stderr_text
    TIMEOUT 30)
if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "M74 ROM-less debug harness failed (${result}):\n${stdout_text}\n${stderr_text}")
endif()

foreach(name IN ITEMS events.tsv reset-event.registers.tsv
        reset-event.tvram.bin reset-event.screen.bmp reset-event.trace.log)
    if(NOT EXISTS "${output}/${name}")
        message(FATAL_ERROR "M74_DEBUG_OUTPUT_MISSING: ${name}")
    endif()
endforeach()

file(READ "${output}/events.tsv" events)
string(ASCII 9 tab)
foreach(pattern IN ITEMS
        "initialized${tab}0${tab}-${tab}4"
        "pc${tab}0${tab}-${tab}1"
        "trace${tab}0${tab}reset-event${tab}1"
        "capture${tab}0${tab}reset-event${tab}7"
        "exit${tab}1${tab}-${tab}0"
        "counter${tab}1${tab}reset-vector${tab}1")
    if(NOT events MATCHES "${pattern}")
        message(FATAL_ERROR "M74_DEBUG_EVENT_SCHEMA: missing ${pattern}")
    endif()
endforeach()

file(READ "${output}/reset-event.registers.tsv" registers)
if(NOT registers MATCHES "schema${tab}vaeg-registers-v1" OR
        NOT registers MATCHES "cs${tab}f000" OR
        NOT registers MATCHES "ip${tab}fff0")
    message(FATAL_ERROR "M74_DEBUG_REGISTER_SCHEMA")
endif()
file(READ "${output}/reset-event.trace.log" trace)
if(NOT trace MATCHES "upd9002-trace-v1" OR
        NOT trace MATCHES "begin step=00000000 cs=f000 ip=fff0")
    message(FATAL_ERROR "M74_DEBUG_TRACE_START")
endif()
file(READ "${output}/reset-event.tvram.bin" tvram LIMIT 8 HEX)
if(NOT tvram STREQUAL "5641454753434e31")
    message(FATAL_ERROR "M74_DEBUG_TVRAM_SCHEMA")
endif()
file(READ "${output}/reset-event.screen.bmp" bmp LIMIT 2 HEX)
if(NOT bmp STREQUAL "424d")
    message(FATAL_ERROR "M74_DEBUG_SCREEN_SCHEMA")
endif()

file(REMOVE_RECURSE "${work}")
file(MAKE_DIRECTORY "${output}")
file(WRITE "${work}/case.debug"
    "debug-script 1\n"
    "limit-frame 1\n"
    "wait-pc dead:beef 1\n"
    "capture unreachable registers\n"
    "exit\n")
execute_process(
    COMMAND "${VAEG_EXECUTABLE}" --smoke --model va
        --debug-script "${work}/case.debug" --debug-output-dir "${output}"
    RESULT_VARIABLE limit_result
    OUTPUT_VARIABLE limit_stdout
    ERROR_VARIABLE limit_stderr
    TIMEOUT 30)
if(NOT limit_result EQUAL 0)
    message(FATAL_ERROR
        "M74 ROM-less frame limit failed (${limit_result}):\n"
        "${limit_stdout}\n${limit_stderr}")
endif()
file(READ "${output}/events.tsv" limit_events)
if(NOT limit_events MATCHES "frame-limit${tab}1${tab}-${tab}1" OR
        limit_events MATCHES "\npc${tab}")
    message(FATAL_ERROR "M74_DEBUG_FRAME_LIMIT")
endif()
file(REMOVE_RECURSE "${work}")
file(MAKE_DIRECTORY "${output}")
file(WRITE "${work}/case.debug"
    "debug-script 1\n"
    "limit-frame 12\n"
    "resource empty none\n"
    "wait-frame 1\n"
    "mount-fdd 1 empty\n"
    "enter\n"
    "exit\n")
execute_process(
    COMMAND "${VAEG_EXECUTABLE}" --smoke --model va
        --debug-script "${work}/case.debug" --debug-output-dir "${output}"
    RESULT_VARIABLE action_result
    OUTPUT_VARIABLE action_stdout
    ERROR_VARIABLE action_stderr
    TIMEOUT 30)
if(NOT action_result EQUAL 0)
    message(FATAL_ERROR
        "M74 ROM-less frame action failed (${action_result}):\n"
        "${action_stdout}\n${action_stderr}")
endif()
file(READ "${output}/events.tsv" action_events)
foreach(pattern IN ITEMS
        "frame${tab}1${tab}-${tab}1"
        "mount-fdd${tab}1${tab}empty${tab}1"
        "input${tab}1${tab}-${tab}3")
    if(NOT action_events MATCHES "${pattern}")
        message(FATAL_ERROR "M74_DEBUG_FRAME_ACTION: missing ${pattern}")
    endif()
endforeach()
if(action_events MATCHES "frame-limit")
    message(FATAL_ERROR "M74_DEBUG_FRAME_ACTION_LIMIT")
endif()
file(REMOVE_RECURSE "${work}")
message(STATUS
    "M74 ROM-less debug harness event, trace, capture, frame-limit, input, "
    "and FDD schemas passed")
