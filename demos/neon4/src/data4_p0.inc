; Copyright (c) 2026 Nakata Maho
; Ported By Maho Nakata.
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the BSD 2-Clause conditions apply.

; ---------------------------------------------------------------------------
; NEON4 P0 data extraction.  Geometry/data only; OPL3 state is intentionally
; excluded from the PC-88VA port.
; ---------------------------------------------------------------------------
; ===========================================================================
; DATA.INC
; Mutable state, geometry tables, sound patches, and sequence data.
; ===========================================================================

draw_page              db 1
frame_counter          dw 0        ; visual timeline (AFS or fixed-FS policy)
render_frame_counter   dw 0        ; immutable timestamp for the frame being drawn
frame_skip             db 0
frame_skip_count       db 0
speed_percent          db 100      ; /SPEED:0-200, 100 = one logical tick per display period
align 2
speed_phase            dw 0        ; visual /SPEED fractional accumulator
audio_speed_phase      dw 0        ; BGM /SPEED accumulator, independent of rendering
adaptive_skip_enabled  db 1       ; default adaptive VSYNC skip; /FS:n selects fixed mode
adaptive_elapsed       db 1       ; display periods consumed by the last rendered iteration
adaptive_vsync_valid   db 0
adaptive_vsync_hooked  db 0
adaptive_vsync_owned   db 0       ; IRQ2 was masked before install, so we own its EOI
adaptive_pic_imr_before db 0
vsync_scheduler_active db 0       ; verified INT 0Ah scheduler is available
vsync_scheduler_running db 0      ; IRQ may run presentation/audio/logical time
vsync_visual_tick_due  db 0       ; fixed-FS: this VSYNC consumes one visual slot
vsync_frame_ready      db 0       ; main thread completed a hidden page
vsync_pending_page     db 1       ; page to expose at the next VSYNC
vsync_present_scene_index db 0    ; immutable palette timestamp of pending page
align 2
vsync_present_scene_frame dw 0
; Prepared palette shared by 16-colour and 256-colour builds.  Entry 0 is the
; whiteout background; entries 1..7 contain the seven animated city colours.
vsync_pending_palette_grb times 8*3 db 0
align 4
adaptive_old_int0a     dd 0
adaptive_vsync_count   dd 0
adaptive_last_present_vsync dd 0
adaptive_pre_present_vsync  dd 0
scene_index            db 0
scene_frame            dw 0
start_scene_request    db 0FFh     ; hidden /SCENE:n developer start, FF=normal

; Camera smoothing is a logical-timeline state, not a render-iteration state.
; AFS may advance frame_counter several times while one page is being drawn.
; These fields let FRAME_RENDER replay only the skipped camera updates.
city_camera_timeline_valid db 0
city_camera_replay         db 0
city_camera_hold_smoothing db 0
align 2
city_camera_last_frame     dw 0

; CPU feature state and NEON RELAY 2 geometry scratch.
cpu_cpuid_present      db 0
cpu_mmx_present        db 0
r2_particle_phase      db 0
r2_section_pulse       db 0
align 2
r2_center_x            dw 320
r2_center_y            dw 200
r2_radius_x            dw 0
r2_radius_y            dw 0
r2_prev_x              dw 0
r2_prev_y              dw 0
r2_current_x           dw 0
r2_current_y           dw 0
r2_mesh_base           dw 0
r2_ring_points         times 16*2 dw 0
r2_colour_cycle        db 1,2,3,4,5,6,7,1,2,3,4,5,6,7,1,2
                       db 3,4,5,6,7,1,2,3,4,5,6,7,1,2,3,4

; Revision 3 scene scratch.  Kept separate from the original tunnel/city state
; so the new entry corridor and restrained re-entry cannot accidentally share
; stale geometry with the previous implementations.
r3_quad_prev_valid      db 0
align 2
r3_entry_vanish_x       dw 320
r3_entry_vanish_y       dw 200
r3_gate_rx               dw 0
r3_gate_ry               dw 0
r3_gate_shear            dw 0
r3_gate_tilt             dw 0
r3_quad_current          times 8 dw 0
r3_quad_previous         times 8 dw 0
r3_route_ptr             dw 0
r3_route_prev_x          dw 0
r3_route_prev_y          dw 0
r3_packet_x              dw 0
r3_axis_point_count      dw 0
r3_axis_current          times 16 dw 0
r3_axis_previous         times 16 dw 0
r4_tunnel_sway_x         dw 0
r4_tunnel_sway_y         dw 0
r4_tunnel_motion_phase   dw 0
r4_tunnel_motion_local   dw 0
r4_tunnel_motion_env     dw 0
r4_tunnel_motion_active  db 0
r11_tunnel_dir_x          dw 0
r11_tunnel_dir_y          dw 0
r11_tunnel_curve_env      dw 0
r11_tunnel_curve_profile  dw 0
r12_tunnel_frame          dw 0
r12_tunnel_ring_limit     db 12
align 2
r4_aurora_offset            dw 0
r4_relay_bridge_remaining   dw 0

; Translation offsets for the eight RELAY SWARM particles at virtual relay
; frame -384.  The bridge evaluates the real RELAY SWARM orbit at virtual
; frames -384..-1 and adds offset * (remaining/256)^2.  Thus each current
; endpoint starts at the former off-screen launch coordinate, the complete
; streak keeps its normal direction, and both position and velocity converge
; to the unmodified RELAY SWARM orbit before the scene boundary.
r4_relay_entry_offset_x:
        dw -640,-367,476,245,-630,203,695,-291
r4_relay_entry_offset_y:
        dw -236,-444,-24,455,46,-543,192,553

draw_color             db 7
grcg_color_cache       db 0FFh

pixel_shift_table      db 0,1,2,3,4,5,6,7
pixel_input_x          dw 0
pixel_mask             db 0

hline_x0               dw 0
hline_x1               dw 0
hline_offset           dw 0
hline_start_mask       db 0
hline_end_mask         db 0
hline_same_byte        db 0
hline_middle_count     dw 0

line_x0                dw 0
line_y0                dw 0
line_x1                dw 0
line_y1                dw 0
line_dx                dw 0
line_dy                dw 0
line_sx                dw 0
line_sy                dw 0
line_err               dw 0
line_e2                dw 0
line_mask              db 0
line_all_visible        db 0

tri_x0                 dw 0
tri_y0                 dw 0
tri_x1                 dw 0
tri_y1                 dw 0
tri_x2                 dw 0
tri_y2                 dw 0
tri_color              db 0
tri_ycur               dw 0
tri_scan_xa            dw 0
tri_scan_xb            dw 0
align 4
tri_long_x             dd 0
tri_long_step          dd 0
tri_short_x            dd 0
tri_short_step         dd 0

edge_x0                dw 0
edge_y0                dw 0

oct_sin_x              dw 0
oct_cos_x              dw 0
oct_sin_y              dw 0
oct_cos_y              dw 0
align 4
oct_tmp_x              dd 0
oct_tmp_y              dd 0
oct_tmp_z1             dd 0
oct_tmp_z2             dd 0
oct_den                dd 0
oct_cross_a            dd 0
oct_num_abs            dd 0
oct_div_sign           dd 0
oct_projected          times 6*3 dw 0

oct_vertices:
        dw 0,-100,0
        dw 0,100,0
        dw -100,0,0
        dw 0,0,-100
        dw 100,0,0
        dw 0,0,100
oct_faces:
        db 0,2,3,5
        db 0,3,4,7
        db 0,4,5,6
        db 0,5,2,4
        db 1,3,2,3
        db 1,4,3,5
        db 1,5,4,2
        db 1,2,5,1
