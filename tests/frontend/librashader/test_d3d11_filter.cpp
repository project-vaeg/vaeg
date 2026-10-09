/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
// Optional real C-API smoke test. Run beside the pinned runtime DLL; no ROMs.
#include <cstdio>
#include <wrl/client.h>
#include "librashader/librashader_loader.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        std::fprintf(stderr, "Usage: vaeg-d3d11-filter-test.exe PRESET.slangp\n");
        return 2;
    }
    char message[512]{};
    auto libra = vaeg_librashader_load_instance(message, sizeof(message));
    if (!libra.instance_loaded) {
        std::fprintf(stderr, "D3D11_RUNTIME_UNAVAILABLE: %s\n", message);
        return 2;
    }
    auto checked = [&](libra_error_t error) {
        if (!error) return true;
        libra.error_print(error);
        libra.error_free(&error);
        return false;
    };
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &context);
    if (FAILED(hr)) {
        std::fprintf(stderr, "D3D11_DEVICE_UNAVAILABLE: %08lx\n", (unsigned long)hr);
        return 2;
    }
    libra_shader_preset_t preset = nullptr;
    if (!checked(libra.preset_create(argv[1], &preset))) return 1;
    libra_d3d11_filter_chain_t chain = nullptr;
    filter_chain_d3d11_opt_t options{};
    options.version = LIBRASHADER_CURRENT_VERSION;
    options.disable_cache = true;
    bool ok = checked(libra.d3d11_filter_chain_create(&preset, device.Get(), &options, &chain));
    if (preset) checked(libra.preset_free(&preset));
    if (!ok || !chain) {
        std::fprintf(stderr, "D3D11_FILTER_CREATE_FAILED\n");
        return 1;
    }
    if (!checked(libra.d3d11_filter_chain_free(&chain))) return 1;
    std::printf("D3D11_FILTER_CREATE_PASS: %s (WARP; cache disabled)\n", argv[1]);
    return 0;
}
