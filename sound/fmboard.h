

#include "soundrom.h"
#include "fmtimer.h"
#include "opngen.h"
#include "psggen.h"
#include "rhythm.h"
#include "adpcm.h"

typedef struct {
	BYTE reg[0x400];
	BYTE opnreg;
	BYTE extreg;
	BYTE opn2reg;
	BYTE ext2reg;
	BYTE adpcmmask;
	BYTE channels;
	BYTE extend;
	BYTE padding;
	UINT16 base;
} OPN_T;

typedef struct {
	BYTE sintm; // bit7  0..割り込み許可  1..割り込み禁止
} _FMBOARDVA;

#ifdef __cplusplus
extern "C" {
#endif

extern UINT32 usesound;
extern OPN_T opn;
extern _FMTIMER fmtimer;
extern _OPNGEN opngen;
extern OPNCH opnch[OPNCH_MAX];
extern _PSGGEN psg1;
extern _PSGGEN psg2;
extern _PSGGEN psg3;
extern _RHYTHM rhythm;
extern _ADPCM adpcm;

extern _FMBOARDVA fmboardva;

REG8 fmboard_getjoy(PSGGEN psg);

void fmboard_extreg(void (*ext)(REG8 enable));
void fmboard_extenable(REG8 enable);

void fmboard_reset(UINT32 type);
void fmboard_bind(void);

void fmboard_fmrestore(REG8 chbase, UINT bank);
void fmboard_rhyrestore(RHYTHM rhy, UINT bank);

void fmboard_setintmask(BYTE mask);
BYTE fmboard_getintmask(void);

#ifdef __cplusplus
}
#endif
