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
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi1_2.h>

#include <SDL.h>
#include <SDL_syswm.h>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "librashader/d3d11_bridge.h"
#include "librashader/frame_conversion.h"
#include "librashader/librashader_loader.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"

struct VAEG_D3D11_STATE {
	HWND hwnd;
	ID3D11Device *device;
	ID3D11DeviceContext *context;
	IDXGISwapChain1 *swap_chain;
	ID3D11RenderTargetView *render_target;
	ID3D11Texture2D *source_texture;
	ID3D11ShaderResourceView *source_view;
	ID3D11VertexShader *vertex_shader;
	ID3D11PixelShader *pixel_shader;
	ID3D11SamplerState *sampler;
	uint8_t *upload_buffer;
	size_t upload_capacity;
	uint32_t source_width;
	uint32_t source_height;
	uint32_t upload_pitch;
	uint32_t drawable_width;
	uint32_t drawable_height;
	libra_instance_t librashader;
	libra_d3d11_filter_chain_t filter_chain;
	bool filter_enabled;
	bool filter_first_frame;
	bool gui_ready;
	ID3D11RasterizerState *pass_through_rasterizer;
	char *error;
	D3D11_VIEWPORT output_viewport;
};

template <typename T>
static void vaeg_d3d11_release(T **object) {
	if ((object != nullptr) && (*object != nullptr)) {
		(*object)->Release();
		*object = nullptr;
	}
}

#include "librashader/d3d11_pass_through.h"

static void vaeg_d3d11_release_state(VAEG_D3D11_STATE *state) {
	if (state == nullptr) {
		return;
	}
	if (state->gui_ready && ImGui::GetCurrentContext()) {
		ImGui_ImplDX11_Shutdown();
	}
	if (state->context) state->context->ClearState();
	if ((state->filter_chain != nullptr) &&
	    (state->librashader.d3d11_filter_chain_free != nullptr)) {
		(void)state->librashader.d3d11_filter_chain_free(&state->filter_chain);
	}
	vaeg_d3d11_release(&state->render_target);
	vaeg_d3d11_release(&state->source_view);
	vaeg_d3d11_release(&state->source_texture);
	vaeg_d3d11_release(&state->sampler);
	vaeg_d3d11_release(&state->pixel_shader);
	vaeg_d3d11_release(&state->pass_through_rasterizer);
	vaeg_d3d11_release(&state->vertex_shader);
	vaeg_d3d11_release(&state->swap_chain);
	if (state->context) state->context->Flush();
	vaeg_d3d11_release(&state->context);
	vaeg_d3d11_release(&state->device);
	free(state->upload_buffer);
	free(state);
}

static void vaeg_d3d11_report_librashader_error(VAEG_D3D11_STATE *state,
                                                 libra_error_t error, const char *operation) {
	if (error == nullptr) {
		return;
	}
	fprintf(stderr, "librashader D3D11 %s failed\n", operation);
	char *detail = nullptr;
	(void)state->librashader.error_write(error, &detail);
	snprintf(state->error, 512, "%s: %s", operation, detail ? detail : "librashader error");
	if (detail) (void)state->librashader.error_free_string(&detail);
	if (state->librashader.error_print != nullptr) {
		(void)state->librashader.error_print(error);
	}
	if (state->librashader.error_free != nullptr) {
		(void)state->librashader.error_free(&error);
	}
}

static int vaeg_d3d11_device_lost(HRESULT result) {
	return (result == DXGI_ERROR_DEVICE_REMOVED) || (result == DXGI_ERROR_DEVICE_RESET) ||
	       (result == DXGI_ERROR_DRIVER_INTERNAL_ERROR);
}

