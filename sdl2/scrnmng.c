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
#include <string.h>
#include "compiler.h"
#include "sdlapi.h"
#include "scrnmng.h"
#include "np2.h"
#include "np2ver.h"
#include "machine/pccore.h"
#include "sgp.h"
#include "videova.h"
#include "tsp.h"
#include "fontdata.h"
#include "diskdrv.h"
#include "fddfile.h"
#include "appicon.h"
#include "framedisp.h"
#include "librashader/native_presenter_controller.h"
#include "librashader/builtin_shaders.h"
#include "gui/gui.h"

typedef struct {
	BOOL enable;
	int width;
	int height;
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	char renderer_backend[64];
	BYTE *shadow;
	int shadow_pitch;
	BOOL visible;
	int scale;
	BOOL aspect;
	int menu_height;
	int scaling;
	int effect;
	int display_mode;
	int window_x;
	int window_y;
	int window_width;
	int window_height;
	BOOL window_maximized;
	UINT8 fscrnmod;
	BOOL dirty;
	BOOL framedisp_enabled;
	VAEG_FRAMEDISP framedisp;
	BOOL rendered_capture_enabled;
	SDL_Surface *rendered_frame;
	VAEG_NATIVE_PRESENTER *native_presenter;
	BOOL native_active;
	BOOL native_fallback_pending;
	UINT64 native_frame_number;
	BOOL native_capture_warning;
	BOOL native_change_pending;
	BOOL native_reload_pending;
	char native_status[256];
	char display_capture_path[MAX_PATH];
	BOOL display_capture_ready;
	BOOL sdl_overlays_drawn;
} SCRNMNG;

typedef struct {
	int width;
	int height;
} SCRNSTAT;

typedef struct {
	BOOL valid;
	UINT32 tick;
	UINT32 frames;
	UINT32 fps_tenths;
} VAEG_SPEEDMETER;

static const char app_name[] = "88VA Eternal Grafx " VAEGREL_CORE;
static const UINT32 vaeg_nominal_frame_rate = 60;
enum {
	SCRNMNG_CANVAS_WIDTH = 640,
	SCRNMNG_CANVAS_HEIGHT = 400,
	/* Taller frames (up to 480 rasters, measured in M104) grow the canvas. */
	SCRNMNG_CANVAS_MAX_HEIGHT = 480
};

static SCRNMNG scrnmng;
static SCRNSTAT scrnstat;
static SCRNSURF scrnsurf;
static VAEG_SPEEDMETER speedmeter;

static BOOL scrnmng_calculate_viewport(VAEG_VIEWPORT *viewport);
static void scrnmng_draw_video_info_overlay(const VAEG_VIEWPORT *viewport);
static void scrnmng_draw_framebuffer_info_overlay(const VAEG_VIEWPORT *viewport);
static BOOL scrnmng_get_drawable_size(int *width, int *height);
static BOOL scrnmng_create_sdl_resources(void);
static BOOL scrnmng_native_fallback(void);

/* NULL selects the session's vaeg.cfg-backed CRT parameter store. */
static const char *const scrnmng_native_parameter_state = NULL;

const char *scrnmng_native_preset_path(void) {
#if defined(VAEG_STATIC_LIBRASHADER)
	const char *configured = np2oscfg.gui_shader_preset;
	if (configured[0] && strcmp(configured, VAEG_DEFAULT_SHADER_PRESET) != 0)
		return configured;
	return vaeg_builtin_shader_preset();
#else
	static char bundled_path[4096];
	const char *configured = np2oscfg.gui_shader_preset;
	SDL_RWops *file;
	char *base;
	if (configured[0] && strcmp(configured, VAEG_DEFAULT_SHADER_PRESET) != 0)
		return configured;
	// Shipped assets follow the executable, independently of the launch directory.
	base = SDL_GetBasePath();
	if (base) {
		snprintf(bundled_path, sizeof(bundled_path), "%s%s", base, VAEG_DEFAULT_SHADER_PRESET);
		SDL_free(base);
		file = SDL_RWFromFile(bundled_path, "rb");
		if (file) {
			SDL_RWclose(file);
			return bundled_path;
		}
	}
	return VAEG_DEFAULT_SHADER_PRESET;
#endif
}

static BOOL scrnmng_native_requested(void) {
	const char *video_driver;

	const char *override = SDL_getenv("VAEG_NATIVE_CRT");
	if (override ? strcmp(override, "1") != 0 : !np2oscfg.gui_native_crt) {
		return FALSE;
	}
	video_driver = SDL_GetCurrentVideoDriver();
	return (video_driver == NULL) ||
	       !vaeg_native_presenter_is_headless_video_driver(video_driver);
}

static void scrnmng_capture_dummy_frame(void) {
	VAEG_VIEWPORT viewport;
	SDL_Surface *source;
	SDL_Rect dst;

	if ((scrnmng.rendered_frame == NULL) || (scrnmng.shadow == NULL)) {
		return;
	}
	source = SDL_CreateRGBSurfaceWithFormatFrom(scrnmng.shadow + (SCRNMNG_SURFACE_GUARD_LEFT * 2),
	                                            scrnmng.width, scrnmng.height, 16,
	                                            scrnmng.shadow_pitch, SDL_PIXELFORMAT_RGB565);
	if (source == NULL) {
		fprintf(stderr, "scsitrace rendered-screen-source-failed error=%s\n", SDL_GetError());
		return;
	}
	(void)SDL_FillRect(scrnmng.rendered_frame, NULL,
	                   SDL_MapRGB(scrnmng.rendered_frame->format, 0, 0, 0));
	if (scrnmng_calculate_viewport(&viewport) == SUCCESS) {
		dst.x = viewport.x;
		dst.y = viewport.y;
		dst.w = viewport.width;
		dst.h = viewport.height;
		(void)SDL_BlitScaled(source, NULL, scrnmng.rendered_frame, &dst);
	}
	SDL_FreeSurface(source);
}

static void scrnmng_capture_rendered_frame(void) {
	int width;
	int height;

	if (scrnmng.renderer == NULL) {
		return;
	}
	if (SDL_GetRendererOutputSize(scrnmng.renderer, &width, &height) != 0 || (width <= 0) ||
	    (height <= 0)) {
		fprintf(stderr, "scsitrace rendered-screen-size-failed error=%s\n", SDL_GetError());
		return;
	}
	if ((scrnmng.rendered_frame == NULL) || (scrnmng.rendered_frame->w != width) ||
	    (scrnmng.rendered_frame->h != height)) {
		if (scrnmng.rendered_frame != NULL) {
			SDL_FreeSurface(scrnmng.rendered_frame);
			scrnmng.rendered_frame = NULL;
		}
		scrnmng.rendered_frame =
		    SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_ARGB8888);
		if (scrnmng.rendered_frame == NULL) {
			fprintf(stderr, "scsitrace rendered-screen-surface-failed error=%s\n", SDL_GetError());
			return;
		}
	}
	if ((SDL_GetCurrentVideoDriver() != NULL) && !strcmp(SDL_GetCurrentVideoDriver(), "dummy")) {
		scrnmng_capture_dummy_frame();
		return;
	}
	if (SDL_RenderReadPixels(scrnmng.renderer, NULL, scrnmng.rendered_frame->format->format,
	                         scrnmng.rendered_frame->pixels, scrnmng.rendered_frame->pitch) != 0) {
		fprintf(stderr, "scsitrace rendered-screen-read-failed error=%s\n", SDL_GetError());
	}
}

static UINT32 scrnmng_png_crc32_update(UINT32 crc, const BYTE *data, size_t length) {
	size_t i;
	int bit;

	for (i = 0; i < length; i++) {
		crc ^= data[i];
		for (bit = 0; bit < 8; bit++) {
			crc = (crc & 1) ? ((crc >> 1) ^ 0xedb88320U) : (crc >> 1);
		}
	}
	return crc;
}

static UINT32 scrnmng_png_adler32(const BYTE *data, size_t length) {
	UINT32 a = 1;
	UINT32 b = 0;
	size_t i;

	for (i = 0; i < length; i++) {
		a += data[i];
		if (a >= 65521U) {
			a -= 65521U;
		}
		b += a;
		if (b >= 65521U) {
			b -= 65521U;
		}
	}
	return (b << 16) | a;
}

static BOOL scrnmng_png_write_u32(FILE *fp, UINT32 value) {
	BYTE bytes[4];

	bytes[0] = (BYTE)(value >> 24);
	bytes[1] = (BYTE)(value >> 16);
	bytes[2] = (BYTE)(value >> 8);
	bytes[3] = (BYTE)value;
	return (fwrite(bytes, 1, sizeof(bytes), fp) == sizeof(bytes)) ? SUCCESS : FAILURE;
}

static BOOL scrnmng_png_write_chunk(FILE *fp, const char type[4], const BYTE *data,
	                                  UINT32 length) {
	UINT32 crc;

	if (scrnmng_png_write_u32(fp, length) != SUCCESS || fwrite(type, 1, 4, fp) != 4) {
		return FAILURE;
	}
	if ((length != 0) && (fwrite(data, 1, length, fp) != length)) {
		return FAILURE;
	}
	crc = scrnmng_png_crc32_update(0xffffffffU, (const BYTE *)type, 4);
	if (length != 0) {
		crc = scrnmng_png_crc32_update(crc, data, length);
	}
	crc ^= 0xffffffffU;
	return scrnmng_png_write_u32(fp, crc);
}

static BOOL scrnmng_png_save_surface(SDL_Surface *surface, const char *path) {
	BYTE ihdr[13];
	BYTE *raw = NULL;
	BYTE *compressed = NULL;
	FILE *fp = NULL;
	size_t row_bytes;
	size_t raw_length;
	size_t compressed_length;
	size_t blocks;
	size_t source_y;
	size_t source_x;
	size_t compressed_pos;
	size_t raw_pos;
	size_t remaining;
	BOOL locked = FALSE;
	BOOL result = FAILURE;

	if ((surface == NULL) || (path == NULL) || (surface->w <= 0) || (surface->h <= 0)) {
		return FAILURE;
	}
	row_bytes = (size_t)surface->w * 4 + 1;
	if ((row_bytes > (SIZE_MAX / (size_t)surface->h)) ||
	    ((raw_length = row_bytes * (size_t)surface->h) > UINT32_MAX)) {
		return FAILURE;
	}
	blocks = (raw_length + 65534) / 65535;
	if ((blocks > (SIZE_MAX - raw_length - 6) / 5) ||
	    ((compressed_length = raw_length + 6 + blocks * 5) > UINT32_MAX)) {
		return FAILURE;
	}
	raw = (BYTE *)malloc(raw_length);
	compressed = (BYTE *)malloc(compressed_length);
	if ((raw == NULL) || (compressed == NULL)) {
		goto cleanup;
	}
	if (SDL_MUSTLOCK(surface)) {
		if (SDL_LockSurface(surface) != 0) {
			goto cleanup;
		}
		locked = TRUE;
	}
	for (source_y = 0; source_y < (size_t)surface->h; source_y++) {
		BYTE *destination = raw + source_y * row_bytes;
		const BYTE *source = (const BYTE *)surface->pixels + source_y * (size_t)surface->pitch;

		destination[0] = 0;
		for (source_x = 0; source_x < (size_t)surface->w; source_x++) {
			const BYTE *pixel_bytes = source + source_x * (size_t)surface->format->BytesPerPixel;
			UINT32 pixel = 0;
			UINT8 red;
			UINT8 green;
			UINT8 blue;
			UINT8 alpha;

			switch (surface->format->BytesPerPixel) {
			case 1:
				pixel = pixel_bytes[0];
				break;
			case 2:
				memcpy(&pixel, pixel_bytes, 2);
				break;
			case 3:
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
				pixel = ((UINT32)pixel_bytes[0] << 16) | ((UINT32)pixel_bytes[1] << 8) |
				        pixel_bytes[2];
#else
				pixel = pixel_bytes[0] | ((UINT32)pixel_bytes[1] << 8) |
				        ((UINT32)pixel_bytes[2] << 16);
#endif
				break;
			default:
				memcpy(&pixel, pixel_bytes, 4);
				break;
			}
			SDL_GetRGBA(pixel, surface->format, &red, &green, &blue, &alpha);
			destination[1 + source_x * 4 + 0] = red;
			destination[1 + source_x * 4 + 1] = green;
			destination[1 + source_x * 4 + 2] = blue;
			destination[1 + source_x * 4 + 3] = alpha;
		}
	}
	if (locked) {
		SDL_UnlockSurface(surface);
		locked = FALSE;
	}

	compressed[0] = 0x78;
	compressed[1] = 0x01;
	compressed_pos = 2;
	raw_pos = 0;
	remaining = raw_length;
	while (remaining != 0) {
		size_t block_length = (remaining > 65535) ? 65535 : remaining;
		compressed[compressed_pos++] = (remaining == block_length) ? 1 : 0;
		compressed[compressed_pos++] = (BYTE)block_length;
		compressed[compressed_pos++] = (BYTE)(block_length >> 8);
		compressed[compressed_pos++] = (BYTE)~block_length;
		compressed[compressed_pos++] = (BYTE)(~block_length >> 8);
		memcpy(compressed + compressed_pos, raw + raw_pos, block_length);
		compressed_pos += block_length;
		raw_pos += block_length;
		remaining -= block_length;
	}
	{
		UINT32 adler = scrnmng_png_adler32(raw, raw_length);
		compressed[compressed_pos++] = (BYTE)(adler >> 24);
		compressed[compressed_pos++] = (BYTE)(adler >> 16);
		compressed[compressed_pos++] = (BYTE)(adler >> 8);
		compressed[compressed_pos++] = (BYTE)adler;
	}
	if (compressed_pos != compressed_length) {
		goto cleanup;
	}
	ihdr[0] = (BYTE)((UINT32)surface->w >> 24);
	ihdr[1] = (BYTE)((UINT32)surface->w >> 16);
	ihdr[2] = (BYTE)((UINT32)surface->w >> 8);
	ihdr[3] = (BYTE)surface->w;
	ihdr[4] = (BYTE)((UINT32)surface->h >> 24);
	ihdr[5] = (BYTE)((UINT32)surface->h >> 16);
	ihdr[6] = (BYTE)((UINT32)surface->h >> 8);
	ihdr[7] = (BYTE)surface->h;
	ihdr[8] = 8;
	ihdr[9] = 6;
	ihdr[10] = 0;
	ihdr[11] = 0;
	ihdr[12] = 0;
	fp = fopen(path, "wb");
	if (fp == NULL) {
		goto cleanup;
	}
	if (fwrite("\x89PNG\r\n\x1a\n", 1, 8, fp) != 8 ||
	    scrnmng_png_write_chunk(fp, "IHDR", ihdr, sizeof(ihdr)) != SUCCESS ||
	    scrnmng_png_write_chunk(fp, "IDAT", compressed, (UINT32)compressed_length) != SUCCESS ||
	    scrnmng_png_write_chunk(fp, "IEND", NULL, 0) != SUCCESS) {
		goto cleanup;
	}
	if (fclose(fp) != 0) {
		fp = NULL;
		goto cleanup;
	}
	fp = NULL;
	result = SUCCESS;

cleanup:
	if (locked) {
		SDL_UnlockSurface(surface);
	}
	if (fp != NULL) {
		(void)fclose(fp);
	}
	free(compressed);
	free(raw);
	return result;
}

