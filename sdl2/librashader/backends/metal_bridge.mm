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
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstddef>
#include <cstdint>

#include <SDL_metal.h>

#include "imgui.h"

#include "librashader/frame_conversion.h"
#include "librashader/librashader_loader.h"
#include "librashader/metal_bridge.h"

struct VAEG_METAL_STATE {
	SDL_MetalView view;
	CAMetalLayer *layer;
	id<MTLDevice> device;
	id<MTLCommandQueue> queue;
	id<MTLRenderPipelineState> pipeline;
	id<MTLRenderPipelineState> gui_pipeline;
	id<MTLSamplerState> gui_sampler;
	id<MTLBuffer> gui_vertex_buffer;
	id<MTLBuffer> gui_index_buffer;
	size_t gui_vertex_capacity;
	size_t gui_index_capacity;
	id<MTLTexture> source_texture;
	id<MTLTexture> filter_output_texture;
	id<MTLTexture> capture_texture;
	uint8_t *upload_buffer;
	size_t upload_capacity;
	uint32_t source_width;
	uint32_t source_height;
	uint32_t upload_pitch;
	NSUInteger filter_output_width;
	NSUInteger filter_output_height;
	MTLPixelFormat filter_output_pixel_format;
	NSUInteger capture_width;
	NSUInteger capture_height;
	MTLPixelFormat capture_pixel_format;
	libra_instance_t librashader;
	libra_mtl_filter_chain_t filter_chain;
	bool filter_enabled;
	bool filter_first_frame;
	MTLViewport output_viewport;
	bool output_viewport_valid;
};

struct VAEG_METAL_GUI_TEXTURE {
	id<MTLTexture> texture;
};

static const char vaeg_metal_passthrough_shader[] = R"metal(
#include <metal_stdlib>
using namespace metal;

struct VAEGVertexOut {
    float4 position [[position]];
    float2 texcoord;
};

vertex VAEGVertexOut vaeg_metal_vertex(uint vertex_id [[vertex_id]]) {
    const float2 positions[4] = {
        float2(-1.0, -1.0), float2(1.0, -1.0),
        float2(-1.0, 1.0), float2(1.0, 1.0)
    };
    const float2 texcoords[4] = {
        float2(0.0, 1.0), float2(1.0, 1.0),
        float2(0.0, 0.0), float2(1.0, 0.0)
    };
    VAEGVertexOut output;
    output.position = float4(positions[vertex_id], 0.0, 1.0);
    output.texcoord = texcoords[vertex_id];
    return output;
}

fragment float4 vaeg_metal_fragment(
    VAEGVertexOut input [[stage_in]],
    texture2d<float, access::sample> source [[texture(0)]]) {
    constexpr sampler source_sampler(filter::nearest, address::clamp_to_edge);
    return source.sample(source_sampler, input.texcoord);
}
)metal";

static const char vaeg_metal_imgui_shader[] = R"metal(
#include <metal_stdlib>
using namespace metal;

struct VAEGImGuiVertex {
    float2 position [[attribute(0)]];
    float2 uv [[attribute(1)]];
    float4 color [[attribute(2)]];
};

struct VAEGImGuiUniforms {
    float4x4 projection;
};

struct VAEGImGuiVertexOut {
    float4 position [[position]];
    float2 uv;
    float4 color;
};

vertex VAEGImGuiVertexOut vaeg_imgui_vertex(
    VAEGImGuiVertex input [[stage_in]],
    constant VAEGImGuiUniforms &uniforms [[buffer(1)]]) {
    VAEGImGuiVertexOut output;
    output.position = uniforms.projection * float4(input.position, 0.0, 1.0);
    output.uv = input.uv;
    output.color = input.color;
    return output;
}

fragment float4 vaeg_imgui_fragment(
    VAEGImGuiVertexOut input [[stage_in]],
    texture2d<float, access::sample> texture [[texture(0)]],
    sampler texture_sampler [[sampler(0)]]) {
    return input.color * texture.sample(texture_sampler, input.uv);
}
)metal";

static void vaeg_metal_release_state(VAEG_METAL_STATE *state) {
	if (state == nullptr) {
		return;
	}
	if (state->source_texture != nil) {
		[state->source_texture release];
	}
	if (state->filter_output_texture != nil) {
		[state->filter_output_texture release];
	}
	if (state->capture_texture != nil) {
		[state->capture_texture release];
	}
	if ((state->filter_chain != nullptr) &&
	    (state->librashader.mtl_filter_chain_free != nullptr)) {
		(void)state->librashader.mtl_filter_chain_free(&state->filter_chain);
	}
	if (state->pipeline != nil) {
		[state->pipeline release];
	}
	if (state->gui_pipeline != nil) {
		[state->gui_pipeline release];
	}
	if (state->gui_sampler != nil) {
		[state->gui_sampler release];
	}
	if (state->gui_vertex_buffer != nil) {
		[state->gui_vertex_buffer release];
	}
	if (state->gui_index_buffer != nil) {
		[state->gui_index_buffer release];
	}
	if (state->queue != nil) {
		[state->queue release];
	}
	if (state->device != nil) {
		[state->device release];
	}
	if (state->view != nullptr) {
		SDL_Metal_DestroyView(state->view);
	}
	free(state->upload_buffer);
	free(state);
}

static void vaeg_metal_report_initialization_failure(const char *stage, NSError *error);

static void vaeg_metal_release_gui_texture(ImTextureData *texture) {
	VAEG_METAL_GUI_TEXTURE *backend_texture;

	if ((texture == nullptr) || (texture->BackendUserData == nullptr)) {
		return;
	}
	backend_texture = static_cast<VAEG_METAL_GUI_TEXTURE *>(texture->BackendUserData);
	if (backend_texture->texture != nil) {
		[backend_texture->texture release];
	}
	delete backend_texture;
	texture->BackendUserData = nullptr;
	texture->SetTexID(ImTextureID_Invalid);
}

