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
#include "librashader/presenter_factory.h"

#if defined(__APPLE__) && defined(VAEG_ENABLE_LIBRASHADER)
#include "librashader/metal_presenter.h"
#endif

#if defined(_WIN32) && defined(VAEG_ENABLE_LIBRASHADER)
#include "librashader/d3d11_presenter.h"
#endif

#if defined(__linux__) && defined(VAEG_ENABLE_LIBRASHADER)
#include "librashader/gl_presenter.h"
#endif

namespace vaeg::librashader {

namespace {

class UnavailablePresenter final : public NativePresenter {
  public:
	PresenterState state() const noexcept override { return PresenterState::Unavailable; }
	PresenterError last_error() const noexcept override { return PresenterError::PlatformUnavailable; }
	PresenterResult initialize(const NativePresenterCreateInfo &) noexcept override {
		return PresenterResult::Fallback;
	}
	PresenterResult present(const VAEG_FRAME_INPUT &) noexcept override {
		return PresenterResult::Fallback;
	}
	PresenterResult set_filter_enabled(bool) noexcept override { return PresenterResult::Fallback; }
	PresenterResult resize(uint32_t, uint32_t) noexcept override { return PresenterResult::Fallback; }
	PresenterResult recover() noexcept override { return PresenterResult::Fallback; }
	void shutdown() noexcept override {}

  private:
	bool apply_backend_filter_parameter(const char *, float) noexcept override { return false; }
};

} // namespace

std::unique_ptr<NativePresenter> create_native_presenter(PresenterBackend backend) {
#if defined(__APPLE__) && defined(VAEG_ENABLE_LIBRASHADER)
	if ((backend == PresenterBackend::Automatic) || (backend == PresenterBackend::Metal)) {
		return create_metal_presenter();
	}
#elif defined(_WIN32) && defined(VAEG_ENABLE_LIBRASHADER)
	if ((backend == PresenterBackend::Automatic) || (backend == PresenterBackend::D3D11)) {
		return create_d3d11_presenter();
	}
#elif defined(__linux__) && defined(VAEG_ENABLE_LIBRASHADER)
	if ((backend == PresenterBackend::Automatic) || (backend == PresenterBackend::OpenGL)) {
		return create_gl_presenter();
	}
#else
	(void)backend;
#endif
	return std::make_unique<UnavailablePresenter>();
}

} // namespace vaeg::librashader
