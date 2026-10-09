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
#ifndef VAEG_SDL2_LIBRASHADER_NATIVE_PRESENTER_H
#define VAEG_SDL2_LIBRASHADER_NATIVE_PRESENTER_H

#include <cstdint>
#include <string>

#include "librashader/frame_input.h"
#include "librashader/shader_preset.h"

namespace vaeg::librashader {

enum class PresenterState : uint8_t {
	Unavailable = 0,
	Initializing,
	PassThrough,
	Filtered
};

enum class PresenterResult : uint8_t {
	Presented = 0,
	Disabled,
	Recovered,
	Fallback,
	Failed
};

enum class PresenterError : uint8_t {
	None = 0,
	InvalidFrame,
	PlatformUnavailable,
	RuntimeMissing,
	AbiMismatch,
	DeviceFailure,
	PresetFailure,
	FilterFailure,
	ResourceFailure
};

enum class PresenterBackend : uint8_t {
	Automatic = 0,
	Metal,
	D3D11,
	OpenGL
};

struct NativePresenterCreateInfo {
	void *host_window;
	uint32_t drawable_width;
	uint32_t drawable_height;
	PresenterBackend backend;
	bool enable_filter;
	const char *preset_path;
	const char *parameter_state_path;
};

bool presenter_state_transition_allowed(PresenterState from, PresenterState to) noexcept;
const char *presenter_state_name(PresenterState state) noexcept;
const char *presenter_result_name(PresenterResult result) noexcept;
const char *presenter_error_name(PresenterError error) noexcept;

class NativePresenter {
  public:
	virtual ~NativePresenter() = default;
	virtual PresenterState state() const noexcept = 0;
	virtual PresenterError last_error() const noexcept = 0;
	virtual const char *error_detail() const noexcept { return preset_error_.c_str(); }
	virtual PresenterResult initialize(const NativePresenterCreateInfo &info) noexcept = 0;
	virtual PresenterResult present(const VAEG_FRAME_INPUT &frame) noexcept = 0;
	virtual PresenterResult set_filter_enabled(bool enabled) noexcept = 0;
	virtual PresenterResult resize(uint32_t drawable_width, uint32_t drawable_height) noexcept = 0;
	virtual PresenterResult recover() noexcept = 0;
	virtual void shutdown() noexcept = 0;
	// Presentation-thread GUI lifecycle; GPU objects stay inside each backend.
	virtual bool gui_prepare() noexcept { return false; }
	virtual void gui_shutdown() noexcept {}
	virtual void set_output_capture(VAEG_OUTPUT_CAPTURE *) noexcept {}
	virtual void set_output_viewport(int, int, int, int) noexcept {}

	std::size_t filter_parameter_count() const noexcept;
	const ShaderParameterInfo *filter_parameter(std::size_t index) const noexcept;
	PresenterResult set_filter_parameter(const char *name, float value) noexcept;
	PresenterResult reset_filter_parameters() noexcept;

  protected:
	bool prepare_filter_parameters(const char *preset_path, bool enable_filter,
	                               const char *parameter_state_path) noexcept;
	bool apply_filter_parameters() noexcept;
	const char *parameter_state_path() const noexcept { return parameter_state_path_.c_str(); }

  private:
	virtual bool apply_backend_filter_parameter(const char *name, float value) noexcept = 0;
	ShaderPreset preset_;
	std::string parameter_state_path_;
	std::string preset_error_;
};

} // namespace vaeg::librashader

#endif
