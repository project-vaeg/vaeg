; Copyright (c) 2026 Nakata Maho
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
; 1. Redistributions of source code must retain the above copyright
;    notice, this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in the
;    documentation and/or other materials provided with the distribution.
;
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
; IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
; OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
; IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
; INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
; BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
; USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
; ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
; THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

bits 16
org 0x100

%define KEYBOARD_BIOS_INT       0x82
%define VIDEO_BIOS_INT          0x8f
%define VIDEO_BIOS_DATA_SEG     0x0338
%define VIDEO_MODE_OFFSET       0x000f
%define VIDEO_G0_BPP_OFFSET     0x0011
%define VIDEO_G1_BPP_OFFSET     0x0012

%define PORT_MEMORY_MAP         0x0153
%define PORT_GRRES              0x0102
%define PORT_GRMODE             0x0100
%define PORT_PALETTE_COMPOSE    0x0106
%define PORT_RGB_COMPOSE        0x0108
%define PORT_FB0_FBW            0x0204
%define PORT_FB0_FBL            0x0206
%define PORT_FB0_DOT            0x0208
%define PORT_FB0_OFX            0x020a
%define PORT_FB0_OFY            0x020c
%define PORT_FB0_DSA_LOW        0x020e
%define PORT_FB0_DSA_HIGH       0x0210
%define PORT_FB0_DSH            0x0212
%define PORT_FB0_DSP            0x0216
%define PORT_TSP_STATUS         0x0142
%define PORT_SGP_COMMAND        0x0500
%define PORT_SGP_CONTROL        0x0504
%define PORT_SGP_STATUS         0x0506
%define PORT_GVRAM_WRITE_MODE   0x0580

%define MEMORY_MAP_GVRAM_SINGLE 0x54
%define GVRAM_CPU_WRITE_MODE    0x10
%define MODE_320X200_G0_ONLY     0xa00e
%define PIXEL_SIZE_G0_16BPP       0x0010
%define COMPOSE_G0_DIRECT       0x0008
%define TSP_STATUS_VBLANK       0x40

; The source surface is 320 pixels wide at 16 bpp (640 bytes per row).
; Two 320x200 pages are stacked vertically in a 320x400 framebuffer.
%define SCREEN_WIDTH            320
%define SCREEN_HEIGHT           200
%define SCREEN_PITCH            640
%define G0_PAGE_A_SGP_BASE      0x200000
%define G0_PAGE_B_SGP_BASE      0x21f400
%define G0_PAGE_A_DSA           0x000000
%define G0_PAGE_B_DSA           0x01f400
%define SCREEN_WORD_COUNT       0xfa00

%define SGP_COMMAND_END         0x0001
%define SGP_COMMAND_SET_WORK    0x0003
%define SGP_COMMAND_SET_DEST    0x0005
%define SGP_COMMAND_SET_COLOR   0x0006
%define SGP_COMMAND_LINE        0x0009
%define SGP_COMMAND_CLS         0x000a
%define SGP_COMMAND_SCAN_RIGHT  0x000b
%define SGP_COMMAND_SCAN_LEFT   0x000c
%define SGP_LINE_COPY           0x0005
%define SGP_LINE_HD             0x0400
%define SGP_LINE_VD             0x0800
%define SGP_BUSY                0x01

; Direct-color demonstration words use the existing VAEG word convention.
%define DIRECT16(r,g,b)           ((((g) & 0x3f) << 10) | (((r) & 0x1f) << 5) | ((b) & 0x1f))

%define SHAPE_VERTICES          0
%define SHAPE_EDGES             2
%define SHAPE_PROJECTED         4
%define SHAPE_ROTATED           6
%define SHAPE_VERTEX_COUNT      8
%define SHAPE_EDGE_COUNT        9
%define SHAPE_CENTER_X          10
%define SHAPE_CENTER_Y          12
%define SHAPE_PHASE_Y           14
%define SHAPE_PHASE_X           15
%define SHAPE_SPEED_Y           16
%define SHAPE_SPEED_X           17
%define SHAPE_SCALE_PHASE       18
%define SHAPE_SCALE_SPEED       19
%define SHAPE_BASE_SCALE        20
%define SHAPE_SCALE_AMPLITUDE   22
%define SHAPE_BRIGHT_COLOR      24
%define SHAPE_DIM_COLOR         26
%define SHAPE_FACE_TABLE        28
%define SHAPE_FACE_COUNT        30
%define SHAPE_RECORD_SIZE       32

%define PROJECTED_VERTEX_SIZE   6
%define SHAPE_COUNT             4
; Four 200-line triangles can emit one LINE record per scanline in addition
; to the grid, edges, and SCAN probes. Reserve enough words for the bounded
; worst case rather than allowing the list to overwrite its work area.
%define COMMAND_LIST_WORDS      8192
%define COMMAND_LIST_BYTES      (COMMAND_LIST_WORDS * 2)
%define SGP_WORK_OFFSET          COMMAND_LIST_BYTES
; Reserve a CPU-visible main-RAM scratch segment outside the COM image.  The
; segment is below the VA system-memory window and does not overlap the DOS
; COM load area used by the boot disk.
%define SGP_BUFFER_SEGMENT       0x5000
%define TRIANGLE_MIN_COUNT      1
%define TRIANGLE_MAX_COUNT      4

