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
#include "sdlapi.h"

#include "imgui.h"
#include "vaeg_licenses.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <string>
#include <vector>

#include "compiler.h"
#include "gui/gui.h"
#include "codecnv.h"
#include "memoryva.h"
#include "maketextva.h"
#include "bkupmemva.h"
#include "n80rom.h"
#include "cmt.h"
#include "diskdrv.h"
#include "dosio.h"
#include "dropmedia.h"
#include "hostfat_manager.h"
#include "hostfat_path.h"
#include "fddfile.h"
#include "fontdata.h"
#include "romva.h"
#include "machine/keystat.h"
#include "newdisk.h"
#include "np2.h"
#include "np2info.h"
#include "np2ver.h"
#include "sound.h"
#include "adpcm.h"
#include "tsp.h"
#include "videova.h"
#include "beep.h"
#include "bmsio.h"
#include "emsio.h"
#include "ini.h"
#include "machine/pccore.h"
#include "machine/timing.h"
#include "sxsi.h"
#include "fdd_mtr.h"
#include "kbdmap.h"
#include "kbdpaste.h"
#include "mousemng.h"
#include "mouseifva.h"
#include "opngen.h"
#include "psggen.h"
#include "rhythm.h"
#include "scrndrawva.h"
#include "scrnmng.h"
#include "scrndraw.h"
#include "sdlkbd.h"
#include "sgp.h"
#include "soundmng.h"
#include "soundopts.h"
#include "strres.h"
#include "sysmng.h"
#include "taskmng.h"
#include "timemng.h"
#include "ymfmbridge.h"
#include "librashader/shader_preset.h"

extern "C" {
extern _RHYTHM rhythm;
extern _ADPCM adpcm;
extern const unsigned char vaeg_gui_font_ttf[];
extern const unsigned int vaeg_gui_font_ttf_size;
extern const unsigned char vaeg_splash_bmp[];
extern const unsigned int vaeg_splash_bmp_size;
}

namespace {

constexpr float kGuiFontSize = 16.0f;
constexpr int kStateSlots = 10;
constexpr int kMasterVolumeMax = 128;
constexpr int kSasiImageCount = 6;
constexpr int kScsiImageCount = 6;
constexpr int kCpuPresets[] = {1, 2, 4, 5, 6, 8, 10, 12, 16, 20};
constexpr int kSgpPresets[] = {1, 2, 4, 8, 16};
constexpr int kNdp8087Presets[] = {5000000, 8000000, 10000000};
constexpr int kSoundBufferPresets[] = {40, 100, 200, 500, 1000};
constexpr UINT kV98FontRomSize = 0x46800;
constexpr const char kAboutInfoTemplate[] = "CPU: %CPU% %CPUCLK%\n"
                                            "SGP: %SGPCLK%\n"
                                            "FRAME: %FRAME%\n"
                                            "BUILD COMMIT: %COMMIT%\n"
                                            "MODEL: %MODEL%\n"
                                            "SOUND: %SND%\n"
                                            "RHYTHM: %RHYTHM%\n"
                                            "\n"
                                            "[88VA]\n"
                                            "ROM TYPE: %ROMTPVA%\n"
                                            "ROM(Main): %BIOSVA%\n"
                                            "ROM(VupB): %BIOS91%\n"
                                            "ROM(Sub): %BIOSSUB%\n"
                                            "Z80 MODE: %Z80MODE%\n"
                                            "ROM(N80): %BIOSN80%";
namespace fs = std::filesystem;

struct SasiImageChoice {
	const char *label;
	UINT hdd_type;
};

static const SasiImageChoice kSasiImageChoices[kSasiImageCount] = {
    {"5 MB", 0}, {"10 MB", 1}, {"15 MB", 2}, {"20 MB", 4}, {"30 MB", 5}, {"40 MB", 6},
};

struct ScsiImageChoice {
	const char *label;
	UINT size_mb;
};

static const ScsiImageChoice kScsiImageChoices[kScsiImageCount] = {
    {"5 MB", 5}, {"10 MB", 10}, {"20 MB", 20}, {"40 MB", 40}, {"80 MB", 80}, {"160 MB", 160},
};

static void reset_guest(void);
static bool is_directory(const std::string &path);
static void open_hostfat_browser(void);
static void draw_hostfat_browser_popup(void);
static void draw_state_error_dialog(void);
static void draw_hostfat_error_dialog(void);

static std::string state_slot_path(int slot) {
	char name[32];
	char path[MAX_PATH];

	std::snprintf(name, sizeof(name), "np2sdl.S%02d", slot);
	file_getstatepath(path, sizeof(path), name);
	return std::string(path);
}

struct BrowserEntry {
	std::string name;
	std::string path;
	bool is_dir = false;
};

struct FullscreenMenuState {
	bool exclusive = false;
	bool visible = false;
	Uint32 last_activity = 0;
	Uint32 entered_at = 0;
};

static constexpr float kFullscreenRevealHeight = 12.0f;

static bool fullscreen_hint_visible(const FullscreenMenuState &state, Uint32 now) {
	return state.exclusive && static_cast<Uint32>(now - state.entered_at) < 3000U;
}

static bool update_fullscreen_menu(FullscreenMenuState &state, bool exclusive,
                                   bool at_top, bool over_bar, bool interacting, Uint32 now) {
	if (!exclusive) {
		state = FullscreenMenuState{};
		return true;
	}
	if (!state.exclusive) {
		state = FullscreenMenuState{};
		state.exclusive = true;
		state.entered_at = now;
	}
	if (at_top) state.visible = true;
	if (state.visible) {
		if (at_top || over_bar || interacting) state.last_activity = now;
		else if (static_cast<Uint32>(now - state.last_activity) >= 500U) state.visible = false;
	}
	return state.visible;
}

struct GuiState {
	FullscreenMenuState fullscreen_menu;
	bool initialized = false;
	bool text_input_active = false;
	SDL_Renderer *renderer = nullptr;
	SDL_Window *window = nullptr;
	bool native_renderer = false;
	SDL_Texture *about_texture = nullptr;
	ImTextureData *native_about_texture = nullptr;
	int about_texture_width = 0;
	int about_texture_height = 0;
	float menu_font_size = kGuiFontSize;
	ImGuiStyle base_style;
	float ui_scale = 0.0f;
	int fdd_dialog_drive = -1;
	char fdd_path[2][MAX_PATH] = {};
	bool fdd_browser_open = false;
	bool fdd_browser_refresh = false;
	std::string fdd_browser_dir;
	std::vector<BrowserEntry> fdd_entries;
	std::string fdd_status;
	std::string font_status;
	std::string screenshot_status;
	vaeg::librashader::ShaderPreset native_crt_preset;
	std::string native_crt_loaded_path;
	std::string native_crt_status;
	bool native_crt_settings_open = false;
	bool new_fdd_open = false;
	bool new_fdd_refresh = false;
	int new_fdd_format = NEWDISK_FDD_MSDOS_2HD;
	int new_fdd_container = NEWDISK_FDD_CONTAINER_D88;
	int new_fdd_drive = 0;
	bool new_fdd_mount_after_create = true;
	char new_fdd_path[MAX_PATH] = {};
	int hdd_dialog_drive = -1;
	char hdd_path[4][MAX_PATH] = {};
	bool hdd_browser_open = false;
	// Cassette tape (vaeg V1/V2 extension).
	bool tape_browser_open = false;
	bool tape_browser_refresh = false;
	std::string tape_browser_dir;
	std::vector<BrowserEntry> tape_entries;
	char tape_path[MAX_PATH] = {};
	std::string tape_loaded;
	std::string tape_recording;
	std::string tape_status;
	bool hdd_browser_refresh = false;
	std::string hdd_browser_dir;
	std::vector<BrowserEntry> hdd_entries;
	std::string hdd_status;
	bool new_sasi_open = false;
	bool new_sasi_refresh = false;
	int new_sasi_drive = 0;
	int new_sasi_choice = 3;
	bool new_sasi_open_after_create = true;
	char new_sasi_path[MAX_PATH] = {};
	bool new_scsi_open = false;
	bool new_scsi_refresh = false;
	int new_scsi_drive = 0;
	int new_scsi_choice = 3;
	bool new_scsi_open_after_create = true;
	char new_scsi_path[MAX_PATH] = {};
	std::string state_status;
	bool state_error_open = false;
	bool state_error_request = false;
	bool state_force_hostfat_available = false;
	std::string state_force_hostfat_path;
	std::string keyboard_status;
	bool keyboard_config_open = false;
	int capture_binding = -1;
	SDL_Scancode capture_swallow = SDL_SCANCODE_UNKNOWN;
	bool configure_open = false;
	bool configure_request = false;
	int pending_cpu_multiplier = PCCORE_STANDARD_MULTIPLE;
	int pending_sgp_mode = SGP_SPEED_MODEL_DEFAULT;
	int pending_sgp_multiplier = 1;
	bool pending_ndp8087_enabled = false;
	int pending_ndp8087_clock_hz = static_cast<int>(UPD8087_DEFAULT_CLOCK_HZ);
	int pending_pacing_ms = 0;
	bool pending_hostfat_enabled = false;
	char pending_hostfat_dir[MAX_PATH] = {};
	bool pending_hostfat_rebuild = false;
	bool hostfat_reset_after_build = false;
	std::string hostfat_rebuild_dir;
	bool hostfat_browser_open = false;
	bool hostfat_browser_request = false;
	bool hostfat_browser_refresh = false;
	std::string hostfat_browser_dir;
	std::vector<BrowserEntry> hostfat_entries;
	std::string hostfat_status;
	bool hostfat_error_open = false;
	bool hostfat_error_request = false;
	std::string hostfat_error_message;
	bool bms_config_open = false;
	bool bms_config_request = false;
	bool pending_bms_enabled = false;
	int pending_bms_port = 0;
	int pending_bms_banks = BMSIO_DEFAULT_BANKS;
	bool ems_config_open = false;
	bool ems_config_request = false;
	bool pending_ems_enabled = false;
	int pending_ems_megabytes = EMSIO_DEFAULT_MEGABYTES;
	bool custom_size_open = false;
	bool custom_size_request = false;
	bool custom_size_integer = true;
	int pending_window_scale = 4;
	int pending_window_width = 640;
	int pending_window_height = 422;
	bool sound_buffer_open = false;
	bool sound_buffer_request = false;
	int pending_sound_buffer_ms = VAEG_SOUND_BUFFER_DEFAULT_MS;
	bool about_open = false;
	bool about_request = false;
	bool about_more = false;
	char about_info[2048] = {};
};

GuiState g_gui;

struct CopyTextFrame {
	UINT16 vw;
	UINT8 mode;
	UINT32 rsa;
	UINT16 rh;
	UINT16 rw;
};

static void copy_append_utf8(std::string *text, UINT32 codepoint) {
	if (codepoint <= 0x7f) {
		text->push_back(static_cast<char>(codepoint));
	} else if (codepoint <= 0x7ff) {
		text->push_back(static_cast<char>(0xc0 | (codepoint >> 6)));
		text->push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
	} else if (codepoint <= 0xffff) {
		text->push_back(static_cast<char>(0xe0 | (codepoint >> 12)));
		text->push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
		text->push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
	} else {
		text->push_back(static_cast<char>(0xf0 | (codepoint >> 18)));
		text->push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3f)));
		text->push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
		text->push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
	}
}

static void copy_append_replacement(std::string *text) {
	copy_append_utf8(text, 0xfffd);
}

static void copy_append_hccode(std::string *text, UINT16 hccode) {
	char sjis[3];
	UINT16 utf[4];
	UINT16 jis1;
	UINT16 jis2;
	UINT row;

	if (hccode & 0x8000) {
		return;
	}
	if ((hccode & 0xff00) == 0) {
		if ((hccode == 0) || (hccode == 0x20)) {
			text->push_back(' ');
		} else if ((hccode >= 0x21) && (hccode <= 0x7e)) {
			text->push_back(static_cast<char>(hccode));
		} else if ((hccode >= 0xa1) && (hccode <= 0xdf)) {
			sjis[0] = static_cast<char>(hccode);
			sjis[1] = '\0';
			ZeroMemory(utf, sizeof(utf));
			codecnv_sjis2utf(utf, NELEMENTS(utf), sjis, 2);
			if (utf[0] != 0) {
				copy_append_utf8(text, utf[0]);
			} else {
				copy_append_replacement(text);
			}
		} else {
			copy_append_replacement(text);
		}
		return;
	}
	jis1 = (hccode & 0x7f) + 0x20;
	jis2 = (hccode >> 8) & 0x7f;
	if ((jis1 < 0x21) || (jis1 > 0x7e) || (jis2 < 0x21) || (jis2 > 0x7e)) {
		copy_append_replacement(text);
		return;
	}
	row = jis1 - 0x21;
	/* The JIS row parity selects the Shift-JIS trail-byte formula. */
	sjis[0] = static_cast<char>((row >> 1) + 0x81);
	if ((static_cast<unsigned char>(sjis[0])) >= 0xa0) {
		sjis[0] = static_cast<char>(sjis[0] + 0x40);
	}
	if (row & 1) {
		sjis[1] = static_cast<char>(jis2 + 0x7e);
	} else {
		sjis[1] = static_cast<char>(jis2 + 0x1f);
		if (static_cast<unsigned char>(sjis[1]) >= 0x7f) {
			sjis[1] = static_cast<char>(sjis[1] + 1);
		}
	}
	sjis[2] = '\0';
	ZeroMemory(utf, sizeof(utf));
	codecnv_sjis2utf(utf, NELEMENTS(utf), sjis, 3);
	if (utf[0] != 0) {
		copy_append_utf8(text, utf[0]);
	} else {
		copy_append_replacement(text);
	}
}

/*
 * V1/V2 text under the TSP's uPD3301 emulation: one byte per character in
 * the 8801 format, read as it is displayed (byte-mode addresses, main RAM in
 * standard speed). With port 30h 80CM clear (40 columns) only even byte
 * columns hold characters. Semigraphics cells are copied as their codes.
 */
static void copy_3301_text(std::vector<std::string> *lines) {
	const UINT step = (videova.txtmode8 & 0x01) ? 1 : 2;
	UINT row;

	for (row = 0;; row++) {
		const BYTE *chars = maketextva_3301_text(row);
		std::string line;
		UINT column;

		if (chars == nullptr) {
			if (row >= tsp.emul_rows) {
				break;
			}
			lines->push_back(line);
			continue;
		}
		for (column = 0; column < tsp.emul_chars; column += step) {
			copy_append_hccode(&line, chars[column]);
		}
		while (!line.empty() && (line.back() == ' ')) {
			line.pop_back();
		}
		lines->push_back(line);
	}
}

static BOOL copy_screen_text(void) {
	std::vector<std::string> lines;
	const UINT32 tvram_size = 0x40000;
	const UINT frame_count = 4;
	const UINT lineheight = (tsp.lineheight != 0) ? tsp.lineheight : 1;
	const UINT visible_columns =
	    (videova.txtmode8 & 0x01) ? (SURFACE_WIDTH / 8) : (SURFACE_WIDTH / 16);
	UINT32 raster_used = 0;
	UINT frame_no;

	if (memoryva_88_mode && tsp.emul) {
		copy_3301_text(&lines);
		raster_used = 0x1fe; /* the native frames below are not shown */
	}
	for (frame_no = 0; frame_no < frame_count && raster_used < 0x1fe; frame_no++) {
		BYTE *entry = textmem + tsp.texttable + frame_no * 0x20;
		CopyTextFrame frame;
		UINT raster_count;
		UINT rows;
		UINT columns;
		UINT frame_columns;
		UINT row;

		if ((tsp.texttable + frame_no * 0x20 + 0x1c) >= tvram_size) {
			break;
		}
		frame.vw = LOADINTELWORD(entry + 0x08) & 0x03ff;
		frame.mode = LOADINTELWORD(entry + 0x0a) & 0x07;
		frame.rsa = LOADINTELWORD(entry + 0x10);
		frame.rh = LOADINTELWORD(entry + 0x14) & 0x01fe;
		frame.rw = LOADINTELWORD(entry + 0x16) & 0x03ff;
		if (frame.rh == 0) {
			frame.rh = 0x01fe;
		}
		raster_count = (frame_no == frame_count - 1) ? 0x1fe - raster_used : frame.rh;
		if (raster_count > 0x1fe - raster_used) {
			raster_count = 0x1fe - raster_used;
		}
		if (raster_count == 0) {
			break;
		}
		rows = raster_count / lineheight;
		frame_columns = frame.rw / 8 + 2;
		columns = (frame_columns < visible_columns) ? frame_columns : visible_columns;
		for (row = 0; row < rows; row++) {
			std::string line;
			UINT column;
			UINT16 previous_hccode = 0;
			BOOL have_previous_hccode = FALSE;
			UINT32 address = frame.rsa + frame.vw * row;

			if ((address >= tvram_size) || ((UINT64)address + columns * 2 > tvram_size)) {
				break;
			}
			for (column = 0; column < columns; column++) {
				BYTE *cell = textmem + address + column * 2;
				UINT16 hccode = LOADINTELWORD(cell);
				BYTE attr = 0;

				if ((UINT64)(cell - textmem) + tsp.attroffset < tvram_size) {
					attr = cell[tsp.attroffset];
				}
				if (have_previous_hccode && (hccode == (previous_hccode | 0x8000))) {
					previous_hccode = hccode;
					continue;
				}
				if ((frame.mode == 1) && (attr & 0x01)) {
					line.push_back(' ');
				} else {
					copy_append_hccode(&line, hccode);
				}
				previous_hccode = hccode;
				have_previous_hccode = TRUE;
			}
			while (!line.empty() && (line.back() == ' ')) {
				line.pop_back();
			}
			lines.push_back(line);
		}
		raster_used += raster_count;
	}
	while (!lines.empty() && lines.back().empty()) {
		lines.pop_back();
	}
	std::string clipboard;
	for (frame_no = 0; frame_no < lines.size(); frame_no++) {
		if (frame_no != 0) {
			clipboard.push_back('\n');
		}
		clipboard += lines[frame_no];
	}
	if (SDL_SetClipboardText(clipboard.c_str()) != 0) {
		g_gui.keyboard_status = "Copy failed: ";
		g_gui.keyboard_status += SDL_GetError();
		return (FAILURE);
	}
	g_gui.keyboard_status = "Copy complete.";
	return (SUCCESS);
}

static BOOL save_screenshot(BOOL with_graphics_analysis) {
	_SYSTIME systime;
	char path[MAX_PATH];
	UINT attempt;
	const UINT32 ticks = SDL_GetTicks();

	if (!g_gui.initialized) {
		return (FAILURE);
	}
	if (timemng_gettime(&systime) != SUCCESS) {
		g_gui.screenshot_status = "スクリーンショットの時刻取得に失敗しました。";
		return (FAILURE);
	}
	for (attempt = 0; attempt < 1000; attempt++) {
		std::snprintf(path, sizeof(path), "vaeg-%04u%02u%02u-%02u%02u%02u-%010u-%03u.png",
		              systime.year, systime.month, systime.day, systime.hour, systime.minute,
		              systime.second, static_cast<unsigned int>(ticks), attempt);
		if (file_attr(path) < 0) {
			break;
		}
	}
	if (attempt == 1000) {
		g_gui.screenshot_status = "スクリーンショット名を確保できません。";
		return (FAILURE);
	}
	if (!with_graphics_analysis) {
		if (scrnmng_request_display_capture(path) != SUCCESS) {
			g_gui.screenshot_status = "Screenshot request failed: ";
			g_gui.screenshot_status += SDL_GetError();
			return FAILURE;
		}
		g_gui.screenshot_status = "Screenshot queued";
		return SUCCESS;
	}
	if (scrnmng_save_guest_frame_with_analysis(path) != SUCCESS) {
		g_gui.screenshot_status = "スクリーンショット保存に失敗しました: ";
		g_gui.screenshot_status += SDL_GetError();
		return (FAILURE);
	}
	g_gui.screenshot_status = "スクリーンショット（加工前）を保存しました: ";
	g_gui.screenshot_status += path;
	std::fprintf(stderr, "screenshot saved path=%s\n", path);
	return (SUCCESS);
}

static void update_text_input_state(void) {
	if (!g_gui.initialized) {
		return;
	}
	ImGuiIO &io = ImGui::GetIO();
	const bool wanted = io.WantTextInput;
	if (wanted == g_gui.text_input_active) {
		return;
	}
	if (wanted) {
		SDL_StartTextInput();
	} else {
		SDL_StopTextInput();
	}
	g_gui.text_input_active = wanted;
}

static SDL_Texture *load_about_texture(SDL_Renderer *renderer, int *width, int *height) {
	SDL_RWops *stream = SDL_RWFromConstMem(vaeg_splash_bmp, static_cast<int>(vaeg_splash_bmp_size));
	if (stream == nullptr) {
		return nullptr;
	}
	SDL_Surface *surface = SDL_LoadBMP_RW(stream, 1);
	if (surface == nullptr) {
		return nullptr;
	}
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (texture != nullptr) {
		*width = surface->w;
		*height = surface->h;
		SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest);
	}
	SDL_FreeSurface(surface);
	return texture;
}

static void load_native_about_texture(void) {
	SDL_RWops *stream = SDL_RWFromConstMem(vaeg_splash_bmp, static_cast<int>(vaeg_splash_bmp_size));
	if (!stream)
		return;
	SDL_Surface *source = SDL_LoadBMP_RW(stream, 1);
	if (!source)
		return;
	SDL_Surface *rgba = SDL_ConvertSurfaceFormat(source, SDL_PIXELFORMAT_RGBA32, 0);
	SDL_FreeSurface(source);
	if (!rgba)
		return;
	auto *texture = IM_NEW(ImTextureData)();
	texture->Create(ImTextureFormat_RGBA32, rgba->w, rgba->h);
	for (int row = 0; row < rgba->h; ++row) {
		std::memcpy(texture->GetPixelsAt(0, row),
		            static_cast<const unsigned char *>(rgba->pixels) + row * rgba->pitch,
		            texture->GetPitch());
	}
	g_gui.about_texture_width = rgba->w;
	g_gui.about_texture_height = rgba->h;
	SDL_FreeSurface(rgba);
	ImGui::RegisterUserTexture(texture);
	g_gui.native_about_texture = texture;
}

static std::string join_path(const std::string &base, const char *leaf) {
	if (base.empty()) {
		return std::string(leaf);
	}
	if ((base.back() == '/') || (base.back() == '\\')) {
		return base + leaf;
	}
	return base + "/" + leaf;
}

static const char *new_sasi_default_name(void) {
	return "new-sasi-hdd.hdd";
}

static std::string new_scsi_default_name(int drive) {
	std::string name = "new-scsi-hdd_id";

	name += std::to_string(std::clamp(drive, 0, SCSIHDD_MAX - 1));
	name += ".hdi";
	return name;
}

static std::string new_scsi_default_path(const std::string &directory, int drive) {
	std::string name = new_scsi_default_name(drive);

	return join_path(directory, name.c_str());
}

static void menu_item_not_implemented(const char *label) {
	ImGui::BeginDisabled();
	ImGui::MenuItem(label);
	ImGui::EndDisabled();
}

static void open_configure_dialog(void) {
	g_gui.pending_cpu_multiplier = static_cast<int>(np2cfg.multiple);
	g_gui.pending_sgp_mode = static_cast<int>(np2cfg.sgp_speed_mode);
	g_gui.pending_sgp_multiplier = static_cast<int>(np2cfg.sgp_multiplier);
	g_gui.pending_ndp8087_enabled = np2cfg.upd8087_enable != 0;
	g_gui.pending_ndp8087_clock_hz = static_cast<int>(np2cfg.upd8087_clock_hz);
	g_gui.pending_pacing_ms = static_cast<int>(np2oscfg.pacing_ms);
	g_gui.pending_hostfat_enabled = np2oscfg.hostfat_enabled != 0;
	milstr_ncpy(g_gui.pending_hostfat_dir, np2oscfg.hostfat_dir, sizeof(g_gui.pending_hostfat_dir));
	g_gui.pending_hostfat_rebuild = false;
	g_gui.configure_open = true;
	g_gui.configure_request = true;
}

