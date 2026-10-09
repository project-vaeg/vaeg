; Copyright (c) 2026 Nakata Maho
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
; 1. Redistributions of source code must retain the above copyright notice,
;    this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright notice,
;    this list of conditions and the following disclaimer in the documentation
;    and/or other materials provided with the distribution.
;
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
; WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
; MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
; EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
; SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
; WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
; OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
; ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

bits 16
cpu 286
org 0x100

; The VA2 CPU does not implement the 386 near-Jcc opcodes (0F 80h-8Fh).
; Keep the conditional half short and use a plain near JMP for distant exits.
%macro long_jc 1
    jnc short %%done
    jmp near %1
%%done:
%endmacro

%macro long_jnz 1
    jz short %%done
    jmp near %1
%%done:
%endmacro

%macro long_jz 1
    jnz short %%done
    jmp near %1
%%done:
%endmacro

%macro long_jne 1
    je short %%done
    jmp near %1
%%done:
%endmacro

%macro long_ja 1
    jbe short %%done
    jmp near %1
%%done:
%endmacro

%define KEYBOARD_BIOS_INT       0x82
%define VIDEO_BIOS_INT          0x8f
%define VIDEO_BIOS_DATA_SEG     0x0338
%define VIDEO_MODE_OFFSET       0x000f
%define VIDEO_G0_BPP_OFFSET     0x0011
%define VIDEO_G1_BPP_OFFSET     0x0012

%define PORT_MEMORY_MAP         0x0153
%define PORT_BMS_SELECTOR       0x01d0
%define PORT_G0_TRANSPARENCY    0x0124
%define PORT_G1_TRANSPARENCY    0x0126
%define PORT_PALETTE_COMPOSE    0x0106
%define PORT_RGB_COMPOSE        0x0108
%define PORT_FB1_FBW            0x0224
%define PORT_FB1_DOT            0x0228
%define PORT_FB1_OFX            0x022a
%define PORT_FB1_OFY            0x022c
%define PORT_FB1_DSA_LOW        0x022e
%define PORT_FB1_DSA_HIGH       0x0230
%define PORT_FB1_DSH            0x0232
%define PORT_FB1_DSP            0x0236
%define PORT_TSP_STATUS         0x0142
%define PORT_SGP_COMMAND        0x0500
%define PORT_SGP_CONTROL        0x0504
%define PORT_SGP_STATUS         0x0506
%define PORT_GVRAM_WRITE_MODE   0x0580

%define MEMORY_MAP_GVRAM_SINGLE 0x54
%define GVRAM_CPU_WRITE_MODE    0x10
%define MODE_320X200_G0_G1      0xe00e
%define PIXEL_SIZE_G0_G1_8BPP   0x0808
%define COMPOSE_G1_OVER_G0      0x0034
%define RGB_COMPOSE_G1_OVER_G0  0x0089
%define TSP_STATUS_VBLANK       0x40

%define G0_SEGMENT              0xa000
%define PAYLOAD_SEGMENT         0x3000
%define SCREEN_WIDTH            320
%define SCREEN_HEIGHT           200
%define SCREEN_PITCH            320
%define G1_BACKING_HEIGHT       400
%define G1_PAGE_A_SGP_BASE      0x220000
%define G1_PAGE_A_DSA           0x020000
%define G1_PAGE_B_SGP_BASE      0x22fa00
%define G1_PAGE_B_DSA           0x02fa00
%define G1_PAGE_BYTES           0xfa00
%define G1_PAGE_WORD_COUNT      0x7d00
%define G1_BACKING_BYTES        0x1f400
%define ORBIT_PUBLICATIONS_PER_REVOLUTION 64
%define QA_CYCLES               M98T_QA_CYCLES
%define QA_PUBLICATIONS         (ORBIT_PUBLICATIONS_PER_REVOLUTION * QA_CYCLES)
%define TARGET_ANCHOR_X         160
%define TARGET_ANCHOR_Y         100
%define FIXED_SCALE_ID          15
%ifndef M98Y_PRIVATE_PROFILE
%define M98Y_PRIVATE_PROFILE     0
%endif
%define ORBIT_RADIUS_X          96
%if M98Y_PRIVATE_PROFILE
%define ORBIT_RADIUS_Y          16
%else
%define ORBIT_RADIUS_Y          48
%endif
%define ORBIT_ENTRY_BYTES       8
%define HUD_X                   4
%define HUD_Y                   4
%define HUD_X1                  70
%define HUD_Y1                  20
%define HUD_WIDTH               66
%define HUD_HEIGHT              16
%define HUD_FPS_X               34
%define HUD_FPS_Y               4
%define HUD_FPS_WIDTH           18
%define HUD_FPS_HEIGHT          8
%define HUD_COUNT_X            58
%define HUD_COUNT_Y            12
%define HUD_COUNT_WIDTH        12
%define HUD_COUNT_HEIGHT       8
%define HUD_FULL_WRITE_BYTES    1056
%define HUD_FPS_WRITE_BYTES     144
%define HUD_COUNT_WRITE_BYTES   96
; The status panel is G0-only and deliberately outside the accepted
; FPS/count rectangles.  The generated tiles are fixed-width, so every
; update also erases stale sign/decimal characters.
%define HUD_STATUS_SPEED_X      4
%define HUD_STATUS_DISTANCE_X   64
%define HUD_STATUS_LOOK_X       106
%define HUD_STATUS_RADIUS_X     154
%define HUD_STATUS_Y            24
%define HUD_STATUS_SECOND_Y     24
%define STATUS_SPEED_WIDTH      60
%define STATUS_DISTANCE_WIDTH   42
%define STATUS_LOOK_WIDTH       42
%define STATUS_RADIUS_WIDTH     54
%define STATUS_FIELD_HEIGHT     8
%if M98Y_PRIVATE_PROFILE
%define ORBIT_RADIUS_ADJUSTMENTS 1
%else
%define ORBIT_RADIUS_ADJUSTMENTS 0
%endif

%ifndef M98Q_BOUNDED_QA
%define M98Q_BOUNDED_QA         0
%endif
%ifndef M98Q_INITIAL_VISIBLE_PAGE
%define M98Q_INITIAL_VISIBLE_PAGE 0
%endif
%ifndef M98W_CLEAR_MODE
%define M98W_CLEAR_MODE         1
%endif
%ifndef M98Q_CLEAR_MODE
%define M98Q_CLEAR_MODE         M98W_CLEAR_MODE
%endif
%ifndef M98T_BOUNDED_QA
%define M98T_BOUNDED_QA         0
%endif
%ifndef M98T_QA_CYCLES
%define M98T_QA_CYCLES          1
%endif
%ifndef M98T_QA_SCENARIO
%define M98T_QA_SCENARIO        0
%endif
%ifndef M98V_ACTIVE_COUNT
%define M98V_ACTIVE_COUNT       4
%endif
%ifndef M98X_RUNTIME_MODE
%define M98X_RUNTIME_MODE       0
%endif
%ifndef M98AA_AUTO_CAMERA
%define M98AA_AUTO_CAMERA       0
%endif
%include "zundamon_multi_instance_contract.inc"

%define CLEAR_MODE_FULL         0
%define CLEAR_MODE_DIRTY        1

%define PAGE_A                  0
%define PAGE_B                  1
%define PAGE_UNINITIALIZED      0
%define PAGE_HIDDEN_CLEAN       1
%define PAGE_HIDDEN_RENDERING   2
%define PAGE_HIDDEN_COMPLETE    3
%define PAGE_VISIBLE            4
%define PAGE_HIDDEN_STALE       5

%define RENDER_IDLE             0
%define RENDER_RENDERING        1
%define RENDER_READY            2

%define KEY_SCAN_ESCAPE         0x00
; INT 82h/AH=09h returns the scan code in AH and internal code in AL.
; Recognize the complete ESC result so a queued command Return is ignored.
%define KEY_INTERNAL_ESCAPE     0x1b
%define KEY_SCAN_SPACE          0x34
%define KEY_SCAN_LEFT           0x3b
%define KEY_SCAN_RIGHT          0x3c
; PC-88 cursor make codes are 3Ah/3Dh.  VA2's compatibility path can also
; expose the AT-style 48h/50h values, so accept both without changing the
; one-press BIOS event policy.
%define KEY_SCAN_UP             0x3a
%define KEY_SCAN_DOWN           0x3d
%define KEY_SCAN_UP_EXTENDED    0x48
%define KEY_SCAN_DOWN_EXTENDED  0x50
%define KEY_SCAN_Q              0x10
%define KEY_SCAN_W              0x11
%define KEY_SCAN_E              0x12
%define KEY_SCAN_O              0x18
%define KEY_SCAN_P              0x19
%define KEY_SCAN_A              0x1d
%define KEY_SCAN_S              0x1e
%define KEY_SCAN_Z              0x29

%define CADENCE_MIN             1
%define CADENCE_MAX             8
%define SPEED_MIN               0
%define SPEED_MAX               12
%if M98Y_PRIVATE_PROFILE
; The private IDA64 candidate uses the requested 0.25X steps from 1.00X
; through 4.00X.  The public profile retains the accepted M98z ladder.
%define SPEED_DEFAULT           0
%else
%define SPEED_DEFAULT           3
%endif
%define DISTANCE_MIN            -4
%define DISTANCE_MAX            4
%define LOOK_MIN                -4
%define LOOK_MAX                4
%define RADIUS_MIN              0
%define RADIUS_MAX              8
%define RADIUS_DEFAULT          4

%define BMS_WINDOW_SEGMENT      0x8000
%define BMS_WINDOW_SGP_BASE     0x080000
%define BMS_BANK_SIZE_LOW       0x0000
%define BMS_BANK_SIZE_HIGH      0x0002
%define BMS_FIRST_SELECTOR      1
%define BMS_SECOND_SELECTOR     2
%define BMS_LAST_SELECTOR       128
%define BMS_INVALID_SELECTOR    129
%define BMS_PROBE_OFFSET        0xffe0
%define NORMAL_OUTSIDE_SEGMENT  0x7ffe
%define NORMAL_OUTSIDE_OFFSET   0x0000
%define NORMAL_UNDER_SEGMENT    0x8001
%define NORMAL_UNDER_OFFSET     0x0000
%define GUARD_BYTES             8

%define ATLAS_METADATA_BYTES    1024
%define ATLAS_DESCRIPTOR_BYTES  32
%define ATLAS_SCALE_COUNT       30
%define ATLAS_SELECTED_DESC     (64 + (ATLAS_SCALE_COUNT - 1) * ATLAS_DESCRIPTOR_BYTES)
%define STAGING_BYTES           4096
%define STAGING_POISON          0xa5

%define SGP_COMMAND_END         0x0001
%define SGP_COMMAND_SET_WORK    0x0003
%define SGP_COMMAND_SET_SOURCE  0x0004
%define SGP_COMMAND_SET_DEST    0x0005
%define SGP_COMMAND_SET_COLOR   0x0006
%define SGP_COMMAND_BITBLT      0x0007
%define SGP_COMMAND_CLS         0x000a
%define SGP_BITBLT_COPY_XPAR    0x0105
%define SGP_BUSY                0x01
%define SGP_COMMAND_LIST_WORDS  64
%define DIRTY_ROW_COMMAND_WORDS 5
%define DIRTY_BATCH_FIXED_WORDS 6
%define DIRTY_ROWS_PER_BATCH    ((SGP_COMMAND_LIST_WORDS - DIRTY_BATCH_FIXED_WORDS) / DIRTY_ROW_COMMAND_WORDS)
%define DIRTY_INTERVAL_BYTES    6
%define DIRTY_INTERVALS_PER_BATCH DIRTY_ROWS_PER_BATCH
%define FOOTPRINT_RECT_BYTES    8
%if M98Y_PRIVATE_PROFILE
%define FOOTPRINT_CAPACITY      64
%define FOOTPRINT_PAGE_SHIFT    9
%define FOOTPRINT_ID_PAGE_SHIFT 6
%define DRAW_ORDER_SEEN_WORDS   4
%define COUNT_TRANSITION_SHIFT  7
%else
%define FOOTPRINT_CAPACITY      16
%define FOOTPRINT_PAGE_SHIFT    7
%define FOOTPRINT_ID_PAGE_SHIFT 4
%define DRAW_ORDER_SEEN_WORDS   1
%define COUNT_TRANSITION_SHIFT  5
%endif
; The SGP destination descriptor encodes the destination x-word selector in
; four bits.  This is unrelated to the capacity-dependent page-footprint
; indexing above and must remain stable for both public and private profiles.
%define SGP_DEST_PAGE_SHIFT     4
%define FOOTPRINT_TOTAL_BYTES   (2 * FOOTPRINT_CAPACITY * FOOTPRINT_RECT_BYTES)

; The accepted M98u include remains a 1..16 reference contract.  Runtime
; private builds widen only the statically allocated storage and parser bound;
; the record ABI and renderer are shared with the public binary.
%define INSTANCE_RECORD_CAPACITY_BYTES (M98U_INSTANCE_RECORD_BYTES * FOOTPRINT_CAPACITY)
%define DRAW_ORDER_CAPACITY_BYTES      FOOTPRINT_CAPACITY

%define PROBE_CHECKPOINT_IP     0x4000
%define LOAD_CHECKPOINT_IP      0x4010
%define TRANSFER_CHECKPOINT_IP  0x4020
%define FLIP_CHECKPOINT_IP      0x4030
%define SETTLED_CHECKPOINT_IP   0x4040
%define REPORT_A_CHECKPOINT_IP  0x4050
%define REPORT_B_CHECKPOINT_IP  0x4060
%define REPORT_C_CHECKPOINT_IP  0x4070
%define REPORT_D_CHECKPOINT_IP  0x4080
%define REPORT_E_CHECKPOINT_IP  0x4090
%define REPORT_F_CHECKPOINT_IP  0x40a0
%define REPORT_G_CHECKPOINT_IP  0x40b0
%define REPORT_H_CHECKPOINT_IP  0x40c0
%define REPORT_I_CHECKPOINT_IP  0x40d0
%define REPORT_J_CHECKPOINT_IP  0x40e0
%define REPORT_K_CHECKPOINT_IP  0x40f0
%define REPORT_L_CHECKPOINT_IP  0x4100
%define REPORT_M_CHECKPOINT_IP  0x4110
%define REPORT_N_CHECKPOINT_IP  0x4120
%define REPORT_O_CHECKPOINT_IP  0x4130
%define REPORT_P_CHECKPOINT_IP  0x4140
%define REPORT_Q_CHECKPOINT_IP  0x4150
%define REPORT_R_CHECKPOINT_IP  0x4160
%define REPORT_S_CHECKPOINT_IP  0x4170
%define PROBE_CHECKPOINT_OFFSET (PROBE_CHECKPOINT_IP - 0x0100)
%define LOAD_CHECKPOINT_OFFSET  (LOAD_CHECKPOINT_IP - 0x0100)
%define TRANSFER_CHECKPOINT_OFFSET (TRANSFER_CHECKPOINT_IP - 0x0100)
%define FLIP_CHECKPOINT_OFFSET  (FLIP_CHECKPOINT_IP - 0x0100)
%define SETTLED_CHECKPOINT_OFFSET (SETTLED_CHECKPOINT_IP - 0x0100)
%define REPORT_A_CHECKPOINT_OFFSET (REPORT_A_CHECKPOINT_IP - 0x0100)
%define REPORT_B_CHECKPOINT_OFFSET (REPORT_B_CHECKPOINT_IP - 0x0100)
%define REPORT_C_CHECKPOINT_OFFSET (REPORT_C_CHECKPOINT_IP - 0x0100)
%define REPORT_D_CHECKPOINT_OFFSET (REPORT_D_CHECKPOINT_IP - 0x0100)
%define REPORT_E_CHECKPOINT_OFFSET (REPORT_E_CHECKPOINT_IP - 0x0100)
%define REPORT_F_CHECKPOINT_OFFSET (REPORT_F_CHECKPOINT_IP - 0x0100)
%define REPORT_G_CHECKPOINT_OFFSET (REPORT_G_CHECKPOINT_IP - 0x0100)
%define REPORT_H_CHECKPOINT_OFFSET (REPORT_H_CHECKPOINT_IP - 0x0100)
%define REPORT_I_CHECKPOINT_OFFSET (REPORT_I_CHECKPOINT_IP - 0x0100)
%define REPORT_J_CHECKPOINT_OFFSET (REPORT_J_CHECKPOINT_IP - 0x0100)
%define REPORT_K_CHECKPOINT_OFFSET (REPORT_K_CHECKPOINT_IP - 0x0100)
%define REPORT_L_CHECKPOINT_OFFSET (REPORT_L_CHECKPOINT_IP - 0x0100)
%define REPORT_M_CHECKPOINT_OFFSET (REPORT_M_CHECKPOINT_IP - 0x0100)
%define REPORT_N_CHECKPOINT_OFFSET (REPORT_N_CHECKPOINT_IP - 0x0100)
%define REPORT_O_CHECKPOINT_OFFSET (REPORT_O_CHECKPOINT_IP - 0x0100)
%define REPORT_P_CHECKPOINT_OFFSET (REPORT_P_CHECKPOINT_IP - 0x0100)
%define REPORT_Q_CHECKPOINT_OFFSET (REPORT_Q_CHECKPOINT_IP - 0x0100)
%define REPORT_R_CHECKPOINT_OFFSET (REPORT_R_CHECKPOINT_IP - 0x0100)
%define REPORT_S_CHECKPOINT_OFFSET (REPORT_S_CHECKPOINT_IP - 0x0100)

%if G1_PAGE_BYTES != SCREEN_WIDTH * SCREEN_HEIGHT
%error "M98q G1 page size does not match the logical viewport"
%endif
%if G1_PAGE_WORD_COUNT * 2 != G1_PAGE_BYTES
%error "M98q G1 word count is inconsistent"
%endif
%if G1_PAGE_B_SGP_BASE - G1_PAGE_A_SGP_BASE != G1_PAGE_BYTES
%error "M98q SGP pages are not adjacent"
%endif
%if G1_PAGE_B_DSA - G1_PAGE_A_DSA != G1_PAGE_BYTES
%error "M98q DSA pages are not adjacent"
%endif
%if G1_BACKING_BYTES != G1_PAGE_BYTES * 2
%error "M98q G1 backing surface is not two pages"
%endif
%if M98Q_INITIAL_VISIBLE_PAGE != PAGE_A && M98Q_INITIAL_VISIBLE_PAGE != PAGE_B
%error "M98q initial visible page must be page A or page B"
%endif
%if M98Q_BOUNDED_QA != 0 && M98Q_BOUNDED_QA != 1
%error "M98q bounded QA flag must be zero or one"
%endif
%if M98T_BOUNDED_QA != 0 && M98T_BOUNDED_QA != 1
%error "M98t bounded QA flag must be zero or one"
%endif
%if M98T_QA_CYCLES < 1 || M98T_QA_CYCLES > 2
%error "M98t bounded QA revolutions must be one or two"
%endif
%if M98T_QA_SCENARIO < 0 || M98T_QA_SCENARIO > 3
%error "M98t QA scenario must be static, ladder, pause, or missed-slot"
%endif
%if M98Q_CLEAR_MODE != CLEAR_MODE_FULL && M98Q_CLEAR_MODE != CLEAR_MODE_DIRTY
%error "M98q clear mode must be full or dirty"
%endif
%if M98W_CLEAR_MODE != CLEAR_MODE_FULL && M98W_CLEAR_MODE != CLEAR_MODE_DIRTY
%error "M98w clear mode must be full or dirty"
%endif
%if DIRTY_ROWS_PER_BATCH < 1
%error "M98q command list cannot hold one dirty row"
%endif
%if DIRTY_BATCH_FIXED_WORDS + DIRTY_ROWS_PER_BATCH * DIRTY_ROW_COMMAND_WORDS > SGP_COMMAND_LIST_WORDS
%error "M98q dirty-row batch exceeds the command-list capacity"
%endif
%if TARGET_ANCHOR_X >= SCREEN_WIDTH || TARGET_ANCHOR_Y >= SCREEN_HEIGHT
%error "M98q target anchor is outside the logical viewport"
%endif
%if !M98X_RUNTIME_MODE && M98V_ACTIVE_COUNT != 1 && M98V_ACTIVE_COUNT != 2 && M98V_ACTIVE_COUNT != 4 && M98V_ACTIVE_COUNT != 8 && M98V_ACTIVE_COUNT != 16
%error "M98v active count must be 1, 2, 4, 8, or 16"
%endif
%if M98X_RUNTIME_MODE != 0 && M98X_RUNTIME_MODE != 1
%error "M98x runtime mode must be zero or one"
%endif
%if M98X_RUNTIME_MODE
%define M98V_COUNT_TILE_INDEX 0
%else
%if M98V_ACTIVE_COUNT = 1
%define M98V_COUNT_TILE_INDEX 0
%elif M98V_ACTIVE_COUNT = 2
%define M98V_COUNT_TILE_INDEX 1
%elif M98V_ACTIVE_COUNT = 4
%define M98V_COUNT_TILE_INDEX 2
%elif M98V_ACTIVE_COUNT = 8
%define M98V_COUNT_TILE_INDEX 3
%else
%define M98V_COUNT_TILE_INDEX 4
%endif
%endif

start:
    ; PC-Engine may choose a different COM load segment.  Relocate the guest
    ; to the established private-RAM segment so debug checkpoints are stable.
    mov ax, cs
    mov [cs:psp_segment], ax
    cmp ax, PAYLOAD_SEGMENT
    je relocated_start
    pushf
    pop bp
    cli
    push cs
    pop ds
    mov ax, PAYLOAD_SEGMENT
    mov es, ax
    mov si, 0x0100
    mov di, 0x0100
    mov cx, program_end - $$
    cld
    rep movsb
    push bp
    popf
    push word PAYLOAD_SEGMENT
    push word relocated_start
    retf

relocated_start:
    push cs
    pop ds
    cld
    call parse_cadence_option
    jc cadence_option_failed
    call initialize_count_state
    jc cadence_option_failed
    mov ah, 0x0f
    mov al, 1
    int KEYBOARD_BIOS_INT
    mov byte [keyboard_repeat_disabled], 1
    call save_video_state
    mov dx, message_start
    call print_string

    call probe_bms_mapping
    long_jc bms_probe_failed
    mov byte [probe_result], 0
    mov bx, PORT_BMS_SELECTOR
    mov cx, BMS_LAST_SELECTOR
    mov dx, BMS_BANK_SIZE_HIGH
    mov si, 0xa55a
    mov di, BMS_INVALID_SELECTOR
    xor bp, bp
    mov ax, 0x98a1
    jmp probe_checkpoint

probe_resume:
    cmp byte [probe_result], 0
    je .probe_passed
    mov word [exit_message], message_bms_failed
    jmp fatal_exit
.probe_passed:
    call load_atlas_to_bms
    long_jc atlas_load_failed
    mov bx, [atlas_chunk_count]
    mov cx, [atlas_payload_bytes]
    mov dx, [atlas_payload_bytes + 2]
    mov si, STAGING_BYTES
    mov di, [atlas_file_size]
    mov bp, [atlas_file_size + 2]
    mov ax, 0x98b1
    jmp load_checkpoint

load_resume:
    mov al, FIXED_SCALE_ID
    call select_scale_descriptor
    jc descriptor_failed
    call validate_orbit_table
    jc descriptor_failed
    call validate_all_multi_instance_states
    jc descriptor_failed
    call initialize_video_double_buffer
    jc transfer_failed
    mov bx, G1_PAGE_BYTES
    mov cx, SCREEN_PITCH
    mov dx, SGP_BITBLT_COPY_XPAR
    mov si, [selected_source_low]
    mov di, [selected_source_high]
    mov bp, 0x0201
    mov ax, 0x98c1
    jmp transfer_checkpoint

transfer_resume:
    mov al, FIXED_SCALE_ID
    call select_scale_descriptor
    jc descriptor_failed
    call validate_orbit_table
    jc descriptor_failed
    mov byte [orbit_phase_next], 0
    call initialize_cadence_scheduler