start:
    push cs
    pop ds
    cld

    call save_video_state
    mov dx, message_start
    call print_string
    call initialize_video
    jc initialization_failed

animation_loop:
    call poll_keyboard
    jc animation_done

    call build_frame_commands
    call run_sgp_command_list
    jc animation_failed
    call wait_vblank_start
    jc animation_failed
    call display_draw_page
    xor byte [draw_page_index], 1
    call select_draw_page
    call advance_shape_phases
    jmp animation_loop

animation_done:
    call restore_video_state
    mov dx, message_done
    call print_string
    mov ax, 0x4c00
    int 0x21

initialization_failed:
    call restore_video_state
    mov dx, message_initialization_failed
    call print_string
    mov ax, 0x4c01
    int 0x21

animation_failed:
    call restore_video_state
    mov dx, message_animation_failed
    call print_string
    mov ax, 0x4c02
    int 0x21

initialize_video:
    mov bx, MODE_320X200_G0_ONLY
    mov cx, PIXEL_SIZE_G0_16BPP
    xor dx, dx
    mov byte [video_mode_changed], 1
    xor ax, ax
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    ; Match the PC-Engine 1.00 200-line, single-plane, non-interlaced field.
    ; This is a word register and must be written as a word on real hardware.
    mov dx, PORT_GRMODE
    mov ax, 0xb462
    out dx, ax
    ; G0: 16 bpp, 320-dot fetch; G1 remains configured but is not selected.
    mov dx, PORT_GRRES
    mov ax, 0x1313
    out dx, ax
    ; G0 is a 320x400 direct-color source framebuffer. Its two contiguous
    ; 320x200 halves are used as hidden/display pages within the 256 KiB
    ; surface.
    call define_g0_surface
    jc .failed

    ; The BIOS descriptor establishes the surface; these registers establish
    ; the 320x400 source layout and 320x200 displayed sub-screen explicitly.
    call configure_g0_framebuffer
    jc .failed

    ; 16-bpp graphics is a direct-color source on the direct-color priority
    ; path. Disable palette sources and place G0 in priority slot 3.
    xor ax, ax
    mov dx, PORT_PALETTE_COMPOSE
    out dx, ax
    mov ax, COMPOSE_G0_DIRECT
    mov dx, PORT_RGB_COMPOSE
    out dx, ax
    mov dx, PORT_MEMORY_MAP
    mov al, MEMORY_MAP_GVRAM_SINGLE
    out dx, al
    mov dx, PORT_GVRAM_WRITE_MODE
    mov al, GVRAM_CPU_WRITE_MODE
    out dx, al

    call select_draw_page
    call build_frame_commands
    call run_sgp_command_list
    jc .failed
    call display_draw_page
    mov byte [draw_page_index], 1
    call select_draw_page

    mov ax, 0x0b01
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    clc
    ret

.failed:
    stc
    ret

; Define the single 320x400 G0 framebuffer used by this direct-color path.
define_g0_surface:
    push es
    push ds
    pop es
    mov ax, 0x0100
    mov cx, 1
    mov di, g0_framebuffer_descriptor
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    mov ax, 0x0200
    mov cx, 1
    mov di, g0_window_descriptor
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    pop es
    clc
    ret

.failed:
    pop es
    stc
    ret

; Configure FB0 according to the VA framebuffer register model:
; FBW=640 bytes, FBL=400 lines, DSH=200, DSP=0, with no source offset.
configure_g0_framebuffer:
    push ax
    push dx
    mov dx, PORT_FB0_FBW
    mov ax, SCREEN_PITCH
    out dx, ax
    mov dx, PORT_FB0_FBL
    mov ax, SCREEN_HEIGHT * 2
    out dx, ax
    mov dx, PORT_FB0_DOT
    xor ax, ax
    out dx, ax
    mov dx, PORT_FB0_OFX
    out dx, ax
    mov dx, PORT_FB0_OFY
    out dx, ax
    mov dx, PORT_FB0_DSA_LOW
    out dx, ax
    mov dx, PORT_FB0_DSA_HIGH
    out dx, ax
    mov dx, PORT_FB0_DSH
    mov ax, SCREEN_HEIGHT
    out dx, ax
    mov dx, PORT_FB0_DSP
    xor ax, ax
    out dx, ax
    pop dx
    pop ax
    clc
    ret

select_draw_page:
    cmp byte [draw_page_index], 0
    je .page_a
    mov word [draw_page_sgp_low], G0_PAGE_B_SGP_BASE & 0xffff
    mov word [draw_page_sgp_high], G0_PAGE_B_SGP_BASE >> 16
    mov word [draw_page_dsa_low], G0_PAGE_B_DSA & 0xffff
    mov word [draw_page_dsa_high], G0_PAGE_B_DSA >> 16
    ret