static int vaeg_d3d11_create_filter_chain(VAEG_D3D11_STATE *state, const char *preset_path) {
	libra_shader_preset_t preset;
	filter_chain_d3d11_opt_t options{};
	libra_error_t error;

	if ((state == nullptr) || (preset_path == nullptr) || (preset_path[0] == '\0')) {
		return 0;
	}
	state->librashader = vaeg_librashader_load_instance(state->error, 512);
	if (!state->librashader.instance_loaded ||
	    (state->librashader.d3d11_filter_chain_create == nullptr)) {
		fprintf(stderr, "librashader D3D11 runtime unavailable\n");
		return 0;
	}
	preset = nullptr;
	error = state->librashader.preset_create(preset_path, &preset);
	if ((error != nullptr) || (preset == nullptr)) {
		vaeg_d3d11_report_librashader_error(state, error, "preset creation");
		return 0;
	}
	options.version = LIBRASHADER_CURRENT_VERSION;
	error = state->librashader.d3d11_filter_chain_create(
		&preset, state->device, &options, &state->filter_chain);
	if ((error != nullptr) || (state->filter_chain == nullptr)) {
		vaeg_d3d11_report_librashader_error(state, error, "filter-chain creation");
		return 0;
	}
	state->filter_enabled = true;
	state->filter_first_frame = true;
	return 1;
}

static int vaeg_d3d11_create_output(VAEG_D3D11_STATE *state) {
	ID3D11Texture2D *back_buffer = nullptr;
	HRESULT result;

	vaeg_d3d11_release(&state->render_target);
	result = state->swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
	if (FAILED(result)) {
		return 0;
	}
	result = state->device->CreateRenderTargetView(back_buffer, nullptr, &state->render_target);
	back_buffer->Release();
	return SUCCEEDED(result) ? 1 : 0;
}

static int vaeg_d3d11_create_shaders(VAEG_D3D11_STATE *state) {
	const D3D11_RASTERIZER_DESC rasterizer = vaeg_d3d11_pass_through_rasterizer_desc();
	if (FAILED(state->device->CreateRasterizerState(&rasterizer, &state->pass_through_rasterizer))) {
		return 0;
	}
	ID3DBlob *vertex_blob = nullptr;
	ID3DBlob *pixel_blob = nullptr;
	ID3DBlob *errors = nullptr;
	HRESULT result;

	result = D3DCompile(vaeg_d3d11_shader_source, sizeof(vaeg_d3d11_shader_source) - 1,
	                    "vaeg_d3d11_pass_through.hlsl", nullptr, nullptr, "vs_main", "vs_4_0",
	                    0, 0, &vertex_blob, &errors);
	vaeg_d3d11_release(&errors);
	if (FAILED(result)) {
		return 0;
	}
	result = state->device->CreateVertexShader(vertex_blob->GetBufferPointer(),
	                                            vertex_blob->GetBufferSize(), nullptr,
	                                            &state->vertex_shader);
	vaeg_d3d11_release(&vertex_blob);
	if (FAILED(result)) {
		return 0;
	}
	result = D3DCompile(vaeg_d3d11_shader_source, sizeof(vaeg_d3d11_shader_source) - 1,
	                    "vaeg_d3d11_pass_through.hlsl", nullptr, nullptr, "ps_main", "ps_4_0",
	                    0, 0, &pixel_blob, &errors);
	vaeg_d3d11_release(&errors);
	if (FAILED(result)) {
		return 0;
	}
	result = state->device->CreatePixelShader(pixel_blob->GetBufferPointer(),
	                                           pixel_blob->GetBufferSize(), nullptr,
	                                           &state->pixel_shader);
	vaeg_d3d11_release(&pixel_blob);
	if (FAILED(result)) {
		return 0;
	}
	D3D11_SAMPLER_DESC sampler_desc{};
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampler_desc.MinLOD = 0.0f;
	sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
	return SUCCEEDED(state->device->CreateSamplerState(&sampler_desc, &state->sampler)) ? 1 : 0;
}