static int vaeg_metal_create_gui_pipeline(VAEG_METAL_STATE *state) {
	MTLRenderPipelineDescriptor *descriptor;
	MTLVertexDescriptor *vertex_descriptor;
	MTLSamplerDescriptor *sampler_descriptor;
	id<MTLLibrary> library;
	id<MTLFunction> vertex_function;
	id<MTLFunction> fragment_function;
	NSError *error = nil;

	if (state->gui_pipeline != nil) {
		return 1;
	}
	library = [state->device newLibraryWithSource:
	                                      [NSString stringWithUTF8String:vaeg_metal_imgui_shader]
	                                         options:nil error:&error];
	if (library == nil) {
		vaeg_metal_report_initialization_failure("ImGui Metal shader", error);
		return 0;
	}
	vertex_function = [library newFunctionWithName:@"vaeg_imgui_vertex"];
	fragment_function = [library newFunctionWithName:@"vaeg_imgui_fragment"];
	if ((vertex_function == nil) || (fragment_function == nil)) {
		fprintf(stderr, "librashader Metal ImGui shader functions unavailable\n");
		[fragment_function release];
		[vertex_function release];
		[library release];
		return 0;
	}
	vertex_descriptor = [[MTLVertexDescriptor alloc] init];
	vertex_descriptor.attributes[0].format = MTLVertexFormatFloat2;
	vertex_descriptor.attributes[0].offset = offsetof(ImDrawVert, pos);
	vertex_descriptor.attributes[0].bufferIndex = 0;
	vertex_descriptor.attributes[1].format = MTLVertexFormatFloat2;
	vertex_descriptor.attributes[1].offset = offsetof(ImDrawVert, uv);
	vertex_descriptor.attributes[1].bufferIndex = 0;
	vertex_descriptor.attributes[2].format = MTLVertexFormatUChar4Normalized;
	vertex_descriptor.attributes[2].offset = offsetof(ImDrawVert, col);
	vertex_descriptor.attributes[2].bufferIndex = 0;
	vertex_descriptor.layouts[0].stride = sizeof(ImDrawVert);
	vertex_descriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
	vertex_descriptor.layouts[0].stepRate = 1;
	descriptor = [[MTLRenderPipelineDescriptor alloc] init];
	descriptor.vertexFunction = vertex_function;
	descriptor.fragmentFunction = fragment_function;
	descriptor.vertexDescriptor = vertex_descriptor;
	descriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
	descriptor.colorAttachments[0].blendingEnabled = YES;
	descriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
	descriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
	descriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
	descriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
	descriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
	descriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
	state->gui_pipeline = [state->device newRenderPipelineStateWithDescriptor:descriptor error:&error];
	sampler_descriptor = [[MTLSamplerDescriptor alloc] init];
	sampler_descriptor.minFilter = MTLSamplerMinMagFilterLinear;
	sampler_descriptor.magFilter = MTLSamplerMinMagFilterLinear;
	sampler_descriptor.sAddressMode = MTLSamplerAddressModeClampToEdge;
	sampler_descriptor.tAddressMode = MTLSamplerAddressModeClampToEdge;
	state->gui_sampler = [state->device newSamplerStateWithDescriptor:sampler_descriptor];
	[sampler_descriptor release];
	[descriptor release];
	[vertex_descriptor release];
	[fragment_function release];
	[vertex_function release];
	[library release];
	if ((state->gui_pipeline == nil) || (state->gui_sampler == nil)) {
		vaeg_metal_report_initialization_failure("ImGui Metal pipeline", error);
		return 0;
	}
	return 1;
}

static void vaeg_metal_report_librashader_error(VAEG_METAL_STATE *state, libra_error_t error,
                                                 const char *operation) {
	if (error == nullptr) {
		return;
	}
	fprintf(stderr, "librashader Metal %s failed\n", operation);
	if (state->librashader.error_print != nullptr) {
		(void)state->librashader.error_print(error);
	}
	if (state->librashader.error_free != nullptr) {
		(void)state->librashader.error_free(&error);
	}
}

static void vaeg_metal_report_initialization_failure(const char *stage, NSError *error) {
	const char *detail = nullptr;

	if (error != nil) {
		detail = [[error localizedDescription] UTF8String];
	}
	fprintf(stderr, "librashader Metal initialization failed: %s%s%s\n", stage,
	        (detail != nullptr) ? ": " : "", (detail != nullptr) ? detail : "");
}

static int vaeg_metal_upload_gui_texture(VAEG_METAL_STATE *state, ImTextureData *data) {
	MTLTextureDescriptor *descriptor;
	id<MTLTexture> texture;
	VAEG_METAL_GUI_TEXTURE *backend_texture;
	unsigned char *converted = nullptr;
	const void *pixels;

	if ((data == nullptr) || (data->Width <= 0) || (data->Height <= 0) ||
	    ((data->Format != ImTextureFormat_RGBA32) && (data->Format != ImTextureFormat_Alpha8))) {
		return 0;
	}
	descriptor = [[MTLTextureDescriptor alloc] init];
	descriptor.textureType = MTLTextureType2D;
	descriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
	descriptor.width = data->Width;
	descriptor.height = data->Height;
	descriptor.mipmapLevelCount = 1;
	descriptor.usage = MTLTextureUsageShaderRead;
	descriptor.storageMode = MTLStorageModeShared;
	texture = [state->device newTextureWithDescriptor:descriptor];
	[descriptor release];
	if (texture == nil) {
		return 0;
	}
	pixels = data->GetPixels();
	if (data->Format == ImTextureFormat_Alpha8) {
		const size_t pixel_count = static_cast<size_t>(data->Width) * data->Height;
		converted = static_cast<unsigned char *>(malloc(pixel_count * 4U));
		if (converted == nullptr) {
			[texture release];
			return 0;
		}
		for (size_t i = 0; i < pixel_count; ++i) {
			converted[i * 4U + 0] = 255;
			converted[i * 4U + 1] = 255;
			converted[i * 4U + 2] = 255;
			converted[i * 4U + 3] = static_cast<const unsigned char *>(pixels)[i];
		}
		pixels = converted;
	}
	[texture replaceRegion:MTLRegionMake2D(0, 0, data->Width, data->Height)
	          mipmapLevel:0 withBytes:pixels bytesPerRow:data->Width * 4U];
	free(converted);
	backend_texture = new VAEG_METAL_GUI_TEXTURE();
	backend_texture->texture = texture;
	data->BackendUserData = backend_texture;
	data->SetTexID(static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(backend_texture)));
	data->SetStatus(ImTextureStatus_OK);
	return 1;
}

