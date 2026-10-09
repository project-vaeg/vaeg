/*
 * sysportva.c: PC-88VA System port/Calendar/Printer/Dip switch
 */

typedef struct {
	BYTE a; // bit5: SPEED, bit4-1: sw5-2
	BYTE dmy1;
	BYTE c;
	BYTE port010;
	BYTE port032;
	BYTE port040;
	WORD modesw; // bit0: X88V1, bit1: X88V2
	BYTE port190;
	BYTE dmy2[31];
} _SYSPORTVA, *SYSPORTVA;

typedef struct {
	BYTE dipsw; // bit0: CRTモード 1..24KHz 0..15KHz
} _SYSPORTVACFG, *SUSPORTVACFG;

#ifdef __cplusplus
extern "C" {
#endif

extern _SYSPORTVA sysportva;
extern _SYSPORTVACFG sysportvacfg;

void systemportva_reset(void);
void systemportva_bind(void);

#ifdef __cplusplus
}
#endif