oct_edges:
        db 0,2, 0,3, 0,4, 0,5
        db 1,2, 1,3, 1,4, 1,5
        db 2,3, 3,4, 4,5, 5,2

tunnel_camera          dw 0
tunnel_base_z          dw 0
tunnel_depth           dw 0
tunnel_world           dw 0
tunnel_scale           dw 0
tunnel_center_x        dw 0
tunnel_center_y        dw 0
tunnel_prev_valid      db 0
tunnel_color           db 0
tunnel_roll_index      dw 0
tunnel_secondary       dw 0
tunnel_current         times 16*2 dw 0
tunnel_previous        times 16*2 dw 0

wave_prev_x            dw 0
wave_prev_y            dw 0
wave_y                 dw 0
diamond_r              dw 0
signal_bar_base         dw 0
bar_y                  dw 0
bar_width              dw 0
bar_x0                 dw 0
bar_x1                 dw 0
orb_x                   dw 0
orb_y                   dw 0
city_height            dw 0
city_x0                dw 0
city_x1                dw 0

circle_x:
        dw 256,237,181,98,0,-98,-181,-237
        dw -256,-237,-181,-98,0,98,181,237
circle_y:
        dw 0,98,181,237,256,237,181,98
        dw 0,-98,-181,-237,-256,-237,-181,-98
screen_border_x:
        dw 0,160,320,480,639,639,639,639
        dw 639,480,320,160,0,0,0,0
screen_border_y:
        dw 0,0,0,0,0,100,200,300
        dw 399,399,399,399,399,300,200,100

; Twelve evenly distributed phases for SIGNAL WAKE's moving bars.
; Successive gaps are 33 or 34 lines and the wrap gap is also 33 lines,
; so no pair becomes doubled at the top of the 400-line display.
signal_bar_phase:
        dw 0,33,67,100,133,167,200,233,267,300,333,367

sin_table:
        dw 0,402,804,1205,1606,2006,2404,2801
        dw 3196,3590,3981,4370,4756,5139,5520,5897
        dw 6270,6639,7005,7366,7723,8076,8423,8765
        dw 9102,9434,9760,10080,10394,10702,11003,11297
        dw 11585,11866,12140,12406,12665,12916,13160,13395
        dw 13623,13842,14053,14256,14449,14635,14811,14978
        dw 15137,15286,15426,15557,15679,15791,15893,15986
        dw 16069,16143,16207,16261,16305,16340,16364,16379
        dw 16384,16379,16364,16340,16305,16261,16207,16143
        dw 16069,15986,15893,15791,15679,15557,15426,15286
        dw 15137,14978,14811,14635,14449,14256,14053,13842
        dw 13623,13395,13160,12916,12665,12406,12140,11866
        dw 11585,11297,11003,10702,10394,10080,9760,9434
        dw 9102,8765,8423,8076,7723,7366,7005,6639
        dw 6270,5897,5520,5139,4756,4370,3981,3590
        dw 3196,2801,2404,2006,1606,1205,804,402
        dw 0,-402,-804,-1205,-1606,-2006,-2404,-2801
        dw -3196,-3590,-3981,-4370,-4756,-5139,-5520,-5897
        dw -6270,-6639,-7005,-7366,-7723,-8076,-8423,-8765
        dw -9102,-9434,-9760,-10080,-10394,-10702,-11003,-11297
        dw -11585,-11866,-12140,-12406,-12665,-12916,-13160,-13395
        dw -13623,-13842,-14053,-14256,-14449,-14635,-14811,-14978
        dw -15137,-15286,-15426,-15557,-15679,-15791,-15893,-15986
        dw -16069,-16143,-16207,-16261,-16305,-16340,-16364,-16379
        dw -16384,-16379,-16364,-16340,-16305,-16261,-16207,-16143
        dw -16069,-15986,-15893,-15791,-15679,-15557,-15426,-15286
        dw -15137,-14978,-14811,-14635,-14449,-14256,-14053,-13842
        dw -13623,-13395,-13160,-12916,-12665,-12406,-12140,-11866
        dw -11585,-11297,-11003,-10702,-10394,-10080,-9760,-9434
        dw -9102,-8765,-8423,-8076,-7723,-7366,-7005,-6639
        dw -6270,-5897,-5520,-5139,-4756,-4370,-3981,-3590
        dw -3196,-2801,-2404,-2006,-1606,-1205,-804,-402

; 0.44 finale polar offsets.  The finale phase and all blade offsets are even,
; so 128 entries per radius cover every rendered angle exactly while replacing
; 48 trig-radius multiplications per visible finale frame with indexed loads.
n4_polar175_even_xy:
        dw 175,0,174,-8,174,-17,173,-25,171,-34,169,-42,167,-50,164,-58
        dw 161,-66,158,-74,154,-82,150,-89,145,-97,140,-104,135,-111,129,-117
        dw 123,-123,117,-129,111,-135,104,-140,97,-145,89,-150,82,-154,74,-158
        dw 66,-161,58,-164,50,-167,42,-169,34,-171,25,-173,17,-174,8,-174
        dw 0,-175,-9,-174,-18,-174,-26,-173,-35,-171,-43,-169,-51,-167,-59,-164
        dw -67,-161,-75,-158,-83,-154,-90,-150,-98,-145,-105,-140,-112,-135,-118,-129
        dw -124,-123,-130,-117,-136,-111,-141,-104,-146,-97,-151,-89,-155,-82,-159,-74
        dw -162,-66,-165,-58,-168,-50,-170,-42,-172,-34,-174,-25,-175,-17,-175,-8
        dw -175,0,-175,9,-175,18,-174,26,-172,35,-170,43,-168,51,-165,59
        dw -162,67,-159,75,-155,83,-151,90,-146,98,-141,105,-136,112,-130,118
        dw -124,124,-118,130,-112,136,-105,141,-98,146,-90,151,-83,155,-75,159
        dw -67,162,-59,165,-51,168,-43,170,-35,172,-26,174,-18,175,-9,175
        dw 0,175,8,175,17,175,25,174,34,172,42,170,50,168,58,165
        dw 66,162,74,159,82,155,89,151,97,146,104,141,111,136,117,130
        dw 123,124,129,118,135,112,140,105,145,98,150,90,154,83,158,75
        dw 161,67,164,59,167,51,169,43,171,35,173,26,174,18,174,9
n4_polar92_even_xy:
        dw 92,0,91,-4,91,-9,91,-13,90,-17,89,-22,88,-26,86,-30
        dw 84,-35,83,-39,81,-43,78,-47,76,-51,73,-54,71,-58,68,-61
        dw 65,-65,61,-68,58,-71,54,-73,51,-76,47,-78,43,-81,39,-83
        dw 35,-84,30,-86,26,-88,22,-89,17,-90,13,-91,9,-91,4,-91
        dw 0,-92,-5,-91,-10,-91,-14,-91,-18,-90,-23,-89,-27,-88,-31,-86
        dw -36,-84,-40,-83,-44,-81,-48,-78,-52,-76,-55,-73,-59,-71,-62,-68
        dw -66,-65,-69,-61,-72,-58,-74,-54,-77,-51,-79,-47,-82,-43,-84,-39
        dw -85,-35,-87,-30,-89,-26,-90,-22,-91,-17,-92,-13,-92,-9,-92,-4
        dw -92,0,-92,5,-92,10,-92,14,-91,18,-90,23,-89,27,-87,31
        dw -85,36,-84,40,-82,44,-79,48,-77,52,-74,55,-72,59,-69,62
        dw -66,66,-62,69,-59,72,-55,74,-52,77,-48,79,-44,82,-40,84
        dw -36,85,-31,87,-27,89,-23,90,-18,91,-14,92,-10,92,-5,92
        dw 0,92,4,92,9,92,13,92,17,91,22,90,26,89,30,87
        dw 35,85,39,84,43,82,47,79,51,77,54,74,58,72,61,69
        dw 65,66,68,62,71,59,73,55,76,52,78,48,81,44,83,40
        dw 84,36,86,31,88,27,89,23,90,18,91,14,91,10,91,5