static int vaeg_metal_update_gui_textures(VAEG_METAL_STATE *state) {
	if (!ImGui::GetCurrentContext()) {
		return 1;
	}
	for (ImTextureData *data : ImGui::GetPlatformIO().Textures) {
		if (data->Status == ImTextureStatus_WantCreate ||
		    ((data->Status == ImTextureStatus_OK) && (data->TexID == ImTextureID_Invalid))) {
			if (!vaeg_metal_upload_gui_texture(state, data)) {
				return 0;
			}
		} else if (data->Status == ImTextureStatus_WantUpdates) {
			/* Re-uploading is bounded and rare: ImGui queues updates only for new glyph blocks. */
			vaeg_metal_release_gui_texture(data);
			data->SetStatus(ImTextureStatus_WantCreate);
			if (!vaeg_metal_upload_gui_texture(state, data)) {
				return 0;
			}
		} else if (data->Status == ImTextureStatus_WantDestroy) {
			vaeg_metal_release_gui_texture(data);
			data->SetStatus(ImTextureStatus_Destroyed);
		}
	}
	return 1;
}

static int vaeg_metal_create_filter_chain(VAEG_METAL_STATE *state, const char *preset_path) {
	libra_shader_preset_t preset;
	filter_chain_mtl_opt_t options;
	libra_error_t error;
	char load_error[512];

	if ((preset_path == nullptr) || (preset_path[0] == '\0')) {
		fprintf(stderr, "librashader Metal filter-chain skipped: preset path is empty\n");
		return 0;
	}
	state->librashader = vaeg_librashader_load_instance(load_error, sizeof(load_error));
	if (!state->librashader.instance_loaded) {
		fprintf(stderr, "librashader Metal filter-chain skipped: %s\n",
		        load_error[0] ? load_error : "API unavailable");
		return 0;
	}
	preset = nullptr;
	error = state->librashader.preset_create(preset_path, &preset);
	if ((error != nullptr) || (preset == nullptr)) {
		vaeg_metal_report_librashader_error(state, error, "preset creation");
		return 0;
	}
	memset(&options, 0, sizeof(options));
	options.version = LIBRASHADER_CURRENT_VERSION;
	error = state->librashader.mtl_filter_chain_create(
		&preset, state->queue, &options, &state->filter_chain);
	if ((error != nullptr) || (state->filter_chain == nullptr)) {
		vaeg_metal_report_librashader_error(state, error, "filter-chain creation");
		return 0;
	}
	state->filter_enabled = true;
	state->filter_first_frame = true;
	return 1;
}

static MTLViewport vaeg_metal_viewport(const VAEG_METAL_STATE *state,
                                       const VAEG_FRAME_INPUT *frame) {
	double output_aspect;
	double source_aspect;
	MTLViewport viewport;

	viewport.originX = 0.0;
	viewport.originY = 0.0;
	viewport.width = state->layer.drawableSize.width;
	viewport.height = state->layer.drawableSize.height;
	viewport.znear = 0.0;
	viewport.zfar = 1.0;
	if (state->output_viewport_valid) {
		return state->output_viewport;
	}
	if ((viewport.width <= 0.0) || (viewport.height <= 0.0)) {
		return viewport;
	}
	source_aspect = static_cast<double>(frame->source_aspect_width) /
	                static_cast<double>(frame->source_aspect_height);
	output_aspect = viewport.width / viewport.height;
	if (source_aspect > output_aspect) {
		const double fitted_height = viewport.width / source_aspect;
		viewport.originY = (viewport.height - fitted_height) * 0.5;
		viewport.height = fitted_height;
	} else {
		const double fitted_width = viewport.height * source_aspect;
		viewport.originX = (viewport.width - fitted_width) * 0.5;
		viewport.width = fitted_width;
	}
	return viewport;
}

static int vaeg_metal_ensure_source_texture(VAEG_METAL_STATE *state, uint32_t width,
                                             uint32_t height) {
	MTLTextureDescriptor *descriptor;
	const size_t required_capacity = static_cast<size_t>(width) * height * 4U;

	if ((width == 0) || (height == 0) || (required_capacity / 4U != static_cast<size_t>(width) * height)) {
		return 0;
	}
	if ((state->source_texture != nil) && (state->source_width == width) &&
	    (state->source_height == height) && (state->upload_capacity >= required_capacity)) {
		return 1;
	}
	if (state->source_texture != nil) {
		[state->source_texture release];
		state->source_texture = nil;
	}
	if (required_capacity > state->upload_capacity) {
		uint8_t *new_buffer = static_cast<uint8_t *>(realloc(state->upload_buffer, required_capacity));
		if (new_buffer == nullptr) {
			return 0;
		}
		state->upload_buffer = new_buffer;
		state->upload_capacity = required_capacity;
	}
	descriptor = [[MTLTextureDescriptor alloc] init];
	descriptor.textureType = MTLTextureType2D;
	descriptor.pixelFormat = MTLPixelFormatRGBA8Unorm;
	descriptor.width = width;
	descriptor.height = height;
	descriptor.mipmapLevelCount = 1;
	descriptor.usage = MTLTextureUsageShaderRead;
	descriptor.storageMode = MTLStorageModeShared;
	state->source_texture = [state->device newTextureWithDescriptor:descriptor];
	[descriptor release];
	if (state->source_texture == nil) {
		return 0;
	}
	state->source_width = width;
	state->source_height = height;
	state->upload_pitch = width * 4U;
	return 1;
}

