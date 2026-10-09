
#include "compiler.h"
#include "scrnmng.h"
#include "scrndraw.h"
#include "sdrawva.h"
#include "scrndrawva.h"

static void SCRNCALL sdrawva16(SDRAWVA sdraw, int maxy) {
	const WORD *p;
	BYTE *q;
	int y;
	int x;

	p = vabitmap;
	q = sdraw->dst;
	y = sdraw->y;
	do {
		if (1) {
			*(UINT16 *)q = 0;
			for (x = 0; x < sdraw->width; x++) {
				WORD c;
				//				RGB32 rgb32;

				q += sdraw->xalign;
				c = p[x];
				//if (c) {
				/*
					rgb32.d = 
						RGB32D(
							colorlevel5[(c & 0x03e0) >> 5], 
							colorlevel6[(c & 0xfc00) >> 10],
							colorlevel5[c & 0x1f] );
					*(UINT16 *)q = scrnmng_makepal16(rgb32);
*/
				*(UINT16 *)q = drawcolor16[c];
				//}
			}
			q -= sdraw->xbytes;
		}
		p += SURFACE_WIDTH;
		q += sdraw->yalign;
	} while (++y < maxy);

	// 本当は、srcも保存できないとダメ
	sdraw->dst = q;
	sdraw->y = y;
}

const SDRAWFNVA sdrawva_getproctbl(const SCRNSURF *surf) {
	(void)surf;
	return (sdrawva16);
}
