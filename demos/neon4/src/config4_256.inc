; Copyright (c) 2026 Nakata Maho
; Ported By Maho Nakata.
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the BSD 2-Clause conditions apply.

; ---------------------------------------------------------------------------
; CONFIG4_256.INC - compile-time constants for NEON 4 GEOMETRIC SOLID
; ---------------------------------------------------------------------------

%define SCREEN_W          640
%define SCREEN_H          400
%define BYTES_PER_LINE     80
%define VRAM_B_SEG      0A800h
%define VRAM_R_SEG      0B000h
%define VRAM_G_SEG      0B800h
%define TEXT_CHAR_SEG   0A000h
%define TEXT_ATTR_SEG   0A200h

; Derive the animation loop directly from the score timing.  One score step
; spans MUSIC_DIVIDER logical frames and one bar contains 16 score steps.
; Keeping these values related prevents the visual loop and BGM loop from
; drifting apart when the score length is changed later.
%define SCENE_COUNT           8
%define MUSIC_STEPS        512
%define MUSIC_CHANNELS       9
%define MUSIC_BAR_COUNT      32
%define OPL_SCORE_CHANNELS    9
%define OPL_PERCUSSION_CHANNELS 6
%define OPL_PATCH_CHANNELS   15
%define MUSIC_DIVIDER        6
%define MUSIC_STEPS_PER_BAR  16
%define MUSIC_BAR_FRAMES     (MUSIC_STEPS_PER_BAR * MUSIC_DIVIDER)
%define TOTAL_FRAMES         (MUSIC_STEPS * MUSIC_DIVIDER)

%if TOTAL_FRAMES != 3072
    %error NEON4 compact score timing must remain 3072 logical frames
%endif
; NEON4 0.21 re-aligns the visual flow to the 0.20 song phrasing.  The 32-bar
; song is divided into eight 4-bar chapters so each visual idea lands on a
; musical phrase boundary: seed -> facet -> material -> morph -> transfer ->
; surface -> grid -> finale.
%define N4_SCENE0_FRAMES      384    ;  4 bars - SIGNAL SEED
%define N4_SCENE1_FRAMES      384    ;  4 bars - FACET ASSEMBLY
%define N4_SCENE2_FRAMES      384    ;  4 bars - MATERIAL ASSEMBLY
%define N4_SCENE3_FRAMES      384    ;  4 bars - MORPH GATE
%define N4_SCENE4_FRAMES      384    ;  4 bars - RASTER TRANSFER
%define N4_SCENE5_FRAMES      384    ;  4 bars - SURFACE WAVE
%define N4_SCENE6_FRAMES      384    ;  4 bars - GRID ARRIVAL
%define N4_SCENE7_FRAMES      384    ;  4 bars - SOLID FINALE
%define N4_FINAL_SCENE_FRAMES N4_SCENE7_FRAMES
%define N4_FINAL_FADE_START    320    ; last 64 frames of 4-bar finale
%define N4_FINALE_BUILD_START   96    ; first bar assembles, remaining bars climax
%define N4_FINALE_CARRIER_END  224
%define N4_MORPH_RASTER_START   96
%define N4_MORPH_RASTER_END    288
%define N4_TUNNEL_RING_STEP  512
%define N4_TUNNEL_RING_COUNT 5

%define OPL_CANDIDATE_COUNT  15
%define OPNA_CANDIDATE_COUNT   4
%define OPN26_CANDIDATE_COUNT   2
%define OPN_DEVICE_NONE          0
%define OPN_DEVICE_OPNA          1
%define OPN_DEVICE_26K           2
%define SOUND_MODE_AUTO         0
%define SOUND_MODE_OPNA         1
%define SOUND_MODE_OPL          2
%define SOUND_MODE_BOTH         3
%define SOUND_MODE_26K          4
%define SOUND_MODE_NONE         5
%define CMD_TOKEN_MAX          31
%define FRAME_SKIP_MAX          10
%define ADAPTIVE_MAX_ELAPSED_FRAMES 12
%define REST              0FFh
%define HOLD              0FEh
%define NOTE(block, semi) (((block) << 4) | (semi))

