<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M99a: History Boundary

Status: PASS

M99 starts from the clean pre-M99 baseline. The prior M99 implementation,
its reports, shader assets, dependency material, and generated evidence were
excluded from the rebuilt line.

The post-baseline history was classified by changed paths and commit purpose.
Only the unrelated utility-media changes required on `main` were retained,
and each was replayed as a newly created commit. No rejected M99 commit was
cherry-picked or used as an ancestor.

No backup branch, tag, recovery ref, or M99-derived artifact was created.
The rebuilt candidate was checked for a matching tree and for the absence of
M99 implementation paths before publication.

Verification:

- the repository roadmap, conventions, and M99 task specification were read;
- the candidate contains no reachable M99 implementation commit;
- the candidate tree matches the retained main content;
- the clean-baseline build and CTest suite completed successfully.
