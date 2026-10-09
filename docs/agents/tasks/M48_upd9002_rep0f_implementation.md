<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# M48 — Implement the explicitly approved REP+0F and state decision

## Session boundary

Work only on M48 after explicit G47 approval.  Do not infer a decision from
silence, broaden the approved transition, begin M49, or declare G48 passed.

## Prerequisites

G47 must explicitly name:

* one REP+0F semantic rule;
* one protected-residue state policy;
* exact dispatch/state changes;
* exact M42/M43 baseline-transition rows, hashes, and signatures;
* evidence sufficiency to begin M48.

If any item is unresolved, if source/dataset/document/probe identity drifted, or
if the implementation requires an unapproved change, stop for re-approval.

## Goal

Implement only the G47-approved semantic and state transition.  Apply exactly
the prospective manifest reviewed at G47, preserve all unaffected behavior, and
create a new explicit post-correction baseline rather than silently rewriting
M42 or M43 history.

## Requirements

1. Reproduce all M47 documentary, corpus, probe, and state evidence before edit.
2. Change only the approved REP+0F dispatch/semantics and state adapter policy.
3. Keep v30c_step as the sole primitive and v30cinit as the sole constructor.
4. Preserve CPU_SHUT FLAGS 0000 and all unaffected CPU286/UPD9002 bytes.
5. Record every changed graph/provenance/support row, record hash, classification,
   and failure signature against immutable M42/M43 artifacts.
6. Add focused regression coverage for the approved behavior and rejected or
   migrated state cases.
7. Prove production isolation, all supported presets, M42–M46 unaffected gates,
   full M43 transition accounting, and the standard human gate.

## Non-goals

No protected-mode deletion or inventory, broad instruction implementation,
timing/prefetch work, file/API rename, known-gap expansion outside the exact
approval, or unrelated state-version change.

## Gate

The implementation equals the exact G47 decision; every prospective transition
row is accounted for; no extra baseline record changes; state and CPU_SHUT
effects match approval; all builds/tests/human checks are green.  Stop at G48.