; Exact lookup replacements for hot-loop integer division.
; Q16 reciprocal table for exact corrected perspective division, d=320..520.
persp_recip_q16:
        dw 204,204,203,202,202,201,201,200,199,199,198,197,197,196,196,195
        dw 195,194,193,193,192,192,191,191,190,189,189,188,188,187,187,186
        dw 186,185,185,184,184,183,183,182,182,181,181,180,180,179,179,178
        dw 178,177,177,176,176,175,175,174,174,173,173,172,172,172,171,171
        dw 170,170,169,169,168,168,168,167,167,166,166,165,165,165,164,164
        dw 163,163,163,162,162,161,161,161,160,160,159,159,159,158,158,157
        dw 157,157,156,156,156,155,155,154,154,154,153,153,153,152,152,152
        dw 151,151,151,150,150,149,149,149,148,148,148,147,147,147,146,146
        dw 146,145,145,145,144,144,144,144,143,143,143,142,142,142,141,141
        dw 141,140,140,140,140,139,139,139,138,138,138,137,137,137,137,136
        dw 136,136,135,135,135,135,134,134,134,134,133,133,133,132,132,132
        dw 132,131,131,131,131,130,130,130,130,129,129,129,129,128,128,128
        dw 128,127,127,127,127,126,126,126,126

tunnel_scale_table:
        dw 296,292,287,283,279,275,271,267,263,260,256,253,250,246,243,240
        dw 237,234,231,228,226,223,220,218,215,213,211,208,206,204,202,200
        dw 197,195,193,191,190,188,186,184,182,180,179,177,175,174,172,171
        dw 169,168,166,165,163,162,161,159,158,157,155,154,153,152,150,149
        dw 148,147,146,145,143,142,141,140,139,138,137,136,135,134,133,132
        dw 131,131,130,129,128,127,126,125,125,124,123,122,121,121,120,119
        dw 118,118,117,116,115,115,114,113,113,112,111,111,110,109,109,108
        dw 107,107,106,106,105,104,104,103,103,102,102,101,101,100,100,99
        dw 98,98,97,97,96,96,95,95,95,94,94,93,93,92,92,91
        dw 91,90,90,90,89,89,88,88,87,87,87,86,86,85,85,85
        dw 84,84,84,83,83,82,82,82,81,81,81,80,80,80,79,79
        dw 79,78,78,78,77,77,77,76,76,76,76,75,75,75,74,74
        dw 74,73,73,73,73,72,72,72,71,71,71,71,70,70,70,70
        dw 69,69,69,69,68,68,68,68,67,67,67,67,66,66,66,66
        dw 65,65,65,65,65,64,64,64,64,63,63,63,63,63,62,62
        dw 62,62,62,61,61,61,61,61,60,60,60,60,60,59,59,59
        dw 59,59,59,58,58,58,58,58,57,57,57,57,57,57,56,56
        dw 56,56,56,56,55,55,55,55,55,55,54,54,54,54,54,54
        dw 53,53,53,53,53,53,53,52,52,52,52,52,52,52,51,51
        dw 51,51,51,51,51,50,50,50,50,50,50,50,50,49,49,49
        dw 49,49,49,49,48,48,48,48,48,48,48,48,47,47,47,47
        dw 47,47,47,47,47,46,46,46,46,46,46,46,46,46,45,45
        dw 45,45,45,45,45,45,45,44,44,44,44,44,44,44,44,44
        dw 43,43,43,43,43,43,43,43,43,43,42,42,42,42,42,42
        dw 42,42,42,42,42,41,41,41,41,41,41,41,41,41,41,41
        dw 40,40,40,40,40,40,40,40,40,40,40,40,39,39,39,39
        dw 39,39,39,39,39,39,39,39,38,38,38,38,38,38,38,38
        dw 38,38,38,38,38,37,37,37,37,37,37,37,37,37,37,37
        dw 37,37,36,36,36,36,36,36,36,36,36,36,36,36,36,36
        dw 35,35,35,35,35,35,35,35,35,35,35,35,35,35,35,34
        dw 34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,33
        dw 33,33,33,33,33,33,33,33,33,33,33,33,33,33,33,33

mod6_table:
        db 0,1,2,3,4,5,0,1,2,3,4,5,0,1,2,3,4,5,0,1,2,3,4,5
        db 0,1,2,3,4,5,0,1,2,3,4,5,0,1,2,3,4,5,0,1,2,3,4,5
        db 0,1,2,3,4,5,0,1,2,3,4,5,0,1,2,3,4,5,0,1,2,3,4,5

; Text overlay data
text_last_scene         db 0FFh
ticker_phase            dw 0
hex_digits              db '0123456789ABCDEF'
hex_buffer              db '0000',0
text_top                db ' NEON RELAY 4 :: GEOMETRIC SOLID                               FRAME=',0
text_status_left        db 'PEGC 256-COLOR / SOLID SURFACES / PAGE FLIP',0
text_status_plane       db 'PEGC PLANE / SOLID + PROCEDURAL TEXTURE',0
text_status_packed      db 'PEGC PACKED LITE / SOLID SURFACE RENDERER',0
text_status_packed_full db 'PEGC PACKED FULL / SOLID + PATTERN FILL',0
text_status_cpu_mmx     db 'CPU: 386 FIXED-POINT SOLID 3D',0
text_status_cpu_scalar  db 'CPU: 386+ INTEGER PERSPECTIVE / SCANLINE FILL',0
text_status_opna6       db 'OPNA6F+3S',0
text_status_opna3       db 'OPNA3F+3S',0
text_status_26k         db '26K 3F+3S',0
text_status_silent      db 'NO SOUND ',0
scene_title_ptrs        dw scene_title_0,scene_title_1,scene_title_2,scene_title_3
                        dw scene_title_4,scene_title_5,scene_title_6,scene_title_7
scene_title_0           db '[00] SIGNAL SEED        / FIRST SURFACE / RASTER CARRIER',0
scene_title_1           db '[01] FACET ASSEMBLY     / SOLID INSIDE WIRE CAGE',0
scene_title_2           db '[02] MATERIAL ASSEMBLY  / SATELLITES / POLYGON RASTER SHEAR',0
scene_title_3           db '[03] MORPH GATE         / CONVEX QUADS / SCANNED TRANSITION',0
scene_title_4           db '[04] RASTER TRANSFER    / LISSAJOUS BLIT / PEGC SHIFT COPY',0
scene_title_5           db '[05] SURFACE WAVE       / RIBBONS / FM STRING VISUALIZER',0
scene_title_6           db '[06] GRID ARRIVAL       / PERSPECTIVE FLOOR / RECONSTRUCTION',0
scene_title_7           db '[07] SOLID FINALE       / SIGNAL CONVERGENCE / COLOUR CORONA',0
text_console_clear_sequence db 1Bh,'*','$'
ticker_text             db ' NEON RELAY 4 // SIGNAL SEED // FACET ASSEMBLY // MATERIAL ASSEMBLY // MORPH GATE // RASTER TRANSFER // SURFACE WAVE // GRID ARRIVAL // SOLID FINALE // ',0
ticker_text_end:

; Common sound state
sound_present           db 0
sound_mode              db SOUND_MODE_AUTO
show_help               db 0
bad_option              db 0
opna_detected           db 0
opna_device_type        db OPN_DEVICE_NONE
opna_volume             db 115     ; OPNA-only: modest FM+SSG boost over patch level
opna_fmwait_count        db 0       ; /FMWAIT: 0=off; bare /FMWAIT selects 2
opna_ssg_trim           db 100     ; /SV relative SSG level; 100 preserves the authored balance
opna_rhythm_volume      db 100
opna_volume_explicit    db 0
opna_tl_adjust          db 0       ; /NV:100 default = patch-table level
cmd_token               times CMD_TOKEN_MAX+1 db 0
music_index             dw 0
music_score_index       dw 0
music_divider           db 1
music_section           db 0FFh
; Visual-only mirror of the first three FM voices.  Updated by the common
; sequencer so scanline animation follows the actual score step.
music_visual_note       db 0FFh,0FFh,0FFh
music_visual_energy     db 0,0,0

; OPNA/YM2608 state
opna_present            db 0
opna_channel_count      db 0
opna_base               dw 0188h
opna_control_port       dw 0
opna_control_saved      db 0
opna_control_changed    db 0
opna_psg_saved          times 14 db 0
opna_psg_mixer          db 3Fh
opna_channel_tmp        db 0
opna_bank_tmp           db 0
opna_local_tmp          db 0
opna_note_tmp           db 0
opna_fnum_tmp           dw 0
opna_ssg_channel_tmp    db 0
opna_ssg_note_tmp       db 0
opna_ssg_period_tmp     dw 0
opna_scene_atten_tmp    db 0

opna_port_candidates    dw 0088h,0188h,0288h,0388h
opn26_port_candidates   dw 0088h,0188h
opn26_probe_result      db 0
opna_operator_offsets   db 0,4,8,12
opna_rhythm_voice_setup db 0D8h,0D4h,098h,0D2h,094h,054h

; Direct rhythm_steps data is stored with the common score.

; Carrier flags in YM register-slot order OP1,OP3,OP2,OP4.
; /NV affects audible carriers.  Per-bar offsets are then applied to all four
; operators, matching the authored OPNA Draft 8 dynamics.
opna_carrier_flags      db 0,0,1,1, 0,1,1,1, 0,0,0,1
                        db 0,0,1,1, 0,1,1,1, 1,1,1,1

; Six four-operator OPNA patches from METROPOLITAN IGNITION.
opna_mul:
        db 01h,04h,02h,01h, 01h,02h,03h,01h, 01h,01h,02h,01h
        db 01h,01h,02h,02h, 01h,03h,02h,01h, 01h,02h,04h,01h
opna_tl:
        db 20h,2Ah,12h,08h, 18h,24h,2Eh,08h, 10h,22h,28h,04h
        db 24h,20h,16h,14h, 22h,28h,14h,0Eh, 24h,2Ah,2Eh,12h
opna_ar:
        db 1Fh,1Fh,1Fh,1Fh, 1Fh,1Fh,1Fh,1Fh, 1Fh,1Fh,1Eh,1Fh
        db 18h,17h,1Ah,19h, 1Fh,1Fh,1Eh,1Fh, 1Eh,1Fh,1Dh,1Fh
opna_dr:
        db 18h,1Ah,10h,0Eh, 17h,16h,18h,14h, 12h,15h,18h,10h
        db 07h,08h,09h,08h, 14h,16h,10h,0Eh, 0Ch,10h,12h,0Ah
opna_sr:
        db 0Ch,0Eh,05h,04h, 0Ah,09h,0Bh,08h, 05h,08h,0Ah,04h
        db 03h,03h,04h,03h, 08h,0Ah,05h,04h, 04h,06h,07h,03h
opna_slrr:
        db 67h,77h,57h,46h, 75h,75h,85h,64h, 55h,66h,76h,45h
        db 78h,78h,67h,67h, 66h,77h,57h,46h, 68h,79h,79h,57h
opna_alg             db 1Ch,1Dh,19h,14h,1Dh,0Fh
opna_pan             db 0C0h,080h,0C0h,040h,080h,040h
opna_fnum_table      dw 026Ah,028Fh,02B6h,02DFh,030Bh,0339h
                     dw 036Ah,039Eh,03D5h,0410h,044Eh,048Fh

; SSG periods use the same pitch convention as FM: block 4 is C5.
opna_ssg_period_c5   dw 239,225,212,200,189,178
                     dw 168,159,150,142,134,126

; Per-bar authored dynamics. FM entries are signed TL offsets; SSG and rhythm
; entries are absolute source levels before /NV or /RV scaling.  Bars 1..32 now
; follow the 0.20 hook-driven arrangement: stable strong sections, deliberate drops, and a clear final return.
opna_bar_fm_atten:
        db 0FFh,04h,01h,09h,0Ah,0Ch ; bar 01
        db 0FFh,04h,01h,09h,0Ah,0Ch ; bar 02
        db 0FFh,04h,01h,09h,0Ah,0Ch ; bar 03
        db 0FFh,04h,01h,09h,0Ah,0Ch ; bar 04
        db 0FFh,04h,01h,09h,0Ah,0Ch ; bar 05
        db 0FFh,04h,01h,09h,0Ah,0Ch ; bar 06
        db 0FFh,04h,01h,09h,0Ah,0Ch ; bar 07
        db 0FEh,03h,01h,06h,07h,08h ; bar 08
        db 0FFh,04h,00h,09h,0Ah,0Ch ; bar 09
        db 0FFh,04h,00h,09h,0Ah,0Ch ; bar 10
        db 0FFh,04h,00h,09h,0Ah,0Ch ; bar 11
        db 0FFh,04h,00h,09h,0Ah,0Ch ; bar 12
        db 0FFh,08h,04h,0Eh,0Fh,12h ; bar 13
        db 0FFh,08h,04h,0Eh,0Fh,12h ; bar 14
        db 0FFh,08h,04h,0Eh,0Fh,12h ; bar 15
        db 0FEh,03h,00h,06h,07h,08h ; bar 16
        db 0FFh,04h,00h,09h,0Ah,0Ch ; bar 17
        db 0FFh,04h,00h,09h,0Ah,0Ch ; bar 18
        db 0FFh,04h,00h,09h,0Ah,0Ch ; bar 19
        db 0FFh,04h,00h,09h,0Ah,0Ch ; bar 20
        db 0FFh,08h,04h,0Eh,0Fh,12h ; bar 21
        db 0FFh,08h,04h,0Eh,0Fh,12h ; bar 22
        db 0FFh,08h,04h,0Eh,0Fh,12h ; bar 23
        db 0FEh,03h,00h,06h,07h,08h ; bar 24
        db 0FDh,02h,0FFh,05h,06h,07h ; bar 25
        db 0FDh,02h,0FFh,05h,06h,07h ; bar 26
        db 0FDh,02h,0FFh,05h,06h,07h ; bar 27
        db 0FDh,02h,0FFh,05h,06h,07h ; bar 28
        db 0FFh,08h,04h,0Eh,0Fh,12h ; bar 29
        db 0FDh,02h,0FFh,05h,06h,07h ; bar 30
        db 0FDh,02h,0FFh,05h,06h,07h ; bar 31
        db 0FDh,06h,03h,09h,0Ah,0Ch ; bar 32
