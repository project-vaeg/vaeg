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
#include "cpucore.h"
#include "machine/statsave.h"
#if !defined(VAEG_M44_RAW_UPD9002_STATE)
#include "upd9002_state.h"
#endif
#include "tests/upd9002/state_scenario.h"

#include <stdio.h>
#include <stdlib.h>

#define M44_GENERATE_DIR "VAEG_M44_SCENARIO_DIR"
#define M44_INPUT_DIR "VAEG_M44_SCENARIO_INPUT_DIR"
#define M44_OUTPUT_DIR "VAEG_M44_SCENARIO_OUTPUT_DIR"

static const char *const m44_scenario_names[] = {"m44-reset.state", "m44-executed-3.state",
                                                 "m44-cpu-shut-request.state"};

#if !defined(VAEG_M44_RAW_UPD9002_STATE)
void upd9002_m42_process_cpu_reset_request(void);
#endif

static void prepare_executed(void) {
	static const UINT8 program[] = {0xb8, 0x34, 0x12, 0x40, 0x90};
	UINT32 index;

	/* Keep this identical to the committed M42 fixture definition. */
	ZeroMemory(&CPU_STATSAVE, sizeof(CPU_STATSAVE));
#if !defined(VAEG_M44_RAW_UPD9002_STATE)
	upd9002_state_reset();
#endif
	upd9002_core_context.s.cpu_type = CPUTYPE_V30;
	CPU_FLAG = 0xf002;
	CPU_ADRSMASK = 0xfffff;
	CPU_SP = 0x8000;
	CPU_IP = 0x2000;
	CPU_REMCLOCK = 100;
	CPU_BASECLOCK = 100;
	CPU_CLOCK = 0x12345678;
	CopyMemory(mem + 0x2000, program, sizeof(program));
	for (index = 0; index < 3; index++) {
		upd9002_core_step();
	}
}

static void prepare_cpu_shut(void) {
	CPU_RESETREQ = 1;
#if defined(VAEG_M44_RAW_UPD9002_STATE)
	/* Exact production reset-request operation at the approved G41 SHA. */
	if (CPU_RESETREQ) {
		CPU_RESETREQ = 0;
		CPU_SHUT();
	}
#else
	upd9002_m42_process_cpu_reset_request();
#endif
}

static void make_path(char *path, const char *directory, const char *name) {
	SPRINTF(path, "%s/%s", directory, name);
}

static int save_scenario(const char *directory, const char *name) {
	char path[MAX_PATH];

	make_path(path, directory, name);
	if (statsave_save(path) != STATFLAG_SUCCESS) {
		fprintf(stderr, "upd9002-state-scenario: save failed: %s\n", name);
		return FAILURE;
	}
	return SUCCESS;
}

static int generate_scenarios(const char *directory) {
	if (save_scenario(directory, m44_scenario_names[0]) != SUCCESS) {
		return FAILURE;
	}
	prepare_executed();
	if (save_scenario(directory, m44_scenario_names[1]) != SUCCESS) {
		return FAILURE;
	}
	prepare_cpu_shut();
	if (save_scenario(directory, m44_scenario_names[2]) != SUCCESS) {
		return FAILURE;
	}
	fprintf(stderr, "upd9002-state-scenario: reset, executed-3, and CPU_SHUT saved\n");
	return SUCCESS;
}

static int roundtrip_scenarios(const char *input, const char *output) {
	char input_path[MAX_PATH];
	char output_path[MAX_PATH];
	UINT index;

	for (index = 0; index < NELEMENTS(m44_scenario_names); index++) {
		make_path(input_path, input, m44_scenario_names[index]);
		make_path(output_path, output, m44_scenario_names[index]);
		if ((statsave_load(input_path) == STATFLAG_FAILURE) ||
		    (statsave_save(output_path) != STATFLAG_SUCCESS)) {
			fprintf(stderr, "upd9002-state-scenario: round trip failed: %s\n",
			        m44_scenario_names[index]);
			return FAILURE;
		}
	}
	fprintf(stderr, "upd9002-state-scenario: three states loaded and re-saved\n");
	return SUCCESS;
}

int upd9002_state_scenario_requested(void) {
	return (getenv(M44_GENERATE_DIR) != NULL) || (getenv(M44_INPUT_DIR) != NULL) ||
	       (getenv(M44_OUTPUT_DIR) != NULL);
}

int upd9002_state_scenario_run(void) {
	const char *generate;
	const char *input;
	const char *output;

	generate = getenv(M44_GENERATE_DIR);
	input = getenv(M44_INPUT_DIR);
	output = getenv(M44_OUTPUT_DIR);
	if ((generate != NULL) && ((input != NULL) || (output != NULL))) {
		fprintf(stderr, "upd9002-state-scenario: conflicting modes\n");
		return FAILURE;
	}
	if (generate != NULL) {
		return generate_scenarios(generate);
	}
	if ((input == NULL) || (output == NULL)) {
		fprintf(stderr, "upd9002-state-scenario: incomplete round-trip mode\n");
		return FAILURE;
	}
	return roundtrip_scenarios(input, output);
}
