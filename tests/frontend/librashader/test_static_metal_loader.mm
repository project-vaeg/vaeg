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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import <Metal/Metal.h>

#include <cstdio>

#include "librashader/librashader_loader.h"

int main() {
	char error[512] = {};
	const libra_instance_t api = vaeg_librashader_load_instance(error, sizeof(error));
	if (!api.instance_loaded) {
		std::fprintf(stderr, "%s\n", error[0] ? error : "static librashader unavailable");
		return 1;
	}
	if ((api.mtl_filter_chain_create == &__librashader__noop_mtl_filter_chain_create) ||
	    (api.mtl_filter_chain_frame == &__librashader__noop_mtl_filter_chain_frame) ||
	    (api.mtl_filter_chain_free == &__librashader__noop_mtl_filter_chain_free) ||
	    (api.mtl_filter_chain_set_param == &__librashader__noop_mtl_filter_chain_set_param)) {
		std::fprintf(stderr, "static Metal filter-chain API resolved to no-op functions\n");
		return 2;
	}
	std::printf("Static Metal C API: ABI=%zu API=%zu; filter-chain functions bound\n",
	            api.instance_abi_version(), api.instance_api_version());
	return 0;
}
