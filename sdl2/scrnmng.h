/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef VAEG_SDL2_SCRNMNG_H
#define VAEG_SDL2_SCRNMNG_H

#include "viewport.h"

enum {
	VAEG_EFFECT_UNFILTERED = 0,
	VAEG_EFFECT_LINEAR,
	VAEG_EFFECT_SCANLINE,
	VAEG_EFFECT_CRT_LITE,
	VAEG_EFFECT_COUNT
};

enum {
	SCRNMNG_SURFACE_GUARD_LEFT = 1
};

enum { VAEG_WINDOW_SCALE_MAX = 16 };

typedef struct {
	BYTE *ptr;
	int xalign;
	int yalign;
	int width;
	int height;
	UINT bpp;
	int extend;
} SCRNSURF;

#ifdef __cplusplus
extern "C" {
#endif

void scrnmng_setwidth(int posx, int width);
#define scrnmng_setextend(e)
void scrnmng_setheight(int posy, int height);
const SCRNSURF *scrnmng_surflock(void);
void scrnmng_surfunlock(const SCRNSURF *surf);

#define scrnmng_haveextend() (0)
#define scrnmng_getbpp() (16)
#define scrnmng_allflash()
#define scrnmng_palchanged()

RGB16 scrnmng_makepal16(RGB32 pal32);

void scrnmng_initialize(void);
BOOL scrnmng_create(int width, int height);
void scrnmng_show(void);
void scrnmng_destroy(void);
void *scrnmng_get_window(void);
void *scrnmng_get_renderer(void);
BOOL scrnmng_get_output_size(int *width, int *height);
BOOL scrnmng_present_startup_image(const void *pixels, int width, int height, int pitch,
                                    int x, int y, int output_width, int output_height);
const char *scrnmng_get_renderer_backend(void);
void scrnmng_set_menu_height(int height);
void scrnmng_set_display(int scale, BOOL aspect);
int scrnmng_get_display_scale(void);
BOOL scrnmng_get_display_aspect(void);
void scrnmng_set_scaling(int scaling);
int scrnmng_get_scaling(void);
void scrnmng_set_effect(int effect);
int scrnmng_get_effect(void);
BOOL scrnmng_get_viewport(VAEG_VIEWPORT *viewport);
BOOL scrnmng_map_window_point(int window_x, int window_y, int *guest_x, int *guest_y);
BOOL scrnmng_set_display_mode(int mode, int monitor, UINT width, UINT height, UINT refresh,
                              UINT8 fscrnmod);
int scrnmng_get_display_mode(void);
BOOL scrnmng_isfullscreen(void);
BOOL scrnmng_capture_window_size(int *width, int *height);
BOOL scrnmng_request_display_capture(const char *path);
BOOL scrnmng_prepare_display_capture(void);
BOOL scrnmng_display_capture_selftest(void);
void scrnmng_log_geometry(const char *reason);
BOOL scrnmng_texture_uniform(BOOL *uniform);
BOOL scrnmng_native_active(void);
BOOL scrnmng_take_native_fallback(void);
BOOL scrnmng_window_rebind_selftest(void);
void scrnmng_draw_native_overlays(void);
void scrnmng_draw_sdl_overlays(void);
BOOL scrnmng_native_gui_prepare(void);
BOOL scrnmng_fallback_to_sdl(void);
void scrnmng_native_gui_shutdown(void);
void scrnmng_request_native_crt(BOOL enabled, BOOL reload);
BOOL scrnmng_native_filter_enabled(void);
BOOL scrnmng_set_native_filter(BOOL enabled);
BOOL scrnmng_apply_native_crt_request(void);
const char *scrnmng_native_status(void);
const char *scrnmng_native_preset_path(void);
BOOL scrnmng_native_set_parameter(const char *name, float value);
float scrnmng_native_mask_intensity(void);
void scrnmng_apply_native_mask_profile(void);
void scrnmng_present_begin(void);
void scrnmng_present_end(void);
BOOL scrnmng_save_rendered_frame(const char *path);
BOOL scrnmng_save_guest_frame(const char *path);
BOOL scrnmng_save_guest_frame_with_analysis(const char *path);
void scrnmng_set_framedisp(BOOL enabled);
void scrnmng_reset_metrics(void);
void scrnmng_refresh_title(void);
void scrnmng_framedisp_tick(UINT32 tick, UINT32 draws, UINT32 frames);

typedef struct {
	int width;
	int height;
	int bpp;
} SCRNMENU;

BOOL scrnmng_entermenu(SCRNMENU *smenu);
void scrnmng_leavemenu(void);
void scrnmng_menudraw(const RECT_T *rct);

#ifdef __cplusplus
}
#endif

#endif
