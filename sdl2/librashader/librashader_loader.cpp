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
#include "librashader_loader.h"

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__) || defined(__unix__) || defined(__linux__)
#include <dlfcn.h>
#endif

namespace {

void *open_runtime_handle() {
#if defined(_WIN32)
    return static_cast<void *>(LoadLibraryW(L"librashader.dll"));
#elif defined(__APPLE__)
    return dlopen("librashader.dylib", RTLD_LAZY);
#elif defined(__unix__) || defined(__linux__)
    return dlopen("librashader.so", RTLD_LAZY);
#else
    return nullptr;
#endif
}

void close_runtime_handle(void *handle, bool official_loader_opened) {
    if (!handle) {
        return;
    }
#if defined(_WIN32)
    FreeLibrary(static_cast<HMODULE>(handle));
    if (official_loader_opened) {
        FreeLibrary(static_cast<HMODULE>(handle));
    }
#elif defined(__APPLE__) || defined(__unix__) || defined(__linux__)
    dlclose(handle);
    if (official_loader_opened) {
        dlclose(handle);
    }
#else
    (void)official_loader_opened;
#endif
}

bool required_symbols_present(const libra_instance_t &instance) {
    return instance.preset_ctx_create !=
               __librashader__noop_preset_ctx_create &&
           instance.preset_ctx_free != __librashader__noop_preset_ctx_free &&
           instance.preset_create_with_options !=
               __librashader__noop_preset_create_with_options &&
           instance.preset_free != __librashader__noop_preset_free &&
           instance.error_errno != __librashader__noop_error_errno &&
           instance.error_free != __librashader__noop_error_free &&
           instance.gl_filter_chain_create !=
               __librashader__noop_gl_filter_chain_create &&
           instance.gl_filter_chain_frame !=
               __librashader__noop_gl_filter_chain_frame &&
           instance.gl_filter_chain_free !=
               __librashader__noop_gl_filter_chain_free;
}

} // namespace

LibrashaderLoader::LibrashaderLoader()
    : instance_(__librashader_make_null_instance()),
      status_(LibrashaderLoadStatus::Unloaded), handle_(nullptr) {}

LibrashaderLoader::~LibrashaderLoader() { unload(); }

bool LibrashaderLoader::load() {
    unload();
    handle_ = open_runtime_handle();
    instance_ = librashader_load_instance();

    const size_t abi = instance_.instance_abi_version();
    const size_t api = instance_.instance_api_version();
    if (!instance_.instance_loaded && abi == 0) {
        status_ = LibrashaderLoadStatus::Absent;
        return false;
    }
    if (abi != LIBRASHADER_CURRENT_ABI) {
        status_ = LibrashaderLoadStatus::AbiMismatch;
        return false;
    }
    if (api < LIBRASHADER_CURRENT_VERSION) {
        status_ = LibrashaderLoadStatus::ApiMismatch;
        return false;
    }
    if (!required_symbols_present(instance_)) {
        status_ = LibrashaderLoadStatus::SymbolsMissing;
        return false;
    }

    status_ = LibrashaderLoadStatus::Ready;
    return true;
}

void LibrashaderLoader::unload() {
    const bool official_loader_opened =
        instance_.instance_abi_version() != 0;
    close_runtime_handle(handle_, official_loader_opened);
    handle_ = nullptr;
    instance_ = __librashader_make_null_instance();
    status_ = LibrashaderLoadStatus::Unloaded;
}

const libra_instance_t *LibrashaderLoader::instance() const {
    return &instance_;
}

LibrashaderLoadStatus LibrashaderLoader::status() const { return status_; }

size_t LibrashaderLoader::abi_version() const {
    return instance_.instance_abi_version();
}

size_t LibrashaderLoader::api_version() const {
    return instance_.instance_api_version();
}
