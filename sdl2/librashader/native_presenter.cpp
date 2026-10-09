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
#include "librashader/native_presenter.h"

#include <cstdio>

namespace vaeg::librashader {

bool presenter_state_transition_allowed(PresenterState from, PresenterState to) noexcept {
	if (from == to) {
		return true;
	}
	switch (from) {
	case PresenterState::Unavailable:
		return to == PresenterState::Initializing;
	case PresenterState::Initializing:
		return (to == PresenterState::Unavailable) || (to == PresenterState::PassThrough) ||
		       (to == PresenterState::Filtered);
	case PresenterState::PassThrough:
		return (to == PresenterState::Filtered) || (to == PresenterState::Initializing) ||
		       (to == PresenterState::Unavailable);
	case PresenterState::Filtered:
		return (to == PresenterState::PassThrough) || (to == PresenterState::Initializing) ||
		       (to == PresenterState::Unavailable);
	default:
		return false;
	}
}

const char *presenter_state_name(PresenterState state) noexcept {
	switch (state) {
	case PresenterState::Unavailable:
		return "unavailable";
	case PresenterState::Initializing:
		return "initializing";
	case PresenterState::PassThrough:
		return "pass-through";
	case PresenterState::Filtered:
		return "filtered";
	default:
		return "unknown";
	}
}

const char *presenter_result_name(PresenterResult result) noexcept {
	switch (result) {
	case PresenterResult::Presented:
		return "presented";
	case PresenterResult::Disabled:
		return "disabled";
	case PresenterResult::Recovered:
		return "recovered";
	case PresenterResult::Fallback:
		return "fallback";
	case PresenterResult::Failed:
		return "failed";
	default:
		return "unknown";
	}
}

const char *presenter_error_name(PresenterError error) noexcept {
	switch (error) {
	case PresenterError::None:
		return "none";
	case PresenterError::InvalidFrame:
		return "invalid_frame";
	case PresenterError::PlatformUnavailable:
		return "platform_unavailable";
	case PresenterError::RuntimeMissing:
		return "runtime_missing";
	case PresenterError::AbiMismatch:
		return "abi_mismatch";
	case PresenterError::DeviceFailure:
		return "device_failure";
	case PresenterError::PresetFailure:
		return "preset_failure";
	case PresenterError::FilterFailure:
		return "filter_failure";
	case PresenterError::ResourceFailure:
		return "resource_failure";
	default:
		return "unknown";
	}
}

std::size_t NativePresenter::filter_parameter_count() const noexcept {
	return preset_.parameters().size();
}

const ShaderParameterInfo *NativePresenter::filter_parameter(std::size_t index) const noexcept {
	return preset_.parameters().at(index);
}

bool NativePresenter::prepare_filter_parameters(const char *preset_path, bool enable_filter,
                                                const char *parameter_state_path) noexcept {
	try {
		preset_error_.clear();
		parameter_state_path_.clear();
		if (parameter_state_path != nullptr) {
			parameter_state_path_ = parameter_state_path;
		}
		preset_.clear();
		if (!enable_filter) {
			return true;
		}
		std::string error;
		if (!preset_.load(preset_path, &error)) {
			preset_error_ = error;
			if (!error.empty()) {
				std::fprintf(stderr, "VAEG shader preset unavailable: %s\n", error.c_str());
			}
			return false;
		}
		if (!(parameter_state_path_.empty()
		          ? preset_.parameters().load_config()
		          : preset_.parameters().load_values(parameter_state_path_.c_str()))) {
			std::fprintf(stderr, "VAEG shader parameter state is invalid; using preset defaults\n");
		}
		return true;
	} catch (...) {
		preset_.clear();
		parameter_state_path_.clear();
		return false;
	}
}

bool NativePresenter::apply_filter_parameters() noexcept {
	for (std::size_t i = 0; i < preset_.parameters().size(); ++i) {
		const ShaderParameterInfo *parameter = preset_.parameters().at(i);
		if ((parameter == nullptr) ||
		    !apply_backend_filter_parameter(parameter->name.c_str(), parameter->value)) {
			return false;
		}
	}
	return true;
}

PresenterResult NativePresenter::set_filter_parameter(const char *name, float value) noexcept {
	if ((state() != PresenterState::PassThrough) && (state() != PresenterState::Filtered)) {
		return PresenterResult::Fallback;
	}
	for (std::size_t i = 0; i < preset_.parameters().size(); ++i) {
		const ShaderParameterInfo *parameter = preset_.parameters().at(i);
		if ((parameter != nullptr) && (parameter->name == ((name != nullptr) ? name : ""))) {
			const float old_value = parameter->value;
			float applied = old_value;
			if (!preset_.parameters().set_value_at(i, value, &applied) ||
			    !apply_backend_filter_parameter(parameter->name.c_str(), applied)) {
				preset_.parameters().restore_value(i, old_value);
				return PresenterResult::Fallback;
			}
			if (!(parameter_state_path_.empty()
			          ? preset_.parameters().save_config()
			          : preset_.parameters().save_values(parameter_state_path_.c_str()))) {
				return PresenterResult::Failed;
			}
			return PresenterResult::Recovered;
		}
	}
	return PresenterResult::Failed;
}

PresenterResult NativePresenter::reset_filter_parameters() noexcept {
	if ((state() != PresenterState::PassThrough) && (state() != PresenterState::Filtered)) {
		return PresenterResult::Fallback;
	}
	preset_.parameters().reset();
	if (!apply_filter_parameters()) {
		return PresenterResult::Fallback;
	}
	if (!(parameter_state_path_.empty()
	          ? preset_.parameters().save_config()
	          : preset_.parameters().save_values(parameter_state_path_.c_str()))) {
		return PresenterResult::Failed;
	}
	return PresenterResult::Recovered;
}

} // namespace vaeg::librashader
