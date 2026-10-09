#include "compiler.h"
#include "strres.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "sound.h"
#include "fmboard.h"
#include "np2info.h"
#include "bkupmemva.h"
#include "np2ver.h"
#include "sgp.h"

#include "memoryva.h"
#include "subsystem.h"
#include "va91.h"

static const char str_comma[] = ", ";
static const char str_2halfMHz[] = "2.5MHz";
#define str_5MHz (str_2halfMHz + 2)
static const char str_8MHz[] = "8MHz";
static const char str_notexist[] = "not exist";
static const char str_disable[] = "disable";

static const char str_blank[] = " ";
static const char str_exist[] = "exist";
static const char str_ok[] = "OK";
static const char str_ng[] = "NG";

static const char str_model_va1[] = "PC-88VA";
static const char str_model_va2[] = "PC-88VA2";

static const char str_romtype_88va[] = "PC-88VA\0"
                                       "PC-88VA2/3\0"
                                       "Unknown";

static const char str_88va_rom00[] = "00";
static const char str_88va_rom08[] = "08";
static const char str_88va_rom1[] = "1";
static const char str_88va_dic[] = "DIC";
static const char str_88va_font[] = "FONT";

static const char str_cpu[] = "uPD9002";
static const char str_sound_opn[] = "OPN";
static const char str_sound_opna[] = "OPNA";

static const char str_clockfmt[] = "%d.%1dMHz";
static const char str_precise_clockfmt[] = "%u.%04uMHz";
static const char str_memfmt[] = "%3uKB";
static const char str_memfmt2[] = "%3uKB + %uKB";
static const char str_memfmt3[] = "%d.%1dMB";
static const char str_rhythm[] = "BSCHTR";

// ---- common

static void info_ver(char *str, int maxlen, NP2INFOEX *ex) {
	milstr_ncpy(str, np2version, maxlen);
	(void)ex;
}

static void info_commit(char *str, int maxlen, NP2INFOEX *ex) {
	milstr_ncpy(str, VAEG_BUILD_COMMIT, maxlen);
	(void)ex;
}

static void info_model(char *str, int maxlen, NP2INFOEX *ex) {
	milstr_ncpy(str, (pccore.model_va == PCMODEL_VA2) ? str_model_va2 : str_model_va1, maxlen);
	(void)ex;
}

static void info_precise_clock(char *str, int maxlen, UINT32 clock) {
	char clockstr[16];

	SPRINTF(clockstr, str_precise_clockfmt, clock / 1000000, (clock % 1000000) / 100);
	milstr_ncpy(str, clockstr, maxlen);
}

static void info_cpuclock(char *str, int maxlen, NP2INFOEX *ex) {
	info_precise_clock(str, maxlen, pccore_cpu_clock());
	(void)ex;
}

static void info_sgpclock(char *str, int maxlen, NP2INFOEX *ex) {
	info_precise_clock(str, maxlen, sgp_effective_clock());
	(void)ex;
}

static void info_frame(char *str, int maxlen, NP2INFOEX *ex) {
	char framestr[16];

	SPRINTF(framestr, "%u", drawcount);
	milstr_ncpy(str, framestr, maxlen);
	(void)ex;
}

static void info_cpu(char *str, int maxlen, NP2INFOEX *ex) {
	milstr_ncpy(str, str_cpu, maxlen);
	(void)ex;
}

static void info_clock(char *str, int maxlen, NP2INFOEX *ex) {
	UINT32 clock;
	char clockstr[16];

	clock = (pccore.realclock + 50000) / 100000;
	SPRINTF(clockstr, str_clockfmt, clock / 10, clock % 10);
	milstr_ncpy(str, clockstr, maxlen);
	(void)ex;
}

static void info_base(char *str, int maxlen, NP2INFOEX *ex) {
	milstr_ncpy(str, (pccore.cpumode & CPUMODE_8MHZ) ? str_8MHz : str_5MHz, maxlen);
	(void)ex;
}

static void info_mem1(char *str, int maxlen, NP2INFOEX *ex) {
	UINT memsize;
	char memstr[32];

	memsize = np2cfg.memsw[2] & 7;
	if (memsize < 6) {
		memsize = (memsize + 1) * 128;
	} else {
		memsize = 640;
	}
	if (pccore.extmem) {
		SPRINTF(memstr, str_memfmt2, memsize, pccore.extmem * 1024);
	} else {
		SPRINTF(memstr, str_memfmt, memsize);
	}
	milstr_ncpy(str, memstr, maxlen);
	(void)ex;
}

static void info_mem2(char *str, int maxlen, NP2INFOEX *ex) {
	UINT memsize;
	char memstr[16];

	memsize = np2cfg.memsw[2] & 7;
	if (memsize < 6) {
		memsize = (memsize + 1) * 128;
	} else {
		memsize = 640;
	}
	memsize += pccore.extmem * 1024;
	SPRINTF(memstr, str_memfmt, memsize);
	milstr_ncpy(str, memstr, maxlen);
	(void)ex;
}

