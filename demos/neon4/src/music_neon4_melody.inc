; Copyright (c) 2026 Nakata Maho
; Ported By Maho Nakata.
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the BSD 2-Clause conditions apply.

; ---------------------------------------------------------------------------
; MUSIC_NEON4_MELODY.INC
; NEON4 0.29 scene-linked hybrid: N4ORG bars 1..8, N4A3 Metro Pulse bars 9..32.
; REST=0FFh, HOLD=0FEh. 32 bars x 16 steps = 512 steps.
;
; Design derived from the strengths of the first NEON BGM, without copying it:
;   * Keep harmony simple enough to read immediately; surface colour supplies complexity.
;   * Establish one two-bar lead hook by literal repetition before varying it.
;   * Give every layer one job: lead / anchor / bass / pad / arpeggio / pulse / fill.
;   * Keep the meter fixed. Energy changes by subdivision and layer removal, not timing drift.
;   * Strong is the default state. Bars 13..15, 21..23 and 29 are deliberate drops.
;   * Scene 2 (bars 9..12) keeps the same meter and harmony roles: bars 9..10 restate A,
;     bars 11..12 reuse the already-established A-prime contour and phrase-end fill.
;   * FM3..5 are sustained OPNA/OPL3 colour, not competing lead phrases.
;   * Native OPNA HH remains unused; BD/SD/CYM/TOM/RIM make the drum structure.
; ---------------------------------------------------------------------------

music_data:

; Channel 0: lead - two-bar hook, literal repetition first, variation later
        db 44h,0FEh,0FFh,0FFh,47h,0FFh,49h,0FFh,4Bh,0FEh,0FFh,0FFh,52h,0FEh,0FFh,0FFh ; bar 01
        db 4Bh,0FEh,0FFh,0FFh,49h,0FFh,47h,0FFh,44h,0FEh,0FFh,0FFh,42h,0FEh,0FFh,0FFh ; bar 02
        db 44h,0FEh,0FFh,0FFh,47h,0FFh,49h,0FFh,4Bh,0FEh,0FFh,0FFh,52h,0FEh,0FFh,0FFh ; bar 03
        db 4Bh,0FEh,0FFh,0FFh,49h,0FFh,47h,0FFh,44h,0FEh,0FFh,0FFh,42h,0FEh,0FFh,0FFh ; bar 04
        db 44h,0FEh,0FFh,0FFh,47h,0FFh,49h,0FFh,4Bh,0FEh,0FFh,0FFh,52h,0FEh,0FFh,0FFh ; bar 05
        db 4Bh,0FEh,0FFh,0FFh,49h,0FFh,47h,0FFh,44h,0FEh,0FFh,0FFh,42h,0FEh,0FFh,0FFh ; bar 06
        db 44h,0FEh,0FFh,0FFh,47h,0FFh,49h,0FFh,4Bh,0FEh,0FFh,0FFh,54h,0FEh,0FFh,0FFh ; bar 07
        db 52h,0FEh,0FFh,0FFh,4Bh,0FFh,49h,0FFh,47h,0FEh,0FFh,0FFh,44h,0FEh,0FFh,0FFh ; bar 08
        db 44h,0FEh,0FFh,0FFh,47h,0FFh,49h,0FFh,4Bh,0FEh,0FFh,0FFh,52h,0FEh,0FFh,0FFh ; bar 09
        db 4Bh,0FEh,0FFh,0FFh,49h,0FFh,47h,0FFh,44h,0FEh,0FFh,0FFh,42h,0FEh,0FFh,0FFh ; bar 10
        db 44h,0FEh,0FFh,0FFh,47h,0FFh,49h,0FFh,4Bh,0FEh,0FFh,0FFh,54h,0FEh,0FFh,0FFh ; bar 11
        db 52h,0FEh,0FFh,0FFh,4Bh,0FFh,49h,0FFh,47h,0FEh,0FFh,0FFh,44h,0FEh,0FFh,0FFh ; bar 12
        db 44h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 13
        db 47h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 14
        db 49h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 15
        db 4Bh,0FEh,0FFh,0FFh,52h,0FFh,54h,0FFh,52h,0FFh,4Bh,0FFh,49h,0FFh,47h,0FFh ; bar 16
        db 47h,0FEh,0FFh,0FFh,49h,0FFh,4Bh,0FFh,52h,0FEh,0FFh,0FFh,54h,0FEh,0FFh,0FFh ; bar 17
        db 52h,0FEh,0FFh,0FFh,4Bh,0FFh,49h,0FFh,47h,0FEh,0FFh,0FFh,44h,0FEh,0FFh,0FFh ; bar 18
        db 47h,0FEh,0FFh,0FFh,49h,0FFh,4Bh,0FFh,52h,0FEh,0FFh,0FFh,54h,0FEh,0FFh,0FFh ; bar 19
        db 52h,0FEh,0FFh,0FFh,4Bh,0FFh,49h,0FFh,47h,0FEh,0FFh,0FFh,44h,0FEh,0FFh,0FFh ; bar 20
        db 47h,0FEh,0FFh,0FFh,4Bh,0FFh,52h,0FFh,54h,0FEh,0FFh,0FFh,52h,0FEh,0FFh,0FFh ; bar 21
        db 52h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 22
        db 4Bh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 23
        db 49h,0FFh,4Bh,0FFh,52h,0FFh,54h,0FFh,57h,0FFh,54h,0FFh,52h,0FFh,4Bh,0FFh ; bar 24
        db 44h,0FEh,0FFh,0FFh,47h,0FFh,49h,0FFh,4Bh,0FEh,0FFh,0FFh,52h,0FEh,0FFh,0FFh ; bar 25
        db 4Bh,0FEh,0FFh,0FFh,49h,0FFh,47h,0FFh,44h,0FEh,0FFh,0FFh,42h,0FEh,0FFh,0FFh ; bar 26
        db 44h,0FEh,0FFh,0FFh,47h,0FFh,49h,0FFh,4Bh,0FEh,0FFh,0FFh,54h,0FEh,0FFh,0FFh ; bar 27
        db 52h,0FEh,0FFh,0FFh,4Bh,0FFh,49h,0FFh,47h,0FEh,0FFh,0FFh,44h,0FEh,0FFh,0FFh ; bar 28
        db 44h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh,0FFh,0FFh ; bar 29
        db 44h,0FEh,0FFh,0FFh,47h,0FFh,49h,0FFh,4Bh,0FEh,0FFh,0FFh,54h,0FEh,0FFh,0FFh ; bar 30
        db 52h,0FEh,0FFh,0FFh,4Bh,0FFh,49h,0FFh,47h,0FEh,0FFh,0FFh,44h,0FEh,0FFh,0FFh ; bar 31
        db 44h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh ; bar 32