static int vaeg_metal_ensure_filter_output_texture(VAEG_METAL_STATE *state, NSUInteger width,
                                                    NSUInteger height, MTLPixelFormat pixel_format) {
	MTLTextureDescriptor *descriptor;

	if ((width == 0) || (height == 0) || (pixel_format == MTLPixelFormatInvalid)) {
		return 0;
	}
	if ((state->filter_output_texture != nil) &&
	    (state->filter_output_width == width) &&
	    (state->filter_output_height == height) &&
	    (state->filter_output_pixel_format == pixel_format)) {
		return 1;
	}
	if (state->filter_output_texture != nil) {
		[state->filter_output_texture release];
		state->filter_output_texture = nil;
	}
	descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pixel_format
	                                                                    width:width
	                                                                   height:height
	                                                                mipmapped:NO];
	descriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite |
	                   MTLTextureUsageRenderTarget | MTLTextureUsagePixelFormatView;
	descriptor.storageMode = MTLStorageModePrivate;
	state->filter_output_texture = [state->device newTextureWithDescriptor:descriptor];
	[descriptor release];
	if (state->filter_output_texture == nil) {
		return 0;
	}
	state->filter_output_width = width;
	state->filter_output_height = height;
	state->filter_output_pixel_format = pixel_format;
	return 1;
}

static int vaeg_metal_ensure_capture_texture(VAEG_METAL_STATE *state, NSUInteger width,
                                              NSUInteger height, MTLPixelFormat pixel_format) {
	MTLTextureDescriptor *descriptor;

	if ((width == 0) || (height == 0) || (pixel_format != MTLPixelFormatBGRA8Unorm)) {
		return 0;
	}
	if ((state->capture_texture != nil) && (state->capture_width == width) &&
	    (state->capture_height == height) && (state->capture_pixel_format == pixel_format)) {
		return 1;
	}
	if (state->capture_texture != nil) {
		[state->capture_texture release];
		state->capture_texture = nil;
	}
	descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pixel_format
	                                                                    width:width
	                                                                   height:height
	                                                                mipmapped:NO];
	descriptor.usage = MTLTextureUsageShaderRead;
	descriptor.storageMode = MTLStorageModeShared;
	state->capture_texture = [state->device newTextureWithDescriptor:descriptor];
	[descriptor release];
	if (state->capture_texture == nil) {
		return 0;
	}
	state->capture_width = width;
	state->capture_height = height;
	state->capture_pixel_format = pixel_format;
	return 1;
}

static int vaeg_metal_capture_drawable(VAEG_METAL_STATE *state,
                                        id<MTLCommandBuffer> command_buffer,
                                        id<MTLTexture> drawable,
                                        VAEG_OUTPUT_CAPTURE *capture) {
	id<MTLBlitCommandEncoder> blit_encoder;

	if (capture != nullptr) {
		capture->complete = 0;
	}
	if ((capture == nullptr) || (capture->pixels == nullptr) ||
	    (capture->width != drawable.width) || (capture->height != drawable.height) ||
	    (capture->pitch_bytes < drawable.width * 4U) ||
	    !vaeg_metal_ensure_capture_texture(state, drawable.width, drawable.height,
	                                        drawable.pixelFormat)) {
		return 0;
	}
	blit_encoder = [command_buffer blitCommandEncoder];
	if (blit_encoder == nil) {
		return 0;
	}
	[blit_encoder copyFromTexture:drawable
                  sourceSlice:0
                  sourceLevel:0
                 sourceOrigin:(MTLOrigin){0, 0, 0}
                   sourceSize:(MTLSize){drawable.width, drawable.height, 1}
                    toTexture:state->capture_texture
             destinationSlice:0
             destinationLevel:0
            destinationOrigin:(MTLOrigin){0, 0, 0}];
	[blit_encoder endEncoding];
	return 1;
}

static int vaeg_metal_finish_capture(VAEG_METAL_STATE *state, id<MTLCommandBuffer> command_buffer,
                                      VAEG_OUTPUT_CAPTURE *capture) {
	NSUInteger row;

	if ((capture == nullptr) || (state->capture_texture == nil) ||
	    ([command_buffer status] != MTLCommandBufferStatusCompleted)) {
		return 0;
	}
	[state->capture_texture getBytes:capture->pixels
	                     bytesPerRow:capture->pitch_bytes
	                      fromRegion:MTLRegionMake2D(0, 0, capture->width, capture->height)
                     mipmapLevel:0];
	for (row = 0; row < capture->height; ++row) {
		unsigned char *pixels = static_cast<unsigned char *>(capture->pixels) +
		                         row * capture->pitch_bytes;
		for (NSUInteger column = 0; column < capture->width; ++column) {
			unsigned char blue = pixels[column * 4U];
			pixels[column * 4U] = pixels[column * 4U + 2U];
			pixels[column * 4U + 2U] = blue;
			pixels[column * 4U + 3U] = 255;
		}
	}
	capture->complete = 1;
	return 1;
}

