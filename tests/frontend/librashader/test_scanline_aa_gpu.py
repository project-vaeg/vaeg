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
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Optional EGL shader raster tests: needs ModernGL + NumPy, no ROMs.

Only descriptor syntax is translated from Slang to desktop GLSL. This is not
a librashader/Windows/physical-GPU lifecycle test. No private screenshots used.
"""
from pathlib import Path
import re
import time
import moderngl
import numpy as np

SHADERS = Path(__file__).resolve().parents[3] / "assets/shaders/crt/shaders"
VERTEX = """#version 450
layout(location=0) out vec2 vTexCoord;
void main() {
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    vTexCoord = p;
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
"""


def fragment(name, point_only=False, legacy_mask=False):
    source = (SHADERS / name).read_text()
    source = source.replace('#include "vaeg-scanline-aa.inc"',
                            (SHADERS / "vaeg-scanline-aa.inc").read_text())
    common, stages = source.split("#pragma stage vertex", 1)
    source = common + stages.split("#pragma stage fragment", 1)[1]
    source = re.sub(r"^#pragma parameter.*$", "", source, flags=re.M)
    source = re.sub(r"layout\(push_constant\) uniform Push\s*\{(.*?)\}\s*params;",
                    r"struct Push {\1}; uniform Push params;", source, flags=re.S)
    source = re.sub(r"set\s*=\s*0\s*,\s*", "", source)
    if point_only:
        # Test-local counterfactual: isolate all non-AA equations for parity.
        line = "float brightness = vaeg_scan_average(phase, thin, footprint);"
        if source.count(line) != 1:
            raise RuntimeError("M99_SCAN_GPU_POINT_FIXTURE")
        source = source.replace(line, "float brightness = vaeg_scan_point(phase, thin);")
    if legacy_mask:
        # One controlled mutation: reproduce the pre-fix coordinate coupling.
        line = "vec2 pixel = gl_FragCoord.xy;"
        if source.count(line) != 1:
            raise RuntimeError("M99_MASK_GPU_LEGACY_FIXTURE")
        source = source.replace(line, "vec2 pixel = vTexCoord * params.OutputSize.xy;")
    return source


def check(condition, code):
    if not condition:
        raise RuntimeError(code)


ctx = moderngl.create_standalone_context(require=450, backend="egl")
print("Renderer:", ctx.info["GL_RENDERER"], ctx.info["GL_VERSION"], flush=True)
programs = [ctx.program(vertex_shader=VERTEX, fragment_shader=fragment(name, counterfactual))
            for name, counterfactual in (("crt-lottes-fast.slang", False),
                                          ("vaeg-crt-aa.slang", False),
                                          ("vaeg-crt-aa.slang", True))]
programs.append(ctx.program(vertex_shader=VERTEX,
                           fragment_shader=fragment("vaeg-crt-aa.slang", legacy_mask=True)))
vaos = [ctx.vertex_array(p, []) for p in programs]
source = ctx.texture((656, 410), 3, dtype="f4")
source.filter = (moderngl.LINEAR, moderngl.LINEAR)
source.repeat_x = source.repeat_y = False
source.use(location=2)
defaults = dict(MASK=1., MASK_INTENSITY=.5, SCANLINE_THINNESS=.5, SCAN_BLUR=2.5,
                CURVATURE=.03, TRINITRON_CURVE=0., CORNER=3., CRT_GAMMA=2.4)


def configure(index, width, height, values):
    params = dict(defaults, **values)
    params.update(SourceSize=(656., 410., 1/656, 1/410),
                  OutputSize=(float(width), float(height), 1/width, 1/height))
    for key, value in params.items():
        name = "params." + key
        if name in programs[index]:
            programs[index][name].value = value


def draw(index, width, height, viewport=None, **values):
    configure(index, width, height, values)
    fbo = ctx.simple_framebuffer((width, height), components=4, dtype="f4")
    fbo.use()
    if viewport is not None:
        ctx.viewport = viewport
    vaos[index].render(vertices=3)
    pixels = np.frombuffer(fbo.read(components=3, dtype="f4"), np.float32).reshape(height, width, 3).copy()
    fbo.release()
    check(np.isfinite(pixels).all(), "M99_SCAN_GPU_NONFINITE")
    return pixels


# Same colors/kernel/warp/mask as the audited reference when AA is isolated.
rng = np.random.default_rng(2600)
source.write(rng.random((410, 656, 3), dtype=np.float32).tobytes())
for mask in range(4):
    for curve in (0., .03, .25):
        settings = dict(MASK=float(mask), CURVATURE=curve)
        old = draw(0, 320, 200, **settings)
        same = draw(2, 320, 200, **settings)
        error = float(np.abs(old - same).max())
        print("point parity", mask, curve, error, flush=True)
        check(error < .001, "M99_SCAN_GPU_COLOR_KERNEL_PARITY")

# No extra color/edge blur when modulation is constant (thinness=0).
old = draw(0, 640, 400, SCANLINE_THINNESS=0.)
new = draw(1, 640, 400, SCANLINE_THINNESS=0.)
check(float(np.abs(old - new).max()) < .001, "M99_SCAN_GPU_EDGE_PARITY")


def amplitude(rows, frequency):
    phase = np.arange(len(rows)) * (2 * np.pi * frequency)
    return float(2 * abs(np.sum(rows * np.exp(1j * phase))) / len(rows))


# Inset viewports on larger targets previously compressed a 3px mask to
# 1.92px, creating a 24px beat. Test actual raster viewports, not just uniforms.
source.write(np.broadcast_to(np.array((.6, .9, .9), np.float32), (410, 656, 3)).copy().tobytes())
def mask_period_error(pixels, viewport, period):
    x, y, _, _ = viewport
    interior = pixels[y + 120:y + 124, x + 64:x + 544]
    return float(np.abs(interior[:, period:] - interior[:, :-period]).max())


def require_mask_period(pixels, viewport, period):
    check(mask_period_error(pixels, viewport, period) < .00001,
          "M99_MASK_GPU_PIXEL_PERIOD")


for width, height, viewport in ((1000, 650, (180, 55, 640, 400)),
                                (1280, 800, (323, 101, 640, 400)),
                                (1001, 701, (37, 49, 641, 401))):
    for mask in range(4):
        settings = dict(MASK=float(mask), CURVATURE=0., CORNER=0.)
        fixed = draw(1, width, height, viewport=viewport, **settings)
        require_mask_period(fixed, viewport, 6 if mask == 3 else 3)
        print("mask pixel period", width, height, viewport, mask,
              mask_period_error(fixed, viewport, 6 if mask == 3 else 3), flush=True)
        if width == 1000 and mask == 1:
            bad = draw(3, width, height, viewport=viewport, **settings)
            try:
                require_mask_period(bad, viewport, 3)
            except RuntimeError as error:
                check(str(error) == "M99_MASK_GPU_PIXEL_PERIOD", "M99_MASK_GPU_WRONG_FAILURE")
            else:
                raise RuntimeError("M99_MASK_GPU_NEGATIVE_NOT_REJECTED")
            x, y, _, _ = viewport
            bands = [amplitude(p[y+120:y+124, x+64:x+544].mean(axis=(0, 2)), 1/24)
                     for p in (bad, fixed)]
            print("mask 24px band before/after", bands, flush=True)
            check(bands[0] > .005 and bands[1] < .00001, "M99_MASK_GPU_24PX_BAND")

# A pixel-exact no-mask A/B also guards against fixing the band by blurring
# or changing image geometry. The sole mutation is the unused mask coordinate.
source.write(rng.random((410, 656, 3), dtype=np.float32).tobytes())
viewport = (180, 55, 640, 400)
fixed = draw(1, 1000, 650, viewport=viewport, MASK=0.)
bad = draw(3, 1000, 650, viewport=viewport, MASK=0.)
check(np.array_equal(fixed[55:455, 180:820], bad[55:455, 180:820]),
      "M99_MASK_GPU_UNMASKED_IMAGE_CHANGED")


for flat in ((.4, .4, .4), (.6, .9, .9), (.9, .1, .1)):
    source.write(np.broadcast_to(np.array(flat, np.float32), (410, 656, 3)).copy().tobytes())
    for height in (400, 800, 1600):
        settings = dict(MASK=0., CURVATURE=0., CORNER=0.)
        # Complete periods of both the scanline and the beat: cropping 40 rows
        # leaks the fundamental into the 40px Fourier bin at 1600px output.
        rows = [draw(index, 256, height, **settings)[:, 64:192].mean(axis=(1, 2))
                for index in (0, 1)]
        bands = [amplitude(row, 1/40) for row in rows]
        print("raster 40px band", flat, height, bands, flush=True)
        # Do not demand a relative reduction of an already sub-LSB baseline.
        # Half an 8-bit code value is the absolute display-domain noise ceiling.
        check(bands[1] < max(bands[0] * .15, .5 / 255), "M99_SCAN_GPU_40PX_BAND")
        check(abs(float(rows[0].mean() - rows[1].mean())) < .04,
              "M99_SCAN_GPU_MEAN_EXPOSURE")

# Small-output limits including zero original weights, all curvature settings.
for thin in (0., .5, 1.):
    for curve in (0., .03, .25):
        draw(1, 160, 100, SCANLINE_THINNESS=thin, CURVATURE=curve)

# Software raster time only; no physical-GPU performance claim.
for height in (400, 1600):
    fbo = ctx.simple_framebuffer((640, height), components=4, dtype="f4")
    fbo.use()
    for index in (0, 3, 1):
        configure(index, 640, height, {})
        vaos[index].render(vertices=3)
        ctx.finish()
        times = []
        for _ in range(20):
            start = time.perf_counter()
            vaos[index].render(vertices=3)
            ctx.finish()
            times.append((time.perf_counter() - start) * 1000)
        label = {0: "original", 3: "AA old mask coordinates", 1: "AA raster coordinates"}[index]
        print("raster ms", height, label, "median/p95",
              np.percentile(times, [50, 95]).tolist(), flush=True)
    fbo.release()
print("M99_SCAN_GPU_PASS", flush=True)
