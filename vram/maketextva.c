/*
 * maketextva.c: PC-88VA Text
 */
/*
ToDo:
	TEXTVA_ATR_UL	= 0x20,		// アンダーライン
	TEXTVA_ATR_DWID	= 0x40,		// ダブルウィドス
	TEXTVA_ATR_DWIDC= 0x80,		// ダブルウィドスコントロール

    アトリビュートモード2,3,4,5
*/

#include "compiler.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "scrnmng.h"
#include "scrndraw.h"
#include "scrndrawva.h"
//#include	"dispsync.h"

#include "cgromva.h"
#include "memoryva.h"
#include "tsp.h"
#include "videova.h"
#include "sysportva.h"
#include "maketextva.h"

//#define	USETABLE					// 返って遅くなってしまうようだ。
//#define	USETABLE2					// 効果がなさそうだ。
#define SLEEP_HACK

enum {
	TEXTVA_LINEHEIGHTMAX = 20,
	TEXTVA_CHARWIDTH = 8,
	TEXTVA_SURFACE_WIDTH = 1024, // テキストの座標系の幅(ドット)は、1024である。

	TEXTVA_FRAMES = 4, // 分割画面の最大数

	TEXTVA_ATR_ST = 0x01,    // シークレット
	TEXTVA_ATR_BL = 0x02,    // ブリンク
	TEXTVA_ATR_RV = 0x04,    // リバース
	TEXTVA_ATR_HL = 0x08,    // ホリゾンタルライン(mode 1)
	TEXTVA_ATR_HL2 = 0x10,   // ホリゾンタルライン(mode 2,3)
	TEXTVA_ATR_UL = 0x20,    // アンダーライン
	TEXTVA_ATR_DWID = 0x40,  // ダブルウィドス
	TEXTVA_ATR_DWIDC = 0x80, // ダブルウィドスコントロール
};

typedef struct {
	UINT8 bg;
	UINT8 fg;
	UINT8 attr;
} _CHARATTR, *CHARATTR;

typedef void (*SYNATTRFN)(BYTE, CHARATTR); // アトリビュート合成ルーチン

typedef struct {        // テキスト分割画面制御テーブル(のコピー)
	UINT16 vw;          // フレームバッファ横幅(バイト)
	UINT8 mode;         // 表示モード
	UINT8 fg;           // フォアグラウンドカラー
	UINT8 bg;           // バックグラウンドカラー
	UINT8 rasteroffset; // ラスタアドレスオフセット
	UINT32 rsa;         // 分割画面スタートアドレス(TVRAM先頭を0としたバイトアドレス)
	UINT16 rh;          // 分割画面の高さ(ラスタ) (16以上の偶数)
	UINT16 rw;          // 分割画面の横幅(ドット)(32の倍数、設定値/8+2文字が表示される)
	UINT16 rwchar;      // 分割画面の横幅(文字) = rw / 8 + 2
	UINT16 rxp;         // 分割画面水平開始位置(ドット)
} _TEXTVAFRAME, *TEXTVAFRAME;

typedef struct {
	UINT screeny; // 現在処理中のラスタ(画面共通の座標系で)
	UINT y;       // 現在処理中のラスタ(テキストの座標系で)
	UINT raster;
	UINT texty;
	UINT lineheight; // 1行のラスタ数
	                 /*
	UINT16	rsa;		// 分割画面スタートアドレス
	UINT16	vw;			// フレームバッファの横幅(バイト)
	UINT16	rw;			// 分割画面の横幅(ドット) 32の倍数 この値/8+2 文字表示される
	UINT16	rwchar;		// 分割画面の横幅(文字) = rw / 8 + 2 
*/
	BOOL linebitmap_ready;
#if defined(SLEEP_HACK)
	BOOL allzero; // 全て空白かつアンダーラインなしかつブリンクなしかつBG=0
	BOOL sleep;   // 前回allzeroなので表示休止
#endif

	UINT frameno;      // 現在参照している分割画面の番号
	UINT framelimit;   // 次の分割画面の開始位置(ラスタ)
	SYNATTRFN synattr; // アトリビュート合成ルーチン
	TEXTVAFRAME frame; // 現在参照している分割画面へのポインタ
	_TEXTVAFRAME _frame[TEXTVA_FRAMES];
	UINT8 emu_color;  // uPD3301 emulation: colour carried to the next row
	UINT8 emu_deco;   // uPD3301 emulation: decoration carried to the next row
	UINT8 emu_active; // emu_color/emu_deco are initialised for this emulation
} _TEXTVAWORK;