; Channel 1: harmonic anchor - one sustained voice-led chord tone per bar
        db 37h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 01
        db 36h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 02
        db 34h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 03
        db 42h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 04
        db 37h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 05
        db 36h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 06
        db 34h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 07
        db 42h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 08
        db 37h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 09
        db 36h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 10
        db 34h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 11
        db 42h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 12
        db 34h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 13
        db 36h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 14
        db 37h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 15
        db 42h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 16
        db 3Bh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 17
        db 36h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 18
        db 37h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 19
        db 34h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 20
        db 3Bh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 21
        db 36h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 22
        db 34h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 23
        db 42h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 24
        db 37h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 25
        db 36h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 26
        db 34h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 27
        db 42h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 28
        db 34h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 29
        db 36h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 30
        db 42h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 31
        db 37h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 32

; Channel 2: synth bass - fixed eighth-note root/fifth engine with rare fills
        db 24h,0FFh,24h,0FFh,2Bh,0FFh,34h,0FFh,24h,0FFh,24h,0FFh,2Bh,0FFh,34h,0FFh ; bar 01
        db 22h,0FFh,22h,0FFh,29h,0FFh,32h,0FFh,22h,0FFh,22h,0FFh,29h,0FFh,32h,0FFh ; bar 02
        db 20h,0FFh,20h,0FFh,27h,0FFh,30h,0FFh,20h,0FFh,20h,0FFh,27h,0FFh,30h,0FFh ; bar 03
        db 2Bh,0FFh,2Bh,0FFh,36h,0FFh,3Bh,0FFh,2Bh,0FFh,2Bh,0FFh,36h,0FFh,3Bh,0FFh ; bar 04
        db 24h,0FFh,24h,0FFh,2Bh,0FFh,34h,0FFh,24h,0FFh,24h,0FFh,2Bh,0FFh,34h,0FFh ; bar 05
        db 22h,0FFh,22h,0FFh,29h,0FFh,32h,0FFh,22h,0FFh,22h,0FFh,29h,0FFh,32h,0FFh ; bar 06
        db 20h,0FFh,20h,0FFh,27h,0FFh,30h,0FFh,20h,0FFh,20h,0FFh,27h,0FFh,30h,0FFh ; bar 07
        db 2Bh,0FFh,2Bh,0FFh,36h,0FFh,3Bh,0FFh,2Bh,0FFh,2Bh,0FFh,36h,0FFh,24h,0FFh ; bar 08
        db 24h,0FFh,24h,0FFh,2Bh,0FFh,34h,0FFh,24h,0FFh,2Bh,0FFh,34h,0FFh,2Bh,0FFh ; bar 09
        db 22h,0FFh,22h,0FFh,29h,0FFh,32h,0FFh,22h,0FFh,29h,0FFh,32h,0FFh,29h,0FFh ; bar 10
        db 20h,0FFh,20h,0FFh,27h,0FFh,30h,0FFh,20h,0FFh,27h,0FFh,30h,0FFh,27h,0FFh ; bar 11
        db 2Bh,0FFh,2Bh,0FFh,36h,0FFh,3Bh,0FFh,2Bh,0FFh,36h,0FFh,3Bh,0FFh,36h,0FFh ; bar 12
        db 20h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,27h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 13
        db 22h,0FFh,22h,0FFh,32h,0FFh,32h,0FFh,22h,0FFh,32h,0FFh,32h,0FFh,32h,0FFh ; bar 14
        db 24h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,2Bh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 15
        db 2Bh,0FFh,2Bh,0FFh,36h,0FFh,3Bh,0FFh,2Bh,0FFh,36h,0FFh,3Bh,0FFh,36h,0FFh ; bar 16
        db 27h,0FFh,27h,0FFh,32h,0FFh,37h,0FFh,27h,0FFh,32h,0FFh,37h,0FFh,32h,0FFh ; bar 17
        db 22h,0FFh,22h,0FFh,29h,0FFh,32h,0FFh,22h,0FFh,29h,0FFh,32h,0FFh,29h,0FFh ; bar 18
        db 24h,0FFh,24h,0FFh,2Bh,0FFh,34h,0FFh,24h,0FFh,2Bh,0FFh,34h,0FFh,2Bh,0FFh ; bar 19
        db 20h,0FFh,20h,0FFh,27h,0FFh,30h,0FFh,20h,0FFh,27h,0FFh,30h,0FFh,27h,0FFh ; bar 20
        db 27h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,32h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 21
        db 22h,0FFh,22h,0FFh,32h,0FFh,32h,0FFh,22h,0FFh,32h,0FFh,32h,0FFh,32h,0FFh ; bar 22
        db 20h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,27h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 23
        db 2Bh,0FFh,2Bh,0FFh,36h,0FFh,3Bh,0FFh,2Bh,0FFh,36h,0FFh,3Bh,0FFh,36h,0FFh ; bar 24
        db 24h,0FFh,24h,0FFh,2Bh,0FFh,34h,0FFh,24h,0FFh,2Bh,0FFh,34h,0FFh,2Bh,0FFh ; bar 25
        db 22h,0FFh,22h,0FFh,29h,0FFh,32h,0FFh,22h,0FFh,29h,0FFh,32h,0FFh,29h,0FFh ; bar 26
        db 20h,0FFh,20h,0FFh,27h,0FFh,30h,0FFh,20h,0FFh,27h,0FFh,30h,0FFh,27h,0FFh ; bar 27
        db 2Bh,0FFh,2Bh,0FFh,36h,0FFh,3Bh,0FFh,2Bh,0FFh,36h,0FFh,3Bh,0FFh,36h,0FFh ; bar 28
        db 20h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,27h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 29
        db 22h,0FFh,22h,0FFh,29h,0FFh,32h,0FFh,22h,0FFh,29h,0FFh,32h,0FFh,29h,0FFh ; bar 30
        db 2Bh,0FFh,2Bh,0FFh,36h,0FFh,3Bh,0FFh,2Bh,0FFh,36h,0FFh,3Bh,0FFh,36h,0FFh ; bar 31
        db 24h,0FFh,24h,0FFh,2Bh,0FFh,34h,0FFh,24h,0FFh,2Bh,0FFh,34h,0FFh,2Bh,0FFh ; bar 32