opna_bar_ssg_volume:
        db 09h,06h,06h ; bar 01
        db 09h,06h,06h ; bar 02
        db 09h,06h,06h ; bar 03
        db 09h,06h,06h ; bar 04
        db 09h,06h,06h ; bar 05
        db 09h,06h,06h ; bar 06
        db 09h,06h,06h ; bar 07
        db 0Ah,07h,07h ; bar 08
        db 09h,06h,06h ; bar 09
        db 09h,06h,06h ; bar 10
        db 09h,06h,06h ; bar 11
        db 09h,06h,06h ; bar 12
        db 05h,03h,03h ; bar 13
        db 05h,03h,03h ; bar 14
        db 05h,03h,03h ; bar 15
        db 0Ah,07h,07h ; bar 16
        db 09h,06h,06h ; bar 17
        db 09h,06h,06h ; bar 18
        db 09h,06h,06h ; bar 19
        db 09h,06h,06h ; bar 20
        db 05h,03h,03h ; bar 21
        db 05h,03h,03h ; bar 22
        db 05h,03h,03h ; bar 23
        db 0Ah,07h,07h ; bar 24
        db 0Bh,08h,08h ; bar 25
        db 0Bh,08h,08h ; bar 26
        db 0Bh,08h,08h ; bar 27
        db 0Bh,08h,08h ; bar 28
        db 05h,03h,03h ; bar 29
        db 0Bh,08h,08h ; bar 30
        db 0Bh,08h,08h ; bar 31
        db 06h,03h,04h ; bar 32
opna_bar_rhythm_level:
        db 32h ; bar 01
        db 33h ; bar 02
        db 32h ; bar 03
        db 33h ; bar 04
        db 32h ; bar 05
        db 33h ; bar 06
        db 32h ; bar 07
        db 38h ; bar 08
        db 35h ; bar 09
        db 36h ; bar 10
        db 35h ; bar 11
        db 36h ; bar 12
        db 20h ; bar 13
        db 20h ; bar 14
        db 18h ; bar 15
        db 3Bh ; bar 16
        db 35h ; bar 17
        db 36h ; bar 18
        db 35h ; bar 19
        db 36h ; bar 20
        db 20h ; bar 21
        db 20h ; bar 22
        db 18h ; bar 23
        db 3Bh ; bar 24
        db 3Dh ; bar 25
        db 3Dh ; bar 26
        db 3Dh ; bar 27
        db 3Dh ; bar 28
        db 20h ; bar 29
        db 3Fh ; bar 30
%include "music_neon4_melody.inc"



; ---------------------------------------------------------------------------
; Minimal compatibility state retained from the NEON3 camera scheduler.
; The geometric NEON4 scenes themselves do not use a world camera.
; ---------------------------------------------------------------------------
city_camera_initialized db 0
align 2
city_global_frame       dw 0

; ---------------------------------------------------------------------------
; NEON 4 / GEOMETRIC SOLID surface-renderer state
; ---------------------------------------------------------------------------
align 4
n4_tri_cross           dd 0
n4_slope_long          dd 0
n4_slope_short         dd 0
n4_x_long              dd 0
n4_x_short             dd 0
n4_tri_x0              dw 0
n4_tri_y0              dw 0
n4_tri_x1              dw 0
n4_tri_y1              dw 0
n4_tri_x2              dw 0
n4_tri_y2              dw 0
n4_v0x                 dw 0
n4_v0y                 dw 0
n4_v1x                 dw 0
n4_v1y                 dw 0
n4_v2x                 dw 0
n4_v2y                 dw 0
n4_scan_y              dw 0
n4_half_end_y          dw 0
n4_span_x0             dw 0
n4_fill_colour_a       db 0
n4_fill_colour_b       db 0
n4_fill_mode           db 0
align 2
n4_quad_world          times 4*3 dw 0
n4_quad_screen         times 4*2 dw 0

n4_obj_x0              dw 0
n4_obj_x1              dw 0
n4_obj_y0              dw 0
n4_obj_y1              dw 0
n4_obj_z0              dw 0
n4_obj_z1              dw 0
n4_side_x              dw 0
n4_obj_colour_front    db 0
n4_obj_colour_alt      db 0
n4_obj_colour_side     db 0
n4_obj_colour_top      db 0
n4_obj_fill_mode       db 0
align 2

n4_grid_x0             dw 0
n4_grid_x1             dw 0
n4_grid_y0             dw 0
n4_grid_y1             dw 0
n4_grid_z              dw 0
n4_grid_step_x         dw 0
n4_grid_step_y         dw 0
n4_grid_cursor         dw 0
n4_grid_colour         db 0
align 2

n4_floor_x0            dw 0
n4_floor_x1            dw 0
n4_floor_y             dw 0
n4_floor_z0            dw 0
n4_floor_z1            dw 0
n4_floor_step_x        dw 0
n4_floor_step_z        dw 0
n4_surface_colour_a    db 0
n4_surface_colour_b    db 0
n4_surface_fill_mode   db 0
align 2

n4_wall_x              dw 0
n4_wall_y0             dw 0
n4_wall_y1             dw 0
n4_wall_z0             dw 0
n4_wall_z1             dw 0

n4_loop_index          dw 0
n4_depth_index         dw 0
n4_segment_index       dw 0
n4_motion_phase        dw 0
n4_tunnel_z0           dw 0
n4_tunnel_z1           dw 0
n4_tunnel_x0           dw 0
n4_tunnel_y0           dw 0
n4_tunnel_x1           dw 0
n4_tunnel_y1           dw 0

n4_fan_cx              dw 0
n4_fan_cy              dw 0
n4_fan_radius          dw 0
n4_tmp_radius          dw 0
n4_tmp_x               dw 0
n4_tmp_y               dw 0
n4_synth_x             dw 0
n4_fan_colour          db 0
n4_fan_phase           db 0
n4_fan_blade           db 0
n4_fan_angle           db 0
n4_tmp_angle           db 0
align 2

; Octagonal tunnel ring, X/Y around the camera axis.  The asymmetry in Y gives
; the lower panels a flatter floor-like read instead of a perfect pipe.
n4_tunnel_ring_xy:
        dw -720,   0
        dw -520, 500
        dw    0, 650
        dw  520, 500
        dw  720,   0
        dw  540,-390
        dw    0,-470
        dw -540,-390
n4_tmp_colour          db 0


; ---------------------------------------------------------------------------
; NEON 4 0.7 geometric-solid renderer state
; ---------------------------------------------------------------------------
N4_GEO_MAX_VERTICES equ 32
N4_GEO_RECIP_SHIFT  equ 3             ; one reciprocal entry per 8 depth units
N4_GEO_FOCAL        equ 248
N4_GEO_MIN_CROSS    equ 72             ; sub-pixel/edge-on faces are not filled
N4_POLY_RASTER_SHIFT equ 10            ; Q2.14 sine -> +/-16px polygon shear

n4_fill_solid_prepared db 0

; Dirty state retained independently for physical PEGC page 0/1.  Values:
; 0=invalid, 1=ordinary union rectangle, 2=RASTER seven-object list,
; 3=SURFACE exact reconstruction, 4=ordinary scanline span history.
; The renderer never lowers resolution; it only avoids rewriting black pixels.
n4_dirty_valid      db 0,0
n4_dirty_x0         dw 0,0
n4_dirty_y0         dw 0,0
n4_dirty_x1         dw 0,0
n4_dirty_y1         dw 0,0
n4_current_dirty_valid db 0
n4_current_dirty_x0 dw 0
n4_current_dirty_y0 dw 0
n4_current_dirty_x1 dw 0
n4_current_dirty_y1 dw 0

; 0.47 scanline dirty-span history for ordinary geometry scenes.  X is stored
; in four-pixel units (0..159), so both physical pages need only 1600 bytes.
; FFh in min[] means that scanline was untouched.  GRID/RASTER/SURFACE keep
; their specialised older clear paths because recording their dense spans costs
; more than it saves.
n4_dirty_span_enable      db 0
        align 2