static _TEXTVAWORK work;

static BYTE linebitmap[TEXTVA_SURFACE_WIDTH * TEXTVA_LINEHEIGHTMAX];
// テキスト1行分のbitmap

BYTE textraster[SURFACE_WIDTH]; // 1ラスタ分のピクセルデータ
                                // 各ピクセルはパレット番号(0～15)

// Per-pixel colour of the character cell (after reverse, before secret and
// blink) for the current text row and raster; multiplane 1 bit/pixel
// graphics are drawn in this colour.
static BYTE linecolor[TEXTVA_SURFACE_WIDTH];
BYTE textcolorraster[SURFACE_WIDTH];

#if defined(USETABLE)
static DWORD font2bitmap[16][16][16]; // [fg][bg][4bit分のビットマップ]
#endif

#if defined(USETABLE2)
static DWORD font2bitmap[16];
#endif

void maketextva_initialize(void) {
#if defined(USETABLE)
	UINT8 fg;
	UINT8 bg;
	UINT8 dat;
	UINT8 fontdata;
	int i;

	for (fg = 0; fg < 16; fg++) {
		for (bg = 0; bg < 16; bg++) {
			for (dat = 0; dat < 0x10; dat++) {
				fontdata = dat;
				for (i = 0; i < 4; i++) {
					((BYTE *)&font2bitmap[fg][bg][dat])[i] = (fontdata & 0x08) ? fg : bg;
					fontdata <<= 1;
				}
			}
		}
	}
#endif

#if defined(USETABLE2)
	UINT8 dat;
	UINT8 fontdata;
	int i;

	for (dat = 0; dat < 0x10; dat++) {
		fontdata = dat;
		for (i = 0; i < 4; i++) {
			((BYTE *)&font2bitmap[dat])[i] = (fontdata & 0x08) ? 0xff : 0;
			fontdata <<= 1;
		}
	}

#endif
}

// attribute mode 0
static void synattr0(BYTE attr, CHARATTR charattr) {
	charattr->bg = attr >> 4;
	charattr->fg = attr & 0x0f;
	charattr->attr = 0;
}

// attribute mode 1
static void synattr1(BYTE attr, CHARATTR charattr) {
	charattr->bg = work.frame->bg;
	charattr->fg = attr >> 4;
	charattr->attr = attr & 0x0f;
}