; Channel 3: OPNA/OPL3 pad root - one sustained chord tone per bar
        db 34h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 01
        db 32h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 02
        db 30h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 03
        db 3Bh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 04
        db 34h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 05
        db 32h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 06
        db 30h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 07
        db 3Bh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 08
        db 34h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 09
        db 32h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 10
        db 30h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 11
        db 3Bh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 12
        db 30h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 13
        db 32h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 14
        db 34h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 15
        db 3Bh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 16
        db 37h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 17
        db 32h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 18
        db 34h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 19
        db 30h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 20
        db 37h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 21
        db 32h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 22
        db 30h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 23
        db 3Bh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 24
        db 34h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 25
        db 32h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 26
        db 30h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 27
        db 3Bh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 28
        db 30h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 29
        db 32h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 30
        db 3Bh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 31
        db 34h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 32

; Channel 4: OPNA/OPL3 pad fifth - one sustained chord tone per bar
        db 3Bh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 01
        db 39h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 02
        db 37h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 03
        db 46h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 04
        db 3Bh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 05
        db 39h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 06
        db 37h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 07
        db 46h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 08
        db 3Bh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 09
        db 39h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 10
        db 37h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 11
        db 46h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 12
        db 37h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 13
        db 39h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 14
        db 3Bh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 15
        db 46h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 16
        db 42h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 17
        db 39h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 18
        db 3Bh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 19
        db 37h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 20
        db 42h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 21
        db 39h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 22
        db 37h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 23
        db 46h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 24
        db 3Bh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 25
        db 39h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 26
        db 37h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 27
        db 46h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 28
        db 37h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 29
        db 39h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 30
        db 46h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 31
        db 3Bh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 32