static int vaeg_d3d11_ensure_source(VAEG_D3D11_STATE *state, uint32_t width, uint32_t height) {
	D3D11_TEXTURE2D_DESC descriptor{};
	D3D11_SHADER_RESOURCE_VIEW_DESC view_descriptor{};
	const size_t required_capacity = static_cast<size_t>(width) * height * 4U;
	HRESULT result;

	if ((width == 0) || (height == 0) ||
	    (required_capacity / 4U != static_cast<size_t>(width) * height)) {
		return 0;
	}
	if ((state->source_texture != nullptr) && (state->source_width == width) &&
	    (state->source_height == height) && (state->upload_capacity >= required_capacity)) {
		return 1;
	}
	vaeg_d3d11_release(&state->source_view);
	vaeg_d3d11_release(&state->source_texture);
	if (required_capacity > state->upload_capacity) {
		uint8_t *new_buffer = static_cast<uint8_t *>(realloc(state->upload_buffer, required_capacity));
		if (new_buffer == nullptr) {
			return 0;
		}
		state->upload_buffer = new_buffer;
		state->upload_capacity = required_capacity;
	}
	descriptor.Width = width;
	descriptor.Height = height;
	descriptor.MipLevels = 1;
	descriptor.ArraySize = 1;
	descriptor.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	descriptor.SampleDesc.Count = 1;
	descriptor.Usage = D3D11_USAGE_DYNAMIC;
	descriptor.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	descriptor.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	result = state->device->CreateTexture2D(&descriptor, nullptr, &state->source_texture);
	if (FAILED(result)) {
		return 0;
	}
	view_descriptor.Format = descriptor.Format;
	view_descriptor.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	view_descriptor.Texture2D.MipLevels = 1;
	result = state->device->CreateShaderResourceView(state->source_texture, &view_descriptor,
	                                                   &state->source_view);
	if (FAILED(result)) {
		vaeg_d3d11_release(&state->source_texture);
		return 0;
	}
	state->source_width = width;
	state->source_height = height;
	state->upload_pitch = width * 4U;
	return 1;
}

static void vaeg_d3d11_viewport(const VAEG_D3D11_STATE *state, const VAEG_FRAME_INPUT *frame,
                                D3D11_VIEWPORT *viewport) {
	if (state->output_viewport.Width > 0 && state->output_viewport.Height > 0) {
		*viewport = state->output_viewport;
		return;
	}
	double output_aspect = static_cast<double>(state->drawable_width) /
	                       static_cast<double>(state->drawable_height);
	double source_aspect = static_cast<double>(frame->source_aspect_width) /
	                       static_cast<double>(frame->source_aspect_height);
	viewport->TopLeftX = 0.0f;
	viewport->TopLeftY = 0.0f;
	viewport->Width = static_cast<float>(state->drawable_width);
	viewport->Height = static_cast<float>(state->drawable_height);
	viewport->MinDepth = 0.0f;
	viewport->MaxDepth = 1.0f;
	if (source_aspect > output_aspect) {
		viewport->Height = static_cast<float>(viewport->Width / source_aspect);
		viewport->TopLeftY = (static_cast<float>(state->drawable_height) - viewport->Height) * 0.5f;
	} else {
		viewport->Width = static_cast<float>(viewport->Height * source_aspect);
		viewport->TopLeftX = (static_cast<float>(state->drawable_width) - viewport->Width) * 0.5f;
	}
}