static void makeline(BYTE *v, UINT16 rwchar) {
	int x;
	UINT r;
	int i;
	BYTE *b;
	WORD hccode;
	BYTE *font;
	BYTE fontdata;
	BYTE attr;
	int fontw;
	UINT fonth;
	_CHARATTR charattr;
	UINT8 bg; //バックグラウンドカラー
	UINT8 fg; //フォアグラウンドカラー
	UINT8 ul; //アンダーラインのカラー
#if defined(USETABLE2)
	DWORD bg4;
	DWORD fg4;
	DWORD bitmap4;
#endif

	/*
	if (rwchar > SURFACE_WIDTH / TEXTVA_CHARWIDTH) {
		rwchar = SURFACE_WIDTH / TEXTVA_CHARWIDTH;
	}
*/
	if (rwchar > TEXTVA_SURFACE_WIDTH / TEXTVA_CHARWIDTH) {
		rwchar = TEXTVA_SURFACE_WIDTH / TEXTVA_CHARWIDTH;
	}

	ZeroMemory(linebitmap, sizeof(linebitmap));
	ZeroMemory(linecolor, sizeof(linecolor));
	b = linebitmap;

	for (x = 0; x < rwchar; x++) {
		hccode = LOADINTELWORD(v);
		attr = *(v + tsp.attroffset);
		v += 2;
		work.synattr(attr, &charattr);

		if (charattr.attr & TEXTVA_ATR_RV) {
			bg = charattr.fg;
			fg = charattr.bg;
		} else {
			bg = charattr.bg;
			fg = charattr.fg;
		}
		ul = fg;
		FillMemory(linecolor + x * TEXTVA_CHARWIDTH, TEXTVA_CHARWIDTH, fg);
		if (charattr.attr & TEXTVA_ATR_ST) {
			fg = bg;
			// アンダーラインは表示される(シークレットにならない)
		} else if ((charattr.attr & TEXTVA_ATR_BL) && ((tsp.blinkcnt2 & 0x18) == 0x08)) {
			fg = bg;
			// アンダーラインはブリンクしない
		}

#if defined(USETABLE2)
		fg4 = fg | ((DWORD)fg << 8) | ((DWORD)fg << 16) | ((DWORD)fg << 24);
		bg4 = bg | ((DWORD)bg << 8) | ((DWORD)bg << 16) | ((DWORD)bg << 24);
#endif

		if ((hccode == 0 || hccode == 0x20) && bg == 0 &&
		    ((charattr.attr & (TEXTVA_ATR_HL | TEXTVA_ATR_HL2)) == 0)) {
			// 空白、背景色0、アンダーラインなし
			b += TEXTVA_CHARWIDTH;
		} else {
#if defined(SLEEP_HACK)
			work.allzero = FALSE;
#endif

			font = cgromva_font(hccode);
			fontw = cgromva_width(hccode);
			fonth = (videova.txtmode & 0x04) ? 8 : 16;

			for (r = 0; r < work.lineheight; r++) {
				fontdata = *font;
				font += fontw;
				if ((charattr.attr & (TEXTVA_ATR_HL | TEXTVA_ATR_HL2)) && r == tsp.hlinepos) {
					for (i = 0; i < 8; i++) {
						b[i] = ul;
					}
				} else if (r < fonth) {
#if defined(USETABLE)
					((DWORD *)b)[0] = font2bitmap[fg][bg][fontdata >> 4];
					((DWORD *)b)[1] = font2bitmap[fg][bg][fontdata & 0x0f];
#else
#if defined(USETABLE2)
					bitmap4 = font2bitmap[fontdata >> 4];
					*((DWORD *)b) = bitmap4 & fg4 | ~bitmap4 & bg4;
					bitmap4 = font2bitmap[fontdata & 0x0f];
					*((DWORD *)(b + 4)) = bitmap4 & fg4 | ~bitmap4 & bg4;
#else
					for (i = 0; i < 8; i++) {
						b[i] = (fontdata & 0x80) ? fg : bg;
						fontdata <<= 1;
					}
#endif
#endif
				} else {
					for (i = 0; i < 8; i++) {
						b[i] = bg;
					}
				}
				b += TEXTVA_SURFACE_WIDTH;
			}
			b -= TEXTVA_SURFACE_WIDTH * work.lineheight - TEXTVA_CHARWIDTH;
		}
	}
}

/*
 * uPD3301 emulation (TSP EMUL 8Ch, V1/V2 mode). The split screen is read in
 * byte mode; each row holds `emul_chars` character bytes followed by
 * `emul_attrs` (column, attribute) pairs. `v` is the fetched row, which
 * starts two bytes before the logical row: the VA2 ROM programs the start
 * address two bytes early and hides them with rxp, matching the TSP's
 * rw/8 + 2 fetch. Attribute bytes follow the PC-8801 text format: bit 3 set
 * is colour (bits 7-5 G, R, B; bit 4 semigraphics), bit 3 clear is
 * decoration (bit 0 secret, 1 blink, 2 reverse, 4 upper line, 5 under
 * line). The eight colours become colour codes 8-15 (BNN manual 8.2.1).
 * Semigraphics carries with the colour, as in X88000; the BNN manual does
 * not list it among the 3301 functions the TSP leaves out (8.2.1).
 *
 * Pair walk: X88000 1.5.3 (public domain), X88ScreenDrawer.cpp, transparent
 * attribute mode. Pairs are consumed in memory order, at most one per
 * character position. When the first pair's column is not zero, each
 * attribute takes effect from the previous pair's column and the first
 * attribute from column 0 (N-88 BASIC's function-key row relies on this).
 * Colour, secret, blink and reverse carry to the next row and frame; upper
 * and under lines start each row clear.
 */
