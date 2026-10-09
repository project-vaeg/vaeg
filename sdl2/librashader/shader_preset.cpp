/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "librashader/shader_preset.h"

#include <cstdio>
#include <utility>

#include "librashader/librashader_loader.h"

namespace vaeg::librashader {

namespace {

static void set_error(std::string *message, const char *text) {
	if (message != nullptr) {
		*message = (text != nullptr) ? text : "unknown librashader error";
	}
}

static void release_error(const libra_instance_t &instance, libra_error_t error) {
	if (error == nullptr) {
		return;
	}
	if (instance.error_print != nullptr) {
		(void)instance.error_print(error);
	}
	if (instance.error_free != nullptr) {
		(void)instance.error_free(&error);
	}
}

} // namespace

void ShaderPreset::clear() noexcept {
	parameters_.clear();
}

bool ShaderPreset::load(const char *path, std::string *error_message) {
	ShaderParameterSet candidate;
	libra_instance_t instance{};
	libra_shader_preset_t preset = nullptr;
	libra_preset_param_list_t list{};
	libra_error_t error;

	clear();
	if ((path == nullptr) || (path[0] == '\0')) {
		set_error(error_message, "shader preset path is empty");
		return false;
	}
	try {
		char load_error[512];
		instance = vaeg_librashader_load_instance(load_error, sizeof(load_error));
		if (!instance.instance_loaded || (instance.preset_create == nullptr) ||
		    (instance.preset_get_runtime_params == nullptr) ||
		    (instance.preset_free_runtime_params == nullptr) || (instance.preset_free == nullptr)) {
			set_error(error_message, load_error[0] ? load_error : "librashader runtime is unavailable");
			return false;
		}
		error = instance.preset_create(path, &preset);
		if ((error != nullptr) || (preset == nullptr)) {
			char *detail = nullptr;
			(void)instance.error_write(error, &detail);
			set_error(error_message, detail ? detail : "shader preset creation failed");
			if (detail) (void)instance.error_free_string(&detail);
			release_error(instance, error);
			return false;
		}
		error = instance.preset_get_runtime_params(&preset, &list);
		if (error != nullptr) {
			char *detail = nullptr;
			if (instance.error_write != nullptr && instance.error_free_string != nullptr) {
				(void)instance.error_write(error, &detail);
			}
			set_error(error_message, detail ? detail : "shader parameter enumeration failed");
			if (detail != nullptr) {
				(void)instance.error_free_string(&detail);
			}
			release_error(instance, error);
			(void)instance.preset_free(&preset);
			return false;
		}
		if ((list.length > 0) && (list.parameters == nullptr)) {
			(void)instance.preset_free(&preset);
			set_error(error_message, "shader preset returned invalid parameter metadata");
			return false;
		}
		if (list.length > ShaderParameterSet::kMaximumParameters) {
			(void)instance.preset_free_runtime_params(list);
			(void)instance.preset_free(&preset);
			set_error(error_message, "shader preset exposes too many parameters");
			return false;
		}
		for (uint64_t i = 0; i < list.length; ++i) {
			const libra_preset_param_t &parameter = list.parameters[i];
			if (!candidate.add(parameter.name, parameter.description, parameter.initial,
		                      parameter.minimum, parameter.maximum, parameter.step)) {
				(void)instance.preset_free_runtime_params(list);
				(void)instance.preset_free(&preset);
				set_error(error_message, "shader preset contains invalid parameter metadata");
				return false;
			}
		}
		(void)instance.preset_free_runtime_params(list);
		(void)instance.preset_free(&preset);
		parameters_ = std::move(candidate);
		return true;
	} catch (...) {
		if (preset != nullptr) {
			(void)instance.preset_free(&preset);
		}
		set_error(error_message, "shader preset metadata allocation failed");
		return false;
	}
}

} // namespace vaeg::librashader