extern "C" int vaeg_metal_bridge_initialize(void *host_window, const char *preset_path,
                                               int enable_filter, VAEG_METAL_BRIDGE *bridge) {
	VAEG_METAL_STATE *state;
	MTLRenderPipelineDescriptor *descriptor;
	id<MTLLibrary> library;
	id<MTLFunction> vertex_function;
	id<MTLFunction> fragment_function;
	NSError *error;

	if ((host_window == nullptr) || (bridge == nullptr)) {
		return 0;
	}
	bridge->state = nullptr;
	state = static_cast<VAEG_METAL_STATE *>(calloc(1, sizeof(*state)));
	if (state == nullptr) {
		return 0;
	}
	state->view = SDL_Metal_CreateView(static_cast<SDL_Window *>(host_window));
	if (state->view == nullptr) {
		vaeg_metal_report_initialization_failure("SDL_Metal_CreateView", nil);
		vaeg_metal_release_state(state);
		return 0;
	}
	state->layer = (__bridge CAMetalLayer *)SDL_Metal_GetLayer(state->view);
	state->device = MTLCreateSystemDefaultDevice();
	if (state->layer == nil) {
		vaeg_metal_report_initialization_failure("SDL_Metal_GetLayer", nil);
		vaeg_metal_release_state(state);
		return 0;
	}
	if (state->device == nil) {
		vaeg_metal_report_initialization_failure("MTLCreateSystemDefaultDevice", nil);
		vaeg_metal_release_state(state);
		return 0;
	}
	state->layer.device = state->device;
	state->layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
	state->layer.framebufferOnly = NO;
	state->queue = [state->device newCommandQueue];
	if (state->queue == nil) {
		vaeg_metal_report_initialization_failure("MTLDevice.newCommandQueue", nil);
		vaeg_metal_release_state(state);
		return 0;
	}
	error = nil;
	library = [state->device newLibraryWithSource:[NSString stringWithUTF8String:vaeg_metal_passthrough_shader]
	                                         options:nil error:&error];
	if (library == nil) {
		vaeg_metal_report_initialization_failure("MTLDevice.newLibraryWithSource", error);
		vaeg_metal_release_state(state);
		return 0;
	}
	vertex_function = [library newFunctionWithName:@"vaeg_metal_vertex"];
	fragment_function = [library newFunctionWithName:@"vaeg_metal_fragment"];
	if ((vertex_function == nil) || (fragment_function == nil)) {
		vaeg_metal_report_initialization_failure("MTLLibrary.newFunctionWithName", nil);
		[fragment_function release];
		[vertex_function release];
		[library release];
		vaeg_metal_release_state(state);
		return 0;
	}
	descriptor = [[MTLRenderPipelineDescriptor alloc] init];
	descriptor.vertexFunction = vertex_function;
	descriptor.fragmentFunction = fragment_function;
	descriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
	state->pipeline = [state->device newRenderPipelineStateWithDescriptor:descriptor error:&error];
	[descriptor release];
	[fragment_function release];
	[vertex_function release];
	[library release];
	if (state->pipeline == nil) {
		vaeg_metal_report_initialization_failure("MTLDevice.newRenderPipelineStateWithDescriptor", error);
		vaeg_metal_release_state(state);
		return 0;
	}
	if ((enable_filter != 0) && !vaeg_metal_create_filter_chain(state, preset_path)) {
		vaeg_metal_release_state(state);
		return 0;
	}
	bridge->state = state;
	bridge->capture = nullptr;
	return 1;
}

extern "C" VAEG_METAL_BRIDGE_RESULT vaeg_metal_bridge_set_filter_enabled(
	VAEG_METAL_BRIDGE *bridge, int enabled) {
	VAEG_METAL_STATE *state;

	if ((bridge == nullptr) || (bridge->state == nullptr)) {
		return VAEG_METAL_BRIDGE_INVALID_ARGUMENT;
	}
	state = static_cast<VAEG_METAL_STATE *>(bridge->state);
	if ((enabled != 0) && (state->filter_chain == nullptr)) {
		return VAEG_METAL_BRIDGE_RESOURCE_FAILURE;
	}
	if ((enabled != 0) && !state->filter_enabled) {
		state->filter_first_frame = true;
	}
	state->filter_enabled = (enabled != 0);
	return VAEG_METAL_BRIDGE_OK;
}

extern "C" VAEG_METAL_BRIDGE_RESULT vaeg_metal_bridge_set_filter_parameter(
	VAEG_METAL_BRIDGE *bridge, const char *name, float value) {
	VAEG_METAL_STATE *state;
	libra_error_t error;

	if ((bridge == nullptr) || (bridge->state == nullptr) || (name == nullptr)) {
		return VAEG_METAL_BRIDGE_RESOURCE_FAILURE;
	}
	state = static_cast<VAEG_METAL_STATE *>(bridge->state);
	if ((state->filter_chain == nullptr) ||
	    (state->librashader.mtl_filter_chain_set_param == nullptr)) {
		return VAEG_METAL_BRIDGE_RESOURCE_FAILURE;
	}
	error = state->librashader.mtl_filter_chain_set_param(&state->filter_chain, name, value);
	if (error != nullptr) {
		vaeg_metal_report_librashader_error(state, error, "parameter update");
		return VAEG_METAL_BRIDGE_RESOURCE_FAILURE;
	}
	state->filter_first_frame = true;
	return VAEG_METAL_BRIDGE_OK;
}

extern "C" void vaeg_metal_bridge_set_drawable_size(const VAEG_METAL_BRIDGE *bridge,
                                                      uint32_t width, uint32_t height) {
	VAEG_METAL_STATE *state;

	if ((bridge == nullptr) || (bridge->state == nullptr)) {
		return;
	}
	state = static_cast<VAEG_METAL_STATE *>(bridge->state);
	if ((width != 0) && (height != 0)) {
		state->layer.drawableSize = CGSizeMake(static_cast<CGFloat>(width), static_cast<CGFloat>(height));
	}
}

