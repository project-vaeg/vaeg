/*
 * subsystemif.h: PC-88VA FD Sub System Interface
 */

#include "i8255.h"

typedef struct {
	_I8255 i8255;
} _SUBSYSTEMIF, *SUBSYSTEMIF;

#ifdef __cplusplus
extern "C" {
#endif

extern _SUBSYSTEMIF subsystemif;

void subsystemif_businporta(BYTE dat);
void subsystemif_businportb(BYTE dat);
void subsystemif_businportc(BYTE dat);

void subsystemif_initialize(void);
void subsystemif_reset(void);
void subsystemif_bind(void);

#ifdef __cplusplus
}
#endif
