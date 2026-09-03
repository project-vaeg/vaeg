#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

import argparse
import hashlib
import pathlib
import re
import unittest


EXPECTED_SHA256 = {
    "assets/shaders/crt/vaeg_crt_lottes_fast.slangp":
        "8489d84dce043c5f68b7b6c7b7e3721bcad82b9b3498a8320f00fba7d6135835",
    "assets/shaders/crt/shaders/crt-lottes-fast.slang":
        "576eddc662ac4f77909c0c14dbd5a16ac4164e50c67527fff634316f4441c482",
    "third_party/librashader/include/librashader.h":
        "5d478897c391af3f60015810b67785ae1a286d262a845485276e36ded9f21e62",
    "third_party/librashader/include/librashader_ld.h":
        "bcffcbc854afb287c9f935c1a0e3b569f5e6775ef85914bd7f7025ad1f6bde33",
}

FORBIDDEN_SHADER_MARKERS = (
    "crt-geom",
    "crt-royale",
    "mega-bezel",
    "mega_bezel",
)


class LibrashaderPipelineTest(unittest.TestCase):
    root = pathlib.Path()

    def read(self, relative):
        return (self.root / relative).read_text(encoding="utf-8")

    def test_bundled_hashes(self):
        for relative, expected in EXPECTED_SHA256.items():
            digest = hashlib.sha256((self.root / relative).read_bytes()).hexdigest()
            self.assertEqual(expected, digest, relative)

    def test_shader_denylist_and_dependency_closure(self):
        shader_root = self.root / "assets/shaders"
        self.assertTrue(shader_root.is_dir())
        for path in shader_root.rglob("*"):
            lower_name = path.name.lower()
            self.assertFalse(
                any(marker in lower_name for marker in FORBIDDEN_SHADER_MARKERS),
                path,
            )
            if path.suffix.lower() in (".slang", ".slangp"):
                text = path.read_text(encoding="utf-8").lower()
                self.assertNotIn("gpl", text, path)
        shader = self.read("assets/shaders/crt/shaders/crt-lottes-fast.slang")
        self.assertNotRegex(shader, r"(?m)^\s*#include\b")
        preset = self.read("assets/shaders/crt/vaeg_crt_lottes_fast.slangp")
        self.assertNotRegex(preset, r"(?mi)^\s*(textures|luts?)\s*=")

    def test_dynamic_loader_and_optional_build_boundary(self):
        cmake = self.read("CMakeLists.txt")
        self.assertRegex(cmake, r"option\(VAEG_ENABLE_LIBRASHADER[\s\S]*?\bOFF\)")
        self.assertIn("if(VAEG_ENABLE_LIBRASHADER)", cmake)
        self.assertIn("${CMAKE_DL_LIBS}", cmake)
        self.assertNotRegex(cmake, r"target_link_libraries\([^)]*librashader")
        loader = self.read("sdl2/librashader/librashader_loader.cpp")
        self.assertIn("librashader_load_instance", loader)
        self.assertRegex(loader, r"dlopen\(\"librashader\.so\"")
        self.assertIn("LIBRASHADER_CURRENT_ABI", loader)
        self.assertIn("LIBRASHADER_CURRENT_VERSION", loader)

    def test_filter_chain_is_not_created_per_frame(self):
        bridge = self.read("sdl2/librashader/librashader_gl_bridge.cpp")
        render_start = bridge.index("bool LibrashaderGlBridge::render_frame")
        draw_start = bridge.index("bool LibrashaderGlBridge::draw_output", render_start)
        render_body = bridge[render_start:draw_start]
        self.assertNotIn("gl_filter_chain_create", render_body)
        self.assertNotRegex(render_body, r"\b(new|malloc|calloc|realloc)\b")
        self.assertIn("gl_filter_chain_frame", render_body)

    def test_source_texture_is_complete_for_librashader_samplers(self):
        bridge = self.read("sdl2/librashader/librashader_gl_bridge.cpp")
        self.assertIn(
            'LOAD_GL(generate_mipmap, GenerateMipmap, "glGenerateMipmap")',
            bridge,
        )
        self.assertIn("api_->generate_mipmap(GL_TEXTURE_2D)", bridge)

    def test_gl_smoke_covers_window_lifecycle_and_benchmark(self):
        smoke = self.read("tests/frontend/librashader/gl_smoke.cpp")
        self.assertIn("SDL_WINDOW_RESIZABLE", smoke)
        self.assertIn("SDL_SetWindowFullscreen", smoke)
        self.assertIn("context recreation passed", smoke)
        self.assertIn("librashader GL benchmark", smoke)

    def test_raw_capture_stays_upstream(self):
        screen = self.read("sdl2/scrnmng.c")
        raw_start = screen.index("BOOL scrnmng_save_guest_frame(")
        raw_end = screen.index("BOOL scrnmng_save_guest_frame_with_analysis", raw_start)
        self.assertNotIn("librashader", screen[raw_start:raw_end].lower())
        present_start = screen.index("void scrnmng_present_begin(void)")
        present_end = screen.index("void scrnmng_present_end(void)", present_start)
        self.assertIn("librashader_gl_bridge_render_frame", screen[present_start:present_end])

    def test_settings_metadata_and_safe_migration(self):
        ini = self.read("sdl2/ini.c")
        np2 = self.read("sdl2/np2.c")
        gui = self.read("sdl2/gui/gui.cpp")
        self.assertIn("VAEG_LIBRASHADER_SETTINGS_VERSION", ini)
        self.assertIn("librashader_overrides", ini)
        self.assertIn("np2_validate_librashader_selection", np2)
        self.assertIn('"recommended"', np2)
        self.assertIn("isfinite(parsed)", np2)
        self.assertIn("CRT Filter Settings...", gui)
        self.assertIn("Capture Presented Frame", gui)
        bridge = self.read("sdl2/librashader/librashader_gl_bridge.cpp")
        self.assertIn("preset_get_runtime_params", bridge)
        self.assertIn("gl_filter_chain_set_param", bridge)

    def test_release_archive_records_and_stages_all_required_inputs(self):
        workflow = self.read(".github/workflows/release.yml")
        self.assertEqual(3, workflow.count("build-librashader-runtime.sh"))
        self.assertEqual(3, workflow.count("LIBRASHADER_MPL-2.0.txt"))
        self.assertEqual(3, workflow.count("LIBRASHADER_SOURCE.txt"))
        self.assertEqual(3, workflow.count("CRT_LOTTES_FAST_PROVENANCE.md"))
        self.assertEqual(3, workflow.count("inspect-librashader-package.py"))
        self.assertNotRegex(workflow.lower(), r"crt[-_]geom|crt[-_]royale|mega[-_]bezel")
        source_record = self.read("third_party/librashader/readme.vaeg.md")
        self.assertIn("87e8a97b50516d997defeaa168173dcd185d4022", source_record)
        self.assertIn("runtime-opengl", source_record)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=pathlib.Path, required=True)
    args = parser.parse_args()
    LibrashaderPipelineTest.root = args.root.resolve()
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(LibrashaderPipelineTest)
    return not unittest.TextTestRunner(verbosity=2).run(suite).wasSuccessful()


if __name__ == "__main__":
    raise SystemExit(main())