render_loop:
    ; Latch the latest control request only at the start of a new hidden
    ; transaction.  Once generation/clear begins, build_active_count remains
    ; immutable until publication.
    xor ax, ax
    mov al, [requested_count]
    cmp ax, 1
    jb descriptor_failed
    cmp ax, FOOTPRINT_CAPACITY
    ja descriptor_failed
    mov [next_render_count], al
    mov [pending_render_count], al
    mov [build_active_count], ax
    mov ax, [count_request_generation]
    mov [pending_count_generation], ax
    mov byte [count_change_pending], 0
    call apply_requested_projection_state
    jc descriptor_failed
    mov al, [orbit_phase_next]
    call generate_multi_instance_frame
    jc descriptor_failed
    mov byte [hidden_render_state], RENDER_RENDERING
    call render_hidden_page_to_ready
    jc runtime_failed
    call qa_force_missed_slots_before_ready
    jc runtime_failed
    mov byte [hidden_render_state], RENDER_READY
.wait_slot:
    call wait_scheduler_edge
    jc runtime_failed
    cmp byte [exit_requested], 0
    jne normal_exit
    cmp byte [eligible_publication], 0
    je .wait_slot
    mov byte [eligible_publication], 0
    call publish_ready_hidden_page
    jc runtime_failed
    mov bx, [page_flips]
    xor cx, cx
    mov cl, [last_published_global_phase]
    xor dx, dx
    mov dl, [published_active_count]
    mov al, [active_divisor]
    mov dh, al
    mov si, [instances_completed]
    mov di, [vblank_edges_total]
    mov bp, [requested_slots]
    mov ax, 0x98d1
    jmp flip_checkpoint

flip_resume:
    call advance_orbit_phase
%if M98T_BOUNDED_QA
    cmp word [revolution_wraps], QA_CYCLES
    je settled_start
%endif
    jmp render_loop

settled_start:
    mov byte [settled_capture_count], 0
settled_loop:
    call wait_vblank_edge
    jc runtime_failed
    push cs
    pop ds
    push cs
    pop es
    mov bx, [page_flips]
    mov cx, [revolution_wraps]
    mov dx, [phase_advances]
    mov si, [page_flips]
    xor ax, ax
    mov al, [visible_page_index]
    mov di, ax
    mov bp, [last_published_dsa]
    mov ax, 0x98d2
    jmp settled_checkpoint

settled_resume:
%if M98T_BOUNDED_QA
    call poll_escape
    ; Bounded QA does not depend on keyboard input; an injected key is ignored.
%else
    call poll_escape
    jc normal_exit
%endif
    inc byte [settled_capture_count]
    cmp byte [settled_capture_count], 2
    jb settled_loop

%if M98T_BOUNDED_QA
    call validate_m98v_bounded_success
    jc descriptor_failed
    jmp normal_exit
%endif
idle_exit_loop:
    call wait_vblank_edge
    jc runtime_failed
    ; Idle display edges are not measured render/publication edges.
    dec word [vblank_edges_seen]
    call poll_escape
    jnc idle_exit_loop

normal_exit:
    mov word [exit_message], message_done
    mov byte [exit_errorlevel], 0
    jmp common_exit

cadence_option_failed:
    mov dx, message_option_failed
    call print_string
    mov ax, 0x4c02
    int 0x21

bms_probe_failed:
    mov byte [probe_result], 1
    mov bx, [probe_stage]
    xor cx, cx
    xor dx, dx
    xor si, si
    xor di, di
    xor bp, bp
    mov ax, 0x98af
    jmp probe_checkpoint
atlas_load_failed:
    mov word [exit_message], message_atlas_failed
    jmp fatal_exit
transfer_failed:
    mov word [exit_message], message_transfer_failed
    jmp fatal_exit
runtime_failed:
    cmp byte [runtime_failure_kind], 2
    je .vblank
    mov word [exit_message], message_transfer_failed
    jmp fatal_exit
.vblank:
    mov word [exit_message], message_runtime_failed
    jmp fatal_exit
descriptor_failed:
    inc word [descriptor_errors]
    mov word [exit_message], message_descriptor_failed

fatal_exit:
    mov byte [exit_errorlevel], 1
common_exit:
    call close_atlas_if_open
    call restore_bms_and_guards
    call restore_video_state
    cmp byte [keyboard_repeat_disabled], 0
    je .repeat_restored
    mov ah, 0x0f
    xor al, al
    int KEYBOARD_BIOS_INT
    mov byte [keyboard_repeat_disabled], 0
.repeat_restored:
    inc word [cleanup_runs]

    mov bx, [build_active_count]
    mov cx, [complete_frames_started]
    mov dx, [complete_frames_ready]
    mov si, [initial_full_page_clears]
    mov di, [steady_full_page_clears]
    mov bp, [transparent_bitblts]
    mov ax, 0x98e1
    jmp report_a_checkpoint

report_a_resume:
    mov bx, [vblank_edges_seen]
    mov cx, [page_a_publications]
    mov dx, [page_b_publications]
    mov si, [sgp_timeouts]
    mov di, [sgp_errors]
    mov bp, [vblank_timeouts]
    mov ax, 0x98e2
    jmp report_b_checkpoint

report_b_resume:
    mov bx, [bms_bank_switches]
    mov cx, [cleanup_runs]
    xor dx, dx
    mov dl, [visible_page_index]
    mov si, [revolution_wraps]
    mov di, [phase_advances]
    mov bp, [bounds_failures]
    mov ax, 0x98e3
    jmp report_c_checkpoint

report_c_resume:
    mov bx, [source_bytes]
    mov cx, [source_bytes + 2]
    mov dx, [full_page_clear_bytes]
    mov si, [full_page_clear_bytes + 2]
    mov di, [bms_bank_selections]
    mov bp, [last_published_dsa]
    mov ax, 0x98e4
    jmp report_d_checkpoint

report_d_resume:
    mov bx, [instances_planned]
    mov cx, [instances_submitted]
    mov dx, [instances_completed]
    mov si, [instances_published]
    mov di, [draw_order_failures]
    mov bp, [bounded_validation_pass]
    mov ax, 0x98e5
    jmp report_e_checkpoint

report_e_resume:
    mov bx, [partial_publication_attempts]
    mov cx, [tie_break_failures]
    mov dx, [source_failures]
    mov si, [framebuffer_mismatches]
    mov di, [runtime_count_changes]
    mov bp, [guard_failures]
    mov ax, 0x98e6
    jmp report_f_checkpoint

report_f_resume:
    mov bx, [full_page_clear_words]
    mov cx, [full_page_clear_words + 2]
    mov dx, [sgp_command_lists]
    mov si, [sgp_commands]
    mov di, [sgp_batches]
    mov bp, [complete_frames_published]
    mov ax, 0x98e7
    jmp report_g_checkpoint

report_g_resume:
    mov bx, [page_flips]
    mov cx, [requested_slots]
    mov dx, [published_frames]
    mov si, [missed_slots]
    mov di, [ready_wait_edges]
    xor ax, ax
    mov al, [hidden_render_state]
    mov bp, ax
    mov ax, 0x98e8
    jmp report_h_checkpoint

report_h_resume:
    xor bx, bx
    mov bl, [active_divisor]
    mov bh, [requested_divisor]
    mov cx, [divider_change_requests]
    mov dx, [divider_changes_applied]
    mov si, [divider_boundary_resets]
    mov di, [pause_requests]
    mov bp, [pause_transitions_applied]
    mov ax, 0x98e9
    jmp report_i_checkpoint

report_i_resume:
    mov bx, [vblank_edges_total]
    mov cx, [vblank_edges_unpaused]
    mov dx, [vblank_edges_paused]
    mov si, [control_endpoint_hits]
    mov di, [phase_publication_total]
    mov bp, [scale_publication_total]
    mov ax, 0x98ea
    jmp report_j_checkpoint

report_j_resume:
    mov bx, [publication_digest]
    mov cx, [publication_digest + 2]
    mov dx, [revolution_wraps]
    mov si, [phase_advances]
    xor ax, ax
    mov al, [hidden_render_state]
    mov di, ax
    xor ax, ax
    mov al, [divider_count]
    mov bp, ax
    mov ax, 0x98eb
    jmp report_k_checkpoint

report_k_resume:
    mov bx, [table_scale_change_edges]
    mov cx, [near_publications]
    mov dx, [far_publications]
    mov si, ORBIT_RADIUS_X
    mov di, ORBIT_RADIUS_Y
    mov bp, ORBIT_RADIUS_ADJUSTMENTS
    mov ax, 0x98ec
    jmp report_l_checkpoint

report_l_resume:
    mov bx, [hud_full_initializations]
    mov cx, [hud_fps_field_updates]
    mov dx, [hud_count_field_updates]
    mov si, [hud_g1_writes]
    mov di, [hud_vblank_overruns]
    mov bp, [hud_mismatches]
    mov ax, 0x98ed
    jmp report_m_checkpoint

report_m_resume:
    mov bx, [hud_bytes_written]
    mov cx, [hud_bytes_written + 2]
    mov dx, [hud_runtime_failure]
    mov si, [descriptor_errors]
    mov di, [source_failures]
    mov bp, [cleanup_runs]
    mov ax, 0x98ee
    jmp report_n_checkpoint

report_n_resume:
    xor bx, bx
    mov bl, [last_published_global_phase]
    xor cx, cx
    mov cl, [pending_global_phase]
    mov dx, [complete_frames_started]
    mov si, [complete_frames_ready]
    mov di, [complete_frames_published]
    mov bp, [bounded_validation_pass]
    mov ax, 0x98ef
    jmp report_o_checkpoint

report_o_resume:
    mov bx, [dirty_candidate_intervals]
    mov cx, [dirty_candidate_intervals + 2]
    mov dx, [dirty_merged_intervals]
    mov si, [dirty_merged_intervals + 2]
    mov di, [dirty_row_cls_commands]
    mov bp, [dirty_first_use_skips]
    mov ax, 0x98f0
    jmp report_p_checkpoint

report_p_resume:
    mov bx, [dirty_words_cleared]
    mov cx, [dirty_words_cleared + 2]
    mov dx, [dirty_bytes_cleared]
    mov si, [dirty_bytes_cleared + 2]
    mov di, [page_footprint_commits]
    mov bp, [union_recompute_mismatches]
    mov ax, 0x98f1
    jmp report_q_checkpoint

report_q_resume:
    mov bx, [dirty_rows_visited]
    mov cx, [dirty_rows_nonempty]
    mov dx, [dirty_rows_cleared]
    mov si, [dirty_overlap_merges]
    mov di, [dirty_adjacency_merges]
    mov bp, [dirty_containment_merges]
    mov ax, 0x98f2
    jmp report_r_checkpoint

report_r_resume:
    mov bx, [union_validation_passes]
    mov cx, [sgp_clear_batches]
    mov dx, [sgp_draw_batches]
    mov si, [page_footprint_commit_failures]
    mov di, [dirty_frames_with_clear]
    mov bp, [dirty_rect_clears]
    mov ax, 0x98f3
    jmp report_s_checkpoint

report_s_resume:
    mov dx, [exit_message]
    call print_string
    mov al, [exit_errorlevel]
    mov ah, 0x4c
    int 0x21

; G98l-A: selector 0 is ordinary RAM; selectors 1..128 are independent BMS
; banks in the default 16-MiB configuration; selector 129 reads open bus.
probe_bms_mapping:
    mov word [probe_stage], 1
    mov dx, PORT_BMS_SELECTOR
    in al, dx
    mov [initial_bms_selector], al
    test al, al
    long_jnz .early_failed
    cli
    call save_and_install_normal_guards
    long_jc .cleanup
    mov word [probe_stage], 2
    mov byte [ordinary_guards_active], 1

    mov al, BMS_FIRST_SELECTOR
    mov di, saved_bank_1
    mov si, signature_bank_1
    call save_and_write_probe_bank
    long_jc .cleanup
    mov word [probe_stage], 3
    or byte [saved_probe_banks], 0x01
    mov al, BMS_SECOND_SELECTOR
    mov di, saved_bank_2
    mov si, signature_bank_2
    call save_and_write_probe_bank
    long_jc .cleanup
    mov word [probe_stage], 4
    or byte [saved_probe_banks], 0x02
    mov al, BMS_LAST_SELECTOR
    mov di, saved_bank_128
    mov si, signature_bank_128
    call save_and_write_probe_bank
    long_jc .cleanup
    mov word [probe_stage], 5
    or byte [saved_probe_banks], 0x04

    mov al, BMS_FIRST_SELECTOR
    mov si, signature_bank_1
    call verify_probe_bank
    jc .cleanup
    mov word [probe_stage], 6
    mov al, BMS_SECOND_SELECTOR
    mov si, signature_bank_2
    call verify_probe_bank
    jc .cleanup
    mov word [probe_stage], 7
    mov al, BMS_LAST_SELECTOR
    mov si, signature_bank_128
    call verify_probe_bank
    jc .cleanup
    mov word [probe_stage], 8

    mov dx, PORT_BMS_SELECTOR
    mov al, BMS_INVALID_SELECTOR
    out dx, al
    in al, dx
    cmp al, BMS_INVALID_SELECTOR
    jne .cleanup
    mov word [probe_stage], 9
    mov ax, BMS_WINDOW_SEGMENT
    mov es, ax
    mov di, BMS_PROBE_OFFSET
    mov cx, GUARD_BYTES
.open_bus:
    cmp byte [es:di], 0xff
    jne .cleanup
    mov byte [es:di], 0x5a
    inc di
    loop .open_bus
    mov al, BMS_FIRST_SELECTOR
    mov si, signature_bank_1
    call verify_probe_bank
    jc .cleanup
    mov word [probe_stage], 10

    call restore_probe_banks
    call select_ordinary_mapping
    call verify_normal_guards
    jc .cleanup
    sti
    clc
    ret
.cleanup:
    call restore_probe_banks
    call select_ordinary_mapping
    sti
.early_failed:
    stc
    ret

save_and_install_normal_guards:
    mov ax, NORMAL_OUTSIDE_SEGMENT
    mov es, ax
    mov si, NORMAL_OUTSIDE_OFFSET
    mov di, saved_normal_outside
    mov cx, GUARD_BYTES
    call save_es_bytes
    mov ax, NORMAL_UNDER_SEGMENT
    mov es, ax
    mov si, NORMAL_UNDER_OFFSET
    mov di, saved_normal_under
    mov cx, GUARD_BYTES
    call save_es_bytes
    mov ax, NORMAL_OUTSIDE_SEGMENT
    mov es, ax
    mov di, NORMAL_OUTSIDE_OFFSET
    mov si, guard_normal_outside
    mov cx, GUARD_BYTES
    call write_es_bytes
    mov ax, NORMAL_UNDER_SEGMENT
    mov es, ax
    mov di, NORMAL_UNDER_OFFSET
    mov si, guard_normal_under
    mov cx, GUARD_BYTES
    call write_es_bytes
    call verify_normal_guards
    ret

save_es_bytes:
.next:
    mov al, [es:si]
    mov [di], al
    inc si
    inc di
    loop .next
    ret

write_es_bytes:
.next:
    mov al, [si]
    mov [es:di], al
    inc si
    inc di
    loop .next
    ret

compare_es_bytes:
.next:
    mov al, [si]
    cmp al, [es:di]
    jne .failed
    inc si
    inc di
    loop .next
    clc
    ret
.failed:
    stc
    ret

verify_normal_guards:
    mov ax, NORMAL_OUTSIDE_SEGMENT
    mov es, ax
    mov di, NORMAL_OUTSIDE_OFFSET
    mov si, guard_normal_outside
    mov cx, GUARD_BYTES
    call compare_es_bytes
    jc .failed
    mov ax, NORMAL_UNDER_SEGMENT
    mov es, ax
    mov di, NORMAL_UNDER_OFFSET
    mov si, guard_normal_under
    mov cx, GUARD_BYTES
    call compare_es_bytes
    jc .failed
    clc
    ret
.failed:
    stc
    ret

save_and_write_probe_bank:
    ; AL=selector, DI=save buffer, SI=signature buffer.
    mov bl, al
    mov dx, PORT_BMS_SELECTOR
    out dx, al
    in al, dx
    cmp al, bl
    jne .failed
    mov ax, BMS_WINDOW_SEGMENT
    mov es, ax
    push si
    mov si, BMS_PROBE_OFFSET
    mov cx, GUARD_BYTES
    call save_es_bytes
    pop si
    mov di, BMS_PROBE_OFFSET
    mov cx, GUARD_BYTES
    call write_es_bytes
    clc
    ret
.failed:
    stc
    ret

verify_probe_bank:
    ; AL=selector, SI=expected signature.
    mov bl, al
    mov dx, PORT_BMS_SELECTOR
    out dx, al
    in al, dx
    cmp al, bl
    jne .failed
    mov ax, BMS_WINDOW_SEGMENT
    mov es, ax
    mov di, BMS_PROBE_OFFSET
    mov cx, GUARD_BYTES
    call compare_es_bytes
    jc .failed
    clc
    ret
.failed:
    stc
    ret

restore_probe_banks:
    test byte [saved_probe_banks], 0x01
    jz .bank_2
    mov al, BMS_FIRST_SELECTOR
    mov si, saved_bank_1
    call restore_one_probe_bank
.bank_2:
    test byte [saved_probe_banks], 0x02
    jz .bank_128
    mov al, BMS_SECOND_SELECTOR
    mov si, saved_bank_2
    call restore_one_probe_bank
.bank_128:
    test byte [saved_probe_banks], 0x04
    jz .done
    mov al, BMS_LAST_SELECTOR
    mov si, saved_bank_128
    call restore_one_probe_bank
.done:
    mov byte [saved_probe_banks], 0
    ret

restore_one_probe_bank:
    mov dx, PORT_BMS_SELECTOR
    out dx, al
    mov ax, BMS_WINDOW_SEGMENT
    mov es, ax
    mov di, BMS_PROBE_OFFSET
    mov cx, GUARD_BYTES
    call write_es_bytes
    ret

select_ordinary_mapping:
    mov dx, PORT_BMS_SELECTOR
    xor al, al
    out dx, al
    ret

; G98l-B: load the 1024-byte header/descriptor region conventionally, then
; stream the payload in at most 4096-byte chunks directly into BMS selector 1.
load_atlas_to_bms:
    call select_ordinary_mapping
    mov dx, atlas_filename
    mov ax, 0x3d00
    int 0x21
    long_jc .failed
    mov [atlas_handle], ax
    mov bx, ax
    mov dx, atlas_metadata
    mov cx, ATLAS_METADATA_BYTES
    mov ah, 0x3f
    int 0x21
    long_jc .failed
    cmp ax, ATLAS_METADATA_BYTES
    long_jne .failed
    call validate_atlas_metadata
    long_jc .failed

    mov word [file_crc_state], 0xffff
    mov word [file_crc_state + 2], 0xffff
    mov word [payload_crc_state], 0xffff
    mov word [payload_crc_state + 2], 0xffff
    ; The file CRC is defined with its own header field zeroed.
    mov word [atlas_metadata + 56], 0
    mov word [atlas_metadata + 58], 0
    push es
    push ds
    pop es
    mov si, atlas_metadata
    mov cx, ATLAS_METADATA_BYTES
    mov bp, file_crc_state
    call crc32_update_es
    pop es

    mov ax, [atlas_payload_bytes]
    mov [atlas_remaining], ax
    mov ax, [atlas_payload_bytes + 2]
    mov [atlas_remaining + 2], ax
    mov word [atlas_loaded], 0
    mov word [atlas_loaded + 2], 0
    mov word [atlas_chunk_count], 0
.read_chunk:
    mov ax, [atlas_remaining]
    or ax, [atlas_remaining + 2]
    long_jz .end_of_payload
    mov cx, STAGING_BYTES
    cmp word [atlas_remaining + 2], 0
    jne .size_ready
    cmp word [atlas_remaining], STAGING_BYTES
    jae .size_ready
    mov cx, [atlas_remaining]
.size_ready:
    mov [current_chunk_bytes], cx
    call select_ordinary_mapping
    mov bx, [atlas_handle]
    mov dx, staging_buffer
    mov ah, 0x3f
    int 0x21
    long_jc .failed
    cmp ax, [current_chunk_bytes]
    long_jne .failed
    push ds
    pop es
    mov si, staging_buffer
    mov cx, [current_chunk_bytes]
    mov bp, file_crc_state
    call crc32_update_es
    mov si, staging_buffer
    mov cx, [current_chunk_bytes]
    mov bp, payload_crc_state
    call crc32_update_es

    mov dx, PORT_BMS_SELECTOR
    mov al, BMS_FIRST_SELECTOR
    out dx, al
    mov ax, [atlas_loaded]
    mov dx, [atlas_loaded + 2]
    call bms_segment_from_offset
    mov si, staging_buffer
    mov cx, [current_chunk_bytes]
    rep movsb
    call select_ordinary_mapping
    mov ax, [current_chunk_bytes]
    add [atlas_loaded], ax
    adc word [atlas_loaded + 2], 0
    sub [atlas_remaining], ax
    sbb word [atlas_remaining + 2], 0
    inc word [atlas_chunk_count]
    jmp .read_chunk

.end_of_payload:
    mov bx, [atlas_handle]
    mov dx, staging_buffer
    mov cx, 1
    mov ah, 0x3f
    int 0x21
    jc .failed
    test ax, ax
    jnz .failed
    call close_atlas_if_open
    call compare_final_crcs
    jc .failed
    call verify_bms_payload_crc
    jc .failed
    call verify_bms_frame_crc
    jc .failed
    call poison_staging_buffer
    call verify_staging_poison
    jc .failed
    call select_ordinary_mapping
    call verify_normal_guards
    ret
.failed:
    call close_atlas_if_open
    call select_ordinary_mapping
    stc
    ret

validate_atlas_metadata:
    cmp word [atlas_metadata + 0], 0x555a
    long_jne .failed
    cmp word [atlas_metadata + 2], 0x444e
    long_jne .failed
    cmp word [atlas_metadata + 4], 0x524f
    long_jne .failed
    cmp word [atlas_metadata + 6], 0x0042
    long_jne .failed
    cmp word [atlas_metadata + 8], 1
    long_jne .failed
    cmp word [atlas_metadata + 10], 64
    long_jne .failed
    cmp word [atlas_metadata + 12], 0
    long_jne .failed
    cmp word [atlas_metadata + 14], 0
    long_jne .failed
    cmp word [atlas_metadata + 16], 1
    long_jne .failed
    cmp word [atlas_metadata + 18], ATLAS_SCALE_COUNT
    long_jne .failed
    cmp word [atlas_metadata + 20], ATLAS_DESCRIPTOR_BYTES
    long_jne .failed
    cmp word [atlas_metadata + 22], 0
    long_jne .failed
    cmp word [atlas_metadata + 24], BMS_BANK_SIZE_LOW
    long_jne .failed
    cmp word [atlas_metadata + 26], BMS_BANK_SIZE_HIGH
    long_jne .failed
    cmp word [atlas_metadata + 28], 1
    long_jne .failed
    cmp word [atlas_metadata + 30], BMS_FIRST_SELECTOR
    long_jne .failed
    cmp word [atlas_metadata + 32], 64
    long_jne .failed
    cmp word [atlas_metadata + 34], 0
    long_jne .failed
    cmp word [atlas_metadata + 36], 960
    long_jne .failed
    cmp word [atlas_metadata + 38], 0
    long_jne .failed
    cmp word [atlas_metadata + 40], ATLAS_METADATA_BYTES
    long_jne .failed
    cmp word [atlas_metadata + 42], 0
    long_jne .failed
    mov ax, [atlas_metadata + 44]
    mov dx, [atlas_metadata + 46]
%if M98Y_PRIVATE_PROFILE
    mov bx, ax
    or bx, dx
%else
    or ax, dx
%endif
    long_jz .failed
    cmp dx, BMS_BANK_SIZE_HIGH
    long_ja .failed
    jne .payload_size_ok
    test ax, ax
    long_jnz .failed
