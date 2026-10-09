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
#include "librashader/native_presenter_controller.h"

#include <cstdio>
#include <cstring>
#include <memory>

#include "librashader/native_presenter.h"
#include "librashader/frame_padding.h"
#include "librashader/presenter_factory.h"

struct VAEG_NATIVE_PRESENTER {
	std::unique_ptr<vaeg::librashader::NativePresenter> implementation;
	const char *backend;
	vaeg::librashader::FramePadding padding;
};

namespace {

// Presentation-thread diagnostic survives destruction of a failed presenter.
char creation_error[256] = "none";

static const char *native_backend_name() noexcept {
#if defined(_WIN32)
	return "D3D11";
#elif defined(__APPLE__)
	return "Metal";
#elif defined(__linux__)
	return "OpenGL";
#else
	return "unavailable";
#endif
}

static VAEG_NATIVE_PRESENTER_RESULT present_once(
	VAEG_NATIVE_PRESENTER *presenter, const VAEG_FRAME_INPUT *frame) {
	using vaeg::librashader::PresenterError;
	using vaeg::librashader::PresenterResult;
	PresenterResult result;

	if ((presenter == nullptr) || (presenter->implementation == nullptr) || (frame == nullptr)) {
		return VAEG_NATIVE_PRESENTER_FALLBACK;
	}
	// Re-evaluate on every retry: a failed filter must fall back to the raw input.
	const auto draw = [&]() {
		VAEG_FRAME_INPUT display = *frame;
		if (presenter->implementation->state() == vaeg::librashader::PresenterState::Filtered) {
			for (size_t i = 0; i < presenter->implementation->filter_parameter_count(); ++i) {
				const auto *parameter = presenter->implementation->filter_parameter(i);
				if (parameter && parameter->name == "SCREEN_SIZE") {
					if (!presenter->padding.prepare(*frame, parameter->value, display))
						return PresenterResult::Failed;
					break;
				}
			}
		}
		return presenter->implementation->present(display);
	};
	result = draw();
	if (result == PresenterResult::Presented) {
		return VAEG_NATIVE_PRESENTER_PRESENTED;
	}
	if (result == PresenterResult::Disabled) {
		return VAEG_NATIVE_PRESENTER_NO_OUTPUT;
	}
	if (result == PresenterResult::Recovered) {
		// A filter failure recovered to native pass-through. Draw that frame now.
		result = draw();
		if (result == PresenterResult::Presented) {
			return VAEG_NATIVE_PRESENTER_PRESENTED;
		}
		if (result == PresenterResult::Disabled) {
			return VAEG_NATIVE_PRESENTER_NO_OUTPUT;
		}
	}
	if (result == PresenterResult::Fallback) {
		const PresenterResult recovered = presenter->implementation->recover();
		if (recovered == PresenterResult::Recovered) {
			result = draw();
			if (result == PresenterResult::Presented) {
				return VAEG_NATIVE_PRESENTER_PRESENTED;
			}
			if (result == PresenterResult::Disabled) {
				return VAEG_NATIVE_PRESENTER_NO_OUTPUT;
			}
		}
	}
	const PresenterError error = presenter->implementation->last_error();
	fprintf(stderr, "Native CRT presentation failed: backend=%s state=%s error=%s\n",
	        presenter->backend, vaeg::librashader::presenter_state_name(
	                                presenter->implementation->state()),
	        vaeg::librashader::presenter_error_name(error));
	return VAEG_NATIVE_PRESENTER_FALLBACK;
}

} // namespace

extern "C" int vaeg_native_presenter_is_headless_video_driver(const char *video_driver) {
	return (video_driver != nullptr) && (strcmp(video_driver, "dummy") == 0);
}

extern "C" void vaeg_native_presenter_set_output_capture(
    VAEG_NATIVE_PRESENTER *presenter, VAEG_OUTPUT_CAPTURE *capture) {
	if (presenter && presenter->implementation)
		presenter->implementation->set_output_capture(capture);
}

extern "C" VAEG_NATIVE_PRESENTER *vaeg_native_presenter_create(
	void *host_window, uint32_t drawable_width, uint32_t drawable_height,
	const char *preset_path, const char *parameter_state_path) {
	using namespace vaeg::librashader;
	std::unique_ptr<VAEG_NATIVE_PRESENTER> presenter;
	NativePresenterCreateInfo info{};
	PresenterResult result;

	snprintf(creation_error, sizeof(creation_error), "none");
	if (host_window == nullptr) {
		snprintf(creation_error, sizeof(creation_error), "invalid host window");
		return nullptr;
	}
	try {
		presenter = std::make_unique<VAEG_NATIVE_PRESENTER>();
		presenter->backend = native_backend_name();
		presenter->implementation = create_native_presenter(PresenterBackend::Automatic);
		if (presenter->implementation == nullptr) {
			snprintf(creation_error, sizeof(creation_error), "platform_unavailable");
			return nullptr;
		}
		info.host_window = host_window;
		info.drawable_width = drawable_width;
		info.drawable_height = drawable_height;
		info.backend = PresenterBackend::Automatic;
		info.enable_filter = true;
		info.preset_path = preset_path;
		info.parameter_state_path = parameter_state_path;
		result = presenter->implementation->initialize(info);
		if (result != PresenterResult::Recovered) {
			snprintf(creation_error, sizeof(creation_error), "%s",
			         vaeg_native_presenter_error(presenter.get()));
			fprintf(stderr, "Native CRT unavailable: backend=%s state=%s error=%s\n",
			        presenter->backend,
			        presenter_state_name(presenter->implementation->state()),
			        presenter_error_name(presenter->implementation->last_error()));
			return nullptr;
		}
		fprintf(stderr, "Native CRT active: backend=%s state=%s\n", presenter->backend,
		        presenter_state_name(presenter->implementation->state()));
		return presenter.release();
	} catch (...) {
		snprintf(creation_error, sizeof(creation_error), "presenter initialization exception");
		fprintf(stderr, "Native CRT unavailable: presenter initialization exception\n");
		return nullptr;
	}
}