.page_a:
    mov word [draw_page_sgp_low], G0_PAGE_A_SGP_BASE & 0xffff
    mov word [draw_page_sgp_high], G0_PAGE_A_SGP_BASE >> 16
    mov word [draw_page_dsa_low], G0_PAGE_A_DSA & 0xffff
    mov word [draw_page_dsa_high], G0_PAGE_A_DSA >> 16
    ret

; DSA0 uses word registers. Byte access to these ports can hang real hardware.
display_draw_page:
    push ax
    push dx
    mov dx, PORT_FB0_DSA_LOW
    mov ax, [draw_page_dsa_low]
    out dx, ax
    add dx, 2
    mov ax, [draw_page_dsa_high]
    out dx, ax
    pop dx
    pop ax
    ret

build_frame_commands:
    ; BIOS calls and keyboard services are not relied upon to preserve DS.
    ; Rebind the COM data segment before addressing command/list storage.
    push cs
    pop ds
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push es
    push ds
    pop es
    mov ax, SGP_BUFFER_SEGMENT
    mov es, ax
    xor di, di

    mov ax, SGP_COMMAND_SET_WORK
    stosw
    mov si, SGP_WORK_OFFSET
    call physical_address_from_es_si
    stosw
    mov ax, dx
    stosw

    ; The two pages are contiguous in the 320x400 source surface, so one
    ; linear CLS clears exactly the hidden 320x200 page.
    mov ax, SGP_COMMAND_CLS
    stosw
    mov ax, [draw_page_sgp_low]
    stosw
    mov ax, [draw_page_sgp_high]
    stosw
    mov ax, SCREEN_WORD_COUNT
    stosw
    xor ax, ax
    stosw
    mov word [last_line_color], 0
    call emit_background_grid
    ; Exercise both documented SCAN commands at the first destination pixel. The
    ; probes terminate immediately and do not modify the destination.
    call emit_scan_probe
    mov bx, shape_records
    mov cx, [active_shape_count]
.shape:
    mov [current_shape], bx
    push bx
    push cx
    call project_shape
    call emit_shape_edges
    pop cx
    pop bx
    add bx, SHAPE_RECORD_SIZE
    loop .shape

    mov ax, SGP_COMMAND_END
    stosw
    xor si, si
    call physical_address_from_es_si
    mov [sgp_command_address_low], ax
    mov [sgp_command_address_high], dx

    pop es
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; Project one signed-byte vertex set with two Q7 rotations and a small
; perspective term. Each product is saved before the next IMUL because the
; one-operand 16-bit form overwrites DX:AX.
project_shape:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    mov bx, [current_shape]
    mov si, [bx + SHAPE_VERTICES]
    mov di, [bx + SHAPE_PROJECTED]
    mov bp, [bx + SHAPE_ROTATED]

    mov al, [bx + SHAPE_PHASE_Y]
    call load_sine_cosine
    mov [sin_y], dx
    mov [cos_y], ax
    mov al, [bx + SHAPE_PHASE_X]
    call load_sine_cosine
    mov [sin_x], dx
    mov [cos_x], ax

    mov al, [bx + SHAPE_SCALE_PHASE]
    xor ah, ah
    and ax, 0x003f
    mov bx, ax
    mov al, [sin_table + bx]
    cbw
    mov bx, [current_shape]
    imul word [bx + SHAPE_SCALE_AMPLITUDE]
    sar ax, 7
    add ax, [bx + SHAPE_BASE_SCALE]
    mov [current_scale], ax

    xor ch, ch
    mov cl, [bx + SHAPE_VERTEX_COUNT]
.vertex:
    mov al, [si]
    cbw
    mov [vertex_x], ax
    mov al, [si + 1]
    cbw
    mov [vertex_y], ax
    mov al, [si + 2]
    cbw
    mov [vertex_z], ax
    add si, 3

    mov ax, [vertex_x]
    imul word [cos_y]
    mov [rotation_term], ax
    mov ax, [vertex_z]
    imul word [sin_y]
    add ax, [rotation_term]
    sar ax, 7
    mov [rotated_x], ax

    mov ax, [vertex_z]
    imul word [cos_y]
    mov [rotation_term], ax
    mov ax, [vertex_x]
    imul word [sin_y]
    mov dx, [rotation_term]
    sub dx, ax
    sar dx, 7
    mov [rotated_z1], dx

    mov ax, [vertex_y]
    imul word [cos_x]
    mov [rotation_term], ax
    mov ax, [rotated_z1]
    imul word [sin_x]
    mov dx, [rotation_term]
    sub dx, ax
    sar dx, 7
    mov [rotated_y], dx

    mov ax, [vertex_y]
    imul word [sin_x]
    mov [rotation_term], ax
    mov ax, [rotated_z1]
    imul word [cos_x]
    add ax, [rotation_term]
    sar ax, 7
    mov [rotated_z], ax

    mov dx, ax
    sar dx, 3
    add dx, [current_scale]
    mov [perspective_scale], dx

    mov ax, [rotated_x]
    imul word [perspective_scale]
    sar ax, 7
    mov bx, [current_shape]
    add ax, [bx + SHAPE_CENTER_X]
    cmp ax, -512
    jge .x_lower_bound
    mov ax, -512