static void draw_multiplier_input(const char *label, int *value, const int *presets,
                                  int preset_count) {
	ImGui::PushID(label);
	ImGui::TextUnformatted(label);
	ImGui::SameLine(145.0f);
	ImGui::SetNextItemWidth(80.0f);
	ImGui::InputInt("##value", value, 1, 0);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(110.0f);
	if (ImGui::BeginCombo("##presets", "Preset")) {
		for (int i = 0; i < preset_count; i++) {
			char item[16];

			std::snprintf(item, sizeof(item), "x%d", presets[i]);
			if (ImGui::Selectable(item, *value == presets[i])) {
				*value = presets[i];
			}
		}
		ImGui::EndCombo();
	}
	ImGui::PopID();
}

static void draw_ndp8087_clock_input(int *value) {
	ImGui::PushID("NDP8087 clock");
	ImGui::TextUnformatted("Clock (Hz)");
	ImGui::SameLine(145.0f);
	ImGui::SetNextItemWidth(125.0f);
	ImGui::InputInt("##value", value, 100000, 1000000);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(110.0f);
	if (ImGui::BeginCombo("##presets", "Preset")) {
		for (int i = 0; i < static_cast<int>(std::size(kNdp8087Presets)); i++) {
			char item[32];
			std::snprintf(item, sizeof(item), "%d MHz", kNdp8087Presets[i] / 1000000);
			if (ImGui::Selectable(item, *value == kNdp8087Presets[i])) {
				*value = kNdp8087Presets[i];
			}
		}
		ImGui::EndCombo();
	}
	ImGui::PopID();
}

static void apply_configure_dialog(void) {
	const bool clock_changed =
	    (np2cfg.baseclock != PCBASECLOCK40) ||
	    (np2cfg.multiple != static_cast<UINT>(g_gui.pending_cpu_multiplier)) ||
	    (np2cfg.sgp_speed_mode != static_cast<UINT8>(g_gui.pending_sgp_mode)) ||
	    (np2cfg.sgp_multiplier != static_cast<UINT8>(g_gui.pending_sgp_multiplier)) ||
	    (np2oscfg.pacing_ms != static_cast<UINT16>(g_gui.pending_pacing_ms));
	const bool ndp8087_changed =
	    ((np2cfg.upd8087_enable != 0) != g_gui.pending_ndp8087_enabled) ||
	    (np2cfg.upd8087_clock_hz != static_cast<UINT32>(g_gui.pending_ndp8087_clock_hz));
	const bool hostfat_changed =
	    ((np2oscfg.hostfat_enabled != 0) != g_gui.pending_hostfat_enabled) ||
	    (std::strcmp(np2oscfg.hostfat_dir, g_gui.pending_hostfat_dir) != 0);
	bool reset_done = false;

	if (clock_changed) {
		np2cfg.baseclock = PCBASECLOCK40;
		np2cfg.multiple = static_cast<UINT>(g_gui.pending_cpu_multiplier);
		np2cfg.sgp_speed_mode = static_cast<UINT8>(g_gui.pending_sgp_mode);
		np2cfg.sgp_multiplier = static_cast<UINT8>(g_gui.pending_sgp_multiplier);
		np2oscfg.pacing_ms = static_cast<UINT16>(g_gui.pending_pacing_ms);
		sysmng_update(SYS_UPDATECFG | SYS_UPDATEOSCFG | SYS_UPDATECLOCK);
		reset_guest();
		reset_done = true;
	}
	if (ndp8087_changed) {
		np2cfg.upd8087_enable = g_gui.pending_ndp8087_enabled ? 1 : 0;
		np2cfg.upd8087_clock_hz = static_cast<UINT32>(g_gui.pending_ndp8087_clock_hz);
		sysmng_update(SYS_UPDATECFG | SYS_UPDATECLOCK);
		if (!reset_done) {
			reset_guest();
			reset_done = true;
		}
	}
	if (hostfat_changed || g_gui.pending_hostfat_rebuild) {
		char error[256]{};
		if (g_gui.pending_hostfat_enabled) {
			const std::string hostfat_dir = vaeg_hostfat::normalize_path(g_gui.pending_hostfat_dir);
			if (hostfat_manager_rebuild_async(hostfat_dir.c_str(), error, sizeof(error)) ==
			    SUCCESS) {
				g_gui.hostfat_rebuild_dir = hostfat_dir;
				g_gui.hostfat_status = "Building immutable HOSTFAT snapshot...";
				g_gui.hostfat_reset_after_build = true;
			} else {
				g_gui.hostfat_status = "HOSTFAT rebuild failed to start: ";
				g_gui.hostfat_status += error;
				g_gui.hostfat_error_message = g_gui.hostfat_status;
				g_gui.hostfat_error_open = true;
				g_gui.hostfat_error_request = true;
			}
		} else if (hostfat_manager_unmount(error, sizeof(error)) == SUCCESS) {
			np2oscfg.hostfat_enabled = 0;
			np2oscfg.hostfat_dir[0] = '\0';
			sysmng_update(SYS_UPDATEOSCFG);
			g_gui.hostfat_status = "HOSTFAT unmounted.";
			if (!reset_done) {
				reset_guest();
			}
		} else {
			g_gui.hostfat_status = "HOSTFAT unmount failed: ";
			g_gui.hostfat_status += error;
			g_gui.hostfat_error_message = g_gui.hostfat_status;
			g_gui.hostfat_error_open = true;
			g_gui.hostfat_error_request = true;
		}
	}
	g_gui.pending_hostfat_rebuild = false;
	g_gui.configure_open = false;
	ImGui::CloseCurrentPopup();
}

static void draw_configure_dialog(void) {
	if (g_gui.configure_request) {
		ImGui::OpenPopup("Configure##clock-config");
		g_gui.configure_request = false;
	}
	if (!g_gui.configure_open) {
		return;
	}
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(560.0f, 800.0f), ImGuiCond_Appearing);
	if (ImGui::BeginPopupModal("Configure##clock-config", &g_gui.configure_open,
	                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
		const bool cpu_valid =
		    pccore_cpu_multiple_valid(static_cast<UINT>(g_gui.pending_cpu_multiplier));
		const bool sgp_mode_valid = sgp_speed_mode_valid(static_cast<UINT>(g_gui.pending_sgp_mode));
		const bool sgp_multiplier_valid =
		    (g_gui.pending_sgp_mode != SGP_SPEED_CUSTOM) ||
		    sgp_speed_multiplier_valid(static_cast<UINT>(g_gui.pending_sgp_multiplier));
		const bool ndp8087_clock_valid =
		    (g_gui.pending_ndp8087_clock_hz >= static_cast<int>(UPD8087_MIN_CLOCK_HZ)) &&
		    (g_gui.pending_ndp8087_clock_hz <= static_cast<int>(UPD8087_MAX_CLOCK_HZ)) &&
		    upd8087_clock_valid(static_cast<UINT32>(g_gui.pending_ndp8087_clock_hz));
		const bool hostfat_valid =
		    !g_gui.pending_hostfat_enabled || is_directory(g_gui.pending_hostfat_dir);
		HOSTFAT_MANAGER_STATUS hostfat_manager_status{};
		hostfat_manager_get_status(&hostfat_manager_status);
		const bool hostfat_idle = hostfat_manager_status.state != HOSTFAT_MANAGER_BUILDING;
		const bool valid =
		    cpu_valid && sgp_mode_valid && sgp_multiplier_valid && ndp8087_clock_valid &&
		    hostfat_valid && hostfat_idle;
		const bool hostfat_action_error =
		    (g_gui.hostfat_status.rfind("HOSTFAT rebuild failed", 0) == 0) ||
		    (g_gui.hostfat_status.rfind("HOSTFAT unmount failed", 0) == 0);

		if (ImGui::BeginChild("cpu-config", ImVec2(0.0f, 145.0f), true,
		                      ImGuiWindowFlags_NoScrollbar)) {
			ImGui::TextUnformatted("CPU");
			ImGui::Separator();
			ImGui::BeginDisabled();
			if (ImGui::BeginCombo("Base clock", "3.9936 MHz")) {
				ImGui::Selectable("3.9936 MHz", true);
				ImGui::EndCombo();
			}
			ImGui::EndDisabled();
			draw_multiplier_input("Multiplier", &g_gui.pending_cpu_multiplier, kCpuPresets,
			                      static_cast<int>(std::size(kCpuPresets)));
			ImGui::Text("Effective CPU clock: %.4f MHz",
			            3.9936 * static_cast<double>(g_gui.pending_cpu_multiplier));
			ImGui::TextUnformatted("Standard setting: x2 (7.9872 MHz)");
		}
		ImGui::EndChild();

		if (ImGui::BeginChild("ndp8087-config", ImVec2(0.0f, 155.0f), true,
		                      ImGuiWindowFlags_NoScrollbar)) {
			const bool requested_changed =
			    ((np2cfg.upd8087_enable != 0) != g_gui.pending_ndp8087_enabled) ||
			    (np2cfg.upd8087_clock_hz != static_cast<UINT32>(g_gui.pending_ndp8087_clock_hz));
			ImGui::TextUnformatted("Intel 8087 (NDP)");
			ImGui::Separator();
			ImGui::Checkbox("Enable 8087", &g_gui.pending_ndp8087_enabled);
			draw_ndp8087_clock_input(&g_gui.pending_ndp8087_clock_hz);
			ImGui::Text("Active: %s, %u Hz", upd8087.enabled ? "enabled" : "disabled",
			            static_cast<unsigned int>(upd8087.clock_hz));
			if (requested_changed) {
				ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f),
				                   "Requested values apply on guest reset.");
			} else {
				ImGui::TextDisabled("Requested values are active.");
			}
			ImGui::TextDisabled("One 8087 maximum; FPO2 is never connected.");
		}
		ImGui::EndChild();

		if (ImGui::BeginChild("sgp-config", ImVec2(0.0f, 165.0f), true,
		                      ImGuiWindowFlags_NoScrollbar)) {
			static const char *modes[] = {"Model default", "Follow CPU", "Custom"};
			const UINT model = (milstr_cmp(np2cfg.model, str_VA1) == 0) ? PCMODEL_VA1 : PCMODEL_VA2;
			const double model_clock_mhz = static_cast<double>(sgp_model_clock(model)) / 1000000.0;
			double effective_scale = 1.0;
			ImGui::TextUnformatted("SGP");
			ImGui::Separator();
			ImGui::Combo("Speed", &g_gui.pending_sgp_mode, modes,
			             static_cast<int>(std::size(modes)));
			if (g_gui.pending_sgp_mode == SGP_SPEED_CUSTOM) {
				draw_multiplier_input("Custom multiplier", &g_gui.pending_sgp_multiplier,
				                      kSgpPresets, static_cast<int>(std::size(kSgpPresets)));
			}
			if (g_gui.pending_sgp_mode == SGP_SPEED_MODEL_DEFAULT) {
				ImGui::TextUnformatted("Effective SGP scale: Model default (x1.0000)");
			} else if (g_gui.pending_sgp_mode == SGP_SPEED_FOLLOW_CPU) {
				ImGui::Text("Effective SGP scale: x%.4f relative to Model default",
				            static_cast<double>(g_gui.pending_cpu_multiplier) /
				                PCCORE_STANDARD_MULTIPLE);
			} else if (g_gui.pending_sgp_mode == SGP_SPEED_CUSTOM) {
				ImGui::Text("Effective SGP scale: x%d relative to Model default",
				            g_gui.pending_sgp_multiplier);
			}
			if (g_gui.pending_sgp_mode == SGP_SPEED_FOLLOW_CPU) {
				effective_scale =
				    static_cast<double>(g_gui.pending_cpu_multiplier) / PCCORE_STANDARD_MULTIPLE;
			} else if (g_gui.pending_sgp_mode == SGP_SPEED_CUSTOM) {
				effective_scale = g_gui.pending_sgp_multiplier;
			}
			ImGui::Text("Effective SGP clock: %.4f MHz", model_clock_mhz * effective_scale);
		}
		ImGui::EndChild();
		ImGui::Text("Host pacing delay per loop (ms)");
		ImGui::SetNextItemWidth(120.0f);
		ImGui::InputInt("##pacing-ms", &g_gui.pending_pacing_ms, 1, 8);
		if (g_gui.pending_pacing_ms < 0)
			g_gui.pending_pacing_ms = 0;
		if (g_gui.pending_pacing_ms > VAEG_PACING_MS_MAX) {
			g_gui.pending_pacing_ms = VAEG_PACING_MS_MAX;
		}

		if (ImGui::BeginChild("hostfat-config", ImVec2(0.0f, 175.0f), true,
		                      ImGuiWindowFlags_NoScrollbar)) {
			ImGui::TextUnformatted("HOSTFAT read-only host folder");
			ImGui::Separator();
			ImGui::Checkbox("Enable HOSTFAT", &g_gui.pending_hostfat_enabled);
			ImGui::SetNextItemWidth(-92.0f);
			ImGui::InputText("##hostfat-dir", g_gui.pending_hostfat_dir,
			                 sizeof(g_gui.pending_hostfat_dir));
			ImGui::SameLine();
			if (ImGui::Button("Browse...", ImVec2(84.0f, 0.0f))) {
				open_hostfat_browser();
			}
			if (ImGui::Button("Rebuild + reset on OK")) {
				g_gui.pending_hostfat_rebuild = true;
			}
			ImGui::SameLine();
			ImGui::TextDisabled("FAT12 max: 63.72 MiB usable");
			if (hostfat_action_error) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.25f, 0.25f, 1.0f));
				ImGui::TextWrapped("%s", g_gui.hostfat_status.c_str());
				ImGui::PopStyleColor();
			}
			if (hostfat_manager_status.state == HOSTFAT_MANAGER_BUILDING) {
				const float fraction =
				    (hostfat_manager_status.total != 0)
				        ? static_cast<float>(static_cast<double>(hostfat_manager_status.completed) /
				                             static_cast<double>(hostfat_manager_status.total))
				        : 0.0f;
				ImGui::ProgressBar(std::clamp(fraction, 0.0f, 1.0f), ImVec2(-1.0f, 0.0f),
				                   hostfat_manager_status.phase);
			} else if (!hostfat_action_error && hostfat_manager_status.message[0] != '\0') {
				if (hostfat_manager_status.state == HOSTFAT_MANAGER_ERROR) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.25f, 0.25f, 1.0f));
					ImGui::TextWrapped("Error: %s", hostfat_manager_status.message);
					ImGui::PopStyleColor();
				} else {
					ImGui::TextWrapped("%s", hostfat_manager_status.message);
				}
			} else {
				ImGui::TextDisabled("%s", hostfat_manager_status.phase);
			}
		}
		ImGui::EndChild();

		if (!cpu_valid) {
			ImGui::TextUnformatted("Multiplier must be between 1 and 32.");
		} else if (!sgp_mode_valid) {
			ImGui::TextUnformatted("Select a valid SGP speed mode.");
		} else if (!sgp_multiplier_valid) {
			ImGui::TextUnformatted("SGP multiplier must be between 1 and 16.");
		} else if (!ndp8087_clock_valid) {
			ImGui::TextUnformatted("8087 clock must be between 1000000 and 20000000 Hz.");
		} else if (!hostfat_valid) {
			ImGui::TextUnformatted("Select an existing HOSTFAT directory.");
		} else if (!hostfat_idle) {
			ImGui::TextUnformatted("Wait for the current HOSTFAT build to finish.");
		}

		const float button_width = 88.0f;
		ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - (button_width * 2.0f + 8.0f));
		ImGui::BeginDisabled(!valid);
		if (ImGui::Button("OK", ImVec2(button_width, 0.0f))) {
			apply_configure_dialog();
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(button_width, 0.0f)) ||
		    ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			g_gui.configure_open = false;
			ImGui::CloseCurrentPopup();
		}
		draw_hostfat_browser_popup();
		ImGui::EndPopup();
	}
}

static void open_bms_config_dialog(void) {
	g_gui.pending_bms_enabled = bmsiocfg.enabled != FALSE;
	g_gui.pending_bms_port = (bmsiocfg.port == BMSIO_PORT_COMPAT) ? 1 : 0;
	g_gui.pending_bms_banks = bmsiocfg.numbanks;
	g_gui.bms_config_open = true;
	g_gui.bms_config_request = true;
}

static void apply_bms_config_dialog(void) {
	const UINT16 port = (g_gui.pending_bms_port == 1) ? BMSIO_PORT_COMPAT : BMSIO_PORT_DEFAULT;
	const BOOL enabled = g_gui.pending_bms_enabled ? TRUE : FALSE;
	const UINT8 banks = static_cast<UINT8>(g_gui.pending_bms_banks);
	const bool changed = (bmsiocfg.enabled != enabled) || (bmsiocfg.port != port) ||
	                     (bmsiocfg.numbanks != banks) || (bmsiocfg.portmask != BMSIO_PORT_MASK);

	if (changed) {
		bmsiocfg.enabled = enabled;
		bmsiocfg.port = port;
		bmsiocfg.portmask = BMSIO_PORT_MASK;
		bmsiocfg.numbanks = banks;
		sysmng_update(SYS_UPDATECFG);
		reset_guest();
	}
	g_gui.bms_config_open = false;
	ImGui::CloseCurrentPopup();
}

static void draw_bms_config_dialog(void) {
	if (g_gui.bms_config_request) {
		ImGui::OpenPopup("I/O Bank Memory##bms-config");
		g_gui.bms_config_request = false;
	}
	if (!g_gui.bms_config_open) {
		return;
	}
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(430.0f, 285.0f), ImGuiCond_Appearing);
	if (ImGui::BeginPopupModal("I/O Bank Memory##bms-config", &g_gui.bms_config_open,
	                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
		static const char *ports[] = {"01D0H (PC-88VA-01/02 mode)", "00ECH (PC-9801 mode)"};
		const bool banks_valid =
		    (g_gui.pending_bms_banks >= 1) && (g_gui.pending_bms_banks <= BMSIO_MAX_BANKS);
		const bool port_valid = (g_gui.pending_bms_port >= 0) &&
		                        (g_gui.pending_bms_port < static_cast<int>(std::size(ports)));

		ImGui::Checkbox("Use I/O Bank Memory", &g_gui.pending_bms_enabled);
		ImGui::SetNextItemWidth(220.0f);
		ImGui::Combo("I/O port", &g_gui.pending_bms_port, ports,
		             static_cast<int>(std::size(ports)));
		ImGui::SetNextItemWidth(120.0f);
		ImGui::InputInt("128KB banks", &g_gui.pending_bms_banks, 1, 16);
		if (banks_valid) {
			ImGui::Text("Capacity: %dKB (%.3fMiB)",
			            g_gui.pending_bms_banks * (BMSIO_BANK_BYTES / 1024),
			            static_cast<double>(g_gui.pending_bms_banks) / 8.0);
		} else {
			ImGui::TextUnformatted("Bank count must be between 1 and 255.");
		}
		ImGui::Separator();
		ImGui::TextWrapped("Selector 0 restores main RAM at 80000H-9FFFFH; "
		                   "selectors 1 through the configured bank count choose BMS storage.");
		ImGui::TextWrapped("Applying a change resets the guest. Disabling BMS or "
		                   "changing its bank count discards current BMS contents.");

		const float button_width = 88.0f;
		ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - (button_width * 2.0f + 8.0f));
		ImGui::BeginDisabled(!banks_valid || !port_valid);
		if (ImGui::Button("OK", ImVec2(button_width, 0.0f))) {
			apply_bms_config_dialog();
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(button_width, 0.0f)) ||
		    ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			g_gui.bms_config_open = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

static void save_main_ram_settings(UINT16 capacity, bool automatic) {
	const UINT16 previous_capacity = np2cfg.main_ram;
	const UINT8 previous_auto = np2cfg.main_ram_auto;
	np2cfg.main_ram = capacity;
	np2cfg.main_ram_auto = automatic ? 1 : 0;
	if (initsave_checked() != SUCCESS) {
		np2cfg.main_ram = previous_capacity;
		np2cfg.main_ram_auto = previous_auto;
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Configuration not saved",
		                        "Could not save the active configuration file. Check write "
		                        "permissions and whether --no-cfg is enabled. Settings were not changed.",
		                        nullptr);
		return;
	}
	sysmng_update(SYS_UPDATECFG);
}

static void open_ems_config_dialog(void) {
	g_gui.pending_ems_enabled = np2cfg.EXTMEM != 0;
	g_gui.pending_ems_megabytes = (np2cfg.EXTMEM != 0) ? np2cfg.EXTMEM : EMSIO_DEFAULT_MEGABYTES;
	g_gui.ems_config_open = true;
	g_gui.ems_config_request = true;
}

static void apply_ems_config_dialog(void) {
	const UINT8 megabytes =
	    g_gui.pending_ems_enabled ? static_cast<UINT8>(g_gui.pending_ems_megabytes) : 0;
	const bool changed = np2cfg.EXTMEM != megabytes;

	if (changed) {
		np2cfg.EXTMEM = megabytes;
		sysmng_update(SYS_UPDATECFG);
		reset_guest();
	}
	g_gui.ems_config_open = false;
	ImGui::CloseCurrentPopup();
}