static void info_mem3(char *str, int maxlen, NP2INFOEX *ex) {
	UINT memsize;
	char memstr[16];

	memsize = np2cfg.memsw[2] & 7;
	if (memsize < 6) {
		memsize = (memsize + 1) * 128;
	} else {
		memsize = 640;
	}
	if (pccore.extmem > 1) {
		SPRINTF(memstr, str_memfmt3, pccore.extmem, memsize / 100);
	} else {
		SPRINTF(memstr, str_memfmt, memsize);
	}
	milstr_ncpy(str, memstr, maxlen);
	(void)ex;
}

static void info_sound(char *str, int maxlen, NP2INFOEX *ex) {
	switch (usesound) {
	case FMBOARD_VA_OPN:
		milstr_ncpy(str, str_sound_opn, maxlen);
		break;

	case FMBOARD_VA_OPNA:
		milstr_ncpy(str, str_sound_opna, maxlen);
		break;

	default:
		milstr_ncpy(str, str_disable, maxlen);
		break;
	}
	(void)ex;
}

static void info_bios(char *str, int maxlen, NP2INFOEX *ex) {
	str[0] = '\0';
	if (pccore.rom & PCROM_BIOS) {
		milstr_ncat(str, str_biosrom, maxlen);
	}
	if (soundrom.name[0]) {
		if (str[0]) {
			milstr_ncat(str, str_comma, maxlen);
		}
		milstr_ncat(str, soundrom.name, maxlen);
	}
	if (str[0] == '\0') {
		milstr_ncat(str, str_notexist, maxlen);
	}
	(void)ex;
}

static void info_romtype_88va(char *str, int maxlen, NP2INFOEX *ex) {
	int romtype;

	romtype = 0xffff - (rom1mem[0xffff] * 256 + rom1mem[0xfffe]);
	if (romtype > 2)
		romtype = 2;
	milstr_ncpy(str, milstr_list(str_romtype_88va, romtype), maxlen);
}

static void info_bios_88va(char *str, int maxlen, NP2INFOEX *ex) {
	milstr_ncpy(str, str_88va_rom00, maxlen);
	milstr_ncat(str, str_blank, maxlen);
	milstr_ncat(str, (((memoryva.rom0exist & 0x00ff) == 0x00ff) ? str_ok : str_ng), maxlen);
	milstr_ncat(str, str_comma, maxlen);

	milstr_ncat(str, str_88va_rom08, maxlen);
	milstr_ncat(str, str_blank, maxlen);
	milstr_ncat(str, (((memoryva.rom0exist & 0x0300) == 0x0300) ? str_ok : str_ng), maxlen);
	milstr_ncat(str, str_comma, maxlen);

	milstr_ncat(str, str_88va_rom1, maxlen);
	milstr_ncat(str, str_blank, maxlen);
	milstr_ncat(str, (((memoryva.rom1exist & 0x0003) == 0x0003) ? str_ok : str_ng), maxlen);
	milstr_ncat(str, str_comma, maxlen);

	milstr_ncat(str, str_88va_font, maxlen);
	milstr_ncat(str, str_blank, maxlen);
	milstr_ncat(str, (((memoryva.sysmromexist & 0x0300) == 0x0300) ? str_ok : str_ng), maxlen);
	milstr_ncat(str, str_comma, maxlen);

	milstr_ncat(str, str_88va_dic, maxlen);
	milstr_ncat(str, str_blank, maxlen);
	milstr_ncat(str, (((memoryva.sysmromexist & 0x3000) == 0x3000) ? str_ok : str_ng), maxlen);
}

static void info_bios_88va91(char *str, int maxlen, NP2INFOEX *ex) {
	if (va91.cfg.enabled) {
		milstr_ncpy(str, str_88va_rom00, maxlen);
		milstr_ncat(str, str_blank, maxlen);
		milstr_ncat(str, (((va91cfg.rom0exist & 0x00ff) == 0x00ff) ? str_ok : str_ng), maxlen);
		milstr_ncat(str, str_comma, maxlen);

		milstr_ncat(str, str_88va_rom08, maxlen);
		milstr_ncat(str, str_blank, maxlen);
		milstr_ncat(str, (((va91cfg.rom0exist & 0x0300) == 0x0300) ? str_ok : str_ng), maxlen);
		milstr_ncat(str, str_comma, maxlen);

		milstr_ncat(str, str_88va_rom1, maxlen);
		milstr_ncat(str, str_blank, maxlen);
		milstr_ncat(str, (((va91cfg.rom1exist & 0x0003) == 0x0003) ? str_ok : str_ng), maxlen);
		milstr_ncat(str, str_comma, maxlen);

		milstr_ncat(str, str_88va_dic, maxlen);
		milstr_ncat(str, str_blank, maxlen);
		milstr_ncat(str, (((va91cfg.sysmromexist & 0x3000) == 0x3000) ? str_ok : str_ng), maxlen);
	} else {
		milstr_ncpy(str, str_disable, maxlen);
	}
}

