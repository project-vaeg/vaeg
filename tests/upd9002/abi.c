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
#include "upd9002_state.h"
#include "upd9002_regs.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define ALIGNOF(type)                                                                              \
	offsetof(                                                                                      \
	    struct {                                                                                   \
		    char byte;                                                                             \
		    type value;                                                                            \
	    },                                                                                         \
	    value)

static void append(char *buffer, size_t size, const char *name, unsigned int value) {
	size_t used;

	used = strlen(buffer);
	snprintf(buffer + used, size - used, "%s=%u\n", name, value);
}

static int verify_upd9002_state_abi(void) {
	if ((sizeof(Upd9002StateImage) != 112) || (ALIGNOF(Upd9002StateImage) != 4)) {
		return 0;
	}
#define OFFSET(field, value)                                                                       \
	if (offsetof(Upd9002StateImage, field) != (value))                                             \
	return 0
	OFFSET(r, 0);
	OFFSET(es_base, 28);
	OFFSET(cs_base, 32);
	OFFSET(ss_base, 36);
	OFFSET(ds_base, 40);
	OFFSET(ss_fix, 44);
	OFFSET(ds_fix, 48);
	OFFSET(adrsmask, 52);
	OFFSET(prefix, 56);
	OFFSET(trap, 58);
	OFFSET(resetreq, 59);
	OFFSET(ovflag, 60);
	OFFSET(GDTR, 64);
	OFFSET(MSW, 70);
	OFFSET(IDTR, 72);
	OFFSET(LDTR, 78);
	OFFSET(LDTRC, 80);
	OFFSET(TR, 86);
	OFFSET(TRC, 88);
	OFFSET(padding, 94);
	OFFSET(cpu_type, 96);
	OFFSET(itfbank, 97);
	OFFSET(ram_d0, 98);
	OFFSET(remainclock, 100);
	OFFSET(baseclock, 104);
	OFFSET(clock, 108);
#undef OFFSET
	return 1;
}

int main(int argc, char **argv) {
	char actual[4096];
	char expected[4096];
	FILE *stream;
	size_t bytes;

	if (!verify_upd9002_state_abi()) {
		return 5;
	}

	actual[0] = '\0';
	append(actual, sizeof(actual), "upd9002_state.size", sizeof(Upd9002RuntimeState));
	append(actual, sizeof(actual), "upd9002_state.align", ALIGNOF(Upd9002RuntimeState));
#define FIELD(name)                                                                                \
	append(actual, sizeof(actual), "upd9002_state.offset." #name,                                  \
	       offsetof(Upd9002RuntimeState, name))
	FIELD(r);
	FIELD(es_base);
	FIELD(cs_base);
	FIELD(ss_base);
	FIELD(ds_base);
	FIELD(ss_fix);
	FIELD(ds_fix);
	FIELD(adrsmask);
	FIELD(prefix);
	FIELD(trap);
	FIELD(resetreq);
	FIELD(ovflag);
	FIELD(GDTR);
	FIELD(MSW);
	FIELD(IDTR);
	FIELD(LDTR);
	FIELD(LDTRC);
	FIELD(TR);
	FIELD(TRC);
	FIELD(padding);
	FIELD(cpu_type);
	FIELD(itfbank);
	FIELD(ram_d0);
	FIELD(remainclock);
	FIELD(baseclock);
	FIELD(clock);
#undef FIELD
	append(actual, sizeof(actual), "upd9002.size", sizeof(UPD9002_REGS));
	append(actual, sizeof(actual), "upd9002.align", ALIGNOF(UPD9002_REGS));
	append(actual, sizeof(actual), "upd9002.offset.tcks", offsetof(UPD9002_REGS, tcks));
	append(actual, sizeof(actual), "upd9002.offset.dmy", offsetof(UPD9002_REGS, dmy));
	fputs(actual, stdout);
	if (argc == 1) {
		return 0;
	}
	if (argc != 2) {
		return 2;
	}
	stream = fopen(argv[1], "rb");
	if (stream == NULL) {
		return 3;
	}
	bytes = fread(expected, 1, sizeof(expected) - 1, stream);
	expected[bytes] = '\0';
	if ((ferror(stream) != 0) || (fgetc(stream) != EOF)) {
		fclose(stream);
		return 4;
	}
	fclose(stream);
	return strcmp(actual, expected) ? 1 : 0;
}