; 3-D renderer constants. Coordinates are integer world units; rotation uses
; Q2.14 sine/cosine values and perspective uses integer 32-bit division.
%define CITY_NEAR_Z          48
%define CITY_FAR_Z        14000
; Line clipping uses a small inward guard.  Interpolated world coordinates are
; rounded back to integer units; clipping exactly on the projection planes can
; otherwise round a point outside and make the complete line disappear.
%define CITY_NEAR_CLIP_Z   (CITY_NEAR_Z + 4)
%define CITY_FAR_CLIP_Z    (CITY_FAR_Z - 4)
%define CITY_FOCAL          300
%define CITY_SCREEN_CX      320
%define CITY_SCREEN_CY      200
%define CITY_CAMERA_EASE_SHIFT 4
%define CITY_PROJECT_LIMIT 30000
%define CITY_GROUND_SEGMENT_Z 1024
%define CITY_PALETTE_BLEND_FRAMES 128
; A small 2-D affine rotozoom panel enters from outside the right edge near the
; finale, then keeps travelling diagonally up-left while zooming in continuously.
; In the last part the transform locks onto one already-filled 24x24 source tile;
; that single tile keeps enlarging until its four edges are outside the viewport.
; Only after the tile covers the screen does the palette fade the image to black.
%define CITY_FINAL_ROTOZOOM_START       640
%define CITY_FINAL_ROTOZOOM_FOCUS_START 800
%define CITY_FINAL_ROTOZOOM_HALF        72
%define CITY_FINAL_ROTOZOOM_STEP        24
%define CITY_FINAL_ROTOZOOM_INSET       2
%define CITY_FINAL_ROTOZOOM_FOCUS_X     12
%define CITY_FINAL_ROTOZOOM_FOCUS_Y     12
%define CITY_FINAL_PALETTE_FADE_START   N4_FINAL_FADE_START
; Once both graphics pages have received the fully covering tile, keep their
; pixels and update only the palette/text.  This removes all final-scene 3-D
; and rotozoom geometry work during the black fade.
%define CITY_FINAL_ROTOZOOM_SINGLE_TILE_START 840 ; keep the panel until the focused tile visually dominates
%define CITY_FINAL_COVER_REUSE_START     849 ; focused filled tile safely covers 640x400 from here
%define CITY_ROUTE_CRUISE_SPEED 20
; Top-level scene-route tuning constants.
; These are NASM preprocessor constants: they do not add runtime loads or
; branches.  Values below are the exact literals previously embedded in the
; scene routines unless explicitly noted as a new tuning value.
%define CITY_ROUTE_CENTER_X                 900
%define CITY_APPROACH_CAMERA_Z              -50
%define CITY_APPROACH_CAMERA_Y             1800
%define CITY_APPROACH_CAMERA_YAW            -24
%define CITY_APPROACH_CAMERA_PITCH          249
%define CITY_CANYON_CAMERA_Z                -50
%define CITY_CANYON_CAMERA_Y                145
%define CITY_CANYON_CAMERA_PITCH            254
%define CITY_BRIDGE_CAMERA_Y                155
%define CITY_BRIDGE_CAMERA_PITCH            253
%define CITY_PARK_ORBIT_CENTER_X           1250
%define CITY_PARK_ORBIT_CENTER_Z           7600
%define CITY_PARK_ORBIT_Y                   640
%define CITY_NEON_CAMERA_Z0                8200
%define CITY_NEON_CRUISE_Y_HIGH             350
%define CITY_NEON_CRUISE_Y_NORMAL           115
%define CITY_NEON_TUNNEL_DESCENT_START      448
%define CITY_NEON_TUNNEL_DESCENT_FRAMES     191
%define CITY_TUNNEL_CAMERA_Z0             15032
%define CITY_RAIL_CAMERA_Z0               17472
%define CITY_RAIL_CAMERA_Y                  135
%define CITY_RAIL_CAMERA_PITCH              254
%define CITY_TERMINAL_CAMERA_Z0           27600
%define CITY_TERMINAL_CAMERA_Y0             190
%define CITY_TERMINAL_CAMERA_PITCH          252
%define CITY_FINAL_CAMERA_Z_BASE          29159
%define CITY_FINAL_CAMERA_Y                2821
%define CITY_FINAL_CAMERA_PITCH             244

