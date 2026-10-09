/*
 * videova.h: PC-88VA Video Control
 */

enum {
	VIDEOVA_PALETTES = 32,
	VIDEOVA_FRAMEBUFFERS = 4,

	VIDEOVA_TEXTSCREEN = 0,
	VIDEOVA_SPRITESCREEN = 1,
	VIDEOVA_GRAPHICSCREEN0 = 2,
	VIDEOVA_GRAPHICSCREEN1 = 3,
	VIDEOVA_SCREENS = 4,

	VIDEOVA_PALETTE_SCREENS = 4, // パレット制御画面に指定可能な画面数
	VIDEOVA_RGB_SCREENS = 2,     // 直接色指定画面に指定可能な画面数

	VIDEOVA_15_98KHZ = 0,
	VIDEOVA_15_73KHZ = 1,
	VIDEOVA_24_8KHZ = 2,
};

typedef struct {
	DWORD fsa; // スタートアドレス (下位2bit=0,#1には設定不可)
	WORD fbw;  // 横幅 (下位2bit=0)
	WORD fbl;  // 縦幅 (#1には設定不可)
	WORD dot;  // ドットアドレス
	WORD ofx;  // 実表示画面のXオフセット(10bit,下位2bit=0, #1には設定不可)
	WORD ofy;  // 実表示画面のYオフセット(10bit,#1には設定不可)
	DWORD dsa; // 実表示画面の表示開始アドレス(下位2bit=0)
	WORD dsh;  // サブ画面高さ (9bit)
	WORD dsp;  // サブ画面表示位置 (9bit)
} _FRAMEBUFFER, *FRAMEBUFFER;

typedef struct {
	BYTE txtmode8; // 030h テキスト制御ポート
	BYTE dmy1;
	WORD grmode;      // 100h 表示画面制御レジスタ
	WORD grres;       // 102h グラフィック画面制御レジスタ
	WORD colcomp;     // 106h パレット指定画面制御レジスタ
	WORD rgbcomp;     // 108h 直接色指定画面制御レジスタ
	WORD mskmode;     // 10ah 画面マスクモードレジスタ
	WORD palmode;     // 10Ch パレットモードレジスタ
	WORD dropcol;     // 10Eh バックドロップカラー
	WORD pagemsk;     // 110h カラーコード/プレーンマスクレジスタ
	WORD xpar_g0;     // 124h グラフィック画面0透明色レジスタ
	WORD xpar_g1;     // 126h グラフィック画面1透明色レジスタ
	WORD xpar_txtspr; // 12eh テキスト/スプライト透明色レジスタ
	WORD mskleft;     // 130h 画面マスクパラメータ
	WORD mskrit;      // 132h
	WORD msktop;      // 134h
	WORD mskbot;      // 136h
	BYTE txtmode;     // 148h テキスト制御ポート
	BYTE dmy2;
	WORD palette[VIDEOVA_PALETTES];

	BYTE crtmode; // dip sw1の値(リセット時に読み取り) 1..24KHz, 0..15KHz
	BYTE dmy3;

	BYTE dmy4[64];

	UINT16 blinkcnt; // 1サイクルごとにインクリメントされるカウンタ
	_FRAMEBUFFER framebuffer[VIDEOVA_FRAMEBUFFERS];

	BYTE dmy5[128];
} _VIDEOVA, *VIDEOVA;

#ifdef __cplusplus
extern "C" {
#endif

extern _VIDEOVA videova;

void videova_reset(void);
void videova_bind(void);

int videova_hsyncmode(void);
typedef struct videova_graphics_window {
	UINT16 startdelay;    // blank graphics lines before line 0
	UINT16 startskip;     // graphics lines read but not shown at the top
	UINT16 linelimit;     // graphics lines read; 0xffff for no limit
	BOOL blankafterlimit; // TRUE: blank past the limit; FALSE: repeat the last line
} VIDEOVA_GRAPHICS_WINDOW;

void videova_graphics_window(int hsyncmode, WORD grmode, const UINT8 *syncparam,
                             VIDEOVA_GRAPHICS_WINDOW *window);
int videova_palettemode(void);
BOOL videova_8801_display(void);
BOOL videova_textmerge(void);

#ifdef __cplusplus
}
#endif