enum {
	EMU3301_SECRET = 0x01,
	EMU3301_BLINK = 0x02,
	EMU3301_REVERSE = 0x04,
	EMU3301_UPPER = 0x10,
	EMU3301_UNDER = 0x20,
	EMU3301_CARRY = EMU3301_SECRET | EMU3301_BLINK | EMU3301_REVERSE,
	EMU3301_GRAPHIC = 0x08, /* in color[]: semigraphics, beside colour bits 2-0 */
	EMU3301_MAXCHARS = 128,
	EMU3301_MAXPAIRS = 32
};

static void makeline_3301(const BYTE *v, UINT16 rwchar) {
	UINT8 color[EMU3301_MAXCHARS];
	UINT8 deco[EMU3301_MAXCHARS];
	const BYTE *row;
	const BYTE *pairs;
	UINT chars;
	int npairs;
	int n;
	int idx;
	int ofs;
	BOOL shifted;
	UINT i;
	UINT col;
	UINT run;
	UINT8 curcolor;
	UINT8 curdeco;
	BYTE *b;

	chars = tsp.emul_chars;
	if (chars > EMU3301_MAXCHARS) {
		chars = EMU3301_MAXCHARS;
	}
	npairs = tsp.emul_attrs;
	if (npairs > EMU3301_MAXPAIRS) {
		npairs = EMU3301_MAXPAIRS;
	}
	row = v + 2;
	pairs = row + tsp.emul_chars;
	shifted = (npairs > 0) && (pairs[0] != 0);
	if (shifted) {
		n = -1;
		idx = -1;
		ofs = -1;
	} else {
		n = 0;
		idx = 0;
		ofs = (npairs > 0) ? pairs[0] : 0x100;
	}
	if (!work.emu_active) {
		/* White, no decoration (X88000 starts from attribute E0h). */
		work.emu_color = 7;
		work.emu_deco = 0;
		work.emu_active = 1;
	}
	curcolor = work.emu_color;
	curdeco = work.emu_deco & EMU3301_CARRY;
	run = 0;
	for (col = 0; col < chars; col++) {
		int attr = -1;
		if (shifted) {
			if ((n < npairs - 1) && ((int)col >= ofs)) {
				idx++;
				n++;
				attr = pairs[idx * 2 + 1];
				ofs = pairs[idx * 2];
			}
		} else if ((n < npairs) && ((int)col >= ofs)) {
			attr = pairs[idx * 2 + 1];
			idx++;
			n++;
			ofs = (idx < npairs) ? pairs[idx * 2] : 0x100;
		}
		if (attr >= 0) {
			for (; run < col; run++) { /* fill the run before this change */
				color[run] = curcolor;
				deco[run] = curdeco;
			}
			if (attr & 0x08) {
				curcolor = (UINT8)((attr >> 5) | ((attr & 0x10) ? EMU3301_GRAPHIC : 0));
			} else {
				curdeco = (UINT8)attr;
			}
		}
	}
	for (; run < chars; run++) {
		color[run] = curcolor;
		deco[run] = curdeco;
	}
	work.emu_color = curcolor;
	work.emu_deco = curdeco;

	if (rwchar > TEXTVA_SURFACE_WIDTH / TEXTVA_CHARWIDTH) {
		rwchar = TEXTVA_SURFACE_WIDTH / TEXTVA_CHARWIDTH;
	}
	ZeroMemory(linebitmap, sizeof(linebitmap));
	ZeroMemory(linecolor, sizeof(linecolor));
	b = linebitmap;
	for (i = 0; i < rwchar; i++, b += TEXTVA_CHARWIDTH) {
		const int c = (int)i - 2;
		UINT8 fg;
		UINT8 bg;
		UINT8 d;
		BYTE code;
		const BYTE *font;
		UINT fonth;
		UINT r;
		int x;

		if ((c < 0) || ((UINT)c >= chars)) {
			continue;
		}
		code = row[c];
		d = deco[c];
		fg = (UINT8)(8 + (color[c] & 0x07));
		bg = work.frame->bg;
		if (d & EMU3301_REVERSE) {
			const UINT8 t = fg;
			fg = bg;
			bg = t;
		}
		FillMemory(linecolor + i * TEXTVA_CHARWIDTH, TEXTVA_CHARWIDTH, fg);
		if ((code == 0 || ((code == 0x20) && !(color[c] & EMU3301_GRAPHIC))) && (bg == 0) &&
		    !(d & (EMU3301_UPPER | EMU3301_UNDER))) {
			continue;
		}
#if defined(SLEEP_HACK)
		work.allzero = FALSE;
#endif
		{
			const UINT8 linecolor = fg;
			UINT8 glyphfg = fg;
			if ((d & EMU3301_SECRET) || ((d & EMU3301_BLINK) && ((tsp.blinkcnt2 & 0x18) == 0x08))) {
				glyphfg = bg;
			}
			const BOOL ank8 = videova_8801_display();
			font = ank8 ? cgromva_ank8(code) : cgromva_font(code);
			fonth = (ank8 || !(videova.txtmode & 0x04)) ? 16 : 8;
			for (r = 0; r < work.lineheight; r++) {
				BYTE *p = b + TEXTVA_SURFACE_WIDTH * r;
				const BOOL line = ((d & EMU3301_UPPER) && (r == 0)) ||
				                  ((d & EMU3301_UNDER) && (r == work.lineheight - 1));
				/* PC-8801 style: each 8x8 row on two rasters. */
				const UINT fontrow = ank8 ? (r >> 1) : r;
				BYTE fontdata = (r < fonth) ? font[fontrow * (ank8 ? 1 : cgromva_width(code))] : 0;
				if (color[c] & EMU3301_GRAPHIC) {
					/* 2x4 blocks: bits 0-3 left column, 4-7 right, top first. */
					const UINT block = (r < fonth) ? (r * 4) / fonth : 4;
					fontdata = (block < 4) ? (BYTE)((((code >> block) & 1) ? 0xf0 : 0) |
					                                (((code >> (block + 4)) & 1) ? 0x0f : 0))
					                       : 0;
				}
				for (x = 0; x < TEXTVA_CHARWIDTH; x++) {
					p[x] = line ? linecolor : ((fontdata & 0x80) ? glyphfg : bg);
					fontdata <<= 1;
				}
			}
		}
	}
}