; Commercial-district billboard avoidance.  Entry timing and dodge peaks stay
; unchanged, but recovery is deliberately much slower.  A nominal recovery is
; four times the 28-frame entry.  If the next horizontal dodge starts sooner,
; that start wins and the previous recovery uses the whole available interval
; so it reaches the base route just as the next manoeuvre begins.  The third
; horizontal recovery can continue while the final vertical drop is active.
%define CITY_NEON_DODGE_ENTRY_FRAMES         28
%define CITY_NEON_DODGE_RETURN_FRAMES       (CITY_NEON_DODGE_ENTRY_FRAMES*4)
%define CITY_NEON_DODGE1_START               49
%define CITY_NEON_DODGE1_CENTER              77
%define CITY_NEON_DODGE2_START              127
%define CITY_NEON_DODGE2_CENTER             155
%define CITY_NEON_DODGE3_START              257
%define CITY_NEON_DODGE3_CENTER             285
%define CITY_NEON_DODGE4_START              335
%define CITY_NEON_DODGE4_END                363
%define CITY_NEON_DODGE1_RETURN_FRAMES      (CITY_NEON_DODGE2_START-CITY_NEON_DODGE1_CENTER)
%define CITY_NEON_DODGE2_RETURN_FRAMES      (CITY_NEON_DODGE3_START-CITY_NEON_DODGE2_CENTER)
%define CITY_NEON_DODGE3_RETURN_FRAMES       CITY_NEON_DODGE_RETURN_FRAMES
%define CITY_NEON_DODGE1_END                (CITY_NEON_DODGE1_CENTER+CITY_NEON_DODGE1_RETURN_FRAMES)
%define CITY_NEON_DODGE2_END                (CITY_NEON_DODGE2_CENTER+CITY_NEON_DODGE2_RETURN_FRAMES)
%define CITY_NEON_DODGE3_END                (CITY_NEON_DODGE3_CENTER+CITY_NEON_DODGE3_RETURN_FRAMES)
%define CITY_NEON_DODGE1_OVERSHOOT_LATERAL   240 ; stronger first RIGHT dodge peak
%define CITY_NEON_DODGE_LATERAL              180
%define CITY_NEON_DODGE2_LATERAL              90 ; second LEFT dodge: half lateral distance
%define CITY_NEON_DODGE_TILT_PHASE           512
%define CITY_NEON_DODGE_FINAL_DROP (CITY_NEON_CRUISE_Y_HIGH-CITY_NEON_CRUISE_Y_NORMAL)

