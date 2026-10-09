# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

if(NOT DEFINED INPUT OR NOT DEFINED OUTPUT OR NOT DEFINED SYMBOL)
    message(FATAL_ERROR
        "embed_binary.cmake requires INPUT, OUTPUT, and SYMBOL")
endif()
if(NOT SYMBOL MATCHES "^[A-Za-z_][A-Za-z0-9_]*$")
    message(FATAL_ERROR "embed_binary.cmake SYMBOL is not a C identifier")
endif()
if(NOT DEFINED SOURCE_LABEL)
    set(SOURCE_LABEL "${INPUT}")
endif()

file(READ "${INPUT}" input_hex HEX)
string(LENGTH "${input_hex}" input_hex_length)
math(EXPR input_size "${input_hex_length} / 2")
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," input_bytes
    "${input_hex}")

file(WRITE "${OUTPUT}"
    "/* Generated from ${SOURCE_LABEL}; do not edit. */\n"
    "const unsigned char ${SYMBOL}[] = {\n"
    "${input_bytes}\n"
    "};\n"
    "const unsigned int ${SYMBOL}_size = ${input_size}u;\n")