.payload_size_ok:
    mov [atlas_payload_bytes], ax
    mov [atlas_payload_bytes + 2], dx
    add ax, ATLAS_METADATA_BYTES
    adc dx, 0
    cmp ax, [atlas_metadata + 48]
    long_jne .failed
    cmp dx, [atlas_metadata + 50]
    long_jne .failed
    mov [atlas_file_size], ax
    mov [atlas_file_size + 2], dx
    mov ax, [atlas_metadata + 52]
    mov [expected_payload_crc], ax
    mov ax, [atlas_metadata + 54]
    mov [expected_payload_crc + 2], ax
    mov ax, [atlas_metadata + 56]
    mov [expected_file_crc], ax
    mov ax, [atlas_metadata + 58]
    mov [expected_file_crc + 2], ax
    cmp word [atlas_metadata + 60], 0
    long_jne .failed
    cmp word [atlas_metadata + 62], 0
    long_jne .failed

    mov al, ATLAS_SCALE_COUNT
    call select_scale_descriptor
    jc .failed
    mov ax, [selected_width]
    mov [atlas_source_width], ax
    mov ax, [selected_height]
    mov [atlas_source_height], ax
    mov ax, [selected_anchor_x]
    mov [atlas_source_anchor_x], ax
    mov ax, [selected_anchor_y]
    mov [atlas_source_anchor_y], ax
    mov word [expected_descriptor_offset], 0
    mov word [expected_descriptor_offset + 2], 0
    mov byte [descriptor_validation_id], 1
.descriptor_loop:
    mov al, [descriptor_validation_id]
    call select_scale_descriptor
    jc .failed
    call validate_canonical_scale
    jc .failed
    mov ax, [selected_bank_offset]
    cmp ax, [expected_descriptor_offset]
    jne .failed
    mov dx, [selected_bank_offset + 2]
    cmp dx, [expected_descriptor_offset + 2]
    jne .failed
    add ax, [selected_payload_bytes]
    adc dx, [selected_payload_bytes + 2]
    add ax, 15
    adc dx, 0
    and ax, 0xfff0
    mov [expected_descriptor_offset], ax
    mov [expected_descriptor_offset + 2], dx
    inc byte [descriptor_validation_id]
    cmp byte [descriptor_validation_id], ATLAS_SCALE_COUNT + 1
    jne .descriptor_loop
    mov al, ATLAS_SCALE_COUNT
    call select_scale_descriptor
    jc .failed
    clc
    ret
.failed:
    stc
    ret

validate_canonical_scale:
    xor cx, cx
    mov cl, [selected_scale_id]
    cmp cl, ATLAS_SCALE_COUNT
    jne .numerator_ready
    mov cx, 31
.numerator_ready:
    mov ax, [atlas_source_width]
    mul cx
    add ax, 15
    adc dx, 0
    mov bx, 31
    div bx
    test ax, ax
    jnz .width_ready
    inc ax
.width_ready:
    cmp ax, [selected_width]
    jne .failed
    mov ax, [atlas_source_height]
    mul cx
    add ax, 15
    adc dx, 0
    mov bx, 31
    div bx
    test ax, ax
    jnz .height_ready
    inc ax
.height_ready:
    cmp ax, [selected_height]
    jne .failed

    mov ax, [atlas_source_anchor_x]
    shl ax, 1
    inc ax
    mul word [selected_width]
    mov bx, [atlas_source_width]
    shl bx, 1
    div bx
    mov bx, [selected_width]
    dec bx
    cmp ax, bx
    jbe .anchor_x_ready
    mov ax, bx
.anchor_x_ready:
    cmp ax, [selected_anchor_x]
    jne .failed
    mov ax, [atlas_source_anchor_y]
    shl ax, 1
    inc ax
    mul word [selected_height]
    mov bx, [atlas_source_height]
    shl bx, 1
    div bx
    mov bx, [selected_height]
    dec bx
    cmp ax, bx
    jbe .anchor_y_ready
    mov ax, bx
.anchor_y_ready:
    cmp ax, [selected_anchor_y]
    jne .failed
    clc
    ret
.failed:
    stc
    ret

; AL is the implicit public scale ID, 1 through 30.  The descriptor identity
; is its canonical table position; IDs 0 and 31 therefore cannot be selected.
select_scale_descriptor:
    cmp al, 1
    jb .failed
    cmp al, ATLAS_SCALE_COUNT
    ja .failed
    mov [selected_scale_id], al
    xor ah, ah
    dec ax
    mov bx, ATLAS_DESCRIPTOR_BYTES
    mul bx
    add ax, 64
    mov bx, atlas_metadata
    add bx, ax

    mov ax, [bx + 0]
    test ax, ax
    jz .failed
    cmp ax, SCREEN_WIDTH
    ja .failed
    mov [selected_width], ax
    mov ax, [bx + 2]
    test ax, ax
    jz .failed
    cmp ax, SCREEN_HEIGHT
    ja .failed
    mov [selected_height], ax
    mov ax, [bx + 4]
    mov [selected_pitch], ax
    mov dx, [selected_width]
    add dx, 3
    and dx, 0xfffc
    cmp ax, dx
    jne .failed
    mov ax, [bx + 6]
    cmp ax, [selected_width]
    jae .failed
    mov [selected_anchor_x], ax
    mov ax, [bx + 8]
    cmp ax, [selected_height]
    jae .failed
    mov [selected_anchor_y], ax
    cmp word [bx + 10], 0
    jne .failed
    cmp word [bx + 12], 0
    jne .failed
    cmp word [bx + 14], 0
    jne .failed
    mov ax, [bx + 16]
    mov dx, [bx + 18]
    mov [selected_bank_offset], ax
    mov [selected_bank_offset + 2], dx
    test ax, 0x000f
    jnz .failed
    mov si, [bx + 20]
    mov di, [bx + 22]
    sub si, ATLAS_METADATA_BYTES
    sbb di, 0
    cmp si, ax
    jne .failed
    cmp di, dx
    jne .failed
    mov ax, [bx + 24]
    mov dx, [bx + 26]
    test dx, dx
    jnz .failed
    mov [selected_payload_bytes], ax
    mov [selected_payload_bytes + 2], dx
    mov ax, [selected_pitch]
    mul word [selected_height]
    cmp ax, [selected_payload_bytes]
    jne .failed
    test dx, dx
    jnz .failed
    mov ax, [selected_bank_offset]
    mov dx, [selected_bank_offset + 2]
    add ax, [selected_payload_bytes]
    adc dx, [selected_payload_bytes + 2]
    cmp dx, BMS_BANK_SIZE_HIGH
    ja .failed
    jne .bank_range_ok
    test ax, ax
    jnz .failed
.bank_range_ok:
    cmp dx, [atlas_payload_bytes + 2]
    ja .failed
    jb .payload_range_ok
    cmp ax, [atlas_payload_bytes]
    ja .failed
.payload_range_ok:
    mov ax, [bx + 28]
    mov [expected_frame_crc], ax
    mov ax, [bx + 30]
    mov [expected_frame_crc + 2], ax
    mov ax, TARGET_ANCHOR_X
    sub ax, [selected_anchor_x]
    jc .failed
    mov [selected_dst_x], ax
    add ax, [selected_width]
    cmp ax, SCREEN_WIDTH
    ja .failed
    mov ax, TARGET_ANCHOR_Y
    sub ax, [selected_anchor_y]
    jc .failed
    mov [selected_dst_y], ax
    add ax, [selected_height]
    cmp ax, SCREEN_HEIGHT
    ja .failed
    mov ax, [selected_bank_offset]
    mov dx, [selected_bank_offset + 2]
    add dx, BMS_WINDOW_SGP_BASE >> 16
    mov [selected_source_low], ax
    mov [selected_source_high], dx
    clc
    ret
.failed:
    stc
    ret

; Scale a signed displacement by a Q8.8 factor with symmetric rounding.
; The helper is integer-only and always derives from the immutable orbit table.
scale_signed_q8:
    push bx
    push cx
    push dx
    push si
    xor si, si
    test ax, ax
    jns .magnitude
    neg ax
    mov si, 1
.magnitude:
    mul bx
    add ax, 128
    adc dx, 0
    mov cx, 8
.shift:
    shr dx, 1
    rcr ax, 1
    loop .shift
    cmp si, 0
    je .done
    neg ax
.done:
    pop si
    pop dx
    pop cx
    pop bx
    ret

; Bind one generated phase to its exact depth descriptor while applying the
; active camera snapshot.  Distance changes only the selected scale, radius
; changes scale both base displacements, and look changes screen Y uniformly.
select_orbit_destination:
    cmp al, ORBIT_PUBLICATIONS_PER_REVOLUTION
    jae .failed
    mov [pending_phase], al
    xor ah, ah
    mov si, ax
    shl si, 1
    shl si, 1
    shl si, 1
    cmp al, [depth_orbit_entries + si + 4]
    jne .failed
    mov ax, [depth_orbit_entries + si]
    mov bx, [active_radius_factor]
    call scale_signed_q8
    mov [pending_orbit_dx], ax
    mov ax, [depth_orbit_entries + si + 2]
    mov bx, [active_radius_factor]
    call scale_signed_q8
    mov [pending_orbit_dy], ax
    mov al, [depth_orbit_entries + si + 6]
    mov [pending_depth_rank], al
    mov al, [depth_orbit_entries + si + 5]
    cmp al, 1
    jb .failed
    cmp al, ATLAS_SCALE_COUNT
    ja .failed
    mov [base_scale_id], al
    ; The depth rank is tied to the base table scale.  Camera distance is a
    ; size cue only and therefore cannot reverse far-to-near ordering.
    xor ax, ax
    mov al, [base_scale_id]
    shl ax, 1
    sub ax, 31
    xor bx, bx
    mov bl, [pending_depth_rank]
    test byte [pending_depth_rank], 0x80
    jz .depth_positive
    dec bh
    cmp ax, bx
    jne .failed
    jmp .depth_checked
.depth_positive:
    cmp ax, bx
    jne .failed
    jmp .depth_checked
.depth_checked:
    ; Apply signed distance bias and clamp the effective descriptor to 1..30.
    xor ax, ax
    mov al, [base_scale_id]
    xor bx, bx
    mov bl, [active_distance_bias]
    cmp byte [active_distance_bias], 0
    jge .bias_ready
    dec bh
.bias_ready:
    add ax, bx
    cmp ax, 1
    jge .scale_low_ready
    mov ax, 1
    inc word [scale_low_clamps]
.scale_low_ready:
    cmp ax, ATLAS_SCALE_COUNT
    jbe .scale_high_ready
    mov ax, ATLAS_SCALE_COUNT
    inc word [scale_high_clamps]
.scale_high_ready:
    mov [selected_scale_id], al
    mov al, [selected_scale_id]
    call select_scale_descriptor
    jc .failed
    ; Compute the adjusted destination and clip at the visible page edge.
    mov ax, [pending_orbit_dx]
    add ax, TARGET_ANCHOR_X
    sub ax, [selected_anchor_x]
    jns .x_nonnegative
    xor ax, ax
    inc word [clipped_instances]
.x_nonnegative:
    mov dx, ax
    add dx, [selected_width]
    cmp dx, SCREEN_WIDTH
    jbe .x_ready
    mov ax, SCREEN_WIDTH
    sub ax, [selected_width]
    mov dx, ax
    inc word [clipped_instances]
.x_ready:
    mov [selected_dst_x], ax
    mov ax, [pending_orbit_dy]
    add ax, TARGET_ANCHOR_Y
    add ax, [screen_y_bias]
    sub ax, [selected_anchor_y]
    jns .y_nonnegative
    xor ax, ax
    inc word [clipped_instances]
.y_nonnegative:
    mov dx, ax
    add dx, [selected_height]
    cmp dx, SCREEN_HEIGHT
    jbe .y_ready
    mov ax, SCREEN_HEIGHT
    sub ax, [selected_height]
    mov dx, ax
    inc word [clipped_instances]
.y_ready:
    mov [selected_dst_y], ax
    ; Half-open pseudo-sprite rectangle must not intersect [4,4,70,20).
    mov ax, [selected_dst_x]
    cmp ax, HUD_X1
    jae .hud_clear
    add ax, [selected_width]
    cmp ax, HUD_X
    jbe .hud_clear
    mov ax, [selected_dst_y]
    cmp ax, HUD_Y1
    jae .hud_clear
    add ax, [selected_height]
    cmp ax, HUD_Y
    jbe .hud_clear
    jmp .failed
.hud_clear:
    clc
    ret
.failed:
    inc word [bounds_failures]
    stc
    ret

; Validate all compiled phase IDs, depth/scale relations, landmarks, cyclic
; change count, scale histogram, and descriptor-specific destinations.
validate_orbit_table:
    cmp word [depth_orbit_entries + 0 * ORBIT_ENTRY_BYTES], ORBIT_RADIUS_X
    jne .failed
    cmp word [depth_orbit_entries + 0 * ORBIT_ENTRY_BYTES + 2], 0
    jne .failed
    cmp byte [depth_orbit_entries + 0 * ORBIT_ENTRY_BYTES + 5], 16
    jne .failed
    cmp byte [depth_orbit_entries + 0 * ORBIT_ENTRY_BYTES + 6], 1
    jne .failed
    cmp word [depth_orbit_entries + 16 * ORBIT_ENTRY_BYTES], 0
    jne .failed
    cmp word [depth_orbit_entries + 16 * ORBIT_ENTRY_BYTES + 2], ORBIT_RADIUS_Y
    jne .failed
    cmp byte [depth_orbit_entries + 16 * ORBIT_ENTRY_BYTES + 5], 30
    jne .failed
    cmp byte [depth_orbit_entries + 16 * ORBIT_ENTRY_BYTES + 6], 29
    jne .failed
    cmp word [depth_orbit_entries + 32 * ORBIT_ENTRY_BYTES], -ORBIT_RADIUS_X
    jne .failed
    cmp word [depth_orbit_entries + 32 * ORBIT_ENTRY_BYTES + 2], 0
    jne .failed
    cmp byte [depth_orbit_entries + 32 * ORBIT_ENTRY_BYTES + 5], 15
    jne .failed
    cmp byte [depth_orbit_entries + 32 * ORBIT_ENTRY_BYTES + 6], -1
    jne .failed
    cmp word [depth_orbit_entries + 48 * ORBIT_ENTRY_BYTES], 0
    jne .failed
    cmp word [depth_orbit_entries + 48 * ORBIT_ENTRY_BYTES + 2], -ORBIT_RADIUS_Y
    jne .failed
    cmp byte [depth_orbit_entries + 48 * ORBIT_ENTRY_BYTES + 5], 1
    jne .failed
    cmp byte [depth_orbit_entries + 48 * ORBIT_ENTRY_BYTES + 6], -29
    jne .failed
    xor si, si
.opposite_loop:
    mov ax, [depth_orbit_entries + si]
    add ax, [depth_orbit_entries + si + 32 * ORBIT_ENTRY_BYTES]
    jne .failed
    mov ax, [depth_orbit_entries + si + 2]
    add ax, [depth_orbit_entries + si + 32 * ORBIT_ENTRY_BYTES + 2]
    jne .failed
    add si, ORBIT_ENTRY_BYTES
    cmp si, 32 * ORBIT_ENTRY_BYTES
    jne .opposite_loop

    push es
    push ds
    pop es
    mov di, table_scale_histogram
    mov cx, ATLAS_SCALE_COUNT
    xor ax, ax
    rep stosb
    pop es
    mov word [table_scale_change_edges], 0
    xor bx, bx
.bounds_loop:
    push bx
    mov ax, bx
    call select_orbit_destination
    pop bx
    jc .failed_counted
    xor ax, ax
    mov al, [selected_scale_id]
    dec ax
    mov si, ax
    inc byte [table_scale_histogram + si]
    mov ax, bx
    inc ax
    and ax, ORBIT_PUBLICATIONS_PER_REVOLUTION - 1
    mov si, ax
    shl si, 1
    shl si, 1
    shl si, 1
    mov ax, bx
    mov di, ax
    shl di, 1
    shl di, 1
    shl di, 1
    mov al, [depth_orbit_entries + di + 5]
    cmp al, [depth_orbit_entries + si + 5]
    je .same_scale
    inc word [table_scale_change_edges]
.same_scale:
    inc bx
    cmp bx, ORBIT_PUBLICATIONS_PER_REVOLUTION
    jne .bounds_loop
    cmp word [table_scale_change_edges], 58
    jne .failed
    xor si, si
.histogram_loop:
    mov al, [expected_scale_histogram + si]
    cmp al, [table_scale_histogram + si]
    jne .failed
    inc si
    cmp si, ATLAS_SCALE_COUNT
    jne .histogram_loop
    mov al, 0
    call select_orbit_destination
    ret
.failed:
    inc word [bounds_failures]
.failed_counted:
    stc
    ret

; Build the exact bounded M98u active prefix for one global phase.  Records
; stay in instance-ID order; draw_order contains the far-to-near permutation.
record_pointer_from_index:
    xor ah, ah
    mov bx, M98U_INSTANCE_RECORD_BYTES
    mul bx
    add ax, instance_records
    mov di, ax
    ret

generate_multi_instance_frame:
    cmp al, ORBIT_PUBLICATIONS_PER_REVOLUTION
    jae .failed
    mov [pending_global_phase], al
    mov byte [generation_instance], 0
.record_loop:
    xor ax, ax
    mov al, [generation_instance]
    mov cl, 6
    shl ax, cl
    xor dx, dx
    mov bx, [build_active_count]
    div bx
    cmp ax, ORBIT_PUBLICATIONS_PER_REVOLUTION
    jae .failed
    mov [generated_phase_offset], al
    add al, [pending_global_phase]
    and al, ORBIT_PUBLICATIONS_PER_REVOLUTION - 1
    mov [generated_phase], al
    call select_orbit_destination
    jc .failed

    mov al, [generation_instance]
    call record_pointer_from_index
    mov al, [generation_instance]
    mov [di + M98U_RECORD_INSTANCE_ID], al
    mov al, [generated_phase_offset]
    mov [di + M98U_RECORD_PHASE_OFFSET], al
    mov al, [generated_phase]
    mov [di + M98U_RECORD_PHASE_ID], al
    mov al, [selected_scale_id]
    mov [di + M98U_RECORD_SCALE_ID], al
    mov al, [pending_depth_rank]
    mov [di + M98U_RECORD_DEPTH_RANK], al
    mov byte [di + M98U_RECORD_BMS_BANK], BMS_FIRST_SELECTOR
    mov al, [selected_scale_id]
    dec al
    mov [di + M98U_RECORD_DESCRIPTOR_INDEX], al
    mov byte [di + M98U_RECORD_RESERVED], 0
    mov ax, [pending_orbit_dx]
    mov [di + M98U_RECORD_DX], ax
    mov bx, ax
    add bx, TARGET_ANCHOR_X
    mov [di + M98U_RECORD_TARGET_ANCHOR_X], bx
    mov ax, [pending_orbit_dy]
    mov [di + M98U_RECORD_DY], ax
    mov bx, ax
    add bx, TARGET_ANCHOR_Y
    add bx, [screen_y_bias]
    mov [di + M98U_RECORD_TARGET_ANCHOR_Y], bx
    mov ax, [selected_width]
    mov [di + M98U_RECORD_WIDTH], ax
    mov ax, [selected_height]
    mov [di + M98U_RECORD_HEIGHT], ax
    mov ax, [selected_pitch]
    mov [di + M98U_RECORD_PITCH], ax
    mov ax, [selected_anchor_x]
    mov [di + M98U_RECORD_ANCHOR_X], ax
    mov ax, [selected_anchor_y]
    mov [di + M98U_RECORD_ANCHOR_Y], ax
    mov ax, [selected_dst_x]
    mov [di + M98U_RECORD_DST_X], ax
    add ax, [selected_width]
    mov [di + M98U_RECORD_DST_X1], ax
    mov ax, [selected_dst_y]
    mov [di + M98U_RECORD_DST_Y], ax
    add ax, [selected_height]
    mov [di + M98U_RECORD_DST_Y1], ax
    mov ax, [selected_bank_offset]
    mov [di + M98U_RECORD_BANK_OFFSET], ax
    mov ax, [selected_bank_offset + 2]
    mov [di + M98U_RECORD_BANK_OFFSET + 2], ax
    mov ax, [selected_source_low]
    mov [di + M98U_RECORD_SGP_SOURCE], ax
    mov ax, [selected_source_high]
    mov [di + M98U_RECORD_SGP_SOURCE + 2], ax
    mov ax, [selected_payload_bytes]
    mov [di + M98U_RECORD_PAYLOAD_BYTES], ax
    mov ax, [selected_payload_bytes + 2]
    mov [di + M98U_RECORD_PAYLOAD_BYTES + 2], ax
    mov ax, [expected_frame_crc]
    mov [di + M98U_RECORD_SOURCE_IDENTITY], ax
    mov ax, [expected_frame_crc + 2]
    mov [di + M98U_RECORD_SOURCE_IDENTITY + 2], ax

    ; Direct-formula phase assignments must be unique in the active prefix.
    xor cx, cx
    mov cl, [generation_instance]
    jcxz .unique
    mov si, instance_records
.unique_loop:
    mov al, [generated_phase]
    cmp al, [si + M98U_RECORD_PHASE_ID]
    je .failed
    add si, M98U_INSTANCE_RECORD_BYTES
    loop .unique_loop
.unique:
    xor bx, bx
    mov bl, [generation_instance]
    mov [draw_order + bx], bl
    inc byte [generation_instance]
    xor ax, ax
    mov al, [generation_instance]
    cmp ax, [build_active_count]
    jb .record_loop

    ; Deterministic bounded insertion sort by signed depth, then instance ID.
    mov byte [sort_position], 1
.sort_outer:
    xor bx, bx
    mov bl, [sort_position]
    mov al, [draw_order + bx]
    mov [sort_candidate], al
    call record_pointer_from_index
    mov [sort_candidate_ptr], di
    mov al, [sort_position]
    mov [sort_scan], al
.sort_inner:
    cmp byte [sort_scan], 0
    je .insert_candidate
    xor bx, bx
    mov bl, [sort_scan]
    dec bx
    mov al, [draw_order + bx]
    mov [sort_prior], al
    call record_pointer_from_index
    mov si, di
    mov di, [sort_candidate_ptr]
    mov al, [si + M98U_RECORD_DEPTH_RANK]
    mov ah, [di + M98U_RECORD_DEPTH_RANK]
    cmp al, ah
    jl .insert_candidate
    jg .shift_prior
    mov al, [si + M98U_RECORD_INSTANCE_ID]
    cmp al, [di + M98U_RECORD_INSTANCE_ID]
    jbe .insert_candidate
.shift_prior:
    xor bx, bx
    mov bl, [sort_scan]
    mov al, [sort_prior]
    mov [draw_order + bx], al
    dec byte [sort_scan]
    jmp .sort_inner
.insert_candidate:
    xor bx, bx
    mov bl, [sort_scan]
    mov al, [sort_candidate]
    mov [draw_order + bx], al
.keep_order:
    inc byte [sort_position]
    xor ax, ax
    mov al, [sort_position]
    cmp ax, [build_active_count]
    jb .sort_outer

%if M98Y_PRIVATE_PROFILE
    push es
    push ds
    pop es
    mov di, draw_order_seen
    xor ax, ax
    mov cx, DRAW_ORDER_SEEN_WORDS
    rep stosw
    pop es
    xor bx, bx
.order_check_private:
    mov al, [draw_order + bx]
    xor ah, ah
    cmp ax, [build_active_count]
    jae .failed
    mov si, ax
    mov cl, al
    and cl, 0x0f
    mov dx, 1
    shl dx, cl
    mov ax, si
    mov cl, 4
    shr ax, cl
    shl ax, 1
    mov di, draw_order_seen
    add di, ax
    test [di], dx
    jnz .failed
    or [di], dx
    inc bx
    mov ax, [build_active_count]
    cmp bx, ax
    jb .order_check_private
    ; The bounded loop above has visited exactly build_active_count entries;
    ; each was range-checked and rejected on a duplicate bit.  Do not require
    ; unused bits in the four-word private mask to be set for counts below 64.
%else
    mov word [draw_order_seen], 0
    xor bx, bx
