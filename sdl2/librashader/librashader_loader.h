/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef VAEG_SDL2_LIBRASHADER_LOADER_H
#define VAEG_SDL2_LIBRASHADER_LOADER_H

/* Select only the native runtime declarations used by each platform bridge. */
#if defined(_WIN32)
#define LIBRA_RUNTIME_D3D11 1
#endif
#if defined(__linux__)
#define LIBRA_RUNTIME_OPENGL 1
#endif

/* The upstream loader exposes Metal declarations only to Objective-C++. */
#if defined(__APPLE__) && defined(__OBJC__)
#define LIBRA_RUNTIME_METAL 1
#endif

#include "librashader_ld.h"

#include <cstdio>

/* Initialization-only diagnostic wrapper; keep the audited loader unchanged. */
static inline libra_instance_t vaeg_librashader_load_instance(char *error, size_t capacity) {
	if (capacity) error[0] = '\0';
#if defined(VAEG_STATIC_LIBRASHADER)
	libra_instance_t instance = __librashader_make_null_instance();
#define VAEG_BIND_LIBRA(name) instance.name = &libra_##name
	VAEG_BIND_LIBRA(instance_api_version);
	VAEG_BIND_LIBRA(instance_abi_version);
	VAEG_BIND_LIBRA(error_print);
	VAEG_BIND_LIBRA(error_write);
	VAEG_BIND_LIBRA(error_free);
	VAEG_BIND_LIBRA(error_free_string);
	VAEG_BIND_LIBRA(preset_create);
	VAEG_BIND_LIBRA(preset_free);
	VAEG_BIND_LIBRA(preset_get_runtime_params);
	VAEG_BIND_LIBRA(preset_free_runtime_params);
#if defined(LIBRA_RUNTIME_D3D11)
	VAEG_BIND_LIBRA(d3d11_filter_chain_create);
	VAEG_BIND_LIBRA(d3d11_filter_chain_frame);
	VAEG_BIND_LIBRA(d3d11_filter_chain_free);
	VAEG_BIND_LIBRA(d3d11_filter_chain_set_param);
#elif defined(LIBRA_RUNTIME_OPENGL)
	VAEG_BIND_LIBRA(gl_filter_chain_create);
	VAEG_BIND_LIBRA(gl_filter_chain_frame);
	VAEG_BIND_LIBRA(gl_filter_chain_free);
	VAEG_BIND_LIBRA(gl_filter_chain_set_param);
#elif defined(LIBRA_RUNTIME_METAL)
	VAEG_BIND_LIBRA(mtl_filter_chain_create);
	VAEG_BIND_LIBRA(mtl_filter_chain_frame);
	VAEG_BIND_LIBRA(mtl_filter_chain_free);
	VAEG_BIND_LIBRA(mtl_filter_chain_set_param);
#endif
#undef VAEG_BIND_LIBRA
	instance.instance_loaded = instance.instance_abi_version() == LIBRASHADER_CURRENT_ABI &&
	                           instance.instance_api_version() >= LIBRASHADER_CURRENT_VERSION;
	if (!instance.instance_loaded)
		std::snprintf(error, capacity, "Static librashader API/ABI mismatch");
	std::fprintf(stderr, "librashader: static; API=%zu ABI=%zu\n",
	             instance.instance_api_version(), instance.instance_abi_version());
	return instance;
#else
#if defined(_WIN32)
	HMODULE module = LoadLibraryW(L"librashader.dll");
	if (!module) {
		const DWORD code = GetLastError();
		std::snprintf(error, capacity, "librashader.dll: LoadLibrary error %lu", (unsigned long)code);
		// These are imports of the pinned official x64 runtime, not of vaeg.exe.
		const char *dependencies[] = {"D3DX9_43.dll", "MSVCP140.dll", "VCRUNTIME140.dll", "VCRUNTIME140_1.dll"};
		for (const char *name : dependencies) {
			HMODULE dependency = LoadLibraryA(name);
			if (!dependency) {
				std::fprintf(stderr, "librashader dependency unavailable: %s (Win32 %lu)\n",
				             name, (unsigned long)GetLastError());
				std::snprintf(error, capacity, "Missing/unloadable %s; see README-native-crt.md", name);
			} else {
				FreeLibrary(dependency);
			}
		}
		std::fprintf(stderr, "librashader.dll load failed: Win32 %lu\n", (unsigned long)code);
		return __librashader_make_null_instance();
	}
	const char *required[] = {"libra_instance_abi_version", "libra_instance_api_version",
	    "libra_preset_create", "libra_preset_get_runtime_params", "libra_preset_free_runtime_params",
	    "libra_preset_free", "libra_error_write", "libra_error_free_string", "libra_error_free",
	    "libra_d3d11_filter_chain_create", "libra_d3d11_filter_chain_frame",
	    "libra_d3d11_filter_chain_free", "libra_d3d11_filter_chain_set_param"};
	for (const char *symbol : required) {
		if (!GetProcAddress(module, symbol)) {
			std::snprintf(error, capacity, "librashader missing symbol: %s", symbol);
			FreeLibrary(module);
			return __librashader_make_null_instance();
		}
	}
#endif
	libra_instance_t instance = librashader_load_instance();
#if defined(_WIN32)
	char module_path[4096]{};
	GetModuleFileNameA(module, module_path, sizeof(module_path));
	std::fprintf(stderr, "librashader runtime: %s; API=%zu ABI=%zu (expected API>=%u ABI=%u)\n",
	             module_path, instance.instance_api_version(), instance.instance_abi_version(),
	             LIBRASHADER_CURRENT_VERSION, LIBRASHADER_CURRENT_ABI);
	FreeLibrary(module); // The official loader retains its own module reference.
#endif
	if (!instance.instance_loaded) {
		std::snprintf(error, capacity, "librashader unavailable or ABI mismatch (expected %u, got %zu)",
		              LIBRASHADER_CURRENT_ABI, instance.instance_abi_version());
	} else if (instance.instance_api_version() < LIBRASHADER_CURRENT_VERSION) {
		std::snprintf(error, capacity, "librashader API too old: %zu (need %u)",
		              instance.instance_api_version(), LIBRASHADER_CURRENT_VERSION);
		return __librashader_make_null_instance();
	}
	return instance;
#endif
}

#endif
