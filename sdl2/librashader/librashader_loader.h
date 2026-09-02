/*
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef VAEG_LIBRASHADER_LOADER_H
#define VAEG_LIBRASHADER_LOADER_H

#include <stddef.h>

#ifndef LIBRA_RUNTIME_OPENGL
#define LIBRA_RUNTIME_OPENGL
#endif
#include "librashader_ld.h"

enum class LibrashaderLoadStatus {
    Unloaded,
    Ready,
    Absent,
    AbiMismatch,
    ApiMismatch,
    SymbolsMissing,
};

class LibrashaderLoader {
public:
    LibrashaderLoader();
    ~LibrashaderLoader();

    bool load();
    void unload();

    const libra_instance_t *instance() const;
    LibrashaderLoadStatus status() const;
    size_t abi_version() const;
    size_t api_version() const;

private:
    libra_instance_t instance_;
    LibrashaderLoadStatus status_;
    void *handle_;
};

#endif