/*
 * PC-8801 40-column text (port 30h 80CM clear) under 3301 emulation: the
 * characters stay at even byte columns of the 80-byte row and each is
 * shown twice as wide, covering its odd neighbour (as X88000 skips odd
 * columns). The two hidden lead cells keep even cells aligned with even
 * logical columns, so each even cell is stretched over itself and the next.
 */
static void widen40_3301(UINT16 rwchar) {
	UINT r;
	UINT i;
	int x;

	if (rwchar > TEXTVA_SURFACE_WIDTH / TEXTVA_CHARWIDTH) {
		rwchar = TEXTVA_SURFACE_WIDTH / TEXTVA_CHARWIDTH;
	}
	for (r = 0; r < work.lineheight; r++) {
		BYTE *row = linebitmap + TEXTVA_SURFACE_WIDTH * r;
		for (i = 0; i + 1 < rwchar; i += 2) {
			BYTE *cell = row + i * TEXTVA_CHARWIDTH;
			for (x = TEXTVA_CHARWIDTH * 2 - 1; x >= 0; x--) {
				cell[x] = cell[x / 2];
			}
		}
	}
	for (i = 0; i + 1 < rwchar; i += 2) {
		FillMemory(linecolor + (i + 1) * TEXTVA_CHARWIDTH, TEXTVA_CHARWIDTH,
		           linecolor[i * TEXTVA_CHARWIDTH]);
	}
}