.x_lower_bound:
    cmp ax, 511
    jle .x_projected
    mov ax, 511
.x_projected:
    mov [ds:di], ax
    add di, 2

    mov ax, [rotated_y]
    imul word [perspective_scale]
    sar ax, 7
    ; Keep projection coordinates in the 320-dot display coordinate system.
    neg ax
    add ax, [bx + SHAPE_CENTER_Y]
    cmp ax, -512
    jge .y_lower_bound
    mov ax, -512
.y_lower_bound:
    cmp ax, 511
    jle .y_projected
    mov ax, 511
.y_projected:
    mov [ds:di], ax
    add di, 2
    mov ax, [rotated_z]
    mov [ds:di], ax
    add di, 2

    ; Keep the rotated object-space coordinates for face normals in COM data.
    mov ax, [rotated_x]
    mov [ds:bp], ax
    add bp, 2
    mov ax, [rotated_y]
    mov [ds:bp], ax
    add bp, 2
    mov ax, [rotated_z]
    mov [ds:bp], ax
    add bp, 2
    dec cx
    jz short .vertices_done
    jmp .vertex

.vertices_done:

    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; Draw a quiet 40-pixel grid into the single direct-color page. It supplies a
; stable 320x200 reference without using CPU GVRAM writes.
emit_background_grid:
    push ax
    push bx
    push cx
    ; Keep the reference background achromatic: CLS supplies black and the
    ; grid is white. Plate colors remain independent of this background.
    mov ax, DIRECT16(31, 63, 31)
    call emit_set_color

    xor bx, bx
    mov cx, 8
.vertical:
    mov [line_x1], bx
    mov [line_x2], bx
    mov word [line_y1], 0
    mov word [line_y2], SCREEN_HEIGHT - 1
    call emit_line
    add bx, 40
    loop .vertical

    xor bx, bx
    mov cx, 10
.horizontal:
    mov [line_y1], bx
    mov [line_y2], bx
    mov word [line_x1], 0
    mov word [line_x2], SCREEN_WIDTH - 1
    call emit_line
    add bx, 40
    loop .horizontal

    mov word [line_x1], SCREEN_WIDTH - 1
    mov word [line_x2], SCREEN_WIDTH - 1
    mov word [line_y1], 0
    mov word [line_y2], SCREEN_HEIGHT - 1
    call emit_line
    mov word [line_x1], 0
    mov word [line_x2], SCREEN_WIDTH - 1
    mov word [line_y1], SCREEN_HEIGHT - 1
    mov word [line_y2], SCREEN_HEIGHT - 1
    call emit_line

.done:
    pop cx
    pop bx
    pop ax
    ret

; Disabled polygon-fill experiment. The source is kept out of the generated
; COM while direct-color BIOS semantics remain unresolved.
%if 0
; Fill each projected triangle through Extended Graphics BIOS AH=12h. The
; BIOS frame mode 4 is the documented single-plane 16-bpp mode; its frame
; table points at the hidden page selected by select_draw_page. This is a
; temporary comparison path, separate from the direct SGP LINE-span routine.
bios_fill_triangles:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    push ds
    push es
    push cs
    pop ds
    push cs
    pop es

    mov ax, [bios_frame_segment]
    mov [bios_frame_info + 4], ax
    mov dx, bios_frame_info
    mov ah, 0x01
    int 0x87

    mov bx, shape_records
    mov cx, [active_shape_count]
.shape:
    mov [current_shape], bx
    call bios_copy_triangle_data
    call bios_select_triangle_colors
    mov bx, bios_line_table
    mov ah, 0x12
    mov al, 0x18
    push cx
    xor cx, cx
    xor dx, dx
    int 0x87
    pop cx
    mov bx, [current_shape]
    add bx, SHAPE_RECORD_SIZE
    loop .shape

    pop es
    pop ds
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; Build the three-edge table expected by the BIOS polygon service.
bios_copy_triangle_data:
    push ax
    push bx
    push si
    push di
    mov bx, [current_shape]
    mov si, [bx + SHAPE_PROJECTED]
    mov di, bios_triangle_data
    mov ax, [si]
    stosw
    mov ax, [si + 2]
    stosw
    mov ax, [si + 6]
    stosw
    mov ax, [si + 8]
    stosw
    mov ax, [si + 6]
    stosw
    mov ax, [si + 8]
    stosw
    mov ax, [si + 12]
    stosw
    mov ax, [si + 14]
    stosw
    mov ax, [si + 12]
    stosw
    mov ax, [si + 14]
    stosw
    mov ax, [si]
    stosw
    mov ax, [si + 2]
    stosw
    mov ax, cs
    mov [bios_line_table + 2], ax
    pop di
    pop si
    pop bx
    pop ax
    ret

