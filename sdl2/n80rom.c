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
/*
 * Identification of the optional N-BASIC ROM (vaeg N mode). The VA carries
 * no N-BASIC; the maintainer's known dumps are listed by SHA-1, and
 * n80.rom is accepted whatever it holds. See
 * docs/modernization/v1v2-n-basic-mode.md.
 */

#include "compiler.h"
#include "dosio.h"
#include "machine/pccore.h"
#include "bios/romva.h"
#include "n80rom.h"
#include "romcheck.h"

#include <string.h>

static const struct {
	const char *sha1;
	const char *label;
} known_n80[] = {
    {"063609dd518c124a4fc9ba35d1bae35771666a34", "N-BASIC 1.2"},
    {"06dae1db384aa29d81c5b6ed587877e7128fcb35", "N-BASIC 1.8"},
};

const char *n80rom_identify_sha1(const char *sha1) {
	UINT i;

	for (i = 0; i < NELEMENTS(known_n80); i++) {
		if (strcmp(sha1, known_n80[i].sha1) == 0) {
			return known_n80[i].label;
		}
	}
	return "unknown dump";
}

/* Label of a ROM file in the ROM directory, or NULL if it is not usable. */
const char *n80rom_label(const char *name) {
	char path[MAX_PATH];
	char sha1[41];
	ROMCHECKSUM sum;

	if ((name == NULL) || (name[0] == '\0')) {
		return NULL;
	}
	getbiospath(path, name, sizeof(path));
	if ((romcheck_file(path, &sum) != SUCCESS) || (sum.size != 0x8000)) {
		return NULL;
	}
	romcheck_sha1_string(sum.sha1, sha1);
	return n80rom_identify_sha1(sha1);
}

UINT n80rom_scan(N80ROMENTRY *entries, UINT count) {
	UINT found = 0;
	UINT i;

	for (i = 0; (i < ROMVA_N80_NAMES) && (i < count); i++) {
		entries[i].name = romva_n80_names[i];
		entries[i].label = n80rom_label(romva_n80_names[i]);
		entries[i].present = (entries[i].label != NULL) ? TRUE : FALSE;
		if (entries[i].present) {
			found++;
		}
	}
	return found;
}
