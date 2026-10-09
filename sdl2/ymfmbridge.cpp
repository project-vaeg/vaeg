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
#include "ymfm_opn.h"

#include <cstring>
#include <vector>

#include "compiler.h"
#include "sound.h"
#include "opngen.h"
#include "ymfmbridge.h"

namespace {

constexpr uint32_t kYm2203Clock = 3993552;
constexpr uint32_t kYm2608Clock = kYm2203Clock * 2;
constexpr unsigned kChipSlots = 2;
constexpr uint32_t kStateVersion = 1;

ymfm::opn_fidelity native_fidelity(UINT fidelity) {
	switch (fidelity) {
	case YMFMBRIDGE_FIDELITY_MEDIUM:
		return ymfm::OPN_FIDELITY_MED;
	case YMFMBRIDGE_FIDELITY_MAXIMUM:
		return ymfm::OPN_FIDELITY_MAX;
	default:
		return ymfm::OPN_FIDELITY_MIN;
	}
}

class vaeg_ymfm_interface : public ymfm::ymfm_interface {
  public:
	void timer_expired(uint32_t timer) {
		m_engine->engine_timer_expired(timer);
	}

	uint8_t ymfm_external_read(ymfm::access_class, uint32_t) override {
		return 0;
	}
};

struct ymfm_slot {
	vaeg_ymfm_interface opn_intf;
	ymfm::ym2203 opn;
	vaeg_ymfm_interface opna_intf;
	ymfm::ym2608 opna;
	bool active;
	bool use_opna;
	bool stereo;
	uint64_t phase;
	int32_t last_left;
	int32_t last_right;

	ymfm_slot()
	    : opn(opn_intf), opna(opna_intf), active(false), use_opna(true), stereo(true), phase(0),
	      last_left(0), last_right(0) {
		opn.set_fidelity(ymfm::OPN_FIDELITY_MIN);
		opna.set_fidelity(ymfm::OPN_FIDELITY_MIN);
	}
};

ymfm_slot g_slots[kChipSlots];
uint32_t g_output_rate = 44100;
uint32_t g_volume = 64;
UINT g_fidelity = YMFMBRIDGE_FIDELITY_DEFAULT;
bool g_vr_enabled = false;
int32_t g_vr_left = 0;
int32_t g_vr_right = 0;

void write_opn(ymfm::ym2203 &chip, uint8_t reg, uint8_t value) {
	chip.write_address(reg);
	chip.write_data(value);
}

void write_opna(ymfm::ym2608 &chip, bool high, uint8_t reg, uint8_t value) {
	if (high) {
		chip.write_address_hi(reg);
		chip.write_data_hi(value);
	} else {
		chip.write_address(reg);
		chip.write_data(value);
	}
}

void generate_slot(ymfm_slot &slot, int64_t &left, int64_t &right) {
	if (!slot.active) {
		return;
	}

	const uint32_t native_rate =
	    slot.use_opna ? slot.opna.sample_rate(kYm2608Clock) : slot.opn.sample_rate(kYm2203Clock);
	slot.phase += native_rate;
	uint32_t samples = static_cast<uint32_t>(slot.phase / g_output_rate);
	slot.phase %= g_output_rate;
	if (samples != 0) {
		int64_t sum_left = 0;
		int64_t sum_right = 0;
		for (uint32_t index = 0; index < samples; index++) {
			if (slot.use_opna) {
				ymfm::ym2608::output_data output;
				slot.opna.generate(&output);
				sum_left += output.data[0];
				sum_right += output.data[1];
			} else {
				ymfm::ym2203::output_data output;
				slot.opn.generate(&output);
				sum_left += output.data[0];
				sum_right += output.data[0];
			}
		}
		slot.last_left = static_cast<int32_t>(sum_left / samples);
		slot.last_right = static_cast<int32_t>(sum_right / samples);
	}

	if (!slot.stereo) {
		const int32_t mono = (slot.last_left + slot.last_right) / 2;
		left += mono;
		right += mono;
	} else {
		left += slot.last_left;
		right += slot.last_right;
	}
}

void state_u64(ymfm::ymfm_saved_state &state, uint64_t &value) {
	uint32_t low = static_cast<uint32_t>(value);
	uint32_t high = static_cast<uint32_t>(value >> 32);
	state.save_restore(low);
	state.save_restore(high);
	if (!state.saving()) {
		value = (static_cast<uint64_t>(high) << 32) | low;
	}
}

void save_restore_state(std::vector<uint8_t> &buffer, bool saving) {
	ymfm::ymfm_saved_state state(buffer, saving);
	uint32_t version = kStateVersion;
	state.save_restore(version);
	state.save_restore(g_output_rate);
	state.save_restore(g_volume);
	state.save_restore(g_fidelity);
	state.save_restore(g_vr_enabled);
	state.save_restore(g_vr_left);
	state.save_restore(g_vr_right);

	for (ymfm_slot &slot : g_slots) {
		uint8_t active = slot.active ? 1 : 0;
		uint8_t use_opna = slot.use_opna ? 1 : 0;
		uint8_t stereo = slot.stereo ? 1 : 0;
		state.save_restore(active);
		state.save_restore(use_opna);
		state.save_restore(stereo);
		state_u64(state, slot.phase);
		state.save_restore(slot.last_left);
		state.save_restore(slot.last_right);
		if (!saving) {
			slot.active = active != 0;
			slot.use_opna = use_opna != 0;
			slot.stereo = stereo != 0;
		}
	}

	if (!saving) {
		for (ymfm_slot &slot : g_slots) {
			slot.opn.set_fidelity(native_fidelity(g_fidelity));
			slot.opna.set_fidelity(native_fidelity(g_fidelity));
		}
	}
	for (ymfm_slot &slot : g_slots) {
		slot.opn.save_restore(state);
		slot.opna.save_restore(state);
	}
}

} // namespace

