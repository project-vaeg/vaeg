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
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

"""check_eol.py honours binary .gitattributes, tested in a temporary repo."""

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

CHECKER = Path(__file__).resolve().parents[2] / "tools" / "repo" / "check_eol.py"


def git(repo, *args):
    subprocess.run(["git", *args], cwd=repo, check=True, capture_output=True)


class CheckEolTest(unittest.TestCase):
    def make_repo(self, directory, attributes):
        repo = Path(directory)
        git(repo, "init", "-q")
        (repo / ".gitattributes").write_text(attributes)
        (repo / "plain.txt").write_bytes(b"lf only\n")
        (repo / "evidence").mkdir()
        (repo / "evidence" / "out.txt").write_bytes(b"from cp/m\r\n\x1a\x1a")
        git(repo, "add", "-A")
        return repo

    def run_checker(self, repo):
        return subprocess.run(
            [sys.executable, str(CHECKER), "--enforce"], cwd=repo, capture_output=True, text=True
        )

    def test_binary_attribute_skips_crlf_evidence(self):
        with tempfile.TemporaryDirectory() as directory:
            repo = self.make_repo(directory, "* text=auto eol=lf\nevidence/** binary\n")
            result = self.run_checker(repo)
            self.assertEqual(result.returncode, 0, result.stderr)

    def test_crlf_without_binary_attribute_is_a_violation(self):
        with tempfile.TemporaryDirectory() as directory:
            repo = self.make_repo(directory, "* text=auto eol=lf\n")
            result = self.run_checker(repo)
            self.assertEqual(result.returncode, 1)
            self.assertIn("VIOLATION [CRLF, want LF] evidence/out.txt", result.stderr)


if __name__ == "__main__":
    unittest.main()