; Terminal complex.  The station-side building is a composition of orthogonal
; boxes inspired by a stepped modern high-rise: broad podium, two side wings,
; a central tower, a setback upper stage and a small crown.  All dimensions are
; kept here so the silhouette can be tuned without hunting geometry literals.
%define CITY_TERMINAL_BUILDING_Z            29600
%define CITY_TERMINAL_PODIUM_X                100
%define CITY_TERMINAL_PODIUM_Y                  0
%define CITY_TERMINAL_PODIUM_W               1740
%define CITY_TERMINAL_PODIUM_H                260
%define CITY_TERMINAL_PODIUM_D                720
%define CITY_TERMINAL_LEFT_X                  140
%define CITY_TERMINAL_LEFT_Y                  260
%define CITY_TERMINAL_LEFT_Z                29640
%define CITY_TERMINAL_LEFT_W                  430
%define CITY_TERMINAL_LEFT_H                  390
%define CITY_TERMINAL_LEFT_D                  640
%define CITY_TERMINAL_CENTER_X                600
%define CITY_TERMINAL_CENTER_Y                260
%define CITY_TERMINAL_CENTER_Z              29620
%define CITY_TERMINAL_CENTER_W                650
%define CITY_TERMINAL_CENTER_H                650
%define CITY_TERMINAL_CENTER_D                680
%define CITY_TERMINAL_UPPER_X                 710
%define CITY_TERMINAL_UPPER_Y                 910
%define CITY_TERMINAL_UPPER_Z               29680
%define CITY_TERMINAL_UPPER_W                 430
%define CITY_TERMINAL_UPPER_H                 500
%define CITY_TERMINAL_UPPER_D                 560
%define CITY_TERMINAL_CROWN_X                 790
%define CITY_TERMINAL_CROWN_Y                1410
%define CITY_TERMINAL_CROWN_Z               29740
%define CITY_TERMINAL_CROWN_W                 270
%define CITY_TERMINAL_CROWN_H                 180
%define CITY_TERMINAL_CROWN_D                 440
%define CITY_TERMINAL_RIGHT_X                1280
%define CITY_TERMINAL_RIGHT_Y                 260
%define CITY_TERMINAL_RIGHT_Z               29640
%define CITY_TERMINAL_RIGHT_W                 500
%define CITY_TERMINAL_RIGHT_H                 430
%define CITY_TERMINAL_RIGHT_D                 640
%define CITY_TERMINAL_ROAD_Z0               28600
%define CITY_TERMINAL_ROAD_Z1               31600
%define CITY_TERMINAL_ROAD_HALF              1200
%define CITY_TERMINAL_ROAD_SPACING            256
; Right-rear landmark beside the terminal.
; Top-view layout (camera approaches from lower Z toward higher Z):
;
;   [ LOW front ]                 ; narrow stem behind SMALL
;   [ LOW rear spanning SMALL + MID + TALL ]
;   [ SMALL ] [ MID ] [ TALL ]
;
; SMALL/MID/TALL are arranged along the thin X/W axis and touch on side
; faces.  LOW starts behind SMALL and broadens only at the rear.  All masses
; stand on the same Y=0 base.
%define CITY_TERMINAL_LANDMARK_TALL_X         2220
%define CITY_TERMINAL_LANDMARK_TALL_Y            0
%define CITY_TERMINAL_LANDMARK_TALL_W          280 ; ~2/3 of v5 420, thin X axis
%define CITY_TERMINAL_LANDMARK_TALL_H         2170 ; ~1.2x v5 1810
%define CITY_TERMINAL_LANDMARK_TALL_D          680 ; retain original long Z depth

; Middle tower: directly LEFT of TALL, same thin-axis width.
%define CITY_TERMINAL_LANDMARK_MID_W  CITY_TERMINAL_LANDMARK_TALL_W
%define CITY_TERMINAL_LANDMARK_MID_X  (CITY_TERMINAL_LANDMARK_TALL_X - CITY_TERMINAL_LANDMARK_MID_W)
%define CITY_TERMINAL_LANDMARK_MID_Y             0
%define CITY_TERMINAL_LANDMARK_MID_D           700
%define CITY_TERMINAL_LANDMARK_MID_Z          30620
%define CITY_TERMINAL_LANDMARK_MID_H          1180

; TALL and MID share the same long-axis centre, so their X side faces read
; as one stepped slab without a front/back offset.
%define CITY_TERMINAL_LANDMARK_TALL_Z (CITY_TERMINAL_LANDMARK_MID_Z + ((CITY_TERMINAL_LANDMARK_MID_D - CITY_TERMINAL_LANDMARK_TALL_D) / 2))