; Channel 5: OPNA/OPL3 high chord colour - sustained third, removed in drops
        db 47h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 01
        db 46h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 02
        db 44h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 03
        db 52h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 04
        db 47h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 05
        db 46h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 06
        db 44h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 07
        db 52h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 08
        db 47h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 09
        db 46h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 10
        db 44h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 11
        db 52h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 12
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 13
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 14
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 15
        db 52h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 16
        db 4Bh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 17
        db 46h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 18
        db 47h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 19
        db 44h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 20
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 21
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 22
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 23
        db 52h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 24
        db 47h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 25
        db 46h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 26
        db 44h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 27
        db 52h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 28
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 29
        db 46h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 30
        db 52h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 31
        db 47h,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FEh,0FFh,0FFh ; bar 32

; Channel 6: SSG-A / OPL3 electric-harp - straight chord-tone arpeggio engine
        db 44h,47h,4Bh,44h,47h,4Bh,44h,47h,4Bh,44h,47h,4Bh,44h,47h,4Bh,44h ; bar 01
        db 42h,46h,49h,42h,46h,49h,42h,46h,49h,42h,46h,49h,42h,46h,49h,42h ; bar 02
        db 40h,44h,47h,40h,44h,47h,40h,44h,47h,40h,44h,47h,40h,44h,47h,40h ; bar 03
        db 4Bh,52h,56h,4Bh,52h,56h,4Bh,52h,56h,4Bh,52h,56h,4Bh,52h,56h,4Bh ; bar 04
        db 44h,47h,4Bh,44h,47h,4Bh,44h,47h,4Bh,44h,47h,4Bh,44h,47h,4Bh,44h ; bar 05
        db 42h,46h,49h,42h,46h,49h,42h,46h,49h,42h,46h,49h,42h,46h,49h,42h ; bar 06
        db 40h,44h,47h,40h,44h,47h,40h,44h,47h,40h,44h,47h,40h,44h,47h,40h ; bar 07
        db 4Bh,52h,56h,4Bh,52h,56h,4Bh,52h,56h,4Bh,52h,56h,4Bh,52h,56h,4Bh ; bar 08
        db 44h,47h,4Bh,44h,47h,4Bh,44h,47h,4Bh,44h,47h,4Bh,44h,47h,4Bh,44h ; bar 09
        db 42h,46h,49h,42h,46h,49h,42h,46h,49h,42h,46h,49h,42h,46h,49h,42h ; bar 10
        db 44h,47h,40h,44h,47h,40h,44h,47h,40h,44h,47h,40h,44h,47h,40h,44h ; bar 11
        db 52h,56h,4Bh,52h,56h,4Bh,52h,56h,4Bh,52h,56h,4Bh,52h,56h,4Bh,52h ; bar 12
        db 40h,0FFh,44h,0FFh,47h,0FFh,40h,0FFh,44h,0FFh,47h,0FFh,40h,0FFh,44h,0FFh ; bar 13
        db 42h,0FFh,46h,0FFh,49h,0FFh,42h,0FFh,46h,0FFh,49h,0FFh,42h,0FFh,46h,0FFh ; bar 14
        db 44h,0FFh,47h,0FFh,4Bh,0FFh,44h,0FFh,47h,0FFh,4Bh,0FFh,44h,0FFh,47h,0FFh ; bar 15
        db 4Bh,52h,56h,4Bh,52h,56h,4Bh,52h,56h,4Bh,52h,56h,4Bh,52h,56h,4Bh ; bar 16
        db 47h,4Bh,52h,47h,4Bh,52h,47h,4Bh,52h,47h,4Bh,52h,47h,4Bh,52h,47h ; bar 17
        db 42h,46h,49h,42h,46h,49h,42h,46h,49h,42h,46h,49h,42h,46h,49h,42h ; bar 18
        db 44h,47h,4Bh,44h,47h,4Bh,44h,47h,4Bh,44h,47h,4Bh,44h,47h,4Bh,44h ; bar 19
        db 40h,44h,47h,40h,44h,47h,40h,44h,47h,40h,44h,47h,40h,44h,47h,40h ; bar 20
        db 47h,0FFh,4Bh,0FFh,52h,0FFh,47h,0FFh,4Bh,0FFh,52h,0FFh,47h,0FFh,4Bh,0FFh ; bar 21
        db 42h,0FFh,46h,0FFh,49h,0FFh,42h,0FFh,46h,0FFh,49h,0FFh,42h,0FFh,46h,0FFh ; bar 22
        db 40h,0FFh,44h,0FFh,47h,0FFh,40h,0FFh,44h,0FFh,47h,0FFh,40h,0FFh,44h,0FFh ; bar 23
        db 4Bh,52h,56h,4Bh,52h,56h,4Bh,52h,56h,4Bh,52h,56h,4Bh,52h,56h,4Bh ; bar 24
        db 44h,47h,4Bh,44h,47h,4Bh,44h,47h,4Bh,44h,47h,4Bh,44h,47h,4Bh,44h ; bar 25
        db 42h,46h,49h,42h,46h,49h,42h,46h,49h,42h,46h,49h,42h,46h,49h,42h ; bar 26
        db 40h,44h,47h,40h,44h,47h,40h,44h,47h,40h,44h,47h,40h,44h,47h,40h ; bar 27
        db 4Bh,52h,56h,4Bh,52h,56h,4Bh,52h,56h,4Bh,52h,56h,4Bh,52h,56h,4Bh ; bar 28
        db 40h,0FFh,44h,0FFh,47h,0FFh,40h,0FFh,44h,0FFh,47h,0FFh,40h,0FFh,44h,0FFh ; bar 29
        db 42h,46h,49h,42h,46h,49h,42h,46h,49h,42h,46h,49h,42h,46h,49h,42h ; bar 30
        db 4Bh,52h,56h,4Bh,52h,56h,4Bh,52h,56h,4Bh,52h,56h,4Bh,52h,56h,4Bh ; bar 31
        db 44h,47h,4Bh,44h,47h,4Bh,44h,47h,4Bh,44h,47h,4Bh,44h,47h,4Bh,44h ; bar 32