extern "C" int vaeg_d3d11_bridge_initialize(void *host_window, const char *preset_path,
                                               int enable_filter, VAEG_D3D11_BRIDGE *bridge) {
	SDL_SysWMinfo window_info;
	VAEG_D3D11_STATE *state;
	DXGI_SWAP_CHAIN_DESC1 swap_chain_descriptor{};
	IDXGIDevice *dxgi_device = nullptr;
	IDXGIAdapter *adapter = nullptr;
	IDXGIFactory2 *factory = nullptr;
	D3D_FEATURE_LEVEL feature_level;
	const D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0};
	HRESULT result;

	if ((host_window == nullptr) || (bridge == nullptr)) {
		return 0;
	}
	bridge->state = nullptr;
	window_info = {};
	SDL_VERSION(&window_info.version);
	if (!SDL_GetWindowWMInfo(static_cast<SDL_Window *>(host_window), &window_info) ||
	    (window_info.subsystem != SDL_SYSWM_WINDOWS)) {
		return 0;
	}
	state = static_cast<VAEG_D3D11_STATE *>(calloc(1, sizeof(*state)));
	if (state == nullptr) {
		return 0;
	}
	state->hwnd = window_info.info.win.window;
	state->error = bridge->error;
	result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
	                           D3D11_CREATE_DEVICE_BGRA_SUPPORT, feature_levels,
	                           ARRAYSIZE(feature_levels), D3D11_SDK_VERSION, &state->device,
	                           &feature_level, &state->context);
	if (result == E_INVALIDARG) {
		const D3D_FEATURE_LEVEL fallback_level = D3D_FEATURE_LEVEL_11_0;
		result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
		                           D3D11_CREATE_DEVICE_BGRA_SUPPORT, &fallback_level, 1,
		                           D3D11_SDK_VERSION, &state->device, &feature_level,
		                           &state->context);
	}
	if (FAILED(result)) {
		vaeg_d3d11_release_state(state);
		return 0;
	}
	result = state->device->QueryInterface(IID_PPV_ARGS(&dxgi_device));
	if (FAILED(result)) {
		vaeg_d3d11_release_state(state);
		return 0;
	}
	result = dxgi_device->GetAdapter(&adapter);
	dxgi_device->Release();
	if (FAILED(result)) {
		vaeg_d3d11_release_state(state);
		return 0;
	}
	result = adapter->GetParent(IID_PPV_ARGS(&factory));
	adapter->Release();
	if (FAILED(result)) {
		vaeg_d3d11_release_state(state);
		return 0;
	}
	swap_chain_descriptor.Width = 0;
	swap_chain_descriptor.Height = 0;
	swap_chain_descriptor.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swap_chain_descriptor.SampleDesc.Count = 1;
	swap_chain_descriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_descriptor.BufferCount = 2;
	swap_chain_descriptor.Scaling = DXGI_SCALING_STRETCH;
	swap_chain_descriptor.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swap_chain_descriptor.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	result = factory->CreateSwapChainForHwnd(state->device, state->hwnd, &swap_chain_descriptor,
	                                          nullptr, nullptr, &state->swap_chain);
	factory->MakeWindowAssociation(state->hwnd, DXGI_MWA_NO_ALT_ENTER);
	factory->Release();
	if (FAILED(result) || !vaeg_d3d11_create_shaders(state) ||
	    !vaeg_d3d11_create_output(state) ||
	    ((enable_filter != 0) && !vaeg_d3d11_create_filter_chain(state, preset_path))) {
		vaeg_d3d11_release_state(state);
		return 0;
	}
	bridge->state = state;
	return 1;
}

extern "C" VAEG_D3D11_BRIDGE_RESULT vaeg_d3d11_bridge_set_drawable_size(
	VAEG_D3D11_BRIDGE *bridge, uint32_t width, uint32_t height) {
	VAEG_D3D11_STATE *state;
	HRESULT result;

	if ((bridge == nullptr) || (bridge->state == nullptr)) {
		return VAEG_D3D11_BRIDGE_INVALID_ARGUMENT;
	}
	if ((width == 0) || (height == 0)) {
		return VAEG_D3D11_BRIDGE_NO_OUTPUT;
	}
	state = static_cast<VAEG_D3D11_STATE *>(bridge->state);
	if ((state->drawable_width == width) && (state->drawable_height == height) &&
	    (state->render_target != nullptr)) {
		return VAEG_D3D11_BRIDGE_OK;
	}
	// Unbind the old back buffer before ResizeBuffers releases its references.
	state->context->OMSetRenderTargets(0, nullptr, nullptr);
	vaeg_d3d11_release(&state->render_target);
	result = state->swap_chain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
	if (FAILED(result)) {
		return vaeg_d3d11_device_lost(result) ? VAEG_D3D11_BRIDGE_DEVICE_LOST
		                                      : VAEG_D3D11_BRIDGE_RESOURCE_FAILURE;
	}
	if (!vaeg_d3d11_create_output(state)) {
		return VAEG_D3D11_BRIDGE_RESOURCE_FAILURE;
	}
	state->drawable_width = width;
	state->drawable_height = height;
	return VAEG_D3D11_BRIDGE_OK;
}