static BOOL scrnmng_path_is_png(const char *path) {
	const char *extension;
	unsigned char first;
	unsigned char second;
	unsigned char third;

	if (path == NULL) {
		return FALSE;
	}
	extension = strrchr(path, '.');
	if ((extension == NULL) || (strlen(extension) != 4)) {
		return FALSE;
	}
	first = (unsigned char)extension[1];
	second = (unsigned char)extension[2];
	third = (unsigned char)extension[3];
	if ((first >= 'A') && (first <= 'Z')) {
		first = (unsigned char)(first - 'A' + 'a');
	}
	if ((second >= 'A') && (second <= 'Z')) {
		second = (unsigned char)(second - 'A' + 'a');
	}
	if ((third >= 'A') && (third <= 'Z')) {
		third = (unsigned char)(third - 'A' + 'a');
	}
	return (first == 'p') && (second == 'n') && (third == 'g') && (extension[4] == '\0');
}

static UINT32 scrnmng_measured_clock(UINT32 configured_clock) {
	UINT64 measured;

	if (!speedmeter.valid || (speedmeter.fps_tenths == 0)) {
		return configured_clock;
	}
	measured = (UINT64)configured_clock * speedmeter.fps_tenths;
	measured /= (UINT64)vaeg_nominal_frame_rate * 10;
	if (measured > 0xffffffffULL) {
		return 0xffffffffU;
	}
	return (UINT32)measured;
}

static void scrnmng_update_speedmeter(UINT32 tick, UINT32 frames) {
	UINT32 elapsed;
	UINT32 frame_delta;
	UINT64 fps_tenths;

	if (!speedmeter.valid || (frames < speedmeter.frames)) {
		speedmeter.valid = TRUE;
		speedmeter.tick = tick;
		speedmeter.frames = frames;
		speedmeter.fps_tenths = 0;
		return;
	}
	elapsed = tick - speedmeter.tick;
	if (elapsed < 1000) {
		return;
	}
	frame_delta = frames - speedmeter.frames;
	fps_tenths = ((UINT64)frame_delta * 10000) / elapsed;
	if (fps_tenths > 0xffffffffULL) {
		fps_tenths = 0xffffffffULL;
	}
	speedmeter.tick = tick;
	speedmeter.frames = frames;
	speedmeter.fps_tenths = (UINT32)fps_tenths;
}

static const char *scrnmng_fdd_title_path(int drive) {
	const char *path;

	path = fdd_diskname((REG8)drive);
	if ((path == NULL) || (path[0] == '\0')) {
		path = diskdrv_fname[drive];
	}
	return ((path != NULL) ? path : "");
}

static const char *scrnmng_fdd_display_name(const char *path) {
	const char *name;
	const char *separator;

	name = path;
	separator = strrchr(name, '/');
	if (separator != NULL) {
		name = separator + 1;
	}
	separator = strrchr(name, '\\');
	if (separator != NULL) {
		name = separator + 1;
	}
	return name;
}

static void scrnmng_append_fdd_title(char *title, size_t title_size, int *length, int drive) {
	const char *path;
	const char *name;

	if ((title == NULL) || (length == NULL) || (*length < 0) ||
	    ((size_t)*length >= title_size)) {
		return;
	}
	path = scrnmng_fdd_title_path(drive);
	name = (path[0] != '\0') ? scrnmng_fdd_display_name(path) : "Empty";
	*length += snprintf(title + *length, title_size - (size_t)*length, " - FDD%d: %s", drive + 1,
	                    name);
}

static void scrnmng_update_title(void) {
	char title[512];
	UINT32 cpu_clock;
	UINT32 sgp_clock;
	int length;

	if (scrnmng.window == NULL) {
		return;
	}
	cpu_clock = scrnmng_measured_clock(pccore_cpu_clock());
	sgp_clock = scrnmng_measured_clock(sgp_effective_clock());
	length = snprintf(title, sizeof(title), "%s", app_name);
	if ((np2oscfg.DISPCLK & VAEG_DISPINFO_FDD) != 0) {
		scrnmng_append_fdd_title(title, sizeof(title), &length, 0);
		scrnmng_append_fdd_title(title, sizeof(title), &length, 1);
	}
	if ((np2oscfg.DISPCLK & VAEG_DISPINFO_FPS) && (length >= 0) &&
	    ((size_t)length < sizeof(title))) {
		length += snprintf(title + length, sizeof(title) - (size_t)length, " - %u.%1uFPS",
		                   (unsigned int)(scrnmng.framedisp.fps_tenths / 10),
		                   (unsigned int)(scrnmng.framedisp.fps_tenths % 10));
	}
	if ((np2oscfg.DISPCLK & VAEG_DISPINFO_CPU_CLOCK) && (length >= 0) &&
	    ((size_t)length < sizeof(title))) {
		length += snprintf(title + length, sizeof(title) - (size_t)length, " - CPU %u.%04uMHz",
		                   (unsigned int)(cpu_clock / 1000000),
		                   (unsigned int)((cpu_clock % 1000000) / 100));
	}
	if ((np2oscfg.DISPCLK & VAEG_DISPINFO_SGP_CLOCK) && (length >= 0) &&
	    ((size_t)length < sizeof(title))) {
		length += snprintf(title + length, sizeof(title) - (size_t)length, " - SGP %u.%04uMHz",
		                   (unsigned int)(sgp_clock / 1000000),
		                   (unsigned int)((sgp_clock % 1000000) / 100));
	}
	if (scrnmng.framedisp_enabled && (length >= 0) && ((size_t)length < sizeof(title))) {
		length += snprintf(title + length, sizeof(title) - (size_t)length, " - FRAME %u",
		                   (unsigned int)drawcount);
	}
	SDL_SetWindowTitle(scrnmng.window, title);
}

static int scrnmng_video_bpp(UINT mode) {
	switch (mode & 3) {
	case 0:
		return 1;
	case 1:
		return 4;
	case 2:
		return 8;
	default:
		return 16;
	}
}
static void scrnmng_format_video_line(char *line, size_t line_size, const char *label, int width,
                                      int height, int bpp) {
	(void)snprintf(line, line_size, "%s %dx%d %dbpp", label, width, height, bpp);
}
static BOOL scrnmng_framebuffer_valid(FRAMEBUFFER framebuffer, int framebuffer_no) {
	if ((framebuffer == NULL) || (framebuffer->fbw == 0xffff) || (framebuffer->dsh == 0)) {
		return FALSE;
	}
	/* FB1 has no writable FSA/FBL/OFX/OFY fields in the VA model. */
	if ((framebuffer_no != 1) && (framebuffer->fsa == 0xffffffffL)) {
		return FALSE;
	}
	return TRUE;
}
static int scrnmng_framebuffer_width(FRAMEBUFFER framebuffer, int bpp) {
	if ((framebuffer == NULL) || (bpp <= 0) || (framebuffer->fbw == 0xffff)) {
		return 0;
	}
	return ((int)framebuffer->fbw * 8) / bpp;
}
static int scrnmng_framebuffer_height(FRAMEBUFFER framebuffer) {
	if ((framebuffer == NULL) || (framebuffer->dsh == 0)) {
		return 0;
	}
	return framebuffer->dsh;
}
static void scrnmng_format_graphics_line(char *line, size_t line_size, const char *label,
                                         BOOL active, int logical_width, int logical_height,
                                         int bpp, int framebuffer_no, FRAMEBUFFER framebuffer) {
	if (!active) {
		(void)snprintf(line, line_size, "%s OFF", label);
		return;
	}
	if (!scrnmng_framebuffer_valid(framebuffer, framebuffer_no)) {
		(void)snprintf(line, line_size, "%s OFF", label);
		return;
	}
	(void)snprintf(line, line_size, "%s ON %dx%d %dbpp", label, logical_width, logical_height, bpp);
}

static int scrnmng_format_video_info_lines(char lines[][96]) {
	const int graphics_height = (videova.grmode & 0x0002) ? 200 : 400;
	const int text_height = (tsp.screenlines != 0) ? tsp.screenlines : graphics_height;
	const int g0_width = (videova.grres & 0x0010) ? 320 : 640;
	const int g1_width = (videova.grres & 0x1000) ? 320 : 640;
	const int g0_bpp = scrnmng_video_bpp(videova.grres);
	const int g1_bpp = scrnmng_video_bpp(videova.grres >> 8);
	const BOOL g0_active = (videova.grmode & 0x8000) != 0;
	/* G1 is a separate screen only in single-plane, two-screen mode. */
	const BOOL g1_active = g0_active && ((videova.grmode & 0x0c00) == 0x0c00);

	scrnmng_format_video_line(lines[0], sizeof(lines[0]), "TEXT", 640, text_height, 4);
	scrnmng_format_video_line(lines[1], sizeof(lines[1]), "SPRITE", 640, text_height, 4);
	scrnmng_format_graphics_line(lines[2], sizeof(lines[2]), "G0", g0_active, g0_width,
	                             graphics_height, g0_bpp, 0, &videova.framebuffer[0]);
	scrnmng_format_graphics_line(lines[3], sizeof(lines[3]), "G1", g1_active, g1_width,
	                             graphics_height, g1_bpp, 1, &videova.framebuffer[1]);
	return 4;
}