.order_check_public:
    mov al, [draw_order + bx]
    xor ah, ah
    cmp ax, [build_active_count]
    jae .failed
    mov cl, al
    mov ax, 1
    shl ax, cl
    test [draw_order_seen], ax
    jnz .failed
    or [draw_order_seen], ax
    inc bx
    mov ax, [build_active_count]
    cmp bx, ax
    jb .order_check_public
    mov ax, [draw_order_seen]
    mov dx, 1
    mov cl, byte [build_active_count]
    shl dx, cl
    dec dx
    cmp ax, dx
    jne .failed
%endif
    mov al, [pending_global_phase]
    mov [pending_phase], al
    clc
    ret
.failed:
    inc word [draw_order_failures]
    stc
    ret

validate_all_multi_instance_states:
    mov byte [validation_global_phase], 0
.phase:
    mov al, [validation_global_phase]
    call generate_multi_instance_frame
    jc .failed
    inc byte [validation_global_phase]
    cmp byte [validation_global_phase], ORBIT_PUBLICATIONS_PER_REVOLUTION
    jb .phase
    clc
    ret
.failed:
    stc
    ret

load_instance_for_draw:
    xor ah, ah
    cmp ax, [build_active_count]
    jae .failed
    call record_pointer_from_index
    mov al, [di + M98U_RECORD_SCALE_ID]
    push di
    call select_scale_descriptor
    pop di
    jc .failed
    mov ax, [di + M98U_RECORD_WIDTH]
    cmp ax, [selected_width]
    jne .failed
    mov ax, [di + M98U_RECORD_HEIGHT]
    cmp ax, [selected_height]
    jne .failed
    mov ax, [di + M98U_RECORD_PITCH]
    cmp ax, [selected_pitch]
    jne .failed
    mov ax, [di + M98U_RECORD_DST_X]
    mov [selected_dst_x], ax
    mov ax, [di + M98U_RECORD_DST_Y]
    mov [selected_dst_y], ax
    mov ax, [di + M98U_RECORD_SGP_SOURCE]
    mov [selected_source_low], ax
    mov ax, [di + M98U_RECORD_SGP_SOURCE + 2]
    mov [selected_source_high], ax
    mov al, [di + M98U_RECORD_PHASE_ID]
    mov [pending_phase], al
    mov al, [di + M98U_RECORD_DEPTH_RANK]
    mov [pending_depth_rank], al
    clc
    ret
.failed:
    inc word [source_failures]
    stc
    ret

compare_final_crcs:
    mov ax, [file_crc_state]
    not ax
    cmp ax, [expected_file_crc]
    jne .failed
    mov ax, [file_crc_state + 2]
    not ax
    cmp ax, [expected_file_crc + 2]
    jne .failed
    mov ax, [payload_crc_state]
    not ax
    cmp ax, [expected_payload_crc]
    jne .failed
    mov ax, [payload_crc_state + 2]
    not ax
    cmp ax, [expected_payload_crc + 2]
    jne .failed
    clc
    ret
.failed:
    stc
    ret

verify_bms_payload_crc:
    mov word [bms_crc_state], 0xffff
    mov word [bms_crc_state + 2], 0xffff
    mov word [crc_range_offset], 0
    mov word [crc_range_offset + 2], 0
    mov ax, [atlas_payload_bytes]
    mov [crc_range_remaining], ax
    mov ax, [atlas_payload_bytes + 2]
    mov [crc_range_remaining + 2], ax
    mov bp, bms_crc_state
    call crc32_bms_range
    mov ax, [bms_crc_state]
    not ax
    cmp ax, [expected_payload_crc]
    jne .failed
    mov ax, [bms_crc_state + 2]
    not ax
    cmp ax, [expected_payload_crc + 2]
    jne .failed
    clc
    ret
.failed:
    stc
    ret

verify_bms_frame_crc:
    mov word [frame_crc_state], 0xffff
    mov word [frame_crc_state + 2], 0xffff
    mov ax, [selected_bank_offset]
    mov [crc_range_offset], ax
    mov ax, [selected_bank_offset + 2]
    mov [crc_range_offset + 2], ax
    mov ax, [selected_payload_bytes]
    mov [crc_range_remaining], ax
    mov ax, [selected_payload_bytes + 2]
    mov [crc_range_remaining + 2], ax
    mov bp, frame_crc_state
    call crc32_bms_range
    mov ax, [frame_crc_state]
    not ax
    cmp ax, [expected_frame_crc]
    jne .failed
    mov ax, [frame_crc_state + 2]
    not ax
    cmp ax, [expected_frame_crc + 2]
    jne .failed
    clc
    ret
.failed:
    stc
    ret

crc32_bms_range:
    mov dx, PORT_BMS_SELECTOR
    mov al, BMS_FIRST_SELECTOR
    out dx, al
.next:
    mov ax, [crc_range_remaining]
    or ax, [crc_range_remaining + 2]
    jz .done
    mov cx, STAGING_BYTES
    cmp word [crc_range_remaining + 2], 0
    jne .size_ready
    cmp word [crc_range_remaining], STAGING_BYTES
    jae .size_ready
    mov cx, [crc_range_remaining]
.size_ready:
    push cx
    mov ax, [crc_range_offset]
    mov dx, [crc_range_offset + 2]
    call bms_segment_from_offset
    mov si, di
    pop cx
    call crc32_update_es
    mov ax, cx
    add [crc_range_offset], ax
    adc word [crc_range_offset + 2], 0
    sub [crc_range_remaining], ax
    sbb word [crc_range_remaining + 2], 0
    jmp .next
.done:
    ret

; AX:DX is a byte offset within one 128-KiB bank.  Return ES:DI inside the
; CPU-visible 80000h-9ffffh aperture.
bms_segment_from_offset:
    mov di, ax
    and di, 0x000f
    mov cx, 4
.shift_low:
    shr ax, 1
    loop .shift_low
    mov cx, 12
.shift_high:
    shl dx, 1
    loop .shift_high
    add ax, dx
    add ax, BMS_WINDOW_SEGMENT
    mov es, ax
    ret

; Update the little-endian CRC32 state at CS:BP over ES:SI/CX.  This is the
; reflected polynomial EDB88320h with initial state FFFFFFFFh.
crc32_update_es:
    push ax
    push bx
    push cx
    push dx
    push si
.byte:
    mov al, [es:si]
    inc si
    xor byte [cs:bp], al
    mov bx, 8
.bit:
    shr word [cs:bp + 2], 1
    rcr word [cs:bp], 1
    jnc .next_bit
    xor word [cs:bp], 0x8320
    xor word [cs:bp + 2], 0xedb8
.next_bit:
    dec bx
    jnz .bit
    loop .byte
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

poison_staging_buffer:
    push ds
    pop es
    mov di, staging_buffer
    mov al, STAGING_POISON
    mov cx, STAGING_BYTES
    rep stosb
    ret

verify_staging_poison:
    push ds
    pop es
    mov di, staging_buffer
    mov al, STAGING_POISON
    mov cx, STAGING_BYTES
    repe scasb
    jne .failed
    clc
    ret
.failed:
    stc
    ret

close_atlas_if_open:
    cmp word [atlas_handle], 0xffff
    je .done
    call select_ordinary_mapping
    mov bx, [atlas_handle]
    mov ah, 0x3e
    int 0x21
    mov word [atlas_handle], 0xffff
.done:
    ret

; M98q initializes both 64,000-byte G1 pages once before either is visible.
initialize_video_double_buffer:
    call verify_page_descriptors
    jc .failed
    mov ax, [selected_bank_offset]
    mov dx, [selected_bank_offset + 2]
    add dx, BMS_WINDOW_SGP_BASE >> 16
    mov [selected_source_low], ax
    mov [selected_source_high], dx
    call verify_staging_poison
    jc .failed
    call verify_bms_payload_crc
    jc .failed
    call verify_all_bms_frame_crcs
    jc .failed
    mov al, FIXED_SCALE_ID
    call select_scale_descriptor
    jc .failed
    call validate_orbit_table
    jc .failed
    call select_ordinary_mapping
    call verify_normal_guards
    jc .failed

    mov bx, MODE_320X200_G0_G1
    mov cx, PIXEL_SIZE_G0_G1_8BPP
    xor dx, dx
    mov byte [video_mode_changed], 1
    xor ax, ax
    int VIDEO_BIOS_INT
    test ax, ax
    long_jnz .failed
    mov ax, 0x0b00
    int VIDEO_BIOS_INT
    test ax, ax
    long_jnz .failed
    mov ax, 0x0900
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    mov ax, 0x0a00
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    mov ax, 0x0300
    mov cx, COMPOSE_G1_OVER_G0
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed

    xor ax, ax
    mov dx, PORT_PALETTE_COMPOSE
    out dx, ax
    mov ax, RGB_COMPOSE_G1_OVER_G0
    mov dx, PORT_RGB_COMPOSE
    out dx, ax
    mov dx, PORT_G0_TRANSPARENCY
    xor ax, ax
    out dx, ax
    mov dx, PORT_G1_TRANSPARENCY
    mov ax, 0x0001
    out dx, ax
    call configure_g1_framebuffer
    mov dx, PORT_MEMORY_MAP
    mov al, MEMORY_MAP_GVRAM_SINGLE
    out dx, al
    mov dx, PORT_GVRAM_WRITE_MODE
    mov al, GVRAM_CPU_WRITE_MODE
    out dx, al
    call draw_g0_checkerboard
    call draw_hud_full
    jc .failed
    call build_initialization_commands
    call run_sgp_command_list
    jc .failed
    mov word [initial_full_page_clears], 2
    inc word [sgp_command_lists]
    add word [sgp_commands], 5
    mov byte [page_state + PAGE_A], PAGE_HIDDEN_CLEAN
    mov byte [page_state + PAGE_B], PAGE_HIDDEN_CLEAN
    mov byte [page_old_valid + PAGE_A], 0
    mov byte [page_old_valid + PAGE_B], 0
    mov byte [page_footprint_valid + PAGE_A], 0
    mov byte [page_footprint_valid + PAGE_B], 0
    mov byte [page_footprint_count + PAGE_A], 0
    mov byte [page_footprint_count + PAGE_B], 0
    mov word [pages_initialized], 2
    call wait_vblank_edge
    jc .failed
    mov al, M98Q_INITIAL_VISIBLE_PAGE
    call publish_page
    jc .failed
    mov byte [visible_page_index], M98Q_INITIAL_VISIBLE_PAGE
%if M98Q_INITIAL_VISIBLE_PAGE = PAGE_A
    mov byte [hidden_page_index], PAGE_B
    mov byte [page_state + PAGE_A], PAGE_VISIBLE
    inc word [page_a_publications]
%else
    mov byte [hidden_page_index], PAGE_A
    mov byte [page_state + PAGE_B], PAGE_VISIBLE
    inc word [page_b_publications]
%endif
    mov ax, 0x0b01
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    clc
    ret
.failed:
    call select_ordinary_mapping
    stc
    ret

verify_all_bms_frame_crcs:
    mov byte [descriptor_validation_id], 1
.next:
    mov al, [descriptor_validation_id]
    call select_scale_descriptor
    jc .failed
    call verify_bms_frame_crc
    jc .failed
    inc byte [descriptor_validation_id]
    cmp byte [descriptor_validation_id], ATLAS_SCALE_COUNT + 1
    jne .next
    mov al, ATLAS_SCALE_COUNT
    call select_scale_descriptor
    jc .failed
    clc
    ret
.failed:
    stc
    ret

verify_page_descriptors:
    cmp word [page_sgp_low + PAGE_A * 2], G1_PAGE_A_SGP_BASE & 0xffff
    jne .failed
    cmp word [page_sgp_high + PAGE_A * 2], G1_PAGE_A_SGP_BASE >> 16
    jne .failed
    cmp word [page_sgp_low + PAGE_B * 2], G1_PAGE_B_SGP_BASE & 0xffff
    jne .failed
    cmp word [page_sgp_high + PAGE_B * 2], G1_PAGE_B_SGP_BASE >> 16
    jne .failed
    cmp word [page_dsa_low + PAGE_A * 2], G1_PAGE_A_DSA & 0xffff
    jne .failed
    cmp word [page_dsa_high + PAGE_A * 2], G1_PAGE_A_DSA >> 16
    jne .failed
    cmp word [page_dsa_low + PAGE_B * 2], G1_PAGE_B_DSA & 0xffff
    jne .failed
    cmp word [page_dsa_high + PAGE_B * 2], G1_PAGE_B_DSA >> 16
    jne .failed
    clc
    ret
.failed:
    stc
    ret

configure_g1_framebuffer:
    push ax
    push dx
    mov dx, PORT_FB1_FBW
    mov ax, SCREEN_PITCH
    out dx, ax
    mov dx, PORT_FB1_DOT
    xor ax, ax
    out dx, ax
    mov dx, PORT_FB1_OFX
    out dx, ax
    mov dx, PORT_FB1_OFY
    out dx, ax
    mov dx, PORT_FB1_DSH
    mov ax, SCREEN_HEIGHT
    out dx, ax
    mov dx, PORT_FB1_DSP
    xor ax, ax
    out dx, ax
    pop dx
    pop ax
    ret

publish_page:
    ; AL is an explicitly validated page index.
    cmp al, PAGE_B
    ja .failed
    xor ah, ah
    mov si, ax
    mov ah, [page_state + si]
    cmp ah, PAGE_HIDDEN_COMPLETE
    je .state_ready
    cmp ah, PAGE_HIDDEN_CLEAN
    jne .failed
.state_ready:
    shl si, 1
    mov dx, PORT_FB1_DSA_LOW
    mov ax, [page_dsa_low + si]
    mov [last_published_dsa], ax
    out dx, ax
    add dx, 2
    mov ax, [page_dsa_high + si]
    out dx, ax
    clc
    ret
.failed:
    stc
    ret

draw_g0_checkerboard:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    push es
    mov ax, G0_SEGMENT
    mov es, ax
    xor di, di
    xor bx, bx
.row:
    mov dx, checker_row_a
    mov bp, checker_row_b
    test bl, 0x10
    jz .phase_ready
    xchg dx, bp
.phase_ready:
    mov cx, 20
.tile:
    mov si, dx
    test cl, 1
    jz .tile_source_ready
    mov si, bp
.tile_source_ready:
    push cx
    mov cx, 16
    rep movsb
    pop cx
    dec cx
    jnz .tile
    inc bx
    cmp bx, SCREEN_HEIGHT
    jne .row
    pop es
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; Initialize exactly 66 bytes on each of 16 G0 rows from the selected
; command-line divisor tile.  This is the only full HUD write.
draw_hud_full:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    push es
    xor bx, bx
    mov bl, [parsed_divisor]
    cmp bl, CADENCE_MIN
    jb .failed
    cmp bl, CADENCE_MAX
    ja .failed
    dec bx
    shl bx, 1
    mov si, [hud_full_tile_pointers + bx]
    mov ax, G0_SEGMENT
    mov es, ax
    mov di, HUD_Y * SCREEN_PITCH + HUD_X
    mov bp, HUD_HEIGHT
.row:
    mov cx, HUD_WIDTH
    rep movsb
    add di, SCREEN_PITCH - HUD_WIDTH
    dec bp
    jnz .row
    inc word [hud_full_initializations]
    inc word [hud_fps_field_updates]
    add word [hud_bytes_written], HUD_FULL_WRITE_BYTES
    adc word [hud_bytes_written + 2], 0
    call draw_hud_count_field
    jc .failed
    call draw_hud_status
    clc
    jmp .done
.failed:
    inc word [hud_mismatches]
    stc
.done:
    pop es
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; Draw one fixed-width generated status tile into G0.  SI/DI/CX/BP are the
; source, destination, width, and row count respectively.  Status writes are
; intentionally separate from the legacy HUD byte counter: the original HUD
; identity and its exact full/FPS/count accounting remain unchanged.
draw_hud_status_tile:
    push ax
    push bx
    push dx
    push si
    push di
    push cx
    push bp
    push es
    mov bx, cx
    mov ax, G0_SEGMENT
    mov es, ax
.row:
    mov cx, bx
    rep movsb
    mov dx, SCREEN_PITCH
    sub dx, bx
    add di, dx
    dec bp
    jnz .row
    inc word [hud_status_updates]
    pop es
    pop bp
    pop cx
    pop di
    pop si
    pop dx
    pop bx
    pop ax
    ret

; Publish the active projection controls as a G0-only status panel.  The
; renderer's visible count/FPS fields retain their accepted locations; this
; panel merely makes the four new controls observable without touching G1.
draw_hud_status:
    push ax
    push bx
    push si
    push di
    push cx
    push bp
    xor bx, bx
    mov bl, [active_speed_index]
    cmp bl, SPEED_MAX
    ja .failed
    shl bx, 1
    mov si, [hud_status_speed_tile_pointers + bx]
    mov di, HUD_STATUS_Y * SCREEN_PITCH + HUD_STATUS_SPEED_X
    mov cx, STATUS_SPEED_WIDTH
    mov bp, STATUS_FIELD_HEIGHT
    call draw_hud_status_tile
    xor ax, ax
    mov al, [active_distance_bias]
    cbw
    add ax, 4
    cmp ax, 0
    jl .failed
    cmp ax, 8
    ja .failed
    mov bx, ax
    shl bx, 1
    mov si, [hud_status_distance_tile_pointers + bx]
    mov di, HUD_STATUS_Y * SCREEN_PITCH + HUD_STATUS_DISTANCE_X
    mov cx, STATUS_DISTANCE_WIDTH
    mov bp, STATUS_FIELD_HEIGHT
    call draw_hud_status_tile
    xor ax, ax
    mov al, [active_look_level]
    cbw
    add ax, 4
    cmp ax, 0
    jl .failed
    cmp ax, 8
    ja .failed
    mov bx, ax
    shl bx, 1
    mov si, [hud_status_look_tile_pointers + bx]
    mov di, HUD_STATUS_SECOND_Y * SCREEN_PITCH + HUD_STATUS_LOOK_X
    mov cx, STATUS_LOOK_WIDTH
    mov bp, STATUS_FIELD_HEIGHT
    call draw_hud_status_tile
    xor bx, bx
    mov bl, [active_radius_index]
    cmp bl, RADIUS_MAX
    ja .failed
    shl bx, 1
    mov si, [hud_status_radius_tile_pointers + bx]
    mov di, HUD_STATUS_SECOND_Y * SCREEN_PITCH + HUD_STATUS_RADIUS_X
    mov cx, STATUS_RADIUS_WIDTH
    mov bp, STATUS_FIELD_HEIGHT
    call draw_hud_status_tile
    clc
    jmp .done
.failed:
    inc word [hud_mismatches]
    stc
.done:
    pop bp
    pop cx
    pop di
    pop si
    pop bx
    pop ax
    ret

; Replace the final two line-2 cells during initialization.  M98x keeps all
; sixteen fixed-width count tiles in the generated include and selects the
; startup count without copying any sprite data.
draw_hud_count_field:
    push ax
    push cx
    push si
    push di
    push bp
    push es
    xor bx, bx
    mov bl, [build_active_count]
    cmp bl, 1
    jb .failed
    cmp bl, FOOTPRINT_CAPACITY
    ja .failed
    dec bx
    shl bx, 1
    mov si, [hud_count_tile_pointers + bx]
    mov ax, G0_SEGMENT
    mov es, ax
    mov di, HUD_COUNT_Y * SCREEN_PITCH + HUD_COUNT_X
    mov bp, HUD_COUNT_HEIGHT
.row:
    mov cx, HUD_COUNT_WIDTH
    rep movsb
    add di, SCREEN_PITCH - HUD_COUNT_WIDTH
    dec bp
    jnz .row
    inc word [hud_zundamon_field_updates]
    inc word [hud_count_field_updates]
    add word [hud_bytes_written], HUD_COUNT_WRITE_BYTES
    adc word [hud_bytes_written + 2], 0
    clc
.ok:
    jmp .done
.failed:
    inc word [hud_mismatches]
    stc
.done:
    pop es
    pop bp
    pop di
    pop si
    pop cx
    pop ax
    ret

; Apply the staged count tile for the frame being published.  The caller has
; already validated the complete hidden frame and is at the publication
; boundary, so this bounded direct-G0 write is committed with DSA1.  The
; fixed-width tile erases both digits when a two-digit value changes to one.
update_hud_count_field:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    push es
    xor bx, bx
    mov bl, [build_active_count]
    cmp bl, 1
    jb .failed
    cmp bl, FOOTPRINT_CAPACITY
    ja .failed
    dec bx
    shl bx, 1
    mov si, [hud_count_tile_pointers + bx]
    mov ax, G0_SEGMENT
    mov es, ax
    mov di, HUD_COUNT_Y * SCREEN_PITCH + HUD_COUNT_X
    mov bp, HUD_COUNT_HEIGHT
.row:
    mov cx, HUD_COUNT_WIDTH
    rep movsb
    add di, SCREEN_PITCH - HUD_COUNT_WIDTH
    dec bp
    jnz .row
    inc word [hud_count_field_updates]
    add word [hud_bytes_written], HUD_COUNT_WRITE_BYTES
    adc word [hud_bytes_written + 2], 0
    mov byte [hud_count_staged], 1
    clc
    jmp .done
.failed:
    inc word [hud_mismatches]
    stc
.done:
    pop es
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; Restore the count field that describes the last committed visible page after
; a staged G0 update could not be paired with the DSA1 publication.  This is
; deliberately counter-free: it is a rollback, not a new visible update.
restore_hud_count_field:
    push ax
    push bx
    push cx
    push si
    push di
    push bp
    push es
    xor bx, bx
    mov bl, [published_active_count]
    cmp bl, 1
    jb .failed
    cmp bl, FOOTPRINT_CAPACITY
    ja .failed
    dec bx
    shl bx, 1
    mov si, [hud_count_tile_pointers + bx]
    mov ax, G0_SEGMENT
    mov es, ax
    mov di, HUD_COUNT_Y * SCREEN_PITCH + HUD_COUNT_X
    mov bp, HUD_COUNT_HEIGHT
.row:
    mov cx, HUD_COUNT_WIDTH
    rep movsb
    add di, SCREEN_PITCH - HUD_COUNT_WIDTH
    dec bp
    jnz .row
    clc
    jmp .done
.failed:
    inc word [hud_mismatches]
    stc
.done:
    pop es
    pop bp
    pop di
    pop si
    pop cx
    pop bx
    pop ax
    ret

; Replace exactly 18 bytes on each of eight G0 rows while the freshly observed
; VBLANK remains high.  The fixed tiles always erase stale third digits or a
; decimal point.  This routine never touches G1 or submits an SGP command.
update_hud_fps_field:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    push es
    mov dx, PORT_TSP_STATUS
    in al, dx
    test al, TSP_STATUS_VBLANK
    jz .overrun
    xor bx, bx
    mov bl, [active_divisor]
    cmp bl, CADENCE_MIN
    jb .failed
    cmp bl, CADENCE_MAX
    ja .failed
    dec bx
    shl bx, 1
    mov si, [hud_fps_tile_pointers + bx]
    mov ax, G0_SEGMENT
    mov es, ax
    mov di, HUD_FPS_Y * SCREEN_PITCH + HUD_FPS_X
    mov bp, HUD_FPS_HEIGHT
.row:
    mov cx, HUD_FPS_WIDTH
    rep movsb
    add di, SCREEN_PITCH - HUD_FPS_WIDTH
    dec bp
    jnz .row
    mov dx, PORT_TSP_STATUS
    in al, dx
    test al, TSP_STATUS_VBLANK
    jnz .field_updated
    ; The complete CPU tile write may legitimately finish at the falling
    ; edge.  It is no longer a partial update, so retain the new field and
    ; record the diagnostic without converting a valid RIGHT/LEFT change into
    ; a fatal runtime exit.
    inc word [hud_vblank_overruns]
.field_updated:
    inc word [hud_fps_field_updates]
    add word [hud_bytes_written], HUD_FPS_WRITE_BYTES
    adc word [hud_bytes_written + 2], 0
    clc
    jmp .done
.overrun:
    inc word [hud_vblank_overruns]
%if M98Y_PRIVATE_PROFILE
    stc
    jmp .done
%endif
.failed:
    inc word [hud_mismatches]
    stc
.done:
    pop es
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