extern "C" VAEG_D3D11_BRIDGE_RESULT vaeg_d3d11_bridge_set_filter_enabled(
	VAEG_D3D11_BRIDGE *bridge, int enabled) {
	VAEG_D3D11_STATE *state;

	if ((bridge == nullptr) || (bridge->state == nullptr)) {
		return VAEG_D3D11_BRIDGE_INVALID_ARGUMENT;
	}
	state = static_cast<VAEG_D3D11_STATE *>(bridge->state);
	if ((enabled != 0) && (state->filter_chain == nullptr)) {
		return VAEG_D3D11_BRIDGE_RESOURCE_FAILURE;
	}
	if ((enabled != 0) && !state->filter_enabled) {
		state->filter_first_frame = true;
	}
	state->filter_enabled = (enabled != 0);
	return VAEG_D3D11_BRIDGE_OK;
}

extern "C" VAEG_D3D11_BRIDGE_RESULT vaeg_d3d11_bridge_set_filter_parameter(
	VAEG_D3D11_BRIDGE *bridge, const char *name, float value) {
	VAEG_D3D11_STATE *state;
	libra_error_t error;

	if ((bridge == nullptr) || (bridge->state == nullptr) || (name == nullptr)) {
		return VAEG_D3D11_BRIDGE_INVALID_ARGUMENT;
	}
	state = static_cast<VAEG_D3D11_STATE *>(bridge->state);
	if ((state->filter_chain == nullptr) ||
	    (state->librashader.d3d11_filter_chain_set_param == nullptr)) {
		return VAEG_D3D11_BRIDGE_RESOURCE_FAILURE;
	}
	error = state->librashader.d3d11_filter_chain_set_param(&state->filter_chain, name, value);
	if (error != nullptr) {
		vaeg_d3d11_report_librashader_error(state, error, "parameter update");
		return VAEG_D3D11_BRIDGE_RESOURCE_FAILURE;
	}
	state->filter_first_frame = true;
	return VAEG_D3D11_BRIDGE_OK;
}

