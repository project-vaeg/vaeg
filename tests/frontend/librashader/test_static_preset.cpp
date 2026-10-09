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

#include "librashader/builtin_shaders.h"
#include "librashader/librashader_loader.h"
#include <cstdio>
#include <cstring>

int main() {
    char error[512]{};
    auto api = vaeg_librashader_load_instance(error, sizeof(error));
    if (!api.instance_loaded) {
        std::fprintf(stderr, "%s\n", error);
        return 1;
    }
    const char *path = vaeg_builtin_shader_preset();
    libra_shader_preset_t preset = nullptr;
    libra_error_t result = api.preset_create(path, &preset);
    if (result || !preset) {
        if (result) {
            api.error_print(result);
            api.error_free(&result);
        }
        return 2;
    }
    libra_preset_param_list_t list{};
    result = api.preset_get_runtime_params(&preset, &list);
    if (result) {
        api.error_print(result);
        api.error_free(&result);
        api.preset_free(&preset);
        return 3;
    }
    bool screen_size = false;
    for (size_t i = 0; i < list.length; ++i)
        if (std::strcmp(list.parameters[i].name, "SCREEN_SIZE") == 0)
            screen_size = true;
    std::printf("Static C API: ABI=%zu API=%zu; embedded preset parameters=%llu; SCREEN_SIZE=%s\n",
        api.instance_abi_version(), api.instance_api_version(),
        static_cast<unsigned long long>(list.length), screen_size ? "present" : "absent");
    api.preset_free_runtime_params(list);
    api.preset_free(&preset);
    return screen_size ? 0 : 4;
}