static void info_bios_88vasubsys(char *str, int maxlen, NP2INFOEX *ex) {
	if (subsystem.romexist) {
		milstr_ncpy(str, str_exist, maxlen);
	} else {
		milstr_ncpy(str, str_notexist, maxlen);
	}
}

/* Optional user-supplied N-BASIC ROM (n80.rom), a vaeg extension. */
static void info_bios_n80(char *str, int maxlen, NP2INFOEX *ex) {
	if (memoryva_n80_exist) {
		milstr_ncpy(str, memoryva_n80_file, maxlen);
	} else {
		milstr_ncpy(str, str_notexist, maxlen);
	}
}

/* V1/V2 (Z80) mode as stored for the next reset: V1/V2 and H/S. */
static void info_z80mode(char *str, int maxlen, NP2INFOEX *ex) {
	if (np2cfg.v1v2_nmode) {
		milstr_ncpy(str, memoryva_n80_exist ? "N" : "N (no N80 ROM)", maxlen);
		return;
	}
	milstr_ncpy(str, bkupmemva_get_88v1() ? "V1" : "V2", maxlen);
	milstr_ncat(str, np2cfg.v1v2_standard ? " S" : " H", maxlen);
}

static void info_rhythm(char *str, int maxlen, NP2INFOEX *ex) {
	char rhythmstr[8];
	UINT exist;
	UINT i;

	exist = rhythm_getcaps();
	milstr_ncpy(rhythmstr, str_rhythm, sizeof(rhythmstr));
	for (i = 0; i < 6; i++) {
		if (!(exist & (1 << i))) {
			rhythmstr[i] = '_';
		}
	}
	milstr_ncpy(str, rhythmstr, maxlen);
	(void)ex;
}

// ---- make string

typedef struct {
	char key[8];
	void (*proc)(char *str, int maxlen, NP2INFOEX *ex);
} INFOPROC;

static const INFOPROC infoproc[] = {{"MODEL", info_model},
                                    {"ROMTPVA", info_romtype_88va},
                                    {"BIOSVA", info_bios_88va},
                                    {"BIOS91", info_bios_88va91},
                                    {"BIOSSUB", info_bios_88vasubsys},
                                    {"BIOSN80", info_bios_n80},
                                    {"Z80MODE", info_z80mode},
                                    {"VER", info_ver},
                                    {"COMMIT", info_commit},
                                    {"CPUCLK", info_cpuclock},
                                    {"SGPCLK", info_sgpclock},
                                    {"FRAME", info_frame},
                                    {"CPU", info_cpu},
                                    {"CLOCK", info_clock},
                                    {"BASE", info_base},
                                    {"MEM1", info_mem1},
                                    {"MEM2", info_mem2},
                                    {"MEM3", info_mem3},
                                    {"SND", info_sound},
                                    {"BIOS", info_bios},
                                    {"RHYTHM", info_rhythm}};

static BOOL defext(char *dst, const char *key, int maxlen, NP2INFOEX *ex) {
	milstr_ncpy(dst, key, maxlen);
	(void)ex;
	return (TRUE);
}

void np2info(char *dst, const char *src, int maxlen, const NP2INFOEX *ex) {
	NP2INFOEX statex;
	char c;
	int leng;
	char infwork[12];
	const INFOPROC *inf;
	const INFOPROC *infterm;

	if ((dst == NULL) || (maxlen <= 0) || (src == NULL)) {
		return;
	}
	if (ex == NULL) {
		milstr_ncpy(statex.cr, str_oscr, sizeof(statex.cr));
		statex.ext = NULL;
	} else {
		statex = *ex;
	}
	if (statex.ext == NULL) {
		statex.ext = defext;
	}
	while (maxlen > 0) {
		c = *src++;
		if (c == '\0') {
			break;
		} else if (c == '\n') {
			milstr_ncpy(dst, statex.cr, maxlen);
		} else if (c != '%') {
			*dst++ = c;
			maxlen--;
			continue;
		} else if (*src == '%') {
			src++;
			*dst++ = c;
			maxlen--;
			continue;
		} else {
			leng = 0;
			while (1) {
				c = *src;
				if (c == '\0') {
					break;
				}
				src++;
				if (c == '%') {
					break;
				}
				if (leng < (int)(sizeof(infwork) - 1)) {
					infwork[leng++] = c;
				}
			}
			infwork[leng] = '\0';
			inf = infoproc;
			infterm = infoproc + (sizeof(infoproc) / sizeof(INFOPROC));
			while (inf < infterm) {
				if (!milstr_cmp(infwork, inf->key)) {
					inf->proc(dst, maxlen, &statex);
					break;
				}
				inf++;
			}
			if (inf >= infterm) {
				if (!(*statex.ext)(dst, infwork, maxlen, &statex)) {
					continue;
				}
			}
		}
		leng = strlen(dst);
		dst += leng;
		maxlen -= leng;
	}
	*dst = '\0';
}
