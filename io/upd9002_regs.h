/*
 * upd9002_regs.h: PC-88VA CPU port
 */

typedef struct {
	BYTE tcks; // タイマ、カウンタの入力クロック指定
	           // bit4-2 タイマ2-0への供給クロック 0..内部 1..外部
	           // bit1-0 分周比　00..2 01..4 10..8 11..16
	BYTE dmy[15];
} UPD9002_REGS;

/* Separate state section: do not repurpose the legacy CPU-port padding. */
typedef struct {
	BYTE ranges[8]; /* FFE0h..FFE7h, low/high bytes of two inclusive ranges. */
	BYTE control;   /* FFEFh: IN/OUT enables and port-matching mode. */
} UPD9002_IOTRAP;

#ifdef __cplusplus
extern "C" {
#endif

extern UPD9002_REGS upd9002_regs;
extern UPD9002_IOTRAP upd9002_iotrap;

void upd9002_regs_reset(void);
void upd9002_regs_bind(void);

#ifdef __cplusplus
}
#endif