n4_dirty_span_base        dw 0
n4_dirty_span_current_y0  dw 07FFFh
n4_dirty_span_current_y1  dw -1
n4_dirty_span_y0          dw 07FFFh,07FFFh
n4_dirty_span_y1          dw -1,-1
n4_dirty_span_min         times 800 db 0FFh
n4_dirty_span_max         times 800 db 0

; RASTER TRANSFER page-local object rectangles: 7 records per physical page.
; n4_dirty_valid value 2 selects these instead of the ordinary union rectangle.
n4_raster_dirty_count db 0,0
align 2
n4_raster_dirty_x0   times 14 dw 0
n4_raster_dirty_y0   times 14 dw 0
n4_raster_dirty_x1   times 14 dw 0
n4_raster_dirty_y1   times 14 dw 0
n4_dirty_tmp_x0      dw 0
n4_dirty_tmp_y0      dw 0
n4_dirty_tmp_x1      dw 0
n4_dirty_tmp_y1      dw 0

; Lightweight object transform.  Each source vertex is transformed once per
; frame and all faces reuse the projected result.
n4_geo_angle_y      db 0
n4_geo_angle_x      db 0
n4_geo_fill_mode    db 0
n4_geo_shade_enable db 1
n4_geo_colour_base  db 32          ; direct physical 32-colour ramp base
n4_poly_raster_phase_bias db 0
n4_ribbon_fill_mode db 0
align 2
n4_geo_center_x     dw 320
n4_geo_center_y     dw 205
n4_geo_object_depth dw 1500
n4_geo_vertex_count dw 0
n4_geo_sin_y        dw 0
n4_geo_cos_y        dw 16384
n4_geo_sin_x        dw 0
n4_geo_cos_x        dw 16384
n4_geo_tmp_x        dd 0
n4_geo_tmp_y        dd 0
n4_geo_tmp_z        dd 0
n4_geo_screen_x     times N4_GEO_MAX_VERTICES dw 0
n4_geo_screen_y     times N4_GEO_MAX_VERTICES dw 0
n4_geo_depth_z      times N4_GEO_MAX_VERTICES dw 0
n4_geo_face_cross   dd 0
n4_geo_face_count   dw 0
n4_morph_t          dw 0
n4_geo_work_vertices times N4_GEO_MAX_VERTICES*3 dw 0

; Reciprocal table trades about 1 KiB of memory for two IDIVs per projected
; vertex.  8-unit buckets keep the maximum visible error around/below one pixel
; for the authored geometry while remaining monotonic and visually smooth.
align 2
n4_geo_recip_lut:
%assign __n4ri 0
%rep 512
%if __n4ri = 0
        dw 0
%else
        dw ((N4_GEO_FOCAL * 256 + (__n4ri*8)/2) / (__n4ri*8))
%endif
%assign __n4ri __n4ri+1
%endrep


; 16.16 reciprocal table for triangle edge dy.  About 2 KiB removes nearly all
; IDIV instructions from visible triangle setup.  Entry 0 is unused.
align 4
n4_scan_recip_q16:
%assign __n4dy 0
%rep 512
%if __n4dy = 0
        dd 0
%else
        dd ((65536 + __n4dy/2) / __n4dy)
%endif
%assign __n4dy __n4dy+1
%endrep

; Convex cube, vertex order: x,y,z.  Face winding is outward; because screen Y
; is inverted, visible faces have a positive screen-space cross product.
n4_geo_cube_vertices:
        dw -220,-220,-220,   220,-220,-220,   220,220,-220,  -220,220,-220
        dw -220,-220, 220,   220,-220, 220,   220,220, 220,  -220,220, 220
; 18% larger construction cage used by FACET ASSEMBLY.  It shares the solid's
; centre/orientation so the object reads as physically contained by the wire.
n4_geo_cube_cage_vertices:
        dw -260,-260,-260,   260,-260,-260,   260,260,-260,  -260,260,-260
        dw -260,-260, 260,   260,-260, 260,   260,260, 260,  -260,260, 260
n4_geo_cube_edges:
        db 0,1, 1,2, 2,3, 3,0
        db 4,5, 5,6, 6,7, 7,4
        db 0,4, 1,5, 2,6, 3,7
n4_geo_cube_faces:
        db 0,3,2,0,  0,2,1,0              ; legacy triangle topology
        db 4,5,6,0,  4,6,7,0
        db 0,4,7,0,  0,7,3,0
        db 1,2,6,0,  1,6,5,0
        db 0,1,5,0,  0,5,4,0
        db 3,7,6,0,  3,6,2,0
; Six logical cube surfaces.  0.6 shades/culls once per quad, then fills both
; halves with the same colour so the triangulation diagonal is invisible.
n4_geo_cube_quads:
        db 0,3,2,1,  4,5,6,7
        db 0,4,7,3,  1,2,6,5
        db 0,1,5,4,  3,7,6,2

n4_geo_octa_vertices:
        dw 0,250,0,  0,-250,0,  -250,0,0,  250,0,0,  0,0,-250,  0,0,250
n4_geo_octa_faces:
        db 0,3,4,0,  0,4,2,0,  0,2,5,0,  0,5,3,0
        db 1,4,3,0,  1,2,4,0,  1,5,2,0,  1,3,5,0

; Additional convex solids used throughout the 0.7 story/stage scenes.
n4_geo_tetra_vertices:
        dw  220, 220, 220,  -220,-220, 220,  -220, 220,-220,   220,-220,-220
n4_geo_tetra_faces:
        db 0,2,1,0,  0,1,3,0,  0,3,2,0,  1,2,3,0

n4_geo_pyramid_vertices:
        dw -220,-220,-220,   220,-220,-220,   220,-220,220,  -220,-220,220
        dw    0, 280,   0
n4_geo_pyramid_faces:
        db 0,2,3,0,  0,1,2,0              ; square base
        db 0,4,1,0,  1,4,2,0,  2,4,3,0,  3,4,0,0

; Icosahedron, phi approximated as 243/150.  Twenty true triangular facets make
; a visibly different smooth-ish solid without a costly sphere tessellation.
n4_geo_icosa_vertices:
        dw -150, 243,   0,   150, 243,   0,  -150,-243,   0,   150,-243,   0
        dw    0,-150, 243,     0, 150, 243,     0,-150,-243,     0, 150,-243
        dw  243,   0,-150,   243,   0, 150,  -243,   0,-150,  -243,   0, 150
n4_geo_icosa_faces:
        db 0,11,5,0,  0,5,1,0,  0,1,7,0,  0,7,10,0, 0,10,11,0
        db 1,5,9,0,   5,11,4,0, 11,10,2,0, 10,7,6,0, 7,1,8,0
        db 3,9,4,0,   3,4,2,0,  3,2,6,0,  3,6,8,0,   3,8,9,0
        db 4,9,5,0,   2,4,11,0, 6,2,10,0, 8,6,7,0,   9,8,1,0

; Convex tapered-prism morph target.  Unlike the old twisted target, all six
; logical surfaces remain outward-facing/convex throughout cube interpolation.
n4_geo_morph_target:
        dw -220,-220,-220,   220,-220,-220,   140,260,-140,  -140,260,-140
        dw -220,-220, 220,   220,-220, 220,   140,260, 140,  -140,260, 140