; Small building: attached to the LEFT side of MID, opposite the TALL face.
; Its front Z line is aligned with MID's front Z line.
%define CITY_TERMINAL_LANDMARK_SMALL_W          140
%define CITY_TERMINAL_LANDMARK_SMALL_X (CITY_TERMINAL_LANDMARK_MID_X - CITY_TERMINAL_LANDMARK_SMALL_W)
%define CITY_TERMINAL_LANDMARK_SMALL_Y            0
%define CITY_TERMINAL_LANDMARK_SMALL_D          180
%define CITY_TERMINAL_LANDMARK_SMALL_Z CITY_TERMINAL_LANDMARK_MID_Z
%define CITY_TERMINAL_LANDMARK_SMALL_H          516

; Low-rise wing: two boxes form the requested stepped top-view footprint.
; FRONT begins directly behind SMALL.  Its left edge is exactly SMALL's left
; edge and it keeps SMALL's width.  REAR begins only after MID/TALL have ended
; in +Z, then broadens to cover the complete SMALL+MID+TALL X span.  The total
; +Z depth remains 896 (= 640 * 1.4) without intersecting MID or TALL.
%define CITY_TERMINAL_LANDMARK_LOW_REAR_H         260
%define CITY_TERMINAL_LANDMARK_LOW_FRONT_H         345  ; fixed: keep prior height when SMALL_H changes
%define CITY_TERMINAL_LANDMARK_LOW_TOTAL_D        896 ; 640 * 1.4
%define CITY_TERMINAL_LANDMARK_LOW_FRONT_X CITY_TERMINAL_LANDMARK_SMALL_X
%define CITY_TERMINAL_LANDMARK_LOW_FRONT_Y          0
%define CITY_TERMINAL_LANDMARK_LOW_FRONT_Z (CITY_TERMINAL_LANDMARK_SMALL_Z + CITY_TERMINAL_LANDMARK_SMALL_D)
%define CITY_TERMINAL_LANDMARK_LOW_FRONT_W CITY_TERMINAL_LANDMARK_SMALL_W
%define CITY_TERMINAL_LANDMARK_LOW_REAR_Z (CITY_TERMINAL_LANDMARK_MID_Z + CITY_TERMINAL_LANDMARK_MID_D)
%define CITY_TERMINAL_LANDMARK_LOW_FRONT_D (CITY_TERMINAL_LANDMARK_LOW_REAR_Z - CITY_TERMINAL_LANDMARK_LOW_FRONT_Z)
%define CITY_TERMINAL_LANDMARK_LOW_REAR_X CITY_TERMINAL_LANDMARK_SMALL_X
%define CITY_TERMINAL_LANDMARK_LOW_REAR_Y           0
%define CITY_TERMINAL_LANDMARK_LOW_REAR_W ((CITY_TERMINAL_LANDMARK_TALL_X + CITY_TERMINAL_LANDMARK_TALL_W) - CITY_TERMINAL_LANDMARK_SMALL_X)
%define CITY_TERMINAL_LANDMARK_LOW_REAR_D (CITY_TERMINAL_LANDMARK_LOW_TOTAL_D - CITY_TERMINAL_LANDMARK_LOW_FRONT_D)
; Low-saturation cool-blue logical palette slots used by the terminal scene.
; Keep every component of this landmark in the same subdued blue family.
%define CITY_TERMINAL_LANDMARK_COLOR_LOW       2
%define CITY_TERMINAL_LANDMARK_COLOR_SMALL     2
%define CITY_TERMINAL_LANDMARK_COLOR_MID       3
%define CITY_TERMINAL_LANDMARK_COLOR_TALL      3
%define CITY_APPROACH_CITY_END_Z       3300   ; angled foreground city stops before canyon world begins
; Tunnel -> railway exposure transition.  The exit flashes between
; bright gray and white while distant, then the last 32 tunnel frames wash
; the background + seven city slots to white.  Railway restores in 10 frames.
%define CITY_TUNNEL_WHITEOUT_START       608
%define CITY_RAIL_WHITEOUT_RECOVER_FRAMES 10
%define CITY_TUNNEL_EXIT_FLASH_GRAY      26
%define CITY_TUNNEL_EXIT_FLASH_WHITE     31
; Street-lamp pulse colours are direct physical palette indices.
; 157..159 are pale warm-yellow shades (starting near RGB 255,255,180) and 31 is white.  The fast phase
; LUT supplies about five pulses per 64 logical frames without interpolation.
; Canyon street-lamp vertical geometry.  These values are shared by the
; pole/arm wireframe and the filled lamp face.  The current revision is
; reduced to 90% of the previous 370-unit profile.
%define CITY_CANYON_LAMP_POLE_Y        258
%define CITY_CANYON_LAMP_BEND_Y        302
%define CITY_CANYON_LAMP_TOP_Y         333
%define CITY_CANYON_LAMP_BODY_BOTTOM_Y (CITY_CANYON_LAMP_TOP_Y-8)
%define CITY_LAMP_STATIC_COLOR 157        ; pale warm yellow, about RGB 255,255,180
%define CITY_LAMP_PULSE_LEVELS 157,157,158,158,159,159,31,31,31,31,159,159,158,158,157,157
%define CITY_NEON_SIGN_COLORS 95,223,156,31    ; direct PEGC indices: vivid cyan, magenta, yellow, white
%define CITY_TUNNEL_EXIT_FORCE_WHITE     632
%define CITY_TUNNEL_CURVE_FRAME_START 320
%define CITY_TUNNEL_CURVE_Z           21432
%define CITY_TUNNEL_CURVE_EXIT_X      1917
%define CITY_TUNNEL_EXIT_CAMERA_Z      27812
%define CITY_TUNNEL_EXIT_SIN_Q14        5139   ; sin(13/256 turn)
%define CITY_TUNNEL_EXIT_COS_Q14       15557   ; cos(13/256 turn)
%define CITY_RAIL_EXIT_PREVIEW_END      4096   ; stays inside signed 16-bit world Z
%define CITY_RAIL_SCENE_Z_BIAS           -612   ; original whole-model rail placement at scene start
%define CITY_TUNNEL_EXIT_Z             28000  ; camera reaches 27812 at scene end
%define CITY_TUNNEL_CURVE_DENOM       40000  ; 2*R, R=20000 world units
%define CITY_TUNNEL_CURVE_EXIT_YAW    13     ; about 18 degrees tangent angle
%define CITY_TUNNEL_CURVE_EXIT_ROLL   8
%define CITY_APPROACH_CANYON_PHASE 12800
%define CITY_WATER_PALETTE_INDEX 60
%define CITY_RAIL_TRAIN_DELAY 32
%define CITY_RAIL_TRAIN_SPEED 25
%define CITY_RAIL_ONCOMING_START       48
%define CITY_RAIL_ONCOMING_END        416
%define CITY_RAIL_ONCOMING_START_Z  15972
%define CITY_RAIL_ONCOMING_SPEED       28
%define CITY_RAIL_ONCOMING_CARS         8
%define CITY_RAIL_PASS_SWERVE_START     308
%define CITY_RAIL_PASS_SWERVE_PEAK      324
%define CITY_RAIL_PASS_SWERVE_END       504
%define CITY_RAIL_PASS_SWERVE_X          37    ; 2/3 lateral kick; 4-frame later onset, 180-frame recovery
%define CITY_RAIL_PASS_ROLL               3    ; about 4.2 degrees right bank, same pressure pulse