; Channel 7: SSG-B / OPL3 glass-organ - fixed off-beat answer
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,4Bh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,47h,0FFh ; bar 01
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,49h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,46h,0FFh ; bar 02
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,47h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,44h,0FFh ; bar 03
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,56h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,52h,0FFh ; bar 04
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,4Bh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,47h,0FFh ; bar 05
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,49h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,46h,0FFh ; bar 06
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,47h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,44h,0FFh ; bar 07
        db 0FFh,0FFh,0FFh,0FFh,52h,0FFh,56h,0FFh,0FFh,0FFh,0FFh,0FFh,5Bh,0FFh,56h,0FFh ; bar 08
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,4Bh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,47h,0FFh ; bar 09
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,49h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,46h,0FFh ; bar 10
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,47h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,44h,0FFh ; bar 11
        db 0FFh,0FFh,0FFh,0FFh,52h,0FFh,56h,0FFh,0FFh,0FFh,0FFh,0FFh,5Bh,0FFh,56h,0FFh ; bar 12
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 13
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,49h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,46h,0FFh ; bar 14
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 15
        db 0FFh,0FFh,0FFh,0FFh,52h,0FFh,56h,0FFh,0FFh,0FFh,0FFh,0FFh,5Bh,0FFh,56h,0FFh ; bar 16
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,52h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,4Bh,0FFh ; bar 17
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,49h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,46h,0FFh ; bar 18
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,4Bh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,47h,0FFh ; bar 19
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,47h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,44h,0FFh ; bar 20
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 21
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,49h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,46h,0FFh ; bar 22
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 23
        db 0FFh,0FFh,0FFh,0FFh,52h,0FFh,56h,0FFh,0FFh,0FFh,0FFh,0FFh,5Bh,0FFh,56h,0FFh ; bar 24
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,4Bh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,47h,0FFh ; bar 25
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,49h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,46h,0FFh ; bar 26
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,47h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,44h,0FFh ; bar 27
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,56h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,52h,0FFh ; bar 28
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 29
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,49h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,46h,0FFh ; bar 30
        db 0FFh,0FFh,0FFh,0FFh,52h,0FFh,56h,0FFh,0FFh,0FFh,0FFh,0FFh,5Bh,0FFh,56h,0FFh ; bar 31
        db 0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 32