static void draw_ems_config_dialog(void) {
	if (g_gui.ems_config_request) {
		ImGui::OpenPopup("EMS Board##ems-config");
		g_gui.ems_config_request = false;
	}
	if (!g_gui.ems_config_open) {
		return;
	}
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(430.0f, 250.0f), ImGuiCond_Appearing);
	if (ImGui::BeginPopupModal("EMS Board##ems-config", &g_gui.ems_config_open,
	                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
		const bool capacity_valid =
		    !g_gui.pending_ems_enabled || ((g_gui.pending_ems_megabytes >= EMSIO_MIN_MEGABYTES) &&
		                                   (g_gui.pending_ems_megabytes <= EMSIO_MAX_MEGABYTES));

		ImGui::Checkbox("Use EMS Board", &g_gui.pending_ems_enabled);
		ImGui::BeginDisabled(!g_gui.pending_ems_enabled);
		ImGui::SetNextItemWidth(120.0f);
		ImGui::InputInt("Installed memory (MB)", &g_gui.pending_ems_megabytes, 1, 1);
		ImGui::EndDisabled();
		if (capacity_valid && g_gui.pending_ems_enabled) {
			ImGui::Text("Capacity: %dMB", g_gui.pending_ems_megabytes);
		} else if (!capacity_valid) {
			ImGui::Text("Capacity must be between %dMB and %dMB.", EMSIO_MIN_MEGABYTES,
			            EMSIO_MAX_MEGABYTES);
		} else {
			ImGui::TextUnformatted("Capacity: disabled");
		}
		ImGui::Separator();
		ImGui::TextWrapped("Applying a change resets the guest and discards "
		                   "current EMS contents. EMMVA also requires a compatible guest EMM "
		                   "manager.");

		const float button_width = 88.0f;
		ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - (button_width * 2.0f + 8.0f));
		ImGui::BeginDisabled(!capacity_valid);
		if (ImGui::Button("OK", ImVec2(button_width, 0.0f))) {
			apply_ems_config_dialog();
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(button_width, 0.0f)) ||
		    ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			g_gui.ems_config_open = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

static UINT8 scale_master_volume(int volume, int max_value) {
	volume = std::clamp(volume, 0, kMasterVolumeMax);
	return static_cast<UINT8>((volume * max_value + kMasterVolumeMax / 2) / kMasterVolumeMax);
}

static void apply_master_volume(int volume) {
	const UINT8 mixer_volume = scale_master_volume(volume, 128);
	const UINT8 motor_volume = scale_master_volume(volume, 100);

	np2cfg.vol_fm = mixer_volume;
	np2cfg.vol_ssg = mixer_volume;
	np2cfg.vol_adpcm = mixer_volume;
	np2cfg.vol_rhythm = mixer_volume;
	opngen_setvol(np2cfg.vol_fm);
	psggen_setvol(np2cfg.vol_ssg);
	rhythm_setvol(np2cfg.vol_rhythm);
	rhythm_update(&rhythm);
	adpcm_setvol(np2cfg.vol_adpcm);
	adpcm_update(&adpcm);

	// The buzzer follows the master volume finely; BEEP_vol (0-3) keeps a
	// value that is not 0 unless the master volume is 0, for older builds.
	np2cfg.beep_level = static_cast<UINT8>(std::clamp(volume, 0, kMasterVolumeMax));
	np2cfg.BEEP_VOL =
	    static_cast<UINT8>((np2cfg.beep_level * 3 + kMasterVolumeMax - 1) / kMasterVolumeMax);
	beep_setlevel(np2cfg.beep_level);

	np2cfg.MOTORVOL = motor_volume;
	fddmtrsnd_volume(np2cfg.MOTORVOL);
	sysmng_update(SYS_UPDATECFG);
}

static void select_opn_backend(UINT backend) {
	if (opngen_getbackend() == backend) {
		return;
	}
	opngen_setbackend(backend);
	milstr_ncpy(np2oscfg.opn_backend, opngen_backendname(backend), sizeof(np2oscfg.opn_backend));
	soundrenewal = 1;
	sysmng_update(SYS_UPDATEOSCFG);
	reset_guest();
}

static void select_ymfm_fidelity(UINT fidelity) {
	if ((fidelity >= YMFMBRIDGE_FIDELITY_COUNT) || (ymfm_opn_getfidelity() == fidelity)) {
		return;
	}
	soundmng_stop();
	ymfm_opn_setfidelity(fidelity);
	milstr_ncpy(np2oscfg.ymfm_fidelity, ymfm_opn_fidelityname(fidelity),
	            sizeof(np2oscfg.ymfm_fidelity));
	soundrenewal = 1;
	sysmng_update(SYS_UPDATEOSCFG);
	reset_guest();
}

static void select_sampling_rate(UINT rate) {
	if (!vaeg_sound_rate_valid(rate) || (np2cfg.samplingrate == rate)) {
		return;
	}
	np2cfg.samplingrate = static_cast<UINT16>(rate);
	soundrenewal = 1;
	sysmng_update(SYS_UPDATECFG | SYS_UPDATERATE);
	reset_guest();
}

static void select_sound_buffer(UINT delayms) {
	if (!vaeg_sound_buffer_valid(delayms) || (np2cfg.delayms == delayms)) {
		return;
	}
	np2cfg.delayms = static_cast<UINT16>(delayms);
	soundrenewal = 1;
	sysmng_update(SYS_UPDATECFG | SYS_UPDATESBUF);
	reset_guest();
}

static void select_boot_model(const char *model, bool enable_8087) {
	const UINT16 old_sound = np2cfg.SOUND_SW;
	const bool old_8087 = np2cfg.upd8087_enable != 0;

	np2_select_boot_model(model);
	np2cfg.upd8087_enable = enable_8087 ? 1 : 0;
	if (np2cfg.SOUND_SW != old_sound) {
		soundrenewal = 1;
	}
	sysmng_update(SYS_UPDATECFG | ((old_8087 != enable_8087) ? SYS_UPDATECLOCK : 0));
	reset_guest();
}

/*
 * V1/V2 ("Z80") mode: V1/V2 is the VA's memory switch B1FC5h bit 0, as its
 * setup menu stores it; H/S is a vaeg extension (see memctrlva_i031). Both
 * take effect at reset, as on the machine.
 */
static void select_v1v2_mode(bool v1, bool standard) {
	bkupmemva_set_88v1(v1 ? TRUE : FALSE);
	np2cfg.v1v2_standard = standard ? 1 : 0;
	np2cfg.v1v2_nmode = 0;
	sysmng_update(SYS_UPDATECFG);
	reset_guest();
}

static void select_monitor(bool khz15) {
	if ((np2cfg.monitor_15khz != 0) == khz15) {
		return;
	}
	np2cfg.monitor_15khz = khz15 ? 1 : 0;
	sysmng_update(SYS_UPDATECFG);
	reset_guest();
}

/* N-BASIC ROM files found when the Z80 mode menu was last opened. */
static N80ROMENTRY g_n80_entries[ROMVA_N80_NAMES];
static bool g_n80_scanned = false;

static void select_sound_hardware(UINT16 sound) {
	if (np2cfg.SOUND_SW == sound) {
		return;
	}
	np2cfg.SOUND_SW = sound;
	soundrenewal = 1;
	sysmng_update(SYS_UPDATECFG | SYS_UPDATESBOARD);
	reset_guest();
}

static int menu_bar_height(void) {
	const ImGuiStyle &style = ImGui::GetStyle();
	return static_cast<int>(std::ceil(g_gui.menu_font_size + (style.FramePadding.y * 2.0f)));
}

static std::string home_dir(void) {
	const char *home;

#if defined(WIN32)
	home = std::getenv("USERPROFILE");
	if ((home == nullptr) || (home[0] == '\0')) {
		home = std::getenv("HOME");
	}
	if ((home == nullptr) || (home[0] == '\0')) {
		const char *drive = std::getenv("HOMEDRIVE");
		const char *path = std::getenv("HOMEPATH");
		if ((drive != nullptr) && (drive[0] != '\0') && (path != nullptr) && (path[0] != '\0')) {
			return std::string(drive) + path;
		}
	}
#else
	home = std::getenv("HOME");
#endif
	if ((home != nullptr) && (home[0] != '\0')) {
		return home;
	}
	return ".";
}

static bool is_directory(const std::string &path) {
	std::error_code ec;
	return fs::is_directory(vaeg_hostfat::path_from_utf8(path), ec);
}

static std::string absolute_path(const std::string &path) {
	std::error_code ec;
	fs::path abs = fs::absolute(vaeg_hostfat::path_from_utf8(path), ec);
	if (ec) {
		return path;
	}
	return abs.u8string();
}

static std::string parent_dir(const std::string &path) {
	std::error_code ec;
	fs::path p = fs::absolute(vaeg_hostfat::path_from_utf8(path), ec);
	if (ec) {
		p = vaeg_hostfat::path_from_utf8(path);
	}
	p = p.parent_path();
	if (p.empty()) {
		return home_dir();
	}
	return p.u8string();
}

static void copy_path(char *dst, size_t dst_size, const std::string &src) {
	milstr_ncpy(dst, src.c_str(), static_cast<int>(dst_size));
}

static std::vector<std::string> host_drive_roots(void) {
	std::vector<std::string> roots;

#if defined(_WIN32)
	for (char letter = 'A'; letter <= 'Z'; letter++) {
		std::string root;

		root += letter;
		root += ":\\";
		if (is_directory(root)) {
			roots.push_back(root);
		}
	}
#endif
	return roots;
}

static bool drive_root_matches(const std::string &path, const std::string &root) {
	if ((path.size() < 2) || (root.size() < 2) || (path[1] != ':') || (root[1] != ':')) {
		return false;
	}
	return static_cast<char>(std::toupper(static_cast<unsigned char>(path[0]))) ==
	       static_cast<char>(std::toupper(static_cast<unsigned char>(root[0])));
}

static bool draw_host_drive_selector(std::string &directory, bool &refresh, const char *id) {
	const std::vector<std::string> roots = host_drive_roots();
	std::string preview = "Current";
	bool changed = false;

	if (roots.empty()) {
		return false;
	}
	for (const std::string &root : roots) {
		if (drive_root_matches(directory, root)) {
			preview = root.substr(0, 2);
			break;
		}
	}
	std::string combo_id = "Drive##";
	combo_id += id;
	ImGui::SetNextItemWidth(110.0f);
	if (ImGui::BeginCombo(combo_id.c_str(), preview.c_str())) {
		for (const std::string &root : roots) {
			const std::string label = root.substr(0, 2);
			const bool selected = (label == preview);

			if (ImGui::Selectable(label.c_str(), selected)) {
				directory = absolute_path(root);
				refresh = true;
				changed = true;
			}
			if (selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Select a host drive");
	}
	return changed;
}

static bool browser_entry_less(const BrowserEntry &a, const BrowserEntry &b) {
	if (a.is_dir != b.is_dir) {
		return a.is_dir > b.is_dir;
	}
	return a.name < b.name;
}

static void refresh_fdd_browser(void) {
	std::error_code ec;

	g_gui.fdd_entries.clear();
	if (!is_directory(g_gui.fdd_browser_dir)) {
		g_gui.fdd_browser_dir = home_dir();
	}
	for (const auto &entry : fs::directory_iterator(fs::u8path(g_gui.fdd_browser_dir), ec)) {
		BrowserEntry item;
		std::error_code st_ec;

		if (ec) {
			break;
		}
		item.is_dir = entry.is_directory(st_ec);
		if ((!item.is_dir) && (!entry.is_regular_file(st_ec))) {
			continue;
		}
		item.name = entry.path().filename().u8string();
		item.path = entry.path().u8string();
		if (item.name.empty() || (item.name[0] == '.')) {
			continue;
		}
		g_gui.fdd_entries.push_back(item);
	}
	std::sort(g_gui.fdd_entries.begin(), g_gui.fdd_entries.end(), browser_entry_less);
	g_gui.fdd_browser_refresh = false;
}

static void refresh_hdd_browser(void) {
	std::error_code ec;

	g_gui.hdd_entries.clear();
	if (!is_directory(g_gui.hdd_browser_dir)) {
		g_gui.hdd_browser_dir = home_dir();
	}
	for (const auto &entry : fs::directory_iterator(fs::u8path(g_gui.hdd_browser_dir), ec)) {
		BrowserEntry item;
		std::error_code st_ec;

		if (ec) {
			break;
		}
		item.is_dir = entry.is_directory(st_ec);
		if ((!item.is_dir) && (!entry.is_regular_file(st_ec))) {
			continue;
		}
		item.name = entry.path().filename().u8string();
		item.path = entry.path().u8string();
		if (item.name.empty() || (item.name[0] == '.')) {
			continue;
		}
		g_gui.hdd_entries.push_back(item);
	}
	std::sort(g_gui.hdd_entries.begin(), g_gui.hdd_entries.end(), browser_entry_less);
	g_gui.hdd_browser_refresh = false;
}

static void refresh_hostfat_browser(void) {
	std::error_code ec;
	g_gui.hostfat_entries.clear();
	if (!is_directory(g_gui.hostfat_browser_dir)) {
		g_gui.hostfat_browser_dir = home_dir();
	}
	for (const auto &entry :
	     fs::directory_iterator(vaeg_hostfat::path_from_utf8(g_gui.hostfat_browser_dir), ec)) {
		if (ec) {
			break;
		}
		std::error_code status_error;
		const bool is_dir = entry.is_directory(status_error);
		if (status_error || !is_dir) {
			continue;
		}
		BrowserEntry item;
		item.is_dir = true;
		item.name = entry.path().filename().u8string();
		item.path = entry.path().u8string();
		if (item.name.empty() || (item.name[0] == '.')) {
			continue;
		}
		g_gui.hostfat_entries.push_back(std::move(item));
	}
	std::sort(g_gui.hostfat_entries.begin(), g_gui.hostfat_entries.end(), browser_entry_less);
	g_gui.hostfat_browser_refresh = false;
}

static void open_hostfat_browser(void) {
	std::string start = g_gui.pending_hostfat_dir;
	if (!is_directory(start)) {
		start = home_dir();
	}
	g_gui.hostfat_browser_dir = absolute_path(start);
	g_gui.hostfat_browser_open = true;
	g_gui.hostfat_browser_request = true;
	g_gui.hostfat_browser_refresh = true;
}

static void draw_hostfat_browser_popup(void) {
	if (g_gui.hostfat_browser_request) {
		ImGui::OpenPopup("Select HOSTFAT folder##hostfat-browser");
		g_gui.hostfat_browser_request = false;
	}
	if (!g_gui.hostfat_browser_open) {
		return;
	}
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(620.0f, 480.0f), ImGuiCond_Appearing);
	if (ImGui::BeginPopupModal("Select HOSTFAT folder##hostfat-browser",
	                           &g_gui.hostfat_browser_open, ImGuiWindowFlags_NoCollapse)) {
		if (g_gui.hostfat_browser_refresh) {
			refresh_hostfat_browser();
		}
		draw_host_drive_selector(g_gui.hostfat_browser_dir, g_gui.hostfat_browser_refresh,
		                         "hostfat");
		ImGui::Text("Target Dir");
		ImGui::TextWrapped("%s", g_gui.hostfat_browser_dir.c_str());
		if (ImGui::Button("Up")) {
			g_gui.hostfat_browser_dir = parent_dir(g_gui.hostfat_browser_dir);
			g_gui.hostfat_browser_refresh = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Refresh")) {
			g_gui.hostfat_browser_refresh = true;
		}
		if (ImGui::BeginChild("hostfat-directory-list", ImVec2(0, 330.0f),
		                      ImGuiChildFlags_Borders)) {
			for (const BrowserEntry &entry : g_gui.hostfat_entries) {
				const std::string label = "[D] " + entry.name;
				if (ImGui::Selectable(label.c_str())) {
					g_gui.hostfat_browser_dir = entry.path;
					g_gui.hostfat_browser_refresh = true;
				}
			}
		}
		ImGui::EndChild();
		if (ImGui::Button("Select this folder")) {
			copy_path(g_gui.pending_hostfat_dir, sizeof(g_gui.pending_hostfat_dir),
			          g_gui.hostfat_browser_dir);
			g_gui.hostfat_browser_open = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel##hostfat-folder") || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			g_gui.hostfat_browser_open = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

static void persist_fdd_dir(const std::string &dir) {
	if (!is_directory(dir)) {
		return;
	}
	copy_path(np2oscfg.gui_fdd_dir, sizeof(np2oscfg.gui_fdd_dir), dir);
	sysmng_update(SYS_UPDATEOSCFG);
}

static void persist_hdd_dir(const std::string &dir) {
	if (!is_directory(dir)) {
		return;
	}
	copy_path(np2oscfg.gui_hdd_dir, sizeof(np2oscfg.gui_hdd_dir), dir);
	sysmng_update(SYS_UPDATEOSCFG);
}

static bool file_is_mountable(const char *path, std::string *error) {
	short attr;

	if ((path == nullptr) || (path[0] == '\0')) {
		*error = "FDD image path is empty.";
		return false;
	}
	attr = file_attr(path);
	if (attr == static_cast<short>(-1)) {
		*error = "FDD image not found.";
		return false;
	}
	if ((attr & FILEATTR_DIRECTORY) != 0) {
		*error = "FDD image path is a directory.";
		return false;
	}
	error->clear();
	return true;
}

static bool hdd_file_is_mountable(const char *path, std::string *error) {
	short attr;

	if ((path == nullptr) || (path[0] == '\0')) {
		*error = "HDD image path is empty.";
		return false;
	}
	attr = file_attr(path);
	if (attr == static_cast<short>(-1)) {
		*error = "HDD image not found.";
		return false;
	}
	if ((attr & FILEATTR_DIRECTORY) != 0) {
		*error = "HDD image path is a directory.";
		return false;
	}
	error->clear();
	return true;
}

static bool hdd_is_scsi(int drive) {
	return ((drive & 0x20) != 0);
}

static int hdd_slot(int drive) {
	return (drive & 0x0f);
}

static const char *hdd_config_path(int drive) {
	int slot;

	slot = hdd_slot(drive);
	return (hdd_is_scsi(drive) ? np2cfg.scsihdd[slot] : np2cfg.sasihdd[slot]);
}

static const char *hdd_interface_name(int drive) {
	return (hdd_is_scsi(drive) ? "SCSI ID " : "SASI-");
}

static std::string display_file_name(const char *path) {
	if ((path == nullptr) || (path[0] == '\0')) {
		return std::string();
	}
	std::string name = fs::u8path(path).filename().u8string();
	const std::size_t separator = name.find_last_of("/\\");
	if (separator != std::string::npos) {
		name.erase(0, separator + 1);
	}
	return name;
}

static void set_fdd_status(int drive, const char *action, const char *path) {
	g_gui.fdd_status = "FDD";
	g_gui.fdd_status += static_cast<char>('1' + drive);
	g_gui.fdd_status += ' ';
	g_gui.fdd_status += action;
	const std::string name = display_file_name(path);
	if (!name.empty()) {
		g_gui.fdd_status += ": ";
		g_gui.fdd_status += name;
	}
}

static void set_hdd_status(int drive, const char *action, const char *path) {
	g_gui.hdd_status = hdd_interface_name(drive);
	if (hdd_is_scsi(drive)) {
		g_gui.hdd_status += std::to_string(hdd_slot(drive));
	} else {
		g_gui.hdd_status += static_cast<char>('1' + hdd_slot(drive));
	}
	g_gui.hdd_status += ' ';
	g_gui.hdd_status += action;
	const std::string name = display_file_name(path);
	if (!name.empty()) {
		g_gui.hdd_status += ": ";
		g_gui.hdd_status += name;
	}
}

static void remember_fdd_mount(int drive, const char *path) {
	if ((drive < 0) || (drive >= 2)) {
		return;
	}
	if ((path != nullptr) && (path[0] != '\0')) {
		copy_path(np2oscfg.fdd_image[drive], sizeof(np2oscfg.fdd_image[drive]), path);
	} else {
		np2oscfg.fdd_image[drive][0] = '\0';
	}
	sysmng_update(SYS_UPDATEOSCFG);
}

static void capture_reset_fdd_mounts(char paths[2][MAX_PATH]) {
	for (int drive = 0; drive < 2; drive++) {
		const char *current;

		paths[drive][0] = '\0';
		if (np2oscfg.fdd_image[drive][0] != '\0') {
			copy_path(paths[drive], MAX_PATH, np2oscfg.fdd_image[drive]);
			continue;
		}
		current = fdd_diskname(static_cast<REG8>(drive));
		if ((current != nullptr) && (current[0] != '\0')) {
			copy_path(paths[drive], MAX_PATH, current);
		}
	}
}

static void restore_reset_fdd_mounts(char paths[2][MAX_PATH]) {
	for (int drive = 0; drive < 2; drive++) {
		if (paths[drive][0] == '\0') {
			continue;
		}
		diskdrv_setfdd(static_cast<REG8>(drive), paths[drive], 0);
		remember_fdd_mount(drive, paths[drive]);
	}
}

static void reset_guest(void) {
	char fdd_paths[2][MAX_PATH];

	capture_reset_fdd_mounts(fdd_paths);
	taskmng_clear_fast_forward();
	pccore_cfgupdate();
	pccore_reset();
	restore_reset_fdd_mounts(fdd_paths);
	sdlkbd_reset_state();
	mousemng_reset();
	scrndrawva_redraw();
}

static void open_fdd_dialog(int drive) {
	const char *current;
	char archive_source_dir[MAX_PATH];
	std::string start_dir;

	g_gui.fdd_dialog_drive = drive;
	current = fdd_diskname(static_cast<REG8>(drive));
	if ((current == nullptr) || (current[0] == '\0')) {
		current = diskdrv_fname[drive];
	}
	if ((current != nullptr) && (current[0] != '\0')) {
		milstr_ncpy(g_gui.fdd_path[drive], current, sizeof(g_gui.fdd_path[drive]));
		if (dropmedia_fdd_source_directory(static_cast<UINT>(drive), current, archive_source_dir,
		                                   sizeof(archive_source_dir))) {
			start_dir = archive_source_dir;
		} else {
			start_dir = parent_dir(current);
		}
	}
	if (start_dir.empty() && (np2oscfg.gui_fdd_dir[0] != '\0') &&
	    is_directory(np2oscfg.gui_fdd_dir)) {
		start_dir = np2oscfg.gui_fdd_dir;
	}
	if (start_dir.empty()) {
		start_dir = home_dir();
	}
	g_gui.fdd_browser_dir = absolute_path(start_dir);
	g_gui.fdd_browser_open = true;
	g_gui.fdd_browser_refresh = true;
}

static void open_hdd_dialog(int drive) {
	const char *current;
	std::string start_dir;

	g_gui.hdd_dialog_drive = drive;
	current = hdd_config_path(drive);
	if ((current != nullptr) && (current[0] != '\0')) {
		milstr_ncpy(g_gui.hdd_path[hdd_slot(drive)], current,
		            sizeof(g_gui.hdd_path[hdd_slot(drive)]));
		start_dir = parent_dir(current);
	} else {
		g_gui.hdd_path[hdd_slot(drive)][0] = '\0';
	}
	if (start_dir.empty() && (np2oscfg.gui_hdd_dir[0] != '\0') &&
	    is_directory(np2oscfg.gui_hdd_dir)) {
		start_dir = np2oscfg.gui_hdd_dir;
	}
	if (start_dir.empty()) {
		start_dir = home_dir();
	}
	g_gui.hdd_browser_dir = absolute_path(start_dir);
	g_gui.hdd_browser_open = true;
	g_gui.hdd_browser_refresh = true;
}

static void open_new_sasi_dialog(int drive) {
	std::string start_dir;
	std::string path;

	g_gui.new_sasi_drive = std::clamp(drive, 0, 1);
	if ((np2oscfg.gui_hdd_dir[0] != '\0') && is_directory(np2oscfg.gui_hdd_dir)) {
		start_dir = np2oscfg.gui_hdd_dir;
	}
	if (start_dir.empty()) {
		start_dir = home_dir();
	}
	g_gui.hdd_browser_dir = absolute_path(start_dir);
	path = join_path(g_gui.hdd_browser_dir, new_sasi_default_name());
	copy_path(g_gui.new_sasi_path, sizeof(g_gui.new_sasi_path), path);
	g_gui.new_sasi_choice = 3;
	g_gui.new_sasi_open_after_create = true;
	g_gui.new_sasi_open = true;
	g_gui.new_sasi_refresh = true;
	g_gui.hdd_browser_open = false;
}

static void open_new_scsi_dialog(int drive) {
	std::string start_dir;
	std::string path;

	g_gui.new_scsi_drive = std::clamp(drive, 0, SCSIHDD_MAX - 1);
	if ((np2oscfg.gui_hdd_dir[0] != '\0') && is_directory(np2oscfg.gui_hdd_dir)) {
		start_dir = np2oscfg.gui_hdd_dir;
	}
	if (start_dir.empty()) {
		start_dir = home_dir();
	}
	g_gui.hdd_browser_dir = absolute_path(start_dir);
	path = new_scsi_default_path(g_gui.hdd_browser_dir, g_gui.new_scsi_drive);
	copy_path(g_gui.new_scsi_path, sizeof(g_gui.new_scsi_path), path);
	g_gui.new_scsi_choice = 3;
	g_gui.new_scsi_open_after_create = true;
	g_gui.new_scsi_open = true;
	g_gui.new_scsi_refresh = true;
	g_gui.hdd_browser_open = false;
}

static const char *new_fdd_default_name(int format, int container) {
	switch (format) {
	case NEWDISK_FDD_BLANK_2DD_720:
		return "newdisk-2dd720.d88";
	case NEWDISK_FDD_BLANK_2D_320:
		return "newdisk-2d320.d88";
	case NEWDISK_FDD_BLANK_2D_360:
		return "newdisk-2d360.d88";
	default:
		break;
	}
	if (container == NEWDISK_FDD_CONTAINER_RAW) {
		if (format == NEWDISK_FDD_MSDOS_2DD) {
			return "newdisk-2dd.img";
		}
		return "newdisk-2hd.img";
	}
	if (format == NEWDISK_FDD_MSDOS_2DD) {
		return "newdisk-2dd.d88";
	}
	return "newdisk-2hd.d88";
}

static void open_new_fdd_dialog(int format) {
	std::string start_dir;
	std::string path;

	g_gui.new_fdd_format = std::clamp(format, 0, NEWDISK_FDD_MSDOS_COUNT - 1);
	g_gui.new_fdd_container = NEWDISK_FDD_CONTAINER_D88;
	if ((np2oscfg.gui_fdd_dir[0] != '\0') && is_directory(np2oscfg.gui_fdd_dir)) {
		start_dir = np2oscfg.gui_fdd_dir;
	}
	if (start_dir.empty()) {
		start_dir = home_dir();
	}
	g_gui.fdd_browser_dir = absolute_path(start_dir);
	path = join_path(g_gui.fdd_browser_dir,
	                 new_fdd_default_name(g_gui.new_fdd_format, g_gui.new_fdd_container));
	copy_path(g_gui.new_fdd_path, sizeof(g_gui.new_fdd_path), path);
	g_gui.new_fdd_drive = 0;
	g_gui.new_fdd_mount_after_create = true;
	g_gui.new_fdd_open = true;
	g_gui.new_fdd_refresh = true;
	g_gui.fdd_browser_open = false;
}

static void mount_fdd_from_dialog(void) {
	int drive;
	const char *path;
	std::string error;

	drive = g_gui.fdd_dialog_drive;
	if ((drive < 0) || (drive >= 2)) {
		return;
	}
	path = g_gui.fdd_path[drive];
	if (!file_is_mountable(path, &error)) {
		g_gui.fdd_status = "FDD";
		g_gui.fdd_status += static_cast<char>('1' + drive);
		g_gui.fdd_status += " mount failed: ";
		g_gui.fdd_status += error;
		return;
	}
	if (dropmedia_path_is_archive(path)) {
		if (!dropmedia_mount_archive(path, static_cast<UINT>(drive))) {
			g_gui.fdd_status = dropmedia_status();
			return;
		}
		g_gui.fdd_status.clear();
		persist_fdd_dir(parent_dir(path));
		g_gui.fdd_browser_open = false;
		return;
	}
	diskdrv_setfdd(static_cast<REG8>(drive), path, 0);
	remember_fdd_mount(drive, path);
	dropmedia_prune_storage();
	persist_fdd_dir(parent_dir(path));
	set_fdd_status(drive, "mounted", path);
	g_gui.fdd_browser_open = false;
}

static void mount_hdd_from_dialog(void) {
	int drive;
	int slot;
	const char *path;
	std::string error;

	drive = g_gui.hdd_dialog_drive;
	slot = hdd_slot(drive);
	if ((drive < 0) || (slot < 0) || (slot >= 4)) {
		return;
	}
	path = g_gui.hdd_path[slot];
	if (!hdd_file_is_mountable(path, &error)) {
		g_gui.hdd_status = hdd_interface_name(drive);
		g_gui.hdd_status += static_cast<char>('1' + slot);
		g_gui.hdd_status += " open failed: ";
		g_gui.hdd_status += error;
		return;
	}
	diskdrv_sethdd(static_cast<REG8>(drive), path);
	persist_hdd_dir(parent_dir(path));
	set_hdd_status(drive, "configured; reset to apply", path);
	g_gui.hdd_browser_open = false;
}

static std::string hdi_path(const char *path) {
	std::string result;

	if (path != nullptr) {
		result = path;
	}
	if (result.empty()) {
		return result;
	}
	fs::path p = fs::u8path(result);
	if (p.extension().empty()) {
		p += ".hdi";
		result = p.u8string();
	}
	return result;
}

static std::string scsi_image_path(const char *path) {
	std::string result;

	if (path != nullptr) {
		result = path;
	}
	if (result.empty()) {
		return result;
	}
	fs::path p = fs::u8path(result);
	if (p.extension().empty()) {
		p += ".hdd";
	} else if ((p.extension() != ".hdd") && (p.extension() != ".hdi")) {
		p.replace_extension(".hdd");
	}
	return p.u8string();
}

static std::string fdd_image_path(const char *path, int container) {
	std::string result;
	std::string extension;

	if (path != nullptr) {
		result = path;
	}
	if (result.empty()) {
		return result;
	}
	fs::path p = fs::u8path(result);
	extension = p.extension().u8string();
	std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});
	const char *wanted = (container == NEWDISK_FDD_CONTAINER_RAW) ? ".img" : ".d88";
	if (extension.empty()) {
		p += wanted;
	} else if ((extension == ".d88") || (extension == ".img")) {
		p.replace_extension(wanted);
	} else if (extension != wanted) {
		p += wanted;
	}
	return p.u8string();
}

static void create_new_fdd_image(void) {
	int drive;
	int format;
	int container;
	std::string path;
	short attr;

	drive = std::clamp(g_gui.new_fdd_drive, 0, 1);
	format = std::clamp(g_gui.new_fdd_format, 0, NEWDISK_FDD_MSDOS_COUNT - 1);
	container = std::clamp(g_gui.new_fdd_container, 0, NEWDISK_FDD_CONTAINER_COUNT - 1);
	path = fdd_image_path(g_gui.new_fdd_path, container);
	if (path.empty()) {
		g_gui.fdd_status = "New FDD image failed: path is empty.";
		return;
	}
	path = absolute_path(path);
	attr = file_attr(path.c_str());
	if (attr != static_cast<short>(-1)) {
		g_gui.fdd_status = "New FDD image failed: file already exists.";
		return;
	}
	if (!is_directory(parent_dir(path))) {
		g_gui.fdd_status = "New FDD image failed: parent directory not found.";
		return;
	}
	if (newdisk_fdd_msdos_ex(path.c_str(), static_cast<UINT>(format),
	                         static_cast<UINT>(container)) != SUCCESS) {
		g_gui.fdd_status = "New FDD image failed: create failed.";
		return;
	}
	copy_path(g_gui.new_fdd_path, sizeof(g_gui.new_fdd_path), path);
	persist_fdd_dir(parent_dir(path));
	if (g_gui.new_fdd_mount_after_create) {
		diskdrv_setfdd(static_cast<REG8>(drive), path.c_str(), 0);
		remember_fdd_mount(drive, path.c_str());
		dropmedia_prune_storage();
		set_fdd_status(drive, "created and mounted", path.c_str());
	} else {
		g_gui.fdd_status = "New FDD image created: ";
		g_gui.fdd_status += display_file_name(path.c_str());
	}
	g_gui.new_fdd_open = false;
}

static void create_new_sasi_image(void) {
	int drive;
	int choice;
	std::string path;
	short attr;

	drive = std::clamp(g_gui.new_sasi_drive, 0, 1);
	choice = std::clamp(g_gui.new_sasi_choice, 0, kSasiImageCount - 1);
	path = hdi_path(g_gui.new_sasi_path);
	if (path.empty()) {
		g_gui.hdd_status = "New SASI image failed: path is empty.";
		return;
	}
	attr = file_attr(path.c_str());
	if (attr != static_cast<short>(-1)) {
		g_gui.hdd_status = "New SASI image failed: file already exists.";
		return;
	}
	if (!is_directory(parent_dir(path))) {
		g_gui.hdd_status = "New SASI image failed: parent directory not found.";
		return;
	}
	newdisk_hdi(path.c_str(), kSasiImageChoices[choice].hdd_type);
	attr = file_attr(path.c_str());
	if ((attr == static_cast<short>(-1)) || ((attr & FILEATTR_DIRECTORY) != 0)) {
		g_gui.hdd_status = "New SASI image failed: create failed.";
		return;
	}
	copy_path(g_gui.new_sasi_path, sizeof(g_gui.new_sasi_path), path);
	persist_hdd_dir(parent_dir(path));
	if (g_gui.new_sasi_open_after_create) {
		diskdrv_sethdd(static_cast<REG8>(drive), path.c_str());
		set_hdd_status(drive, "created and configured; reset to apply", path.c_str());
	} else {
		g_gui.hdd_status = "New SASI image created: ";
		g_gui.hdd_status += display_file_name(path.c_str());
	}
	g_gui.new_sasi_open = false;
}

static void create_new_scsi_image(void) {
	int drive;
	int choice;
	std::string path;
	short attr;

	drive = std::clamp(g_gui.new_scsi_drive, 0, SCSIHDD_MAX - 1);
	choice = std::clamp(g_gui.new_scsi_choice, 0, kScsiImageCount - 1);
	path = scsi_image_path(g_gui.new_scsi_path);
	if (path.empty()) {
		g_gui.hdd_status = "New SCSI image failed: path is empty.";
		return;
	}
	path = absolute_path(path);
	attr = file_attr(path.c_str());
	if (attr != static_cast<short>(-1)) {
		g_gui.hdd_status = "New SCSI image failed: file already exists.";
		return;
	}
	if (!is_directory(parent_dir(path))) {
		g_gui.hdd_status = "New SCSI image failed: parent directory not found.";
		return;
	}
	newdisk_vhd(path.c_str(), kScsiImageChoices[choice].size_mb);
	attr = file_attr(path.c_str());
	if ((attr == static_cast<short>(-1)) || ((attr & FILEATTR_DIRECTORY) != 0)) {
		g_gui.hdd_status = "New SCSI image failed: create failed.";
		return;
	}
	copy_path(g_gui.new_scsi_path, sizeof(g_gui.new_scsi_path), path);
	persist_hdd_dir(parent_dir(path));
	if (g_gui.new_scsi_open_after_create) {
		diskdrv_sethdd(static_cast<REG8>(0x20 | drive), path.c_str());
		set_hdd_status(0x20 | drive, "created and configured; reset to apply", path.c_str());
	} else {
		g_gui.hdd_status = "New SCSI image created: ";
		g_gui.hdd_status += display_file_name(path.c_str());
	}
	g_gui.new_scsi_open = false;
}

static void eject_fdd(int drive) {
	diskdrv_setfdd(static_cast<REG8>(drive), nullptr, 0);
	remember_fdd_mount(drive, nullptr);
	dropmedia_prune_storage();
	set_fdd_status(drive, "ejected", nullptr);
}

static void remove_hdd(int drive) {
	diskdrv_sethdd(static_cast<REG8>(drive), nullptr);
	sxsi_open();
	if (hdd_is_scsi(drive)) {
		if (!sxsi_isscsi()) {
			pccore.hddif &= ~PCHDD_SCSI;
		}
	} else if (!sxsi_issasi()) {
		pccore.hddif &= ~PCHDD_SASI;
	}
	set_hdd_status(drive, "removed; reset to apply", nullptr);
}

static void draw_fdd_browser(void) {
	int drive;

	if (!g_gui.fdd_browser_open) {
		return;
	}
	drive = g_gui.fdd_dialog_drive;
	if ((drive < 0) || (drive >= 2)) {
		g_gui.fdd_browser_open = false;
		return;
	}
	if (g_gui.fdd_browser_refresh) {
		refresh_fdd_browser();
	}
	ImGui::SetNextWindowSize(ImVec2(620.0f, 420.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Mount FDD image or archive", &g_gui.fdd_browser_open)) {
		if (draw_host_drive_selector(g_gui.fdd_browser_dir, g_gui.fdd_browser_refresh,
		                             "fdd-open")) {
			g_gui.fdd_path[drive][0] = '\0';
		}
		ImGui::Text("Target Dir");
		ImGui::TextWrapped("%s", g_gui.fdd_browser_dir.c_str());
		if (ImGui::Button("Home")) {
			g_gui.fdd_browser_dir = home_dir();
			g_gui.fdd_browser_refresh = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Up")) {
			g_gui.fdd_browser_dir = parent_dir(g_gui.fdd_browser_dir);
			g_gui.fdd_browser_refresh = true;
		}
		ImGui::Separator();
		const ImGuiStyle &style = ImGui::GetStyle();
		float footer_height = 2.0f * ImGui::GetFrameHeightWithSpacing();
		if (!g_gui.fdd_status.empty()) {
			footer_height += ImGui::CalcTextSize(g_gui.fdd_status.c_str(), nullptr, false,
			                                     ImGui::GetContentRegionAvail().x)
			                     .y +
			                 2.0f * style.ItemSpacing.y + 1.0f;
		}
		const float list_height = (std::max)(ImGui::GetFrameHeight() * 3.0f,
		                                     ImGui::GetContentRegionAvail().y - footer_height);
		if (ImGui::BeginChild("fdd-browser-list", ImVec2(0, list_height),
		                      ImGuiChildFlags_Borders)) {
			for (const auto &entry : g_gui.fdd_entries) {
				std::string label = entry.is_dir ? "[D] " : "    ";
				label += entry.name;
				if (ImGui::Selectable(label.c_str())) {
					if (entry.is_dir) {
						g_gui.fdd_browser_dir = entry.path;
						g_gui.fdd_browser_refresh = true;
					} else {
						copy_path(g_gui.fdd_path[drive], sizeof(g_gui.fdd_path[drive]), entry.path);
					}
				}
			}
		}
		ImGui::EndChild();
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputText("##fdd-path", g_gui.fdd_path[drive], sizeof(g_gui.fdd_path[drive]));
		if (ImGui::Button("Mount")) {
			mount_fdd_from_dialog();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			g_gui.fdd_browser_open = false;
		}
		if (!g_gui.fdd_status.empty()) {
			ImGui::Separator();
			ImGui::TextWrapped("%s", g_gui.fdd_status.c_str());
		}
	}
	ImGui::End();
}

static void draw_hdd_browser(void) {
	int drive;
	int slot;

	if (!g_gui.hdd_browser_open) {
		return;
	}
	drive = g_gui.hdd_dialog_drive;
	slot = hdd_slot(drive);
	if ((drive < 0) || (slot < 0) || (slot >= 4)) {
		g_gui.hdd_browser_open = false;
		return;
	}
	if (g_gui.hdd_browser_refresh) {
		refresh_hdd_browser();
	}
	ImGui::SetNextWindowSize(ImVec2(620.0f, 420.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Open HDD image", &g_gui.hdd_browser_open)) {
		if (draw_host_drive_selector(g_gui.hdd_browser_dir, g_gui.hdd_browser_refresh,
		                             "hdd-open")) {
			g_gui.hdd_path[slot][0] = '\0';
		}
		ImGui::Text("Target Dir");
		ImGui::TextWrapped("%s", g_gui.hdd_browser_dir.c_str());
		if (ImGui::Button("Home")) {
			g_gui.hdd_browser_dir = home_dir();
			g_gui.hdd_browser_refresh = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Up")) {
			g_gui.hdd_browser_dir = parent_dir(g_gui.hdd_browser_dir);
			g_gui.hdd_browser_refresh = true;
		}
		ImGui::Separator();
		if (ImGui::BeginChild("hdd-browser-list", ImVec2(0, 230.0f), ImGuiChildFlags_Borders)) {
			for (const auto &entry : g_gui.hdd_entries) {
				std::string label = entry.is_dir ? "[D] " : "    ";
				label += entry.name;
				if (ImGui::Selectable(label.c_str())) {
					if (entry.is_dir) {
						g_gui.hdd_browser_dir = entry.path;
						g_gui.hdd_browser_refresh = true;
					} else {
						copy_path(g_gui.hdd_path[slot], sizeof(g_gui.hdd_path[slot]), entry.path);
					}
				}
			}
		}
		ImGui::EndChild();
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputText("##hdd-path", g_gui.hdd_path[slot], sizeof(g_gui.hdd_path[slot]));
		if (ImGui::Button("Open")) {
			mount_hdd_from_dialog();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			g_gui.hdd_browser_open = false;
		}
		if (!g_gui.hdd_status.empty()) {
			ImGui::Separator();
			ImGui::TextWrapped("%s", g_gui.hdd_status.c_str());
		}
	}
	ImGui::End();
}

static void draw_new_sasi_dialog(void) {
	if (!g_gui.new_sasi_open) {
		return;
	}
	if (g_gui.new_sasi_refresh) {
		refresh_hdd_browser();
		g_gui.new_sasi_refresh = false;
	}
	ImGui::SetNextWindowSize(ImVec2(620.0f, 500.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Create SASI HDD image", &g_gui.new_sasi_open)) {
		if (draw_host_drive_selector(g_gui.hdd_browser_dir, g_gui.new_sasi_refresh, "new-sasi")) {
			copy_path(g_gui.new_sasi_path, sizeof(g_gui.new_sasi_path),
			          join_path(g_gui.hdd_browser_dir, new_sasi_default_name()));
		}
		ImGui::Text("Target Dir");
		ImGui::TextWrapped("%s", g_gui.hdd_browser_dir.c_str());
		if (ImGui::Button("Home")) {
			g_gui.hdd_browser_dir = home_dir();
			g_gui.new_sasi_refresh = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Up")) {
			g_gui.hdd_browser_dir = parent_dir(g_gui.hdd_browser_dir);
			g_gui.new_sasi_refresh = true;
		}
		ImGui::Separator();
		if (ImGui::BeginChild("new-sasi-browser-list", ImVec2(0, 170.0f),
		                      ImGuiChildFlags_Borders)) {
			for (const auto &entry : g_gui.hdd_entries) {
				std::string label = entry.is_dir ? "[D] " : "    ";
				label += entry.name;
				if (ImGui::Selectable(label.c_str())) {
					if (entry.is_dir) {
						g_gui.hdd_browser_dir = entry.path;
						copy_path(g_gui.new_sasi_path, sizeof(g_gui.new_sasi_path),
						          join_path(entry.path, new_sasi_default_name()));
						g_gui.new_sasi_refresh = true;
					} else {
						copy_path(g_gui.new_sasi_path, sizeof(g_gui.new_sasi_path), entry.path);
					}
				}
			}
		}
		ImGui::EndChild();
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputText("##new-sasi-path", g_gui.new_sasi_path, sizeof(g_gui.new_sasi_path));
		ImGui::Text("Image size");
		for (int i = 0; i < kSasiImageCount; i++) {
			if (i > 0) {
				ImGui::SameLine();
			}
			ImGui::RadioButton(kSasiImageChoices[i].label, &g_gui.new_sasi_choice, i);
		}
		ImGui::Text("Configure after create");
		ImGui::RadioButton("SASI-1", &g_gui.new_sasi_drive, 0);
		ImGui::SameLine();
		ImGui::RadioButton("SASI-2", &g_gui.new_sasi_drive, 1);
		ImGui::Checkbox("Set HDD file after create", &g_gui.new_sasi_open_after_create);
		if (ImGui::Button("Create")) {
			create_new_sasi_image();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			g_gui.new_sasi_open = false;
		}
		if (!g_gui.hdd_status.empty()) {
			ImGui::Separator();
			ImGui::TextWrapped("%s", g_gui.hdd_status.c_str());
		}
	}
	ImGui::End();
}

static void draw_new_scsi_dialog(void) {
	if (!g_gui.new_scsi_open) {
		return;
	}
	if (g_gui.new_scsi_refresh) {
		refresh_hdd_browser();
		g_gui.new_scsi_refresh = false;
	}
	ImGui::SetNextWindowSize(ImVec2(620.0f, 500.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Create SCSI HDD image", &g_gui.new_scsi_open)) {
		if (draw_host_drive_selector(g_gui.hdd_browser_dir, g_gui.new_scsi_refresh, "new-scsi")) {
			copy_path(g_gui.new_scsi_path, sizeof(g_gui.new_scsi_path),
			          new_scsi_default_path(g_gui.hdd_browser_dir, g_gui.new_scsi_drive));
		}
		ImGui::Text("Target Dir");
		ImGui::TextWrapped("%s", g_gui.hdd_browser_dir.c_str());
		if (ImGui::Button("Home")) {
			g_gui.hdd_browser_dir = home_dir();
			g_gui.new_scsi_refresh = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Up")) {
			g_gui.hdd_browser_dir = parent_dir(g_gui.hdd_browser_dir);
			g_gui.new_scsi_refresh = true;
		}
		ImGui::Separator();
		if (ImGui::BeginChild("new-scsi-browser-list", ImVec2(0, 170.0f),
		                      ImGuiChildFlags_Borders)) {
			for (const auto &entry : g_gui.hdd_entries) {
				std::string label = entry.is_dir ? "[D] " : "    ";
				label += entry.name;
				if (ImGui::Selectable(label.c_str())) {
					if (entry.is_dir) {
						g_gui.hdd_browser_dir = entry.path;
						copy_path(g_gui.new_scsi_path, sizeof(g_gui.new_scsi_path),
						          new_scsi_default_path(entry.path, g_gui.new_scsi_drive));
						g_gui.new_scsi_refresh = true;
					} else {
						copy_path(g_gui.new_scsi_path, sizeof(g_gui.new_scsi_path), entry.path);
					}
				}
			}
		}
		ImGui::EndChild();
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputText("##new-scsi-path", g_gui.new_scsi_path, sizeof(g_gui.new_scsi_path));
		ImGui::Text("Image size");
		for (int i = 0; i < kScsiImageCount; i++) {
			if (i > 0) {
				ImGui::SameLine();
			}
			ImGui::RadioButton(kScsiImageChoices[i].label, &g_gui.new_scsi_choice, i);
		}
		ImGui::Text("Configure after create");
		for (int drive = 0; drive < SCSIHDD_MAX; drive++) {
			int previous_drive = g_gui.new_scsi_drive;

			if (drive > 0) {
				ImGui::SameLine();
			}
			std::string label = "SCSI ID ";
			label += std::to_string(drive);
			if (ImGui::RadioButton(label.c_str(), &g_gui.new_scsi_drive, drive) &&
			    (std::string(g_gui.new_scsi_path) ==
			     new_scsi_default_path(g_gui.hdd_browser_dir, previous_drive))) {
				copy_path(g_gui.new_scsi_path, sizeof(g_gui.new_scsi_path),
				          new_scsi_default_path(g_gui.hdd_browser_dir, drive));
			}
		}
		ImGui::Checkbox("Set HDD file after create", &g_gui.new_scsi_open_after_create);
		if (ImGui::Button("Create")) {
			create_new_scsi_image();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			g_gui.new_scsi_open = false;
		}
		if (!g_gui.hdd_status.empty()) {
			ImGui::Separator();
			ImGui::TextWrapped("%s", g_gui.hdd_status.c_str());
		}
	}
	ImGui::End();
}

static void draw_new_fdd_dialog(void) {
	if (!g_gui.new_fdd_open) {
		return;
	}
	if (g_gui.new_fdd_refresh) {
		refresh_fdd_browser();
		g_gui.new_fdd_refresh = false;
	}
	ImGui::SetNextWindowSize(ImVec2(620.0f, 500.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("New FDD image", &g_gui.new_fdd_open)) {
		if (draw_host_drive_selector(g_gui.fdd_browser_dir, g_gui.new_fdd_refresh, "new-fdd")) {
			copy_path(
			    g_gui.new_fdd_path, sizeof(g_gui.new_fdd_path),
			    join_path(g_gui.fdd_browser_dir,
			              new_fdd_default_name(g_gui.new_fdd_format, g_gui.new_fdd_container)));
		}
		ImGui::Text("Target Dir");
		ImGui::TextWrapped("%s", g_gui.fdd_browser_dir.c_str());
		if (ImGui::Button("Home##new-fdd")) {
			g_gui.fdd_browser_dir = home_dir();
			copy_path(
			    g_gui.new_fdd_path, sizeof(g_gui.new_fdd_path),
			    join_path(g_gui.fdd_browser_dir,
			              new_fdd_default_name(g_gui.new_fdd_format, g_gui.new_fdd_container)));
			g_gui.new_fdd_refresh = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Up##new-fdd")) {
			g_gui.fdd_browser_dir = parent_dir(g_gui.fdd_browser_dir);
			copy_path(
			    g_gui.new_fdd_path, sizeof(g_gui.new_fdd_path),
			    join_path(g_gui.fdd_browser_dir,
			              new_fdd_default_name(g_gui.new_fdd_format, g_gui.new_fdd_container)));
			g_gui.new_fdd_refresh = true;
		}
		ImGui::Separator();
		if (ImGui::BeginChild("new-fdd-browser-list", ImVec2(0, 170.0f), ImGuiChildFlags_Borders)) {
			for (const auto &entry : g_gui.fdd_entries) {
				std::string label = entry.is_dir ? "[D] " : "    ";
				label += entry.name;
				if (ImGui::Selectable(label.c_str())) {
					if (entry.is_dir) {
						g_gui.fdd_browser_dir = entry.path;
						copy_path(
						    g_gui.new_fdd_path, sizeof(g_gui.new_fdd_path),
						    join_path(entry.path, new_fdd_default_name(g_gui.new_fdd_format,
						                                               g_gui.new_fdd_container)));
						g_gui.new_fdd_refresh = true;
					} else {
						copy_path(g_gui.new_fdd_path, sizeof(g_gui.new_fdd_path), entry.path);
					}
				}
			}
		}
		ImGui::EndChild();
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputText("##new-fdd-path", g_gui.new_fdd_path, sizeof(g_gui.new_fdd_path));
		ImGui::Text("Image format");
		if (ImGui::RadioButton("D88", &g_gui.new_fdd_container, NEWDISK_FDD_CONTAINER_D88)) {
			copy_path(g_gui.new_fdd_path, sizeof(g_gui.new_fdd_path),
			          fdd_image_path(g_gui.new_fdd_path, NEWDISK_FDD_CONTAINER_D88));
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("IMG (raw)", &g_gui.new_fdd_container, NEWDISK_FDD_CONTAINER_RAW)) {
			copy_path(g_gui.new_fdd_path, sizeof(g_gui.new_fdd_path),
			          fdd_image_path(g_gui.new_fdd_path, NEWDISK_FDD_CONTAINER_RAW));
		}
		ImGui::Text("Disk format");
		ImGui::RadioButton("2HD (1.2 MB)", &g_gui.new_fdd_format, NEWDISK_FDD_MSDOS_2HD);
		ImGui::SameLine();
		ImGui::RadioButton("2DD (640 KB)", &g_gui.new_fdd_format, NEWDISK_FDD_MSDOS_2DD);
		ImGui::TextDisabled("Unformatted (blank sectors, D88 only):");
		ImGui::RadioButton("2DD (720 KB)", &g_gui.new_fdd_format, NEWDISK_FDD_BLANK_2DD_720);
		ImGui::SameLine();
		ImGui::RadioButton("2D (320 KB)", &g_gui.new_fdd_format, NEWDISK_FDD_BLANK_2D_320);
		ImGui::SameLine();
		ImGui::RadioButton("2D (360 KB)", &g_gui.new_fdd_format, NEWDISK_FDD_BLANK_2D_360);
		if (!newdisk_fdd_has_filesystem(static_cast<UINT>(g_gui.new_fdd_format)) &&
		    (g_gui.new_fdd_container != NEWDISK_FDD_CONTAINER_D88)) {
			g_gui.new_fdd_container = NEWDISK_FDD_CONTAINER_D88;
			copy_path(g_gui.new_fdd_path, sizeof(g_gui.new_fdd_path),
			          fdd_image_path(g_gui.new_fdd_path, NEWDISK_FDD_CONTAINER_D88));
		}
		ImGui::Text("Mount after create");
		ImGui::RadioButton("FDD1##new-fdd", &g_gui.new_fdd_drive, 0);
		ImGui::SameLine();
		ImGui::RadioButton("FDD2##new-fdd", &g_gui.new_fdd_drive, 1);
		ImGui::Checkbox("Mount image after create", &g_gui.new_fdd_mount_after_create);
		if (ImGui::Button("Create##new-fdd")) {
			create_new_fdd_image();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel##new-fdd")) {
			g_gui.new_fdd_open = false;
		}
		if (!g_gui.fdd_status.empty()) {
			ImGui::Separator();
			ImGui::TextWrapped("%s", g_gui.fdd_status.c_str());
		}
	}
	ImGui::End();
}

/* Emulation speed: 10-400 % of real time in 10 % steps, and No Wait (full
 * speed) at the right end of the slider. */
static void draw_speed_control(void) {
	enum {
		SPEED_MIN = 10,
		SPEED_MAX = 400,
		SPEED_FULL = SPEED_MAX + 10
	};
	const int speed = (np2oscfg.speed_percent == 0) ? 100 : np2oscfg.speed_percent;
	int value = np2oscfg.NOWAIT ? SPEED_FULL : speed;

	ImGui::TextUnformatted("速度");
	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 12.0f);
	if (ImGui::SliderInt("##speed", &value, SPEED_MIN, SPEED_FULL,
	                     np2oscfg.NOWAIT ? "最大 (No Wait)" : "%d%%",
	                     ImGuiSliderFlags_AlwaysClamp)) {
		value = ((value + 5) / 10) * 10;
		if (value > SPEED_MAX) {
			np2oscfg.NOWAIT = 1;
		} else {
			np2oscfg.NOWAIT = 0;
			np2oscfg.speed_percent = static_cast<UINT16>(value);
			timing_setspeed(np2oscfg.speed_percent);
		}
		sysmng_update(SYS_UPDATEOSCFG);
	}
	if (ImGui::MenuItem("速度を 100% に戻す", nullptr, false,
	                    (np2oscfg.NOWAIT != 0) || (speed != 100))) {
		np2oscfg.NOWAIT = 0;
		np2oscfg.speed_percent = 100;
		timing_setspeed(100);
		sysmng_update(SYS_UPDATEOSCFG);
	}
	// CPU speed: only the CPU slows (V3 and V1/V2); frames, timers and sound
	// keep real time.
	int cpu = static_cast<int>(pccore_cpu_speed());
	ImGui::TextUnformatted("CPU 速度");
	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 12.0f);
	if (ImGui::SliderInt("##cpuspeed", &cpu, 10, 100, "%d%%", ImGuiSliderFlags_AlwaysClamp)) {
		cpu = ((cpu + 2) / 5) * 5;
		np2cfg.cpu_speed = static_cast<UINT8>(cpu);
		pccore_set_cpu_speed(np2cfg.cpu_speed);
		sysmng_update(SYS_UPDATECFG);
	}
	if (ImGui::MenuItem("CPU 速度を 100% に戻す", nullptr, false, pccore_cpu_speed() != 100)) {
		np2cfg.cpu_speed = 100;
		pccore_set_cpu_speed(100);
		sysmng_update(SYS_UPDATECFG);
	}
}

static void draw_emulate_menu(void) {
	if (ImGui::BeginMenu("エミュレート")) {
		if (ImGui::MenuItem("リセット")) {
			reset_guest();
		}
		ImGui::Separator();
		draw_speed_control();
		ImGui::Separator();
		if (ImGui::BeginMenu("起動機種")) {
			if (ImGui::MenuItem("VA", nullptr, milstr_cmp(np2cfg.model, str_VA1) == 0)) {
				select_boot_model(str_VA1, false);
			}
			if (ImGui::MenuItem("VA2/VA3", nullptr,
			                    (milstr_cmp(np2cfg.model, str_VA2) == 0) &&
			                        (np2cfg.upd8087_enable == 0))) {
				select_boot_model(str_VA2, false);
			}
			if (ImGui::MenuItem("VA2/VA3 + 8087", nullptr,
			                    (milstr_cmp(np2cfg.model, str_VA2) == 0) &&
			                        (np2cfg.upd8087_enable != 0))) {
				select_boot_model(str_VA2, true);
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("モニタ")) {
			// DIP switch SW1, read by the ROM at reset.
			if (ImGui::MenuItem("24 kHz", nullptr, np2cfg.monitor_15khz == 0)) {
				select_monitor(false);
			}
			if (ImGui::MenuItem("15 kHz", nullptr, np2cfg.monitor_15khz != 0)) {
				select_monitor(true);
			}
			ImGui::Separator();
			// Analog or digital RGB output; display only, no reset.
			if (ImGui::MenuItem("デジタル RGB (8色)", nullptr, np2cfg.monitor_digital != 0)) {
				np2cfg.monitor_digital = np2cfg.monitor_digital ? 0 : 1;
				sysmng_update(SYS_UPDATECFG);
				scrndrawva_redraw();
			}
			ImGui::Separator();
			// 15 kHz scanlines: gaps as on a CRT, or filled as on an LCD.
			if (ImGui::MenuItem("15 kHz 走査線: 隙間あり (CRT)", nullptr,
			                    np2cfg.monitor_15khz_fill == 0)) {
				np2cfg.monitor_15khz_fill = 0;
				sysmng_update(SYS_UPDATECFG);
				pccore_redraw();
			}
			if (ImGui::MenuItem("15 kHz 走査線: 埋める (液晶)", nullptr,
			                    np2cfg.monitor_15khz_fill != 0)) {
				np2cfg.monitor_15khz_fill = 1;
				sysmng_update(SYS_UPDATECFG);
				pccore_redraw();
			}
			ImGui::Separator();
			// V1/V2 at 24 kHz: the VA's own picture or a PC-8801-style one
			// (8x8 text doubled, 200-line graphics with scanline gaps).
			if (ImGui::MenuItem("V1/V2 表示: VA 標準", nullptr, np2cfg.v1v2_8801_display == 0)) {
				np2cfg.v1v2_8801_display = 0;
				sysmng_update(SYS_UPDATECFG);
				pccore_redraw();
			}
			if (ImGui::MenuItem("V1/V2 表示: PC-8801 風", nullptr, np2cfg.v1v2_8801_display != 0)) {
				np2cfg.v1v2_8801_display = 1;
				sysmng_update(SYS_UPDATECFG);
				pccore_redraw();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Z80モード")) {
			static const struct {
				const char *label;
				bool v1;
				bool standard;
			} modes[] = {{"V2 H", false, false},
			             {"V2 S", false, true},
			             {"V1 H", true, false},
			             {"V1 S", true, true}};
			const bool v1 = bkupmemva_get_88v1() != FALSE;
			const bool standard = np2cfg.v1v2_standard != 0;
			const bool nmode = np2cfg.v1v2_nmode != 0;

			for (const auto &mode : modes) {
				if (ImGui::MenuItem(mode.label, nullptr,
				                    !nmode && (v1 == mode.v1) && (standard == mode.standard))) {
					select_v1v2_mode(mode.v1, mode.standard);
				}
			}
			ImGui::Separator();
			// vaeg extension: start in N-BASIC from a user-supplied ROM. The ROM
			// directory is scanned (SHA-1) once each time the menu opens.
			if (!g_n80_scanned) {
				n80rom_scan(g_n80_entries, ROMVA_N80_NAMES);
				g_n80_scanned = true;
			}
			bool any_n80 = false;
			for (const auto &entry : g_n80_entries) {
				if (!entry.present) {
					continue;
				}
				any_n80 = true;
				const std::string label =
				    std::string("N (PC-8001): ") + entry.name + " - " + entry.label;
				const bool loaded =
				    nmode && memoryva_n80_exist && (strcmp(memoryva_n80_file, entry.name) == 0);
				if (ImGui::MenuItem(label.c_str(), nullptr, loaded)) {
					np2cfg.v1v2_nmode = 1;
					milstr_ncpy(np2cfg.v1v2_n80rom, entry.name, sizeof(np2cfg.v1v2_n80rom));
					sysmng_update(SYS_UPDATECFG);
					reset_guest();
				}
			}
			if (!any_n80) {
				ImGui::MenuItem("N (PC-8001): no N-BASIC ROM", nullptr, false, false);
			}
			ImGui::EndMenu();
		} else {
			g_n80_scanned = false;
		}
		ImGui::Separator();
		if (ImGui::MenuItem("Configure...")) {
			open_configure_dialog();
		}
		ImGui::Separator();
		if (ImGui::MenuItem("終了")) {
			taskmng_exit();
		}
		ImGui::EndMenu();
	}
}

static void draw_edit_menu(void) {
	if (ImGui::BeginMenu("編集")) {
#if defined(__APPLE__)
		const char *copy_shortcut = "Cmd+C";
		const char *shortcut = "Cmd+V";
#else
		const char *copy_shortcut = "Ctrl+Shift+C";
		const char *shortcut = "Ctrl+V";
#endif
		if (ImGui::MenuItem("Copy screen text", copy_shortcut)) {
			copy_screen_text();
		}
		if (ImGui::MenuItem("貼り付け", shortcut)) {
			kbdpaste_start_clipboard();
		}
		if (ImGui::MenuItem("Cancel Paste", nullptr, false, kbdpaste_active() ? true : false)) {
			kbdpaste_cancel();
		}
		if (kbdpaste_status()[0] != '\0') {
			ImGui::Separator();
			ImGui::TextWrapped("%s", kbdpaste_status());
		}
		ImGui::EndMenu();
	}
}

static void draw_fdd_mount_state(int drive) {
	const char *path;
	bool inserting;

	path = fdd_diskname(static_cast<REG8>(drive));
	inserting = false;
	if ((path == nullptr) || (path[0] == '\0')) {
		path = diskdrv_fname[drive];
		inserting = path[0] != '\0';
	}
	if ((path == nullptr) || (path[0] == '\0')) {
		ImGui::TextDisabled("FDD%d: Empty", drive + 1);
		return;
	}
	const std::string name = display_file_name(path);
	if (inserting) {
		ImGui::Text("FDD%d: %s (inserting)", drive + 1, name.c_str());
	} else {
		ImGui::Text("FDD%d: %s", drive + 1, name.c_str());
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("%s", name.c_str());
	}
}

static void draw_hdd_mount_state(int drive) {
	std::string label;
	const char *path;

	path = hdd_config_path(drive);
	label = hdd_interface_name(drive);
	if (hdd_is_scsi(drive)) {
		label += std::to_string(hdd_slot(drive));
	} else {
		label += static_cast<char>('1' + hdd_slot(drive));
	}
	if ((path == nullptr) || (path[0] == '\0')) {
		ImGui::TextDisabled("%s: Empty", label.c_str());
		return;
	}
	const std::string name = display_file_name(path);
	ImGui::Text("%s: %s", label.c_str(), name.c_str());
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("%s", name.c_str());
	}
}

static void draw_fdd_menu(void) {
	if (ImGui::BeginMenu("FDD")) {
		if (ImGui::MenuItem("FDD1 Open...")) {
			open_fdd_dialog(0);
		}
		if (ImGui::MenuItem("FDD1 Eject")) {
			eject_fdd(0);
		}
		draw_fdd_mount_state(0);
		ImGui::Separator();
		if (ImGui::MenuItem("FDD2 Open...")) {
			open_fdd_dialog(1);
		}
		if (ImGui::MenuItem("FDD2 Eject")) {
			eject_fdd(1);
		}
		draw_fdd_mount_state(1);
		ImGui::Separator();
		menu_item_not_implemented("FDD3 Open... (not implemented)");
		menu_item_not_implemented("FDD3 Eject (not implemented)");
		menu_item_not_implemented("FDD4 Open... (not implemented)");
		menu_item_not_implemented("FDD4 Eject (not implemented)");
		if (dropmedia_status()[0] != '\0') {
			ImGui::Separator();
			ImGui::TextWrapped("%s", dropmedia_status());
		}
		ImGui::Separator();
		if (ImGui::BeginMenu("New FDD image")) {
			if (ImGui::MenuItem("2HD (1.2 MB)...")) {
				open_new_fdd_dialog(NEWDISK_FDD_MSDOS_2HD);
			}
			if (ImGui::MenuItem("2DD (640 KB)...")) {
				open_new_fdd_dialog(NEWDISK_FDD_MSDOS_2DD);
			}
			ImGui::Separator();
			ImGui::TextDisabled("Unformatted");
			if (ImGui::MenuItem("2DD (720 KB)...")) {
				open_new_fdd_dialog(NEWDISK_FDD_BLANK_2DD_720);
			}
			if (ImGui::MenuItem("2D (320 KB)...")) {
				open_new_fdd_dialog(NEWDISK_FDD_BLANK_2D_320);
			}
			if (ImGui::MenuItem("2D (360 KB)...")) {
				open_new_fdd_dialog(NEWDISK_FDD_BLANK_2D_360);
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenu();
	}
}

// ---- Cassette tape (vaeg extension for V1/V2 mode, M103j)

static void refresh_tape_browser(void) {
	std::error_code ec;

	g_gui.tape_entries.clear();
	if (!is_directory(g_gui.tape_browser_dir)) {
		g_gui.tape_browser_dir = home_dir();
	}
	for (const auto &entry : fs::directory_iterator(fs::u8path(g_gui.tape_browser_dir), ec)) {
		BrowserEntry item;
		std::error_code st_ec;

		if (ec) {
			break;
		}
		item.is_dir = entry.is_directory(st_ec);
		if ((!item.is_dir) && (!entry.is_regular_file(st_ec))) {
			continue;
		}
		item.name = entry.path().filename().u8string();
		item.path = entry.path().u8string();
		if (item.name.empty() || (item.name[0] == '.')) {
			continue;
		}
		g_gui.tape_entries.push_back(item);
	}
	std::sort(g_gui.tape_entries.begin(), g_gui.tape_entries.end(), browser_entry_less);
	g_gui.tape_browser_refresh = false;
}

static void open_tape_dialog(void) {
	std::string start_dir;

	if (!g_gui.tape_loaded.empty()) {
		start_dir = parent_dir(g_gui.tape_loaded);
	} else if (!g_gui.tape_recording.empty()) {
		start_dir = parent_dir(g_gui.tape_recording);
	}
	if (start_dir.empty() && (np2oscfg.gui_fdd_dir[0] != '\0') &&
	    is_directory(np2oscfg.gui_fdd_dir)) {
		start_dir = np2oscfg.gui_fdd_dir;
	}
	if (start_dir.empty()) {
		start_dir = home_dir();
	}
	g_gui.tape_browser_dir = absolute_path(start_dir);
	g_gui.tape_status.clear();
	g_gui.tape_browser_open = true;
	g_gui.tape_browser_refresh = true;
}

static void stop_tape_recording(void) {
	if (cmt_saving()) {
		g_gui.tape_status =
		    (cmt_save_end() == SUCCESS) ? "Recording saved: " : "Recording could not be written: ";
		g_gui.tape_status += g_gui.tape_recording;
	}
	g_gui.tape_recording.clear();
}

static void draw_tape_browser(void) {
	if (!g_gui.tape_browser_open) {
		return;
	}
	if (g_gui.tape_browser_refresh) {
		refresh_tape_browser();
	}
	ImGui::SetNextWindowSize(ImVec2(620.0f, 420.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Cassette tape image", &g_gui.tape_browser_open)) {
		draw_host_drive_selector(g_gui.tape_browser_dir, g_gui.tape_browser_refresh, "tape-open");
		ImGui::TextWrapped("%s", g_gui.tape_browser_dir.c_str());
		if (ImGui::Button("Home")) {
			g_gui.tape_browser_dir = home_dir();
			g_gui.tape_browser_refresh = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Up")) {
			g_gui.tape_browser_dir = parent_dir(g_gui.tape_browser_dir);
			g_gui.tape_browser_refresh = true;
		}
		ImGui::Separator();
		const float list_height = (std::max)(ImGui::GetFrameHeight() * 3.0f,
		                                     ImGui::GetContentRegionAvail().y -
		                                         3.0f * ImGui::GetFrameHeightWithSpacing());
		if (ImGui::BeginChild("tape-browser-list", ImVec2(0, list_height),
		                      ImGuiChildFlags_Borders)) {
			for (const auto &entry : g_gui.tape_entries) {
				std::string label = entry.is_dir ? "[D] " : "    ";
				label += entry.name;
				if (ImGui::Selectable(label.c_str())) {
					if (entry.is_dir) {
						g_gui.tape_browser_dir = entry.path;
						g_gui.tape_browser_refresh = true;
					} else {
						copy_path(g_gui.tape_path, sizeof(g_gui.tape_path), entry.path);
					}
				}
			}
		}
		ImGui::EndChild();
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputText("##tape-path", g_gui.tape_path, sizeof(g_gui.tape_path));
		if (ImGui::Button("Load tape (.cmt / .t88)")) {
			if (cmt_open(g_gui.tape_path) == SUCCESS) {
				g_gui.tape_loaded = g_gui.tape_path;
				g_gui.tape_status = "Tape loaded.";
				g_gui.tape_browser_open = false;
			} else {
				g_gui.tape_status = "Not a usable tape image.";
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Record to this file")) {
			stop_tape_recording();
			if ((g_gui.tape_path[0] != '\0') && (cmt_save_begin(g_gui.tape_path) == SUCCESS)) {
				g_gui.tape_recording = g_gui.tape_path;
				g_gui.tape_status = "Recording; written when stopped.";
				g_gui.tape_browser_open = false;
			} else {
				g_gui.tape_status = "Enter a file name to record to.";
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			g_gui.tape_browser_open = false;
		}
		if (!g_gui.tape_status.empty()) {
			ImGui::TextWrapped("%s", g_gui.tape_status.c_str());
		}
	}
	ImGui::End();
}

static void draw_tape_menu(void) {
	if (ImGui::BeginMenu("Tape")) {
		if (ImGui::MenuItem("Load / record...")) {
			open_tape_dialog();
		}
		if (ImGui::MenuItem("Rewind", nullptr, false, cmt_inserted() != FALSE)) {
			cmt_rewind();
		}
		if (ImGui::MenuItem("Eject", nullptr, false, cmt_inserted() != FALSE)) {
			cmt_eject();
			g_gui.tape_loaded.clear();
		}
		if (ImGui::MenuItem("Stop recording", nullptr, false, cmt_saving() != FALSE)) {
			stop_tape_recording();
		}
		bool fast = np2cfg.cmt_fast != 0;
		if (ImGui::MenuItem("Fast load", nullptr, fast)) {
			np2cfg.cmt_fast = fast ? 0 : 1;
			sysmng_update(SYS_UPDATECFG);
		}
		ImGui::Separator();
		if (cmt_inserted()) {
			const std::string name = display_file_name(g_gui.tape_loaded.c_str());
			ImGui::Text("Tape: %s", name.c_str());
			ImGui::Text("%u / %u bytes", static_cast<unsigned>(cmt_position()),
			            static_cast<unsigned>(cmt_length()));
		} else {
			ImGui::TextDisabled("No tape");
		}
		if (cmt_saving()) {
			const std::string name = display_file_name(g_gui.tape_recording.c_str());
			ImGui::Text("Recording: %s (%u bytes)", name.c_str(),
			            static_cast<unsigned>(cmt_saved_bytes()));
		}
		if (!g_gui.tape_status.empty()) {
			ImGui::TextWrapped("%s", g_gui.tape_status.c_str());
		}
		ImGui::TextDisabled("V1/V2 BASIC (vaeg extension)");
		ImGui::EndMenu();
	}
}

static void draw_harddisk_menu(void) {
	if (ImGui::BeginMenu("HardDisk")) {
		if (ImGui::MenuItem("SASI-1 Open...")) {
			open_hdd_dialog(0);
		}
		if (ImGui::MenuItem("SASI-1 Remove")) {
			remove_hdd(0);
		}
		draw_hdd_mount_state(0);
		ImGui::Separator();
		if (ImGui::MenuItem("SASI-2 Open...")) {
			open_hdd_dialog(1);
		}
		if (ImGui::MenuItem("SASI-2 Remove")) {
			remove_hdd(1);
		}
		draw_hdd_mount_state(1);
		ImGui::Separator();
		for (int drive = 0; drive < SCSIHDD_MAX; drive++) {
			int encoded;

			encoded = 0x20 | drive;
			if (ImGui::MenuItem(
			        (std::string("SCSI ID ") + std::to_string(drive) + " Open...").c_str())) {
				open_hdd_dialog(encoded);
			}
			if (ImGui::MenuItem(
			        (std::string("SCSI ID ") + std::to_string(drive) + " Remove").c_str())) {
				remove_hdd(encoded);
			}
			draw_hdd_mount_state(encoded);
			if (drive != SCSIHDD_MAX - 1) {
				ImGui::Separator();
			}
		}
		ImGui::Separator();
		if (ImGui::MenuItem("New SASI image...")) {
			open_new_sasi_dialog(0);
		}
		if (ImGui::MenuItem("New SCSI image...")) {
			open_new_scsi_dialog(0);
		}
		ImGui::EndMenu();
	}
}

static void set_display_scale(int scale) {
	np2oscfg.gui_scale = static_cast<BYTE>(scale);
	scrnmng_set_display(np2oscfg.gui_scale, np2oscfg.gui_aspect);
	sysmng_update(SYS_UPDATEOSCFG);
}

static void set_display_aspect(bool aspect) {
	np2oscfg.gui_aspect = aspect ? 1 : 0;
	scrnmng_set_display(np2oscfg.gui_scale, np2oscfg.gui_aspect);
	sysmng_update(SYS_UPDATEOSCFG);
}

static void set_display_scaling(int scaling) {
	np2oscfg.gui_scaling = static_cast<BYTE>(scaling);
	scrnmng_set_scaling(np2oscfg.gui_scaling);
	sysmng_update(SYS_UPDATEOSCFG);
}

static void set_display_effect(int effect) {
	np2oscfg.gui_effect = static_cast<BYTE>(effect);
	scrnmng_set_effect(np2oscfg.gui_effect);
	sysmng_update(SYS_UPDATEOSCFG);
}

static void load_native_crt_preset(void) {
	std::string error;

	if (np2oscfg.gui_shader_preset[0] == '\0') {
		milstr_ncpy(np2oscfg.gui_shader_preset, VAEG_DEFAULT_SHADER_PRESET,
		            sizeof(np2oscfg.gui_shader_preset));
	}
	if (!g_gui.native_crt_preset.load(scrnmng_native_preset_path(), &error)) {
		g_gui.native_crt_loaded_path.clear();
		g_gui.native_crt_status = "Preset load failed: ";
		g_gui.native_crt_status += error;
		return;
	}
	g_gui.native_crt_loaded_path = np2oscfg.gui_shader_preset;
	if (!g_gui.native_crt_preset.parameters().load_config()) {
		g_gui.native_crt_status = "Preset loaded; parameter state is invalid";
	} else {
		g_gui.native_crt_status = "Preset loaded";
	}
}

static void set_native_crt_mode(bool filtered) {
	if (scrnmng_native_active()) {
		if (!scrnmng_set_native_filter(filtered ? TRUE : FALSE)) {
			g_gui.native_crt_status = filtered ? "CRT unavailable; reload preset to retry"
			                                   : "Unfiltered librashader mode unavailable";
			return;
		}
		np2oscfg.gui_native_crt = 1;
		np2oscfg.gui_native_filter = filtered ? 1 : 0;
		g_gui.native_crt_settings_open = false;
		g_gui.native_crt_status = filtered ? "CRT enabled" : "Librashader unfiltered";
	} else {
		np2oscfg.gui_native_crt = 1;
		np2oscfg.gui_native_filter = filtered ? 1 : 0;
		scrnmng_request_native_crt(TRUE, TRUE);
#if defined(_WIN32) && defined(VAEG_ENABLE_LIBRASHADER)
		g_gui.native_crt_status =
		    filtered ? "Applying CRT selection" : "Applying unfiltered selection";
#else
		g_gui.native_crt_status = filtered ? "librashader selected; restart to apply"
		                                   : "unfiltered librashader selected; restart to apply";
#endif
	}
	sysmng_update(SYS_UPDATEOSCFG);
}

static void draw_renderer_selection(void) {
	bool enabled = scrnmng_native_active() != FALSE;
	bool filtered = enabled && scrnmng_native_filter_enabled() != FALSE;
	if (ImGui::BeginMenu("描画方式")) {
		if (ImGui::MenuItem("標準（SDL）", nullptr, !enabled)) {
			np2oscfg.gui_native_filter = 1;
			scrnmng_request_native_crt(FALSE, TRUE);
			sysmng_update(SYS_UPDATEOSCFG);
			g_gui.native_crt_status = "SDL selected";
		}
		if (ImGui::MenuItem("CRT効果（librashader）", nullptr, filtered)) {
			set_native_crt_mode(true);
		}
		if (ImGui::MenuItem("加工なし（librashader）", nullptr, enabled && !filtered)) {
			set_native_crt_mode(false);
		}
		ImGui::EndMenu();
	}
	const char *renderer_status = scrnmng_native_status();
	if (std::strcmp(renderer_status, "SDL") != 0 &&
	    std::strcmp(renderer_status, "Native CRT ON") != 0) {
		ImGui::TextWrapped("描画状態: %s", renderer_status);
	}
#if !defined(_WIN32) || !defined(VAEG_ENABLE_LIBRASHADER)
	if (np2oscfg.gui_native_crt && !enabled) {
		ImGui::TextWrapped("librashader requested; restart required (if supported by this build)");
	}
#endif
	if (enabled && filtered) {
		if (ImGui::MenuItem("CRT設定…")) {
			g_gui.native_crt_settings_open = true;
		}
	}
}

static void draw_native_crt_settings(void) {
	if (!g_gui.native_crt_settings_open || !scrnmng_native_filter_enabled()) {
		return;
	}
	ImGui::SetNextWindowSize(ImVec2(560, 420), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("CRT設定###Shader settings", &g_gui.native_crt_settings_open)) {
		ImGui::End();
		return;
	}
	ImGui::TextWrapped("Active: %s", scrnmng_native_status());
	ImGui::Separator();
	if (ImGui::InputText("Preset path", np2oscfg.gui_shader_preset,
	                     sizeof(np2oscfg.gui_shader_preset))) {
		g_gui.native_crt_status = "Preset path changed; press Reload";
		sysmng_update(SYS_UPDATEOSCFG);
	}
	if (ImGui::Button("Reload preset")) {
		load_native_crt_preset();
		if (scrnmng_native_active())
			scrnmng_request_native_crt(TRUE, TRUE);
	}
	ImGui::SameLine();
	if (ImGui::Button("Clear status")) {
		g_gui.native_crt_status.clear();
	}
	if (g_gui.native_crt_loaded_path != np2oscfg.gui_shader_preset) {
		ImGui::TextDisabled("Preset is not loaded");
	} else {
		vaeg::librashader::ShaderParameterSet &parameters =
		    g_gui.native_crt_preset.parameters();
		vaeg::librashader::ShaderParameterInfo *mask_parameter = nullptr;
		for (std::size_t i = 0; i < parameters.size(); ++i) {
			vaeg::librashader::ShaderParameterInfo *parameter = parameters.at(i);
			if ((parameter != nullptr) && (parameter->name == "MASK_INTENSITY")) {
				mask_parameter = parameter;
				break;
			}
		}
		if (mask_parameter != nullptr) {
			if (np2oscfg.gui_native_mask_auto) {
				mask_parameter->value = scrnmng_native_mask_intensity();
			}
			bool auto_mask = np2oscfg.gui_native_mask_auto != 0;
			if (ImGui::Checkbox("RGB mask: Auto by window scale", &auto_mask)) {
				np2oscfg.gui_native_mask_auto = auto_mask ? 1 : 0;
				if (auto_mask) {
					mask_parameter->value = scrnmng_native_mask_intensity();
				}
				scrnmng_apply_native_mask_profile();
				if (!parameters.save_config()) {
					g_gui.native_crt_status = "RGB mask mode save failed";
				} else {
					g_gui.native_crt_status = auto_mask ? "RGB mask automatic"
					                                  : "RGB mask manual";
					sysmng_update(SYS_UPDATEOSCFG);
				}
			}
		}
		for (std::size_t i = 0; i < parameters.size(); ++i) {
			vaeg::librashader::ShaderParameterInfo *parameter = parameters.at(i);
			if (parameter == nullptr) {
				continue;
			}
			float value = parameter->value;
			ImGui::PushID(static_cast<int>(i));
			const char *format = parameter->name == "SCREEN_SIZE" ? "%.2f" : "%.3f";
			if (ImGui::SliderFloat(parameter->name.c_str(), &value, parameter->minimum,
			                       parameter->maximum, format)) {
				(void)parameters.set_value_at(i, value, nullptr);
				if (parameter->name == "MASK_INTENSITY") {
					np2oscfg.gui_native_mask_auto = 0;
				}
				if (scrnmng_native_active() &&
				    scrnmng_native_set_parameter(parameter->name.c_str(), parameter->value) !=
				        SUCCESS) {
					g_gui.native_crt_status = "Live parameter update failed";
				} else if (!parameters.save_config()) {
					g_gui.native_crt_status = "Parameter save failed";
				} else {
					g_gui.native_crt_status = "Parameter updated";
					sysmng_update(SYS_UPDATEOSCFG);
				}
			}
			if (!parameter->description.empty()) {
				ImGui::TextDisabled("%s", parameter->description.c_str());
			}
			ImGui::PopID();
		}
		if (ImGui::Button("Reset parameters")) {
			np2oscfg.gui_native_mask_auto = 1;
			parameters.reset();
			if (mask_parameter != nullptr) {
				mask_parameter->value = scrnmng_native_mask_intensity();
			}
			if (scrnmng_native_active()) {
				for (std::size_t i = 0; i < parameters.size(); ++i) {
					const auto *parameter = parameters.at(i);
					(void)scrnmng_native_set_parameter(parameter->name.c_str(), parameter->value);
				}
			}
			if (!parameters.save_config()) {
				g_gui.native_crt_status = "Parameter reset save failed";
			} else {
				g_gui.native_crt_status = "Parameters reset";
				sysmng_update(SYS_UPDATEOSCFG);
			}
		}
	}
	if (!g_gui.native_crt_status.empty()) {
		ImGui::TextDisabled("%s", g_gui.native_crt_status.c_str());
	}
	ImGui::End();
}

static void set_display_mode(int mode) {
	UINT width = np2oscfg.fscrn_cx;
	UINT height = np2oscfg.fscrn_cy;
	UINT refresh = np2oscfg.gui_fullscreen_refresh;
	UINT8 fscrnmod = np2oscfg.fscrnmod;

	if (scrnmng_get_display_mode() == mode) {
		return;
	}
	if (mode == VAEG_DISPLAY_EXCLUSIVE) {
		width = 0;
		height = 0;
		refresh = 0;
		fscrnmod = static_cast<UINT8>((fscrnmod & 3) | 4);
	}
	if (scrnmng_set_display_mode(mode, np2oscfg.gui_monitor, width, height, refresh, fscrnmod) ==
	    SUCCESS) {
		np2oscfg.gui_display_mode = static_cast<BYTE>(mode);
		if (mode == VAEG_DISPLAY_EXCLUSIVE) {
			np2oscfg.fscrn_cx = 0;
			np2oscfg.fscrn_cy = 0;
			np2oscfg.gui_fullscreen_refresh = 0;
			np2oscfg.fscrnmod = fscrnmod;
		}
	} else {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Display mode change failed: %s", SDL_GetError());
		np2oscfg.gui_display_mode = VAEG_DISPLAY_WINDOWED;
	}
	scrnmng_set_menu_height(scrnmng_get_display_mode() == VAEG_DISPLAY_EXCLUSIVE ? 0 : menu_bar_height());
	sysmng_update(SYS_UPDATEOSCFG);
}

static void open_custom_size_dialog(void) {
	g_gui.custom_size_integer = true;
	g_gui.pending_window_scale = np2oscfg.gui_scale ? np2oscfg.gui_scale : 1;
	g_gui.pending_window_width = np2oscfg.gui_window_width;
	g_gui.pending_window_height = np2oscfg.gui_window_height;
	g_gui.custom_size_request = true;
}

static void draw_custom_size_dialog(void) {
	if (g_gui.custom_size_request) {
		g_gui.custom_size_request = false;
		g_gui.custom_size_open = true;
		ImGui::OpenPopup("Custom window size##display");
	}
	if (!g_gui.custom_size_open) {
		return;
	}
	ImGui::SetNextWindowSize(ImVec2(380.0f, 200.0f), ImGuiCond_Appearing);
	if (ImGui::BeginPopupModal("Custom window size##display", &g_gui.custom_size_open,
	                           ImGuiWindowFlags_NoResize)) {
		if (ImGui::RadioButton("Integer multiplier", g_gui.custom_size_integer)) {
			g_gui.custom_size_integer = true;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Pixels", !g_gui.custom_size_integer)) {
			g_gui.custom_size_integer = false;
		}
		if (g_gui.custom_size_integer) {
			ImGui::InputInt("Multiplier (x)", &g_gui.pending_window_scale);
		} else {
			ImGui::InputInt("Logical width", &g_gui.pending_window_width);
			ImGui::InputInt("Logical height", &g_gui.pending_window_height);
		}
		const bool valid =
		    g_gui.custom_size_integer
		        ? (g_gui.pending_window_scale >= 1 &&
		           g_gui.pending_window_scale <= VAEG_WINDOW_SCALE_MAX)
		        : ((g_gui.pending_window_width >= 320) && (g_gui.pending_window_width <= 7680) &&
		           (g_gui.pending_window_height >= 240) && (g_gui.pending_window_height <= 4320));
		if (!valid) {
			if (g_gui.custom_size_integer) {
				ImGui::Text("Multiplier must be 1-%d.", VAEG_WINDOW_SCALE_MAX);
			} else {
				ImGui::TextUnformatted("Size must be between 320x240 and 7680x4320.");
			}
		}
		ImGui::BeginDisabled(!valid || (scrnmng_get_display_mode() != VAEG_DISPLAY_WINDOWED));
		if (ImGui::Button("Apply")) {
			if (g_gui.custom_size_integer) {
				set_display_scale(g_gui.pending_window_scale);
			} else {
				SDL_SetWindowSize(static_cast<SDL_Window *>(scrnmng_get_window()),
				                  g_gui.pending_window_width, g_gui.pending_window_height);
				np2oscfg.gui_window_width = static_cast<UINT16>(g_gui.pending_window_width);
				np2oscfg.gui_window_height = static_cast<UINT16>(g_gui.pending_window_height);
				np2oscfg.gui_scale = 0;
			}
			sysmng_update(SYS_UPDATEOSCFG);
			g_gui.custom_size_open = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			g_gui.custom_size_open = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

static void open_sound_buffer_dialog(void) {
	g_gui.pending_sound_buffer_ms = np2cfg.delayms;
	g_gui.sound_buffer_request = true;
}

static void draw_sound_buffer_dialog(void) {
	if (g_gui.sound_buffer_request) {
		g_gui.sound_buffer_request = false;
		g_gui.sound_buffer_open = true;
		ImGui::OpenPopup("Custom sound buffer##sound");
	}
	if (!g_gui.sound_buffer_open) {
		return;
	}
	ImGui::SetNextWindowSize(ImVec2(360.0f, 145.0f), ImGuiCond_Appearing);
	if (ImGui::BeginPopupModal("Custom sound buffer##sound", &g_gui.sound_buffer_open,
	                           ImGuiWindowFlags_NoResize)) {
		ImGui::InputInt("Buffer length (ms)", &g_gui.pending_sound_buffer_ms);
		const bool valid =
		    vaeg_sound_buffer_valid(static_cast<UINT>(g_gui.pending_sound_buffer_ms)) != FALSE;
		if (!valid) {
			ImGui::TextUnformatted("Buffer length must be between 40 and 1000 ms.");
		}
		ImGui::BeginDisabled(!valid);
		if (ImGui::Button("Apply")) {
			const UINT delayms = static_cast<UINT>(g_gui.pending_sound_buffer_ms);

			g_gui.sound_buffer_open = false;
			ImGui::CloseCurrentPopup();
			select_sound_buffer(delayms);
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			g_gui.sound_buffer_open = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

static void set_key_mode(BYTE mode) {
	np2cfg.KEY_MODE = mode;
	keystat_resetjoykey();
	sysmng_update(SYS_UPDATECFG);
}

static void set_f12_key(BYTE mode) {
	np2oscfg.F12KEY = mode;
	sdlkbd_resetf12();
	sysmng_update(SYS_UPDATEOSCFG);
}

static void set_mouse_capture(bool capture) {
	np2oscfg.MOUSE_SW = capture ? 1 : 0;
	mousemng_setcapture(capture ? TRUE : FALSE);
	sysmng_update(SYS_UPDATEOSCFG);
}

static void set_mouse_device(UINT8 device) {
	if ((device != MOUSEIFVA_JOYPAD) && (device != MOUSEIFVA_MOUSE)) {
		return;
	}
	mouseifvacfg.device = device;
	sysmng_update(SYS_UPDATECFG);
}

static void set_mouse_rapid(bool rapid) {
	np2cfg.MOUSERAPID = rapid ? 1 : 0;
	sysmng_update(SYS_UPDATECFG);
}

static void set_keyboard_layout(const char *layout) {
	kbdmap_set_layout(layout);
	g_gui.keyboard_status = "Keyboard layout: ";
	g_gui.keyboard_status += kbdmap_layout_name();
	sysmng_update(SYS_UPDATEOSCFG);
}

static void set_kana_input(const char *mode) {
	kbdmap_set_kana_input(mode);
	g_gui.keyboard_status = "Kana input: ";
	g_gui.keyboard_status += kbdmap_kana_input_name();
	sysmng_update(SYS_UPDATEOSCFG);
}

static void set_tenkey_overlay(bool enabled) {
	kbdmap_set_tenkey_overlay(enabled ? TRUE : FALSE);
	g_gui.keyboard_status = enabled ? "Tenkey overlay enabled." : "Tenkey overlay disabled.";
	sysmng_update(SYS_UPDATEOSCFG);
}

static const char *binding_name(SDL_Scancode scancode) {
	const char *name;

	if (scancode == SDL_SCANCODE_UNKNOWN) {
		return "(unassigned)";
	}
	name = SDL_GetScancodeName(scancode);
	if ((name == nullptr) || (name[0] == '\0')) {
		return "(unknown)";
	}
	return name;
}

static void reset_keyboard_to_jis(void) {
	kbdmap_reset_to_jis();
	g_gui.keyboard_status = "Keyboard map reset to JIS.";
	sysmng_update(SYS_UPDATEOSCFG);
}

static void reset_keyboard_to_us(void) {
	kbdmap_reset_to_us();
	g_gui.keyboard_status = "Keyboard map reset to US.";
	sysmng_update(SYS_UPDATEOSCFG);
}

static void begin_key_capture(int index) {
	g_gui.capture_binding = index;
	g_gui.capture_swallow = SDL_SCANCODE_UNKNOWN;
	const KBDMAP_ENTRY *entry = kbdmap_entry(index);
	g_gui.keyboard_status = "Capture: ";
	g_gui.keyboard_status += (entry != nullptr) ? entry->label : "(unknown)";
}

static void clear_key_binding(int index) {
	if (kbdmap_set_binding(index, SDL_SCANCODE_UNKNOWN) == SUCCESS) {
		g_gui.keyboard_status = "Binding cleared.";
		sysmng_update(SYS_UPDATEOSCFG);
	}
}

static void draw_keyboard_config(void) {
	if (!g_gui.keyboard_config_open) {
		return;
	}
	ImGui::SetNextWindowSize(ImVec2(760.0f, 520.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Keyboard mapping", &g_gui.keyboard_config_open)) {
		if (ImGui::Button("Reset to JIS defaults")) {
			reset_keyboard_to_jis();
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset to US defaults")) {
			reset_keyboard_to_us();
		}
		if (!g_gui.keyboard_status.empty()) {
			ImGui::TextWrapped("%s", g_gui.keyboard_status.c_str());
		}
		ImGui::Separator();
		if (ImGui::BeginTable("keyboard-map-table", 6,
		                      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
		                          ImGuiTableFlags_ScrollY,
		                      ImVec2(0.0f, 390.0f))) {
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("Role");
			ImGui::TableSetupColumn("Guest / action");
			ImGui::TableSetupColumn("Binding");
			ImGui::TableSetupColumn("Status");
			ImGui::TableSetupColumn("Capture");
			ImGui::TableSetupColumn("Clear");
			ImGui::TableHeadersRow();
			for (int i = 0; i < kbdmap_entry_count(); i++) {
				const KBDMAP_ENTRY *entry = kbdmap_entry(i);
				char guest[16];
				if (entry == nullptr) {
					continue;
				}
				if (entry->guest_code == KBDMAP_NC) {
					std::snprintf(guest, sizeof(guest), "host");
				} else {
					std::snprintf(guest, sizeof(guest), "0x%02x",
					              static_cast<unsigned int>(entry->guest_code));
				}
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(entry->label);
				ImGui::TableSetColumnIndex(1);
				ImGui::TextUnformatted(guest);
				ImGui::TableSetColumnIndex(2);
				ImGui::TextUnformatted(binding_name(kbdmap_binding(i)));
				ImGui::TableSetColumnIndex(3);
				ImGui::TextUnformatted(kbdmap_status_name(kbdmap_binding_status(i)));
				ImGui::TableSetColumnIndex(4);
				ImGui::PushID(i);
				if (g_gui.capture_binding == i) {
					ImGui::Button("...");
				} else if (ImGui::Button("Set")) {
					begin_key_capture(i);
				}
				ImGui::TableSetColumnIndex(5);
				if (ImGui::Button("Clear")) {
					clear_key_binding(i);
				}
				ImGui::PopID();
			}
			ImGui::EndTable();
		}
	}
	ImGui::End();
}

static bool font_preset_path(const char *filename, char *path, size_t path_size) {
	short attr;

	getbiospath(path, filename, static_cast<int>(path_size));
	attr = file_attr(path);
	return (attr != static_cast<short>(-1)) && ((attr & FILEATTR_DIRECTORY) == 0);
}

static void load_font_preset(const char *filename) {
	char path[MAX_PATH];
	FILEH fh;
	UINT size;

	if (!font_preset_path(filename, path, sizeof(path))) {
		g_gui.font_status = "Font not found: ";
		g_gui.font_status += filename;
		return;
	}
	fh = file_open_rb(path);
	if (fh == FILEH_INVALID) {
		g_gui.font_status = "Font open failed: ";
		g_gui.font_status += filename;
		return;
	}
	size = file_getsize(fh);
	file_close(fh);
	if (size != kV98FontRomSize) {
		g_gui.font_status = "Font size mismatch: ";
		g_gui.font_status += filename;
		return;
	}
	if (!romva_load_pc98_font(path)) {
		g_gui.font_status = "Font load failed: ";
		g_gui.font_status += filename;
		return;
	}
	file_cpyname(np2cfg.fontfile, filename, sizeof(np2cfg.fontfile));
	pccore_redraw();
	sysmng_update(SYS_UPDATECFG);
	g_gui.font_status = "Font loaded: ";
	g_gui.font_status += filename;
}

static void load_default_va_font(void) {
	const char *filename = romva_default_font_filename();

	if (!romva_load_default_font()) {
		g_gui.font_status = "VA font load failed: ";
		g_gui.font_status += filename;
		return;
	}
	np2cfg.fontfile[0] = '\0';
	pccore_redraw();
	sysmng_update(SYS_UPDATECFG);
	g_gui.font_status = "Font loaded: ";
	g_gui.font_status += filename;
}

static void draw_screen_menu(void) {
	if (ImGui::BeginMenu("画面")) {
		const char *screenshot_shortcut =
		    (np2oscfg.F12KEY == KBDMAP_F12_SCREENSHOT) ? "PrintScreen / F12" : "PrintScreen";
		if (ImGui::MenuItem("スクリーンショットを保存", screenshot_shortcut)) {
			save_screenshot(FALSE);
		}
		if (ImGui::MenuItem("スクリーンショットを保存（加工前）")) {
			save_screenshot(TRUE);
		}
		if (!g_gui.screenshot_status.empty()) {
			ImGui::TextDisabled("%s", g_gui.screenshot_status.c_str());
		}
		ImGui::Separator();
		draw_renderer_selection();
		if (!scrnmng_native_active() && ImGui::BeginMenu("エフェクト")) {
			static const char *labels[] = {"Unfiltered", "Linear", "Scanline", "CRT Lite"};
			for (int value = 0; value < VAEG_EFFECT_COUNT; value++) {
				if (ImGui::MenuItem(labels[value], nullptr, np2oscfg.gui_effect == value)) {
					set_display_effect(value);
				}
			}
			ImGui::EndMenu();
		}
		ImGui::Separator();
		if (ImGui::BeginMenu("Scaling")) {
			static const char *labels[] = {"Native", "Fit", "Fit 8-dot", "Integer", "Stretch"};
			for (int value = 0; value < VAEG_SCALING_COUNT; value++) {
				if (ImGui::MenuItem(labels[value], nullptr, np2oscfg.gui_scaling == value)) {
					set_display_scaling(value);
				}
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Window size")) {
			if (ImGui::MenuItem("Native", nullptr, np2oscfg.gui_scale == 1)) {
				set_display_scale(1);
			}
			if (ImGui::MenuItem("x2", nullptr, np2oscfg.gui_scale == 2)) {
				set_display_scale(2);
			}
			if (ImGui::MenuItem("x3", nullptr, np2oscfg.gui_scale == 3)) {
				set_display_scale(3);
			}
			if (ImGui::MenuItem("x4", nullptr, np2oscfg.gui_scale == 4)) {
				set_display_scale(4);
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Custom...", nullptr,
			                    np2oscfg.gui_scale == 0 || np2oscfg.gui_scale > 4,
			                    scrnmng_get_display_mode() == VAEG_DISPLAY_WINDOWED)) {
				open_custom_size_dialog();
			}
			ImGui::EndMenu();
		}
		bool aspect = np2oscfg.gui_aspect != 0;
		if (ImGui::MenuItem("Aspect correction", nullptr, aspect)) {
			set_display_aspect(!aspect);
		}
		const bool fullscreen = scrnmng_get_display_mode() != VAEG_DISPLAY_WINDOWED;
		if (ImGui::MenuItem("全画面表示", nullptr, fullscreen)) {
			set_display_mode(fullscreen ? VAEG_DISPLAY_WINDOWED : VAEG_DISPLAY_EXCLUSIVE);
		}
		ImGui::Separator();
		if (ImGui::BeginMenu("Frame skip")) {
			static const char *labels[] = {"Auto", "Full frame", "1/2 frame", "1/3 frame",
			                               "1/4 frame"};
			for (int value = 0; value < static_cast<int>(std::size(labels)); value++) {
				if (ImGui::MenuItem(labels[value], nullptr, np2oscfg.DRAW_SKIP == value)) {
					np2oscfg.DRAW_SKIP = static_cast<BYTE>(value);
					sysmng_update(SYS_UPDATEOSCFG);
				}
			}
			ImGui::EndMenu();
		}
		ImGui::Separator();
		menu_item_not_implemented("Rotate left/right (not implemented)");
		menu_item_not_implemented("Screen option... (not implemented)");
		ImGui::Separator();
		if (ImGui::BeginMenu("Font")) {
			char path[MAX_PATH];
			const char *va_filename = romva_default_font_filename();
			const bool va_available = font_preset_path(va_filename, path, sizeof(path));
			const bool va_selected = np2cfg.fontfile[0] == '\0';
			if (ImGui::MenuItem("VA default", nullptr, va_selected, va_available)) {
				load_default_va_font();
			}
			if (!va_available) {
				ImGui::TextDisabled("%s not found in the ROM directory", va_filename);
			}
			const bool pc98_available = font_preset_path(pc98fontromname, path, sizeof(path));
			const bool pc98_selected =
			    np2cfg.fontfile[0] && !file_cmpname(file_getname(np2cfg.fontfile), pc98fontromname);
			if (ImGui::MenuItem("98font compatibility", nullptr, pc98_selected, pc98_available)) {
				load_font_preset(pc98fontromname);
			}
			if (!pc98_available) {
				ImGui::TextDisabled("98font.rom not found in the ROM directory");
			}
			ImGui::EndMenu();
		}
		if (!g_gui.font_status.empty()) {
			ImGui::TextDisabled("%s", g_gui.font_status.c_str());
		}
		ImGui::EndMenu();
	}
}

static void draw_device_menu(void) {
	if (ImGui::BeginMenu("デバイス")) {
		if (ImGui::BeginMenu("キーボード")) {
			if (ImGui::MenuItem("Keyboard", nullptr, np2cfg.KEY_MODE == 0)) {
				set_key_mode(0);
			}
			if (ImGui::MenuItem("JoyKey-1", nullptr, np2cfg.KEY_MODE == 1)) {
				set_key_mode(1);
			}
			if (ImGui::MenuItem("JoyKey-2", nullptr, np2cfg.KEY_MODE == 2)) {
				set_key_mode(2);
			}
			if (ImGui::MenuItem("Mouse key", nullptr, np2cfg.KEY_MODE == 3)) {
				set_key_mode(3);
			}
			if (ImGui::BeginMenu("F12 binding")) {
				if (ImGui::MenuItem("Mouse", nullptr, np2oscfg.F12KEY == 0)) {
					set_f12_key(0);
				}
				if (ImGui::MenuItem("COPY", nullptr, np2oscfg.F12KEY == 1)) {
					set_f12_key(1);
				}
				if (ImGui::MenuItem("STOP", nullptr, np2oscfg.F12KEY == 2)) {
					set_f12_key(2);
				}
				if (ImGui::MenuItem("Tenkey =", nullptr, np2oscfg.F12KEY == 3)) {
					set_f12_key(3);
				}
				if (ImGui::MenuItem("Tenkey ,", nullptr, np2oscfg.F12KEY == 4)) {
					set_f12_key(4);
				}
				if (ImGui::MenuItem("PC key", nullptr, np2oscfg.F12KEY == 5)) {
					set_f12_key(5);
				}
				if (ImGui::MenuItem("Full speed (No Wait)", nullptr,
				                    np2oscfg.F12KEY == KBDMAP_F12_FULL_SPEED)) {
					set_f12_key(KBDMAP_F12_FULL_SPEED);
				}
				if (ImGui::MenuItem("スクリーンショット", nullptr,
				                    np2oscfg.F12KEY == KBDMAP_F12_SCREENSHOT)) {
					set_f12_key(KBDMAP_F12_SCREENSHOT);
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Host layout")) {
				if (ImGui::MenuItem("JIS physical", nullptr,
				                    std::string(kbdmap_layout_name()) == "jis")) {
					set_keyboard_layout("jis");
				}
				if (ImGui::MenuItem("US keytop", nullptr,
				                    std::string(kbdmap_layout_name()) == "us")) {
					set_keyboard_layout("us");
				}
				if (ImGui::MenuItem("Custom", nullptr,
				                    std::string(kbdmap_layout_name()) == "custom")) {
					set_keyboard_layout("custom");
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Kana input")) {
				if (ImGui::MenuItem("JIS Kana", nullptr,
				                    std::string(kbdmap_kana_input_name()) == "jis-kana")) {
					set_kana_input("jis-kana");
				}
				if (ImGui::MenuItem("Roman Kana", nullptr,
				                    std::string(kbdmap_kana_input_name()) == "roman")) {
					set_kana_input("roman");
				}
				ImGui::EndMenu();
			}
			{
				bool tenkey_overlay = kbdmap_tenkey_overlay_enabled() ? true : false;
				if (ImGui::MenuItem("Tenkey overlay (YUI/HJK/NM,.)", nullptr, tenkey_overlay)) {
					set_tenkey_overlay(!tenkey_overlay);
				}
			}
			if (ImGui::MenuItem("Key bindings...")) {
				g_gui.keyboard_config_open = true;
			}
			menu_item_not_implemented("Mechanical keys (not implemented)");
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("音")) {
			if (ImGui::BeginMenu("FM sound OPN/OPNA")) {
				const bool va1 = milstr_cmp(np2cfg.model, str_VA1) == 0;
				ImGui::BeginDisabled(!va1);
				if (ImGui::MenuItem("OPN (VA)", nullptr, np2cfg.SOUND_SW == FMBOARD_VA_OPN)) {
					select_sound_hardware(FMBOARD_VA_OPN);
				}
				ImGui::EndDisabled();
				if (ImGui::MenuItem("OPNA (VA2/VA3, VA + Sound Board II)", nullptr,
				                    np2cfg.SOUND_SW == FMBOARD_VA_OPNA)) {
					select_sound_hardware(FMBOARD_VA_OPNA);
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("FM sound backend")) {
				const UINT backend = opngen_getbackend();
				if (ImGui::MenuItem("NP2", nullptr, backend == OPN_BACKEND_NP2)) {
					select_opn_backend(OPN_BACKEND_NP2);
				}
				if (ImGui::MenuItem("ymfm", nullptr, backend == OPN_BACKEND_YMFM)) {
					select_opn_backend(OPN_BACKEND_YMFM);
				}
				ImGui::EndMenu();
			}
			ImGui::BeginDisabled(opngen_getbackend() != OPN_BACKEND_YMFM);
			if (ImGui::BeginMenu("ymfm fidelity")) {
				const UINT fidelity = ymfm_opn_getfidelity();
				if (ImGui::MenuItem("Minimum (~166 kHz native, default)", nullptr,
				                    fidelity == YMFMBRIDGE_FIDELITY_MINIMUM)) {
					select_ymfm_fidelity(YMFMBRIDGE_FIDELITY_MINIMUM);
				}
				if (ImGui::MenuItem("Medium (~333 kHz native)", nullptr,
				                    fidelity == YMFMBRIDGE_FIDELITY_MEDIUM)) {
					select_ymfm_fidelity(YMFMBRIDGE_FIDELITY_MEDIUM);
				}
				if (ImGui::MenuItem("Maximum (~998 kHz native, high CPU)", nullptr,
				                    fidelity == YMFMBRIDGE_FIDELITY_MAXIMUM)) {
					select_ymfm_fidelity(YMFMBRIDGE_FIDELITY_MAXIMUM);
				}
				ImGui::EndMenu();
			}
			ImGui::EndDisabled();
			if (ImGui::BeginMenu("Sampling rate")) {
				if (ImGui::MenuItem("11.025 kHz", nullptr, np2cfg.samplingrate == 11025)) {
					select_sampling_rate(11025);
				}
				if (ImGui::MenuItem("22.05 kHz", nullptr, np2cfg.samplingrate == 22050)) {
					select_sampling_rate(22050);
				}
				if (ImGui::MenuItem("44.1 kHz (Recommended)", nullptr,
				                    np2cfg.samplingrate == 44100)) {
					select_sampling_rate(44100);
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Sound buffer")) {
				bool preset_selected = false;
				for (const int preset : kSoundBufferPresets) {
					char label[32];
					const bool selected = np2cfg.delayms == preset;

					std::snprintf(label, sizeof(label), "%d ms", preset);
					preset_selected = preset_selected || selected;
					if (ImGui::MenuItem(label, nullptr, selected)) {
						select_sound_buffer(static_cast<UINT>(preset));
					}
				}
				char custom_label[48];
				std::snprintf(custom_label, sizeof(custom_label), "Custom... (%u ms)",
				              np2cfg.delayms);
				if (ImGui::MenuItem(custom_label, nullptr, !preset_selected)) {
					open_sound_buffer_dialog();
				}
				ImGui::EndMenu();
			}
			bool enabled = soundmng_isenabled() ? true : false;
			if (ImGui::MenuItem("Sound on/off", nullptr, enabled)) {
				np2oscfg.sound_enabled = enabled ? 0 : 1;
				soundmng_setenabled(np2oscfg.sound_enabled ? TRUE : FALSE);
				sysmng_update(SYS_UPDATEOSCFG);
			}
			bool motor = np2cfg.MOTOR != 0;
			if (ImGui::MenuItem("Seek/motor sound", nullptr, motor)) {
				np2cfg.MOTOR = motor ? 0 : 1;
				if (np2cfg.MOTOR == 0) {
					fddmtrsnd_stop();
				}
				sysmng_update(SYS_UPDATECFG);
			}
			int volume = np2cfg.vol_fm;
			if (ImGui::SliderInt("Master volume", &volume, 0, 128)) {
				apply_master_volume(volume);
			}
			int tape_volume = np2cfg.cmt_vol;
			if (ImGui::SliderInt("Tape volume", &tape_volume, 0, 128)) {
				np2cfg.cmt_vol = static_cast<UINT8>(tape_volume);
				cmt_setvol(np2cfg.cmt_vol);
				sysmng_update(SYS_UPDATECFG);
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("メインメモリ容量")) {
			const UINT16 capacities[] = {256, 384, 512, 640};
			for (UINT16 capacity : capacities) {
				char label[16];
				std::snprintf(label, sizeof(label), "%u KB", capacity);
				if (ImGui::MenuItem(label, nullptr, np2cfg.main_ram == capacity)) {
					save_main_ram_settings(capacity, np2cfg.main_ram_auto != 0);
				}
			}
			ImGui::Separator();
			if (ImGui::MenuItem("バックアップメモリへ自動反映", nullptr,
			                    np2cfg.main_ram_auto != 0)) {
				save_main_ram_settings(np2cfg.main_ram, np2cfg.main_ram_auto == 0);
			}
			ImGui::TextDisabled("変更は次のリセットで反映");
			ImGui::TextDisabled("現在の実装容量: %u KB", pccore_mainram_limit() / 1024);
			ImGui::EndMenu();
		}
		if (ImGui::MenuItem("I/O Bank Memory...", nullptr, bmsiocfg.enabled != FALSE)) {
			open_bms_config_dialog();
		}
		if (ImGui::MenuItem("EMS Board...", nullptr, np2cfg.EXTMEM != 0)) {
			open_ems_config_dialog();
		}

		if (ImGui::BeginMenu("Mouse")) {
			bool capture = np2oscfg.MOUSE_SW != 0;
			const char *capture_shortcut =
			    (np2oscfg.F12KEY == 0) ? "F12 / middle click" : "Middle click";
			if (ImGui::MenuItem("Capture mouse", capture_shortcut, capture)) {
				set_mouse_capture(!capture);
			}
			if (ImGui::BeginMenu("VA controller port")) {
				if (ImGui::MenuItem("Joystick", nullptr, mouseifvacfg.device == MOUSEIFVA_JOYPAD)) {
					set_mouse_device(MOUSEIFVA_JOYPAD);
				}
				if (ImGui::MenuItem("Mouse", nullptr, mouseifvacfg.device == MOUSEIFVA_MOUSE)) {
					set_mouse_device(MOUSEIFVA_MOUSE);
				}
				ImGui::EndMenu();
			}
			bool rapid = np2cfg.MOUSERAPID != 0;
			if (ImGui::MenuItem("Rapid buttons", nullptr, rapid)) {
				set_mouse_rapid(!rapid);
			}
			if (mousemng_status()[0] != '\0') {
				ImGui::Separator();
				ImGui::BeginDisabled();
				ImGui::MenuItem(mousemng_status());
				ImGui::EndDisabled();
			}
			ImGui::EndMenu();
		}
		menu_item_not_implemented("Serial option... (not implemented)");
		menu_item_not_implemented("MIDI option... (not implemented)");
		ImGui::EndMenu();
	}
}

static void open_about_dialog(void) {
	g_gui.about_open = true;
	g_gui.about_request = true;
	g_gui.about_more = false;
	g_gui.about_info[0] = '\0';
}

static void set_info_layer(UINT layer, bool enabled) {
	scrndrawva_set_layer_enabled(layer, enabled ? TRUE : FALSE);
	pccore_redraw();
}

static void draw_info_menu(void) {
	if (ImGui::BeginMenu("情報")) {
		bool show_fdd = (np2oscfg.DISPCLK & VAEG_DISPINFO_FDD) != 0;
		if (ImGui::MenuItem("Show FDD", nullptr, show_fdd)) {
			np2oscfg.DISPCLK ^= VAEG_DISPINFO_FDD;
			scrnmng_refresh_title();
			sysmng_update(SYS_UPDATEOSCFG);
		}
		bool show_fps = (np2oscfg.DISPCLK & VAEG_DISPINFO_FPS) != 0;
		if (ImGui::MenuItem("Show FPS", nullptr, show_fps)) {
			np2oscfg.DISPCLK ^= VAEG_DISPINFO_FPS;
			scrnmng_refresh_title();
			sysmng_update(SYS_UPDATEOSCFG);
		}
		bool show_cpu_clock = (np2oscfg.DISPCLK & VAEG_DISPINFO_CPU_CLOCK) != 0;
		if (ImGui::MenuItem("Show CPU clock", nullptr, show_cpu_clock)) {
			np2oscfg.DISPCLK ^= VAEG_DISPINFO_CPU_CLOCK;
			scrnmng_refresh_title();
			sysmng_update(SYS_UPDATEOSCFG);
		}
		bool show_sgp_clock = (np2oscfg.DISPCLK & VAEG_DISPINFO_SGP_CLOCK) != 0;
		if (ImGui::MenuItem("Show SGP clock", nullptr, show_sgp_clock)) {
			np2oscfg.DISPCLK ^= VAEG_DISPINFO_SGP_CLOCK;
			scrnmng_refresh_title();
			sysmng_update(SYS_UPDATEOSCFG);
		}
		bool show_frame = (np2oscfg.DISPCLK & VAEG_DISPINFO_FRAME) != 0;
		if (ImGui::MenuItem("Show frame", nullptr, show_frame)) {
			np2oscfg.DISPCLK ^= VAEG_DISPINFO_FRAME;
			scrnmng_set_framedisp((np2oscfg.DISPCLK & VAEG_DISPINFO_FRAME) ? TRUE : FALSE);
			sysmng_update(SYS_UPDATEOSCFG);
		}
		bool show_video = (np2oscfg.DISPCLK & VAEG_DISPINFO_VIDEO) != 0;
		if (ImGui::MenuItem("Show video info overlay", nullptr, show_video)) {
			np2oscfg.DISPCLK ^= VAEG_DISPINFO_VIDEO;
			sysmng_update(SYS_UPDATEOSCFG);
		}
		bool show_framebuffer = (np2oscfg.DISPCLK & VAEG_DISPINFO_FRAMEBUFFER) != 0;
		if (ImGui::MenuItem("Show FB info overlay", nullptr, show_framebuffer)) {
			np2oscfg.DISPCLK ^= VAEG_DISPINFO_FRAMEBUFFER;
			sysmng_update(SYS_UPDATEOSCFG);
		}
		ImGui::Separator();
		bool show_text = scrndrawva_layer_enabled(VAEG_VA_LAYER_TEXT) != FALSE;
		if (ImGui::MenuItem("Show text", nullptr, show_text)) {
			set_info_layer(VAEG_VA_LAYER_TEXT, !show_text);
		}
		bool show_sprite = scrndrawva_layer_enabled(VAEG_VA_LAYER_SPRITE) != FALSE;
		if (ImGui::MenuItem("Show sprite", nullptr, show_sprite)) {
			set_info_layer(VAEG_VA_LAYER_SPRITE, !show_sprite);
		}
		bool show_graphics0 = scrndrawva_layer_enabled(VAEG_VA_LAYER_GRAPHICS0) != FALSE;
		if (ImGui::MenuItem("Show graphics 0", nullptr, show_graphics0)) {
			set_info_layer(VAEG_VA_LAYER_GRAPHICS0, !show_graphics0);
		}
		bool show_graphics1 = scrndrawva_layer_enabled(VAEG_VA_LAYER_GRAPHICS1) != FALSE;
		if (ImGui::MenuItem("Show graphics 1", nullptr, show_graphics1)) {
			set_info_layer(VAEG_VA_LAYER_GRAPHICS1, !show_graphics1);
		}
		ImGui::Separator();
		if (ImGui::MenuItem("About")) {
			open_about_dialog();
		}
		ImGui::EndMenu();
	}
}

static void draw_third_party_licenses(void) {
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	const float unit = ImGui::GetFontSize();
	ImGui::SetNextWindowSize(ImVec2((std::min)(54.0f * unit, viewport->WorkSize.x * 0.95f),
	                                (std::min)(36.0f * unit, viewport->WorkSize.y * 0.95f)),
	                         ImGuiCond_Appearing);
	bool open = true;
	if (ImGui::BeginPopupModal("Third-party licenses", &open)) {
		static int selected = 0;
		ImGui::TextWrapped("Licenses and source references are embedded in this executable. "
		                   "Optional runtime notices do not indicate that a runtime is loaded.");
		ImGui::Text("Dear ImGui %s; SDL headers %d.%d.%d", IMGUI_VERSION, SDL_MAJOR_VERSION,
		            SDL_MINOR_VERSION, SDL_PATCHLEVEL);
		if (ImGui::BeginCombo("Component", vaeg_licenses[selected].name)) {
			for (int i = 0; i < IM_ARRAYSIZE(vaeg_licenses); ++i) {
				if (ImGui::Selectable(vaeg_licenses[i].name, selected == i))
					selected = i;
			}
			ImGui::EndCombo();
		}
		if (ImGui::Button("Copy text"))
			ImGui::SetClipboardText(vaeg_licenses[selected].text);
		ImGui::SameLine();
		if (ImGui::Button("Close"))
			ImGui::CloseCurrentPopup();
		const bool long_notice = std::strlen(vaeg_licenses[selected].text) > 65536;
		if (ImGui::BeginChild("license-text", ImVec2(0, 0), ImGuiChildFlags_Borders,
		                      long_notice ? ImGuiWindowFlags_HorizontalScrollbar : 0)) {
			if (!long_notice)
				ImGui::PushTextWrapPos(0.0f);
			ImGui::TextUnformatted(vaeg_licenses[selected].text);
			if (!long_notice)
				ImGui::PopTextWrapPos();
		}
		ImGui::EndChild();
		ImGui::EndPopup();
	}
}

static void draw_about_dialog(void) {
	if (g_gui.about_request) {
		g_gui.about_request = false;
		ImGui::OpenPopup("About...##vaeg");
	}
	if (!g_gui.about_open) {
		return;
	}
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	const float ui_scale = ImGui::GetFontSize() / 16.0f;
	const float width =
	    (std::min)((g_gui.about_more ? 620.0f : 360.0f) * ui_scale, viewport->WorkSize.x * 0.9f);
	const float height =
	    (std::min)((g_gui.about_more ? 700.0f : 350.0f) * ui_scale, viewport->WorkSize.y * 0.9f);
	ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
	if (ImGui::BeginPopupModal("About...##vaeg", &g_gui.about_open,
	                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
		if (g_gui.about_texture != nullptr || g_gui.native_about_texture != nullptr) {
			const float available = ImGui::GetContentRegionAvail().x;
			const float image_width = static_cast<float>(g_gui.about_texture_width);
			const float image_height = static_cast<float>(g_gui.about_texture_height);
			const float scale = (std::min)(1.0f, available / image_width);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (available - image_width * scale) * 0.5f);
			ImGui::Image(g_gui.native_about_texture ? g_gui.native_about_texture->GetTexRef()
			                                        : ImTextureRef(g_gui.about_texture),
			             ImVec2(image_width * scale, image_height * scale));
		} else {
			ImGui::TextUnformatted("88VA Eternal Grafx");
		}

		bool close_about = false;
		if (ImGui::BeginTable("about-footer", 2, ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("text", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("buttons", ImGuiTableColumnFlags_WidthFixed, 88.0f);
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("88VA Eternal Grafx  %s", VAEGREL_CORE);
			ImGui::TableNextColumn();
			close_about = ImGui::Button("OK", ImVec2(-1.0f, 0.0f));
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("PC-88VA emulator core");
			ImGui::TableNextColumn();
			ImGui::BeginDisabled(g_gui.about_more);
			if (ImGui::Button("More >>", ImVec2(-1.0f, 0.0f))) {
				np2info(g_gui.about_info, kAboutInfoTemplate, sizeof(g_gui.about_info), nullptr);
				if (memoryva_n80_exist) {
					const char *id = n80rom_label(memoryva_n80_file);
					milstr_ncat(g_gui.about_info, " (", sizeof(g_gui.about_info));
					milstr_ncat(g_gui.about_info, (id != nullptr) ? id : "unreadable",
					            sizeof(g_gui.about_info));
					milstr_ncat(g_gui.about_info, ")", sizeof(g_gui.about_info));
				}
				g_gui.about_more = true;
			}
			ImGui::EndDisabled();
			ImGui::EndTable();
		}

		if (ImGui::Button("Third-party licenses..."))
			ImGui::OpenPopup("Third-party licenses");
		draw_third_party_licenses();

		if (g_gui.about_more) {
			ImGui::SeparatorText("Running VA configuration");
			ImGui::InputTextMultiline("##runtime-info", g_gui.about_info, sizeof(g_gui.about_info),
			                          ImVec2(-1.0f, -1.0f), ImGuiInputTextFlags_ReadOnly);
		}

		if (close_about || (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Escape))) {
			g_gui.about_open = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

static void draw_state_menu(void) {
	if (ImGui::BeginMenu("状態")) {
		if (ImGui::BeginMenu("Save")) {
			for (int slot = 0; slot < kStateSlots; slot++) {
				char label[32];
				std::snprintf(label, sizeof(label), "Slot %d", slot);
				if (ImGui::MenuItem(label)) {
					std::string path = state_slot_path(slot);
					soundmng_stop();
					int ret = statsave_save(path.c_str());
					if (ret != STATFLAG_SUCCESS) {
						file_delete(path.c_str());
						g_gui.state_status = "State save failed: ";
						g_gui.state_status += path;
					} else {
						g_gui.state_status = "State saved: ";
						g_gui.state_status += path;
					}
					soundmng_play();
				}
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Load")) {
			for (int slot = 0; slot < kStateSlots; slot++) {
				char label[32];
				std::snprintf(label, sizeof(label), "Slot %d", slot);
				if (ImGui::MenuItem(label)) {
					std::string path = state_slot_path(slot);
					char error[1024];
					char override_error[1024];
					error[0] = '\0';
					override_error[0] = '\0';
					g_gui.state_force_hostfat_available = false;
					g_gui.state_force_hostfat_path.clear();
					int ret = statsave_check(path.c_str(), error, sizeof(error));
					if ((ret & ~STATFLAG_DISKCHG) != 0) {
						int override_ret = statsave_check_hostfat_override(
						    path.c_str(), override_error, sizeof(override_error));
						g_gui.state_status = "State load failed: ";
						g_gui.state_status += path;
						if (error[0] != '\0') {
							g_gui.state_status += " (";
							g_gui.state_status += error;
							g_gui.state_status += ")";
						}
						if ((override_ret & ~STATFLAG_DISKCHG) == 0) {
							g_gui.state_force_hostfat_available = true;
							g_gui.state_force_hostfat_path = path;
						}
						g_gui.state_error_request = true;
					} else {
						taskmng_clear_fast_forward();
						statsave_load(path.c_str());
						sdlkbd_reset_state();
						mousemng_reset();
						scrndrawva_redraw();
						g_gui.state_status = "State loaded: ";
						g_gui.state_status += path;
						if ((ret & STATFLAG_DISKCHG) != 0) {
							g_gui.state_status += " (disk warning ignored)";
						}
					}
				}
			}
			ImGui::EndMenu();
		}
		if (!g_gui.state_status.empty()) {
			ImGui::Separator();
			ImGui::TextWrapped("%s", g_gui.state_status.c_str());
		}
		ImGui::EndMenu();
	}
}

static void draw_hostfat_error_dialog(void) {
	if (g_gui.hostfat_error_request) {
		g_gui.hostfat_error_request = false;
		ImGui::OpenPopup("HOSTFAT error##hostfat-error");
	}
	if (ImGui::BeginPopupModal("HOSTFAT error##hostfat-error", &g_gui.hostfat_error_open,
	                           ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.25f, 0.25f, 1.0f));
		ImGui::TextUnformatted("HOSTFAT rebuild/reset failed.");
		ImGui::PopStyleColor();
		ImGui::Separator();
		ImGui::TextWrapped("%s", g_gui.hostfat_error_message.c_str());
		ImGui::Spacing();
		if (ImGui::Button("OK##hostfat-error", ImVec2(120.0f, 0.0f)) ||
		    ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			g_gui.hostfat_error_open = false;
			g_gui.hostfat_error_message.clear();
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

static void draw_state_error_dialog(void) {
	bool force_load = false;
	std::string force_path;

	if (g_gui.state_error_request) {
		g_gui.state_error_request = false;
		g_gui.state_error_open = true;
		ImGui::OpenPopup("State load rejected##state-load-error");
	}
	if (ImGui::BeginPopupModal("State load rejected##state-load-error", &g_gui.state_error_open,
	                           ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::TextWrapped("%s", g_gui.state_status.c_str());
		if (g_gui.state_force_hostfat_available) {
			ImGui::Spacing();
			ImGui::TextWrapped("This save state references a different HOSTFAT snapshot. "
			                   "Force load keeps the current HOSTFAT mount state and "
			                   "read-only snapshot. Guest-cached FAT, directory, open-file, "
			                   "or file data may no longer match.");
		}
		ImGui::Separator();
		const char *cancel_label = g_gui.state_force_hostfat_available ? "Cancel" : "OK";
		if (ImGui::Button(cancel_label, ImVec2(120.0f, 0.0f)) ||
		    ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			g_gui.state_error_open = false;
			g_gui.state_force_hostfat_available = false;
			g_gui.state_force_hostfat_path.clear();
			ImGui::CloseCurrentPopup();
		}
		if (g_gui.state_force_hostfat_available) {
			ImGui::SameLine();
			if (ImGui::Button("Force load", ImVec2(120.0f, 0.0f))) {
				force_load = true;
				force_path = g_gui.state_force_hostfat_path;
				g_gui.state_error_open = false;
				g_gui.state_force_hostfat_available = false;
				g_gui.state_force_hostfat_path.clear();
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::EndPopup();
	}
	if (force_load) {
		taskmng_clear_fast_forward();
		int ret = statsave_load_hostfat_override(force_path.c_str());
		if (ret == STATFLAG_FAILURE) {
			g_gui.state_status = "Forced state load failed: ";
			g_gui.state_status += force_path;
			g_gui.state_error_request = true;
		} else {
			sdlkbd_reset_state();
			mousemng_reset();
			scrndrawva_redraw();
			g_gui.state_status = "State loaded with HOSTFAT override: ";
			g_gui.state_status += force_path;
			if ((ret & STATFLAG_DISKCHG) != 0) {
				g_gui.state_status += " (disk warning ignored)";
			}
		}
	}
}

} // namespace
extern "C" BOOL gui_copy_screen_text(void) {
	return copy_screen_text();
}

extern "C" BOOL gui_save_screenshot(void) {
	return save_screenshot(FALSE);
}

BOOL gui_initialize(void *window, void *renderer, const char *argv0) {
	(void)argv0;
	if ((window == nullptr) || ((renderer == nullptr) && !scrnmng_native_active())) {
		return FAILURE;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImFontConfig font_config;
	font_config.FontDataOwnedByAtlas = false;
	ImFont *font = io.Fonts->AddFontFromMemoryTTF(
	    const_cast<unsigned char *>(vaeg_gui_font_ttf), static_cast<int>(vaeg_gui_font_ttf_size),
	    kGuiFontSize, &font_config, io.Fonts->GetGlyphRangesJapanese());
	if (font == nullptr) {
		std::fprintf(stderr, "Error: failed to load embedded GUI font\n");
		ImGui::DestroyContext();
		return FAILURE;
	}

	ImGui::StyleColorsDark();
	g_gui.base_style = ImGui::GetStyle();
	g_gui.ui_scale = 0.0f;
	scrnmng_set_menu_height(scrnmng_get_display_mode() == VAEG_DISPLAY_EXCLUSIVE ? 0 : menu_bar_height());
	g_gui.window = static_cast<SDL_Window *>(window);
	g_gui.native_renderer = renderer == nullptr;
	if (g_gui.native_renderer) {
		if (!ImGui_ImplSDL2_InitForOther(g_gui.window)) {
			ImGui::DestroyContext();
			return FAILURE;
		}
		if (scrnmng_native_gui_prepare() != SUCCESS) {
			ImGui_ImplSDL2_Shutdown();
			ImGui::DestroyContext();
			return FAILURE;
		}
		g_gui.initialized = true;
		load_native_about_texture();
		load_native_crt_preset();
		SDL_Log("GUI renderer: native; base font=16 pixels, DPI-scaled");
		return SUCCESS;
	}
	if (!ImGui_ImplSDL2_InitForSDLRenderer(static_cast<SDL_Window *>(window),
	                                       static_cast<SDL_Renderer *>(renderer))) {
		return FAILURE;
	}
	g_gui.renderer = static_cast<SDL_Renderer *>(renderer);
	if (!ImGui_ImplSDLRenderer2_Init(g_gui.renderer)) {
		ImGui_ImplSDL2_Shutdown();
		return FAILURE;
	}
	g_gui.about_texture =
	    load_about_texture(g_gui.renderer, &g_gui.about_texture_width, &g_gui.about_texture_height);
	if (g_gui.about_texture == nullptr) {
		std::fprintf(stderr, "Warning: failed to load embedded About image: %s\n", SDL_GetError());
	}
	g_gui.initialized = true;
	return SUCCESS;
}

void gui_shutdown(void) {
	if (!g_gui.initialized) {
		return;
	}
	mousemng_setguiblocked(TRUE);
	/* Renderer changes can tear down a GUI while its popup owns capture. */
	ImGui_ImplSDL2_SetMouseCaptureMode(ImGui_ImplSDL2_MouseCaptureMode_Disabled);
	SDL_CaptureMouse(SDL_FALSE);
	if (g_gui.text_input_active) {
		SDL_StopTextInput();
		g_gui.text_input_active = false;
	}
	if (g_gui.about_texture != nullptr) {
		SDL_DestroyTexture(g_gui.about_texture);
		g_gui.about_texture = nullptr;
	}
	if (g_gui.native_renderer)
		scrnmng_native_gui_shutdown();
	else
		ImGui_ImplSDLRenderer2_Shutdown();
	if (g_gui.native_about_texture) {
		ImGui::UnregisterUserTexture(g_gui.native_about_texture);
		IM_DELETE(g_gui.native_about_texture);
	}
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	g_gui = GuiState{};
}

static bool fullscreen_menu_mouse_zone(void) {
	if (scrnmng_get_display_mode() != VAEG_DISPLAY_EXCLUSIVE ||
	    SDL_GetRelativeMouseMode() || SDL_GetMouseFocus() != g_gui.window) return false;
	int x, y, width, height;
	SDL_GetMouseState(&x, &y);
	SDL_GetWindowSize(g_gui.window, &width, &height);
	const float limit = g_gui.fullscreen_menu.visible ?
	    static_cast<float>(menu_bar_height()) : kFullscreenRevealHeight * g_gui.ui_scale;
	return x >= 0 && x < width && y >= 0 && y < limit;
}

BOOL gui_process_event(const void *event) {
	if ((!g_gui.initialized) || (event == nullptr)) {
		return FALSE;
	}
	const SDL_Event *sdl_event = static_cast<const SDL_Event *>(event);
	ImGui_ImplSDL2_ProcessEvent(sdl_event);

	if (g_gui.capture_binding >= 0) {
		if ((sdl_event->type == SDL_KEYDOWN) && (!sdl_event->key.repeat)) {
			SDL_Scancode scancode = sdl_event->key.keysym.scancode;
			if (kbdmap_set_binding(g_gui.capture_binding, scancode) == SUCCESS) {
				const KBDMAP_ENTRY *entry = kbdmap_entry(g_gui.capture_binding);
				g_gui.keyboard_status = "Bound ";
				g_gui.keyboard_status += (entry != nullptr) ? entry->label : "(unknown)";
				g_gui.keyboard_status += " to ";
				g_gui.keyboard_status += binding_name(scancode);
				sysmng_update(SYS_UPDATEOSCFG);
			}
			g_gui.capture_swallow = scancode;
			g_gui.capture_binding = -1;
			return TRUE;
		}
		if (sdl_event->type == SDL_KEYUP) {
			return TRUE;
		}
	}
	if ((g_gui.capture_swallow != SDL_SCANCODE_UNKNOWN) && (sdl_event->type == SDL_KEYUP) &&
	    (sdl_event->key.keysym.scancode == g_gui.capture_swallow)) {
		g_gui.capture_swallow = SDL_SCANCODE_UNKNOWN;
		return TRUE;
	}

	ImGuiIO &io = ImGui::GetIO();
	switch (sdl_event->type) {
	case SDL_KEYDOWN:
	case SDL_KEYUP:
	case SDL_TEXTINPUT:
		return (io.WantCaptureKeyboard || io.WantTextInput) ? TRUE : FALSE;

	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
	case SDL_MOUSEMOTION:
	case SDL_MOUSEWHEEL:
		return (io.WantCaptureMouse || fullscreen_menu_mouse_zone()) ? TRUE : FALSE;

	default:
		return FALSE;
	}
}

BOOL gui_guest_keyboard_blocked(void) {
	if (!g_gui.initialized) {
		return FALSE;
	}
	ImGuiIO &io = ImGui::GetIO();
	if (io.WantCaptureKeyboard || io.WantTextInput || (g_gui.capture_binding >= 0)) {
		return TRUE;
	}
	return (g_gui.fdd_browser_open || g_gui.hdd_browser_open || g_gui.hostfat_browser_open ||
	        g_gui.tape_browser_open || g_gui.new_fdd_open || g_gui.new_sasi_open ||
	        g_gui.new_scsi_open || g_gui.keyboard_config_open || g_gui.configure_open ||
	        g_gui.bms_config_open || g_gui.custom_size_open || g_gui.state_error_open ||
	        g_gui.hostfat_error_open || g_gui.about_open)
	           ? TRUE
	           : FALSE;
}

BOOL gui_guest_mouse_blocked(void) {
	if (!g_gui.initialized) {
		return FALSE;
	}
	ImGuiIO &io = ImGui::GetIO();
	if (io.WantCaptureMouse || fullscreen_menu_mouse_zone() || (g_gui.capture_binding >= 0)) {
		return TRUE;
	}
	return (g_gui.fdd_browser_open || g_gui.hdd_browser_open || g_gui.hostfat_browser_open ||
	        g_gui.tape_browser_open || g_gui.new_fdd_open || g_gui.new_sasi_open ||
	        g_gui.new_scsi_open || g_gui.keyboard_config_open || g_gui.configure_open ||
	        g_gui.bms_config_open || g_gui.custom_size_open || g_gui.state_error_open ||
	        g_gui.hostfat_error_open || g_gui.about_open)
	           ? TRUE
	           : FALSE;
}

void gui_new_frame(void) {
	if (!g_gui.initialized) {
		return;
	}
	if (g_gui.native_renderer) {
		if (scrnmng_native_gui_prepare() != SUCCESS) {
			if (scrnmng_fallback_to_sdl() != SUCCESS ||
			    gui_initialize(scrnmng_get_window(), scrnmng_get_renderer(), nullptr) != SUCCESS) {
				taskmng_exit();
				return;
			}
			(void)scrnmng_take_native_fallback();
		}
	} else {
		ImGui_ImplSDLRenderer2_NewFrame();
	}
	ImGui_ImplSDL2_NewFrame();
	if (g_gui.native_renderer) {
		int width, height, pixels_width, pixels_height;
		SDL_GetWindowSize(g_gui.window, &width, &height);
		SDL_GetWindowSizeInPixels(g_gui.window, &pixels_width, &pixels_height);
		if (width > 0 && height > 0) {
			ImGui::GetIO().DisplayFramebufferScale =
			    ImVec2(static_cast<float>(pixels_width) / width,
			           static_cast<float>(pixels_height) / height);
		}
	}
	float ui_scale = 1.0f;
#if defined(_WIN32)
	const float pixel_scale = ImGui::GetIO().DisplayFramebufferScale.x;
	ui_scale = ImGui_ImplSDL2_GetContentScaleForWindow(g_gui.window) /
	           (pixel_scale > 0.0f ? pixel_scale : 1.0f);
#endif
	if (np2oscfg.gui_ui_scale >= 100 && np2oscfg.gui_ui_scale <= 300) {
		ui_scale = np2oscfg.gui_ui_scale / 100.0f;
	}
	ui_scale = (std::max)(1.0f, (std::min)(4.0f, ui_scale));
	if (std::fabs(ui_scale - g_gui.ui_scale) > 0.001f) {
		ImGuiStyle &style = ImGui::GetStyle();
		style = g_gui.base_style;
		style.ScaleAllSizes(ui_scale);
		style.FontScaleDpi = ui_scale;
		g_gui.menu_font_size = kGuiFontSize * ui_scale;
		g_gui.ui_scale = ui_scale;
		SDL_Log("GUI scale: %.2f; font=%.1f; mode=%s", ui_scale, g_gui.menu_font_size,
		        np2oscfg.gui_ui_scale == 0 ? "automatic DPI" : "manual");
	}
	ImGui::NewFrame();
	scrnmng_set_menu_height(scrnmng_get_display_mode() == VAEG_DISPLAY_EXCLUSIVE
	                            ? 0 : static_cast<int>(std::ceil(ImGui::GetFrameHeight())));
}

void gui_draw(void) {
	if (!g_gui.initialized) {
		return;
	}
	// Capture on a fresh frame without menus/dialogs; information overlays
	// are still emitted by gui_render/scrnmng_present_end.
	if (scrnmng_prepare_display_capture()) return;
	const UINT hostfat_event = hostfat_manager_poll();
	if (hostfat_event == HOSTFAT_MANAGER_EVENT_MOUNTED) {
		HOSTFAT_MANAGER_STATUS status{};
		hostfat_manager_get_status(&status);
		g_gui.hostfat_status = status.message;
		if (g_gui.hostfat_reset_after_build) {
			np2oscfg.hostfat_enabled = 1;
			milstr_ncpy(np2oscfg.hostfat_dir, g_gui.hostfat_rebuild_dir.c_str(),
			            sizeof(np2oscfg.hostfat_dir));
			sysmng_update(SYS_UPDATEOSCFG);
			g_gui.hostfat_rebuild_dir.clear();
			g_gui.hostfat_reset_after_build = false;
			reset_guest();
		}
	} else if (hostfat_event == HOSTFAT_MANAGER_EVENT_FAILED) {
		HOSTFAT_MANAGER_STATUS status{};
		hostfat_manager_get_status(&status);
		g_gui.hostfat_status = "HOSTFAT rebuild failed: ";
		g_gui.hostfat_status += status.message;
		g_gui.hostfat_rebuild_dir.clear();
		g_gui.hostfat_reset_after_build = false;
		g_gui.hostfat_error_message = g_gui.hostfat_status;
		g_gui.hostfat_error_open = true;
		g_gui.hostfat_error_request = true;
		g_gui.configure_open = true;
		g_gui.configure_request = true;
	}
	const ImGuiIO &io = ImGui::GetIO();
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	const float mouse_y = io.MousePos.y - viewport->Pos.y;
	const bool mouse_in_window = SDL_GetMouseFocus() == g_gui.window &&
	    !SDL_GetRelativeMouseMode() && io.MousePos.x >= viewport->Pos.x &&
	    io.MousePos.x < viewport->Pos.x + viewport->Size.x && mouse_y >= 0;
	const bool show_menu = update_fullscreen_menu(
	    g_gui.fullscreen_menu, scrnmng_get_display_mode() == VAEG_DISPLAY_EXCLUSIVE,
	    mouse_in_window && mouse_y < kFullscreenRevealHeight * g_gui.ui_scale,
	    mouse_in_window && mouse_y < ImGui::GetFrameHeight(),
	    ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel) ||
	        ImGui::IsAnyItemActive(), SDL_GetTicks());
	if (show_menu && ImGui::BeginMainMenuBar()) {
		const bool paused = taskmng_ispaused() ? true : false;
		if (ImGui::Button(paused ? "Resume" : "Pause")) {
			taskmng_toggle_pause();
		}
		ImGui::Separator();
		draw_emulate_menu();
		draw_fdd_menu();
		draw_harddisk_menu();
		draw_tape_menu();
		draw_edit_menu();
		draw_screen_menu();
		draw_device_menu();
		draw_state_menu();
		draw_info_menu();
		ImGui::EndMainMenuBar();
	}
	if (!show_menu && fullscreen_hint_visible(g_gui.fullscreen_menu, SDL_GetTicks())) {
		ImGui::SetNextWindowPos(
		    ImVec2(viewport->Pos.x + viewport->Size.x * 0.5f,
		           viewport->Pos.y + ImGui::GetFrameHeight() + 8.0f * g_gui.ui_scale),
		    ImGuiCond_Always, ImVec2(0.5f, 0.0f));
		ImGui::SetNextWindowBgAlpha(0.85f);
		if (ImGui::Begin("##fullscreen_hint", nullptr,
		                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
		                 ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings |
		                 ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav)) {
			ImGui::TextUnformatted("画面上端でメニュー表示");
		}
		ImGui::End();
	}
	draw_state_error_dialog();
	draw_fdd_browser();
	draw_hdd_browser();
	draw_tape_browser();
	draw_new_fdd_dialog();
	draw_new_sasi_dialog();
	draw_new_scsi_dialog();
	draw_keyboard_config();
	draw_configure_dialog();
	draw_hostfat_error_dialog();
	draw_bms_config_dialog();
	draw_ems_config_dialog();
	draw_custom_size_dialog();
	draw_native_crt_settings();
	draw_sound_buffer_dialog();
	draw_about_dialog();
}

void gui_render(void) {
	if (!g_gui.initialized) {
		return;
	}
	if (g_gui.native_renderer) scrnmng_draw_native_overlays();
	ImGui::Render();
	update_text_input_state();
	if (!g_gui.native_renderer) {
		scrnmng_draw_sdl_overlays();
		ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), g_gui.renderer);
	}
}

void gui_display_capture_result(const char *path, BOOL success) {
	g_gui.screenshot_status = success == SUCCESS ? "Screenshot saved: " : "Screenshot failed: ";
	g_gui.screenshot_status += path;
	if (success != SUCCESS) {
		g_gui.screenshot_status += " / ";
		g_gui.screenshot_status += SDL_GetError();
	}
	SDL_Log("%s", g_gui.screenshot_status.c_str());
}

static void draw_overlay_rect(ImDrawList *list, ImVec2 scale, ImVec2 origin,
                              int x, int y, int width, int height, ImU32 color) {
	if (scale.x <= 0 || scale.y <= 0) return;
	list->AddRectFilled(
	    ImVec2(origin.x + x / scale.x, origin.y + y / scale.y),
	    ImVec2(origin.x + (x + width) / scale.x, origin.y + (y + height) / scale.y),
	    color);
}

void gui_overlay_rect(int x, int y, int width, int height,
                      unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
	if (!g_gui.initialized || !g_gui.native_renderer) return;
	draw_overlay_rect(ImGui::GetBackgroundDrawList(), ImGui::GetIO().DisplayFramebufferScale,
	                  ImGui::GetMainViewport()->Pos, x, y, width, height, IM_COL32(r, g, b, a));
}

BOOL gui_overlay_selftest(void) {
	FullscreenMenuState menu;
	update_fullscreen_menu(menu, true, false, false, false, 100);
	if (!fullscreen_hint_visible(menu, 3099) || fullscreen_hint_visible(menu, 3100)) {
		std::fprintf(stderr, "selftest: FULLSCREEN_HINT_DURATION\n");
		return FAILURE;
	}
	update_fullscreen_menu(menu, false, false, false, false, 200);
	if (fullscreen_hint_visible(menu, 201)) return FAILURE;
	if (!update_fullscreen_menu(menu, false, false, false, false, 0) ||
	    update_fullscreen_menu(menu, true, false, false, false, 1) ||
	    !update_fullscreen_menu(menu, true, true, false, false, 10) ||
	    !update_fullscreen_menu(menu, true, false, false, false, 509) ||
	    update_fullscreen_menu(menu, true, false, false, false, 510) ||
	    !update_fullscreen_menu(menu, true, true, false, false, 600) ||
	    !update_fullscreen_menu(menu, true, false, false, true, 2000) ||
	    !update_fullscreen_menu(menu, true, false, true, false, 3000) ||
	    update_fullscreen_menu(menu, true, false, false, false, 3500) ||
	    !update_fullscreen_menu(menu, false, false, false, false, 3501) ||
	    update_fullscreen_menu(menu, true, false, false, false, 3502)) {
		std::fprintf(stderr, "selftest: FULLSCREEN_MENU_POLICY_MISMATCH\n");
		return FAILURE;
	}
	update_fullscreen_menu(menu, true, true, false, false, 0xffffff00U);
	if (update_fullscreen_menu(menu, true, false, false, false, 0x00000100U)) {
		std::fprintf(stderr, "selftest: FULLSCREEN_MENU_TIMER_WRAP\n");
		return FAILURE;
	}
	ImGuiContext *saved = ImGui::GetCurrentContext();
	ImGuiContext *context = ImGui::CreateContext();
	ImGui::SetCurrentContext(context);
	ImGuiIO &io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.DisplaySize = ImVec2(640, 400);
	io.Fonts->Build();
	ImGui::NewFrame();
	ImDrawList *list = ImGui::GetBackgroundDrawList();
	const ImU32 color = IM_COL32(255, 255, 192, 255);
	BOOL result = SUCCESS;
	for (int scale = 1; scale <= 2; ++scale) {
		const int first = list->VtxBuffer.Size;
		draw_overlay_rect(list, ImVec2(scale, scale), ImVec2(3, 5), 20, 40, 8, 8, color);
		if (list->VtxBuffer.Size != first + 4 ||
		    list->VtxBuffer[first].pos.x != 3 + 20.0f / scale ||
		    list->VtxBuffer[first].pos.y != 5 + 40.0f / scale ||
		    list->VtxBuffer[first + 2].pos.x != 3 + 28.0f / scale ||
		    list->VtxBuffer[first + 2].pos.y != 5 + 48.0f / scale ||
		    list->VtxBuffer[first].col != color) result = FAILURE;
	}
	ImGui::EndFrame();
	ImGui::DestroyContext(context);
	ImGui::SetCurrentContext(saved);
	if (result != SUCCESS) std::fprintf(stderr, "selftest: OVERLAY_GEOMETRY_MISMATCH\n");
	return result;
}