; Perspective checker bands: Y and half-width at each depth boundary.  Using a
; table removes per-tile perspective division; interpolation within a band is
; exact enough for the regular floor grid.
n4_checker_y:       dw 206,218,235,258,292,334,380
n4_checker_halfw:   dw 28,48,76,112,160,220,292
n4_checker_center_x dw 320
n4_checker_y_offset dw 0
n4_checker_row_limit db 6
align 2
n4_ribbon_base_y   dw 0
n4_ribbon_thickness dw 0
n4_ribbon_phase_bias db 0
n4_ribbon_colour_base db 160
; BGM-synchronised scanline-string state used in SURFACE WAVE.
n4_string_base_y       dw 0
n4_string_prev_x       dw 0
n4_string_prev_y       dw 0
n4_string_phase        dw 0          ; current segment phase, advanced by recurrence
n4_string_phase_step   dw 0          ; spatial phase increment for one segment
n4_string_energy3      dw 0          ; visual energy * 3, invariant for one string
n4_string_note         db 0FFh
n4_string_energy       db 0
n4_string_colour       db 0
n4_string_channel      db 0
; 256-colour SURFACE WAVE page-exact erase state.  Dirty type 3 reconstructs
; the previous page image instead of clearing its large conservative envelope.
n4_surface_clear_active db 0
n4_surface_saved_draw_color db 0
align 2
n4_surface_prev_frame   dw 0,0
; Exact FM marker positions for each physical page.  Layout is
; page * (3 channels * 16 markers) + channel * 16 + marker, one word each.
; Saving the rendered Y coordinate avoids reconstruction errors when AFS/music
; state advances between page uses.
n4_surface_marker_y     times 96 dw 0
n4_surface_marker_store_base dw 0
n4_surface_saved_frame  dw 0
n4_wire_pass           db 0               ; 0=far cage edges, 1=near cage edges
align 2
n4_wire_z0             dw 0
n4_wire_z1             dw 0
; Fixed-end amplitude envelope across 32 segments: a plucked-string silhouette.
n4_string_envelope:
        db 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
align 2

; Direct palette indices spread across several of the 32-entry ramps.
; These are intentionally physical colours (>=8), not logical 1..7 slots.
n4_burst_colours:
        db 43,52,61,75,84,93,107,116,125,139,148,157,171,180,203,236
n4_finale_colours:
        db 47,58,79,90,111,122,143,154,175,186,207,218,231,239,247,255

; 48x48 user-supplied raster icon reduced to four grey levels on top of the
; existing framed signal card background.  Each row stores a span count and the
; packed span list stores x0,x1,physical-colour triplets.
; User-editable four-tone palette for the 48x48 image.
; Entries are source luminance levels 0..3 (darkest -> lightest).
; Reorder/change ONLY these four physical palette indices to tune the image.
; CAT 48x48 card colours.  These two values control the box itself and are
; intentionally independent from the four image luminance levels below.
; White/light areas of the source PNG are transparent, so a bright box fill is
; needed when the original image had a white background.
n4_cat48_box_fill      db 95      ; card interior / transparent-image background
n4_cat48_box_edge      db 52      ; two-pixel frame and side rails

; User image luminance mapping: darkest -> lightest source level.
n4_cat48_palette:
        db 52,63,84,95

n4_cat48_row_counts:
        db 6,9,19,15,24,20,14,7,11,10,12,11
        db 10,9,11,13,11,10,9,5,12,15,8,10
        db 10,9,8,10,13,22,18,13,15,12,14,9
        db 12,10,16,26,24,21,20,24,19,22,14,8