extern "C" VAEG_D3D11_BRIDGE_RESULT vaeg_d3d11_bridge_present(
	VAEG_D3D11_BRIDGE *bridge, const VAEG_FRAME_INPUT *frame) {
	VAEG_D3D11_STATE *state;
	D3D11_MAPPED_SUBRESOURCE mapped{};
	D3D11_VIEWPORT viewport;
	libra_viewport_t libra_viewport;
	frame_d3d11_opt_t filter_options{};
	RECT client_rect;
	ID3D11RenderTargetView *render_target;
	const float clear_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	HRESULT result;

	if ((bridge == nullptr) || (bridge->state == nullptr) || (frame == nullptr)) {
		return VAEG_D3D11_BRIDGE_INVALID_ARGUMENT;
	}
	if (vaeg_frame_input_validate(frame) != VAEG_FRAME_INPUT_OK) {
		return VAEG_D3D11_BRIDGE_INVALID_FRAME;
	}
	state = static_cast<VAEG_D3D11_STATE *>(bridge->state);
	if (!GetClientRect(state->hwnd, &client_rect) || (client_rect.right <= 0) ||
	    (client_rect.bottom <= 0)) {
		return VAEG_D3D11_BRIDGE_NO_OUTPUT;
	}
	if ((static_cast<uint32_t>(client_rect.right) != state->drawable_width) ||
	    (static_cast<uint32_t>(client_rect.bottom) != state->drawable_height)) {
		const VAEG_D3D11_BRIDGE_RESULT resize_result = vaeg_d3d11_bridge_set_drawable_size(
			bridge, static_cast<uint32_t>(client_rect.right), static_cast<uint32_t>(client_rect.bottom));
		if (resize_result != VAEG_D3D11_BRIDGE_OK) {
			return resize_result;
		}
	}
	if (!vaeg_d3d11_ensure_source(state, frame->width, frame->height)) {
		return VAEG_D3D11_BRIDGE_RESOURCE_FAILURE;
	}
	if (vaeg_frame_convert_rgba8888(frame, state->upload_buffer, state->upload_pitch,
	                                state->upload_capacity) != VAEG_FRAME_CONVERSION_OK) {
		return VAEG_D3D11_BRIDGE_INVALID_FRAME;
	}
	result = state->context->Map(state->source_texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (FAILED(result)) {
		return vaeg_d3d11_device_lost(result) ? VAEG_D3D11_BRIDGE_DEVICE_LOST
		                                      : VAEG_D3D11_BRIDGE_RESOURCE_FAILURE;
	}
	for (uint32_t row = 0; row < frame->height; row++) {
		memcpy(static_cast<uint8_t *>(mapped.pData) + row * mapped.RowPitch,
		       state->upload_buffer + row * state->upload_pitch, state->upload_pitch);
	}
	state->context->Unmap(state->source_texture, 0);
	vaeg_d3d11_viewport(state, frame, &viewport);
	render_target = state->render_target;
	state->context->OMSetRenderTargets(1, &render_target, nullptr);
	state->context->ClearRenderTargetView(render_target, clear_color);
	state->context->RSSetViewports(1, &viewport);
	if (state->filter_enabled) {
		libra_viewport.x = viewport.TopLeftX;
		libra_viewport.y = viewport.TopLeftY;
		libra_viewport.width = static_cast<uint32_t>(viewport.Width);
		libra_viewport.height = static_cast<uint32_t>(viewport.Height);
		memset(&filter_options, 0, sizeof(filter_options));
		filter_options.version = LIBRASHADER_CURRENT_VERSION;
		filter_options.clear_history = state->filter_first_frame;
		filter_options.frame_direction = 1;
		filter_options.rotation = 0;
		filter_options.total_subframes = 1;
		filter_options.current_subframe = 1;
		filter_options.aspect_ratio = static_cast<float>(frame->source_aspect_width) /
		                              static_cast<float>(frame->source_aspect_height);
		filter_options.frames_per_second =
			static_cast<float>(frame->source_frame_rate_numerator) /
			static_cast<float>(frame->source_frame_rate_denominator);
		filter_options.frametime_delta =
			static_cast<uint32_t>(frame->frame_time_delta_ns / 1000000U);
		libra_error_t error = state->librashader.d3d11_filter_chain_frame(
			&state->filter_chain, state->context, frame->frame_number, state->source_view, render_target,
			&libra_viewport, nullptr, &filter_options);
		if (error != nullptr) {
			vaeg_d3d11_report_librashader_error(state, error, "frame rendering");
			return VAEG_D3D11_BRIDGE_FILTER_FAILURE;
		}
		state->filter_first_frame = false;
	} else {
		state->context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
		state->context->OMSetDepthStencilState(nullptr, 0);
		state->context->RSSetState(state->pass_through_rasterizer);
		state->context->IASetInputLayout(nullptr);
		state->context->GSSetShader(nullptr, nullptr, 0);
		state->context->VSSetShader(state->vertex_shader, nullptr, 0);
		state->context->PSSetShader(state->pixel_shader, nullptr, 0);
		state->context->PSSetShaderResources(0, 1, &state->source_view);
		state->context->PSSetSamplers(0, 1, &state->sampler);
		state->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		state->context->Draw(4, 0);
	}
	ID3D11ShaderResourceView *no_source = nullptr;
	state->context->PSSetShaderResources(0, 1, &no_source);
	if (state->gui_ready && ImGui::GetCurrentContext() && ImGui::GetDrawData()) {
		state->context->OMSetRenderTargets(1, &render_target, nullptr);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}
	if (bridge->capture) {
		VAEG_OUTPUT_CAPTURE *capture = bridge->capture;
		ID3D11Texture2D *back = nullptr, *staging = nullptr;
		D3D11_TEXTURE2D_DESC desc{};
		D3D11_MAPPED_SUBRESOURCE mapped{};
		if (SUCCEEDED(state->swap_chain->GetBuffer(0, IID_PPV_ARGS(&back)))) {
			back->GetDesc(&desc);
			if (capture->pixels && capture->width == desc.Width && capture->height == desc.Height &&
			    capture->pitch_bytes >= desc.Width * 4 && desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM) {
				desc.Usage = D3D11_USAGE_STAGING;
				desc.BindFlags = 0;
				desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
				desc.MiscFlags = 0;
				if (SUCCEEDED(state->device->CreateTexture2D(&desc, nullptr, &staging))) {
					state->context->CopyResource(staging, back);
					if (SUCCEEDED(state->context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped))) {
						for (UINT y = 0; y < desc.Height; ++y) {
							const auto *src = static_cast<const unsigned char *>(mapped.pData) + y * mapped.RowPitch;
							auto *dst = static_cast<unsigned char *>(capture->pixels) + y * capture->pitch_bytes;
							for (UINT x = 0; x < desc.Width; ++x) {
								dst[4*x] = src[4*x+2]; dst[4*x+1] = src[4*x+1];
								dst[4*x+2] = src[4*x]; dst[4*x+3] = 255;
							}
						}
						state->context->Unmap(staging, 0);
						capture->complete = 1;
					}
					staging->Release();
				}
			}
			back->Release();
		}
	}
	result = state->swap_chain->Present(1, 0);
	if (result == DXGI_STATUS_OCCLUDED) {
		return VAEG_D3D11_BRIDGE_NO_OUTPUT;
	}
	if (vaeg_d3d11_device_lost(result)) {
		return VAEG_D3D11_BRIDGE_DEVICE_LOST;
	}
	return SUCCEEDED(result) ? VAEG_D3D11_BRIDGE_OK : VAEG_D3D11_BRIDGE_RESOURCE_FAILURE;
}