extern "C" void ymfm_opn_initialize(UINT rate) {
	g_output_rate = (rate != 0) ? rate : 44100;
	ymfm_opn_reset();
}

extern "C" UINT ymfm_opn_parsefidelity(const char *name) {
	if ((name != nullptr) && (std::strcmp(name, "medium") == 0)) {
		return YMFMBRIDGE_FIDELITY_MEDIUM;
	}
	if ((name != nullptr) && (std::strcmp(name, "maximum") == 0)) {
		return YMFMBRIDGE_FIDELITY_MAXIMUM;
	}
	return YMFMBRIDGE_FIDELITY_DEFAULT;
}

extern "C" const char *ymfm_opn_fidelityname(UINT fidelity) {
	switch (fidelity) {
	case YMFMBRIDGE_FIDELITY_MEDIUM:
		return "medium";
	case YMFMBRIDGE_FIDELITY_MAXIMUM:
		return "maximum";
	default:
		return "minimum";
	}
}

extern "C" void ymfm_opn_setfidelity(UINT fidelity) {
	if (fidelity >= YMFMBRIDGE_FIDELITY_COUNT) {
		fidelity = YMFMBRIDGE_FIDELITY_DEFAULT;
	}
	g_fidelity = fidelity;
	for (ymfm_slot &slot : g_slots) {
		slot.opn.set_fidelity(native_fidelity(fidelity));
		slot.opna.set_fidelity(native_fidelity(fidelity));
		slot.phase = 0;
		slot.last_left = 0;
		slot.last_right = 0;
	}
}

extern "C" UINT ymfm_opn_getfidelity(void) {
	return g_fidelity;
}

extern "C" void ymfm_opn_setvol(UINT vol) {
	g_volume = vol;
}

extern "C" void ymfm_opn_setvr(REG8 channel, REG8 value) {
	if ((channel & 3) && value) {
		g_vr_enabled = true;
		g_vr_left = (channel & 1) ? value : 0;
		g_vr_right = (channel & 2) ? value : 0;
	} else {
		g_vr_enabled = false;
		g_vr_left = 0;
		g_vr_right = 0;
	}
}

extern "C" void ymfm_opn_reset(void) {
	for (ymfm_slot &slot : g_slots) {
		slot.opn.reset();
		slot.opna.reset();
		slot.active = false;
		slot.use_opna = true;
		slot.stereo = true;
		slot.phase = 0;
		slot.last_left = 0;
		slot.last_right = 0;
	}
}