static int scrnmng_format_framebuffer_lines(char lines[][48], int framebuffer_no, int bpp,
                                            FRAMEBUFFER framebuffer) {
	int width;
	int source_height;
	int line_count;

	if (!scrnmng_framebuffer_valid(framebuffer, framebuffer_no)) {
		(void)snprintf(lines[0], 48, "FB%d OFF", framebuffer_no);
		return 1;
	}
	width = scrnmng_framebuffer_width(framebuffer, bpp);
	line_count = 0;
	if (framebuffer->fbl == 0xffff) {
		(void)snprintf(lines[line_count++], 48, "FB%d source %dxN/A", framebuffer_no, width);
	} else {
		source_height = (int)framebuffer->fbl + 1;
		(void)snprintf(lines[line_count++], 48, "FB%d source %dx%d", framebuffer_no, width,
		               source_height);
	}
	(void)snprintf(lines[line_count++], 48, "FB%d view %dx%d", framebuffer_no, width,
	               scrnmng_framebuffer_height(framebuffer));
	(void)snprintf(lines[line_count++], 48, "FB%d DSA %06Xh", framebuffer_no,
	               (unsigned int)(framebuffer->dsa & 0x03ffffu));
	return line_count;
}

static void scrnmng_draw_text_glyphs(int x, int y, int scale, const char *text, SDL_Color color) {
	const unsigned char *glyph;
	int character;
	int row;
	int bit;

	if ((text == NULL) || (scale <= 0)) {
		return;
	}
	if (!scrnmng.native_active)
		SDL_SetRenderDrawColor(scrnmng.renderer, color.r, color.g, color.b, color.a);
	while (*text != '\0') {
		character = (unsigned char)*text++;
		glyph = fontdata_8 + (character * 8);
		for (row = 0; row < 8; row++) {
			for (bit = 0; bit < 8; bit++) {
				if ((glyph[row] & (0x80 >> bit)) != 0) {
					SDL_Rect pixel = {x + bit * scale, y + row * scale, scale, scale};
					if (scrnmng.native_active)
						gui_overlay_rect(pixel.x, pixel.y, pixel.w, pixel.h, color.r, color.g, color.b, color.a);
					else
						SDL_RenderFillRect(scrnmng.renderer, &pixel);
				}
			}
		}
		x += 8 * scale;
	}
}

static void scrnmng_draw_text_glyphs_surface(SDL_Surface *surface, int x, int y, int scale,
                                             const char *text, SDL_Color color) {
	const unsigned char *glyph;
	UINT32 pixel_value;
	int character;
	int row;
	int bit;

	if ((surface == NULL) || (text == NULL) || (scale <= 0)) {
		return;
	}
	pixel_value = SDL_MapRGBA(surface->format, color.r, color.g, color.b, color.a);
	while (*text != '\0') {
		character = (unsigned char)*text++;
		glyph = fontdata_8 + (character * 8);
		for (row = 0; row < 8; row++) {
			for (bit = 0; bit < 8; bit++) {
				if ((glyph[row] & (0x80 >> bit)) != 0) {
					SDL_Rect pixel = {x + bit * scale, y + row * scale, scale, scale};
					(void)SDL_FillRect(surface, &pixel, pixel_value);
				}
			}
		}
		x += 8 * scale;
	}
}

static BOOL scrnmng_draw_graphics_analysis(SDL_Surface *surface) {
	char video_lines[4][96];
	char framebuffer_lines[VIDEOVA_FRAMEBUFFERS * 3][48];
	const int video_count = (np2oscfg.DISPCLK & VAEG_DISPINFO_VIDEO)
	                            ? scrnmng_format_video_info_lines(video_lines) : 0;
	const int line_height = 8;
	const int padding = 4;
	const SDL_Color text_color = {255, 255, 192, 255};
	int framebuffer_count = 0;
	int line_count;
	int width = 0;
	int panel_width;
	int panel_height;
	int panel_y;
	int i;
	SDL_Surface *panel;
	SDL_Rect destination;

	if ((surface == NULL) || (surface->w <= 0) || (surface->h <= 0)) {
		return FAILURE;
	}
	for (i = 0; (np2oscfg.DISPCLK & VAEG_DISPINFO_FRAMEBUFFER) && i < VIDEOVA_FRAMEBUFFERS; i++) {
		const int bpp = (i & 1) ? scrnmng_video_bpp(videova.grres >> 8)
		                       : scrnmng_video_bpp(videova.grres);
		framebuffer_count += scrnmng_format_framebuffer_lines(&framebuffer_lines[framebuffer_count], i,
	                                                     bpp, &videova.framebuffer[i]);
	}
	line_count = video_count + framebuffer_count;
	if (line_count == 0) return SUCCESS;
	for (i = 0; i < video_count; i++) {
		const int length = (int)strlen(video_lines[i]);
		if (length > width) {
			width = length;
		}
	}
	for (i = 0; i < framebuffer_count; i++) {
		const int length = (int)strlen(framebuffer_lines[i]);
		if (length > width) {
			width = length;
		}
	}
	panel_width = width * 8 + padding * 2;
	panel_height = line_count * line_height + padding * 2;
	if (panel_width > surface->w) {
		panel_width = surface->w;
	}
	if (panel_height > surface->h) {
		panel_height = surface->h;
	}
	panel_y = (surface->h > panel_height) ? 2 : 0;
	if (panel_y + panel_height > surface->h) {
		panel_y = surface->h - panel_height;
	}
	panel = SDL_CreateRGBSurfaceWithFormat(0, panel_width, panel_height, 32,
	                                       SDL_PIXELFORMAT_ARGB8888);
	if (panel == NULL) {
		return FAILURE;
	}
	if (SDL_FillRect(panel, NULL, SDL_MapRGBA(panel->format, 0, 0, 0, 190)) != 0) {
		SDL_FreeSurface(panel);
		return FAILURE;
	}
	SDL_SetSurfaceBlendMode(panel, SDL_BLENDMODE_BLEND);
	destination.x = surface->w - panel_width;
	destination.y = panel_y;
	destination.w = panel_width;
	destination.h = panel_height;
	if (SDL_BlitSurface(panel, NULL, surface, &destination) != 0) {
		SDL_FreeSurface(panel);
		return FAILURE;
	}
	SDL_FreeSurface(panel);

	for (i = 0; i < video_count; i++) {
		scrnmng_draw_text_glyphs_surface(surface, destination.x + padding,
		                                  destination.y + padding + i * line_height, 1,
		                                  video_lines[i], text_color);
	}
	for (i = 0; i < framebuffer_count; i++) {
		scrnmng_draw_text_glyphs_surface(surface, destination.x + padding,
		                                  destination.y + padding + (video_count + i) * line_height,
		                                  1, framebuffer_lines[i], text_color);
	}
	return SUCCESS;
}

static int scrnmng_menu_offset(void) {
	int window_width;
	int window_height;
	int output_width;
	int output_height;

	if (scrnmng.window == NULL) {
		return 0;
	}
	SDL_GetWindowSize(scrnmng.window, &window_width, &window_height);
	if ((scrnmng_get_drawable_size(&output_width, &output_height) != SUCCESS) ||
	    (window_height <= 0) || (output_height <= 0)) {
		return scrnmng.menu_height;
	}
	return (int)(((SINT64)scrnmng.menu_height * output_height + (window_height / 2)) /
	             window_height);
}