extern "C" void vaeg_metal_bridge_set_output_viewport(VAEG_METAL_BRIDGE *bridge, int x, int y,
                                                        int width, int height) {
	VAEG_METAL_STATE *state;

	if ((bridge == nullptr) || (bridge->state == nullptr)) {
		return;
	}
	state = static_cast<VAEG_METAL_STATE *>(bridge->state);
	if ((width <= 0) || (height <= 0)) {
		state->output_viewport_valid = false;
		return;
	}
	state->output_viewport.originX = static_cast<double>(x);
	state->output_viewport.originY = static_cast<double>(y);
	state->output_viewport.width = static_cast<double>(width);
	state->output_viewport.height = static_cast<double>(height);
	state->output_viewport.znear = 0.0;
	state->output_viewport.zfar = 1.0;
	state->output_viewport_valid = true;
}

extern "C" int vaeg_metal_bridge_gui_prepare(VAEG_METAL_BRIDGE *bridge) {
	VAEG_METAL_STATE *state;

	if ((bridge == nullptr) || (bridge->state == nullptr) || !ImGui::GetCurrentContext()) {
		return 0;
	}
	state = static_cast<VAEG_METAL_STATE *>(bridge->state);
	if (!vaeg_metal_create_gui_pipeline(state) || !vaeg_metal_update_gui_textures(state)) {
		return 0;
	}
	ImGuiIO &io = ImGui::GetIO();
	io.BackendRendererName = "vaeg_metal";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
	return 1;
}

extern "C" void vaeg_metal_bridge_gui_shutdown(VAEG_METAL_BRIDGE *bridge) {
	VAEG_METAL_STATE *state;

	if ((bridge == nullptr) || (bridge->state == nullptr) || !ImGui::GetCurrentContext()) {
		return;
	}
	state = static_cast<VAEG_METAL_STATE *>(bridge->state);
	for (ImTextureData *data : ImGui::GetPlatformIO().Textures) {
		vaeg_metal_release_gui_texture(data);
	}
	if (state->gui_pipeline != nil) {
		[state->gui_pipeline release];
		state->gui_pipeline = nil;
	}
	if (state->gui_sampler != nil) {
		[state->gui_sampler release];
		state->gui_sampler = nil;
	}
	if (state->gui_vertex_buffer != nil) {
		[state->gui_vertex_buffer release];
		state->gui_vertex_buffer = nil;
		state->gui_vertex_capacity = 0;
	}
	if (state->gui_index_buffer != nil) {
		[state->gui_index_buffer release];
		state->gui_index_buffer = nil;
		state->gui_index_capacity = 0;
	}
}

