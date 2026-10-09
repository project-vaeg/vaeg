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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# IRET restoration audit schema v1

The M60e evidence pack audits all 5,000 selected and applicable `CF IRET`
records. Case rows are sorted by the canonical SHA-256 of the complete SST
record and contain initial, expected, and actual state side by side.

The six stack bytes are interpreted independently as three little-endian
words: restored IP, restored CS, and restored FLAGS. Logical addresses use
16-bit offset wrapping and physical addresses use the 20-bit target address
mask. Final-state reconstruction does not claim transient bus-read ordering.

The FLAGS table contains one row for each bit 0 through 15. Rule values are
`loadable`, `preserved`, `forced-zero`, `forced-one`,
`condition-dependent`, or `undetermined`. Architectural and full-16-bit
fingerprint relevance are recorded separately.

All JSON is UTF-8, LF, canonically key-sorted, and timestamp-free.
`iret_cases.json.gz` uses the repository deterministic gzip writer.
