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
#include <memory>
#include "librashader/native_presenter_controller.h"
#include "librashader/presenter_factory.h"

using namespace vaeg::librashader;
namespace {
int initialize_count, recover_count, draw_count, gui_count, shutdown_count;
bool fail_filter, fail_device, fail_recovery, fail_initialize;
int viewport_values[4];
const void *observed_pixels;
uint32_t observed_width, observed_height;
const char *runtime_preset;
const char *diagnostic = "";
VAEG_OUTPUT_CAPTURE *capture_target;
class FakePresenter final : public NativePresenter {
    PresenterState state_ = PresenterState::Unavailable;
public:
    PresenterState state() const noexcept override { return state_; }
    PresenterError last_error() const noexcept override { return PresenterError::None; }
    const char *error_detail() const noexcept override { return diagnostic; }
    PresenterResult initialize(const NativePresenterCreateInfo &) noexcept override {
        if (fail_initialize) return PresenterResult::Fallback;
        if (runtime_preset && !prepare_filter_parameters(runtime_preset, true, nullptr))
            return PresenterResult::Fallback;
        ++initialize_count; state_ = PresenterState::Filtered; return PresenterResult::Recovered;
    }
    PresenterResult present(const VAEG_FRAME_INPUT &frame) noexcept override {
        ++draw_count; observed_pixels = frame.pixels;
        observed_width = frame.width; observed_height = frame.height;
        if (capture_target) capture_target->complete = 1;
        if (fail_device) { fail_device = false; return PresenterResult::Fallback; }
        if (fail_filter) {
            fail_filter = false; state_ = PresenterState::PassThrough;
            return PresenterResult::Recovered;
        }
        return PresenterResult::Presented;
    }
    PresenterResult set_filter_enabled(bool enabled) noexcept override {
        state_ = enabled ? PresenterState::Filtered : PresenterState::PassThrough;
        return enabled ? PresenterResult::Recovered : PresenterResult::Disabled;
    }
    PresenterResult resize(uint32_t w, uint32_t h) noexcept override {
        return w && h ? PresenterResult::Recovered : PresenterResult::Disabled;
    }
    PresenterResult recover() noexcept override {
        ++recover_count;
        return fail_recovery ? PresenterResult::Fallback : PresenterResult::Recovered;
    }
    void shutdown() noexcept override { ++shutdown_count; }
    bool gui_prepare() noexcept override { ++gui_count; return true; }
    void gui_shutdown() noexcept override { --gui_count; }
    void set_output_capture(VAEG_OUTPUT_CAPTURE *capture) noexcept override {
        capture_target = capture;
    }
    void set_output_viewport(int x, int y, int w, int h) noexcept override {
        viewport_values[0] = x; viewport_values[1] = y;
        viewport_values[2] = w; viewport_values[3] = h;
    }
private:
    bool apply_backend_filter_parameter(const char *, float) noexcept override { return true; }
};
int failures;
void check(bool ok, const char *name) {
    if (!ok) { ++failures; std::fprintf(stderr, "CONTROLLER_TEST_FAILED: %s\n", name); }
}
}
namespace vaeg::librashader {
std::unique_ptr<NativePresenter> create_native_presenter(PresenterBackend) {
    return std::make_unique<FakePresenter>();
}
}
int main(int argc, char **argv) {
    int window = 0;
    const unsigned char pixels[] = {0x00, 0xf8, 0xe0, 0x07};
    VAEG_FRAME_INPUT frame{};
    frame.pixels = pixels;
    auto *p = vaeg_native_presenter_create(&window, 640, 422, "test", nullptr);
    check(p && initialize_count == 1, "initialization");
    VAEG_OUTPUT_CAPTURE capture{};
    vaeg_native_presenter_set_output_capture(p, &capture);
    check(capture_target == &capture, "output capture target forwarded");
    check(vaeg_native_presenter_present(p, &frame) == VAEG_NATIVE_PRESENTER_PRESENTED &&
          capture.complete == 1, "capture completes during presentation");
    vaeg_native_presenter_set_output_capture(p, nullptr);
    check(capture_target == nullptr, "one-shot readback target detached");
    draw_count = 0;
    diagnostic = "Missing/unloadable D3DX9_43.dll";
    check(std::strcmp(vaeg_native_presenter_error(p), diagnostic) == 0,
          "runtime dependency detail reaches frontend");
    diagnostic = "";
    check(std::strcmp(vaeg_native_presenter_error(p), "none") == 0, "cleared diagnostic");
    check(vaeg_native_presenter_gui_prepare(p) == 1 && gui_count == 1, "GUI attach");
    vaeg_native_presenter_set_output_viewport(p, 0, 44, 1280, 800);
    check(viewport_values[1] == 44 && viewport_values[2] == 1280 &&
          viewport_values[3] == 800, "HiDPI menu inset");
    for (int i = 0; i < 100; ++i) {
        check(vaeg_native_presenter_set_filter(p, i % 2), "live toggle");
        check(std::strcmp(vaeg_native_presenter_state(p),
                          i % 2 ? "filtered" : "pass-through") == 0, "toggle state");
    }
    check(initialize_count == 1 && recover_count == 0, "toggle retains device");
    fail_filter = true;
    check(vaeg_native_presenter_present(p, &frame) == VAEG_NATIVE_PRESENTER_PRESENTED,
          "filter failure renders pass-through in same submission");
    check(draw_count == 2 && recover_count == 0, "filter recovery retains native ownership");
    check(observed_pixels == pixels && pixels[0] == 0 && pixels[1] == 0xf8,
          "raw frame remains borrowed and unchanged");
    check(vaeg_native_presenter_resize(p, 0, 0) == VAEG_NATIVE_PRESENTER_NO_OUTPUT,
          "minimized drawable");
    fail_device = true;
    check(vaeg_native_presenter_present(p, &frame) == VAEG_NATIVE_PRESENTER_PRESENTED &&
          recover_count == 1, "device recovery retries");
    fail_device = fail_recovery = true;
    check(vaeg_native_presenter_present(p, &frame) == VAEG_NATIVE_PRESENTER_FALLBACK,
          "failed recovery requests SDL");
    vaeg_native_presenter_gui_shutdown(p);
    vaeg_native_presenter_destroy(p);
    check(gui_count == 0 && shutdown_count == 1, "GUI detaches before teardown");
    check(!vaeg_native_presenter_gui_prepare(nullptr) &&
          !vaeg_native_presenter_set_filter(nullptr, 1), "null lifecycle");
    fail_initialize = true;
    diagnostic = "test GPU initialization failure";
    check(!vaeg_native_presenter_create(&window, 640, 422, "test", nullptr),
          "failed creation returns null");
    check(std::strcmp(vaeg_native_presenter_creation_error(), diagnostic) == 0,
          "creation detail survives failed presenter destruction");
    fail_initialize = false;
    diagnostic = "";
    p = vaeg_native_presenter_create(&window, 640, 422, "test", nullptr);
    check(p && std::strcmp(vaeg_native_presenter_creation_error(), "none") == 0,
          "successful retry clears creation failure");
    vaeg_native_presenter_destroy(p);
    if (argc == 2) {
        // Optional real-runtime metadata + fake GPU integration, no physical rendering.
        runtime_preset = argv[1];
        fail_recovery = false;
        unsigned char grid[32]{};
        frame = {grid, 4, 4, 8, VAEG_FRAME_PIXEL_RGB565, VAEG_FRAME_ROWS_TOP_DOWN,
                 1, 1, 60, 1, 1, 16666667};
        p = vaeg_native_presenter_create(&window, 640, 422, "test", nullptr);
        check(p != nullptr, "real runtime preset metadata");
        check(vaeg_native_presenter_set_parameter(p, "SCREEN_SIZE", 80), "padding control");
        check(vaeg_native_presenter_present(p, &frame) == VAEG_NATIVE_PRESENTER_PRESENTED &&
              observed_width == 6 && observed_height == 6 && observed_pixels != grid,
              "controller supplies padded input");
        const int initial_creates = initialize_count;
        check(vaeg_native_presenter_set_filter(p, 0), "menu pass-through");
        check(vaeg_native_presenter_present(p, &frame) == VAEG_NATIVE_PRESENTER_PRESENTED &&
              observed_width == 4 && observed_height == 4 && observed_pixels == grid,
              "menu pass-through bypasses CRT padding");
        check(vaeg_native_presenter_set_filter(p, 1), "menu CRT resume");
        check(vaeg_native_presenter_present(p, &frame) == VAEG_NATIVE_PRESENTER_PRESENTED &&
              observed_width == 6 && observed_height == 6 && initialize_count == initial_creates,
              "menu resume retains preset parameters and device");
        fail_filter = true;
        check(vaeg_native_presenter_present(p, &frame) == VAEG_NATIVE_PRESENTER_PRESENTED &&
              observed_width == 4 && observed_pixels == grid,
              "failed filter retries unpadded input");
        check(vaeg_native_presenter_set_filter(p, 1), "reenable filter");
        check(vaeg_native_presenter_set_parameter(p, "SCREEN_SIZE", 100), "100 control");
        check(vaeg_native_presenter_present(p, &frame) == VAEG_NATIVE_PRESENTER_PRESENTED &&
              observed_width == 4 && observed_pixels == grid, "100 bypass after padding");
        vaeg_native_presenter_destroy(p);
    }
    std::printf("controller checks: %s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
