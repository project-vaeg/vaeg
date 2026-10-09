/*
 * mouseifva.h: PC-88VA Mouse interface
 */

typedef struct {
	UINT8 state;
	UINT8 laststrobe;
} _MOUSEIFVA, *MOUSEIFVA;

typedef struct {
	UINT8 device; // マウスポートに接続されている装置
} _MOUSEIFVACFG, *MOUSEIFVACFG;

enum {
	MOUSEIFVA_JOYPAD = 0,
	MOUSEIFVA_MOUSE,
};

#ifdef __cplusplus
extern "C" {
#endif

extern _MOUSEIFVA mouseifva;
extern _MOUSEIFVACFG mouseifvacfg;

void mouseifva_reset(void);
void mouseifva_bind(void);

void mouseifva_outstrobe(UINT8 strobe);
void mouseifva_indata(UINT8 *data4, UINT8 *data2);

void mouseva_outstrobe(UINT8 strobe);
void mouseva_indata(UINT8 *data, UINT8 *button);
void joypad_indata(UINT8 *data, UINT8 *button);

#ifdef __cplusplus
}
#endif