static void scrnmng_draw_video_info_overlay(const VAEG_VIEWPORT *viewport) {
	char lines[4][96];
	int line_count;
	int scale;
	int line_height;
	int width;
	int height;
	int output_width;
	int output_height;
	int i;
	SDL_Rect background;
	SDL_Color text_color;

	if ((viewport == NULL) || (!viewport->valid) ||
	    ((np2oscfg.DISPCLK & VAEG_DISPINFO_VIDEO) == 0)) {
		return;
	}
	if ((scrnmng_get_drawable_size(&output_width, &output_height) != SUCCESS) ||
	    (output_width <= 0) || (output_height <= 0)) {
		return;
	}
	line_count = scrnmng_format_video_info_lines(lines);

	scale = (int)viewport->scale_x;
	if (scale < 1) {
		scale = 1;
	}
	if (scale > 3) {
		scale = 3;
	}
	line_height = 8 * scale;
	width = 0;
	for (i = 0; i < line_count; i++) {
		const int length = (int)strlen(lines[i]);
		if (length > width) {
			width = length;
		}
	}
	width = width * 8 * scale + 8 * scale;
	height = line_height * line_count + 8 * scale;
	if ((width >= output_width) || (height >= output_height)) {
		return;
	}
	background.x = output_width - width;
	background.y = scrnmng_menu_offset() + 2;
	background.w = width;
	background.h = height;
	if (scrnmng.native_active) {
		gui_overlay_rect(background.x, background.y, background.w, background.h, 0, 0, 0, 190);
	} else {
		SDL_SetRenderDrawBlendMode(scrnmng.renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(scrnmng.renderer, 0, 0, 0, 190);
		SDL_RenderFillRect(scrnmng.renderer, &background);
	}
	text_color.r = 255;
	text_color.g = 255;
	text_color.b = 192;
	text_color.a = 255;
	for (i = 0; i < line_count; i++) {
		scrnmng_draw_text_glyphs(background.x + 4 * scale,
		                         background.y + 4 * scale + i * line_height, scale, lines[i],
		                         text_color);
	}
	if (!scrnmng.native_active)
		SDL_SetRenderDrawBlendMode(scrnmng.renderer, SDL_BLENDMODE_NONE);
}

static void scrnmng_draw_framebuffer_info_overlay(const VAEG_VIEWPORT *viewport) {
	char lines[VIDEOVA_FRAMEBUFFERS * 3][48];
	int g0_bpp;
	int g1_bpp;
	int line_count;
	int scale;
	int line_height;
	int width;
	int height;
	int output_width;
	int output_height;
	int y_offset;
	int i;
	SDL_Rect background;
	SDL_Color text_color;

	if ((viewport == NULL) || (!viewport->valid) ||
	    ((np2oscfg.DISPCLK & VAEG_DISPINFO_FRAMEBUFFER) == 0)) {
		return;
	}
	if ((scrnmng_get_drawable_size(&output_width, &output_height) != SUCCESS) ||
	    (output_width <= 0) || (output_height <= 0)) {
		return;
	}
	g0_bpp = scrnmng_video_bpp(videova.grres);
	g1_bpp = scrnmng_video_bpp(videova.grres >> 8);
	line_count = 0;
	for (i = 0; i < VIDEOVA_FRAMEBUFFERS; i++) {
		int bpp = (i & 1) ? g1_bpp : g0_bpp;
		line_count +=
		    scrnmng_format_framebuffer_lines(&lines[line_count], i, bpp, &videova.framebuffer[i]);
	}
	scale = (int)viewport->scale_x;
	if (scale < 1) {
		scale = 1;
	}
	if (scale > 3) {
		scale = 3;
	}
	line_height = 8 * scale;
	width = 0;
	for (i = 0; i < line_count; i++) {
		const int length = (int)strlen(lines[i]);
		if (length > width) {
			width = length;
		}
	}
	width = width * 8 * scale + 8 * scale;
	height = line_height * line_count + 8 * scale;
	y_offset = scrnmng_menu_offset() + 2;
	if ((np2oscfg.DISPCLK & VAEG_DISPINFO_VIDEO) != 0) {
		y_offset += 4 * line_height + 8 * scale + 4;
	}
	if ((width >= output_width) || (y_offset + height >= output_height)) {
		return;
	}
	background.x = output_width - width;
	background.y = y_offset;
	background.w = width;
	background.h = height;
	if (scrnmng.native_active) {
		gui_overlay_rect(background.x, background.y, background.w, background.h, 0, 0, 0, 190);
	} else {
		SDL_SetRenderDrawBlendMode(scrnmng.renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(scrnmng.renderer, 0, 0, 0, 190);
		SDL_RenderFillRect(scrnmng.renderer, &background);
	}
	text_color.r = 255;
	text_color.g = 255;
	text_color.b = 192;
	text_color.a = 255;
	for (i = 0; i < line_count; i++) {
		scrnmng_draw_text_glyphs(background.x + 4 * scale,
		                         background.y + 4 * scale + i * line_height, scale, lines[i],
		                         text_color);
	}
	if (!scrnmng.native_active)
		SDL_SetRenderDrawBlendMode(scrnmng.renderer, SDL_BLENDMODE_NONE);
}

void scrnmng_draw_native_overlays(void) {
	VAEG_VIEWPORT viewport;
	if (scrnmng.native_active && scrnmng_calculate_viewport(&viewport) == SUCCESS) {
		scrnmng_draw_video_info_overlay(&viewport);
		scrnmng_draw_framebuffer_info_overlay(&viewport);
	}
}

static void scrnmng_log_renderer(void) {
	SDL_RendererInfo info;
	const char *name;
	const char *kind;

	if ((scrnmng.renderer == NULL) || (SDL_GetRendererInfo(scrnmng.renderer, &info) != 0)) {
		SDL_Log("SDL renderer backend: unknown");
		return;
	}
	name = (info.name) ? info.name : "unknown";
	snprintf(scrnmng.renderer_backend, sizeof(scrnmng.renderer_backend), "%s", name);
	kind = (info.flags & SDL_RENDERER_ACCELERATED) ? "accelerated" : "software";
	SDL_Log("SDL renderer backend: %s (%s)", name, kind);
}

static BOOL scrnmng_upload_shadow(void) {
	if ((scrnmng.texture == NULL) || (scrnmng.shadow == NULL)) {
		return (FAILURE);
	}
	SDL_Rect rect;

	rect.x = 0;
	rect.y = 0;
	rect.w = scrnmng.width;
	rect.h = scrnmng.height;
	if (SDL_UpdateTexture(scrnmng.texture, &rect, scrnmng.shadow + (SCRNMNG_SURFACE_GUARD_LEFT * 2),
	                      scrnmng.shadow_pitch) != 0) {
		fprintf(stderr, "Error: SDL_UpdateTexture: %s\n", SDL_GetError());
		return (FAILURE);
	}
	scrnmng.dirty = TRUE;
	return (SUCCESS);
}

/* Canvas rows for a guest frame of the given height: 400 to 480. */
static int scrnmng_canvas_height(int height) {
	if (height <= SCRNMNG_CANVAS_HEIGHT) {
		return SCRNMNG_CANVAS_HEIGHT;
	}
	if (height > SCRNMNG_CANVAS_MAX_HEIGHT) {
		return SCRNMNG_CANVAS_MAX_HEIGHT;
	}
	return height;
}

static void scrnmng_clear_shadow(void) {
	if (scrnmng.shadow == NULL) {
		return;
	}
	ZeroMemory(scrnmng.shadow, scrnmng.shadow_pitch * SCRNMNG_CANVAS_MAX_HEIGHT);
	(void)scrnmng_upload_shadow();
}

static BOOL scrnmng_calculate_viewport(VAEG_VIEWPORT *viewport) {
	VAEG_VIEWPORT_INPUT input;
	int window_width;
	int window_height;
	int output_width;
	int output_height;

	if (scrnmng.window == NULL) {
		return (FAILURE);
	}
	SDL_GetWindowSize(scrnmng.window, &window_width, &window_height);
	if ((scrnmng_get_drawable_size(&output_width, &output_height) != SUCCESS) ||
	    (window_width <= 0) || (window_height <= 0)) {
		return (FAILURE);
	}
	input.guest_width = scrnmng.width;
	input.guest_height = scrnmng.height;
	input.drawable_width = output_width;
	input.drawable_height = output_height;
	input.menu_inset =
	    (int)(((SINT64)scrnmng.menu_height * output_height + (window_height / 2)) / window_height);
	if (scrnmng.display_mode == VAEG_DISPLAY_WINDOWED) {
		input.scaling = scrnmng.scaling;
	} else {
		switch (scrnmng.fscrnmod & 3) {
		case 0:
			input.scaling = VAEG_SCALING_NATIVE;
			break;
		case 1:
			input.scaling = VAEG_SCALING_FIT_8DOT;
			break;
		case 3:
			input.scaling = VAEG_SCALING_STRETCH;
			break;
		default:
			input.scaling = VAEG_SCALING_FIT;
			break;
		}
	}
	input.aspect = scrnmng.aspect;
	return (vaeg_viewport_calculate(&input, viewport));
}

void scrnmng_log_geometry(const char *reason) {
	int window_w;
	int window_h;
	int drawable_w;
	int drawable_h;
	VAEG_VIEWPORT viewport;

	if (!scrnmng.window) {
		return;
	}
	if (reason == NULL) {
		reason = "unknown";
	}
	window_w = 0;
	window_h = 0;
	drawable_w = 0;
	drawable_h = 0;
	SDL_GetWindowSize(scrnmng.window, &window_w, &window_h);
	(void)scrnmng_get_drawable_size(&drawable_w, &drawable_h);
	ZeroMemory(&viewport, sizeof(viewport));
	(void)scrnmng_calculate_viewport(&viewport);
	fprintf(stderr,
	        "SDL2 geometry [%s]: window=%dx%d drawable=%dx%d "
	        "guest=%d,%d %dx%d scale=%d menu=%d mode=%d effect=%d\n",
	        reason, window_w, window_h, drawable_w, drawable_h, viewport.x, viewport.y,
	        viewport.width, viewport.height, scrnmng.scale, scrnmng.menu_height, scrnmng.scaling,
	        scrnmng.effect);
}

static BOOL scrnmng_get_drawable_size(int *width, int *height) {
	int drawable_width;
	int drawable_height;

	if ((width == NULL) || (height == NULL) || (scrnmng.window == NULL)) {
		return FAILURE;
	}
	drawable_width = 0;
	drawable_height = 0;
	if ((scrnmng.renderer != NULL) &&
	    (SDL_GetRendererOutputSize(scrnmng.renderer, &drawable_width, &drawable_height) == 0)) {
		/* SDL_Renderer reports the physical drawable size for high-DPI windows. */
	} else {
		SDL_GetWindowSizeInPixels(scrnmng.window, &drawable_width, &drawable_height);
	}
	if ((drawable_width <= 0) || (drawable_height <= 0)) {
		return FAILURE;
	}
	*width = drawable_width;
	*height = drawable_height;
	return SUCCESS;
}

BOOL scrnmng_texture_uniform(BOOL *uniform) {
	const BYTE *base;
	BYTE first0;
	BYTE first1;
	int x;
	int y;

	if (uniform == NULL) {
		return (FAILURE);
	}
	*uniform = TRUE;
	if ((!scrnmng.enable) || (scrnmng.shadow == NULL)) {
		return (FAILURE);
	}
	base = scrnmng.shadow + (SCRNMNG_SURFACE_GUARD_LEFT * 2);
	first0 = base[0];
	first1 = base[1];
	for (y = 0; y < scrnmng.height; y++) {
		const BYTE *row;

		row = base + (y * scrnmng.shadow_pitch);
		for (x = 0; x < scrnmng.width; x++) {
			const BYTE *pixel;

			pixel = row + (x * 2);
			if ((pixel[0] != first0) || (pixel[1] != first1)) {
				*uniform = FALSE;
				return (SUCCESS);
			}
		}
	}
	return (SUCCESS);
}

BOOL scrnmng_native_active(void) {
	return scrnmng.native_active;
}

BOOL scrnmng_take_native_fallback(void) {
	const BOOL pending = scrnmng.native_fallback_pending;

	scrnmng.native_fallback_pending = FALSE;
	return pending;
}

void scrnmng_initialize(void) {
	ZeroMemory(&scrnmng, sizeof(scrnmng));
	scrnmng.scale = 1;
	scrnmng.scaling = VAEG_SCALING_FIT;
	scrnmng.effect = VAEG_EFFECT_UNFILTERED;
	scrnmng.display_mode = VAEG_DISPLAY_WINDOWED;
	scrnmng.fscrnmod = 2;
	scrnstat.width = 640;
	scrnstat.height = 400;
}

static BOOL scrnmng_create_sdl_resources(void) {
	scrnmng.renderer = SDL_CreateRenderer(scrnmng.window, -1, SDL_RENDERER_ACCELERATED);
	if (scrnmng.renderer == NULL) {
		scrnmng.renderer = SDL_CreateRenderer(scrnmng.window, -1, SDL_RENDERER_SOFTWARE);
	}
	if (scrnmng.renderer == NULL) {
		fprintf(stderr, "Error: SDL_CreateRenderer: %s\n", SDL_GetError());
		return FAILURE;
	}
	scrnmng_log_renderer();
	SDL_RenderSetLogicalSize(scrnmng.renderer, 0, 0);
	scrnmng.texture =
	    SDL_CreateTexture(scrnmng.renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STATIC,
	                      SCRNMNG_CANVAS_WIDTH, SCRNMNG_CANVAS_MAX_HEIGHT);
	if (scrnmng.texture == NULL) {
		fprintf(stderr, "Error: SDL_CreateTexture: %s\n", SDL_GetError());
		SDL_DestroyRenderer(scrnmng.renderer);
		scrnmng.renderer = NULL;
		return FAILURE;
	}
	SDL_SetTextureScaleMode(scrnmng.texture, (scrnmng.effect == VAEG_EFFECT_UNFILTERED)
	                                             ? SDL_ScaleModeNearest
	                                             : SDL_ScaleModeLinear);
	return SUCCESS;
}

/* A native DXGI swap chain and SDL's selected presentation API must not
 * inherit each other's HWND state. Keep guest pixels/state, replace only
 * the host window when returning from native presentation on Windows. */
static BOOL scrnmng_detach_native_window(void) {
	SDL_Window *old_window = scrnmng.window;
	SDL_Window *new_window;
	SDL_DisplayMode mode = {0};
	Uint32 flags = SDL_GetWindowFlags(old_window);
	int x, y, width, height;
	SDL_GetWindowPosition(old_window, &x, &y);
	SDL_GetWindowSize(old_window, &width, &height);
	SDL_GetWindowDisplayMode(old_window, &mode);
	new_window = SDL_CreateWindow(app_name, x, y, width, height,
	    SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI |
	    (flags & (SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP)));
	if (!new_window) {
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Renderer switch: new window failed: %s", SDL_GetError());
		return FAILURE;
	}
	SDL_CaptureMouse(SDL_FALSE);
	SDL_DestroyWindow(old_window);
	scrnmng.window = new_window;
	appicon_set_window(new_window);
	SDL_SetWindowMinimumSize(new_window, 320, 240);
	if (flags & SDL_WINDOW_FULLSCREEN) {
		if (SDL_SetWindowDisplayMode(new_window, &mode) != 0 ||
		    SDL_SetWindowFullscreen(new_window, flags & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0) {
			SDL_LogError(SDL_LOG_CATEGORY_VIDEO, "Renderer switch: fullscreen restore failed: %s", SDL_GetError());
			return FAILURE;
		}
	} else if (flags & SDL_WINDOW_MAXIMIZED) {
		SDL_MaximizeWindow(new_window);
	}
	if (!(flags & SDL_WINDOW_HIDDEN)) {
		SDL_ShowWindow(new_window);
		SDL_RaiseWindow(new_window);
	}
	SDL_Log("Renderer switch: native window detached; new SDL window=%u", SDL_GetWindowID(new_window));
	return SUCCESS;
}

static BOOL scrnmng_native_fallback(void) {
	if (!scrnmng.native_active || (scrnmng.native_presenter == NULL)) {
		return FAILURE;
	}
	snprintf(scrnmng.native_status, sizeof(scrnmng.native_status), "CRT fallback: %s",
	         vaeg_native_presenter_error(scrnmng.native_presenter));
	gui_shutdown();
	vaeg_native_presenter_destroy(scrnmng.native_presenter);
	scrnmng.native_presenter = NULL;
	scrnmng.native_active = FALSE;
	scrnmng.native_fallback_pending = TRUE;
#if defined(_WIN32)
	if (scrnmng_detach_native_window() != SUCCESS) return FAILURE;
#endif
	if (scrnmng_create_sdl_resources() != SUCCESS) {
		fprintf(stderr, "Error: SDL fallback renderer creation failed: %s\n", SDL_GetError());
		return FAILURE;
	}
	(void)scrnmng_upload_shadow();
	scrnmng_update_title();
	fprintf(stderr, "Native CRT disabled after failure; SDL presentation restored\n");
	return SUCCESS;
}

BOOL scrnmng_window_rebind_selftest(void) {
	SDL_Window *saved_window = scrnmng.window;
	const BOOL video_was_active = SDL_WasInit(SDL_INIT_VIDEO) != 0;
	BOOL result = SUCCESS;
	int i;
	if (!video_was_active && SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) return FAILURE;
	scrnmng.window = SDL_CreateWindow("renderer rebind test", 0, 0, 640, 400, SDL_WINDOW_HIDDEN);
	if (!scrnmng.window) result = FAILURE;
	for (i = 0; i < 10 && result == SUCCESS; ++i) {
		const Uint32 old_id = SDL_GetWindowID(scrnmng.window);
		int width, height;
		result = scrnmng_detach_native_window();
		if (result != SUCCESS) break;
		SDL_GetWindowSize(scrnmng.window, &width, &height);
		if (SDL_GetWindowID(scrnmng.window) == old_id || width != 640 || height != 400 ||
		    !(SDL_GetWindowFlags(scrnmng.window) & SDL_WINDOW_HIDDEN)) result = FAILURE;
		if (result == SUCCESS) {
			SDL_Renderer *renderer = SDL_CreateRenderer(scrnmng.window, -1, SDL_RENDERER_SOFTWARE);
			SDL_Rect pixel = {0, 0, 1, 1};
			BYTE rgba[4] = {0};
			if (!renderer) { result = FAILURE; break; }
			SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
			if (SDL_RenderClear(renderer) != 0 ||
			    SDL_RenderReadPixels(renderer, &pixel, SDL_PIXELFORMAT_RGBA32, rgba, 4) != 0 ||
			    rgba[0] != 255 || rgba[1] != 0 || rgba[2] != 0) result = FAILURE;
			SDL_RenderPresent(renderer);
			SDL_DestroyRenderer(renderer);
		}
	}
	if (scrnmng.window) SDL_DestroyWindow(scrnmng.window);
	scrnmng.window = saved_window;
	if (!video_was_active) SDL_QuitSubSystem(SDL_INIT_VIDEO);
	if (result != SUCCESS) fprintf(stderr, "selftest: WINDOW_REBIND_FAILED: %s\n", SDL_GetError());
	return result;
}

BOOL scrnmng_create(int width, int height) {
	const char *preset_path;

	width = max(320, width);
	height = max(240, height);
	if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
		fprintf(stderr, "Error: SDL video init: %s\n", SDL_GetError());
		return (FAILURE);
	}
	scrnmng.window =
	    SDL_CreateWindow(app_name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
	                     SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	if (scrnmng.window == NULL) {
		fprintf(stderr, "Error: SDL_CreateWindow: %s\n", SDL_GetError());
		return (FAILURE);
	}
	SDL_EventState(SDL_DROPBEGIN, SDL_ENABLE);
	SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
	SDL_EventState(SDL_DROPCOMPLETE, SDL_ENABLE);
	appicon_set_window(scrnmng.window);
	SDL_SetWindowMinimumSize(scrnmng.window, 320, 240);
	SDL_GetWindowPosition(scrnmng.window, &scrnmng.window_x, &scrnmng.window_y);
	scrnmng.window_width = width;
	scrnmng.window_height = height;
	scrnmng.width = SCRNMNG_CANVAS_WIDTH;
	scrnmng.height = scrnmng_canvas_height(scrnstat.height);
	scrnmng.shadow_pitch = (SCRNMNG_CANVAS_WIDTH + SCRNMNG_SURFACE_GUARD_LEFT) * 2;
	scrnmng.shadow = (BYTE *)calloc(SCRNMNG_CANVAS_MAX_HEIGHT, scrnmng.shadow_pitch);
	{
		const char *rendered_path;

		rendered_path = getenv("VAEG_SCREEN_DUMP");
		scrnmng.rendered_capture_enabled = (rendered_path != NULL) && (rendered_path[0] != '\0');
	}
	if (scrnmng.shadow == NULL) {
		fprintf(stderr, "Error: shadow framebuffer allocation failed\n");
		scrnmng_destroy();
		return (FAILURE);
	}
	scrnmng.enable = TRUE;
	snprintf(scrnmng.native_status, sizeof(scrnmng.native_status), "SDL");
	if (scrnmng_native_requested()) {
		preset_path = scrnmng_native_preset_path();
		scrnmng.native_presenter = vaeg_native_presenter_create(scrnmng.window, 0, 0, preset_path,
		                                                        scrnmng_native_parameter_state);
		if (scrnmng.native_presenter != NULL) {
			scrnmng.native_active = TRUE;
			if (!np2oscfg.gui_native_filter &&
			    !vaeg_native_presenter_set_filter(scrnmng.native_presenter, 0)) {
				vaeg_native_presenter_destroy(scrnmng.native_presenter);
				scrnmng.native_presenter = NULL;
				scrnmng.native_active = FALSE;
				snprintf(scrnmng.native_status, sizeof(scrnmng.native_status),
				         "SDL fallback: native unfiltered mode unavailable");
			}
		}
		if (scrnmng.native_active) {
			scrnmng.renderer_backend[0] = '\0';
			fprintf(stderr, "Native librashader selected: backend=%s mode=%s preset=%s\n",
			        vaeg_native_presenter_backend(scrnmng.native_presenter),
			        np2oscfg.gui_native_filter ? "CRT" : "unfiltered", preset_path);
		} else {
			if (scrnmng.native_status[0] == '\0' ||
			    strncmp(scrnmng.native_status, "SDL fallback:", 13) != 0) {
				snprintf(scrnmng.native_status, sizeof(scrnmng.native_status),
				         "SDL fallback: %s", vaeg_native_presenter_creation_error());
			}
			fprintf(stderr, "Native librashader selected but unavailable; using SDL fallback\n");
		}
	}
	if (scrnmng.native_active) {
		scrnmng_apply_native_mask_profile();
	}
	if (!scrnmng.native_active && (scrnmng_create_sdl_resources() != SUCCESS)) {
		scrnmng_destroy();
		return (FAILURE);
	}
	scrnmng_clear_shadow();
	scrnmng_update_title();
	return (SUCCESS);
}

void scrnmng_show(void) {
	if ((scrnmng.window) && (!scrnmng.visible)) {
		SDL_ShowWindow(scrnmng.window);
		scrnmng.visible = TRUE;
		scrnmng_log_geometry("startup");
	}
}

void scrnmng_destroy(void) {
	scrnmng.enable = FALSE;
	scrnmng.visible = FALSE;
	if (scrnmng.native_presenter != NULL) {
		vaeg_native_presenter_destroy(scrnmng.native_presenter);
		scrnmng.native_presenter = NULL;
	}
	scrnmng.native_active = FALSE;
	if (scrnmng.texture) {
		SDL_DestroyTexture(scrnmng.texture);
		scrnmng.texture = NULL;
	}
	if (scrnmng.shadow) {
		free(scrnmng.shadow);
		scrnmng.shadow = NULL;
	}
	scrnmng.shadow_pitch = 0;
	if (scrnmng.rendered_frame != NULL) {
		SDL_FreeSurface(scrnmng.rendered_frame);
		scrnmng.rendered_frame = NULL;
	}
	if (scrnmng.renderer) {
		SDL_DestroyRenderer(scrnmng.renderer);
		scrnmng.renderer = NULL;
	}
	if (scrnmng.window) {
		SDL_DestroyWindow(scrnmng.window);
		scrnmng.window = NULL;
	}
	SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_TIMER);
}

void *scrnmng_get_window(void) {
	return (scrnmng.window);
}

BOOL scrnmng_get_output_size(int *width, int *height) {
	return scrnmng_get_drawable_size(width, height);
}

BOOL scrnmng_present_startup_image(const void *pixels, int width, int height, int pitch,
                                    int x, int y, int output_width, int output_height) {
	VAEG_FRAME_INPUT frame;
	VAEG_VIEWPORT viewport;
	int drawable_width, drawable_height;
	BOOL filtered;
	VAEG_NATIVE_PRESENTER_RESULT result;
	if (!scrnmng.native_active || !pixels || width <= 0 || height <= 0 || pitch <= 0 ||
	    scrnmng_get_drawable_size(&drawable_width, &drawable_height) != SUCCESS) return FAILURE;
	if (vaeg_native_presenter_resize(scrnmng.native_presenter, drawable_width, drawable_height)
	    != VAEG_NATIVE_PRESENTER_PRESENTED) return FAILURE;
	filtered = strcmp(vaeg_native_presenter_state(scrnmng.native_presenter), "filtered") == 0;
	if (!vaeg_native_presenter_set_filter(scrnmng.native_presenter, 0)) return FAILURE;
	vaeg_native_presenter_set_output_viewport(scrnmng.native_presenter, x, y, output_width, output_height);
	vaeg_frame_input_initialize(&frame, pixels, width, height, pitch, VAEG_FRAME_PIXEL_ARGB8888,
	    VAEG_FRAME_ROWS_TOP_DOWN, width, height, 60, 1, 0, 16666667U);
	result = vaeg_native_presenter_present(scrnmng.native_presenter, &frame);
	/* The startup image is not guest VRAM and must not seed CRT history. */
	if (filtered) (void)vaeg_native_presenter_set_filter(scrnmng.native_presenter, 1);
	if (scrnmng_calculate_viewport(&viewport) == SUCCESS)
		vaeg_native_presenter_set_output_viewport(scrnmng.native_presenter, viewport.x,
		    viewport.y, viewport.width, viewport.height);
	return result == VAEG_NATIVE_PRESENTER_PRESENTED ? SUCCESS : FAILURE;
}

void *scrnmng_get_renderer(void) {
	return (scrnmng.renderer);
}

BOOL scrnmng_native_gui_prepare(void) {
	return vaeg_native_presenter_gui_prepare(scrnmng.native_presenter) ? SUCCESS : FAILURE;
}

BOOL scrnmng_fallback_to_sdl(void) {
	return scrnmng_native_fallback();
}

void scrnmng_native_gui_shutdown(void) {
	vaeg_native_presenter_gui_shutdown(scrnmng.native_presenter);
}

const char *scrnmng_native_status(void) {
	if (scrnmng.native_active) {
		const char *error = vaeg_native_presenter_error(scrnmng.native_presenter);
		if (strcmp(error, "none") != 0) {
			snprintf(scrnmng.native_status, sizeof(scrnmng.native_status),
			         "CRT unavailable: %s / native unfiltered", error);
			return scrnmng.native_status;
		}
		return strcmp(vaeg_native_presenter_state(scrnmng.native_presenter), "filtered") == 0
		           ? "Native CRT ON"
		           : "Native librashader / unfiltered";
	}
	return scrnmng.native_status;
}

BOOL scrnmng_native_set_parameter(const char *name, float value) {
	return vaeg_native_presenter_set_parameter(scrnmng.native_presenter, name, value) ? SUCCESS
	                                                                                  : FAILURE;
}

void scrnmng_request_native_crt(BOOL enabled, BOOL reload) {
	np2oscfg.gui_native_crt = enabled ? 1 : 0;
	scrnmng.native_change_pending = TRUE;
	scrnmng.native_reload_pending = reload;
}

BOOL scrnmng_native_filter_enabled(void) {
	return scrnmng.native_active &&
	       strcmp(vaeg_native_presenter_state(scrnmng.native_presenter), "filtered") == 0;
}

BOOL scrnmng_set_native_filter(BOOL enabled) {
	// Presentation-thread operation: keep the device, GUI and preset intact.
	if (!scrnmng.native_active ||
	    !vaeg_native_presenter_set_filter(scrnmng.native_presenter, enabled)) {
		return FAILURE;
	}
	if (enabled) {
		scrnmng_apply_native_mask_profile();
	}
	scrnmng_update_title();
	return SUCCESS;
}

float scrnmng_native_mask_intensity(void) {
	if (scrnmng.scale <= 1) {
		return 0.15f;
	}
	if (scrnmng.scale == 2) {
		return 0.20f;
	}
	return 0.30f;
}

void scrnmng_apply_native_mask_profile(void) {
	if (!np2oscfg.gui_native_mask_auto || !scrnmng.native_active ||
	    !scrnmng_native_filter_enabled()) {
		return;
	}
	(void)scrnmng_native_set_parameter("MASK_INTENSITY", scrnmng_native_mask_intensity());
}

BOOL scrnmng_apply_native_crt_request(void) {
#if defined(_WIN32) && defined(VAEG_ENABLE_LIBRASHADER)
	const char *preset;
	BOOL was_native;
	if (!scrnmng.native_change_pending)
		return SUCCESS;
	scrnmng.native_change_pending = FALSE;
	if (scrnmng.native_active && np2oscfg.gui_native_crt && !scrnmng.native_reload_pending) {
		if (!vaeg_native_presenter_set_filter(scrnmng.native_presenter,
		                                      np2oscfg.gui_native_filter)) {
			fprintf(stderr, "Native CRT toggle failed\n");
		}
		if (np2oscfg.gui_native_filter) {
			scrnmng_apply_native_mask_profile();
		}
		scrnmng_update_title();
		return SUCCESS;
	}
	if (!scrnmng.native_active && !np2oscfg.gui_native_crt)
		return SUCCESS;
	// Called between ImGui frames on the presentation thread.
	was_native = scrnmng.native_active;
	SDL_Log("Renderer switch: releasing GUI");
	gui_shutdown();
	SDL_Log("Renderer switch: releasing native presenter");
	vaeg_native_presenter_destroy(scrnmng.native_presenter);
	scrnmng.native_presenter = NULL;
	scrnmng.native_active = FALSE;
	if (scrnmng.texture)
		SDL_DestroyTexture(scrnmng.texture);
	scrnmng.texture = NULL;
	if (scrnmng.renderer)
		SDL_DestroyRenderer(scrnmng.renderer);
	scrnmng.renderer = NULL;
	preset = scrnmng_native_preset_path();
	if (np2oscfg.gui_native_crt) {
		scrnmng.native_presenter =
		    vaeg_native_presenter_create(scrnmng.window, 0, 0, preset, scrnmng_native_parameter_state);
	}
	scrnmng.native_active = scrnmng.native_presenter != NULL;
	if (scrnmng.native_active) {
		scrnmng_apply_native_mask_profile();
	}
	if (!scrnmng.native_active) {
		if (was_native && scrnmng_detach_native_window() != SUCCESS) return FAILURE;
		SDL_Log("Renderer switch: creating SDL renderer");
		if (np2oscfg.gui_native_crt) {
			snprintf(scrnmng.native_status, sizeof(scrnmng.native_status),
			         "SDL fallback: %s", vaeg_native_presenter_creation_error());
		} else {
			snprintf(scrnmng.native_status, sizeof(scrnmng.native_status), "SDL");
		}
		if (scrnmng_create_sdl_resources() != SUCCESS)
			return FAILURE;
		(void)scrnmng_upload_shadow();
	}
	scrnmng_update_title();
	SDL_Log("Renderer switch: initializing GUI");
	if (gui_initialize(scrnmng.window, scrnmng.renderer, NULL) == SUCCESS) {
		SDL_Log("Renderer switch complete: %s", scrnmng_get_renderer_backend());
		return SUCCESS;
	}
	if (!scrnmng.native_active || scrnmng_native_fallback() != SUCCESS) return FAILURE;
	(void)scrnmng_take_native_fallback();
	return gui_initialize(scrnmng.window, scrnmng.renderer, NULL);
#else
	scrnmng.native_change_pending = FALSE;
	return SUCCESS;
#endif
}

const char *scrnmng_get_renderer_backend(void) {
	if (scrnmng.native_active && (scrnmng.native_presenter != NULL)) {
		return vaeg_native_presenter_backend(scrnmng.native_presenter);
	}
	if (scrnmng.renderer_backend[0] == '\0') {
		return ("unknown");
	}
	return (scrnmng.renderer_backend);
}

static BOOL scrnmng_update_window_size(void) {
	if (scrnmng.window && (scrnmng.display_mode == VAEG_DISPLAY_WINDOWED)) {
		int current_w;
		int current_h;
		int target_w;
		int target_h;

		current_w = 0;
		current_h = 0;
		target_w = scrnmng.width * scrnmng.scale;
		target_h = scrnmng.menu_height + (scrnmng.height * scrnmng.scale);
		SDL_GetWindowSize(scrnmng.window, &current_w, &current_h);
		if ((current_w == target_w) && (current_h == target_h)) {
			return (FALSE);
		}
		SDL_SetWindowSize(scrnmng.window, target_w, target_h);
		return (TRUE);
	}
	return (FALSE);
}

void scrnmng_set_menu_height(int height) {
	if (height < 0) {
		height = 0;
	}
	if (scrnmng.menu_height == height) {
		return;
	}
	scrnmng.menu_height = height;
	if (scrnmng.visible) {
		scrnmng_log_geometry("menu-height");
	}
}

void scrnmng_set_display(int scale, BOOL aspect) {
	if (scale < 1) {
		scale = 1;
	} else if (scale > VAEG_WINDOW_SCALE_MAX) {
		scale = VAEG_WINDOW_SCALE_MAX;
	}
	scrnmng.scale = scale;
	scrnmng.aspect = aspect ? TRUE : FALSE;
	scrnmng_update_window_size();
	scrnmng_apply_native_mask_profile();
	if (scrnmng.visible) {
		scrnmng_log_geometry("scale-change");
	}
}

int scrnmng_get_display_scale(void) {
	return (scrnmng.scale);
}

BOOL scrnmng_get_display_aspect(void) {
	return (scrnmng.aspect);
}

void scrnmng_set_scaling(int scaling) {
	if ((scaling < 0) || (scaling >= VAEG_SCALING_COUNT)) {
		scaling = VAEG_SCALING_FIT;
	}
	scrnmng.scaling = scaling;
	if (scrnmng.visible) {
		scrnmng_log_geometry("scaling-change");
	}
}

int scrnmng_get_scaling(void) {
	return (scrnmng.scaling);
}

void scrnmng_set_effect(int effect) {
	if ((effect < 0) || (effect >= VAEG_EFFECT_COUNT)) {
		effect = VAEG_EFFECT_UNFILTERED;
	}
	scrnmng.effect = effect;
	if (scrnmng.texture != NULL) {
		SDL_SetTextureScaleMode(scrnmng.texture, (effect == VAEG_EFFECT_UNFILTERED)
		                                             ? SDL_ScaleModeNearest
		                                             : SDL_ScaleModeLinear);
	}
}

int scrnmng_get_effect(void) {
	return (scrnmng.effect);
}

BOOL scrnmng_get_viewport(VAEG_VIEWPORT *viewport) {
	if (viewport == NULL) {
		return (FAILURE);
	}
	return (scrnmng_calculate_viewport(viewport));
}

BOOL scrnmng_map_window_point(int window_x, int window_y, int *guest_x, int *guest_y) {
	VAEG_VIEWPORT viewport;
	int window_width;
	int window_height;
	int drawable_x;
	int drawable_y;
	int output_width;
	int output_height;

	if (scrnmng_calculate_viewport(&viewport) != SUCCESS) {
		return (FAILURE);
	}
	SDL_GetWindowSize(scrnmng.window, &window_width, &window_height);
	if (scrnmng_get_drawable_size(&output_width, &output_height) != SUCCESS) {
		return (FAILURE);
	}
	if ((window_width <= 0) || (window_height <= 0)) {
		return (FAILURE);
	}
	drawable_x = (int)(((SINT64)window_x * output_width) / window_width);
	drawable_y = (int)(((SINT64)window_y * output_height) / window_height);
	return (vaeg_viewport_map_point(&viewport, scrnmng.width, scrnmng.height, drawable_x,
	                                drawable_y, guest_x, guest_y));
}

static void scrnmng_remember_window(void) {
	if ((scrnmng.window == NULL) || (scrnmng.display_mode != VAEG_DISPLAY_WINDOWED)) {
		return;
	}
	SDL_GetWindowPosition(scrnmng.window, &scrnmng.window_x, &scrnmng.window_y);
	SDL_GetWindowSize(scrnmng.window, &scrnmng.window_width, &scrnmng.window_height);
	scrnmng.window_maximized =
	    (SDL_GetWindowFlags(scrnmng.window) & SDL_WINDOW_MAXIMIZED) ? TRUE : FALSE;
}

static void scrnmng_restore_window(void) {
	SDL_RestoreWindow(scrnmng.window);
	SDL_SetWindowPosition(scrnmng.window, scrnmng.window_x, scrnmng.window_y);
	SDL_SetWindowSize(scrnmng.window, max(320, scrnmng.window_width),
	                  max(240, scrnmng.window_height));
	if (scrnmng.window_maximized) {
		SDL_MaximizeWindow(scrnmng.window);
	}
}

static BOOL scrnmng_find_display_mode(int monitor, int width, int height, int refresh,
                                      SDL_DisplayMode *result) {
	int count;
	int index;

	count = SDL_GetNumDisplayModes(monitor);
	for (index = 0; index < count; index++) {
		SDL_DisplayMode mode;

		if ((SDL_GetDisplayMode(monitor, index, &mode) == 0) && (mode.w == width) &&
		    (mode.h == height) && ((refresh == 0) || (mode.refresh_rate == refresh))) {
			*result = mode;
			return (SUCCESS);
		}
	}
	SDL_SetError("Requested display mode is not available");
	return (FAILURE);
}

BOOL scrnmng_set_display_mode(int mode, int monitor, UINT width, UINT height, UINT refresh,
                              UINT8 fscrnmod) {
	SDL_Rect bounds;
	SDL_DisplayMode desktop;
	SDL_DisplayMode selected;
	int target_width;
	int target_height;
	int display_count;

	if ((scrnmng.window == NULL) || (mode < 0) || (mode >= VAEG_DISPLAY_MODE_COUNT)) {
		return (FAILURE);
	}
	display_count = SDL_GetNumVideoDisplays();
	if (display_count <= 0) {
		return (mode == VAEG_DISPLAY_WINDOWED ? SUCCESS : FAILURE);
	}
	if ((monitor < 0) || (monitor >= display_count)) {
		monitor = 0;
	}
	if ((SDL_GetDisplayBounds(monitor, &bounds) != 0) ||
	    (SDL_GetDesktopDisplayMode(monitor, &desktop) != 0)) {
		return (FAILURE);
	}
	if (scrnmng.display_mode == VAEG_DISPLAY_WINDOWED) {
		scrnmng_remember_window();
	}
	if (SDL_SetWindowFullscreen(scrnmng.window, 0) != 0) {
		return (FAILURE);
	}
	scrnmng.fscrnmod = vaeg_fscrnmod_sanitize(fscrnmod, NULL);
	if (mode == VAEG_DISPLAY_WINDOWED) {
		scrnmng.display_mode = mode;
		scrnmng_restore_window();
		scrnmng_log_geometry("windowed");
		return (SUCCESS);
	}
	SDL_RestoreWindow(scrnmng.window);
	SDL_SetWindowPosition(scrnmng.window, bounds.x, bounds.y);
	if (mode == VAEG_DISPLAY_BORDERLESS) {
		if (SDL_SetWindowFullscreen(scrnmng.window, SDL_WINDOW_FULLSCREEN_DESKTOP) == 0) {
			scrnmng.display_mode = mode;
			scrnmng_log_geometry("borderless");
			return (SUCCESS);
		}
	} else {
		vaeg_fullscreen_size(width, height, scrnmng.fscrnmod, desktop.w, desktop.h, &target_width,
		                     &target_height);
		if ((scrnmng_find_display_mode(monitor, target_width, target_height, refresh, &selected) ==
		     SUCCESS) &&
		    (SDL_SetWindowDisplayMode(scrnmng.window, &selected) == 0) &&
		    (SDL_SetWindowFullscreen(scrnmng.window, SDL_WINDOW_FULLSCREEN) == 0)) {
			scrnmng.display_mode = mode;
			scrnmng_log_geometry("exclusive");
			return (SUCCESS);
		}
	}
	(void)SDL_SetWindowFullscreen(scrnmng.window, 0);
	scrnmng.display_mode = VAEG_DISPLAY_WINDOWED;
	scrnmng_restore_window();
	return (FAILURE);
}

int scrnmng_get_display_mode(void) {
	return (scrnmng.display_mode);
}

BOOL scrnmng_isfullscreen(void) {
	return (scrnmng.display_mode != VAEG_DISPLAY_WINDOWED);
}

BOOL scrnmng_capture_window_size(int *width, int *height) {
	if ((scrnmng.window == NULL) || (scrnmng.display_mode != VAEG_DISPLAY_WINDOWED) ||
	    (width == NULL) || (height == NULL)) {
		return (FAILURE);
	}
	SDL_GetWindowSize(scrnmng.window, width, height);
	return (SUCCESS);
}

RGB16 scrnmng_makepal16(RGB32 pal32) {
	RGB16 ret;

	ret = (pal32.p.r & 0xf8) << 8;
#if defined(SIZE_QVGA)
	ret += (pal32.p.g & 0xfc) << (3 + 16);
#else
	ret += (pal32.p.g & 0xfc) << 3;
#endif
	ret += pal32.p.b >> 3;
	return (ret);
}

void scrnmng_setwidth(int posx, int width) {
	if (width < 1) {
		width = 1;
	}
	if (scrnstat.width == width) {
		return;
	}
	scrnstat.width = width;
	scrnmng_clear_shadow();
	if (scrnmng.visible) {
		scrnmng_log_geometry("mode-width");
	}
	(void)posx;
}

void scrnmng_setheight(int posy, int height) {
	if (height < 1) {
		height = 1;
	}
	if (scrnstat.height == height) {
		return;
	}
	scrnstat.height = height;
	if (scrnmng.shadow != NULL) {
		scrnmng.height = scrnmng_canvas_height(height);
	}
	scrnmng_clear_shadow();
	if (scrnmng.visible) {
		scrnmng_log_geometry("mode-height");
	}
	(void)posy;
}

const SCRNSURF *scrnmng_surflock(void) {
	if ((!scrnmng.enable) || (scrnmng.shadow == NULL)) {
		return (NULL);
	}
	scrnsurf.ptr = scrnmng.shadow;
	scrnsurf.xalign = 2;
	scrnsurf.yalign = scrnmng.shadow_pitch;
	scrnsurf.bpp = 16;
	scrnsurf.width = min(scrnstat.width, scrnmng.width);
	scrnsurf.height = min(scrnstat.height, scrnmng.height);
	scrnsurf.extend = 0;
	return (&scrnsurf);
}

void scrnmng_surfunlock(const SCRNSURF *surf) {
	if ((surf == NULL) || (scrnmng.texture == NULL)) {
		return;
	}
	(void)scrnmng_upload_shadow();
}

void scrnmng_present_begin(void) {
	VAEG_VIEWPORT viewport;
	SDL_Rect dst;
	int row;
	int x;
	scrnmng.sdl_overlays_drawn = FALSE;

	if (!scrnmng.enable) {
		return;
	}
	if (scrnmng.native_active) {
		scrnmng.dirty = FALSE;
		return;
	}
	if ((scrnmng.renderer == NULL) || (scrnmng.texture == NULL)) {
		return;
	}
	SDL_SetRenderDrawColor(scrnmng.renderer, 0, 0, 0, 255);
	SDL_RenderClear(scrnmng.renderer);
	if (scrnmng_calculate_viewport(&viewport) != SUCCESS) {
		return;
	}
	dst.x = viewport.x;
	dst.y = viewport.y;
	dst.w = viewport.width;
	dst.h = viewport.height;
	{
		SDL_Rect src;

		src.x = 0;
		src.y = 0;
		src.w = scrnmng.width;
		src.h = scrnmng.height;
		SDL_RenderCopy(scrnmng.renderer, scrnmng.texture, &src, &dst);
	}
	SDL_RenderSetClipRect(scrnmng.renderer, &dst);
	if (((scrnmng.effect == VAEG_EFFECT_SCANLINE) || (scrnmng.effect == VAEG_EFFECT_CRT_LITE)) &&
	    (dst.h >= scrnmng.height)) {
		SDL_SetRenderDrawBlendMode(scrnmng.renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(scrnmng.renderer, 0, 0, 0,
		                       (scrnmng.effect == VAEG_EFFECT_CRT_LITE) ? 42 : 34);
		for (row = 1; row < scrnmng.height; row += 2) {
			int y0;
			int y1;
			SDL_Rect line;

			y0 = dst.y + (row * dst.h) / scrnmng.height;
			y1 = dst.y + ((row + 1) * dst.h) / scrnmng.height;
			if (y1 <= y0) {
				y1 = y0 + 1;
			}
			line.x = dst.x;
			line.y = y0;
			line.w = dst.w;
			line.h = y1 - y0;
			SDL_RenderFillRect(scrnmng.renderer, &line);
		}
	}
	if (scrnmng.effect == VAEG_EFFECT_CRT_LITE) {
		if (dst.w >= (scrnmng.width * 2)) {
			SDL_SetRenderDrawBlendMode(scrnmng.renderer, SDL_BLENDMODE_ADD);
			for (x = dst.x; x < (dst.x + dst.w); x += 3) {
				SDL_SetRenderDrawColor(scrnmng.renderer, 24, 2, 2, 10);
				SDL_RenderDrawLine(scrnmng.renderer, x, dst.y, x, dst.y + dst.h - 1);
				if ((x + 1) < (dst.x + dst.w)) {
					SDL_SetRenderDrawColor(scrnmng.renderer, 2, 24, 2, 10);
					SDL_RenderDrawLine(scrnmng.renderer, x + 1, dst.y, x + 1, dst.y + dst.h - 1);
				}
				if ((x + 2) < (dst.x + dst.w)) {
					SDL_SetRenderDrawColor(scrnmng.renderer, 2, 2, 24, 10);
					SDL_RenderDrawLine(scrnmng.renderer, x + 2, dst.y, x + 2, dst.y + dst.h - 1);
				}
			}
		}
		SDL_SetRenderDrawBlendMode(scrnmng.renderer, SDL_BLENDMODE_BLEND);
		for (x = 0; x < 8; x++) {
			SDL_Rect left = {dst.x + x, dst.y, 1, dst.h};
			SDL_Rect right = {dst.x + dst.w - 1 - x, dst.y, 1, dst.h};
			SDL_Rect top = {dst.x, dst.y + x, dst.w, 1};
			SDL_Rect bottom = {dst.x, dst.y + dst.h - 1 - x, dst.w, 1};
			SDL_SetRenderDrawColor(scrnmng.renderer, 0, 0, 0, (UINT8)(38 - x * 4));
			SDL_RenderFillRect(scrnmng.renderer, &left);
			SDL_RenderFillRect(scrnmng.renderer, &right);
			SDL_RenderFillRect(scrnmng.renderer, &top);
			SDL_RenderFillRect(scrnmng.renderer, &bottom);
		}
	}
	SDL_RenderSetClipRect(scrnmng.renderer, NULL);
	scrnmng.dirty = FALSE;
}

BOOL scrnmng_request_display_capture(const char *path) {
	if (!path || !path[0] || strlen(path) >= sizeof(scrnmng.display_capture_path) ||
	    scrnmng.display_capture_path[0]) {
		SDL_SetError("Invalid or already pending screenshot request");
		return FAILURE;
	}
	milstr_ncpy(scrnmng.display_capture_path, path, sizeof(scrnmng.display_capture_path));
	return SUCCESS;
}

BOOL scrnmng_prepare_display_capture(void) {
	scrnmng.display_capture_ready = scrnmng.display_capture_path[0] != '\0';
	return scrnmng.display_capture_ready;
}

static SDL_Surface *scrnmng_crop_display_capture(SDL_Surface *surface) {
	int window_width, window_height;
	if (surface) {
		SDL_GetWindowSize(scrnmng.window, &window_width, &window_height);
		const int inset = window_height > 0 ?
		    (int)(((SINT64)scrnmng.menu_height * surface->h + window_height / 2) / window_height) : 0;
		if (inset >= 0 && inset < surface->h) {
			return SDL_CreateRGBSurfaceWithFormatFrom(
			    (UINT8 *)surface->pixels + inset * surface->pitch, surface->w,
			    surface->h - inset, 32, surface->pitch, SDL_PIXELFORMAT_RGBA32);
		}
	}
	return NULL;
}

static void scrnmng_finish_display_capture(SDL_Surface *surface, BOOL captured) {
	BOOL saved = FAILURE;
	SDL_Surface *cropped = NULL;
	if (captured == SUCCESS && surface) {
		cropped = scrnmng_crop_display_capture(surface);
		if (cropped) saved = scrnmng_png_save_surface(cropped, scrnmng.display_capture_path);
	} else {
		SDL_SetError("Display readback failed or is unsupported by this native backend");
	}
	gui_display_capture_result(scrnmng.display_capture_path, saved);
	SDL_FreeSurface(cropped);
	SDL_FreeSurface(surface);
	scrnmng.display_capture_path[0] = '\0';
	scrnmng.display_capture_ready = FALSE;
}

static SDL_Surface *scrnmng_read_sdl_output(void) {
	int width, height;
	SDL_Surface *surface;
	if (SDL_GetRendererOutputSize(scrnmng.renderer, &width, &height) != 0) return NULL;
	surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
	if (surface && SDL_RenderReadPixels(scrnmng.renderer, NULL, SDL_PIXELFORMAT_RGBA32,
	                                   surface->pixels, surface->pitch) != 0) {
		SDL_FreeSurface(surface);
		return NULL;
	}
	return surface;
}

BOOL scrnmng_display_capture_selftest(void) {
	const SCRNMNG saved = scrnmng;
	const UINT8 saved_info = np2oscfg.DISPCLK;
	const BOOL video_active = SDL_WasInit(SDL_INIT_VIDEO) != 0;
	SDL_Surface *white = NULL, *baseline = NULL, *filtered = NULL, *info = NULL;
	VAEG_VIEWPORT viewport;
	BOOL ok = FAILURE;
	if (!video_active && SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) return FAILURE;
	ZeroMemory(&scrnmng, sizeof(scrnmng));
	scrnmng.window = SDL_CreateWindow("display capture test", 0, 0, 640, 422, SDL_WINDOW_HIDDEN);
	if (!scrnmng.window) goto cleanup;
	scrnmng.renderer = SDL_CreateRenderer(scrnmng.window, -1, SDL_RENDERER_SOFTWARE);
	if (!scrnmng.renderer) goto cleanup;
	white = SDL_CreateRGBSurfaceWithFormat(0, 640, 400, 32, SDL_PIXELFORMAT_RGBA32);
	if (!white) goto cleanup;
	SDL_FillRect(white, NULL, SDL_MapRGBA(white->format, 255, 255, 255, 255));
	scrnmng.texture = SDL_CreateTextureFromSurface(scrnmng.renderer, white);
	if (!scrnmng.texture) goto cleanup;
	scrnmng.enable = TRUE;
	scrnmng.width = 640; scrnmng.height = 400;
	scrnmng.menu_height = 22; scrnmng.scale = 1;
	scrnmng.scaling = VAEG_SCALING_FIT;
	np2oscfg.DISPCLK = 0;
	scrnmng_present_begin();
	baseline = scrnmng_read_sdl_output();
	scrnmng.effect = VAEG_EFFECT_SCANLINE;
	scrnmng_present_begin();
	filtered = scrnmng_read_sdl_output();
	if (!baseline || !filtered || scrnmng_calculate_viewport(&viewport) != SUCCESS) goto cleanup;
	info = scrnmng_crop_display_capture(baseline);
	if (!info || info->w != 640 || info->h != 400 || ((BYTE *)info->pixels)[0] != 255) goto cleanup;
	SDL_FreeSurface(info); info = NULL;
	scrnmng.menu_height = 0;
	info = scrnmng_crop_display_capture(baseline);
	if (!info || info->h != 422 || info->pixels != baseline->pixels) goto cleanup;
	SDL_FreeSurface(info); info = NULL;
	scrnmng.menu_height = 22;
	if (((BYTE *)baseline->pixels)[23 * baseline->pitch] != 255 ||
	    ((BYTE *)filtered->pixels)[23 * filtered->pitch] >= 255) goto cleanup;
	np2oscfg.DISPCLK = VAEG_DISPINFO_VIDEO | VAEG_DISPINFO_FRAMEBUFFER;
	scrnmng_draw_sdl_overlays();
	info = scrnmng_read_sdl_output();
	if (!info || memcmp(info->pixels, filtered->pixels, info->pitch * info->h) == 0) goto cleanup;
	{
		SDL_Surface *twice;
		scrnmng_draw_sdl_overlays();
		twice = scrnmng_read_sdl_output();
		const BOOL matches = twice && memcmp(twice->pixels, info->pixels, info->pitch * info->h) == 0;
		SDL_FreeSurface(twice);
		if (!matches) goto cleanup;
	}
	SDL_FreeSurface(info); info = NULL;
	np2oscfg.DISPCLK = 0;
	scrnmng_present_begin();
	scrnmng_draw_video_info_overlay(&viewport);
	scrnmng_draw_framebuffer_info_overlay(&viewport);
	info = scrnmng_read_sdl_output();
	if (!info || memcmp(info->pixels, filtered->pixels, info->pitch * info->h) != 0) goto cleanup;
	if (scrnmng_request_display_capture("test.png") != SUCCESS ||
	    scrnmng.display_capture_ready || !scrnmng_prepare_display_capture()) goto cleanup;
	if (scrnmng_request_display_capture("second.png") != FAILURE) goto cleanup;
	{
		const UINT8 flags[] = {0, VAEG_DISPINFO_VIDEO, VAEG_DISPINFO_FRAMEBUFFER,
		                       VAEG_DISPINFO_VIDEO | VAEG_DISPINFO_FRAMEBUFFER};
		UINT32 hashes[4] = {0};
		int mode, other;
		for (mode = 0; mode < 4; ++mode) {
			SDL_Surface *raw = SDL_ConvertSurface(white, white->format, 0);
			size_t byte;
			if (!raw) goto cleanup;
			np2oscfg.DISPCLK = flags[mode];
			if (scrnmng_draw_graphics_analysis(raw) != SUCCESS) {
				SDL_FreeSurface(raw); goto cleanup;
			}
			if (mode == 0 && memcmp(raw->pixels, white->pixels, raw->pitch * raw->h) != 0) {
				SDL_FreeSurface(raw); goto cleanup;
			}
			for (byte = 0; byte < (size_t)raw->pitch * raw->h; ++byte)
				hashes[mode] = hashes[mode] * 33U + ((BYTE *)raw->pixels)[byte];
			SDL_FreeSurface(raw);
			for (other = 0; other < mode; ++other)
				if (hashes[mode] == hashes[other]) goto cleanup;
		}
	}
	{
		const int requested[] = {1, 4, 5, VAEG_WINDOW_SCALE_MAX, VAEG_WINDOW_SCALE_MAX + 1, 0};
		const int expected[] = {1, 4, 5, VAEG_WINDOW_SCALE_MAX, VAEG_WINDOW_SCALE_MAX, 1};
		unsigned i;
		/* Small synthetic source avoids allocating enormous test windows. */
		scrnmng.width = 32;
		scrnmng.height = 20;
		scrnmng.display_mode = VAEG_DISPLAY_WINDOWED;
		for (i = 0; i < sizeof(requested) / sizeof(requested[0]); ++i) {
			int width, height;
			scrnmng_set_display(requested[i], TRUE);
			SDL_GetWindowSize(scrnmng.window, &width, &height);
			if (scrnmng_get_display_scale() != expected[i] ||
			    width != 32 * expected[i] || height != 20 * expected[i] + scrnmng.menu_height) {
				fprintf(stderr, "selftest: WINDOW_MULTIPLIER_MISMATCH\n");
				goto cleanup;
			}
		}
	}
	ok = SUCCESS;
cleanup:
	SDL_FreeSurface(info); SDL_FreeSurface(filtered); SDL_FreeSurface(baseline);
	SDL_FreeSurface(white);
	SDL_DestroyTexture(scrnmng.texture);
	SDL_DestroyRenderer(scrnmng.renderer);
	SDL_DestroyWindow(scrnmng.window);
	scrnmng = saved;
	np2oscfg.DISPCLK = saved_info;
	if (!video_active) SDL_QuitSubSystem(SDL_INIT_VIDEO);
	if (ok != SUCCESS) fprintf(stderr, "selftest: DISPLAY_CAPTURE_MISMATCH: %s\n", SDL_GetError());
	return ok;
}

void scrnmng_draw_sdl_overlays(void) {
	VAEG_VIEWPORT viewport;
	if (scrnmng.native_active || !scrnmng.renderer || scrnmng.sdl_overlays_drawn) return;
	if (scrnmng_calculate_viewport(&viewport) == SUCCESS) {
		scrnmng_draw_video_info_overlay(&viewport);
		scrnmng_draw_framebuffer_info_overlay(&viewport);
	}
	scrnmng.sdl_overlays_drawn = TRUE;
}

void scrnmng_present_end(void) {
	VAEG_VIEWPORT viewport;
	VAEG_FRAME_INPUT frame;
	VAEG_NATIVE_PRESENTER_RESULT native_result;
	int drawable_width;
	int drawable_height;

	if (!scrnmng.enable) {
		return;
	}
	if (scrnmng.native_active) {
		if (scrnmng.rendered_capture_enabled && !scrnmng.native_capture_warning) {
			fprintf(stderr,
			        "Warning: rendered capture is unavailable while Native CRT owns the output; "
			        "use guest-frame capture instead\n");
			scrnmng.native_capture_warning = TRUE;
		}
		if (scrnmng_get_drawable_size(&drawable_width, &drawable_height) != SUCCESS) {
			return;
		}
		native_result = vaeg_native_presenter_resize(
		    scrnmng.native_presenter, (UINT32)drawable_width, (UINT32)drawable_height);
		if (native_result == VAEG_NATIVE_PRESENTER_NO_OUTPUT) {
			return;
		}
		if (native_result == VAEG_NATIVE_PRESENTER_FALLBACK) {
			(void)scrnmng_native_fallback();
		} else {
			if (scrnmng_calculate_viewport(&viewport) == SUCCESS) {
				vaeg_native_presenter_set_output_viewport(scrnmng.native_presenter, viewport.x,
				                                          viewport.y, viewport.width,
				                                          viewport.height);
			}
			vaeg_frame_input_initialize(
			    &frame, scrnmng.shadow + (SCRNMNG_SURFACE_GUARD_LEFT * 2), scrnmng.width,
			    scrnmng.height, (UINT32)scrnmng.shadow_pitch, VAEG_FRAME_PIXEL_RGB565,
			    VAEG_FRAME_ROWS_TOP_DOWN, scrnmng.aspect ? 4U : (UINT32)scrnmng.width,
			    scrnmng.aspect ? 3U : (UINT32)scrnmng.height, vaeg_nominal_frame_rate, 1U,
			    scrnmng.native_frame_number++, 16666667U);
			if (scrnmng.display_capture_ready) {
				SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(
				    0, drawable_width, drawable_height, 32, SDL_PIXELFORMAT_RGBA32);
				VAEG_OUTPUT_CAPTURE capture = {0};
				if (surface) {
					capture.pixels = surface->pixels;
					capture.width = surface->w; capture.height = surface->h;
					capture.pitch_bytes = surface->pitch;
				}
				vaeg_native_presenter_set_output_capture(scrnmng.native_presenter, &capture);
				native_result = vaeg_native_presenter_present(scrnmng.native_presenter, &frame);
				vaeg_native_presenter_set_output_capture(scrnmng.native_presenter, NULL);
				scrnmng_finish_display_capture(surface, capture.complete ? SUCCESS : FAILURE);
			} else {
				native_result = vaeg_native_presenter_present(scrnmng.native_presenter, &frame);
			}
			if (native_result == VAEG_NATIVE_PRESENTER_NO_OUTPUT) {
				return;
			}
			if (native_result == VAEG_NATIVE_PRESENTER_FALLBACK) {
				(void)scrnmng_native_fallback();
			}
		}
		if (!scrnmng.native_active && (scrnmng.renderer != NULL)) {
			scrnmng_present_begin();
			scrnmng_present_end();
		}
		return;
	}
	if (scrnmng.renderer == NULL) {
		return;
	}
	scrnmng_draw_sdl_overlays();
	if (scrnmng.rendered_capture_enabled) {
		scrnmng_capture_rendered_frame();
	}
	if (scrnmng.display_capture_ready) {
		SDL_Surface *surface = scrnmng_read_sdl_output();
		scrnmng_finish_display_capture(surface, surface ? SUCCESS : FAILURE);
	}
	SDL_RenderPresent(scrnmng.renderer);
}

BOOL scrnmng_save_rendered_frame(const char *path) {
	if ((path == NULL) || (path[0] == '\0') || (scrnmng.renderer == NULL)) {
		return (FAILURE);
	}
	scrnmng_capture_rendered_frame();
	if (scrnmng.rendered_frame == NULL) {
		return (FAILURE);
	}
	if (scrnmng_path_is_png(path)) {
		if (scrnmng_png_save_surface(scrnmng.rendered_frame, path) != SUCCESS) {
			fprintf(stderr, "scsitrace rendered-screen-save-failed path=%s error=%s\n", path,
			        SDL_GetError());
			return (FAILURE);
		}
		return (SUCCESS);
	}
	if (SDL_SaveBMP(scrnmng.rendered_frame, path) != 0) {
		fprintf(stderr, "scsitrace rendered-screen-save-failed path=%s error=%s\n", path,
		        SDL_GetError());
		return (FAILURE);
	}
	return (SUCCESS);
}

BOOL scrnmng_save_guest_frame(const char *path) {
	SDL_Surface *surface;
	BOOL result;

	if ((path == NULL) || (path[0] == '\0') || (scrnmng.shadow == NULL)) {
		return (FAILURE);
	}
	surface = SDL_CreateRGBSurfaceWithFormatFrom(scrnmng.shadow + (SCRNMNG_SURFACE_GUARD_LEFT * 2),
	                                             scrnmng.width, scrnmng.height, 16,
	                                             scrnmng.shadow_pitch, SDL_PIXELFORMAT_RGB565);
	if (surface == NULL) {
		fprintf(stderr, "guest-screen-save-source-failed path=%s error=%s\n", path, SDL_GetError());
		return (FAILURE);
	}
	if (scrnmng_path_is_png(path)) {
		result = scrnmng_png_save_surface(surface, path);
	} else {
		result = (SDL_SaveBMP(surface, path) == 0) ? SUCCESS : FAILURE;
	}
	if (result != SUCCESS) {
		fprintf(stderr, "guest-screen-save-failed path=%s error=%s\n", path, SDL_GetError());
	}
	SDL_FreeSurface(surface);
	return (result);
}

BOOL scrnmng_save_guest_frame_with_analysis(const char *path) {
	SDL_Surface *source;
	SDL_Surface *surface;
	BOOL result;

	if ((path == NULL) || (path[0] == '\0') || (scrnmng.shadow == NULL)) {
		return (FAILURE);
	}
	source = SDL_CreateRGBSurfaceWithFormatFrom(scrnmng.shadow + (SCRNMNG_SURFACE_GUARD_LEFT * 2),
	                                            scrnmng.width, scrnmng.height, 16,
	                                            scrnmng.shadow_pitch, SDL_PIXELFORMAT_RGB565);
	if (source == NULL) {
		fprintf(stderr, "guest-screen-analysis-source-failed path=%s error=%s\n", path,
		        SDL_GetError());
		return (FAILURE);
	}
	surface = SDL_CreateRGBSurfaceWithFormat(0, scrnmng.width, scrnmng.height, 32,
	                                         SDL_PIXELFORMAT_ARGB8888);
	if (surface == NULL) {
		fprintf(stderr, "guest-screen-analysis-surface-failed path=%s error=%s\n", path,
		        SDL_GetError());
		SDL_FreeSurface(source);
		return (FAILURE);
	}
	if ((SDL_BlitSurface(source, NULL, surface, NULL) != 0) ||
	    (scrnmng_draw_graphics_analysis(surface) != SUCCESS)) {
		fprintf(stderr, "guest-screen-analysis-render-failed path=%s error=%s\n", path,
		        SDL_GetError());
		SDL_FreeSurface(surface);
		SDL_FreeSurface(source);
		return (FAILURE);
	}
	if (scrnmng_path_is_png(path)) {
		result = scrnmng_png_save_surface(surface, path);
	} else {
		result = (SDL_SaveBMP(surface, path) == 0) ? SUCCESS : FAILURE;
	}
	if (result != SUCCESS) {
		fprintf(stderr, "guest-screen-analysis-save-failed path=%s error=%s\n", path,
		        SDL_GetError());
	}
	SDL_FreeSurface(surface);
	SDL_FreeSurface(source);
	return (result);
}

void scrnmng_set_framedisp(BOOL enabled) {
	scrnmng.framedisp_enabled = enabled ? TRUE : FALSE;
	scrnmng_reset_metrics();
}

void scrnmng_reset_metrics(void) {
	vaeg_framedisp_reset(&scrnmng.framedisp, SDL_GetTicks(), drawcount);
	speedmeter.valid = FALSE;
	scrnmng_update_title();
}

void scrnmng_refresh_title(void) {
	scrnmng_update_title();
}

void scrnmng_framedisp_tick(UINT32 tick, UINT32 draws, UINT32 frames) {
	if (scrnmng.framedisp_enabled) {
		(void)vaeg_framedisp_update(&scrnmng.framedisp, tick, draws);
	}
	scrnmng_update_speedmeter(tick, frames);
	scrnmng_update_title();
}

BOOL scrnmng_entermenu(SCRNMENU *smenu) {
	if (smenu) {
		smenu->width = scrnmng.width;
		smenu->height = scrnmng.height;
		smenu->bpp = 16;
	}
	return (FAILURE);
}

void scrnmng_leavemenu(void) {
}

void scrnmng_menudraw(const RECT_T *rct) {
	(void)rct;
}
