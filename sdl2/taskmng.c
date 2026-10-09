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
#include "taskmng.h"
#include "sdlkbd.h"
#include "kbdmap.h"
#include "kbdpaste.h"
#include "mousemng.h"
#include "pacing.h"
#include "np2.h"
#include "scrnmng.h"
#include "sysmng.h"
#include "machine/timing.h"
#include "soundmng.h"
#include "gui/gui.h"
#include "dropmedia.h"

BOOL task_avail;
static BOOL task_paused;
static VAEG_PACING_STATE task_pacing;
static BOOL paste_shortcut_down;
static BOOL copy_shortcut_down;
static BOOL middle_shortcut_down;
static BOOL mouse_key_shortcut_down;
static BOOL screenshot_shortcut_down;

static BOOL taskmng_paste_shortcut(const SDL_Event *event, BOOL captured) {
	UINT16 modifier;

	if (event->type == SDL_KEYUP) {
		if ((event->key.keysym.scancode == SDL_SCANCODE_V) && paste_shortcut_down) {
			paste_shortcut_down = FALSE;
			return TRUE;
		}
		return FALSE;
	}
	if ((event->type != SDL_KEYDOWN) || (event->key.keysym.scancode != SDL_SCANCODE_V)) {
		return FALSE;
	}
	modifier = (UINT16)event->key.keysym.mod;
#if defined(__APPLE__)
	if (!(modifier & KMOD_GUI)) {
#else
	if (!(modifier & KMOD_CTRL)) {
#endif
		return FALSE;
	}
	if (captured) {
		return FALSE;
	}
	paste_shortcut_down = TRUE;
	if (!event->key.repeat) {
		kbdpaste_start_clipboard();
	}
	return TRUE;
}

static BOOL taskmng_copy_shortcut(const SDL_Event *event, BOOL captured) {
	UINT16 modifier;

	if (event->type == SDL_KEYUP) {
		if ((event->key.keysym.scancode == SDL_SCANCODE_C) && copy_shortcut_down) {
			copy_shortcut_down = FALSE;
			return TRUE;
		}
		return FALSE;
	}
	if ((event->type != SDL_KEYDOWN) || (event->key.keysym.scancode != SDL_SCANCODE_C)) {
		return FALSE;
	}
	modifier = (UINT16)event->key.keysym.mod;
#if defined(__APPLE__)
	if (!(modifier & KMOD_GUI)) {
#else
	if (!(modifier & KMOD_CTRL) || !(modifier & KMOD_SHIFT)) {
#endif
		return FALSE;
	}
	if (captured) {
		return FALSE;
	}
	copy_shortcut_down = TRUE;
	if (!event->key.repeat) {
		gui_copy_screen_text();
	}
	return TRUE;
}

static BOOL taskmng_screenshot_shortcut(const SDL_Event *event, BOOL captured) {
	if ((event->type != SDL_KEYDOWN) && (event->type != SDL_KEYUP)) {
		return FALSE;
	}
	if ((event->type == SDL_KEYUP) && screenshot_shortcut_down &&
	    (kbdmap_special_action((UINT)event->key.keysym.scancode) == KBDMAP_SPECIAL_SCREENSHOT)) {
		screenshot_shortcut_down = FALSE;
		return TRUE;
	}
	if (kbdmap_special_action((UINT)event->key.keysym.scancode) != KBDMAP_SPECIAL_SCREENSHOT) {
		return FALSE;
	}
	if (captured) {
		return FALSE;
	}
	if ((event->type == SDL_KEYDOWN) && !event->key.repeat) {
		screenshot_shortcut_down = TRUE;
		(void)gui_save_screenshot();
	}
	return TRUE;
}

static void taskmng_toggle_mouse(void) {
	np2oscfg.MOUSE_SW = mousemng_togglecapture() ? 1 : 0;
	sysmng_update(SYS_UPDATEOSCFG);
}

static BOOL taskmng_mouse_key_shortcut(const SDL_Event *event, BOOL captured) {
	KBDMAP_SPECIAL_ACTION action;

	if ((event->type != SDL_KEYDOWN) && (event->type != SDL_KEYUP)) {
		return FALSE;
	}
	action = kbdmap_special_action((UINT)event->key.keysym.scancode);
	if ((event->type == SDL_KEYUP) && mouse_key_shortcut_down) {
		mouse_key_shortcut_down = FALSE;
		return TRUE;
	}
	if ((action != KBDMAP_SPECIAL_MOUSE_CAPTURE) || (np2oscfg.F12KEY != 0)) {
		return FALSE;
	}
	if ((event->type == SDL_KEYDOWN) && !event->key.repeat && !captured) {
		mouse_key_shortcut_down = TRUE;
		taskmng_toggle_mouse();
	}
	return TRUE;
}

static BOOL taskmng_mouse_button_shortcut(const SDL_Event *event, BOOL captured) {
	if (((event->type != SDL_MOUSEBUTTONDOWN) && (event->type != SDL_MOUSEBUTTONUP)) ||
	    (event->button.button != SDL_BUTTON_MIDDLE)) {
		return FALSE;
	}
	if (event->type == SDL_MOUSEBUTTONUP) {
		if (middle_shortcut_down) {
			middle_shortcut_down = FALSE;
			return TRUE;
		}
		return FALSE;
	}
	if (!captured) {
		middle_shortcut_down = TRUE;
		taskmng_toggle_mouse();
		return TRUE;
	}
	return FALSE;
}

void taskmng_initialize(void) {
	task_avail = TRUE;
	task_paused = FALSE;
	paste_shortcut_down = FALSE;
	middle_shortcut_down = FALSE;
	mouse_key_shortcut_down = FALSE;
	screenshot_shortcut_down = FALSE;
	vaeg_pacing_reset(&task_pacing);
	kbdpaste_initialize();
	mousemng_initialize();
	mousemng_setguiblocked(gui_guest_mouse_blocked());
	mousemng_setcapture(np2oscfg.MOUSE_SW ? TRUE : FALSE);
}

void taskmng_exit(void) {
	task_avail = FALSE;
	task_paused = FALSE;
	paste_shortcut_down = FALSE;
	middle_shortcut_down = FALSE;
	mouse_key_shortcut_down = FALSE;
	screenshot_shortcut_down = FALSE;
	vaeg_pacing_reset(&task_pacing);
	kbdpaste_shutdown();
	mousemng_shutdown();
}

void taskmng_toggle_pause(void) {
	task_paused = task_paused ? FALSE : TRUE;
	/* Drop wall-clock debt so resuming does not catch up skipped time. */
	timing_reset();
	if (task_paused) {
		soundmng_stop();
	} else {
		soundmng_play();
	}
}

BOOL taskmng_ispaused(void) {
	return (task_paused);
}

void taskmng_clear_fast_forward(void) {
	vaeg_pacing_reset(&task_pacing);
}

BOOL taskmng_effective_nowait(BOOL configured_nowait) {
	return (vaeg_pacing_effective_nowait(&task_pacing, configured_nowait));
}

UINT taskmng_effective_drawskip(UINT configured_drawskip) {
	return (vaeg_pacing_effective_drawskip(&task_pacing, configured_drawskip));
}

void taskmng_rol(void) {
	SDL_Event e;

	while (task_avail && SDL_PollEvent(&e)) {
		BOOL captured;
		BOOL shortcut;
		/* Rebinding a renderer may replace the host window. Old focus/size
		 * notifications must not overwrite the replacement window's state. */
		if (e.type == SDL_WINDOWEVENT) {
			SDL_Window *window = (SDL_Window *)scrnmng_get_window();
			if (window && e.window.windowID != SDL_GetWindowID(window)) continue;
		}

		if (dropmedia_process_event(&e)) {
			continue;
		}
		shortcut = FALSE;
		if (e.type == SDL_KEYDOWN) {
			if (kbdmap_special_action((UINT)e.key.keysym.scancode) == KBDMAP_SPECIAL_FAST_FORWARD) {
				shortcut = vaeg_pacing_key(&task_pacing, (UINT)e.key.keysym.scancode, TRUE,
				                           e.key.repeat ? TRUE : FALSE);
			}
		} else if (e.type == SDL_KEYUP) {
			shortcut = vaeg_pacing_key(&task_pacing, (UINT)e.key.keysym.scancode, FALSE, FALSE);
		} else if ((e.type == SDL_WINDOWEVENT) && (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST)) {
			vaeg_pacing_reset(&task_pacing);
			paste_shortcut_down = FALSE;
			copy_shortcut_down = FALSE;
			middle_shortcut_down = FALSE;
			screenshot_shortcut_down = FALSE;
			kbdpaste_cancel();
			mousemng_setfocus(FALSE);
		} else if ((e.type == SDL_WINDOWEVENT) &&
		           (e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)) {
			mousemng_setfocus(TRUE);
		} else if ((e.type == SDL_WINDOWEVENT) &&
		           (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)) {
			int width;
			int height;

			if ((scrnmng_capture_window_size(&width, &height) == SUCCESS) &&
			    ((np2oscfg.gui_window_width != width) || (np2oscfg.gui_window_height != height))) {
				np2oscfg.gui_window_width = (UINT16)min(width, 65535);
				np2oscfg.gui_window_height = (UINT16)min(height, 65535);
				sysmng_update(SYS_UPDATEOSCFG);
			}
		} else if (e.type == SDL_QUIT) {
			vaeg_pacing_reset(&task_pacing);
			paste_shortcut_down = FALSE;
			copy_shortcut_down = FALSE;
			middle_shortcut_down = FALSE;
			screenshot_shortcut_down = FALSE;
			kbdpaste_cancel();
			mousemng_setfocus(FALSE);
		}
		captured = gui_process_event(&e);
		mousemng_setguiblocked(gui_guest_mouse_blocked());
		if (taskmng_paste_shortcut(&e, captured)) {
			shortcut = TRUE;
		}
		if (taskmng_mouse_key_shortcut(&e, captured)) {
			shortcut = TRUE;
		}
		if (taskmng_copy_shortcut(&e, captured)) {
			shortcut = TRUE;
		}
		if (taskmng_screenshot_shortcut(&e, captured)) {
			shortcut = TRUE;
		}
		if (taskmng_mouse_button_shortcut(&e, captured)) {
			shortcut = TRUE;
		}
		switch (e.type) {
		case SDL_QUIT:
			taskmng_exit();
			break;

		case SDL_KEYDOWN:
			if (!shortcut) {
				sdlkbd_keydown((UINT)e.key.keysym.scancode, e.key.keysym.sym,
				               (UINT16)e.key.keysym.mod, captured, e.key.repeat ? TRUE : FALSE);
			}
			break;

		case SDL_KEYUP:
			if (!shortcut) {
				sdlkbd_keyup((UINT)e.key.keysym.scancode, e.key.keysym.sym,
				             (UINT16)e.key.keysym.mod, captured);
			}
			break;

		case SDL_TEXTINPUT:
			sdlkbd_textinput(e.text.text, captured);
			break;

		case SDL_MOUSEMOTION:
			if (!captured && !shortcut) {
				mousemng_motion(e.motion.xrel, e.motion.yrel);
			}
			break;

		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			if (!shortcut &&
			    ((e.button.button == SDL_BUTTON_LEFT) || (e.button.button == SDL_BUTTON_RIGHT))) {
				UINT button;

				button = (e.button.button == SDL_BUTTON_LEFT) ? MOUSEMNG_BUTTON_LEFT
				                                              : MOUSEMNG_BUTTON_RIGHT;
				if (!captured || (e.type == SDL_MOUSEBUTTONUP)) {
					mousemng_button(button, e.type == SDL_MOUSEBUTTONDOWN ? TRUE : FALSE);
				}
			}
			break;

		default:
			break;
		}
	}
	if (task_avail) {
		mousemng_setguiblocked(gui_guest_mouse_blocked());
		kbdpaste_tick(SDL_GetTicks(), gui_guest_keyboard_blocked());
	}
}

BOOL taskmng_sleep(UINT32 tick) {
	Uint64 base;
	Uint64 freq;
	Uint64 elapsed;

	base = SDL_GetPerformanceCounter();
	freq = SDL_GetPerformanceFrequency();
	do {
		taskmng_rol();
		if (!task_avail) {
			break;
		}
		SDL_Delay(1);
		elapsed = ((SDL_GetPerformanceCounter() - base) * 1000) / freq;
	} while (elapsed < tick);
	return (task_avail);
}