build_initialization_commands:
    push ax
    push dx
    push si
    push di
    push es
    push ds
    pop es
    mov di, sgp_command_list
    mov ax, SGP_COMMAND_SET_WORK
    stosw
    mov si, sgp_work_area
    call physical_address_from_ds_si
    stosw
    mov ax, dx
    stosw
    mov ax, SGP_COMMAND_SET_COLOR
    stosw
    xor ax, ax
    stosw
    mov ax, SGP_COMMAND_CLS
    stosw
    mov ax, G1_PAGE_A_SGP_BASE & 0xffff
    stosw
    mov ax, G1_PAGE_A_SGP_BASE >> 16
    stosw
    mov ax, G1_PAGE_WORD_COUNT
    stosw
    xor ax, ax
    stosw
    mov ax, SGP_COMMAND_CLS
    stosw
    mov ax, G1_PAGE_B_SGP_BASE & 0xffff
    stosw
    mov ax, G1_PAGE_B_SGP_BASE >> 16
    stosw
    mov ax, G1_PAGE_WORD_COUNT
    stosw
    xor ax, ax
    stosw
    mov ax, SGP_COMMAND_END
    stosw
    jmp finalize_sgp_command_list

build_full_page_clear_commands:
    push ax
    push dx
    push si
    push di
    push es
    push ds
    pop es
    xor ah, ah
    mov si, ax
    shl si, 1
    mov di, sgp_command_list
    mov ax, SGP_COMMAND_SET_WORK
    stosw
    push si
    mov si, sgp_work_area
    call physical_address_from_ds_si
    stosw
    mov ax, dx
    stosw
    pop si
    mov ax, SGP_COMMAND_SET_COLOR
    stosw
    xor ax, ax
    stosw
    mov ax, SGP_COMMAND_CLS
    stosw
    mov ax, [page_sgp_low + si]
    stosw
    mov ax, [page_sgp_high + si]
    stosw
    mov ax, G1_PAGE_WORD_COUNT
    stosw
    xor ax, ax
    stosw
    mov ax, SGP_COMMAND_END
    stosw
    jmp finalize_sgp_command_list

build_bitblt_commands:
    ; The explicit hidden page chooses only the destination.  The selected
    ; M98u record supplies its descriptor, position, and shared-bank source.
    push ax
    push dx
    push si
    push di
    push es
    push ds
    pop es
    xor ah, ah
    mov si, ax
    shl si, 1
    mov di, sgp_command_list
    mov ax, SGP_COMMAND_SET_WORK
    stosw
    push si
    mov si, sgp_work_area
    call physical_address_from_ds_si
    stosw
    mov ax, dx
    stosw
    pop si

    mov ax, SGP_COMMAND_SET_SOURCE
    stosw
    mov ax, 2
    stosw
    mov ax, [selected_width]
    stosw
    mov ax, [selected_height]
    stosw
    mov ax, [selected_pitch]
    stosw
    mov ax, [selected_source_low]
    stosw
    mov ax, [selected_source_high]
    stosw

    mov ax, SGP_COMMAND_SET_DEST
    stosw
    mov ax, [selected_dst_x]
    and ax, 1
    shl ax, SGP_DEST_PAGE_SHIFT
    or ax, 2
    stosw
    mov ax, [selected_width]
    stosw
    mov ax, [selected_height]
    stosw
    mov ax, SCREEN_PITCH
    stosw
    mov ax, [selected_dst_y]
    mul word [screen_pitch_word]
    mov bx, [selected_dst_x]
    add ax, bx
    adc dx, 0
    and ax, 0xfffe
    add ax, [page_sgp_low + si]
    adc dx, [page_sgp_high + si]
    stosw
    mov ax, dx
    stosw
    mov ax, SGP_COMMAND_BITBLT
    stosw
    mov ax, SGP_BITBLT_COPY_XPAR
    stosw
    mov ax, SGP_COMMAND_END
    stosw
    jmp finalize_sgp_command_list

; Build a bounded list containing one zero-valued CLS per old scanline.
; dirty_row_address is a physical byte address, while CLS length is an exact
; word count.  The live SGP decrements this count after every 16-bit write.
build_dirty_row_commands:
    push ax
    push dx
    push si
    push di
    push es
    push ds
    pop es
    mov di, sgp_command_list
    mov ax, SGP_COMMAND_SET_WORK
    stosw
    mov si, sgp_work_area
    call physical_address_from_ds_si
    stosw
    mov ax, dx
    stosw
    mov ax, SGP_COMMAND_SET_COLOR
    stosw
    xor ax, ax
    stosw
    mov cx, [dirty_batch_rows]
    test cx, cx
    jz .invalid
    cmp cx, DIRTY_ROWS_PER_BATCH
    ja .invalid
    mov bx, [dirty_row_address]
    mov dx, [dirty_row_address + 2]
.row:
    mov ax, SGP_COMMAND_CLS
    stosw
    mov ax, bx
    stosw
    mov ax, dx
    stosw
    mov ax, [dirty_words_per_row]
    stosw
    xor ax, ax
    stosw
    add bx, SCREEN_PITCH
    adc dx, 0
    loop .row
    mov [dirty_row_address], bx
    mov [dirty_row_address + 2], dx
    mov ax, SGP_COMMAND_END
    stosw
    jmp finalize_sgp_command_list
.invalid:
    ; The caller validates this state before entering the builder.  Preserve a
    ; bounded END-only list if an internal invariant is nevertheless broken.
    mov word [dirty_builder_failed], 1
    mov di, sgp_command_list
    mov ax, SGP_COMMAND_END
    stosw
finalize_sgp_command_list:
    mov si, sgp_command_list
    call physical_address_from_ds_si
    mov [sgp_command_address_low], ax
    mov [sgp_command_address_high], dx
    pop es
    pop di
    pop si
    pop dx
    pop ax
    ret

prepare_pending_rectangle:
    cmp byte [selected_scale_id], 1
    jb .failed
    cmp byte [selected_scale_id], ATLAS_SCALE_COUNT
    ja .failed
    mov ax, [selected_dst_x]
    cmp ax, SCREEN_WIDTH
    jae .failed
    mov [pending_x], ax
    mov bx, ax
    add bx, [selected_width]
    jc .failed
    cmp bx, SCREEN_WIDTH
    ja .failed
    cmp bx, ax
    jbe .failed
    mov ax, [selected_dst_y]
    cmp ax, SCREEN_HEIGHT
    jae .failed
    mov [pending_y], ax
    mov bx, ax
    add bx, [selected_height]
    jc .failed
    cmp bx, SCREEN_HEIGHT
    ja .failed
    cmp bx, ax
    jbe .failed
    mov ax, [selected_width]
    mov [pending_width], ax
    mov ax, [selected_height]
    mov [pending_height], ax
    mov al, [selected_scale_id]
    mov [pending_scale_id], al
    clc
    ret
.failed:
    stc
    ret

; Validate the complete committed footprint for the hidden physical page.
; Each entry is a logical half-open x/y/width/height rectangle in instance-ID
; order.  No SGP command is emitted by this pass.
validate_committed_footprint:
    mov al, [hidden_page_index]
    cmp al, PAGE_B
    ja .failed
    xor ah, ah
    mov bx, ax
    cmp byte [page_footprint_valid + bx], 0
    je .none
    xor ah, ah
    mov al, [page_footprint_count + bx]
    cmp ax, 1
    jb .failed
    cmp ax, FOOTPRINT_CAPACITY
    ja .failed
    mov cx, ax
    shl bx, FOOTPRINT_PAGE_SHIFT
    mov si, page_footprint_rects
    add si, bx
    mov word [dirty_sort_scan], 0
.rectangle:
    xor ax, ax
    mov al, [hidden_page_index]
    shl ax, FOOTPRINT_ID_PAGE_SHIFT
    add ax, [dirty_sort_scan]
    mov bx, ax
    xor ax, ax
    mov al, [page_footprint_instance_ids + bx]
    cmp ax, [dirty_sort_scan]
    jne .failed
    mov ax, [si]
    cmp ax, SCREEN_WIDTH
    jae .failed
    mov dx, [si + 4]
    test dx, dx
    jz .failed
    add dx, ax
    jc .failed
    cmp dx, SCREEN_WIDTH
    ja .failed
    cmp dx, ax
    jbe .failed
    mov ax, [si + 2]
    cmp ax, SCREEN_HEIGHT
    jae .failed
    mov dx, [si + 6]
    test dx, dx
    jz .failed
    add dx, ax
    jc .failed
    cmp dx, SCREEN_HEIGHT
    ja .failed
    cmp dx, ax
    jbe .failed
    add si, FOOTPRINT_RECT_BYTES
    inc word [dirty_sort_scan]
    loop .rectangle
.none:
    clc
    ret
.failed:
    inc word [bounds_failures]
    stc
    ret

; Build one canonical row union in bounded scratch.  Rounding occurs before
; sorting, and the optional accounting flag makes the second pass read-only
; with respect to the published counters.
build_dirty_row_union:
    mov word [dirty_candidate_count], 0
    mov word [dirty_merged_count], 0
    mov al, [hidden_page_index]
    xor ah, ah
    mov bx, ax
    mov cl, [page_footprint_count + bx]
    xor ch, ch
    shl bx, FOOTPRINT_PAGE_SHIFT
    mov si, page_footprint_rects
    add si, bx
    xor bx, bx
    mov di, dirty_interval_candidates
.candidate:
    cmp bx, cx
    jae .sort
    mov ax, [si + 2]
    cmp [dirty_row_y], ax
    jb .next_rectangle
    mov dx, [si + 6]
    add dx, ax
    cmp [dirty_row_y], dx
    jae .next_rectangle
    mov ax, [si]
    and ax, 0xfffe
    mov [di], ax
    mov ax, [si]
    add ax, [si + 4]
    inc ax
    and ax, 0xfffe
    mov [di + 2], ax
    mov ax, bx
    mov [di + 4], ax
    add di, DIRTY_INTERVAL_BYTES
    inc word [dirty_candidate_count]
.next_rectangle:
    add si, FOOTPRINT_RECT_BYTES
    inc bx
    jmp .candidate

.sort:
    mov ax, [dirty_candidate_count]
    cmp ax, 1
    jbe .merge_start
    mov word [dirty_sort_position], 1
.sort_outer:
    mov ax, [dirty_sort_position]
    mov dx, DIRTY_INTERVAL_BYTES
    mul dx
    mov si, dirty_interval_candidates
    add si, ax
    mov ax, [si]
    mov [dirty_next_x0], ax
    mov ax, [si + 2]
    mov [dirty_next_x1], ax
    mov ax, [si + 4]
    mov [dirty_next_id], ax
    mov ax, [dirty_sort_position]
    mov [dirty_sort_scan], ax
.sort_inner:
    cmp word [dirty_sort_scan], 0
    je .insert
    mov ax, [dirty_sort_scan]
    dec ax
    mov dx, DIRTY_INTERVAL_BYTES
    mul dx
    mov si, dirty_interval_candidates
    add si, ax
    mov ax, [si]
    cmp ax, [dirty_next_x0]
    jb .insert
    ja .shift
    mov ax, [si + 2]
    cmp ax, [dirty_next_x1]
    jb .insert
    ja .shift
    mov ax, [si + 4]
    cmp ax, [dirty_next_id]
    jbe .insert
.shift:
    mov ax, [dirty_sort_scan]
    mov dx, DIRTY_INTERVAL_BYTES
    mul dx
    mov di, dirty_interval_candidates
    add di, ax
    mov ax, [si]
    mov [di], ax
    mov ax, [si + 2]
    mov [di + 2], ax
    mov ax, [si + 4]
    mov [di + 4], ax
    dec word [dirty_sort_scan]
    jmp .sort_inner
.insert:
    mov ax, [dirty_sort_scan]
    mov dx, DIRTY_INTERVAL_BYTES
    mul dx
    mov di, dirty_interval_candidates
    add di, ax
    mov ax, [dirty_next_x0]
    mov [di], ax
    mov ax, [dirty_next_x1]
    mov [di + 2], ax
    mov ax, [dirty_next_id]
    mov [di + 4], ax
    inc word [dirty_sort_position]
    mov ax, [dirty_candidate_count]
    cmp [dirty_sort_position], ax
    jb .sort_outer

.merge_start:
    mov ax, [dirty_candidate_count]
    test ax, ax
    jz .done
    mov si, dirty_interval_candidates
    mov ax, [si]
    mov [dirty_current_x0], ax
    mov ax, [si + 2]
    mov [dirty_current_x1], ax
    mov ax, [si + 4]
    mov [dirty_current_id], ax
    mov word [dirty_sort_scan], 1
.merge_next:
    mov ax, [dirty_sort_scan]
    cmp ax, [dirty_candidate_count]
    jae .emit_final
    mov dx, DIRTY_INTERVAL_BYTES
    mul dx
    mov si, dirty_interval_candidates
    add si, ax
    mov ax, [si]
    mov [dirty_next_x0], ax
    mov ax, [si + 2]
    mov [dirty_next_x1], ax
    mov ax, [si + 4]
    mov [dirty_next_id], ax
    mov ax, [dirty_next_x0]
    cmp ax, [dirty_current_x1]
    ja .emit_and_start
    jb .overlap
    cmp byte [dirty_union_accounting], 0
    je .adjacent_done
    inc word [dirty_adjacency_merges]
.adjacent_done:
    jmp .extend
.overlap:
    cmp byte [dirty_union_accounting], 0
    je .overlap_done
    mov ax, [dirty_next_x1]
    cmp ax, [dirty_current_x1]
    jae .count_overlap
    inc word [dirty_containment_merges]
    jmp .overlap_done
.count_overlap:
    inc word [dirty_overlap_merges]
.overlap_done:
.extend:
    mov ax, [dirty_next_x1]
    cmp ax, [dirty_current_x1]
    jbe .merged_increment
    mov [dirty_current_x1], ax
.merged_increment:
    inc word [dirty_sort_scan]
    jmp .merge_next
.emit_and_start:
    call emit_dirty_merged_interval
    mov ax, [dirty_next_x0]
    mov [dirty_current_x0], ax
    mov ax, [dirty_next_x1]
    mov [dirty_current_x1], ax
    mov ax, [dirty_next_id]
    mov [dirty_current_id], ax
    inc word [dirty_sort_scan]
    jmp .merge_next
.emit_final:
    call emit_dirty_merged_interval
.done:
    ret

emit_dirty_merged_interval:
    mov ax, [dirty_merged_count]
    mov dx, DIRTY_INTERVAL_BYTES
    mul dx
    mov di, dirty_interval_merged
    add di, ax
    mov ax, [dirty_current_x0]
    mov [di], ax
    mov ax, [dirty_current_x1]
    mov [di + 2], ax
    mov ax, [dirty_current_id]
    mov [di + 4], ax
    inc word [dirty_merged_count]
    ret

; Validate every merged interval's physical row range during pass 1.  This
; deliberately performs no command-list submission or G1 write, so a malformed
; late row cannot be discovered after an earlier clear has already run.
validate_dirty_row_ranges:
    mov ax, [dirty_merged_count]
    cmp ax, FOOTPRINT_CAPACITY
    ja .failed
    mov bx, [hidden_page_index]
    cmp bx, PAGE_B
    ja .failed
    shl bx, 1
    mov ax, [dirty_row_y]
    cmp ax, SCREEN_HEIGHT
    jae .failed
    mul word [screen_pitch_word]
    add ax, [page_sgp_low + bx]
    adc dx, [page_sgp_high + bx]
    mov [dirty_row_address], ax
    mov [dirty_row_address + 2], dx
    mov ax, [page_sgp_low + bx]
    mov dx, [page_sgp_high + bx]
    add ax, G1_PAGE_BYTES & 0xffff
    adc dx, G1_PAGE_BYTES >> 16
    mov [dirty_interval_end_address], ax
    mov [dirty_interval_end_address + 2], dx
    mov si, dirty_interval_merged
    mov cx, [dirty_merged_count]
.interval:
    test cx, cx
    jz .done
    mov ax, [si]
    mov dx, [si + 2]
    cmp ax, SCREEN_WIDTH
    jae .failed
    cmp dx, SCREEN_WIDTH
    ja .failed
    test ax, 1
    jnz .failed
    test dx, 1
    jnz .failed
    cmp dx, ax
    jbe .failed
    mov ax, [dirty_row_address]
    mov dx, [dirty_row_address + 2]
    add ax, [si + 2]
    adc dx, 0
    cmp dx, [dirty_interval_end_address + 2]
    ja .failed
    jb .end_in_range
    cmp ax, [dirty_interval_end_address]
    ja .failed
.end_in_range:
    mov ax, [si + 2]
    sub ax, [si]
    test ax, 1
    jnz .failed
    shr ax, 1
    test ax, ax
    jz .failed
    add si, DIRTY_INTERVAL_BYTES
    dec cx
    jmp .interval
.done:
    clc
    ret
.failed:
    inc word [bounds_failures]
    stc
    ret

; Account and validate all rows without writing G1.  The resulting totals are
; recomputed by clear_hidden_footprint_rows before any CLS is submitted.
prepare_dirty_clear_state:
    mov byte [dirty_clear_needed], 0
    mov word [dirty_builder_failed], 0
    call validate_committed_footprint
    jc .failed
    mov al, [hidden_page_index]
    xor ah, ah
    mov bx, ax
    cmp byte [page_footprint_valid + bx], 0
    je .first_use
    mov byte [dirty_clear_needed], 1
    inc word [dirty_frames_with_clear]
    inc word [dirty_rect_clears]
    mov word [dirty_pass1_candidates], 0
    mov word [dirty_pass1_candidates + 2], 0
    mov word [dirty_pass1_merged], 0
    mov word [dirty_pass1_merged + 2], 0
    mov byte [dirty_union_accounting], 1
    xor ax, ax
.row:
    mov [dirty_row_y], ax
    inc word [dirty_rows_visited]
    call build_dirty_row_union
    call validate_dirty_row_ranges
    jc .failed
    mov ax, [dirty_candidate_count]
    add [dirty_pass1_candidates], ax
    adc word [dirty_pass1_candidates + 2], 0
    add [dirty_candidate_intervals], ax
    adc word [dirty_candidate_intervals + 2], 0
    mov ax, [dirty_merged_count]
    add [dirty_pass1_merged], ax
    adc word [dirty_pass1_merged + 2], 0
    add [dirty_merged_intervals], ax
    adc word [dirty_merged_intervals + 2], 0
    test ax, ax
    jz .next_row
    inc word [dirty_rows_nonempty]
.next_row:
    inc word [dirty_row_y]
    mov ax, [dirty_row_y]
    cmp word [dirty_row_y], SCREEN_HEIGHT
    jb .row
    mov byte [dirty_union_accounting], 0
    inc word [union_validation_passes]
    clc
    ret
.first_use:
    inc word [dirty_first_use_skips]
    clc
    ret
.failed:
    mov byte [dirty_union_accounting], 0
    stc
    ret

build_dirty_union_commands:
    push ax
    push dx
    push si
    push di
    push es
    push ds
    pop es
    mov di, sgp_command_list
    mov ax, SGP_COMMAND_SET_WORK
    stosw
    mov si, sgp_work_area
    call physical_address_from_ds_si
    stosw
    mov ax, dx
    stosw
    mov ax, SGP_COMMAND_SET_COLOR
    stosw
    xor ax, ax
    stosw
    mov cx, [dirty_batch_intervals]
    test cx, cx
    jz .invalid
    cmp cx, DIRTY_INTERVALS_PER_BATCH
    ja .invalid
    mov bx, [hidden_page_index]
    xor bh, bh
    shl bx, 1
    mov ax, [dirty_row_y]
    mul word [screen_pitch_word]
    add ax, [page_sgp_low + bx]
    adc dx, [page_sgp_high + bx]
    mov [dirty_row_address], ax
    mov [dirty_row_address + 2], dx
.interval:
    mov ax, [dirty_interval_cursor]
    mov dx, DIRTY_INTERVAL_BYTES
    mul dx
    mov si, dirty_interval_merged
    add si, ax
    mov ax, [si]
    add ax, [dirty_row_address]
    mov [dirty_next_x0], ax
    mov dx, [dirty_row_address + 2]
    adc dx, 0
    mov [dirty_next_x1], dx
    ; Validate the exclusive end against this hidden page before emitting the
    ; command.  Pass 1 has already checked the logical bounds; this catches
    ; address arithmetic and page-boundary regressions at the final builder.
    mov ax, [dirty_row_address]
    mov dx, [dirty_row_address + 2]
    add ax, [si + 2]
    adc dx, 0
    mov [dirty_interval_end_address], ax
    mov [dirty_interval_end_address + 2], dx
    mov ax, [page_sgp_low + bx]
    mov dx, [page_sgp_high + bx]
    add ax, G1_PAGE_BYTES & 0xffff
    adc dx, G1_PAGE_BYTES >> 16
    cmp word [dirty_interval_end_address + 2], dx
    ja .invalid
    jb .end_in_range
    cmp word [dirty_interval_end_address], ax
    ja .invalid
.end_in_range:
    mov ax, [si + 2]
    sub ax, [si]
    test ax, 1
    jnz .invalid
    shr ax, 1
    test ax, ax
    jz .invalid
    mov [dirty_words_per_row], ax
    ; The checked row/interval geometry is established by pass 1.  Emit the
    ; exact physical row address and exclusive word count once.
    mov ax, SGP_COMMAND_CLS
    stosw
    mov ax, [dirty_next_x0]
    stosw
    mov ax, [dirty_next_x1]
    stosw
    mov ax, [dirty_words_per_row]
    stosw
    xor ax, ax
    stosw
    inc word [dirty_interval_cursor]
    loop .interval
    mov ax, SGP_COMMAND_END
    stosw
    jmp finalize_sgp_command_list
.invalid:
    mov word [dirty_builder_failed], 1
    mov di, sgp_command_list
    mov ax, SGP_COMMAND_END
    stosw
    jmp finalize_sgp_command_list

; Optional bounded QA probe.  With no matching debug action this is a normal
; near-zero-cost call/return; a debug script may capture the row-union state.
dirty_union_checkpoint:
    ret

clear_hidden_footprint_rows:
    call dirty_union_checkpoint
    call prepare_dirty_clear_state
    jc .failed
    cmp byte [dirty_clear_needed], 0
    je .done
    mov word [dirty_pass2_candidates], 0
    mov word [dirty_pass2_candidates + 2], 0
    mov word [dirty_pass2_merged], 0
    mov word [dirty_pass2_merged + 2], 0
    xor ax, ax
.row:
    mov [dirty_row_y], ax
    call build_dirty_row_union
    mov ax, [dirty_candidate_count]
    add [dirty_pass2_candidates], ax
    adc word [dirty_pass2_candidates + 2], 0
    mov ax, [dirty_merged_count]
    add [dirty_pass2_merged], ax
    adc word [dirty_pass2_merged + 2], 0
    test ax, ax
    jz .next_row
    inc word [dirty_rows_cleared]
    mov word [dirty_interval_cursor], 0
.batch:
    mov ax, [dirty_merged_count]
    sub ax, [dirty_interval_cursor]
    jz .next_row
    cmp ax, DIRTY_INTERVALS_PER_BATCH
    jbe .batch_size
    mov ax, DIRTY_INTERVALS_PER_BATCH
.batch_size:
    mov [dirty_batch_intervals], ax
    mov ax, [dirty_interval_cursor]
    mov [dirty_batch_start], ax
    call build_dirty_union_commands
    cmp word [dirty_builder_failed], 0
    jne .failed
    call run_sgp_command_list
    jc .failed
    inc word [sgp_command_lists]
    inc word [sgp_batches]
    inc word [sgp_clear_batches]
    mov ax, [dirty_batch_intervals]
    add ax, 3
    add [sgp_commands], ax
    mov ax, [dirty_batch_intervals]
    add [dirty_row_cls_commands], ax
    adc word [dirty_row_cls_commands + 2], 0
    mov ax, [dirty_batch_start]
    mov dx, DIRTY_INTERVAL_BYTES
    mul dx
    mov si, dirty_interval_merged
    add si, ax
    mov cx, [dirty_batch_intervals]
.bytes:
    mov ax, [si + 2]
    sub ax, [si]
    shr ax, 1
    add [dirty_words_cleared], ax
    adc word [dirty_words_cleared + 2], 0
    shl ax, 1
    add [dirty_bytes_cleared], ax
    adc word [dirty_bytes_cleared + 2], 0
    add si, DIRTY_INTERVAL_BYTES
    loop .bytes
    jmp .batch
