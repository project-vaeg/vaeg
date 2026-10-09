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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <cstdio>
#include <cstring>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include "librashader/d3d11_pass_through.h"

using Microsoft::WRL::ComPtr;
#define REQUIRE(call) do { HRESULT hr = (call); if (FAILED(hr)) { \
    std::fprintf(stderr, "D3D11_WARP_FAILED: %s: 0x%08lx\n", #call, (unsigned long)hr); return 1; } } while (0)

int main() {
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    REQUIRE(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0,
                             D3D11_SDK_VERSION, &device, nullptr, &context));
    ComPtr<ID3DBlob> vs_code, ps_code, errors;
    REQUIRE(D3DCompile(vaeg_d3d11_shader_source, std::strlen(vaeg_d3d11_shader_source),
                      nullptr, nullptr, nullptr, "vs_main", "vs_4_0", 0, 0, &vs_code, &errors));
    REQUIRE(D3DCompile(vaeg_d3d11_shader_source, std::strlen(vaeg_d3d11_shader_source),
                      nullptr, nullptr, nullptr, "ps_main", "ps_4_0", 0, 0, &ps_code, &errors));
    ComPtr<ID3D11VertexShader> vs;
    ComPtr<ID3D11PixelShader> ps;
    REQUIRE(device->CreateVertexShader(vs_code->GetBufferPointer(), vs_code->GetBufferSize(), nullptr, &vs));
    REQUIRE(device->CreatePixelShader(ps_code->GetBufferPointer(), ps_code->GetBufferSize(), nullptr, &ps));

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = desc.Height = 16;
    desc.MipLevels = desc.ArraySize = desc.SampleDesc.Count = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    ComPtr<ID3D11Texture2D> output, readback, source;
    REQUIRE(device->CreateTexture2D(&desc, nullptr, &output));
    desc.BindFlags = 0;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    REQUIRE(device->CreateTexture2D(&desc, nullptr, &readback));
    desc.Width = desc.Height = 1;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.CPUAccessFlags = 0;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    const unsigned int white = 0xffffffff;
    D3D11_SUBRESOURCE_DATA data{&white, 4, 4};
    REQUIRE(device->CreateTexture2D(&desc, &data, &source));
    ComPtr<ID3D11ShaderResourceView> srv;
    ComPtr<ID3D11RenderTargetView> rtv;
    REQUIRE(device->CreateShaderResourceView(source.Get(), nullptr, &srv));
    REQUIRE(device->CreateRenderTargetView(output.Get(), nullptr, &rtv));
    D3D11_SAMPLER_DESC sampler_desc{};
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc.AddressU = sampler_desc.AddressV = sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    ComPtr<ID3D11SamplerState> sampler;
    REQUIRE(device->CreateSamplerState(&sampler_desc, &sampler));
    auto raster_desc = vaeg_d3d11_pass_through_rasterizer_desc();
    ComPtr<ID3D11RasterizerState> rasterizer;
    REQUIRE(device->CreateRasterizerState(&raster_desc, &rasterizer));
    D3D11_VIEWPORT viewport{0, 0, 16, 16, 0, 1};
    context->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);
    context->RSSetViewports(1, &viewport);
    context->VSSetShader(vs.Get(), nullptr, 0);
    context->PSSetShader(ps.Get(), nullptr, 0);
    context->PSSetShaderResources(0, 1, srv.GetAddressOf());
    context->PSSetSamplers(0, 1, sampler.GetAddressOf());
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    unsigned counts[2]{};
    for (int pass = 0; pass < 2; ++pass) {
        const float black[4] = {0, 0, 0, 1};
        context->ClearRenderTargetView(rtv.Get(), black);
        context->RSSetState(pass == 0 ? nullptr : rasterizer.Get());
        context->Draw(4, 0);
        context->CopyResource(readback.Get(), output.Get());
        D3D11_MAPPED_SUBRESOURCE mapped{};
        REQUIRE(context->Map(readback.Get(), 0, D3D11_MAP_READ, 0, &mapped));
        for (unsigned y = 0; y < 16; ++y) {
            const auto *row = static_cast<const unsigned char *>(mapped.pData) + y * mapped.RowPitch;
            for (unsigned x = 0; x < 16; ++x) if (row[x * 4] == 255) ++counts[pass];
        }
        context->Unmap(readback.Get(), 0);
    }
    std::printf("D3D11 WARP white pixels: default=%u; explicit-no-cull=%u / 256\n", counts[0], counts[1]);
    if (counts[0] != 0 || counts[1] != 256) {
        std::fprintf(stderr, "D3D11_RASTER_COVERAGE_MISMATCH\n");
        return 1;
    }
    return 0;
}
