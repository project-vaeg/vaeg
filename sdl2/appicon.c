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

#include "compiler.h"
#include "sdlapi.h"
#include "appicon.h"

extern const unsigned char vaeg_app_icon_ico[];
extern const unsigned int vaeg_app_icon_ico_size;

static UINT16 appicon_u16(const unsigned char *data) {
	return ((UINT16)(data[0] | ((UINT16)data[1] << 8)));
}

static UINT32 appicon_u32(const unsigned char *data) {
	return ((UINT32)data[0] | ((UINT32)data[1] << 8) | ((UINT32)data[2] << 16) |
	        ((UINT32)data[3] << 24));
}

static SDL_Surface *appicon_load_surface(void) {
	const unsigned char *icon;
	const unsigned char *entry;
	const unsigned char *bitmap;
	const unsigned char *pixels;
	SDL_Surface *surface;
	UINT32 best_area;
	UINT32 area;
	UINT32 offset;
	UINT32 size;
	UINT32 dib_size;
	UINT16 count;
	UINT16 index;
	UINT16 best_index;
	int width;
	int height;
	int row;

	icon = vaeg_app_icon_ico;
	if ((vaeg_app_icon_ico_size < 6) || (appicon_u16(icon) != 0) || (appicon_u16(icon + 2) != 1)) {
		return (NULL);
	}
	count = appicon_u16(icon + 4);
	if ((count == 0) || ((UINT32)count > ((vaeg_app_icon_ico_size - 6) / 16))) {
		return (NULL);
	}
	best_index = 0xffff;
	best_area = 0;
	for (index = 0; index < count; index++) {
		entry = icon + 6 + (index * 16);
		width = entry[0] ? entry[0] : 256;
		height = entry[1] ? entry[1] : 256;
		if (appicon_u16(entry + 6) != 32) {
			continue;
		}
		size = appicon_u32(entry + 8);
		offset = appicon_u32(entry + 12);
		if ((offset > vaeg_app_icon_ico_size) || (size > (vaeg_app_icon_ico_size - offset)) ||
		    (size < 40)) {
			continue;
		}
		bitmap = icon + offset;
		if ((appicon_u32(bitmap) < 40) || ((int)appicon_u32(bitmap + 4) != width) ||
		    ((int)appicon_u32(bitmap + 8) != (height * 2)) || (appicon_u16(bitmap + 12) != 1) ||
		    (appicon_u16(bitmap + 14) != 32) || (appicon_u32(bitmap + 16) != 0)) {
			continue;
		}
		area = (UINT32)(width * height);
		if (area > best_area) {
			best_area = area;
			best_index = index;
		}
	}
	if (best_index == 0xffff) {
		return (NULL);
	}
	entry = icon + 6 + (best_index * 16);
	width = entry[0] ? entry[0] : 256;
	height = entry[1] ? entry[1] : 256;
	offset = appicon_u32(entry + 12);
	bitmap = icon + offset;
	dib_size = appicon_u32(bitmap);
	if ((dib_size > vaeg_app_icon_ico_size - offset) ||
	    ((UINT32)(width * height * 4) > (vaeg_app_icon_ico_size - offset - dib_size))) {
		return (NULL);
	}
	pixels = bitmap + dib_size;
	surface =
	    SDL_CreateRGBSurface(0, width, height, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
	if (surface == NULL) {
		return (NULL);
	}
	for (row = 0; row < height; row++) {
		memcpy((BYTE *)surface->pixels + (row * surface->pitch),
		       pixels + ((height - row - 1) * width * 4), width * 4);
	}
	return (surface);
}

void appicon_set_window(void *window) {
	SDL_Surface *surface;

	if (window == NULL) {
		return;
	}
	surface = appicon_load_surface();
	if (surface == NULL) {
		fprintf(stderr, "Warning: embedded application icon is invalid: %s\n", SDL_GetError());
		return;
	}
	SDL_SetWindowIcon((SDL_Window *)window, surface);
	SDL_FreeSurface(surface);
}