.next_row:
    mov ax, [dirty_row_y]
    inc ax
    mov [dirty_row_y], ax
    cmp ax, SCREEN_HEIGHT
    jb .row
    mov byte [dirty_union_accounting], 0
    mov ax, [dirty_pass1_candidates]
    cmp ax, [dirty_pass2_candidates]
    jne .mismatch
    mov ax, [dirty_pass1_candidates + 2]
    cmp ax, [dirty_pass2_candidates + 2]
    jne .mismatch
    mov ax, [dirty_pass1_merged]
    cmp ax, [dirty_pass2_merged]
    jne .mismatch
    mov ax, [dirty_pass1_merged + 2]
    cmp ax, [dirty_pass2_merged + 2]
    jne .mismatch
.done:
    clc
    ret

.mismatch:
    inc word [union_recompute_mismatches]
.failed:
    stc
    ret

; Commit all newly published logical rectangles to the physical page that was
; hidden during rendering.  The bounded copy is infallible after publication.
commit_pending_footprint:
    mov al, [hidden_page_index]
    xor ah, ah
    mov bx, ax
    mov byte [page_footprint_valid + bx], 0
    mov al, [build_active_count]
    mov [page_footprint_count + bx], al
    shl bx, FOOTPRINT_PAGE_SHIFT
    mov si, page_footprint_rects
    add si, bx
    mov byte [generation_instance], 0
.copy:
    mov al, [generation_instance]
    call record_pointer_from_index
    mov ax, [di + M98U_RECORD_DST_X]
    mov [si], ax
    mov ax, [di + M98U_RECORD_DST_Y]
    mov [si + 2], ax
    mov ax, [di + M98U_RECORD_WIDTH]
    mov [si + 4], ax
    mov ax, [di + M98U_RECORD_HEIGHT]
    mov [si + 6], ax
    xor ax, ax
    mov al, [hidden_page_index]
    shl ax, FOOTPRINT_ID_PAGE_SHIFT
    mov bx, ax
    xor ax, ax
    mov al, [generation_instance]
    add bx, ax
    mov al, [generation_instance]
    mov [page_footprint_instance_ids + bx], al
    add si, FOOTPRINT_RECT_BYTES
    inc byte [generation_instance]
    xor ax, ax
    mov al, [generation_instance]
    cmp ax, [build_active_count]
    jb .copy
    xor ax, ax
    mov al, [hidden_page_index]
    mov bx, ax
    mov al, [pending_global_phase]
    mov [page_footprint_phase + bx], al
    mov byte [page_footprint_valid + bx], 1
    inc word [page_footprint_commits]
    ret

render_hidden_page_to_ready:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    mov byte [runtime_failure_kind], 1
    mov al, [visible_page_index]
    cmp al, PAGE_B
    ja .state_failed
    cmp al, [hidden_page_index]
    je .state_failed
    mov al, [hidden_page_index]
    cmp al, PAGE_B
    ja .state_failed
    xor ah, ah
    mov si, ax
    mov bl, [page_state + si]
    cmp bl, PAGE_HIDDEN_CLEAN
    je .state_ready
    cmp bl, PAGE_HIDDEN_STALE
    jne .state_failed
.state_ready:
    call wait_sgp_idle
    jc .failed
    mov byte [page_state + si], PAGE_HIDDEN_RENDERING
    inc word [render_batches_started]
    inc word [complete_frames_started]
    mov ax, [build_active_count]
    add [instances_planned], ax
    add word [baseline_full_clear_words], G1_PAGE_WORD_COUNT
    adc word [baseline_full_clear_words + 2], 0
    add word [baseline_full_clear_bytes], G1_PAGE_BYTES & 0xffff
    adc word [baseline_full_clear_bytes + 2], G1_PAGE_BYTES >> 16
%if M98W_CLEAR_MODE = CLEAR_MODE_DIRTY
    ; M98w validates and clears only the hidden page's committed footprint.
    ; Bank selection is deliberately deferred until every clear batch is idle.
    call clear_hidden_footprint_rows
    jc .failed
%else
    call select_render_bms
    mov al, [hidden_page_index]
    call build_full_page_clear_commands
    call run_sgp_command_list
    jc .failed
    inc word [sgp_command_lists]
    inc word [sgp_batches]
    add word [sgp_commands], 3
    inc word [steady_full_page_clears]
    add word [full_page_clear_words], G1_PAGE_WORD_COUNT
    adc word [full_page_clear_words + 2], 0
    add word [full_page_clear_bytes], G1_PAGE_BYTES & 0xffff
    adc word [full_page_clear_bytes + 2], 0
%endif

%if M98W_CLEAR_MODE = CLEAR_MODE_DIRTY
    call select_render_bms
%endif

    mov byte [draw_position], 0
.draw_loop:
    xor bx, bx
    mov bl, [draw_position]
    mov al, [draw_order + bx]
    mov [current_draw_instance], al
    call load_instance_for_draw
    jc .state_failed
    mov al, [hidden_page_index]
    call build_bitblt_commands
    call run_sgp_command_list
    jc .failed
    inc word [sgp_command_lists]
    inc word [sgp_batches]
    inc word [sgp_draw_batches]
    add word [sgp_commands], 4
    inc word [instances_submitted]
    inc word [instances_completed]
    inc word [transparent_bitblts]
    mov ax, [selected_payload_bytes]
    add [source_bytes], ax
    adc word [source_bytes + 2], 0
    inc byte [draw_position]
    xor ax, ax
    mov al, [draw_position]
    cmp ax, [build_active_count]
    jb .draw_loop

    inc word [render_batches_completed]
    inc word [complete_frames_ready]
    mov al, [pending_global_phase]
    mov [pending_phase], al
    mov al, [hidden_page_index]
    xor ah, ah
    mov si, ax
    mov byte [page_state + si], PAGE_HIDDEN_COMPLETE
    call verify_staging_poison
    jc .sgp_error
    call select_render_ordinary
    call verify_normal_guards
    jc .sgp_error
    clc
    jmp .done
.sgp_error:
    inc word [sgp_errors]
.state_failed:
    mov byte [runtime_failure_kind], 1
.failed:
    stc
.done:
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

publish_ready_hidden_page:
    push ax
    push bx
    push si
    cmp byte [hidden_render_state], RENDER_READY
    jne .failed
    ; Count requests never relabel an in-flight frame.  The count tile is
    ; changed only when this complete frame is about to become visible.
    mov al, [build_active_count]
    cmp al, [published_active_count]
    je .count_ready
    call update_hud_count_field
    jc .failed
.count_ready:
    mov al, [hidden_page_index]
    call publish_page
    jc .failed
    ; The DSA1 write succeeded, so commit the complete pending footprint for
    ; this physical page before changing page roles.  The copy is bounded and
    ; cannot fail after publication.
    call commit_pending_footprint
    mov al, [hidden_page_index]
    xor ah, ah
    mov si, ax
    mov byte [page_state + si], PAGE_VISIBLE
    cmp al, PAGE_A
    jne .published_b
    inc word [page_a_publications]
    jmp .published
.published_b:
    inc word [page_b_publications]
.published:
    ; The old field is no longer needed once DSA1 accepted the page.  Until
    ; this point a failed publication can restore it from the committed
    ; visible count below.
    mov byte [hud_count_staged], 0
    mov al, [visible_page_index]
    xor ah, ah
    mov si, ax
    mov byte [page_state + si], PAGE_HIDDEN_STALE
    mov al, [visible_page_index]
    mov bl, [hidden_page_index]
    mov [visible_page_index], bl
    mov [hidden_page_index], al
    inc word [page_flips]
    inc word [published_updates]
    inc word [published_frames]
    inc word [complete_frames_published]
    mov ax, [build_active_count]
    add [instances_published], ax
    call record_count_publication
    mov al, [pending_global_phase]
    mov [last_published_global_phase], al
    mov [last_published_phase], al
    mov al, [build_active_count]
    mov [published_active_count], al
    call draw_hud_status
    call record_multi_frame_publication
    mov byte [hidden_render_state], RENDER_IDLE
    call qa_after_publication
    clc
    jmp .done
.failed:
    cmp byte [hud_count_staged], 0
    je .failed_no_hud
    call restore_hud_count_field
    inc word [hud_count_rollbacks]
    mov byte [hud_count_staged], 0
.failed_no_hud:
    stc
.done:
    pop si
    pop bx
    pop ax
    ret

; Commit count state only after DSA1 publication has succeeded.  The old
; visible count is retained for the transition matrix and the HUD is therefore
; never allowed to describe an in-flight frame.
record_count_publication:
    push ax
    push bx
    push dx
    push si
    mov al, [published_active_count]
    cmp al, [build_active_count]
    je .same
    inc word [count_change_publications]
    inc word [runtime_count_changes]
    mov al, [build_active_count]
    cmp al, [published_active_count]
    jbe .decrease
    inc word [count_increase_applies]
    jmp .record
.decrease:
    inc word [count_decrease_applies]
    jmp .record
.same:
    inc word [same_count_publications]
.record:
    xor ax, ax
    mov al, [build_active_count]
    cmp al, 1
    jb .failed
    cmp al, FOOTPRINT_CAPACITY
    ja .failed
    dec ax
    mov si, ax
    shl si, 1
    inc word [count_publications + si]
    xor ax, ax
    mov al, [published_active_count]
    cmp al, 1
    jb .failed
    cmp al, FOOTPRINT_CAPACITY
    ja .failed
    dec ax
%if M98Y_PRIVATE_PROFILE
    shl ax, 1
    shl ax, 1
%endif
    shl ax, 1
    shl ax, 1
    shl ax, 1
    shl ax, 1
    xor bx, bx
    mov bl, [build_active_count]
    dec bx
    add ax, bx
    shl ax, 1
    mov si, ax
%if M98Y_PRIVATE_PROFILE
    ; The 64-instance private candidate keeps bounded aggregate count
    ; telemetry; the legacy 16x16 transition matrix is intentionally not
    ; resident in the 64-KiB COM image.
%else
    inc word [count_transition_publications + si]
%endif
    mov al, [build_active_count]
    mov [visible_published_count], al
    mov ax, [pending_count_generation]
    mov [published_count_generation], ax
    clc
    jmp .done
.failed:
    inc word [hud_mismatches]
    stc
.done:
    pop si
    pop dx
    pop bx
    pop ax
    ret

record_multi_frame_publication:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    xor ax, ax
    mov al, [pending_global_phase]
    mov si, ax
    shl si, 1
    inc word [phase_publications + si]
    inc word [phase_publication_total]
    xor bx, bx
.record:
    mov al, bl
    push bx
    call record_pointer_from_index
    pop bx
    xor ax, ax
    mov al, [di + M98U_RECORD_SCALE_ID]
    dec ax
    mov si, ax
    shl si, 1
    inc word [scale_publications + si]
    inc word [scale_publication_total]
    cmp byte [di + M98U_RECORD_PHASE_ID], 32
    jae .far
    inc word [near_publications]
    jmp .next
.far:
    inc word [far_publications]
.next:
    inc bx
    mov ax, [build_active_count]
    cmp bx, ax
    jb .record
    xor ax, ax
    mov al, [pending_global_phase]
    mov ah, [published_active_count]
    add [publication_digest], ax
    adc word [publication_digest + 2], 0
    xor ax, ax
    mov al, [visible_page_index]
    inc ax
    add [publication_digest + 2], ax
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

select_render_bms:
    inc word [bms_bank_selections]
    mov dx, PORT_BMS_SELECTOR
    in al, dx
    cmp al, BMS_FIRST_SELECTOR
    je .done
    mov al, BMS_FIRST_SELECTOR
    out dx, al
    inc word [bms_bank_switches]
.done:
    ret

record_orbit_publication:
    push ax
    push bx
    push cx
    push dx
    push si
    xor ax, ax
    mov al, [pending_phase]
    mov si, ax
    shl si, 1
    inc word [phase_publications + si]
    inc word [phase_publication_total]
    xor ax, ax
    mov al, [pending_scale_id]
    dec ax
    mov si, ax
    shl si, 1
    inc word [scale_publications + si]
    inc word [scale_publication_total]
    cmp byte [pending_phase], 32
    jae .far
    inc word [near_publications]
    jmp .half_counted
.far:
    inc word [far_publications]
.half_counted:

    mov ax, [selected_payload_bytes]
    add [source_bytes], ax
    adc word [source_bytes + 2], 0

    xor ax, ax
    mov al, [pending_phase]
    mov ah, [pending_scale_id]
    add [publication_digest], ax
    adc word [publication_digest + 2], 0
    xor ax, ax
    mov al, [visible_page_index]
    inc ax
    add [publication_digest + 2], ax
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

advance_orbit_phase:
    ; The Q8.8 accumulator advances only after a complete publication.  The
    ; lookup phase is its integer high byte, preserving the accepted modulo
    ; 64 orbit at the default 1.00X increment.
    cmp byte [paused], 0
    jne .done
    inc word [phase_advances]
    mov bl, [orbit_phase_next]
    xor bh, bh
    mov ax, [phase_accumulator]
    add ax, [active_speed_increment]
    and ax, 0x3fff
    mov [phase_accumulator], ax
    mov [orbit_phase_next], ah
    cmp ah, bl
    jae .done
    inc word [revolution_wraps]
.done:
    ret

; Copy requested speed/distance/look/radius controls into the immutable
; snapshot used by the next frame.  The range checks are deliberately kept in
; one place so malformed state cannot reach descriptor or geometry math.
apply_requested_projection_state:
    push ax
    push bx
    push cx
    push dx
    push si
    cmp byte [requested_speed_index], SPEED_MAX
    ja .failed
    cmp byte [requested_distance_bias], DISTANCE_MIN
    jl .failed
    cmp byte [requested_distance_bias], DISTANCE_MAX
    jg .failed
    cmp byte [requested_look_level], LOOK_MIN
    jl .failed
    cmp byte [requested_look_level], LOOK_MAX
    jg .failed
    cmp byte [requested_radius_index], RADIUS_MAX
    ja .failed
    xor si, si
    mov al, [requested_speed_index]
    cmp al, [active_speed_index]
    je .speed_ready
    mov [active_speed_index], al
    inc word [speed_changes_applied]
.speed_ready:
    xor bx, bx
    mov bl, [active_speed_index]
    shl bx, 1
    mov ax, [orbit_speed_increments + bx]
    mov [active_speed_increment], ax
    mov al, [requested_distance_bias]
    cmp al, [active_distance_bias]
    je .distance_ready
    mov [active_distance_bias], al
    inc word [distance_changes_applied]
    mov si, 1
.distance_ready:
    mov al, [requested_look_level]
    cmp al, [active_look_level]
    je .look_ready
    mov [active_look_level], al
    inc word [look_changes_applied]
    mov si, 1
.look_ready:
    mov al, [requested_radius_index]
    cmp al, [active_radius_index]
    je .radius_ready
    mov [active_radius_index], al
    inc word [radius_changes_applied]
    mov si, 1
.radius_ready:
    xor bx, bx
    mov bl, [active_radius_index]
    shl bx, 1
    mov ax, [orbit_radius_factors + bx]
    mov [active_radius_factor], ax
    xor ax, ax
    mov al, [active_look_level]
    cbw
    mov bx, 4
    imul bx
    mov [screen_y_bias], ax
    cmp byte [paused], 0
    je .ok
    cmp si, 0
    je .ok
    inc word [paused_geometry_redraws]
.ok:
    clc
    jmp .done
.failed:
    inc word [bounds_failures]
    stc
.done:
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

validate_m98v_bounded_success:
    mov word [validation_failure_code], 0xffff
    cmp word [build_active_count], M98V_ACTIVE_COUNT
    jne .failed
    cmp word [render_batches_started], QA_PUBLICATIONS
    jne .failed
    cmp word [render_batches_completed], QA_PUBLICATIONS
    jne .failed
    cmp word [complete_frames_started], QA_PUBLICATIONS
    jne .failed
    cmp word [complete_frames_ready], QA_PUBLICATIONS
    jne .failed
    cmp word [complete_frames_published], QA_PUBLICATIONS
    jne .failed
    cmp word [published_frames], QA_PUBLICATIONS
    jne .failed
    cmp word [initial_full_page_clears], 2
    jne .failed
    mov word [validation_failure_code], 1
%if M98W_CLEAR_MODE = CLEAR_MODE_DIRTY
    cmp word [steady_full_page_clears], 0
    jne .failed
    cmp word [dirty_rect_clears], QA_PUBLICATIONS - 2
    jne .failed
    cmp word [dirty_first_use_skips], 2
    jne .failed
    cmp word [dirty_frames_with_clear], QA_PUBLICATIONS - 2
    jne .failed
    cmp word [dirty_row_cls_commands], 0
    je .failed
    mov word [validation_failure_code], 2
    ; Exact union totals and row/byte identities are checked by the independent
    ; host oracle; the guest retains the nonzero/safe lower-bound proof here.
%else
    cmp word [steady_full_page_clears], QA_PUBLICATIONS
    jne .failed
    cmp word [dirty_rect_clears], 0
    jne .failed
    cmp word [dirty_row_cls_commands], 0
    jne .failed
    cmp word [dirty_row_cls_commands + 2], 0
    jne .failed
%endif
    cmp word [transparent_bitblts], QA_PUBLICATIONS * M98V_ACTIVE_COUNT
    jne .failed
    cmp word [instances_planned], QA_PUBLICATIONS * M98V_ACTIVE_COUNT
    jne .failed
    cmp word [instances_submitted], QA_PUBLICATIONS * M98V_ACTIVE_COUNT
    jne .failed
    cmp word [instances_completed], QA_PUBLICATIONS * M98V_ACTIVE_COUNT
    jne .failed
    cmp word [instances_published], QA_PUBLICATIONS * M98V_ACTIVE_COUNT
    jne .failed
    mov word [validation_failure_code], 3
    cmp word [page_flips], QA_PUBLICATIONS
    jne .failed
    cmp word [published_updates], QA_PUBLICATIONS
    jne .failed
    cmp word [phase_advances], QA_PUBLICATIONS
    jne .failed
    mov ax, [published_updates]
    add ax, [missed_slots]
    cmp ax, [requested_slots]
    jne .failed
    cmp word [partial_publication_attempts], 0
    jne .failed
    cmp word [draw_order_failures], 0
    jne .failed
    cmp word [tie_break_failures], 0
    jne .failed
    cmp word [sgp_timeouts], 0
    jne .failed
    cmp word [sgp_errors], 0
    jne .failed
    cmp word [vblank_timeouts], 0
    jne .failed
    cmp word [descriptor_errors], 0
    jne .failed
    cmp word [bounds_failures], 0
    jne .failed
    cmp word [source_failures], 0
    jne .failed
    cmp word [guard_failures], 0
    jne .failed
    cmp word [framebuffer_mismatches], 0
    jne .failed
    cmp word [runtime_count_changes], 0
    jne .failed
    mov word [validation_failure_code], 4
    cmp word [revolution_wraps], QA_CYCLES
    jne .failed
    cmp word [phase_publication_total], QA_PUBLICATIONS
    jne .failed
    cmp word [scale_publication_total], QA_PUBLICATIONS * M98V_ACTIVE_COUNT
    jne .failed
    cmp word [near_publications], (QA_PUBLICATIONS * M98V_ACTIVE_COUNT) / 2
    jne .failed
    cmp word [far_publications], (QA_PUBLICATIONS * M98V_ACTIVE_COUNT) / 2
    jne .failed
    mov word [validation_failure_code], 5
%if M98W_CLEAR_MODE = CLEAR_MODE_FULL
    cmp word [sgp_command_lists], 1 + QA_PUBLICATIONS * (1 + M98V_ACTIVE_COUNT)
    jne .failed
    cmp word [sgp_batches], QA_PUBLICATIONS * (1 + M98V_ACTIVE_COUNT)
    jne .failed
    cmp word [sgp_commands], 5 + QA_PUBLICATIONS * (3 + 4 * M98V_ACTIVE_COUNT)
    jne .failed
%else
    cmp word [union_recompute_mismatches], 0
    jne .failed
    cmp word [sgp_clear_batches], 0
    je .failed
%endif
    cmp word [hud_full_initializations], 1
    jne .failed
    mov ax, [divider_changes_applied]
    inc ax
    cmp ax, [hud_fps_field_updates]
    jne .failed
    cmp word [hud_zundamon_field_updates], 1
    jne .failed
    cmp word [hud_count_field_updates], 1
    jne .failed
    cmp word [hud_g1_writes], 0
    jne .failed
    cmp word [hud_vblank_overruns], 0
    jne .failed
    cmp word [hud_mismatches], 0
    jne .failed
    cmp word [hud_runtime_failure], 0
    jne .failed
    mov word [validation_failure_code], 6
    mov ax, [divider_changes_applied]
    mov bx, HUD_FPS_WRITE_BYTES
    mul bx
    add ax, HUD_FULL_WRITE_BYTES + HUD_COUNT_WRITE_BYTES
    adc dx, 0
    cmp ax, [hud_bytes_written]
    jne .failed
    cmp dx, [hud_bytes_written + 2]
    jne .failed
%if M98W_CLEAR_MODE = CLEAR_MODE_FULL
    cmp word [full_page_clear_words], (QA_PUBLICATIONS * G1_PAGE_WORD_COUNT) & 0xffff
    jne .failed
    cmp word [full_page_clear_words + 2], (QA_PUBLICATIONS * G1_PAGE_WORD_COUNT) >> 16
    jne .failed
%else
    cmp word [full_page_clear_words], 0
    jne .failed
    cmp word [full_page_clear_words + 2], 0
    jne .failed
%endif
%if M98W_CLEAR_MODE = CLEAR_MODE_FULL
    cmp word [full_page_clear_bytes], (QA_PUBLICATIONS * G1_PAGE_BYTES) & 0xffff
    jne .failed
    cmp word [full_page_clear_bytes + 2], (QA_PUBLICATIONS * G1_PAGE_BYTES) >> 16
    jne .failed
%else
    cmp word [full_page_clear_bytes], 0
    jne .failed
    cmp word [full_page_clear_bytes + 2], 0
    jne .failed
%endif
    mov word [validation_failure_code], 7
    xor si, si
    mov cx, ORBIT_PUBLICATIONS_PER_REVOLUTION
.phase_loop:
    cmp word [phase_publications + si], QA_CYCLES
    jne .failed
    add si, 2
    loop .phase_loop
    xor si, si
    mov cx, ATLAS_SCALE_COUNT
.scale_loop:
    xor ax, ax
    mov al, [expected_scale_histogram + si]
    mov bx, QA_CYCLES * M98V_ACTIVE_COUNT
    mul bx
    mov di, si
    shl di, 1
    cmp ax, [scale_publications + di]
    jne .failed
    inc si
    loop .scale_loop
    mov word [validation_failure_code], 8
    mov word [bounded_validation_pass], 1
    clc
    ret
.failed:
    stc
    ret

validate_bounded_success:
    cmp word [render_batches_started], QA_PUBLICATIONS
    jne .failed
    cmp word [render_batches_completed], QA_PUBLICATIONS
    jne .failed
    cmp word [initial_full_page_clears], 2
    jne .failed
%if M98Q_CLEAR_MODE = CLEAR_MODE_DIRTY
    cmp word [steady_full_page_clears], 0
    jne .failed
    cmp word [dirty_rect_clears], QA_PUBLICATIONS - 2
    jne .failed
    mov ax, [dirty_words_cleared + 2]
    cmp ax, [baseline_full_clear_words + 2]
    jb .dirty_volume_ok
    ja .failed
    mov ax, [dirty_words_cleared]
    cmp ax, [baseline_full_clear_words]
    jae .failed
.dirty_volume_ok:
%else
    cmp word [steady_full_page_clears], QA_PUBLICATIONS
    jne .failed
    cmp word [dirty_rect_clears], 0
    jne .failed
    cmp word [dirty_row_cls_commands], 0
    jne .failed
    cmp word [dirty_row_cls_commands + 2], 0
    jne .failed