; Channel 8: SSG-C / OPL3 muted-marimba - quarter-note tonal pulse
        db 34h,0FFh,0FFh,0FFh,34h,0FFh,0FFh,0FFh,34h,0FFh,0FFh,0FFh,34h,0FFh,0FFh,0FFh ; bar 01
        db 32h,0FFh,0FFh,0FFh,32h,0FFh,0FFh,0FFh,32h,0FFh,0FFh,0FFh,32h,0FFh,0FFh,0FFh ; bar 02
        db 30h,0FFh,0FFh,0FFh,30h,0FFh,0FFh,0FFh,30h,0FFh,0FFh,0FFh,30h,0FFh,0FFh,0FFh ; bar 03
        db 3Bh,0FFh,0FFh,0FFh,3Bh,0FFh,0FFh,0FFh,3Bh,0FFh,0FFh,0FFh,3Bh,0FFh,0FFh,0FFh ; bar 04
        db 34h,0FFh,0FFh,0FFh,34h,0FFh,0FFh,0FFh,34h,0FFh,0FFh,0FFh,34h,0FFh,0FFh,0FFh ; bar 05
        db 32h,0FFh,0FFh,0FFh,32h,0FFh,0FFh,0FFh,32h,0FFh,0FFh,0FFh,32h,0FFh,0FFh,0FFh ; bar 06
        db 30h,0FFh,0FFh,0FFh,30h,0FFh,0FFh,0FFh,30h,0FFh,0FFh,0FFh,30h,0FFh,0FFh,0FFh ; bar 07
        db 3Bh,0FFh,0FFh,0FFh,3Bh,0FFh,0FFh,0FFh,3Bh,0FFh,0FFh,0FFh,3Bh,0FFh,0FFh,0FFh ; bar 08
        db 34h,0FFh,34h,0FFh,34h,0FFh,34h,0FFh,34h,0FFh,34h,0FFh,34h,0FFh,34h,0FFh ; bar 09
        db 32h,0FFh,32h,0FFh,32h,0FFh,32h,0FFh,32h,0FFh,32h,0FFh,32h,0FFh,32h,0FFh ; bar 10
        db 30h,0FFh,30h,0FFh,30h,0FFh,30h,0FFh,30h,0FFh,30h,0FFh,30h,0FFh,30h,0FFh ; bar 11
        db 3Bh,0FFh,3Bh,0FFh,3Bh,0FFh,3Bh,0FFh,3Bh,0FFh,3Bh,0FFh,3Bh,0FFh,3Bh,0FFh ; bar 12
        db 30h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,30h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 13
        db 32h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,32h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 14
        db 34h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 15
        db 3Bh,0FFh,3Bh,0FFh,3Bh,0FFh,3Bh,0FFh,3Bh,0FFh,3Bh,0FFh,3Bh,0FFh,3Bh,0FFh ; bar 16
        db 37h,0FFh,37h,0FFh,37h,0FFh,37h,0FFh,37h,0FFh,37h,0FFh,37h,0FFh,37h,0FFh ; bar 17
        db 32h,0FFh,32h,0FFh,32h,0FFh,32h,0FFh,32h,0FFh,32h,0FFh,32h,0FFh,32h,0FFh ; bar 18
        db 34h,0FFh,34h,0FFh,34h,0FFh,34h,0FFh,34h,0FFh,34h,0FFh,34h,0FFh,34h,0FFh ; bar 19
        db 30h,0FFh,30h,0FFh,30h,0FFh,30h,0FFh,30h,0FFh,30h,0FFh,30h,0FFh,30h,0FFh ; bar 20
        db 37h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,37h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 21
        db 32h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,32h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 22
        db 30h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 23
        db 3Bh,0FFh,3Bh,0FFh,3Bh,0FFh,3Bh,0FFh,3Bh,0FFh,3Bh,0FFh,3Bh,0FFh,3Bh,0FFh ; bar 24
        db 34h,0FFh,34h,0FFh,34h,0FFh,34h,0FFh,34h,0FFh,34h,0FFh,34h,0FFh,34h,0FFh ; bar 25
        db 32h,0FFh,32h,0FFh,32h,0FFh,32h,0FFh,32h,0FFh,32h,0FFh,32h,0FFh,32h,0FFh ; bar 26
        db 30h,0FFh,30h,0FFh,30h,0FFh,30h,0FFh,30h,0FFh,30h,0FFh,30h,0FFh,30h,0FFh ; bar 27
        db 3Bh,0FFh,3Bh,0FFh,3Bh,0FFh,3Bh,0FFh,3Bh,0FFh,3Bh,0FFh,3Bh,0FFh,3Bh,0FFh ; bar 28
        db 30h,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh ; bar 29
        db 32h,0FFh,32h,0FFh,32h,0FFh,32h,0FFh,32h,0FFh,32h,0FFh,32h,0FFh,32h,0FFh ; bar 30
        db 3Bh,0FFh,3Bh,0FFh,3Bh,0FFh,3Bh,0FFh,3Bh,0FFh,3Bh,0FFh,3Bh,0FFh,3Bh,0FFh ; bar 31
        db 34h,0FFh,34h,0FFh,34h,0FFh,34h,0FFh,34h,0FFh,34h,0FFh,34h,0FFh,34h,0FFh ; bar 32