extern "C" void vaeg_d3d11_bridge_shutdown(VAEG_D3D11_BRIDGE *bridge) {
	VAEG_D3D11_STATE *state;

	if ((bridge == nullptr) || (bridge->state == nullptr)) {
		return;
	}
	state = static_cast<VAEG_D3D11_STATE *>(bridge->state);
	bridge->state = nullptr;
	vaeg_d3d11_release_state(state);
}

extern "C" int vaeg_d3d11_bridge_gui_prepare(VAEG_D3D11_BRIDGE *bridge) {
	if (!bridge || !bridge->state || !ImGui::GetCurrentContext()) return 0;
	auto *state = static_cast<VAEG_D3D11_STATE *>(bridge->state);
	if (!state->gui_ready) {
		if (!ImGui_ImplDX11_Init(state->device, state->context)) return 0;
		if (!ImGui_ImplDX11_CreateDeviceObjects()) {
			ImGui_ImplDX11_Shutdown();
			return 0;
		}
		state->gui_ready = true;
	}
	ImGui_ImplDX11_NewFrame();
	return 1;
}

extern "C" void vaeg_d3d11_bridge_gui_shutdown(VAEG_D3D11_BRIDGE *bridge) {
	if (!bridge || !bridge->state) return;
	auto *state = static_cast<VAEG_D3D11_STATE *>(bridge->state);
	if (state->gui_ready && ImGui::GetCurrentContext()) ImGui_ImplDX11_Shutdown();
	state->gui_ready = false;
}

extern "C" void vaeg_d3d11_bridge_set_output_viewport(VAEG_D3D11_BRIDGE *bridge,
                                                      int x, int y, int width, int height) {
	if (!bridge || !bridge->state) return;
	auto *state = static_cast<VAEG_D3D11_STATE *>(bridge->state);
	state->output_viewport = {static_cast<float>(x), static_cast<float>(y),
	                         static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f};
}
