/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <windows.h>

// The pinned runtime's shared D3D cache references DXC even for D3D11/FXC.
// Resolve that optional compiler only when a DXC operation is actually requested.
// Default D3D11 presets must not require dxcompiler.dll at process startup.
static HRESULT WINAPI vaeg_optional_dxc(REFCLSID clsid, REFIID iid, void **result) {
    using Create = HRESULT(WINAPI *)(REFCLSID, REFIID, void **);
    static const HMODULE module = LoadLibraryExW(
        L"dxcompiler.dll", nullptr, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    static const Create create = module
        ? reinterpret_cast<Create>(GetProcAddress(module, "DxcCreateInstance")) : nullptr;
    if (result) *result = nullptr;
    return create ? create(clsid, iid, result) : HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
}

// Rust's windows binding references the import-address symbol. Supplying it
// here keeps the unused DXC branch from introducing a mandatory DLL import.
extern "C" {
HRESULT WINAPI DxcCreateInstance(REFCLSID clsid, REFIID iid, void **result) {
    return vaeg_optional_dxc(clsid, iid, result);
}
HRESULT(WINAPI *__imp_DxcCreateInstance)(REFCLSID, REFIID, void **) = vaeg_optional_dxc;
}