; ---------------------------------------------------------------------------
; Experimental directed-camera edition (camera path only; model geometry is
; unchanged from the normal v18 source).
; ---------------------------------------------------------------------------
%define CITY_SPECIAL_APPROACH_MID_X             220 ; inside the canyon road gap
%define CITY_SPECIAL_APPROACH_GAP_X              60 ; nearly centred before road entry
%define CITY_SPECIAL_APPROACH_X_ALIGN_END       191 ; move into the gap while still above roofs
%define CITY_SPECIAL_APPROACH_MID_Y             850
%define CITY_SPECIAL_APPROACH_GAP_Y             420
%define CITY_SPECIAL_APPROACH_GAP_ROLL          -60 ; about -84 degrees
%define CITY_SPECIAL_BRIDGE_CRUISE_Y            (CITY_BRIDGE_CAMERA_Y * 2)

%define CITY_SPECIAL_PARK_ENTRY_END             255
%define CITY_SPECIAL_PARK_ORBIT_START            256
%define CITY_SPECIAL_PARK_ORBIT_END              767
%define CITY_SPECIAL_PARK_LOW_Y                  150
%define CITY_SPECIAL_PARK_ORBIT_Y                170
%define CITY_SPECIAL_PARK_ORBIT_RADIUS           800
%define CITY_SPECIAL_PARK_ORBIT_ROLL             -14

