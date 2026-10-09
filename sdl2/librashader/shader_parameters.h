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
#ifndef VAEG_SDL2_LIBRASHADER_SHADER_PARAMETERS_H
#define VAEG_SDL2_LIBRASHADER_SHADER_PARAMETERS_H

#include <cstddef>
#include <string>
#include <vector>

namespace vaeg::librashader {

struct ShaderParameterInfo {
	std::string name;
	std::string description;
	float initial;
	float minimum;
	float maximum;
	float step;
	float value;
};

class ShaderParameterSet {
  public:
	static constexpr std::size_t kMaximumParameters = 256;

	void clear() noexcept;
	bool add(const char *name, const char *description, float initial, float minimum,
	         float maximum, float step);

	std::size_t size() const noexcept { return parameters_.size(); }
	const ShaderParameterInfo *at(std::size_t index) const noexcept;
	ShaderParameterInfo *at(std::size_t index) noexcept;
	const ShaderParameterInfo *find(const char *name) const noexcept;

	bool set_value(const char *name, float requested, float *applied) noexcept;
	bool set_value_at(std::size_t index, float requested, float *applied) noexcept;
	void restore_value(std::size_t index, float value) noexcept;
	void reset() noexcept;

	/* Missing state files are valid; malformed files leave this set unchanged. */
	bool load_values(const char *path);
	bool save_values(const char *path) const;
	bool load_config();
	bool save_config() const;

  private:
	std::vector<ShaderParameterInfo> parameters_;
};

float shader_parameter_clamp(float value, float minimum, float maximum) noexcept;

} // namespace vaeg::librashader

#endif
