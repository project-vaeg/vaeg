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
#include "librashader/d3d11_presenter.h"

#include <cstdio>

#include "librashader/d3d11_bridge.h"

namespace vaeg::librashader {

namespace {

class D3D11Presenter final : public NativePresenter {
  public:
	D3D11Presenter() noexcept
	    : state_(PresenterState::Unavailable), error_(PresenterError::None), host_window_(nullptr),
	      drawable_width_(0), drawable_height_(0), backend_(PresenterBackend::D3D11),
	      filter_enabled_(false) {
		preset_path_[0] = '\0';
	}

	~D3D11Presenter() override { shutdown(); }

	PresenterState state() const noexcept override { return state_; }
	PresenterError last_error() const noexcept override { return error_; }
	const char *error_detail() const noexcept override {
		if (error_ == PresenterError::None) return "";
		const char *preset_error = NativePresenter::error_detail();
		return preset_error[0] ? preset_error : bridge_.error;
	}

	PresenterResult initialize(const NativePresenterCreateInfo &info) noexcept override {
		if ((info.host_window == nullptr) ||
		    ((info.backend != PresenterBackend::Automatic) &&
		     (info.backend != PresenterBackend::D3D11))) {
			state_ = PresenterState::Unavailable;
			error_ = PresenterError::PlatformUnavailable;
			return PresenterResult::Fallback;
		}
		shutdown();
		bridge_.error[0] = '\0';
		host_window_ = info.host_window;
		drawable_width_ = info.drawable_width;
		drawable_height_ = info.drawable_height;
		backend_ = info.backend;
		filter_enabled_ = info.enable_filter;
		if (info.preset_path == nullptr) {
			preset_path_[0] = '\0';
		} else {
			std::snprintf(preset_path_, sizeof(preset_path_), "%s", info.preset_path);
		}
		const bool preset_ready = prepare_filter_parameters(info.preset_path, info.enable_filter,
		                                                   info.parameter_state_path);
		bool filter_ready = info.enable_filter && preset_ready;
		state_ = PresenterState::Initializing;
		if (!vaeg_d3d11_bridge_initialize(info.host_window, info.preset_path,
		                                  filter_ready ? 1 : 0, &bridge_)) {
			// A filter failure must not discard a working D3D11 pass-through path.
			if (filter_ready && vaeg_d3d11_bridge_initialize(info.host_window, nullptr, 0, &bridge_)) {
				filter_ready = false;
			} else {
				state_ = PresenterState::Unavailable;
				error_ = PresenterError::DeviceFailure;
				return PresenterResult::Fallback;
			}
		}
		if (filter_ready && !apply_filter_parameters()) {
			(void)vaeg_d3d11_bridge_set_filter_enabled(&bridge_, 0);
			filter_ready = false;
		}
		if ((drawable_width_ != 0) && (drawable_height_ != 0) &&
		    (vaeg_d3d11_bridge_set_drawable_size(&bridge_, drawable_width_, drawable_height_) !=
		     VAEG_D3D11_BRIDGE_OK)) {
			shutdown();
			state_ = PresenterState::Unavailable;
			error_ = PresenterError::ResourceFailure;
			return PresenterResult::Fallback;
		}
		filter_enabled_ = filter_ready;
		vaeg_d3d11_bridge_set_output_viewport(&bridge_, viewport_[0], viewport_[1],
		                                     viewport_[2], viewport_[3]);
		state_ = filter_ready ? PresenterState::Filtered : PresenterState::PassThrough;
		error_ = !preset_ready ? PresenterError::PresetFailure :
		         (info.enable_filter && !filter_ready ? PresenterError::FilterFailure : PresenterError::None);
		return PresenterResult::Recovered;
	}

	void set_output_capture(VAEG_OUTPUT_CAPTURE *capture) noexcept override {
		bridge_.capture = capture;
	}

	PresenterResult present(const VAEG_FRAME_INPUT &frame) noexcept override {
		VAEG_D3D11_BRIDGE_RESULT result;

		if ((state_ != PresenterState::PassThrough) && (state_ != PresenterState::Filtered)) {
			return PresenterResult::Fallback;
		}
		result = vaeg_d3d11_bridge_present(&bridge_, &frame);
		if (result == VAEG_D3D11_BRIDGE_OK) {
			return PresenterResult::Presented;
		}
		if (result == VAEG_D3D11_BRIDGE_INVALID_FRAME) {
			error_ = PresenterError::InvalidFrame;
			return PresenterResult::Failed;
		}
		if (result == VAEG_D3D11_BRIDGE_NO_OUTPUT) {
			return PresenterResult::Disabled;
		}
		if (result == VAEG_D3D11_BRIDGE_FILTER_FAILURE) {
			if (vaeg_d3d11_bridge_set_filter_enabled(&bridge_, 0) == VAEG_D3D11_BRIDGE_OK) {
				filter_enabled_ = false;
				state_ = PresenterState::PassThrough;
				error_ = PresenterError::FilterFailure;
				return PresenterResult::Recovered;
			}
			error_ = PresenterError::FilterFailure;
		}
		error_ = (result == VAEG_D3D11_BRIDGE_DEVICE_LOST) ? PresenterError::DeviceFailure
		                                                   : PresenterError::ResourceFailure;
		state_ = PresenterState::Unavailable;
		return PresenterResult::Fallback;
	}

