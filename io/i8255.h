/*
 * i8255.c: i8255
 *
 */

#if !defined(_i8255_h_)
#define _i8255_h_

typedef void (*I8255_busout)(BYTE);

// 状態
typedef struct {
	BYTE porta;
	BYTE portb;
	BYTE portc;
	BYTE mode;
	BYTE portcinmask;
} _I8255, *I8255;

// 構成(接続)
typedef struct {
	I8255 s;
	I8255_busout busoutporta;
	I8255_busout busoutportb;
	I8255_busout busoutportc;
} _I8255CFG, *I8255CFG;

#ifdef __cplusplus
extern "C" {
#endif

void i8255_init(I8255CFG p, I8255 s);
void i8255_reset(I8255CFG p);

void i8255_outporta(I8255CFG p, BYTE dat);
BYTE i8255_inporta(I8255CFG p);
void i8255_outportb(I8255CFG p, BYTE dat);
BYTE i8255_inportb(I8255CFG p);
void i8255_outportc(I8255CFG p, BYTE dat);
BYTE i8255_inportc(I8255CFG p);

void i8255_outctrl(I8255CFG p, BYTE dat);

void i8255_businporta(I8255CFG p, BYTE dat);
void i8255_businportb(I8255CFG p, BYTE dat);
void i8255_businportc(I8255CFG p, BYTE dat);

#ifdef __cplusplus
}
#endif

#endif /* _i8255_h_ */
