/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "compiler.h"
#include "viewport.h"

static int fit_width(int available_width, int available_height, int ratio_width, int ratio_height) {
	UINT64 width;

	width = ((UINT64)available_height * ratio_width) / ratio_height;
	if (width > (UINT64)available_width) {
		width = available_width;
	}
	return ((int)width);
}

BOOL vaeg_viewport_calculate(const VAEG_VIEWPORT_INPUT *input, VAEG_VIEWPORT *viewport) {
	int available_width;
	int available_height;
	int ratio_width;
	int ratio_height;
	int scale;

	if ((input == NULL) || (viewport == NULL)) {
		return (FAILURE);
	}
	ZeroMemory(viewport, sizeof(*viewport));
	if ((input->guest_width <= 0) || (input->guest_height <= 0) || (input->drawable_width <= 0) ||
	    (input->drawable_height <= 0) || (input->scaling < 0) ||
	    (input->scaling >= VAEG_SCALING_COUNT)) {
		return (FAILURE);
	}
	viewport->y = max(0, min(input->menu_inset, input->drawable_height));
	available_width = input->drawable_width;
	available_height = input->drawable_height - viewport->y;
	if ((available_width <= 0) || (available_height <= 0)) {
		return (FAILURE);
	}
	ratio_width = input->aspect ? 4 : input->guest_width;
	ratio_height = input->aspect ? 3 : input->guest_height;

	switch (input->scaling) {
	case VAEG_SCALING_NATIVE:
		viewport->height = min(input->guest_height, available_height);
		viewport->width = (int)(((UINT64)viewport->height * ratio_width) / ratio_height);
		if (viewport->width > available_width) {
			viewport->width = available_width;
			viewport->height = (int)(((UINT64)viewport->width * ratio_height) / ratio_width);
		}
		break;

	case VAEG_SCALING_FIT:
	case VAEG_SCALING_FIT_8DOT:
		viewport->width = fit_width(available_width, available_height, ratio_width, ratio_height);
		if (input->scaling == VAEG_SCALING_FIT_8DOT) {
			viewport->width &= ~7;
		}
		if (viewport->width <= 0) {
			return (FAILURE);
		}
		viewport->height = (int)(((UINT64)viewport->width * ratio_height) / ratio_width);
		break;

	case VAEG_SCALING_INTEGER:
		scale = min(available_width / input->guest_width, available_height / input->guest_height);
		if (scale < 1) {
			scale = 1;
		}
		viewport->width = min(input->guest_width * scale, available_width);
		viewport->height = min(input->guest_height * scale, available_height);
		break;

	case VAEG_SCALING_STRETCH:
		viewport->width = available_width;
		viewport->height = available_height;
		break;
	}
	if ((viewport->width <= 0) || (viewport->height <= 0)) {
		return (FAILURE);
	}
	viewport->x = (available_width - viewport->width) / 2;
	viewport->y += (available_height - viewport->height) / 2;
	viewport->scale_x = (double)viewport->width / input->guest_width;
	viewport->scale_y = (double)viewport->height / input->guest_height;
	viewport->valid = TRUE;
	return (SUCCESS);
}

BOOL vaeg_viewport_map_point(const VAEG_VIEWPORT *viewport, int guest_width, int guest_height,
                             int drawable_x, int drawable_y, int *guest_x, int *guest_y) {
	if ((viewport == NULL) || (!viewport->valid) || (guest_width <= 0) || (guest_height <= 0) ||
	    (guest_x == NULL) || (guest_y == NULL) || (drawable_x < viewport->x) ||
	    (drawable_y < viewport->y) || (drawable_x >= (viewport->x + viewport->width)) ||
	    (drawable_y >= (viewport->y + viewport->height))) {
		return (FAILURE);
	}
	*guest_x = (int)(((UINT64)(drawable_x - viewport->x) * guest_width) / viewport->width);
	*guest_y = (int)(((UINT64)(drawable_y - viewport->y) * guest_height) / viewport->height);
	return (SUCCESS);
}

UINT8 vaeg_fscrnmod_sanitize(UINT value, BOOL *masked) {
	if (masked != NULL) {
		*masked = (value & ~7U) ? TRUE : FALSE;
	}
	return ((UINT8)(value & 7U));
}

void vaeg_fullscreen_size(UINT fscrn_cx, UINT fscrn_cy, UINT8 fscrnmod, int current_width,
                          int current_height, int *width, int *height) {
	BOOL current;

	current = (fscrnmod & 4) ? TRUE : FALSE;
	if (width != NULL) {
		*width = current ? max(1, current_width) : (fscrn_cx ? (int)fscrn_cx : 640);
	}
	if (height != NULL) {
		*height = current ? max(1, current_height) : (fscrn_cy ? (int)fscrn_cy : 400);
	}
}