/*
40桁に拡大する処理(linebitmapを加工)
*/
static void conv40cm(UINT16 rwchar) {
	BYTE *b;
	UINT r;
	UINT x;

	if (rwchar > TEXTVA_SURFACE_WIDTH / TEXTVA_CHARWIDTH) {
		rwchar = TEXTVA_SURFACE_WIDTH / TEXTVA_CHARWIDTH;
	}

	b = linebitmap;
	for (r = 0; r < work.lineheight; r++) {
		for (x = 0; x < rwchar; x++) {
			BYTE tmp[4];
			if (x & 1) {
				// 80桁換算時に奇数桁→右半分を拡大
				*(DWORD *)tmp = *((DWORD *)(b + 4));
			} else {
				// 80桁換算時に偶数桁→左半分を拡大
				*(DWORD *)tmp = *(DWORD *)b;
			}
			*b++ = tmp[0];
			*b++ = tmp[0];
			*b++ = tmp[1];
			*b++ = tmp[1];
			*b++ = tmp[2];
			*b++ = tmp[2];
			*b++ = tmp[3];
			*b++ = tmp[3];
		}
		b += TEXTVA_SURFACE_WIDTH - rwchar * TEXTVA_CHARWIDTH;
	}
}

#if 0
void maketextva(void) {
	UINT	x;
	UINT	y;
	UINT	raster;
	UINT	texty;
	UINT16	rsa;		// 分割画面スタートアドレス
	UINT16	vw;			// フレームバッファの横幅(バイト)
	UINT16	rw;			// 分割画面の横幅(ドット) 32の倍数 この値/8+2 文字表示される
	UINT16	rwchar;		// 分割画面の横幅(文字) = rw / 8 + 2 
	BYTE	*v;			// TVRAM
	BYTE	*b;			// bitmap
	BYTE	*lb;		// linebitmap
	BOOL	linebitmap_ready;

	linebitmap_ready = FALSE;
	rsa = 0;
	vw = 80*2;
	rw = 640;
	rwchar = rw / 8 + 2;
	raster = 0;
	texty = 0;
	for (y=0; y<SURFACE_HEIGHT; y++) {
		if (!linebitmap_ready) {
			v = textmem + rsa + vw * texty;
			makeline(v, rwchar);

			texty++;
			linebitmap_ready = TRUE;
		}

		b = np2_tbitmap + SURFACE_WIDTH * y;
		lb = linebitmap + SURFACE_WIDTH * raster;
		for (x = 0; x < SURFACE_WIDTH; x++) {
			*b = *lb;
			b++;
			lb++;
		}

		raster++;
		if (raster >= tsp.lineheight) {
			linebitmap_ready = FALSE;
			raster = 0;
		}
	}


}
#endif

static void selectframe(int no) {
	static SYNATTRFN synattrtbl[] = {synattr0, synattr1, synattr0, synattr0,
	                                 synattr0, synattr0, synattr0, synattr0};

	work.frameno = no;
	work.frame = &work._frame[no];
	work.framelimit = work.y + work.frame->rh;
	if (no == TEXTVA_FRAMES - 1) { // 最後の分割画面なら
		work.framelimit = 0x1fe;   // rh に指定可能な最大値
	}
	work.texty = 0;
	work.raster = work.frame->rasteroffset;
	work.linebitmap_ready = FALSE;
	work.synattr = synattrtbl[work.frame->mode & 0x07];
}