%endif
    cmp word [transparent_bitblts], QA_PUBLICATIONS
    jne .failed
    cmp word [page_flips], QA_PUBLICATIONS
    jne .failed
    cmp word [published_updates], QA_PUBLICATIONS
    jne .failed
    cmp word [phase_advances], QA_PUBLICATIONS
    jne .failed
    mov ax, [published_updates]
    add ax, [missed_slots]
    cmp ax, [requested_slots]
    jne .failed
    cmp byte [active_divisor], CADENCE_MIN
    jb .failed
    cmp byte [active_divisor], CADENCE_MAX
    ja .failed
    cmp byte [requested_divisor], CADENCE_MIN
    jb .failed
    cmp byte [requested_divisor], CADENCE_MAX
    ja .failed
    cmp word [partial_publication_attempts], 0
    jne .failed
    cmp word [revolution_wraps], QA_CYCLES
    jne .failed
    cmp word [scale_changes], 58 * QA_CYCLES - 1
    jne .failed
    cmp word [sgp_timeouts], 0
    jne .failed
    cmp word [sgp_errors], 0
    jne .failed
    cmp word [vblank_timeouts], 0
    jne .failed
    cmp word [descriptor_errors], 0
    jne .failed
    cmp word [bounds_failures], 0
    jne .failed
    cmp word [dirty_full_mismatches], 0
    jne .failed
    cmp word [guard_failures], 0
    jne .failed
    cmp word [baseline_full_clear_words], (QA_PUBLICATIONS * G1_PAGE_WORD_COUNT) & 0xffff
    jne .failed
    cmp word [baseline_full_clear_words + 2], (QA_PUBLICATIONS * G1_PAGE_WORD_COUNT) >> 16
    jne .failed
    cmp word [baseline_full_clear_bytes], (QA_PUBLICATIONS * G1_PAGE_BYTES) & 0xffff
    jne .failed
    cmp word [baseline_full_clear_bytes + 2], (QA_PUBLICATIONS * G1_PAGE_BYTES) >> 16
    jne .failed
    cmp word [phase_publication_total], QA_PUBLICATIONS
    jne .failed
    cmp word [scale_publication_total], QA_PUBLICATIONS
    jne .failed
    cmp word [table_scale_change_edges], 58
    jne .failed
    cmp word [near_publications], QA_PUBLICATIONS / 2
    jne .failed
    cmp word [far_publications], QA_PUBLICATIONS / 2
    jne .failed
    cmp byte [selected_scale_id], 15
    jne .failed
    cmp byte [pending_scale_id], 15
    jne .failed
    cmp byte [pending_depth_rank], -1
    jne .failed
    cmp word [hud_full_initializations], 1
    jne .failed
    mov ax, [divider_changes_applied]
    inc ax
    cmp ax, [hud_fps_field_updates]
    jne .failed
    cmp word [hud_zundamon_field_updates], 1
    jne .failed
    cmp word [hud_g1_writes], 0
    jne .failed
    cmp word [hud_vblank_overruns], 0
    jne .failed
    cmp word [hud_mismatches], 0
    jne .failed
    cmp word [hud_runtime_failure], 0
    jne .failed
    mov ax, [divider_changes_applied]
    mov bx, HUD_FPS_WRITE_BYTES
    mul bx
    add ax, HUD_FULL_WRITE_BYTES
    adc dx, 0
    cmp ax, [hud_bytes_written]
    jne .failed
    cmp dx, [hud_bytes_written + 2]
    jne .failed
    xor si, si
    mov cx, ORBIT_PUBLICATIONS_PER_REVOLUTION
.phase_loop:
    cmp word [phase_publications + si], QA_CYCLES
    jne .failed
.next:
    add si, 2
    loop .phase_loop
    xor si, si
    mov cx, ATLAS_SCALE_COUNT
.scale_loop:
    xor ax, ax
    mov al, [expected_scale_histogram + si]
    mov bx, QA_CYCLES
    mul bx
    mov di, si
    shl di, 1
    cmp ax, [scale_publications + di]
    jne .failed
    inc si
    loop .scale_loop
    mov word [bounded_validation_pass], 1
    clc
    ret
.failed:
    stc
    ret

; Parse the complete PSP tail before graphics mode.  M98x accepts at most
; one exact /V1..8 and one exact /N token in either order.  Public builds
; accept /N1..16; the private IDA64 profile widens that same grammar to
; /N1..64 without changing token-boundary or duplicate rejection rules.
; retain the old fixed-count contract and reject /N entirely.
parse_cadence_option:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push es
    mov byte [parsed_divisor], CADENCE_MIN
    mov byte [parsed_count], 4
    mov byte [cadence_option_seen], 0
    mov byte [count_option_seen], 0
    mov ax, [psp_segment]
    mov es, ax
    xor cx, cx
    mov cl, [es:0x0080]
    mov si, 0x0081
.skip_space:
    test cx, cx
    jz .success
    mov al, [es:si]
    cmp al, ' '
    je .space
    cmp al, 9
    je .space
    xor dx, dx
    mov di, si
.scan_token:
    cmp dx, cx
    jae .token_end
    mov al, [es:di]
    cmp al, ' '
    je .token_end
    cmp al, 9
    je .token_end
    inc dx
    inc di
    jmp .scan_token
.token_end:
    cmp dx, 2
    jb .failed
    cmp byte [es:si], '/'
    jne .failed
    mov al, [es:si + 1]
    and al, 0xdf
    cmp al, 'V'
    je .token_v
    cmp al, 'N'
    jne .failed
%if M98X_RUNTIME_MODE
    cmp byte [count_option_seen], 0
    jne .failed
%if !M98Y_PRIVATE_PROFILE
    cmp dx, 3
    je .count_one_digit
    cmp dx, 4
    jne .failed
    cmp byte [es:si + 2], '1'
    jne .failed
    mov al, [es:si + 3]
    cmp al, '0'
    jb .failed
    cmp al, '6'
    ja .failed
    sub al, '0'
    add al, 10
    jmp .count_store
%else
    cmp dx, 3
    je .count_one_digit
    cmp dx, 4
    jne .failed
    mov al, [es:si + 2]
    cmp al, '1'
    jb .failed
    cmp al, '6'
    ja .failed
    mov ah, [es:si + 3]
    cmp ah, '0'
    jb .failed
    cmp ah, '9'
    ja .failed
    cmp al, '6'
    jne .count_two_digit_ok
    cmp ah, '4'
    ja .failed
.count_two_digit_ok:
    mov al, [es:si + 2]
    sub al, '0'
    mov ah, 10
    mul ah
    add al, [es:si + 3]
    sub al, '0'
    jmp .count_store
%endif
.count_one_digit:
    mov al, [es:si + 2]
    cmp al, '1'
    jb .failed
    cmp al, '9'
    ja .failed
    sub al, '0'
.count_store:
    mov [parsed_count], al
    mov byte [count_option_seen], 1
    jmp .token_done
%else
    jmp .failed
%endif
.token_v:
    cmp byte [cadence_option_seen], 0
    jne .failed
    cmp dx, 3
    jne .failed
    mov al, [es:si + 2]
    cmp al, '1'
    jb .failed
    cmp al, '8'
    ja .failed
    sub al, '0'
    mov [parsed_divisor], al
    mov byte [cadence_option_seen], 1
.token_done:
    sub cx, dx
    add si, dx
    jmp .skip_space
.space:
    inc si
    dec cx
    jmp .skip_space
.failed:
    stc
    jmp .done
.success:
    clc
.done:
    pop es
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

initialize_count_state:
%if M98X_RUNTIME_MODE
    xor ax, ax
    mov al, [parsed_count]
%else
    mov ax, M98V_ACTIVE_COUNT
%endif
    cmp ax, 1
    jb .failed
    cmp ax, FOOTPRINT_CAPACITY
    ja .failed
    mov [build_active_count], ax
    mov [requested_count], al
    mov [next_render_count], al
    mov [pending_render_count], al
    mov [visible_published_count], al
    mov [published_active_count], al
    mov word [count_request_generation], 0
    mov word [pending_count_generation], 0
    mov word [published_count_generation], 0
    mov byte [count_change_pending], 0
    clc
    ret
.failed:
    stc
    ret

initialize_cadence_scheduler:
    mov al, [parsed_divisor]
    mov [active_divisor], al
    mov [requested_divisor], al
    mov byte [divider_count], 0
    mov byte [paused], 0
    mov byte [pause_toggle_pending], 0
    mov byte [exit_requested], 0
    mov byte [eligible_publication], 0
    mov byte [hidden_render_state], RENDER_IDLE
    mov dx, PORT_TSP_STATUS
    in al, dx
    and al, TSP_STATUS_VBLANK
    mov [vblank_last_high], al
    mov byte [scheduler_active], 1
    ret

poll_control_requests:
%if M98Y_PRIVATE_PROFILE
    ; Preserve caller state across the keyboard BIOS boundary.  Private
    ; rendering loops keep SGP ports and batch counters live across polling.
    pushf
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    push ds
    push es
%else
    push ax
%endif
    mov ah, 0x0a
    int KEYBOARD_BIOS_INT
%if M98Y_PRIVATE_PROFILE
    push cs
    pop ds
%endif
    jc .done
    mov ah, 0x09
    int KEYBOARD_BIOS_INT
%if M98Y_PRIVATE_PROFILE
    push cs
    pop ds
%endif
    cmp ah, KEY_SCAN_ESCAPE
    jne .check_left
    cmp al, KEY_INTERNAL_ESCAPE
    je .escape
.check_left:
    cmp ah, KEY_SCAN_LEFT
    je .faster
    cmp ah, KEY_SCAN_RIGHT
    je .slower
    cmp ah, KEY_SCAN_A
    je .speed_up
    cmp ah, KEY_SCAN_Z
    je .speed_down
    cmp ah, KEY_SCAN_Q
    je .distance_near
    cmp ah, KEY_SCAN_E
    je .distance_far
    cmp ah, KEY_SCAN_W
    je .look_up
    cmp ah, KEY_SCAN_S
    je .look_down
    cmp ah, KEY_SCAN_O
    je .radius_up
    cmp ah, KEY_SCAN_P
    je .radius_down
%if M98X_RUNTIME_MODE
    cmp ah, KEY_SCAN_UP
    je .count_up
    cmp ah, KEY_SCAN_UP_EXTENDED
    je .count_up
    cmp ah, KEY_SCAN_DOWN
    je .count_down
    cmp ah, KEY_SCAN_DOWN_EXTENDED
    je .count_down
%endif
    cmp ah, KEY_SCAN_SPACE
    je .pause
    jmp .done
.escape:
    mov byte [exit_requested], 1
    jmp .done
.faster:
    cmp byte [requested_divisor], CADENCE_MIN
    jbe .clamped
    dec byte [requested_divisor]
    inc word [divider_change_requests]
    jmp .done
.slower:
    cmp byte [requested_divisor], CADENCE_MAX
    jae .clamped
    inc byte [requested_divisor]
    inc word [divider_change_requests]
    jmp .done
.speed_up:
    cmp byte [requested_speed_index], SPEED_MAX
    jae .projection_saturated
    inc byte [requested_speed_index]
    inc word [speed_change_requests]
    jmp .done
.speed_down:
    cmp byte [requested_speed_index], SPEED_MIN
    jbe .projection_saturated
    dec byte [requested_speed_index]
    inc word [speed_change_requests]
    jmp .done
.distance_near:
    cmp byte [requested_distance_bias], DISTANCE_MAX
    jae .projection_saturated
    inc byte [requested_distance_bias]
    inc word [distance_change_requests]
    jmp .done
.distance_far:
    cmp byte [requested_distance_bias], DISTANCE_MIN
    jle .projection_saturated
    dec byte [requested_distance_bias]
    inc word [distance_change_requests]
    jmp .done
.look_up:
    cmp byte [requested_look_level], LOOK_MAX
    jae .projection_saturated
    inc byte [requested_look_level]
    inc word [look_change_requests]
    jmp .done
.look_down:
    cmp byte [requested_look_level], LOOK_MIN
    jle .projection_saturated
    dec byte [requested_look_level]
    inc word [look_change_requests]
    jmp .done
.radius_up:
    cmp byte [requested_radius_index], RADIUS_MAX
    jae .projection_saturated
    inc byte [requested_radius_index]
    inc word [radius_change_requests]
    jmp .done
.radius_down:
    cmp byte [requested_radius_index], RADIUS_MIN
    jbe .projection_saturated
    dec byte [requested_radius_index]
    inc word [radius_change_requests]
    jmp .done
.projection_saturated:
    inc word [saturated_key_requests]
    jmp .done
.pause:
    xor byte [pause_toggle_pending], 1
    inc word [pause_requests]
    jmp .done
%if M98X_RUNTIME_MODE
.count_up:
    inc word [count_key_requests]
    inc word [count_up_requests]
    cmp byte [requested_count], FOOTPRINT_CAPACITY
    jae .count_saturated
    inc byte [requested_count]
    inc word [count_request_generation]
    inc word [count_request_events]
    cmp byte [hidden_render_state], RENDER_IDLE
    je .count_up_latched
    inc word [count_requests_coalesced]
.count_up_latched:
    mov byte [count_change_pending], 1
    jmp .done
.count_down:
    inc word [count_key_requests]
    inc word [count_down_requests]
    cmp byte [requested_count], 1
    jbe .count_saturated
    dec byte [requested_count]
    inc word [count_request_generation]
    inc word [count_request_events]
    cmp byte [hidden_render_state], RENDER_IDLE
    je .count_down_latched
    inc word [count_requests_coalesced]
.count_down_latched:
    mov byte [count_change_pending], 1
    jmp .done
.count_saturated:
    inc word [count_noop_saturations]
    jmp .done
%endif
.clamped:
    inc word [control_endpoint_hits]
.done:
%if M98Y_PRIVATE_PROFILE
    pop es
    pop ds
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
%endif
    pop ax
%if M98Y_PRIVATE_PROFILE
    popf
%endif
    ret

observe_vblank_sample:
    push ax
    push dx
    cmp byte [scheduler_active], 0
    je .no_edge
    mov dx, PORT_TSP_STATUS
    in al, dx
    test al, TSP_STATUS_VBLANK
    jz .low
    cmp byte [vblank_last_high], 0
    jne .no_edge
    mov byte [vblank_last_high], TSP_STATUS_VBLANK
    call process_scheduler_edge
    pop dx
    pop ax
    stc
    ret
.low:
    mov byte [vblank_last_high], 0
.no_edge:
    pop dx
    pop ax
    clc
    ret

%if M98AA_AUTO_CAMERA
; The private IDA64 candidate has a deterministic camera demonstration mode.
; Every fifteen VBLANK edges (a bounded quarter-second cadence at the nominal
; model), speed advances by one 0.25X step in a triangle from 1.00X to 4.00X,
; distance walks between -4 and +4, and radius walks across every 0.125X
; factor from 0.50X to 1.50X.  Requests remain separate from the immutable
; frame snapshot, so a busy SGP transaction is never relabelled.
advance_auto_camera:
    cmp byte [paused], 0
    jne .done
    inc word [auto_camera_vblank_ticks]
    cmp word [auto_camera_vblank_ticks], 15
    jb .done
    mov word [auto_camera_vblank_ticks], 0

    cmp byte [auto_speed_direction], 0
    je .speed_down
    cmp byte [requested_speed_index], SPEED_MAX
    jae .speed_reverse
    inc byte [requested_speed_index]
    inc word [speed_change_requests]
    jmp .distance
.speed_reverse:
    mov byte [auto_speed_direction], 0
    dec byte [requested_speed_index]
    inc word [speed_change_requests]
    jmp .distance
.speed_down:
    cmp byte [requested_speed_index], SPEED_MIN
    jbe .speed_forward
    dec byte [requested_speed_index]
    inc word [speed_change_requests]
    jmp .distance
.speed_forward:
    mov byte [auto_speed_direction], 1
    inc byte [requested_speed_index]
    inc word [speed_change_requests]

.distance:
    cmp byte [auto_distance_direction], 0
    je .distance_down
    cmp byte [requested_distance_bias], DISTANCE_MAX
    jae .distance_reverse
    inc byte [requested_distance_bias]
    inc word [distance_change_requests]
    jmp .radius
.distance_reverse:
    mov byte [auto_distance_direction], 0
    dec byte [requested_distance_bias]
    inc word [distance_change_requests]
    jmp .radius
.distance_down:
    cmp byte [requested_distance_bias], DISTANCE_MIN
    jbe .distance_forward
    dec byte [requested_distance_bias]
    inc word [distance_change_requests]
    jmp .radius
.distance_forward:
    mov byte [auto_distance_direction], 1
    inc byte [requested_distance_bias]
    inc word [distance_change_requests]
    jmp .radius
.radius:
    cmp byte [auto_radius_direction], 0
    je .radius_down
    cmp byte [requested_radius_index], RADIUS_MAX
    jae .radius_reverse
    inc byte [requested_radius_index]
    inc word [radius_change_requests]
    jmp .done
.radius_reverse:
    mov byte [auto_radius_direction], 0
    dec byte [requested_radius_index]
    inc word [radius_change_requests]
    jmp .done
.radius_down:
    cmp byte [requested_radius_index], RADIUS_MIN
    jbe .radius_forward
    dec byte [requested_radius_index]
    inc word [radius_change_requests]
    jmp .done
.radius_forward:
    mov byte [auto_radius_direction], 1
    inc byte [requested_radius_index]
    inc word [radius_change_requests]
.done:
    ret
%endif

process_scheduler_edge:
    push ax
    push bx
    inc word [vblank_edges_total]
    mov byte [scheduler_boundary_reset], 0
%if M98AA_AUTO_CAMERA
    call advance_auto_camera
%endif
    call qa_before_scheduler_actions
    cmp byte [exit_requested], 0
    jne .done
    cmp byte [pause_toggle_pending], 0
    je .check_divisor
    xor byte [paused], 1
    mov byte [pause_toggle_pending], 0
    inc word [pause_transitions_applied]
    mov byte [scheduler_boundary_reset], 1
.check_divisor:
    mov al, [requested_divisor]
    cmp al, [active_divisor]
    je .classify
%if M98Y_PRIVATE_PROFILE
    ; Keep the old active divisor until its complete FPS field has been
    ; written.  A narrowly missed VBLANK is retried on the next edge rather
    ; than routing a valid RIGHT/LEFT request through cleanup.
    mov bl, [active_divisor]
    mov [active_divisor], al
    inc word [divider_changes_applied]
    call update_hud_fps_field
    jnc .hud_applied
    mov [active_divisor], bl
    dec word [divider_changes_applied]
    jmp .classify
%else
    mov [active_divisor], al
    inc word [divider_changes_applied]
    call update_hud_fps_field
    jnc .hud_applied
    mov word [hud_runtime_failure], 1
    mov byte [runtime_failure_kind], 1
%endif
.hud_applied:
    mov byte [scheduler_boundary_reset], 1
.classify:
    cmp byte [paused], 0
    je .unpaused
    inc word [vblank_edges_paused]
    mov byte [divider_count], 0
    cmp byte [scheduler_boundary_reset], 0
    je .done
    inc word [divider_boundary_resets]
    jmp .done
.unpaused:
    inc word [vblank_edges_unpaused]
    cmp byte [scheduler_boundary_reset], 0
    je .count_divider
    mov byte [divider_count], 0
    inc word [divider_boundary_resets]
    jmp .ready_wait
.count_divider:
    inc byte [divider_count]
    mov al, [divider_count]
    cmp al, [active_divisor]
    jb .ready_wait
    mov byte [divider_count], 0
    inc word [requested_slots]
    cmp byte [hidden_render_state], RENDER_READY
    jne .missed
    mov byte [eligible_publication], 1
    jmp .done
.missed:
    inc word [missed_slots]
    jmp .done
.ready_wait:
    cmp byte [hidden_render_state], RENDER_READY
    jne .done
    inc word [ready_wait_edges]
.done:
    pop bx
    pop ax
    ret

wait_scheduler_edge:
    mov bx, 4
.outer:
    mov cx, 0xffff
.poll:
    call poll_control_requests
    cmp byte [exit_requested], 0
    jne .ready
    call observe_vblank_sample
    jc .ready
    loop .poll
    dec bx
    jnz .outer
    inc word [vblank_timeouts]
    mov byte [runtime_failure_kind], 2
    stc
    ret
.ready:
    cmp word [hud_runtime_failure], 0
    jne .failed
    clc
    ret
.failed:
    stc
    ret

qa_queue_divisor:
    ; AL is a checked divisor used only by compile-time bounded QA scenarios.
    cmp al, CADENCE_MIN
    jb .failed
    cmp al, CADENCE_MAX
    ja .failed
    cmp al, [requested_divisor]
    je .done
    mov [requested_divisor], al
    inc word [divider_change_requests]
.done:
    clc
    ret
.failed:
    stc
    ret

qa_force_missed_slots_before_ready:
%if M98T_BOUNDED_QA && M98T_QA_SCENARIO = 3
    cmp byte [qa_stage], 0
    jne .done
    call wait_scheduler_edge
    jc .failed
    call wait_scheduler_edge
    jc .failed
    mov byte [qa_stage], 1
%endif
.done:
    clc
    ret
.failed:
    stc
    ret

qa_queue_pause:
    xor byte [pause_toggle_pending], 1
    inc word [pause_requests]
    ret

qa_after_sgp_submission:
%if M98T_BOUNDED_QA && M98T_QA_SCENARIO = 1
    cmp byte [hidden_render_state], RENDER_RENDERING
    jne .done
    cmp byte [qa_stage], 0
    jne .done
    cmp word [published_updates], 4
    jne .done
    mov al, 2
    call qa_queue_divisor
    jc .done
    mov byte [qa_stage], 1
%elif M98T_BOUNDED_QA && M98T_QA_SCENARIO = 2
    cmp byte [hidden_render_state], RENDER_RENDERING
    jne .done
    cmp byte [qa_stage], 0
    jne .done
    cmp word [published_updates], 4
    jne .done
    call qa_queue_pause
    mov byte [qa_pause_edges_remaining], 5
    mov byte [qa_stage], 1
%endif
.done:
    ret

qa_after_publication:
%if M98T_BOUNDED_QA && M98T_QA_SCENARIO = 1
    cmp byte [qa_stage], 1
    jb .done
    cmp byte [qa_stage], 14
    jae .done
    xor bx, bx
    mov bl, [qa_stage]
    dec bx
    mov si, bx
    shl si, 1
    mov ax, [qa_ladder_publications + si]
    cmp ax, [published_updates]
    jne .done
    mov al, [qa_ladder_divisors + bx]
    call qa_queue_divisor
    jc .done
    inc byte [qa_stage]
%elif M98T_BOUNDED_QA && M98T_QA_SCENARIO = 2
    cmp byte [qa_stage], 2
    jne .pause_at_40
    cmp word [published_updates], 20
    jne .done
    call qa_queue_pause
    mov byte [qa_pause_edges_remaining], 5
    mov byte [qa_stage], 3
    jmp .done
.pause_at_40:
    cmp byte [qa_stage], 4
    jne .done
    cmp word [published_updates], 40
    jne .done
    call qa_queue_pause
    mov byte [qa_pause_edges_remaining], 5
    mov byte [qa_stage], 5
%endif
.done:
    ret

qa_before_scheduler_actions:
%if M98T_BOUNDED_QA && M98T_QA_SCENARIO = 2
    cmp byte [paused], 0
    je .done
    cmp byte [pause_toggle_pending], 0
    jne .done
    cmp byte [qa_pause_edges_remaining], 0
    je .done
    dec byte [qa_pause_edges_remaining]
    jne .done
    call qa_queue_pause
    inc byte [qa_stage]
%endif
.done:
    ret

select_render_ordinary:
    mov dx, PORT_BMS_SELECTOR
    in al, dx
    test al, al
    jz .done
    xor al, al
    out dx, al
    inc word [bms_bank_switches]
.done:
    ret