extern "C" const char *vaeg_native_presenter_creation_error(void) {
	return creation_error;
}

extern "C" VAEG_NATIVE_PRESENTER_RESULT vaeg_native_presenter_resize(
	VAEG_NATIVE_PRESENTER *presenter, uint32_t drawable_width, uint32_t drawable_height) {
	using namespace vaeg::librashader;
	PresenterResult result;

	if ((presenter == nullptr) || (presenter->implementation == nullptr)) {
		return VAEG_NATIVE_PRESENTER_FALLBACK;
	}
	try {
		result = presenter->implementation->resize(drawable_width, drawable_height);
		if (result == PresenterResult::Recovered) {
			return VAEG_NATIVE_PRESENTER_PRESENTED;
		}
		if (result == PresenterResult::Disabled) {
			return VAEG_NATIVE_PRESENTER_NO_OUTPUT;
		}
		if (result == PresenterResult::Fallback) {
			result = presenter->implementation->recover();
			if (result == PresenterResult::Recovered) {
				return VAEG_NATIVE_PRESENTER_PRESENTED;
			}
		}
	} catch (...) {
		fprintf(stderr, "Native CRT resize failed: backend=%s\n", presenter->backend);
	}
	return VAEG_NATIVE_PRESENTER_FALLBACK;
}

extern "C" VAEG_NATIVE_PRESENTER_RESULT vaeg_native_presenter_present(
	VAEG_NATIVE_PRESENTER *presenter, const VAEG_FRAME_INPUT *frame) {
	try {
		return present_once(presenter, frame);
	} catch (...) {
		if (presenter != nullptr) {
			fprintf(stderr, "Native CRT presentation exception: backend=%s\n", presenter->backend);
		}
		return VAEG_NATIVE_PRESENTER_FALLBACK;
	}
}

extern "C" void vaeg_native_presenter_destroy(VAEG_NATIVE_PRESENTER *presenter) {
	if (presenter == nullptr) {
		return;
	}
	try {
		if (presenter->implementation != nullptr) {
			presenter->implementation->shutdown();
		}
		delete presenter;
	} catch (...) {
		delete presenter;
	}
}

extern "C" const char *vaeg_native_presenter_backend(
	const VAEG_NATIVE_PRESENTER *presenter) {
	return (presenter != nullptr) ? presenter->backend : "unavailable";
}

extern "C" const char *vaeg_native_presenter_state(
	const VAEG_NATIVE_PRESENTER *presenter) {
	if ((presenter == nullptr) || (presenter->implementation == nullptr)) {
		return "unavailable";
	}
	return vaeg::librashader::presenter_state_name(presenter->implementation->state());
}

extern "C" const char *vaeg_native_presenter_error(
	const VAEG_NATIVE_PRESENTER *presenter) {
	if ((presenter == nullptr) || (presenter->implementation == nullptr)) {
		return "platform_unavailable";
	}
	const char *detail = presenter->implementation->error_detail();
	return detail[0] ? detail : vaeg::librashader::presenter_error_name(presenter->implementation->last_error());
}

extern "C" int vaeg_native_presenter_gui_prepare(VAEG_NATIVE_PRESENTER *presenter) {
	return presenter && presenter->implementation && presenter->implementation->gui_prepare();
}

extern "C" void vaeg_native_presenter_gui_shutdown(VAEG_NATIVE_PRESENTER *presenter) {
	if (presenter && presenter->implementation) presenter->implementation->gui_shutdown();
}

extern "C" void vaeg_native_presenter_set_output_viewport(VAEG_NATIVE_PRESENTER *presenter,
                                                           int x, int y, int width, int height) {
	if (presenter && presenter->implementation)
		presenter->implementation->set_output_viewport(x, y, width, height);
}

extern "C" int vaeg_native_presenter_set_filter(VAEG_NATIVE_PRESENTER *presenter, int enabled) {
	if (!presenter || !presenter->implementation) return 0;
	const auto result = presenter->implementation->set_filter_enabled(enabled != 0);
	return result == vaeg::librashader::PresenterResult::Recovered ||
	       result == vaeg::librashader::PresenterResult::Disabled;
}

extern "C" int vaeg_native_presenter_set_parameter(VAEG_NATIVE_PRESENTER *presenter,
                                                    const char *name, float value) {
	return presenter && presenter->implementation &&
	       presenter->implementation->set_filter_parameter(name, value) ==
	           vaeg::librashader::PresenterResult::Recovered;
}