n4_cat48_row_spans:
        db 7,7,3,8,10,2,11,11,3,36,36,3
        db 37,39,2,40,40,3,6,6,1,7,7,0
        db 8,12,1,13,13,2,14,14,3,33,33,3
        db 34,34,2,35,40,1,41,41,2,5,5,1
        db 6,6,0,7,7,2,8,8,3,10,11,3
        db 12,12,2,13,13,1,14,14,0,15,15,1
        db 16,16,3,31,31,3,32,32,1,33,33,0
        db 34,34,1,35,35,2,36,36,3,40,40,2
        db 41,41,0,42,42,2,4,4,3,5,5,0
        db 6,6,2,14,14,2,15,15,1,16,16,0
        db 17,17,2,18,18,3,29,29,3,30,30,1
        db 31,31,0,32,32,1,33,33,2,41,41,2
        db 42,42,0,4,4,3,5,5,0,6,6,3
        db 9,9,3,10,11,1,12,12,2,16,16,2
        db 17,17,1,18,18,0,19,19,1,20,23,2
        db 24,24,3,25,27,2,28,28,1,29,29,0
        db 30,30,1,31,31,2,34,34,3,35,35,2
        db 36,37,1,38,38,3,41,41,3,42,42,0
        db 43,43,3,4,4,3,5,5,0,6,6,3
        db 10,10,3,11,11,2,12,13,1,14,14,3
        db 18,18,3,19,19,2,20,27,1,28,28,2
        db 29,29,3,33,33,2,34,34,0,35,35,1
        db 36,36,2,37,37,3,41,41,3,42,42,0
        db 43,43,3,4,4,3,5,5,1,6,6,3
        db 12,12,3,13,13,1,14,14,0,15,15,3
        db 22,25,3,32,32,2,33,33,0,34,34,1
        db 35,35,3,41,41,2,42,42,0,5,5,1
        db 6,6,2,12,13,3,14,15,1,32,33,1
        db 34,35,3,41,42,1,5,6,1,12,13,3
        db 14,14,2,15,15,0,16,16,3,31,31,3
        db 32,32,0,33,33,2,34,35,3,41,41,1
        db 42,42,2,5,5,2,6,6,1,12,13,3
        db 14,14,1,15,15,0,32,33,1,34,35,3
        db 40,40,3,41,41,1,42,42,2,5,5,2
        db 6,6,0,7,7,3,13,13,2,14,14,0
        db 15,15,2,32,32,2,33,33,0,34,34,2
        db 40,40,3,41,41,1,42,42,3,5,5,3
        db 6,6,0,7,7,3,12,12,2,13,13,0
        db 14,14,2,33,33,1,34,34,0,35,35,2
        db 40,40,2,41,41,1,6,6,1,7,7,2
        db 12,12,1,13,13,2,20,20,3,27,27,3
        db 34,34,2,35,35,1,40,40,2,41,41,1
        db 6,7,1,19,19,3,20,20,0,21,21,3
        db 26,26,3,27,27,0,28,28,3,38,39,3
        db 40,41,1,6,6,2,7,7,0,19,19,3
        db 20,20,0,21,21,3,26,26,3,27,27,0
        db 28,28,3,37,39,3,40,40,1,41,41,2
        db 6,6,3,7,7,0,8,8,3,19,19,3
        db 20,20,0,21,21,3,26,26,3,27,27,0
        db 28,28,3,37,38,3,39,39,2,40,40,0
        db 41,41,3,7,7,1,8,8,2,19,19,3
        db 20,20,0,21,21,3,26,26,3,27,27,0
        db 28,28,3,37,38,3,39,39,1,40,40,0
        db 7,8,1,19,19,3,20,20,1,21,21,3
        db 26,26,3,27,27,0,28,28,3,36,38,3
        db 39,39,0,40,40,1,7,7,3,8,8,0
        db 9,9,3,20,20,3,27,27,3,36,37,3
        db 38,38,2,39,39,0,40,40,2,8,8,1
        db 9,9,2,36,37,3,38,38,1,39,39,0
        db 4,7,2,8,9,0,10,13,2,14,16,3
        db 22,22,3,23,24,2,25,25,3,30,33,3
        db 34,36,2,37,37,1,38,39,0,40,43,2
        db 4,8,1,9,10,0,11,13,1,14,16,0
        db 17,17,2,21,21,2,22,25,0,26,26,3
        db 29,29,3,30,30,1,31,32,0,33,36,1
        db 37,38,0,39,42,1,43,43,2,9,10,1
        db 11,17,3,21,21,3,22,25,0,26,26,3
        db 30,34,3,35,36,2,37,38,1,9,9,2
        db 10,10,0,11,12,3,22,22,3,23,24,1
        db 25,25,2,32,34,3,35,36,2,37,37,0
        db 38,38,2,4,6,3,7,9,2,10,10,0
        db 11,17,1,18,18,2,28,28,3,29,35,1
        db 36,37,0,38,40,2,41,43,3,3,3,3
        db 4,10,1,11,11,0,12,15,1,16,18,2
        db 29,30,2,31,35,1,36,36,0,37,43,1
        db 4,6,3,11,12,1,13,13,3,30,32,3
        db 33,34,2,35,35,0,36,36,1,41,43,3
        db 11,11,3,12,12,0,13,13,1,14,14,2
        db 15,17,1,28,28,3,29,29,2,30,34,1
        db 35,35,0,36,36,3,8,8,3,9,9,2
        db 10,10,1,11,14,0,15,16,1,17,17,2
        db 26,28,3,29,29,2,30,31,1,32,35,0
        db 36,37,1,38,38,2,39,39,3,5,5,3
        db 6,7,1,8,8,0,9,9,1,10,11,2
        db 12,12,3,13,13,2,14,14,0,15,15,1
        db 16,16,3,26,27,3,28,31,2,32,32,1
        db 33,33,0,34,34,2,35,35,3,36,37,2
        db 38,38,1,39,39,0,40,40,1,41,41,2
        db 42,42,3,5,6,1,7,7,2,8,8,3
        db 14,14,3,15,15,1,16,16,0,17,17,1
        db 18,19,2,20,25,3,26,28,2,29,30,1
        db 31,31,0,32,32,1,33,33,3,39,39,3
        db 40,40,2,41,41,1,42,42,2,16,16,2
        db 17,20,0,21,21,2,22,24,3,25,25,2
        db 26,26,1,27,30,0,31,31,2,35,35,2
        db 36,36,1,37,37,0,38,38,1,39,39,2
        db 16,16,2,17,17,1,18,21,3,23,25,3
        db 26,28,2,29,29,1,30,30,0,31,31,2
        db 34,34,2,35,35,0,36,36,2,37,37,3
        db 38,38,1,39,39,0,40,40,2,15,15,2
        db 16,16,0,17,17,3,27,28,3,29,29,2
        db 30,30,1,31,31,0,32,32,2,34,34,0
        db 35,35,1,38,38,3,39,40,1,14,14,3
        db 15,15,0,16,16,2,27,28,3,29,30,2
        db 31,31,1,32,32,0,33,33,3,34,34,0
        db 35,35,2,38,38,3,39,39,1,40,40,0
        db 41,41,3,14,15,1,29,29,3,30,31,2
        db 32,32,0,33,35,1,36,37,3,38,38,2
        db 39,39,0,40,40,1,13,13,3,14,14,0
        db 15,15,3,30,30,3,31,31,2,32,32,1
        db 33,33,0,34,34,1,35,35,0,36,37,1
        db 38,39,0,40,40,2,5,6,3,13,13,2
        db 14,14,1,28,28,3,31,31,3,32,32,2
        db 33,34,0,35,38,1,39,39,3,41,42,3
        db 3,3,2,4,6,0,7,7,1,8,8,2
        db 13,13,2,14,14,1,19,20,3,28,28,3
        db 31,31,3,32,32,2,33,33,0,34,34,2
        db 39,39,3,40,40,1,41,43,0,44,44,2
        db 2,2,3,3,3,0,4,4,2,5,6,3
        db 7,7,1,8,8,0,9,9,1,10,10,2
        db 11,12,3,13,13,2,14,14,0,19,19,2
        db 20,20,3,27,28,3,31,31,3,32,32,2
        db 33,33,0,34,34,2,35,36,3,37,37,2
        db 38,38,1,39,39,0,40,40,1,41,43,2
        db 44,44,0,45,45,3,2,2,3,3,3,0
        db 4,4,2,8,8,3,9,9,1,10,11,0
        db 12,12,1,13,14,0,15,15,3,19,20,2
        db 21,21,3,27,27,2,28,28,3,31,31,3
        db 32,32,1,33,33,0,34,35,1,36,37,0
        db 38,38,1,39,39,2,41,42,3,43,43,2
        db 44,44,0,45,45,3,3,3,2,4,4,0
        db 5,5,2,6,6,3,11,13,3,14,14,0
        db 15,15,2,19,19,2,20,20,0,21,21,2
        db 22,22,3,26,26,2,27,27,0,28,28,3
        db 31,31,3,32,33,1,34,36,3,40,40,3
        db 41,42,2,43,43,0,44,44,1,4,4,1
        db 5,5,0,6,6,2,7,7,3,14,14,2
        db 15,15,0,19,19,3,20,20,1,21,21,0
        db 22,23,3,26,26,0,27,27,2,30,30,3
        db 31,31,2,32,32,0,33,33,2,40,40,3
        db 41,41,1,42,42,0,43,43,1,5,5,1
        db 6,6,0,7,7,1,8,9,3,14,14,3
        db 15,15,0,16,16,3,19,19,3,20,20,2
        db 21,21,0,22,22,2,23,23,3,25,25,2
        db 26,26,0,27,27,3,29,29,3,30,31,2
        db 32,32,0,33,33,2,36,38,3,39,39,2
        db 40,40,1,41,41,0,42,42,1,6,6,2
        db 7,8,0,9,9,2,10,10,3,15,16,1
        db 20,20,2,21,21,0,22,23,1,24,24,2
        db 25,26,1,28,28,3,29,30,2,31,31,1
        db 32,32,0,33,36,3,37,37,2,38,38,1
        db 39,40,0,41,41,2,8,8,2,9,9,1
        db 10,10,0,11,11,1,12,12,2,13,14,3
        db 15,15,2,16,16,0,17,17,2,20,20,2
        db 21,22,0,23,24,1,25,26,0,27,28,2
        db 29,30,1,31,31,0,32,32,1,33,35,2
        db 36,36,1,37,38,0,39,39,1,40,40,3
        db 9,9,3,10,10,2,11,11,1,12,17,0
        db 18,19,1,20,20,0,21,21,1,22,22,3
        db 25,25,3,26,26,1,27,35,0,36,36,1
        db 37,37,2,38,38,3,12,13,3,14,19,2
        db 20,20,3,27,27,3,28,29,2,30,30,3
        db 31,33,2,34,35,3

; BitBlt source is page-local VRAM scratch.  Valid flags avoid rebuilding it
; after the first visit to scene 4 on each physical display page.
n4_blit_asset_valid       db 0,0
n4_blit_batch_active      db 0
n4_blit_b0_bank_cache     db 0FFh
n4_blit_dst_shift         db 0
align 2
n4_blit_dst_x             dw 0
n4_blit_dst_y             dw 0
n4_blit_dst_aperture      dw 0
n4_blit_shift_word        dw 0
n4_blit_tmp_x0            dw 0
n4_blit_tmp_x1            dw 0
n4_blit_tmp_y             dw 0
n4_blit_start_mask        dw 0
n4_blit_end_mask          dw 0
n4_blit_packed_chunk      dw 0
n4_raster_base_x          dw 0
n4_raster_base_y          dw 0
n4_raster_dst_row_aperture dw 0
n4_raster_phase_bias      db 0
n4_raster_phase_work      db 0
align 2
n4_story_raster_x         dw 0
n4_story_raster_y         dw 0
align 4
n4_blit_packed_dst        dd 0
n4_raster_packed_row      dd 0
n4_blit_phase_seed:
        db 0,29,61,97,131,163,199,229
n4_blit_ybias:
        dw -92,-60,-28,4,36,68,100,-4