run_sgp_command_list:
    call wait_sgp_idle
    jc .failed
    mov dx, PORT_GVRAM_WRITE_MODE
    mov al, GVRAM_CPU_WRITE_MODE
    out dx, al
    mov dx, PORT_SGP_COMMAND
    mov ax, [sgp_command_address_low]
    out dx, ax
    add dx, 2
    mov ax, [sgp_command_address_high]
    out dx, ax
    mov dx, PORT_SGP_CONTROL
    xor al, al
    out dx, al
    mov dx, PORT_SGP_STATUS
    mov al, SGP_BUSY
    out dx, al
    call qa_after_sgp_submission
    call wait_sgp_idle
    jnc .complete
    ; Abort only after the bounded completion wait fails.  This leaves the
    ; aperture safe to restore without publishing the incomplete page.
    mov dx, PORT_SGP_CONTROL
    mov al, 0x02
    out dx, al
    mov dx, PORT_SGP_CONTROL
    xor al, al
    out dx, al
    stc
    ret
.complete:
    clc
    ret
.failed:
    stc
    ret

wait_sgp_idle:
    mov dx, PORT_SGP_STATUS
    mov bx, 4
.outer:
    mov cx, 0xffff
.poll:
    in al, dx
    test al, SGP_BUSY
    jz .ready
    cmp byte [scheduler_active], 0
    je .continue
    call poll_control_requests
    call observe_vblank_sample
    cmp word [hud_runtime_failure], 0
    jne .hud_failed
.continue:
    loop .poll
    dec bx
    jnz .outer
    inc word [sgp_timeouts]
    stc
    ret
.ready:
    clc
    ret
.hud_failed:
    stc
    ret

wait_vblank_edge:
    mov dx, PORT_TSP_STATUS
    mov bx, 4
.wait_display:
    mov cx, 0xffff
.display_poll:
    in al, dx
    test al, TSP_STATUS_VBLANK
    jz .display_seen
    loop .display_poll
    dec bx
    jnz .wait_display
    inc word [vblank_timeouts]
    stc
    ret
.display_seen:
    mov bx, 4
.wait_vblank:
    mov cx, 0xffff
.vblank_poll:
    in al, dx
    test al, TSP_STATUS_VBLANK
    jnz .ready
    loop .vblank_poll
    dec bx
    jnz .wait_vblank
    inc word [vblank_timeouts]
    stc
    ret
.ready:
    inc word [vblank_edges_seen]
    clc
    ret

poll_escape:
    mov ah, 0x0a
    int KEYBOARD_BIOS_INT
    jc .none
    mov ah, 0x09
    int KEYBOARD_BIOS_INT
    cmp ah, KEY_SCAN_ESCAPE
    jne .none
    cmp al, KEY_INTERNAL_ESCAPE
    je .escape
.none:
    clc
    ret
.escape:
    stc
    ret

physical_address_from_ds_si:
    mov ax, ds
    xor dx, dx
    mov cx, 4
.shift:
    shl ax, 1
    rcl dx, 1
    loop .shift
    add ax, si
    adc dx, 0
    ret

save_video_state:
    mov dx, PORT_MEMORY_MAP
    in al, dx
    mov [saved_memory_map], al
    mov dx, VIDEO_BIOS_DATA_SEG
    mov es, dx
    mov ax, [es:VIDEO_MODE_OFFSET]
    mov [saved_video_mode], ax
    mov al, [es:VIDEO_G0_BPP_OFFSET]
    mov [saved_g0_bpp], al
    mov al, [es:VIDEO_G1_BPP_OFFSET]
    mov [saved_g1_bpp], al
    ret

restore_video_state:
    push cs
    pop ds
    mov dx, PORT_MEMORY_MAP
    mov al, [saved_memory_map]
    out dx, al
    cmp byte [video_mode_changed], 0
    je .done
    mov bx, [saved_video_mode]
    mov cl, [saved_g0_bpp]
    mov ch, [saved_g1_bpp]
    xor dx, dx
    xor ax, ax
    int VIDEO_BIOS_INT
.done:
    ret

restore_normal_guards:
    cmp byte [ordinary_guards_active], 0
    je .done
    call select_ordinary_mapping
    mov ax, NORMAL_OUTSIDE_SEGMENT
    mov es, ax
    mov di, NORMAL_OUTSIDE_OFFSET
    mov si, saved_normal_outside
    mov cx, GUARD_BYTES
    call write_es_bytes
    mov ax, NORMAL_UNDER_SEGMENT
    mov es, ax
    mov di, NORMAL_UNDER_OFFSET
    mov si, saved_normal_under
    mov cx, GUARD_BYTES
    call write_es_bytes
    mov byte [ordinary_guards_active], 0
.done:
    ret

restore_bms_and_guards:
    call restore_probe_banks
    call select_ordinary_mapping
    call restore_normal_guards
    ret

print_string:
    mov ah, 0x09
    int 0x21
    ret

message_start:
%if M98Y_PRIVATE_PROFILE
    db "M98Y_INIT: private IDA billboard profile", 13, 10
    db "Selector 0 is ordinary RAM; selector 1 is the validated atlas bank.", 13, 10, "$"
%else
    db "M98V_INIT: full-page multi-ZUNDAMON frame baseline", 13, 10
    db "Selector 0 is ordinary RAM; selector 1 is the atlas bank.", 13, 10, "$"
%endif
message_done:
%if M98Y_PRIVATE_PROFILE
    db "M98Y_EXIT: ordinary mapping, keyboard, and video state restored.", 13, 10, "$"
%else
    db "M98V_EXIT: ordinary mapping, keyboard, and video state restored.", 13, 10, "$"
%endif
message_option_failed:
    db "M98V_OPTION: use zero or one exact /V1 through /V8 option.", 13, 10, "$"
message_bms_failed:
    db "M98V_FAIL: predecessor BMS mapping probe failed.", 13, 10, "$"
message_atlas_failed:
    db "M98V_FAIL: atlas validation or streaming failed.", 13, 10, "$"
message_descriptor_failed:
    db "M98V_FAIL: instance list, descriptor, HUD, or bounded invariant failed.", 13, 10, "$"
message_transfer_failed:
    db "M98V_FAIL: complete hidden-page SGP frame or HUD update failed.", 13, 10, "$"
message_runtime_failed:
    db "M98V_FAIL: bounded VBLANK edge wait timed out.", 13, 10, "$"
atlas_filename:
%if M98Y_PRIVATE_PROFILE
    db "ZUNDAMON.BIN", 0
%else
    db "ZUNDORB.BIN", 0
%endif

; Keep every capture PC stable even as the surrounding helpers evolve.
times PROBE_CHECKPOINT_OFFSET - ($ - $$) db 0x90
probe_checkpoint:
    jmp probe_resume
times LOAD_CHECKPOINT_OFFSET - ($ - $$) db 0x90
load_checkpoint:
    jmp load_resume
times TRANSFER_CHECKPOINT_OFFSET - ($ - $$) db 0x90
transfer_checkpoint:
    jmp transfer_resume
times FLIP_CHECKPOINT_OFFSET - ($ - $$) db 0x90
flip_checkpoint:
    jmp flip_resume
times SETTLED_CHECKPOINT_OFFSET - ($ - $$) db 0x90
settled_checkpoint:
    jmp settled_resume
times REPORT_A_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_a_checkpoint:
    jmp report_a_resume
times REPORT_B_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_b_checkpoint:
    jmp report_b_resume
times REPORT_C_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_c_checkpoint:
    jmp report_c_resume
times REPORT_D_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_d_checkpoint:
    jmp report_d_resume
times REPORT_E_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_e_checkpoint:
    jmp report_e_resume
times REPORT_F_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_f_checkpoint:
    jmp report_f_resume
times REPORT_G_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_g_checkpoint:
    jmp report_g_resume
times REPORT_H_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_h_checkpoint:
    jmp report_h_resume
times REPORT_I_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_i_checkpoint:
    jmp report_i_resume
times REPORT_J_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_j_checkpoint:
    jmp report_j_resume
times REPORT_K_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_k_checkpoint:
    jmp report_k_resume
times REPORT_L_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_l_checkpoint:
    jmp report_l_resume
times REPORT_M_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_m_checkpoint:
    jmp report_m_resume
times REPORT_N_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_n_checkpoint:
    jmp report_n_resume
times REPORT_O_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_o_checkpoint:
    jmp report_o_resume
times REPORT_P_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_p_checkpoint:
    jmp report_p_resume
times REPORT_Q_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_q_checkpoint:
    jmp report_q_resume
times REPORT_R_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_r_checkpoint:
    jmp report_r_resume
times REPORT_S_CHECKPOINT_OFFSET - ($ - $$) db 0x90
report_s_checkpoint:
    jmp report_s_resume

align 2, db 0
sgp_command_list:
    times SGP_COMMAND_LIST_WORDS dw 0
sgp_work_area:
    times 29 dw 0
checker_row_a:
    times 8 db 0x24
    times 8 db 0x49
checker_row_b:
    times 8 db 0x49
    times 8 db 0x24
qa_ladder_publications: dw 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56
qa_ladder_divisors: db 3, 4, 5, 6, 7, 8, 7, 6, 5, 4, 3, 2, 1
expected_scale_histogram: db 1,2,2,2,2,4,2,2,2,2,2,2,2,2,3,3,2,2,2,2,2,2,2,2,4,2,2,2,2,1
%include "zundamon_depth_table.inc"
%if DEPTH_TABLE_PHASE_COUNT != ORBIT_PUBLICATIONS_PER_REVOLUTION
%error "M98t depth include has the wrong phase count"
%endif
%if DEPTH_TABLE_ENTRY_BYTES != 8
%error "M98t depth include has the wrong entry size"
%endif
%if DEPTH_TABLE_RADIUS_X != ORBIT_RADIUS_X || DEPTH_TABLE_RADIUS_Y != ORBIT_RADIUS_Y
%error "M98t depth include has the wrong radii"
%endif
%if DEPTH_TABLE_RADIUS_ADJUSTMENTS != ORBIT_RADIUS_ADJUSTMENTS
%error "M98t depth include has the wrong radius-adjustment count"
%endif
%include "zundamon_hud_table.inc"
%include "zundamon_status_table.inc"
%if HUD_TILE_COUNT != 8 || HUD_FULL_TILE_BYTES != HUD_FULL_WRITE_BYTES || HUD_FPS_TILE_BYTES != HUD_FPS_WRITE_BYTES
%error "M98v HUD include has the wrong FPS tile contract"
%endif
%if HUD_COUNT_TILE_COUNT != FOOTPRINT_CAPACITY || HUD_COUNT_TILE_BYTES != HUD_COUNT_WRITE_BYTES
%error "M98x HUD include has the wrong runtime count tile contract"
%endif
%if M98Y_PRIVATE_PROFILE
%ifndef HUD_PROFILE_IDA
%error "M98y private build requires the IDA HUD profile"
%endif
%endif
guard_normal_outside: db 0x5a,0xa5,0x3c,0xc3,0x69,0x96,0x0f,0xf0
guard_normal_under:   db 0xa5,0x5a,0xc3,0x3c,0x96,0x69,0xf0,0x0f
signature_bank_1:    db 0x11,0x21,0x31,0x41,0x51,0x61,0x71,0x81
signature_bank_2:    db 0x12,0x22,0x32,0x42,0x52,0x62,0x72,0x82
signature_bank_128:  db 0x18,0x28,0x38,0x48,0x58,0x68,0x78,0x88

align 2, db 0
screen_pitch_word: dw SCREEN_PITCH
page_sgp_low: dw G1_PAGE_A_SGP_BASE & 0xffff, G1_PAGE_B_SGP_BASE & 0xffff
page_sgp_high: dw G1_PAGE_A_SGP_BASE >> 16, G1_PAGE_B_SGP_BASE >> 16
page_dsa_low: dw G1_PAGE_A_DSA & 0xffff, G1_PAGE_B_DSA & 0xffff
page_dsa_high: dw G1_PAGE_A_DSA >> 16, G1_PAGE_B_DSA >> 16
page_state: db PAGE_UNINITIALIZED, PAGE_UNINITIALIZED
page_old_valid: db 0, 0
page_old_scale_id: db 0, 0
page_old_phase: db 0, 0
page_old_depth_rank: db 0, 0
visible_page_index: db M98Q_INITIAL_VISIBLE_PAGE
hidden_page_index: db 1 - M98Q_INITIAL_VISIBLE_PAGE
settled_capture_count: db 0
selected_scale_id: db 0
orbit_phase_next: db 0
pending_phase: db 0
last_published_phase: db 0
pending_depth_rank: db 0
visible_depth_rank: db 0
last_published_depth_rank: db 0
visible_scale_id: db 0
last_published_scale_id: db 0
descriptor_validation_id: db 0
runtime_failure_kind: db 0
exit_errorlevel: db 1
dirty_clear_needed: db 0
parsed_divisor: db CADENCE_MIN
active_divisor: db CADENCE_MIN
requested_divisor: db CADENCE_MIN
divider_count: db 0
paused: db 0
pause_toggle_pending: db 0
exit_requested: db 0
eligible_publication: db 0
hidden_render_state: db RENDER_IDLE
vblank_last_high: db 0
scheduler_active: db 0
scheduler_boundary_reset: db 0
cadence_option_seen: db 0
keyboard_repeat_disabled: db 0
qa_stage: db 0
qa_pause_edges_remaining: db 0
pending_global_phase: db 0
last_published_global_phase: db 0
published_active_count: db M98V_ACTIVE_COUNT
; M98z requested and active projection controls.  Requested values may change
; while SGP is busy; active values are copied only at a transaction boundary.
requested_speed_index: db SPEED_DEFAULT
active_speed_index: db SPEED_DEFAULT
requested_distance_bias: db 0
active_distance_bias: db 0
requested_look_level: db 0
active_look_level: db 0
requested_radius_index: db RADIUS_DEFAULT
active_radius_index: db RADIUS_DEFAULT
active_speed_increment: dw 256
active_radius_factor: dw 256
phase_accumulator: dw 0
base_scale_id: db 0
screen_y_bias: dw 0
speed_change_requests: dw 0
speed_changes_applied: dw 0
distance_change_requests: dw 0
distance_changes_applied: dw 0
look_change_requests: dw 0
look_changes_applied: dw 0
radius_change_requests: dw 0
radius_changes_applied: dw 0
saturated_key_requests: dw 0
paused_geometry_redraws: dw 0
scale_low_clamps: dw 0
scale_high_clamps: dw 0
clipped_instances: dw 0
%if M98AA_AUTO_CAMERA
auto_camera_vblank_ticks: dw 0
auto_speed_direction: db 1
auto_distance_direction: db 1
auto_radius_direction: db 1
%endif
align 2, db 0
%if M98Y_PRIVATE_PROFILE
; IDA64 speed selection is 1.00X..4.00X in exact 0.25X steps.
orbit_speed_increments: dw 256,320,384,448,512,576,640,704,768,832,896,960,1024
%else
orbit_speed_increments: dw 64,128,192,256,320,384,512,768,1024,1280,1536,1792,2048
%endif
orbit_radius_factors: dw 128,160,192,224,256,288,320,352,384
; M98x runtime count state.  The frame's build_active_count is latched at
; render-loop entry and is immutable through clear, draw, READY, and publish.
parsed_count: db 4
requested_count: db 4
next_render_count: db 4
pending_render_count: db 4
visible_published_count: db 4
count_change_pending: db 0
hud_count_staged: db 0
count_option_seen: db 0
pending_count_generation: dw 0
count_request_generation: dw 0
published_count_generation: dw 0
generation_instance: db 0
generated_phase_offset: db 0
generated_phase: db 0
validation_global_phase: db 0
sort_position: db 0
sort_candidate: db 0
sort_prior: db 0
sort_scan: db 0
draw_position: db 0
current_draw_instance: db 0
align 2, db 0
sort_candidate_ptr: dw 0
draw_order_seen: times DRAW_ORDER_SEEN_WORDS dw 0
page_old_x: dw 0, 0
page_old_y: dw 0, 0
page_old_width: dw 0, 0
page_old_height: dw 0, 0
page_footprint_valid: db 0, 0
page_footprint_phase: db 0, 0
page_footprint_count: db 0, 0
align 2, db 0
page_footprint_rects: times FOOTPRINT_TOTAL_BYTES db 0
page_footprint_instance_ids: times 2 * FOOTPRINT_CAPACITY db 0
dirty_interval_candidates: times FOOTPRINT_CAPACITY * DIRTY_INTERVAL_BYTES db 0
dirty_interval_merged: times FOOTPRINT_CAPACITY * DIRTY_INTERVAL_BYTES db 0
pending_x: dw 0
pending_y: dw 0
pending_width: dw 0
pending_height: dw 0
pending_scale_id: db 0
align 2, db 0
pending_orbit_dx: dw 0
pending_orbit_dy: dw 0
dirty_clear_x0: dw 0
dirty_words_per_row: dw 0
dirty_rows_remaining: dw 0
dirty_batch_rows: dw 0
dirty_batch_intervals: dw 0
dirty_row_address: dw 0, 0
dirty_interval_end_address: dw 0, 0
dirty_builder_failed: dw 0
dirty_union_accounting: db 0
align 2, db 0
dirty_sort_position: dw 0
dirty_sort_scan: dw 0
last_published_dsa: dw G1_PAGE_A_DSA & 0xffff
psp_segment: dw 0
exit_message: dw message_transfer_failed
pages_initialized: dw 0
build_active_count: dw M98V_ACTIVE_COUNT
render_batches_started: dw 0
render_batches_completed: dw 0
complete_frames_started: dw 0
complete_frames_ready: dw 0
complete_frames_published: dw 0
published_frames: dw 0
instances_planned: dw 0
instances_submitted: dw 0
instances_completed: dw 0
instances_published: dw 0
draw_order_failures: dw 0
tie_break_failures: dw 0
initial_full_page_clears: dw 0
steady_full_page_clears: dw 0
dirty_rect_clears: dw 0
transparent_bitblts: dw 0
vblank_edges_seen: dw 0
page_flips: dw 0
page_a_publications: dw 0
page_b_publications: dw 0
sgp_timeouts: dw 0
sgp_errors: dw 0
vblank_timeouts: dw 0
bms_bank_switches: dw 0
bms_bank_selections: dw 0
descriptor_errors: dw 0
cleanup_runs: dw 0
vblank_edges_total: dw 0
vblank_edges_unpaused: dw 0
vblank_edges_paused: dw 0
divider_change_requests: dw 0
divider_changes_applied: dw 0
divider_boundary_resets: dw 0
pause_requests: dw 0
pause_transitions_applied: dw 0
control_endpoint_hits: dw 0
requested_slots: dw 0
published_updates: dw 0
missed_slots: dw 0
ready_wait_edges: dw 0
phase_advances: dw 0
scale_changes: dw 0
table_scale_change_edges: dw 0
near_publications: dw 0
far_publications: dw 0
scale_publication_total: dw 0
hud_full_initializations: dw 0
hud_fps_field_updates: dw 0
hud_zundamon_field_updates: dw 0
hud_count_field_updates: dw 0
hud_status_updates: dw 0
hud_count_rollbacks: dw 0
runtime_count_changes: dw 0
count_key_requests: dw 0
count_up_requests: dw 0
count_down_requests: dw 0
count_noop_saturations: dw 0
count_request_events: dw 0
count_requests_coalesced: dw 0
count_change_publications: dw 0
same_count_publications: dw 0
count_increase_applies: dw 0
count_decrease_applies: dw 0
count_publications: times FOOTPRINT_CAPACITY dw 0
%if M98Y_PRIVATE_PROFILE
count_transition_publications: times 0 dw 0
%else
count_transition_publications: times FOOTPRINT_CAPACITY * FOOTPRINT_CAPACITY dw 0
%endif
hud_g1_writes: dw 0
hud_vblank_overruns: dw 0
hud_mismatches: dw 0
hud_runtime_failure: dw 0
partial_publication_attempts: dw 0
dirty_full_mismatches: dw 0
guard_failures: dw 0
sgp_command_lists: dw 0
sgp_commands: dw 0
sgp_batches: dw 0
revolution_wraps: dw 0
phase_publication_total: dw 0
bounds_failures: dw 0
source_failures: dw 0
framebuffer_mismatches: dw 0
bounded_validation_pass: dw 0
validation_failure_code: dw 0
source_bytes: dw 0, 0
full_page_clear_words: dw 0, 0
full_page_clear_bytes: dw 0, 0
dirty_row_cls_commands: dw 0, 0
dirty_words_cleared: dw 0, 0
dirty_bytes_cleared: dw 0, 0
page_footprint_commits: dw 0
page_footprint_commit_failures: dw 0
dirty_frames_with_clear: dw 0
dirty_first_use_skips: dw 0
dirty_candidate_intervals: dw 0, 0
dirty_merged_intervals: dw 0, 0
dirty_overlap_merges: dw 0
dirty_adjacency_merges: dw 0
dirty_containment_merges: dw 0
dirty_rows_visited: dw 0
dirty_rows_nonempty: dw 0
dirty_rows_cleared: dw 0
union_validation_passes: dw 0
union_recompute_mismatches: dw 0
sgp_clear_batches: dw 0
sgp_draw_batches: dw 0
dirty_pass1_candidates: dw 0, 0
dirty_pass1_merged: dw 0, 0
dirty_pass2_merged: dw 0, 0
dirty_pass2_candidates: dw 0, 0
dirty_row_y: dw 0
dirty_candidate_count: dw 0
dirty_sorted_count: dw 0
dirty_merged_count: dw 0
dirty_interval_cursor: dw 0
dirty_batch_start: dw 0
dirty_current_x0: dw 0
dirty_current_x1: dw 0
dirty_current_id: dw 0
dirty_next_x0: dw 0
dirty_next_x1: dw 0
dirty_next_id: dw 0
baseline_full_clear_words: dw 0, 0
baseline_full_clear_bytes: dw 0, 0
publication_digest: dw 0, 0
phase_publications: times ORBIT_PUBLICATIONS_PER_REVOLUTION dw 0
scale_publications: times ATLAS_SCALE_COUNT dw 0
hud_bytes_written: dw 0, 0
table_scale_histogram: times ATLAS_SCALE_COUNT db 0
align 2, db 0
instance_records: times INSTANCE_RECORD_CAPACITY_BYTES db 0
draw_order: times DRAW_ORDER_CAPACITY_BYTES db 0
sgp_command_address_low: dw 0
sgp_command_address_high: dw 0
saved_video_mode: dw 0
atlas_handle: dw 0xffff
atlas_payload_bytes: dw 0, 0
atlas_file_size: dw 0, 0
atlas_remaining: dw 0, 0
atlas_loaded: dw 0, 0
atlas_chunk_count: dw 0
current_chunk_bytes: dw 0
expected_payload_crc: dw 0, 0
expected_file_crc: dw 0, 0
expected_frame_crc: dw 0, 0
file_crc_state: dw 0, 0
payload_crc_state: dw 0, 0
bms_crc_state: dw 0, 0
frame_crc_state: dw 0, 0
crc_range_offset: dw 0, 0
crc_range_remaining: dw 0, 0
selected_width: dw 0
selected_height: dw 0
selected_pitch: dw 0
selected_anchor_x: dw 0
selected_anchor_y: dw 0
atlas_source_width: dw 0
atlas_source_height: dw 0
atlas_source_anchor_x: dw 0
atlas_source_anchor_y: dw 0
expected_descriptor_offset: dw 0, 0
selected_bank_offset: dw 0, 0
selected_payload_bytes: dw 0, 0
selected_dst_x: dw 0
selected_dst_y: dw 0
selected_source_low: dw 0
selected_source_high: dw 0
saved_normal_outside: times GUARD_BYTES db 0
saved_normal_under: times GUARD_BYTES db 0
saved_bank_1: times GUARD_BYTES db 0
saved_bank_2: times GUARD_BYTES db 0
saved_bank_128: times GUARD_BYTES db 0
saved_memory_map: db 0
saved_g0_bpp: db 0
saved_g1_bpp: db 0
video_mode_changed: db 0
initial_bms_selector: db 0
probe_result: db 0
probe_stage: dw 0
saved_probe_banks: db 0
ordinary_guards_active: db 0

align 16, db 0
atlas_metadata:
    times ATLAS_METADATA_BYTES db 0
align 16, db 0
staging_buffer:
    times STAGING_BYTES db 0
program_end:

%if program_end - $$ >= 65280
%error "M98t guest exceeds the 64-KiB DOS payload limit"
%endif
