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
#include "librashader/shader_parameters.h"

#include <cmath>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>
#include "librashader/shader_config.h"

namespace vaeg::librashader {

namespace {

constexpr const char kStateHeader[] = "VAEG_SHADER_PARAMETERS 1";
constexpr std::size_t kMaximumNameLength = 256;
constexpr std::size_t kMaximumDescriptionLength = 512;
constexpr std::size_t kMaximumStateLineLength = 1024;
char *config_buffer = nullptr;
std::size_t config_capacity = 0;
using ParameterValues = std::vector<std::pair<std::string, float>>;

static bool read_parameter_values(std::istream &input, ParameterValues &pending) {
	std::string line;
	if (!std::getline(input, line) || line != kStateHeader)
		return false;
	while (std::getline(input, line)) {
		if (line.empty())
			continue;
		if (line.size() > kMaximumStateLineLength ||
		    pending.size() >= ShaderParameterSet::kMaximumParameters)
			return false;
		const auto equals = line.find('=');
		if (equals == std::string::npos || equals == 0 || equals > kMaximumNameLength)
			return false;
		const std::string name = line.substr(0, equals);
		if (name.find_first_of(";\r\n\"") != std::string::npos)
			return false;
		std::istringstream value_stream(line.substr(equals + 1));
		float value;
		char trailing;
		if (!(value_stream >> value) || (value_stream >> trailing) || !std::isfinite(value))
			return false;
		pending.emplace_back(name, value);
	}
	return !input.bad();
}

static bool store_config_values(const ParameterValues &values) {
	if (config_buffer == nullptr)
		return true;
	std::ostringstream output;
	output << kStateHeader << ';' << std::setprecision(9);
	for (const auto &entry : values) {
		if (entry.first.find_first_of(";=\r\n\"") != std::string::npos ||
		    !std::isfinite(entry.second))
			return false;
		output << entry.first << '=' << entry.second << ';';
	}
	const std::string text = output.str();
	if (text.size() >= config_capacity)
		return false;
	std::memcpy(config_buffer, text.c_str(), text.size() + 1);
	return true;
}

static bool valid_range(float initial, float minimum, float maximum, float step) noexcept {
	return std::isfinite(initial) && std::isfinite(minimum) && std::isfinite(maximum) &&
	       std::isfinite(step) && (minimum <= maximum) && (step > 0.0f);
}

static bool has_valid_name(const char *name) noexcept {
	return (name != nullptr) && (name[0] != '\0') &&
	       (std::char_traits<char>::length(name) <= kMaximumNameLength);
}

static bool has_valid_description(const char *description) noexcept {
	return (description == nullptr) ||
	       (std::char_traits<char>::length(description) <= kMaximumDescriptionLength);
}

} // namespace

float shader_parameter_clamp(float value, float minimum, float maximum) noexcept {
	if (!std::isfinite(value)) {
		return minimum;
	}
	if (value < minimum) {
		return minimum;
	}
	if (value > maximum) {
		return maximum;
	}
	return value;
}

void ShaderParameterSet::clear() noexcept {
	parameters_.clear();
}

bool ShaderParameterSet::add(const char *name, const char *description, float initial,
                             float minimum, float maximum, float step) {
	if (!has_valid_name(name) || !has_valid_description(description) ||
	    !valid_range(initial, minimum, maximum, step) ||
	    (parameters_.size() >= kMaximumParameters) || (find(name) != nullptr)) {
		return false;
	}
	ShaderParameterInfo parameter;
	parameter.name = name;
	parameter.description = (description != nullptr) ? description : "";
	parameter.initial = shader_parameter_clamp(initial, minimum, maximum);
	parameter.minimum = minimum;
	parameter.maximum = maximum;
	parameter.step = step;
	parameter.value = parameter.initial;
	parameters_.push_back(std::move(parameter));
	return true;
}

const ShaderParameterInfo *ShaderParameterSet::at(std::size_t index) const noexcept {
	return (index < parameters_.size()) ? &parameters_[index] : nullptr;
}

ShaderParameterInfo *ShaderParameterSet::at(std::size_t index) noexcept {
	return (index < parameters_.size()) ? &parameters_[index] : nullptr;
}

const ShaderParameterInfo *ShaderParameterSet::find(const char *name) const noexcept {
	if (name == nullptr) {
		return nullptr;
	}
	for (const ShaderParameterInfo &parameter : parameters_) {
		if (parameter.name == name) {
			return &parameter;
		}
	}
	return nullptr;
}

bool ShaderParameterSet::set_value(const char *name, float requested, float *applied) noexcept {
	if (name == nullptr) {
		return false;
	}
	for (std::size_t i = 0; i < parameters_.size(); ++i) {
		if (parameters_[i].name == name) {
			return set_value_at(i, requested, applied);
		}
	}
	return false;
}

bool ShaderParameterSet::set_value_at(std::size_t index, float requested, float *applied) noexcept {
	ShaderParameterInfo *parameter = at(index);
	if (parameter == nullptr) {
		return false;
	}
	parameter->value = shader_parameter_clamp(requested, parameter->minimum, parameter->maximum);
	if (applied != nullptr) {
		*applied = parameter->value;
	}
	return true;
}

void ShaderParameterSet::restore_value(std::size_t index, float value) noexcept {
	ShaderParameterInfo *parameter = at(index);
	if (parameter != nullptr) {
		parameter->value = shader_parameter_clamp(value, parameter->minimum, parameter->maximum);
	}
}

void ShaderParameterSet::reset() noexcept {
	for (ShaderParameterInfo &parameter : parameters_) {
		parameter.value = parameter.initial;
	}
}

bool ShaderParameterSet::load_values(const char *path) {
	if ((path == nullptr) || (path[0] == '\0')) {
		return true;
	}
	std::ifstream input(path);
	if (!input.is_open()) {
		return true;
	}
	ParameterValues pending;
	if (!read_parameter_values(input, pending))
		return false;
	for (const auto &entry : pending) {
		(void)set_value(entry.first.c_str(), entry.second, nullptr);
	}
	return true;
}

bool ShaderParameterSet::load_config() {
	if (config_buffer == nullptr || config_buffer[0] == '\0')
		return true;
	std::string text(config_buffer);
	std::replace(text.begin(), text.end(), ';', '\n');
	std::istringstream input(text);
	ParameterValues pending;
	if (!read_parameter_values(input, pending))
		return false;
	for (const auto &entry : pending)
		(void)set_value(entry.first.c_str(), entry.second, nullptr);
	return true;
}

bool ShaderParameterSet::save_config() const {
	ParameterValues values;
	for (const auto &parameter : parameters_)
		values.emplace_back(parameter.name, parameter.value);
	return store_config_values(values);
}

bool ShaderParameterSet::save_values(const char *path) const {
	if ((path == nullptr) || (path[0] == '\0')) {
		return true;
	}
	const std::string destination(path);
	const std::string temporary = destination + ".tmp";
	{
		std::ofstream output(temporary, std::ios::trunc);
		if (!output.is_open()) {
			return false;
		}
		output << kStateHeader << '\n';
		output << std::setprecision(9);
		for (const ShaderParameterInfo &parameter : parameters_) {
			output << parameter.name << '=' << parameter.value << '\n';
		}
		if (!output.good()) {
			output.close();
			std::remove(temporary.c_str());
			return false;
		}
	}
	if (std::rename(temporary.c_str(), destination.c_str()) != 0) {
		std::remove(temporary.c_str());
		return false;
	}
	return true;
}

} // namespace vaeg::librashader

extern "C" int vaeg_shader_config_bind(char *buffer, size_t capacity) {
	using namespace vaeg::librashader;
	config_buffer = buffer;
	config_capacity = capacity;
	if (buffer == nullptr)
		return 1;
	if (capacity == 0 || std::memchr(buffer, '\0', capacity) == nullptr) {
		config_buffer = nullptr;
		config_capacity = 0;
		return 0;
	}
	return 1;
}