void maketextva_begin(BOOL *scrn200) {
	int i;
	BYTE *fbinfo;

#if defined(SLEEP_HACK)
	/* Main-RAM text (see maketextva_3301_row) does not mark textmem dirty. */
	if (work.allzero && !textmem_dirty && !tsp_dirty && !videova_textmerge() &&
	    !(memoryva_88_mode && (sysportva.port032 & 0x10))) {
		work.sleep = TRUE;
	} else {
		work.sleep = FALSE;
	}
	textmem_dirty = FALSE;
	tsp_dirty = FALSE;
	work.allzero = TRUE;
#endif

	work.y = 0;
	work.screeny = 0;
	if (!tsp.emul) {
		/* uPD3301 attributes carry across frames while emulating. */
		work.emu_active = 0;
	}

	work.lineheight = tsp.lineheight;
	if (work.lineheight > TEXTVA_LINEHEIGHTMAX) {
		work.lineheight = TEXTVA_LINEHEIGHTMAX;
	}

	// 分割画面制御テーブルの情報をコピー
	fbinfo = textmem + tsp.texttable;
	for (i = 0; i < TEXTVA_FRAMES; i++) {
		TEXTVAFRAME f;
		WORD d;

		f = &work._frame[i];

		f->vw = LOADINTELWORD(fbinfo + 0x08) & 0x03ff;
		d = LOADINTELWORD(fbinfo + 0x0a);
		f->mode = d & 0x1f;
		f->bg = (d & 0x0f00) >> 8;
		f->fg = (d & 0xf000) >> 12;
		f->rasteroffset = fbinfo[0x0d] & 0x1f;
		f->rsa = LOADINTELWORD(fbinfo + 0x10); // ToDo: テクマニでは16bitだが、18bitではないか？
		f->rh = LOADINTELWORD(fbinfo + 0x14) & 0x01fe;
		if (f->rh == 0)
			f->rh = 0x01fe;
		f->rw = LOADINTELWORD(fbinfo + 0x16) & 0x03ff;
		f->rwchar = f->rw / TEXTVA_CHARWIDTH + 2;
		f->rxp = LOADINTELWORD(fbinfo + 0x1a) & 0x03ff;

		fbinfo += 0x0020;
	}

	selectframe(0);

	//*scrn200 = tsp.hsync15khz && ((tsp.syncparam[0] & 0xc0) != 0x40);
	*scrn200 = (videova_hsyncmode() != VIDEOVA_24_8KHZ) && ((tsp.syncparam[0] & 0xc0) != 0x40);
}

/*
 * TSP byte-access mode (V1/V2): BNN manual 8.2.1 maps the local word
 * addresses 3000h-37FFh and B000h-B7FFh to the byte addresses 3000h-3FFFh
 * and B000h-BFFFh, the 3000h range being TVRAM A6000h-A6FFFh. Keeping the
 * low 12 bits and doubling the upper four fits both ranges.
 */
UINT32 maketextva_bytelocal(UINT32 local) {
	return ((((local & 0xf000) << 1) | (local & 0x0fff)) & (sizeof(textmem) - 1));
}

/*
 * Only 3000h-3FFFh and B000h-BFFFh are usable in byte-access mode (BNN
 * manual 8.2.1); rows starting elsewhere are not displayed. After a uPD3301
 * RESET the VA2 ROM moves the emulated screen to local 0800h, so this also
 * leaves the text blank while the 3301 display is stopped. `[DERIVED]`: the
 * real TSP's output for these addresses is not documented.
 */
/*
 * Source of an emulated 3301 row at TVRAM byte `offset`. With port 32h TMODE
 * set in 88 mode, PC-8801 software keeps its text in main RAM F000h-FFFFh
 * (no fast TVRAM), as N-88 BASIC does in V1S mode. The VA always reports
 * high speed and never runs that way; under the vaeg standard-speed
 * extension the 4 KiB TVRAM window (6000h-6FFFh) is then read from the
 * 88-mode main RAM instead, as the PC-8801's CRTC DMA would.
 */
static BYTE emul_row_ram[256];

static const BYTE *maketextva_3301_row(UINT32 offset) {
	UINT i;

	if (!memoryva_88_mode || !(sysportva.port032 & 0x10) || (offset < 0x6000) ||
	    (offset >= 0x7000)) {
		return textmem + offset;
	}
	for (i = 0; i < sizeof(emul_row_ram); i++) {
		emul_row_ram[i] = mem[0x1f000 + ((offset - 0x6000 + i) & 0x0fff)];
	}
	return emul_row_ram;
}

/*
 * Characters of row `row` of the emulated uPD3301 screen, as displayed (for
 * the GUI's text copy), or NULL past its last row or outside the usable
 * byte-mode ranges. The two hidden lead cells are skipped; tsp.emul_chars
 * bytes follow.
 */
