#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

"""Static preset and reference-coordinate checks, not a GPU rendering test.

Optionally compile both stages with --glslang /path/to/glslangValidator.
"""
import argparse
import ctypes as ct
from fractions import Fraction
from pathlib import Path
import subprocess
import shutil
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
CRT = ROOT / "assets/shaders/crt"
SOURCE = (CRT / "shaders/vaeg-screen-size.slang").read_text()


class ScreenSizeTests(unittest.TestCase):
    def test_preset_order_and_extent(self):
        preset = dict(line.strip().split(" = ", 1)
                      for line in (CRT / "vaeg_crt_default.slangp").read_text().splitlines()
                      if " = " in line)
        self.assertEqual(preset["shaders"], "2")
        self.assertEqual(preset["shader0"], "shaders/vaeg-screen-size.slang")
        self.assertEqual(preset["shader1"], "shaders/vaeg-crt-aa.slang")
        self.assertEqual(preset["filter_linear0"], "false")
        self.assertEqual(preset["scale_type0"], "source")
        self.assertEqual(preset["scale0"], "1.0")
        self.assertEqual(preset["scale_type1"], "viewport")
        self.assertEqual(preset["CURVATURE"], '"0.030"')

    def test_shader_contract(self):
        self.assertIn('SCREEN_SIZE "Screen size (%)" 98.00 80.0 120.0 0.01', SOURCE)
        self.assertIn("texelFetch(Source, pixel, 0)", SOURCE)
        self.assertNotIn("texture(Source,", SOURCE)
        self.assertNotIn("#include", SOURCE)
        self.assertTrue(SOURCE.startswith("#version 450\n"))

    def test_copy_pixel_centers(self):
        # The GPU pass must address the same texel at every output pixel center.
        for extent in (640, 400, 672, 420, 800, 500):
            for x in range(extent):
                coordinate = Fraction(2 * x + 1, 2 * extent)
                self.assertEqual(int(coordinate * extent), x)

    def test_crt_mask_d3d11_lvalues(self):
        # X3500 regression: glslang accepts dynamic vector writes but the
        # runtime's D3DCompile does not. Raster parity covers all four masks.
        shader = (CRT / "shaders/vaeg-crt-aa.slang").read_text()
        self.assertNotRegex(shader, r"mask\s*\[[^\]]+\]\s*=")
        for component in "rgb":
            self.assertIn(f"mask.{component} = selected;", shader)

    def test_crt_mask_uses_raster_coordinates(self):
        shader = (CRT / "shaders/vaeg-crt-aa.slang").read_text()
        self.assertIn("vec2 pixel = gl_FragCoord.xy;", shader)
        self.assertNotIn("vTexCoord * params.OutputSize.xy", shader)
        # Guest sampling/curvature still use normalized image coordinates.
        self.assertIn("vec2 screen = vTexCoord * 2.0 - 1.0;", shader)


def compile_stages(compiler, spirv_cross=None):
    for name in ("vaeg-screen-size.slang", "vaeg-crt-aa.slang"):
        shader = (CRT / "shaders" / name).read_text()
        shader = shader.replace('#include "vaeg-scanline-aa.inc"',
                                (CRT / "shaders/vaeg-scanline-aa.inc").read_text())
        common, stages = shader.split("#pragma stage vertex", 1)
        vertex, fragment = stages.split("#pragma stage fragment", 1)
        common = "\n".join(line for line in common.splitlines()
                           if not line.startswith("#pragma parameter")) + "\n"
        with tempfile.TemporaryDirectory(prefix="vaeg-screen-size-") as directory:
            for suffix, stage in (("vert", vertex), ("frag", fragment)):
                source = Path(directory) / (name + "." + suffix)
                source.write_text(common + stage)
                subprocess.run([compiler, "-V", str(source), "-o",
                                str(Path(directory) / (suffix + ".spv"))], check=True)
                if spirv_cross:
                    hlsl = Path(directory) / (suffix + ".hlsl")
                    subprocess.run([spirv_cross, str(Path(directory) / (suffix + ".spv")),
                                    "--hlsl", "--shader-model", "50", "--output", str(hlsl)],
                                   check=True)
                    if name == "vaeg-crt-aa.slang" and suffix == "frag":
                        if "SV_Position" not in hlsl.read_text():
                            raise RuntimeError("M99_MASK_HLSL_POSITION_MISSING")
                    subprocess.run([compiler, "-D", "-V", "-S", suffix, "-e", "main",
                                    str(hlsl), "-o", str(Path(directory) / "hlsl.spv")],
                                   check=True)