; Select a flat shade from the rotated face normal and a fixed viewer-side
; light. Both the border and interior use the same shade for this comparison.
bios_select_triangle_colors:
    push ax
    push bx
    push si
    mov bx, [current_shape]
    mov si, [bx + SHAPE_ROTATED]
    mov ax, [si + 6]
    sub ax, [si]
    mov [normal_edge1_x], ax
    mov ax, [si + 8]
    sub ax, [si + 2]
    mov [normal_edge1_y], ax
    mov ax, [si + 12]
    sub ax, [si]
    mov [normal_edge2_x], ax
    mov ax, [si + 14]
    sub ax, [si + 2]
    mov [normal_edge2_y], ax
    mov ax, [normal_edge1_x]
    imul word [normal_edge2_y]
    mov [normal_cross_z], ax
    mov ax, [normal_edge1_y]
    imul word [normal_edge2_x]
    sub [normal_cross_z], ax
    mov ax, [bx + SHAPE_DIM_COLOR]
    cmp word [normal_cross_z], 0
    jle .color_ready
    mov ax, [bx + SHAPE_BRIGHT_COLOR]
.color_ready:
    mov [bios_line_table + 10], ax
    mov [bios_line_table + 12], ax
    pop si
    pop bx
    pop ax
    ret
%endif

; Disabled direct-SGP polygon-fill comparison routine. The active animation
; path uses outline LINE records only; keep this code out of the COM until a
; fill contract is selected.
%if 0
; Retained direct-SGP comparison routine. The active animation path uses the
; BIOS polygon call above, so this routine is not emitted in the current build.
; Fill one projected triangular plate with a small set of horizontal SGP LINE
; spans.  This deliberately uses only bounded 16-bit additions and shifts;
; all destination writes remain SGP operations.  The direct-color path does
; not depend on the 4-bpp Graphics BIOS paint service.
emit_shape_fill:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    mov bx, [current_shape]
    mov di, [bx + SHAPE_PROJECTED]
    mov ax, [di]
    mov [tri_v0x], ax
    mov ax, [di + 2]
    mov [tri_v0y], ax
    mov ax, [di + 6]
    mov [tri_v1x], ax
    mov ax, [di + 8]
    mov [tri_v1y], ax
    mov ax, [di + 12]
    mov [tri_v2x], ax
    mov ax, [di + 14]
    mov [tri_v2y], ax

    ; The light is fixed on the viewer's +Z axis.  Compute the rotated face
    ; normal's Z component from two edges; its sign is the normal/light dot
    ; product for this light, so back-facing plates use the dim shade.
    mov si, [bx + SHAPE_ROTATED]
    mov ax, [si + 6]
    sub ax, [si]
    mov [normal_edge1_x], ax
    mov ax, [si + 8]
    sub ax, [si + 2]
    mov [normal_edge1_y], ax
    mov ax, [si + 12]
    sub ax, [si]
    mov [normal_edge2_x], ax
    mov ax, [si + 14]
    sub ax, [si + 2]
    mov [normal_edge2_y], ax
    mov ax, [normal_edge1_x]
    imul word [normal_edge2_y]
    mov [normal_cross_z], ax
    mov ax, [normal_edge1_y]
    imul word [normal_edge2_x]
    sub [normal_cross_z], ax
    cmp word [normal_cross_z], 0
    jle .dim
    mov ax, [bx + SHAPE_BRIGHT_COLOR]
    jmp .color_ready
.dim:
    mov ax, [bx + SHAPE_DIM_COLOR]
.color_ready:
    call emit_set_color

    ; Determine a clipped horizontal bounding span using comparisons only.
    mov ax, [tri_v0x]
    mov [tri_min_x], ax
    mov [tri_max_x], ax
    mov dx, [tri_v1x]
    cmp dx, [tri_min_x]
    jge .x1_min_done
    mov [tri_min_x], dx
.x1_min_done:
    cmp dx, [tri_max_x]
    jle .x1_max_done
    mov [tri_max_x], dx
.x1_max_done:
    mov dx, [tri_v2x]
    cmp dx, [tri_min_x]
    jge .x2_min_done
    mov [tri_min_x], dx
.x2_min_done:
    cmp dx, [tri_max_x]
    jle .x2_max_done
    mov [tri_max_x], dx
.x2_max_done:
    cmp word [tri_min_x], 0
    jge .min_x_ok
    mov word [tri_min_x], 0
.min_x_ok:
    cmp word [tri_max_x], SCREEN_WIDTH - 1
    jle .max_x_ok
    mov word [tri_max_x], SCREEN_WIDTH - 1
