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
#include "machine/pccore.h"
#include "iocore.h"
#include "iocoreva.h"
#include "tests/idp/m69_status_composition.h"

#include <stdio.h>

enum {
	M69_PORT_STATUS = 0x142,
	M69_PORT_STATUS_HIGH = 0x143,
	M69_PORT_PARAMETER = 0x146,
	M69_CMD_SYNC = 0x10,
	M69_STATUS_BUSY = 0x04,
	M69_STATUS_VB = 0x40,
	M69_VSYNC_ACTIVE = 0x20,
	M69_SYNC_PARAM_COUNT = 14
};

static void m69_prepare_io(void) {
	pccore.model_va = PCMODEL_VA1;
	pccore.multiple = 1;
	CPU_REMCLOCK = 1000000;
	iocore_create();
	if (iocore_build() != SUCCESS) {
		fprintf(stderr, "idp-m69-status: iocore_build failed\n");
		return;
	}
	iocore_bind();
	tsp_reset();
	tsp_bind();
}

static REG8 m69_expected(REG8 stored, int vb) {
	if (vb) {
		return ((REG8)(stored | M69_STATUS_VB));
	}
	return (stored);
}

static REG8 m69_read_status(REG8 stored, int vb) {
	tsp.status = stored;
	tsp.vsync = (UINT8)(vb ? M69_VSYNC_ACTIVE : 0);
	CPU_REMCLOCK = 1000000;
	return (iocore_inp8(M69_PORT_STATUS));
}

static int m69_check_status_case(const char *group, REG8 stored, int vb) {
	REG8 expected;
	REG8 actual;

	expected = m69_expected(stored, vb);
	actual = m69_read_status(stored, vb);
	if (actual != expected) {
		fprintf(stderr, "idp-m69-status: %s stored=%02x vb=%u expected=%02x actual=%02x\n", group,
		        stored, vb ? 1 : 0, expected, actual);
		return (1);
	}
	return (0);
}

static int m69_check_word_access(REG8 stored, int vb) {
	REG16 expected;
	REG16 actual;

	tsp.status = stored;
	tsp.vsync = (UINT8)(vb ? M69_VSYNC_ACTIVE : 0);
	CPU_REMCLOCK = 1000000;
	expected = (REG16)((0xffU << 8) | m69_expected(stored, vb));
	actual = iocore_inp16(M69_PORT_STATUS);
	if (actual != expected) {
		fprintf(stderr, "idp-m69-status: word-in stored=%02x vb=%u expected=%04x actual=%04x\n",
		        stored, vb ? 1 : 0, expected, actual);
		return (1);
	}
	return (0);
}

static int m69_check_exhaustive(void) {
	unsigned int stored;
	unsigned int vb;
	unsigned int failures;

	failures = 0;
	for (stored = 0; stored <= 0xff; stored++) {
		for (vb = 0; vb <= 1; vb++) {
			REG8 expected;
			REG8 actual;

			expected = m69_expected((REG8)stored, (int)vb);
			actual = m69_read_status((REG8)stored, (int)vb);
			if (actual != expected) {
				if (failures < 16) {
					fprintf(stderr,
					        "idp-m69-status: exhaustive stored=%02x vb=%u "
					        "expected=%02x actual=%02x\n",
					        stored, vb, expected, actual);
				}
				failures++;
			}
		}
	}
	if (failures) {
		fprintf(stderr, "idp-m69-status: exhaustive failures=%u over 512 rows\n", failures);
		return (1);
	}
	return (0);
}

static int m69_check_busy_lifecycle(void) {
	unsigned int i;
	REG8 actual;
	int failures;

	failures = 0;
	m69_prepare_io();
	tsp.vsync = 0;
	iocore_out8(M69_PORT_STATUS, M69_CMD_SYNC);
	actual = iocore_inp8(M69_PORT_STATUS);
	if (actual != M69_STATUS_BUSY) {
		fprintf(stderr, "idp-m69-status: busy-vb0 expected=%02x actual=%02x\n", M69_STATUS_BUSY,
		        actual);
		failures++;
	}
	for (i = 0; i < M69_SYNC_PARAM_COUNT; i++) {
		iocore_out8(M69_PORT_PARAMETER, 0);
	}
	actual = iocore_inp8(M69_PORT_STATUS);
	if (actual != 0) {
		fprintf(stderr, "idp-m69-status: busy-clear-vb0 expected=00 actual=%02x\n", actual);
		failures++;
	}

	m69_prepare_io();
	tsp.vsync = M69_VSYNC_ACTIVE;
	iocore_out8(M69_PORT_STATUS, M69_CMD_SYNC);
	actual = iocore_inp8(M69_PORT_STATUS);
	if (actual != (M69_STATUS_BUSY | M69_STATUS_VB)) {
		fprintf(stderr, "idp-m69-status: busy-vb1 expected=%02x actual=%02x\n",
		        M69_STATUS_BUSY | M69_STATUS_VB, actual);
		failures++;
	}
	for (i = 0; i < M69_SYNC_PARAM_COUNT; i++) {
		iocore_out8(M69_PORT_PARAMETER, 0);
	}
	actual = iocore_inp8(M69_PORT_STATUS);
	if (actual != M69_STATUS_VB) {
		fprintf(stderr, "idp-m69-status: busy-clear-vb1 expected=%02x actual=%02x\n", M69_STATUS_VB,
		        actual);
		failures++;
	}
	return (failures != 0);
}

int idp_m69_status_composition_main(void) {
	static const REG8 required[] = {0x00, 0x04, 0x01, 0x02, 0x08, 0x10,
	                                0x20, 0x80, 0x3f, 0xbf, 0x40, 0x44};
	unsigned int i;
	unsigned int failures;

	failures = 0;
	m69_prepare_io();
	for (i = 0; i < NELEMENTS(required); i++) {
		failures += (unsigned int)m69_check_status_case("truth", required[i], 0);
		failures += (unsigned int)m69_check_status_case("truth", required[i], 1);
	}
	failures += (unsigned int)m69_check_exhaustive();
	failures += (unsigned int)m69_check_word_access(0x04, 0);
	failures += (unsigned int)m69_check_word_access(0x04, 1);
	failures += (unsigned int)m69_check_busy_lifecycle();
	iocore_destroy();

	if (failures) {
		fprintf(stderr, "idp-m69-status: %u status-composition groups failed\n", failures);
		return (FAILURE);
	}
	puts("idp-m69-status: status composition checks passed");
	return (SUCCESS);
}
