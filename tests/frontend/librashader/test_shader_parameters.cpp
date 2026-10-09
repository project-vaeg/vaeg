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
#undef NDEBUG
#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>

#include "librashader/shader_parameters.h"
#include "librashader/shader_config.h"

using namespace vaeg::librashader;

int main() {
	const char *state_path = "vaeg-shader-parameters-test.state";
	ShaderParameterSet parameters;
	ShaderParameterSet restored;
	float applied = 0.0f;

	assert(parameters.add("MASK", "Mask strength", 0.5f, 0.0f, 1.0f, 0.1f));
	assert(parameters.add("CURVATURE", "Curvature", 0.25f, 0.0f, 1.0f, 0.05f));
	assert(!parameters.add("MASK", "duplicate", 0.0f, 0.0f, 1.0f, 0.1f));
	assert(parameters.set_value("MASK", 2.0f, &applied) && applied == 1.0f);
	assert(parameters.set_value("CURVATURE", -1.0f, &applied) && applied == 0.0f);
	assert(parameters.save_values(state_path));

	assert(restored.add("MASK", "Mask strength", 0.5f, 0.0f, 1.0f, 0.1f));
	assert(restored.add("CURVATURE", "Curvature", 0.25f, 0.0f, 1.0f, 0.05f));
	assert(restored.load_values(state_path));
	assert(restored.at(0)->value == 1.0f && restored.at(1)->value == 0.0f);
	assert(restored.set_value("MASK", 0.5f, nullptr));
	restored.reset();
	assert(restored.at(0)->value == 0.5f && restored.at(1)->value == 0.25f);
	{
		std::ofstream clamped(state_path, std::ios::trunc);
		clamped << "VAEG_SHADER_PARAMETERS 1\nMASK=9\nCURVATURE=-3\n";
	}
	assert(restored.load_values(state_path));
	assert(restored.at(0)->value == 1.0f && restored.at(1)->value == 0.0f);
	restored.reset();

	{
		std::ofstream malformed(state_path, std::ios::trunc);
		malformed << "VAEG_SHADER_PARAMETERS 1\nMASK=not-a-number\n";
	}
	assert(!restored.load_values(state_path));
	assert(restored.at(0)->value == 0.5f && restored.at(1)->value == 0.25f);
	std::remove(state_path);
	char config[8192] = "VAEG_SHADER_PARAMETERS 1;MASK=0.75;CURVATURE=0.03;";
	assert(vaeg_shader_config_bind(config, sizeof(config)));
	assert(restored.load_config());
	assert(restored.at(0)->value == 0.75f && restored.at(1)->value == 0.03f);
	assert(restored.set_value("MASK", 0.25f, nullptr));
	assert(restored.save_config());
	const std::string saved(config);
	assert(vaeg_shader_config_bind(config, sizeof(config)));
	assert(std::string(config) == saved);
	restored.reset();
	assert(restored.load_config() && restored.at(0)->value == 0.25f);
	char session[8192] = {};
	assert(vaeg_shader_config_bind(session, sizeof(session))); // --no-cfg
	restored.reset();
	assert(restored.load_config() && restored.at(0)->value == 0.5f);
	assert(restored.save_config()); // Memory only; no standalone file writer.
	char small[8] = {};
	assert(vaeg_shader_config_bind(small, sizeof(small)));
	assert(!restored.save_config() && small[0] == '\0'); // No partial write.
	char malformed[128] = "VAEG_SHADER_PARAMETERS 1;MASK=0.8;CURVATURE=oops;";
	assert(vaeg_shader_config_bind(malformed, sizeof(malformed)));
	assert(!restored.load_config());
	assert(restored.at(0)->value == 0.5f); // Transactional parse.
	assert(vaeg_shader_config_bind(nullptr, 0));
	std::remove(state_path);
	std::remove((std::string(state_path) + ".tmp").c_str());
	return 0;
}
