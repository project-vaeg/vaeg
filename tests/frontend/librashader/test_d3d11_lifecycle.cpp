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
#include "librashader/d3d11_presenter.h"
#include "librashader/d3d11_bridge.h"
using namespace vaeg::librashader;
namespace {
int failures, creates, draws, gui_attaches, gui_detaches;
int output[4];
char preset_seen[1024];
VAEG_D3D11_BRIDGE_RESULT next_draw = VAEG_D3D11_BRIDGE_OK;
void check(bool ok, const char *message) {
    if (!ok) { ++failures; std::fprintf(stderr, "D3D11_LIFECYCLE_FAILED: %s\n", message); }
}
}
extern "C" int vaeg_d3d11_bridge_initialize(void *, const char *preset, int,
                                            VAEG_D3D11_BRIDGE *b) {
    ++creates; b->state = b;
    std::snprintf(preset_seen, sizeof(preset_seen), "%s", preset ? preset : "");
    return 1;
}
extern "C" void vaeg_d3d11_bridge_shutdown(VAEG_D3D11_BRIDGE *b) { b->state = nullptr; }
extern "C" VAEG_D3D11_BRIDGE_RESULT vaeg_d3d11_bridge_set_drawable_size(
    VAEG_D3D11_BRIDGE *, uint32_t, uint32_t) { return VAEG_D3D11_BRIDGE_OK; }
extern "C" VAEG_D3D11_BRIDGE_RESULT vaeg_d3d11_bridge_set_filter_enabled(
    VAEG_D3D11_BRIDGE *, int) { return VAEG_D3D11_BRIDGE_OK; }
extern "C" VAEG_D3D11_BRIDGE_RESULT vaeg_d3d11_bridge_set_filter_parameter(
    VAEG_D3D11_BRIDGE *, const char *, float) { return VAEG_D3D11_BRIDGE_OK; }
extern "C" VAEG_D3D11_BRIDGE_RESULT vaeg_d3d11_bridge_present(
    VAEG_D3D11_BRIDGE *b, const VAEG_FRAME_INPUT *) {
    ++draws; auto result = next_draw; next_draw = VAEG_D3D11_BRIDGE_OK;
    if (result == VAEG_D3D11_BRIDGE_FILTER_FAILURE)
        std::snprintf(b->error, sizeof(b->error), "fixture filter compilation failed");
    return result;
}
extern "C" int vaeg_d3d11_bridge_gui_prepare(VAEG_D3D11_BRIDGE *) {
    ++gui_attaches; return 1;
}
extern "C" void vaeg_d3d11_bridge_gui_shutdown(VAEG_D3D11_BRIDGE *) { ++gui_detaches; }
extern "C" void vaeg_d3d11_bridge_set_output_viewport(VAEG_D3D11_BRIDGE *,
                                                      int x, int y, int w, int h) {
    output[0] = x; output[1] = y; output[2] = w; output[3] = h;
}
int main() {
    int window = 0;
    auto p = create_d3d11_presenter();
    NativePresenterCreateInfo info{&window, 1280, 844, PresenterBackend::D3D11,
                                   false, "fixture.slangp", ""};
    check(p->initialize(info) == PresenterResult::Recovered, "initialize");
    p->set_output_viewport(0, 44, 1280, 800);
    check(p->gui_prepare(), "GUI attach");
    for (int i = 0; i < 50; ++i) {
        check(p->set_filter_enabled(true) == PresenterResult::Recovered, "enable");
        check(p->set_filter_enabled(false) == PresenterResult::Disabled, "disable");
    }
    check(creates == 1, "toggles do not recreate device");
    VAEG_FRAME_INPUT frame{};
    next_draw = VAEG_D3D11_BRIDGE_FILTER_FAILURE;
    check(p->present(frame) == PresenterResult::Recovered &&
          p->state() == PresenterState::PassThrough, "filter failure preserves native output");
    check(p->present(frame) == PresenterResult::Presented &&
          p->last_error() == PresenterError::FilterFailure, "fallback reason remains visible");
    check(std::strcmp(p->error_detail(), "fixture filter compilation failed") == 0,
          "bridge error survives successful pass-through presentation");
    check(p->resize(0, 0) == PresenterResult::Disabled, "minimize");
    next_draw = VAEG_D3D11_BRIDGE_DEVICE_LOST;
    check(p->present(frame) == PresenterResult::Fallback, "device loss");
    check(p->recover() == PresenterResult::Recovered && creates == 2, "device reconstruction");
    check(p->error_detail()[0] == '\0', "successful recovery clears old diagnostic");
    check(std::strcmp(preset_seen, "fixture.slangp") == 0,
          "recovery does not alias and overwrite remembered preset");
    check(output[0] == 0 && output[1] == 44 && output[2] == 1280 && output[3] == 800,
          "recovery preserves GUI inset and scaling viewport");
    p->gui_shutdown();
    check(gui_attaches == 1 && gui_detaches == 1, "GUI lifecycle forwarding");
    p->shutdown(); p->shutdown();
    check(p->state() == PresenterState::Unavailable, "repeated teardown");
    std::printf("D3D11 presenter mock checks: %s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