; Direct rhythm masks: hook-driven OPNA/OPL3 percussion arrangement
; bits: 01=BD 02=SD 04=CYM 08=HH(unused) 10=TOM 20=RIM
rhythm_steps:
        db 05h,00h,00h,00h,02h,00h,00h,00h,01h,00h,00h,00h,02h,00h,01h,00h ; bar 01
        db 01h,00h,00h,00h,02h,00h,01h,00h,01h,00h,00h,00h,02h,00h,20h,00h ; bar 02
        db 01h,00h,00h,00h,02h,00h,00h,00h,01h,00h,00h,00h,02h,00h,00h,00h ; bar 03
        db 01h,00h,00h,00h,02h,00h,01h,00h,01h,00h,00h,00h,02h,00h,20h,00h ; bar 04
        db 01h,00h,00h,00h,02h,00h,00h,00h,01h,00h,00h,00h,02h,00h,00h,00h ; bar 05
        db 01h,00h,00h,00h,02h,00h,01h,00h,01h,00h,00h,00h,02h,00h,20h,00h ; bar 06
        db 01h,00h,00h,00h,02h,00h,00h,00h,01h,00h,00h,00h,02h,00h,00h,00h ; bar 07
        db 01h,00h,00h,00h,02h,00h,00h,00h,01h,00h,10h,00h,12h,00h,30h,00h ; bar 08
        db 01h,00h,00h,00h,02h,00h,01h,00h,01h,00h,20h,00h,02h,00h,01h,00h ; bar 09
        db 01h,00h,00h,00h,02h,00h,01h,00h,01h,00h,20h,00h,02h,00h,21h,00h ; bar 10
        db 01h,00h,00h,00h,02h,00h,01h,00h,01h,00h,20h,00h,02h,00h,01h,00h ; bar 11
        db 01h,00h,00h,00h,02h,00h,01h,00h,01h,00h,30h,00h,12h,00h,31h,00h ; bar 12
        db 05h,00h,00h,00h,00h,00h,00h,00h,02h,00h,00h,00h,00h,00h,00h,00h ; bar 13
        db 01h,00h,00h,00h,00h,00h,00h,00h,02h,00h,00h,00h,00h,00h,20h,00h ; bar 14
        db 01h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,10h,00h,20h,00h ; bar 15
        db 01h,00h,00h,00h,02h,00h,01h,00h,01h,00h,30h,00h,12h,00h,31h,00h ; bar 16
        db 05h,00h,00h,00h,02h,00h,01h,00h,01h,00h,20h,00h,02h,00h,01h,00h ; bar 17
        db 01h,00h,00h,00h,02h,00h,01h,00h,01h,00h,20h,00h,02h,00h,21h,00h ; bar 18
        db 01h,00h,00h,00h,02h,00h,01h,00h,01h,00h,20h,00h,02h,00h,01h,00h ; bar 19
        db 01h,00h,00h,00h,02h,00h,01h,00h,01h,00h,20h,00h,02h,00h,21h,00h ; bar 20
        db 05h,00h,00h,00h,00h,00h,00h,00h,02h,00h,00h,00h,00h,00h,00h,00h ; bar 21
        db 01h,00h,00h,00h,00h,00h,00h,00h,02h,00h,00h,00h,00h,00h,20h,00h ; bar 22
        db 01h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,00h,10h,00h,20h,00h ; bar 23
        db 01h,00h,00h,00h,02h,00h,01h,00h,01h,00h,30h,00h,12h,00h,31h,00h ; bar 24
        db 05h,00h,00h,00h,02h,00h,01h,00h,01h,00h,20h,00h,02h,00h,01h,00h ; bar 25
        db 01h,00h,00h,00h,02h,00h,01h,00h,01h,00h,20h,00h,02h,00h,21h,00h ; bar 26
        db 01h,00h,00h,00h,02h,00h,01h,00h,01h,00h,20h,00h,02h,00h,01h,00h ; bar 27
        db 01h,00h,00h,00h,02h,00h,01h,00h,01h,00h,20h,00h,02h,00h,21h,00h ; bar 28
        db 05h,00h,00h,00h,00h,00h,00h,00h,02h,00h,00h,00h,00h,00h,00h,00h ; bar 29
        db 05h,00h,00h,00h,02h,00h,01h,00h,01h,00h,20h,00h,02h,00h,01h,00h ; bar 30
        db 01h,00h,00h,00h,02h,00h,01h,00h,01h,00h,30h,00h,12h,00h,31h,00h ; bar 31
        db 05h,00h,00h,00h,02h,00h,01h,00h,01h,00h,20h,00h,02h,00h,01h,00h ; bar 32