extern "C" void ymfm_opn_setcfg(REG8 maxch, UINT flag) {
	for (unsigned index = 0; index < kChipSlots; index++) {
		ymfm_slot &slot = g_slots[index];
		const unsigned first_channel = index * 6;
		const unsigned channels =
		    (maxch > first_channel) ? min(static_cast<unsigned>(maxch - first_channel), 6U) : 0;
		slot.active = channels != 0;
		slot.use_opna = (channels > 3) || ((flag & OPN_CHMASK) != 0);
		slot.stereo = (flag & OPN_CHMASK) == OPN_STEREO;
		slot.phase = 0;
		slot.last_left = 0;
		slot.last_right = 0;
		write_opna(slot.opna, false, 0x29, channels > 3 ? 0x80 : 0x00);
	}
}

extern "C" void ymfm_opn_setextch(UINT chnum, REG8 data) {
	const unsigned chip = chnum / 6;
	if ((chip >= kChipSlots) || ((chnum % 6) != 2)) {
		return;
	}
	write_opn(g_slots[chip].opn, 0x27, data);
	write_opna(g_slots[chip].opna, false, 0x27, data);
}

extern "C" void ymfm_opn_setcontrol(REG8 chbase, REG8 reg, REG8 value) {
	const unsigned chip = chbase / 6;
	if (chip >= kChipSlots) {
		return;
	}
	write_opn(g_slots[chip].opn, reg, value);
	write_opna(g_slots[chip].opna, false, reg, value);
}

extern "C" void ymfm_opn_setreg(REG8 chbase, REG8 reg, REG8 value) {
	const unsigned chip = chbase / 6;
	const bool high = (chbase % 6) >= 3;
	if (chip >= kChipSlots) {
		return;
	}
	if (!high) {
		write_opn(g_slots[chip].opn, reg, value);
	}
	write_opna(g_slots[chip].opna, high, reg, value);
}

extern "C" void ymfm_opn_keyon(UINT chnum, REG8 value) {
	const unsigned chip = chnum / 6;
	const unsigned channel = chnum % 6;
	if (chip >= kChipSlots) {
		return;
	}
	if (channel < 3) {
		write_opn(g_slots[chip].opn, 0x28, value);
	}
	write_opna(g_slots[chip].opna, false, 0x28, value);
}

extern "C" void ymfm_opn_timerover(UINT timer) {
	for (ymfm_slot &slot : g_slots) {
		if (slot.active) {
			slot.opn_intf.timer_expired(timer);
			slot.opna_intf.timer_expired(timer);
		}
	}
}

extern "C" void ymfm_opn_getpcm(SINT32 *pcm, UINT count, BOOL use_vr) {
	while (count--) {
		int64_t left = 0;
		int64_t right = 0;
		for (ymfm_slot &slot : g_slots) {
			generate_slot(slot, left, right);
		}
		if (use_vr && g_vr_enabled) {
			const int64_t cross_left = (right >> 5) * g_vr_right;
			const int64_t cross_right = (left >> 5) * g_vr_left;
			left += cross_left;
			right += cross_right;
		}
		pcm[0] += static_cast<SINT32>(left * g_volume / 64);
		pcm[1] += static_cast<SINT32>(right * g_volume / 64);
		pcm += 2;
	}
}

extern "C" UINT32 ymfm_opn_state_size(void) {
	std::vector<uint8_t> state;
	save_restore_state(state, true);
	return static_cast<UINT32>(state.size());
}

extern "C" int ymfm_opn_state_save(void *buffer, UINT32 size) {
	std::vector<uint8_t> state;
	if ((buffer == nullptr) || (size != ymfm_opn_state_size())) {
		return (-1);
	}
	save_restore_state(state, true);
	std::memcpy(buffer, state.data(), state.size());
	return (0);
}

extern "C" int ymfm_opn_state_load(const void *buffer, UINT32 size) {
	std::vector<uint8_t> state;
	uint32_t version;

	if ((buffer == nullptr) || (size != ymfm_opn_state_size())) {
		return (-1);
	}
	state.assign(static_cast<const uint8_t *>(buffer), static_cast<const uint8_t *>(buffer) + size);
	if (state.size() < sizeof(version)) {
		return (-1);
	}
	version = static_cast<uint32_t>(state[0]) | (static_cast<uint32_t>(state[1]) << 8) |
	          (static_cast<uint32_t>(state[2]) << 16) | (static_cast<uint32_t>(state[3]) << 24);
	if (version != kStateVersion) {
		return (-1);
	}
	save_restore_state(state, false);
	return (0);
}