.max_x_ok:
    cmp word [tri_min_x], SCREEN_WIDTH - 1
    jg .done
    cmp word [tri_max_x], 0
    jl .done

    mov ax, [tri_min_x]
    add ax, [tri_max_x]
    sar ax, 1
    mov [tri_center_x], ax
    mov ax, [tri_max_x]
    sub ax, [tri_min_x]
    sar ax, 2
    mov [tri_quarter_width], ax

    ; Emit full spans at the projected vertex rows and narrower spans between
    ; each pair of vertices. Their overlap gives a stable filled-plate
    ; impression while avoiding unsafe division.
    mov ax, [tri_min_x]
    mov [line_x1], ax
    mov ax, [tri_max_x]
    mov [line_x2], ax
    mov ax, [tri_v0y]
    call emit_clipped_horizontal
    mov ax, [tri_v1y]
    call emit_clipped_horizontal
    mov ax, [tri_v2y]
    call emit_clipped_horizontal
    mov ax, [tri_v0y]
    add ax, [tri_v1y]
    sar ax, 1
    mov bx, [tri_quarter_width]
    call emit_clipped_centered
    mov ax, [tri_v1y]
    add ax, [tri_v2y]
    sar ax, 1
    mov bx, [tri_quarter_width]
    call emit_clipped_centered
    mov ax, [tri_v2y]
    add ax, [tri_v0y]
    sar ax, 1
    mov bx, [tri_quarter_width]
    call emit_clipped_centered
.done:
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; Emit a horizontal span if its row is inside the 320x200 logical display.
; AX is the row and line_x1/line_x2 are already clipped.
emit_clipped_horizontal:
    cmp ax, 0
    jl .done
    cmp ax, SCREEN_HEIGHT - 1
    jg .done
    mov [line_y1], ax
    mov [line_y2], ax
    call emit_line
.done:
    ret

; Emit a centered span. AX is the row and BX is the half-width.
emit_clipped_centered:
    cmp ax, 0
    jl .done
    cmp ax, SCREEN_HEIGHT - 1
    jg .done
    mov dx, [tri_center_x]
    sub dx, bx
    mov [line_x1], dx
    mov dx, [tri_center_x]
    add dx, bx
    mov [line_x2], dx
    mov [line_y1], ax
    mov [line_y2], ax
    call emit_line
.done:
    ret
%endif

emit_scan_probe:
    push ax
    mov ax, SGP_COMMAND_SET_DEST
    stosw
    mov ax, 3
    stosw
    mov ax, 8
    stosw
    mov ax, 1
    stosw
    mov ax, SCREEN_PITCH
    stosw
    mov ax, [draw_page_sgp_low]
    stosw
    mov ax, [draw_page_sgp_high]
    stosw
    mov ax, SGP_COMMAND_SCAN_RIGHT
    stosw

    mov ax, SGP_COMMAND_SET_DEST
    stosw
    mov ax, 3
    stosw
    mov ax, 8
    stosw
    mov ax, 1
    stosw
    mov ax, SCREEN_PITCH
    stosw
    mov ax, [draw_page_sgp_low]
    stosw
    mov ax, [draw_page_sgp_high]
    stosw
    mov ax, SGP_COMMAND_SCAN_LEFT
    stosw
    pop ax
    ret

; AL is a 0..63 phase. Return cosine in AX and sine in DX, both signed Q7.
load_sine_cosine:
    push bx
    xor ah, ah
    and ax, 0x003f
    mov bx, ax
    mov al, [sin_table + bx]
    cbw
    mov dx, ax
    add bx, 16
    and bx, 0x003f
    mov al, [sin_table + bx]
    cbw
    pop bx
    ret

emit_shape_edges:
    push ax
    push bx
    push cx
    push dx
    push si
    push bp
    mov bx, [current_shape]
    mov si, [bx + SHAPE_EDGES]
    xor ch, ch
    mov cl, [bx + SHAPE_EDGE_COUNT]
.edge:
    mov al, [si]
    inc si
    call projected_vertex_address
    mov ax, [bp]
    mov [line_x1], ax
    mov ax, [bp + 2]
    mov [line_y1], ax
    mov ax, [bp + 4]
    mov [line_z1], ax

    mov al, [si]
    inc si
    call projected_vertex_address
    mov ax, [bp]
    mov [line_x2], ax
    mov ax, [bp + 2]
    mov [line_y2], ax
    mov ax, [bp + 4]
    add ax, [line_z1]

    mov bx, [current_shape]
    test ax, ax
    js .dim
    mov ax, [bx + SHAPE_BRIGHT_COLOR]
    jmp .color_ready
.dim:
    mov ax, [bx + SHAPE_DIM_COLOR]
.color_ready:
    call emit_set_color
    call emit_line
    loop .edge

    pop bp
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; Convert the vertex index in AL to a pointer in BP.
projected_vertex_address:
    push ax
    push bx
    xor ah, ah
    mov bp, ax
    shl ax, 1
    shl bp, 1
    shl bp, 1
    add bp, ax
    mov bx, [current_shape]
    add bp, [bx + SHAPE_PROJECTED]
    pop bx
    pop ax
    ret
emit_set_color:
    cmp ax, [last_line_color]
    je .done
    mov [last_line_color], ax
    push ax
    mov ax, SGP_COMMAND_SET_COLOR
    stosw
    pop ax
    stosw
.done:
    ret

