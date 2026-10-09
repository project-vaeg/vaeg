
#define UPD4990_REGLEN 8

typedef struct {
	UINT8 last;
	UINT8 cmd;
	UINT8 serial;
	UINT8 parallel;
	BYTE reg[UPD4990_REGLEN];
	UINT pos;
	UINT8 cdat;
	UINT8 regsft;
} _UPD4990, *UPD4990;

#ifdef __cplusplus
extern "C" {
#endif

void IOOUTCALL upd4990_o20(UINT port, REG8 dat);

void uPD4990_reset(void);
void uPD4990_bind(void);

#ifdef __cplusplus
}
#endif
