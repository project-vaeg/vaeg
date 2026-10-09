#
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
# USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# /

set(shader_root "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/crt")
set(shader_output "${CMAKE_CURRENT_BINARY_DIR}/generated/vaeg_builtin_shaders.h")
set(shader_paths
    vaeg_crt_default.slangp
    shaders/vaeg-screen-size.slang
    shaders/vaeg-crt-aa.slang
    shaders/vaeg-scanline-aa.inc)
set(shader_records "")
set(shader_identity "")
foreach(path IN LISTS shader_paths)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${shader_root}/${path}")
    file(READ "${shader_root}/${path}" contents)
    file(SHA256 "${shader_root}/${path}" digest)
    string(APPEND shader_identity "${path}:${digest}\n")
    if(contents MATCHES "\\)vaegasset\"")
        message(FATAL_ERROR "Shader raw-string delimiter collision: ${path}")
    endif()
    string(APPEND shader_records "{\"${path}\", R\"vaegasset(${contents})vaegasset\"},\n")
endforeach()
string(SHA256 shader_digest "${shader_identity}")
file(WRITE "${shader_output}"
    "// Generated from the audited bundled CRT closure.\n"
    "static const char vaeg_shader_digest[] = \"${shader_digest}\";\n"
    "struct VaegShaderAsset { const char *path; const char *text; };\n"
    "static const VaegShaderAsset vaeg_builtin_shaders[] = {\n${shader_records}};\n")
