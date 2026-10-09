# M5 — EOL normalization: LF everywhere, declared exceptions CRLF

## Goal
Deterministic line endings enforced by `.gitattributes`, not by
contributor git config. Existing files are mostly LF already (CVS-era
history — see readme.md); this milestone converts the CRLF stragglers
and locks the policy.

## Steps
1. Add `.gitattributes` at repo root (template below) and commit it
   alone: `M5: add .gitattributes (EOL + binary policy)`.
2. Renormalize: `git add --renormalize .` → single commit
   `M5: normalize EOL to LF (mechanical)`.
   Verify with `git diff --stat HEAD~1`: whitespace-only. Spot-check a
   few files with `git diff --ignore-cr-at-eol HEAD~1` → must be empty.
3. Append the renormalization commit hash to `.git-blame-ignore-revs`
   (create the file if absent) in the same PR:
   `M5: add renormalization commit to blame-ignore-revs`.

## .gitattributes template
```
* text=auto eol=lf

# VS/VC project machinery and cmd scripts stay CRLF
*.dsp  text eol=crlf
*.dsw  text eol=crlf
*.sln  text eol=crlf
*.vcproj  text eol=crlf
*.vcxproj text eol=crlf
*.vcxproj.filters text eol=crlf
*.bat  text eol=crlf

# Binary payloads — never renormalize, never diff as text
*.rom binary
*.bin binary
*.d88 binary
*.ico binary
*.cur binary
*.bmp binary
*.png binary
*.wav binary
romimage/** binary
```
Adjust the binary list against the M0 census before committing; the
census, not this template, is authoritative.

## Machine checks
- `python tools/repo/check_eol.py --enforce` → clean.
- Both builds (VS2008, v141) pass.
- `.rc` files: confirm rc.exe accepts LF (it does; if any legacy tool
  in the build chain rejects LF, add that extension to the CRLF list
  and document it).

## GATE G5 (human)
Standard checklist on both binaries. **This is the last gate that
includes VS2008.** Pushed tags are immutable. After G5 passes, create a
NEW tag `vs2008-final` on the G5 commit; baseline-vs2008 stays where it
is.