%define CITY_SPECIAL_NEON_DODGE1_Y               150 ; up
%define CITY_SPECIAL_NEON_DODGE2_Y                90 ; down
%define CITY_SPECIAL_NEON_DODGE3_Y               120 ; up
%define CITY_SPECIAL_NEON_DODGE_PITCH_PHASE      1536 ; about 8.4 degrees toward vertical motion
%define CITY_SPECIAL_NEON_DODGE3_RETURN_FRAMES   (CITY_NEON_DODGE4_START-CITY_NEON_DODGE3_CENTER)
%define CITY_SPECIAL_NEON_DODGE3_END             CITY_NEON_DODGE4_START

%define CITY_SPECIAL_TUNNEL_LOW_Y                -70
%define CITY_SPECIAL_TUNNEL_SWAY_X                35
%define CITY_SPECIAL_TUNNEL_ENTRY_SETTLE          63

%define CITY_SPECIAL_RAIL_HIGH_Y                 430 ; above 360-unit catenary pole top
%define CITY_SPECIAL_RAIL_CLIMB_END              191

%define CITY_SPECIAL_TERMINAL_ORBIT_CENTER_X     (CITY_TERMINAL_LANDMARK_TALL_X + (CITY_TERMINAL_LANDMARK_TALL_W / 2))
%define CITY_SPECIAL_TERMINAL_ORBIT_CENTER_Z     (CITY_TERMINAL_LANDMARK_TALL_Z + (CITY_TERMINAL_LANDMARK_TALL_D / 2))
%define CITY_SPECIAL_TERMINAL_ORBIT_RADIUS       2500
%define CITY_SPECIAL_TERMINAL_ORBIT_Y             900
%define CITY_SPECIAL_TERMINAL_ORBIT_PHASE0       (231 << 8)
%define CITY_SPECIAL_TERMINAL_ORBIT_ENTRY_X       920
%define CITY_SPECIAL_TERMINAL_ORBIT_ENTRY_Z     28926
%define CITY_SPECIAL_TERMINAL_ORBIT_ROLL          -12
%define CITY_SPECIAL_TERMINAL_BLEND_START          32
%define CITY_SPECIAL_TERMINAL_ORBIT_START          64
%define CITY_SPECIAL_TERMINAL_ORBIT_END           319
%define CITY_SPECIAL_TERMINAL_EXIT_START          320
%define CITY_RAIL_CAMERA_ACCEL_START 576
%define CITY_RAIL_CAMERA_ACCEL_END 768
%define CITY_RAIL_CAMERA_FAST_SPEED 42
%define CITY_RAIL_MODEL_END          27400   ; stop near terminal far edge; avoids a long far-rail tail
%define CITY_TERMINAL_RAIL_Z_BIAS    4188   ; preserve far-rail position across -4800 -> 0 terminal model rebase
%define CITY_TERMINAL_TURN_START 32
%define CITY_TERMINAL_DECEL_FRAMES 64
%define CITY_TERMINAL_TURN_LAST 351