; Emit one documented SGP LINE command. Direction is encoded with
; VD=0800h and HD=0400h; width and height include both endpoints.
emit_line:
    push ax
    push bx
    push cx
    push dx
    mov ax, SGP_COMMAND_LINE
    stosw
    mov bx, SGP_LINE_COPY

    mov ax, [line_x2]
    sub ax, [line_x1]
    jns .x_positive
    neg ax
    or bx, SGP_LINE_HD
.x_positive:
    inc ax
    mov [line_width], ax

    mov ax, [line_y2]
    sub ax, [line_y1]
    jns .y_positive
    neg ax
    or bx, SGP_LINE_VD
.y_positive:
    inc ax
    mov [line_height], ax

    mov ax, bx
    stosw
    ; A 16-bpp pixel occupies one SGP word, so the descriptor starts at
    ; lane zero with scrnmode=3. The destination address is aligned to the
    ; two-byte pixel.
    mov ax, 3
    stosw
    mov ax, [line_width]
    stosw
    mov ax, [line_height]
    stosw
    mov ax, SCREEN_PITCH
    stosw

    mov ax, [line_y1]
    mov bx, SCREEN_PITCH
    mul bx
    mov bx, [line_x1]
    shl bx, 1
    add ax, bx
    adc dx, 0
    add ax, [draw_page_sgp_low]
    adc dx, [draw_page_sgp_high]
    stosw
    mov ax, dx
    stosw

    pop dx
    pop cx
    pop bx
    pop ax
    ret

advance_shape_phases:
    push ax
    push bx
    push cx
    mov bx, shape_records
    mov cx, SHAPE_COUNT
.next:
    mov al, [bx + SHAPE_SPEED_Y]
    add [bx + SHAPE_PHASE_Y], al
    and byte [bx + SHAPE_PHASE_Y], 0x3f
    mov al, [bx + SHAPE_SPEED_X]
    add [bx + SHAPE_PHASE_X], al
    and byte [bx + SHAPE_PHASE_X], 0x3f
    mov al, [bx + SHAPE_SCALE_SPEED]
    add [bx + SHAPE_SCALE_PHASE], al
    and byte [bx + SHAPE_SCALE_PHASE], 0x3f
    add bx, SHAPE_RECORD_SIZE
    loop .next
    pop cx
    pop bx
    pop ax
    ret

poll_keyboard:
    mov ah, 0x0a
    int KEYBOARD_BIOS_INT
    jc .none
    mov ah, 0x09
    int KEYBOARD_BIOS_INT
    cmp ah, 0
    je .escape
.check_up:
    cmp ah, 0x48
    je .increase_count
    cmp ah, 0x3a
    je .increase_count
    cmp al, '+'
    je .increase_count
    cmp ah, 0x50
    je .decrease_count
    cmp ah, 0x3d
    je .decrease_count
    cmp al, '-'
    je .decrease_count
.none:
    clc
    ret
.increase_count:
    cmp word [active_shape_count], TRIANGLE_MAX_COUNT
    jae .none
    inc word [active_shape_count]
    clc
    ret
.decrease_count:
    cmp word [active_shape_count], TRIANGLE_MIN_COUNT
    jbe .none
    dec word [active_shape_count]
    clc
    ret
.escape:
    stc
    ret
wait_vblank_start:
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
    stc
    ret
.ready:
    clc
    ret

; The command address ports are word ports. The control and busy latches are
; byte ports. SET WORK is always the first command in every list.
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
    call wait_sgp_idle
    ret
.failed:
    stc
    ret

wait_sgp_idle:
    mov dx, PORT_SGP_STATUS
    mov cx, 0xffff
.poll:
    in al, dx
    test al, SGP_BUSY
    jz .ready
    loop .poll
    stc
    ret
.ready:
    clc
    ret

physical_address_from_es_si:
    mov ax, es
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
    test al, 0x10
    jz .bios_state
    mov byte [saved_single_plane], 1
    mov dx, PORT_GVRAM_WRITE_MODE
    in al, dx
    mov [saved_write_mode], al
.bios_state:
    mov ax, VIDEO_BIOS_DATA_SEG
    mov es, ax
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
    je .write_mode
    mov bx, [saved_video_mode]
    mov cl, [saved_g0_bpp]
    mov ch, [saved_g1_bpp]
    xor dx, dx
    xor ax, ax
    int VIDEO_BIOS_INT
    mov ax, 0x0a00
    int VIDEO_BIOS_INT
    mov dx, PORT_MEMORY_MAP
    mov al, [saved_memory_map]
    out dx, al
.write_mode:
    cmp byte [saved_single_plane], 0
    je .done
    mov dx, PORT_GVRAM_WRITE_MODE
    mov al, [saved_write_mode]
    out dx, al
.done:
    ret

print_string:
    mov ah, 0x09
    int 0x21
    ret

message_start:
    db "SGP triangle plates: 16-bit direct color, two G0 pages", 13, 10
    db "Up/Down changes triangle count; ESC exits.", 13, 10, "$"
message_done:
    db "Video state restored.", 13, 10, "$"
message_initialization_failed:
    db "Video or SGP initialization failed.", 13, 10, "$"