static int vaeg_metal_render_gui(VAEG_METAL_STATE *state, id<MTLCommandBuffer> command_buffer,
                                 id<MTLTexture> target) {
	const ImDrawData *draw_data;
	MTLRenderPassDescriptor *pass_descriptor;
	id<MTLRenderCommandEncoder> encoder;
	struct VAEG_METAL_IMGUI_UNIFORMS {
		float projection[16];
	} uniforms;
	ImVec2 framebuffer_scale;
	float display_width;
	float display_height;
	size_t vertex_offset = 0;
	size_t index_offset = 0;

	if (!ImGui::GetCurrentContext() || (state->gui_pipeline == nil) ||
	    (state->gui_sampler == nil) || (target == nil)) {
		return 1;
	}
	draw_data = ImGui::GetDrawData();
	if ((draw_data == nullptr) || (draw_data->TotalVtxCount <= 0) ||
	    (draw_data->TotalIdxCount <= 0) || (draw_data->DisplaySize.x <= 0.0f) ||
	    (draw_data->DisplaySize.y <= 0.0f)) {
		return 1;
	}
	if (draw_data->FramebufferScale.x > 0.0f && draw_data->FramebufferScale.y > 0.0f) {
		framebuffer_scale = draw_data->FramebufferScale;
	} else {
		framebuffer_scale = ImVec2(1.0f, 1.0f);
	}
	display_width = draw_data->DisplaySize.x * framebuffer_scale.x;
	display_height = draw_data->DisplaySize.y * framebuffer_scale.y;
	if ((state->gui_vertex_buffer == nil) ||
	    (state->gui_vertex_capacity < static_cast<size_t>(draw_data->TotalVtxCount) *
                                        sizeof(ImDrawVert))) {
		const size_t capacity = static_cast<size_t>(draw_data->TotalVtxCount) * sizeof(ImDrawVert);
		if (state->gui_vertex_buffer != nil) [state->gui_vertex_buffer release];
		state->gui_vertex_buffer = [state->device newBufferWithLength:capacity
		                                                        options:MTLResourceStorageModeShared];
		state->gui_vertex_capacity = state->gui_vertex_buffer ? capacity : 0;
	}
	if ((state->gui_index_buffer == nil) ||
	    (state->gui_index_capacity < static_cast<size_t>(draw_data->TotalIdxCount) *
                                       sizeof(ImDrawIdx))) {
		const size_t capacity = static_cast<size_t>(draw_data->TotalIdxCount) * sizeof(ImDrawIdx);
		if (state->gui_index_buffer != nil) [state->gui_index_buffer release];
		state->gui_index_buffer = [state->device newBufferWithLength:capacity
	                                                       options:MTLResourceStorageModeShared];
		state->gui_index_capacity = state->gui_index_buffer ? capacity : 0;
	}
	if ((state->gui_vertex_buffer == nil) || (state->gui_index_buffer == nil)) {
		return 0;
	}
	for (int list_index = 0; list_index < draw_data->CmdListsCount; ++list_index) {
		const ImDrawList *draw_list = draw_data->CmdLists[list_index];
		memcpy(static_cast<unsigned char *>(state->gui_vertex_buffer.contents) + vertex_offset,
		       draw_list->VtxBuffer.Data, draw_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(static_cast<unsigned char *>(state->gui_index_buffer.contents) + index_offset,
		       draw_list->IdxBuffer.Data, draw_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vertex_offset += draw_list->VtxBuffer.Size * sizeof(ImDrawVert);
		index_offset += draw_list->IdxBuffer.Size * sizeof(ImDrawIdx);
	}
	memset(&uniforms, 0, sizeof(uniforms));
	uniforms.projection[0] = 2.0f / display_width;
	uniforms.projection[5] = -2.0f / display_height;
	uniforms.projection[10] = 1.0f;
	uniforms.projection[12] = -1.0f - draw_data->DisplayPos.x * 2.0f / display_width;
	uniforms.projection[13] = 1.0f + draw_data->DisplayPos.y * 2.0f / display_height;
	uniforms.projection[15] = 1.0f;
	pass_descriptor = [MTLRenderPassDescriptor renderPassDescriptor];
	pass_descriptor.colorAttachments[0].texture = target;
	pass_descriptor.colorAttachments[0].loadAction = MTLLoadActionLoad;
	pass_descriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
	encoder = [command_buffer renderCommandEncoderWithDescriptor:pass_descriptor];
	if (encoder == nil) {
		return 0;
	}
	[encoder setViewport:(MTLViewport){0.0, 0.0, (double)target.width, (double)target.height, 0.0, 1.0}];
	[encoder setRenderPipelineState:state->gui_pipeline];
	[encoder setVertexBuffer:state->gui_vertex_buffer offset:0 atIndex:0];
	[encoder setVertexBytes:&uniforms length:sizeof(uniforms) atIndex:1];
	[encoder setFragmentSamplerState:state->gui_sampler atIndex:0];
	vertex_offset = 0;
	index_offset = 0;
	for (int list_index = 0; list_index < draw_data->CmdListsCount; ++list_index) {
		const ImDrawList *draw_list = draw_data->CmdLists[list_index];
		for (const ImDrawCmd &command : draw_list->CmdBuffer) {
			if (command.UserCallback != nullptr) {
				continue;
			}
			const ImVec4 clip = command.ClipRect;
			int left = static_cast<int>((clip.x - draw_data->DisplayPos.x) * framebuffer_scale.x);
			int top = static_cast<int>((clip.y - draw_data->DisplayPos.y) * framebuffer_scale.y);
			int right = static_cast<int>((clip.z - draw_data->DisplayPos.x) * framebuffer_scale.x);
			int bottom = static_cast<int>((clip.w - draw_data->DisplayPos.y) * framebuffer_scale.y);
			if (left < 0) left = 0;
			if (top < 0) top = 0;
			if (right > static_cast<int>(target.width)) right = target.width;
			if (bottom > static_cast<int>(target.height)) bottom = target.height;
			if ((right <= left) || (bottom <= top)) continue;
			VAEG_METAL_GUI_TEXTURE *texture = reinterpret_cast<VAEG_METAL_GUI_TEXTURE *>(
			    static_cast<uintptr_t>(command.GetTexID()));
			if ((texture == nullptr) || (texture->texture == nil)) continue;
			[encoder setScissorRect:(MTLScissorRect){(NSUInteger)left, (NSUInteger)top,
			                                      (NSUInteger)(right - left),
			                                      (NSUInteger)(bottom - top)}];
			[encoder setFragmentTexture:texture->texture atIndex:0];
			[encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
			                    indexCount:command.ElemCount
			                      indexType:(sizeof(ImDrawIdx) == 2 ? MTLIndexTypeUInt16 : MTLIndexTypeUInt32)
			                    indexBuffer:state->gui_index_buffer
			              indexBufferOffset:index_offset + command.IdxOffset * sizeof(ImDrawIdx)
			                  instanceCount:1
			                     baseVertex:(NSInteger)(vertex_offset / sizeof(ImDrawVert)) +
			                                command.VtxOffset
			                   baseInstance:0];
		}
		vertex_offset += draw_list->VtxBuffer.Size * sizeof(ImDrawVert);
		index_offset += draw_list->IdxBuffer.Size * sizeof(ImDrawIdx);
	}
	[encoder endEncoding];
	return 1;
}

extern "C" VAEG_METAL_BRIDGE_RESULT vaeg_metal_bridge_present(
	VAEG_METAL_BRIDGE *bridge, const VAEG_FRAME_INPUT *frame) {
	VAEG_METAL_STATE *state;
	MTLRenderPassDescriptor *pass_descriptor;
	id<CAMetalDrawable> drawable;
	id<MTLCommandBuffer> command_buffer;
	id<MTLRenderCommandEncoder> encoder;
	MTLViewport viewport;
	libra_viewport_t libra_viewport;
	frame_mtl_opt_t filter_options;
	libra_error_t error;
	VAEG_OUTPUT_CAPTURE *capture;
	int capture_submitted;

	if ((bridge == nullptr) || (bridge->state == nullptr) || (frame == nullptr)) {
		return VAEG_METAL_BRIDGE_INVALID_ARGUMENT;
	}
	if (vaeg_frame_input_validate(frame) != VAEG_FRAME_INPUT_OK) {
		return VAEG_METAL_BRIDGE_INVALID_FRAME;
	}
	state = static_cast<VAEG_METAL_STATE *>(bridge->state);
	capture = bridge->capture;
	capture_submitted = 0;
	if (!vaeg_metal_ensure_source_texture(state, frame->width, frame->height)) {
		return VAEG_METAL_BRIDGE_RESOURCE_FAILURE;
	}
	if (vaeg_frame_convert_rgba8888(frame, state->upload_buffer, state->upload_pitch,
	                                state->upload_capacity) != VAEG_FRAME_CONVERSION_OK) {
		return VAEG_METAL_BRIDGE_INVALID_FRAME;
	}
	[state->source_texture replaceRegion:MTLRegionMake2D(0, 0, state->source_width, state->source_height)
	                       mipmapLevel:0
	                         withBytes:state->upload_buffer
	                       bytesPerRow:state->upload_pitch];
	drawable = [state->layer nextDrawable];
	if (drawable == nil) {
		return VAEG_METAL_BRIDGE_NO_DRAWABLE;
	}
	command_buffer = [state->queue commandBuffer];
	if (command_buffer == nil) {
		return VAEG_METAL_BRIDGE_RESOURCE_FAILURE;
	}
	viewport = vaeg_metal_viewport(state, frame);
	if ((viewport.width <= 0.0) || (viewport.height <= 0.0)) {
		return VAEG_METAL_BRIDGE_NO_DRAWABLE;
	}
	if (state->filter_enabled) {
		if (!vaeg_metal_ensure_filter_output_texture(state, drawable.texture.width,
		                                             drawable.texture.height,
		                                             drawable.texture.pixelFormat)) {
			fprintf(stderr, "librashader Metal filter output texture unavailable\n");
			return VAEG_METAL_BRIDGE_FILTER_FAILURE;
		}
		libra_viewport.x = static_cast<float>(viewport.originX);
		libra_viewport.y = static_cast<float>(viewport.originY);
		libra_viewport.width = static_cast<uint32_t>(viewport.width);
		libra_viewport.height = static_cast<uint32_t>(viewport.height);
		memset(&filter_options, 0, sizeof(filter_options));
		filter_options.version = LIBRASHADER_CURRENT_VERSION;
		/* The bundled VAeg CRT preset has no OriginalHistory inputs. The pinned
		 * Metal runtime attempts to encode an empty render pass when clear_history
		 * is true for such a preset, and reports that as a command-buffer error. */
		filter_options.clear_history = false;
		filter_options.frame_direction = 1;
		filter_options.rotation = 0;
		filter_options.total_subframes = 1;
		filter_options.current_subframe = 1;
		filter_options.aspect_ratio = static_cast<float>(frame->source_aspect_width) /
		                              static_cast<float>(frame->source_aspect_height);
		filter_options.frames_per_second =
			static_cast<float>(frame->source_frame_rate_numerator) /
			static_cast<float>(frame->source_frame_rate_denominator);
		filter_options.frametime_delta = static_cast<uint32_t>(frame->frame_time_delta_ns / 1000000U);
		error = state->librashader.mtl_filter_chain_frame(
			&state->filter_chain, command_buffer, 1, state->source_texture,
			state->filter_output_texture,
			&libra_viewport, nullptr, &filter_options);
		if (error != nullptr) {
			vaeg_metal_report_librashader_error(state, error, "frame rendering");
			return VAEG_METAL_BRIDGE_FILTER_FAILURE;
		}
		/* librashader deliberately terminates at a caller-owned surface. Clear the
		 * drawable first, then copy only the filtered viewport to preserve the
		 * letterbox border before the native ImGui pass is encoded. */
		pass_descriptor = [MTLRenderPassDescriptor renderPassDescriptor];
		pass_descriptor.colorAttachments[0].texture = drawable.texture;
		pass_descriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
		pass_descriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
		pass_descriptor.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
		encoder = [command_buffer renderCommandEncoderWithDescriptor:pass_descriptor];
		if (encoder == nil) {
			fprintf(stderr, "librashader Metal drawable clear encoder unavailable\n");
			return VAEG_METAL_BRIDGE_FILTER_FAILURE;
		}
		[encoder endEncoding];
		id<MTLBlitCommandEncoder> blit_encoder = [command_buffer blitCommandEncoder];
		if (blit_encoder == nil) {
			fprintf(stderr, "librashader Metal drawable blit encoder unavailable\n");
			return VAEG_METAL_BRIDGE_FILTER_FAILURE;
		}
		[blit_encoder copyFromTexture:state->filter_output_texture
		                 sourceSlice:0
		                 sourceLevel:0
		                sourceOrigin:(MTLOrigin){(NSUInteger)libra_viewport.x,
		                                       (NSUInteger)libra_viewport.y, 0}
		                  sourceSize:(MTLSize){(NSUInteger)libra_viewport.width,
		                                     (NSUInteger)libra_viewport.height, 1}
		                   toTexture:drawable.texture
		            destinationSlice:0
		            destinationLevel:0
		           destinationOrigin:(MTLOrigin){(NSUInteger)libra_viewport.x,
		                                          (NSUInteger)libra_viewport.y, 0}];
		[blit_encoder endEncoding];
		state->filter_first_frame = false;
	} else {
		pass_descriptor = [MTLRenderPassDescriptor renderPassDescriptor];
		pass_descriptor.colorAttachments[0].texture = drawable.texture;
		pass_descriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
		pass_descriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
		pass_descriptor.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
		encoder = [command_buffer renderCommandEncoderWithDescriptor:pass_descriptor];
		if (encoder == nil) {
			return VAEG_METAL_BRIDGE_RESOURCE_FAILURE;
		}
		[encoder setViewport:viewport];
		[encoder setRenderPipelineState:state->pipeline];
		[encoder setFragmentTexture:state->source_texture atIndex:0];
		[encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
		[encoder endEncoding];
	}
	if (ImGui::GetCurrentContext() && !vaeg_metal_render_gui(state, command_buffer, drawable.texture)) {
		fprintf(stderr, "librashader Metal ImGui frame rendering failed\n");
	}
	if (capture != nullptr) {
		capture_submitted =
		    vaeg_metal_capture_drawable(state, command_buffer, drawable.texture, capture);
	}
	[command_buffer presentDrawable:drawable];
	[command_buffer commit];
	if (capture_submitted) {
		[command_buffer waitUntilCompleted];
		(void)vaeg_metal_finish_capture(state, command_buffer, capture);
	}
	return VAEG_METAL_BRIDGE_OK;
}

extern "C" void vaeg_metal_bridge_shutdown(VAEG_METAL_BRIDGE *bridge) {
	VAEG_METAL_STATE *state;

	if (bridge == nullptr) {
		return;
	}
	bridge->capture = nullptr;
	if (bridge->state == nullptr) {
		return;
	}
	state = static_cast<VAEG_METAL_STATE *>(bridge->state);
	bridge->state = nullptr;
	vaeg_metal_release_state(state);
}