def check_runtime(path, preset_file=CRT / "vaeg_crt_default.slangp"):
    """Exercise the pinned public C API without creating a GPU device."""
    class Parameter(ct.Structure):
        _fields_ = [("name", ct.c_char_p), ("description", ct.c_char_p),
                    ("initial", ct.c_float), ("minimum", ct.c_float),
                    ("maximum", ct.c_float), ("step", ct.c_float)]

    class Parameters(ct.Structure):
        _fields_ = [("parameters", ct.POINTER(Parameter)), ("length", ct.c_uint64)]

    runtime = ct.CDLL(str(Path(path).resolve()))
    pointer = ct.c_void_p
    signatures = {
        "libra_preset_create": ([ct.c_char_p, ct.POINTER(pointer)], pointer),
        "libra_preset_get_runtime_params": ([ct.POINTER(pointer), ct.POINTER(Parameters)], pointer),
        "libra_preset_get_param": ([ct.POINTER(pointer), ct.c_char_p, ct.POINTER(ct.c_float)], pointer),
        "libra_preset_free_runtime_params": ([Parameters], pointer),
        "libra_preset_free": ([ct.POINTER(pointer)], pointer),
        "libra_error_write": ([pointer, ct.POINTER(ct.c_char_p)], ct.c_int32),
        "libra_error_free_string": ([ct.POINTER(ct.c_char_p)], ct.c_int32),
        "libra_error_free": ([ct.POINTER(pointer)], ct.c_int32),
    }
    for name, (arguments, result) in signatures.items():
        function = getattr(runtime, name)
        function.argtypes, function.restype = arguments, result

    def checked(error):
        if error:
            detail = ct.c_char_p()
            runtime.libra_error_write(error, ct.byref(detail))
            message = detail.value.decode("utf-8", errors="replace") if detail.value else "unknown error"
            runtime.libra_error_free_string(ct.byref(detail))
            owned = pointer(error)
            runtime.libra_error_free(ct.byref(owned))
            raise RuntimeError(message)

    preset = pointer()
    checked(runtime.libra_preset_create(str(preset_file).encode(), ct.byref(preset)))
    try:
        params = Parameters()
        checked(runtime.libra_preset_get_runtime_params(ct.byref(preset), ct.byref(params)))
        try:
            values = {params.parameters[i].name.decode():
                      (params.parameters[i].initial, params.parameters[i].minimum,
                       params.parameters[i].maximum, params.parameters[i].step)
                      for i in range(params.length)}
            if values.get("SCREEN_SIZE") != (98.0, 80.0, 120.0, ct.c_float(0.01).value):
                raise RuntimeError("M99_SCREEN_SIZE_METADATA_MISMATCH")
            if values["CURVATURE"][0] != ct.c_float(0.030).value:
                raise RuntimeError("M99_CURVATURE_DEFAULT_MISMATCH")
            for name, metadata in values.items():
                initial = ct.c_float(metadata[0])
                checked(runtime.libra_preset_get_param(ct.byref(preset), name.encode(), ct.byref(initial)))
                expected = ct.c_float(0.030).value if name == "CURVATURE" else metadata[0]
                if initial.value != expected:
                    raise RuntimeError("M99_PRESET_DEFAULT_MISMATCH")
            print("Runtime parameter enumeration PASS:", values)
        finally:
            checked(runtime.libra_preset_free_runtime_params(params))
    finally:
        checked(runtime.libra_preset_free(ct.byref(preset)))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--glslang")
    parser.add_argument("--spirv-cross")
    parser.add_argument("--runtime")
    args = parser.parse_args()
    if args.glslang:
        compile_stages(args.glslang, args.spirv_cross)
    if args.runtime:
        check_runtime(args.runtime)
        with tempfile.TemporaryDirectory(prefix="vaeg-version-header-") as directory:
            fixture = Path(directory) / "crt"
            shutil.copytree(CRT, fixture)
            check_runtime(args.runtime, fixture / "vaeg_crt_default.slangp")
            shader = fixture / "shaders/vaeg-screen-size.slang"
            shader.write_text("/* header before version */\n" + shader.read_text())
            try:
                check_runtime(args.runtime, fixture / "vaeg_crt_default.slangp")
            except RuntimeError as error:
                if not str(error).startswith("PreprocessError(MissingVersionHeader):"):
                    raise
                print("MissingVersionHeader negative regression PASS")
            else:
                raise RuntimeError("M99_MISSING_VERSION_NOT_REJECTED")
    unittest.main(argv=[__file__])