	PresenterResult set_filter_enabled(bool enabled) noexcept override {
		VAEG_D3D11_BRIDGE_RESULT result;

		if ((state_ != PresenterState::PassThrough) && (state_ != PresenterState::Filtered)) {
			return PresenterResult::Fallback;
		}
		result = vaeg_d3d11_bridge_set_filter_enabled(&bridge_, enabled ? 1 : 0);
		if (result == VAEG_D3D11_BRIDGE_OK) {
			filter_enabled_ = enabled;
			state_ = enabled ? PresenterState::Filtered : PresenterState::PassThrough;
			error_ = PresenterError::None;
			return enabled ? PresenterResult::Recovered : PresenterResult::Disabled;
		}
		error_ = PresenterError::FilterFailure;
		return PresenterResult::Fallback;
	}

	PresenterResult resize(uint32_t drawable_width, uint32_t drawable_height) noexcept override {
		VAEG_D3D11_BRIDGE_RESULT result;

		if ((state_ != PresenterState::PassThrough) && (state_ != PresenterState::Filtered)) {
			return PresenterResult::Fallback;
		}
		if ((drawable_width == 0) || (drawable_height == 0)) {
			return PresenterResult::Disabled;
		}
		result = vaeg_d3d11_bridge_set_drawable_size(&bridge_, drawable_width, drawable_height);
		if (result != VAEG_D3D11_BRIDGE_OK) {
			error_ = PresenterError::ResourceFailure;
			state_ = PresenterState::Unavailable;
			return PresenterResult::Fallback;
		}
		drawable_width_ = drawable_width;
		drawable_height_ = drawable_height;
		return PresenterResult::Recovered;
	}

	PresenterResult recover() noexcept override {
		NativePresenterCreateInfo info;
		char saved_preset[sizeof(preset_path_)];
		char saved_parameters[1024];

		if (host_window_ == nullptr) {
			return PresenterResult::Fallback;
		}
		info.host_window = host_window_;
		info.drawable_width = drawable_width_;
		info.drawable_height = drawable_height_;
		info.backend = backend_;
		info.enable_filter = filter_enabled_;
		std::snprintf(saved_preset, sizeof(saved_preset), "%s", preset_path_);
		std::snprintf(saved_parameters, sizeof(saved_parameters), "%s", parameter_state_path());
		info.preset_path = saved_preset[0] ? saved_preset : nullptr;
		info.parameter_state_path = saved_parameters;
		return initialize(info);
	}

	void shutdown() noexcept override {
		vaeg_d3d11_bridge_shutdown(&bridge_);
		state_ = PresenterState::Unavailable;
		error_ = PresenterError::None;
	}

  private:
	bool gui_prepare() noexcept override { return vaeg_d3d11_bridge_gui_prepare(&bridge_) != 0; }
	void gui_shutdown() noexcept override { vaeg_d3d11_bridge_gui_shutdown(&bridge_); }
	void set_output_viewport(int x, int y, int width, int height) noexcept override {
		viewport_[0] = x; viewport_[1] = y; viewport_[2] = width; viewport_[3] = height;
		vaeg_d3d11_bridge_set_output_viewport(&bridge_, x, y, width, height);
	}

	bool apply_backend_filter_parameter(const char *name, float value) noexcept override {
		return vaeg_d3d11_bridge_set_filter_parameter(&bridge_, name, value) ==
		       VAEG_D3D11_BRIDGE_OK;
	}

	VAEG_D3D11_BRIDGE bridge_{};
	PresenterState state_;
	PresenterError error_;
	void *host_window_;
	uint32_t drawable_width_;
	uint32_t drawable_height_;
	PresenterBackend backend_;
	bool filter_enabled_;
	char preset_path_[1024];
	int viewport_[4]{};
};

} // namespace

std::unique_ptr<NativePresenter> create_d3d11_presenter() {
	return std::make_unique<D3D11Presenter>();
}

} // namespace vaeg::librashader