const BYTE *maketextva_3301_text(UINT row) {
	const BYTE *frame;
	UINT32 local;

	if (!tsp.emul || (row >= tsp.emul_rows)) {
		return NULL;
	}
	frame = textmem + ((tsp.texttable + tsp.emul_frame * 32) & (sizeof(textmem) - 32));
	local = (LOADINTELWORD(frame + 0x10) >> 1) + (LOADINTELWORD(frame + 0x08) >> 1) * row;
	if (!maketextva_bytelocal_usable(local)) {
		return NULL;
	}
	return maketextva_3301_row(maketextva_bytelocal(local)) + 2;
}

BOOL maketextva_bytelocal_usable(UINT32 local) {
	return ((local >= 0x3000) && (local < 0x4000)) || ((local >= 0xb000) && (local < 0xc000));
}

void maketextva_blankraster(void) {
	ZeroMemory(textraster, sizeof(textraster));
	ZeroMemory(textcolorraster, sizeof(textcolorraster));
}

void maketextva_raster(void) {
	UINT x;
	BYTE *v;   // TVRAM
	BYTE *b;   // bitmap
	BYTE *lb;  // linebitmap
	BYTE *lbs; // linebitmap 当該ラインの左端
	TEXTVAFRAME f;

	if (!tsp.dspon) {
		maketextva_blankraster();
		return;
	}

	if (videova.txtmode & 0x80) {
		// テキスト表示OFF
		maketextva_blankraster();
		return;
	}

#if defined(SLEEP_HACK)
	if (work.sleep) {
		maketextva_blankraster();
		return;
	}
#endif

	if (!tsp.textmg || (work.screeny & 1) == 0) {
		if (work.y >= SURFACE_HEIGHT)
			return;

		while (work.y >= work.framelimit) {
			work.frameno++;
			selectframe(work.frameno);
		}

		f = work.frame;

		if (!work.linebitmap_ready) {
			if (tsp.emul && (work.frameno == tsp.emul_frame)) {
				/* Byte mode: the table's address and pitch are twice the
				 * local byte values; see maketextva_bytelocal(). */
				const UINT32 local = (f->rsa >> 1) + (f->vw >> 1) * work.texty;

				if ((work.texty < tsp.emul_rows) && maketextva_bytelocal_usable(local)) {
					v = (BYTE *)maketextva_3301_row(maketextva_bytelocal(local));
					makeline_3301(v, f->rwchar);
					if (!(videova.txtmode8 & 0x01)) {
						widen40_3301(f->rwchar);
					}
				} else {
					ZeroMemory(linebitmap, sizeof(linebitmap));
					ZeroMemory(linecolor, sizeof(linecolor));
				}
			} else {
				v = textmem + f->rsa + f->vw * work.texty;
				makeline(v, f->rwchar);
				if (!videova.txtmode8 & 0x01) {
					// 40桁モード
					conv40cm(f->rwchar);
				}
			}

			work.texty++;
			work.linebitmap_ready = TRUE;
		}
		/*
		b = textraster;
		lb = linebitmap + SURFACE_WIDTH * work.raster;
		for (x = 0; x < SURFACE_WIDTH; x++) {
			*b = *lb;
			b++;
			lb++;
		}
	*/
		b = textraster;
		lbs = linebitmap + TEXTVA_SURFACE_WIDTH * work.raster;
		if (f->rxp) {
			lb = lbs + TEXTVA_SURFACE_WIDTH - f->rxp;
		} else {
			lb = lbs;
		}
		x = 0;
		if (f->rxp < SURFACE_WIDTH) {
			for (; x < f->rxp; x++) {
				textcolorraster[x] = linecolor[lb - lbs];
				*b = *lb;
				b++;
				lb++;
			}
			lb = lbs;
		}
		for (; x < SURFACE_WIDTH; x++) {
			textcolorraster[x] = linecolor[lb - lbs];
			*b = *lb;
			b++;
			lb++;
		}

		work.raster++;
		if (work.raster >= work.lineheight) {
			work.linebitmap_ready = FALSE;
			work.raster = 0;
		}

		work.y++;
	}
	work.screeny++;
}