message_animation_failed:
    db "Animation synchronization failed.", 13, 10, "$"

align 2, db 0
g0_framebuffer_descriptor:
    dw 16, SCREEN_WIDTH, SCREEN_HEIGHT * 2
g0_window_descriptor:
    dw 0, 0, SCREEN_HEIGHT, 0, 0

; Signed Q7 sine values for one complete turn in 64 steps.
sin_table:
    db 0, 12, 25, 37, 49, 60, 71, 81
    db 90, 98, 106, 112, 117, 122, 125, 127
    db 127, 127, 125, 122, 117, 112, 106, 98
    db 90, 81, 71, 60, 49, 37, 25, 12
    db 0, -12, -25, -37, -49, -60, -71, -81
    db -90, -98, -106, -112, -117, -122, -125, -127
    db -127, -127, -125, -122, -117, -112, -106, -98
    db -90, -81, -71, -60, -49, -37, -25, -12

align 2, db 0
shape_records:
    dw triangle0_vertices, triangle_edges, triangle0_projected, triangle0_rotated
    db 3, 3
    dw 70, 55
    db 0, 7, 1, 2, 0, 1
    dw 96, 4, DIRECT16(31, 20, 4), DIRECT16(8, 8, 8)
    dw 0, 0

    dw triangle1_vertices, triangle_edges, triangle1_projected, triangle1_rotated
    db 3, 3
    dw 160, 75
    db 11, 5, 2, 1, 16, 2
    dw 104, 5, DIRECT16(8, 48, 31), DIRECT16(4, 16, 20)
    dw 0, 0

    dw triangle2_vertices, triangle_edges, triangle2_projected, triangle2_rotated
    db 3, 3
    dw 245, 125
    db 23, 3, 1, 1, 32, 1
    dw 92, 5, DIRECT16(8, 48, 12), DIRECT16(4, 12, 4)
    dw 0, 0

    dw triangle3_vertices, triangle_edges, triangle3_projected, triangle3_rotated
    db 3, 3
    dw 145, 165
    db 37, 9, 2, 3, 48, 2
    dw 110, 4, DIRECT16(31, 63, 31), DIRECT16(6, 16, 24)
    dw 0, 0

triangle0_vertices:
    db 0, -34, 0, 30, -25, 0, 30, 25, 0
triangle1_vertices:
    db 0, -30, 0, 27, -22, 0, 27, 22, 0
triangle2_vertices:
    db 0, -28, 0, 25, -22, 0, 25, 22, 0
triangle3_vertices:
    db 0, -36, 0, 32, -28, 0, 32, 28, 0
triangle_edges:
    db 0, 1, 1, 2, 2, 0


align 2, db 0
triangle0_projected: times 3 * PROJECTED_VERTEX_SIZE db 0
triangle1_projected: times 3 * PROJECTED_VERTEX_SIZE db 0
triangle2_projected: times 3 * PROJECTED_VERTEX_SIZE db 0
triangle3_projected: times 3 * PROJECTED_VERTEX_SIZE db 0
triangle0_rotated: times 3 * PROJECTED_VERTEX_SIZE db 0
triangle1_rotated: times 3 * PROJECTED_VERTEX_SIZE db 0
triangle2_rotated: times 3 * PROJECTED_VERTEX_SIZE db 0
triangle3_rotated: times 3 * PROJECTED_VERTEX_SIZE db 0

align 2, db 0
saved_memory_map: db 0
saved_write_mode: db 0
saved_single_plane: db 0
video_mode_changed: db 0
saved_video_mode: dw 0
saved_g0_bpp: db 0
saved_g1_bpp: db 0
draw_page_sgp_low: dw 0
draw_page_sgp_high: dw 0
draw_page_dsa_low: dw 0
draw_page_dsa_high: dw 0
draw_page_index: db 0
sgp_command_address_low: dw 0
sgp_command_address_high: dw 0
current_shape: dw 0
last_line_color: dw 0
sin_x: dw 0
cos_x: dw 0
sin_y: dw 0
cos_y: dw 0
current_scale: dw 0
perspective_scale: dw 0
vertex_x: dw 0
vertex_y: dw 0
vertex_z: dw 0
rotated_x: dw 0
rotated_y: dw 0
rotated_z1: dw 0
rotated_z: dw 0
rotation_term: dw 0
line_x1: dw 0
line_y1: dw 0
line_z1: dw 0
line_x2: dw 0
line_y2: dw 0
line_width: dw 0
line_height: dw 0
active_shape_count: dw 3
tri_v0x: dw 0
tri_v0y: dw 0
tri_v1x: dw 0
tri_v1y: dw 0
tri_v2x: dw 0
tri_v2y: dw 0
tri_min_x: dw 0
tri_max_x: dw 0
tri_center_x: dw 0
tri_quarter_width: dw 0
normal_edge1_x: dw 0
normal_edge1_y: dw 0
normal_edge2_x: dw 0
normal_edge2_y: dw 0
normal_cross_z: dw 0
