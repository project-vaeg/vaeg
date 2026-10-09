#
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
# USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

"""Exercise embedded preset extraction with the real static C API probe."""
import pathlib
import subprocess
import sys
import tempfile

probe = pathlib.Path(sys.argv[1]).resolve()
source = pathlib.Path(__file__).resolve().parents[3] / "assets/shaders/crt"
with tempfile.TemporaryDirectory(prefix="vaeg-static-cache-") as directory:
    work = pathlib.Path(directory) / "QA_日本語"
    work.mkdir()

    def invoke():
        return subprocess.run([str(probe)], cwd=work, capture_output=True, text=True)

    first = invoke()
    assert first.returncode == 0, first.stdout + first.stderr
    files = sorted(work.glob("vaeg-cache/shaders/**/*"))
    files = [p for p in files if p.is_file()]
    assert len(files) == 4, files
    root = next((work / "vaeg-cache/shaders").iterdir())
    for file in files:
        assert file.read_bytes() == (source / file.relative_to(root)).read_bytes()
    before = {p: p.stat().st_mtime_ns for p in files}
    second = invoke()
    assert second.returncode == 0, second.stdout + second.stderr
    assert before == {p: p.stat().st_mtime_ns for p in files}, "Cache was unnecessarily rewritten"
    # One controlled mutation after a passing real-runtime fixture.
    preset = root / "vaeg_crt_default.slangp"
    data = preset.read_bytes()
    preset.write_bytes(b"!" + data[1:])
    rejected = invoke()
    assert rejected.returncode == 2, rejected.stdout + rejected.stderr
    assert "VAEG_SHADER_CACHE_MISMATCH" in rejected.stderr, rejected.stderr
print("PASS: UTF-8 cwd, exact four-file closure, cache reuse, controlled corruption rejection")
