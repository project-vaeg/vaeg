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
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

"""Check the standalone notice generator without SDL, a GPU, or Git history."""
import pathlib
import re
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]


class LicenseEmbeddingTest(unittest.TestCase):
    def generate(self, crt):
        with tempfile.TemporaryDirectory(prefix="vaeg-licenses-") as directory:
            work = pathlib.Path(directory)
            project = work / "CMakeLists.txt"
            project.write_text(
                "cmake_minimum_required(VERSION 3.20)\n"
                "project(license_test NONE)\n"
                f'set(CMAKE_CURRENT_SOURCE_DIR "{ROOT.as_posix()}")\n'
                f"set(VAEG_ENABLE_LIBRASHADER {'ON' if crt else 'OFF'})\n"
                "set(VAEG_ENABLE_ARCHIVE_DROP OFF)\n"
                'file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")\n'
                f'include("{ROOT.as_posix()}/cmake/third_party_licenses.cmake")\n',
                encoding="utf-8",
            )
            subprocess.run(
                ["cmake", "-S", str(work), "-B", str(work / "build")],
                check=True, capture_output=True, text=True,
            )
            return (work / "build/generated/vaeg_licenses.h").read_text(encoding="utf-8")

    def check_common(self, generated):
        entries = re.findall(r'\{"([^"]+)", R"vaeglicense\((.*?)\)vaeglicense"\}', generated, re.S)
        self.assertGreaterEqual(len(entries), 12)
        self.assertEqual(len(entries), len({name for name, _ in entries}))
        for relative in (
            "external/imgui/LICENSE.txt",
            "external/ymfm/LICENSE",
            "external/suzukiplan-z80/LICENSE.txt",
            "assets/OFL.txt",
            "LICENSES/legacy-vaeg.txt",
            "docs/licenses/sdl2-notice.txt",
        ):
            expected = (ROOT / relative).read_text(encoding="utf-8")
            self.assertIn(expected, [text for _, text in entries], relative)
        for component in ("rectpack", "textedit", "truetype"):
            original = (ROOT / f"external/imgui/imstb_{component}.h").read_text(encoding="utf-8")
            expected = original[original.index("ALTERNATIVE A - MIT License"):].split("*/", 1)[0]
            self.assertIn(expected, [text for _, text in entries])
        self.assertNotIn('{"LibArchive', generated)

    def test_feature_off(self):
        generated = self.generate(False)
        self.check_common(generated)
        self.assertNotIn('{"librashader', generated)
        self.assertNotIn('{"CRT', generated)

    def test_feature_on(self):
        generated = self.generate(True)
        self.check_common(generated)
        self.assertIn((ROOT / "external/librashader/LICENSE.md").read_text(encoding="utf-8"), generated)
        original = (ROOT / "external/librashader/include/librashader_ld.h").read_text(encoding="utf-8")
        self.assertIn(original[2:original.index("*/")], generated)
        self.assertIn((ROOT / "assets/shaders/crt/licenses/crt-default-license.txt").read_text(encoding="utf-8"), generated)


if __name__ == "__main__":
    unittest.main()
