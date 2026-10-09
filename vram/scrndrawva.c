#include "compiler.h"
#include "scrnmng.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "iocoreva.h"
#include "scrndraw.h"
#include "scrndrawva.h"
#include "sdrawva.h"
#include "maketextva.h"
#include "makesprva.h"
#include "makegrphva.h"

enum {
	COLORCODES = 0x10000,
};

WORD vabitmap[SURFACE_SIZE]; // 画面合成結果(VA 16bit/pixel)

RGB16 drawcolor16[COLORCODES];

static WORD rgb8to16[256]; // VA 8bit RGB→16bit RGB

static void scrndrawva_initialize(void) {
	static int initialized = 0;

	if (initialized)
		return;
	initialized = 1;

	ZeroMemory(vabitmap, sizeof(vabitmap));

	// VA 8bit RGB→16bit RGB 変換テーブルの作成
	{
		int i, r, g, b;

		for (i = 0; i < 256; i++) {
			b = i & 3;
			r = (i >> 2) & 7;
			g = (i >> 5) & 7;
			rgb8to16[i] = (g << 13) | (g == 0 ? 0 : 0x1c00) | (r << 7) | (r == 0 ? 0 : 0x0060) |
			              (b << 3) | (b == 0 ? 0 : 0x0007);
		}
	}
}

/*
 * Output level (0-255) of a colour component `value` of `bits` bits (5 or 6).
 * Digital RGB monitor: [VA1] schematic pp. 261 and 263, the digital R, G and
 * B pins carry the analog colour drive through 1 kOhm, with no conversion
 * logic, so the monitor's input threshold decides each component. [POLICY]
 * (M103i) on from a quarter of full scale; on a real digital monitor the VA
 * demo looked almost white (maintainer).
 */
BYTE scrndrawva_outputlevel(UINT value, UINT bits, BOOL digital) {
	if (digital) {
		return (value >= (1u << (bits - 2))) ? 0xff : 0;
	}
	return (BYTE)((value << (8 - bits)) | (value ? ((1u << (8 - bits)) - 1) : 0));
}

static void scrndrawva_makedrawcolor(void) {
	BYTE colorlevel5[32]; // 出力レベル変換 5bit→8bit
	BYTE colorlevel6[64]; // 出力レベル変換 6bit→8bit
	int i;
	int c;

	for (i = 0; i < 32; i++) {
		colorlevel5[i] = scrndrawva_outputlevel(i, 5, np2cfg.monitor_digital);
	}
	for (i = 0; i < 64; i++) {
		colorlevel6[i] = scrndrawva_outputlevel(i, 6, np2cfg.monitor_digital);
	}

	for (c = 0; c < COLORCODES; c++) {
		{
			RGB32 rgb32;
			rgb32.d = RGB32D(colorlevel5[(c & 0x03e0) >> 5], colorlevel6[(c & 0xfc00) >> 10],
			                 colorlevel5[c & 0x1f]);
			drawcolor16[c] = scrnmng_makepal16(rgb32);
		}
	}
}

BYTE scrndrawva_draw(BYTE redraw) {
	BYTE ret;
	const SCRNSURF *surf;
	SDRAWFNVA sdrawfn;
	_SDRAWVA sdraw;
	int height;

	scrndrawva_initialize();

	ret = 0;
	surf = scrnmng_surflock();
	if (surf == NULL) {
		goto sddr_exit1;
	}

	sdrawfn = sdrawva_getproctbl(surf);
	if (sdrawfn == NULL) {
		goto sddr_exit2;
	}

	height = surf->height;

	sdraw.dst = surf->ptr;
	sdraw.width = surf->width;
	sdraw.xbytes = surf->xalign * surf->width;
	sdraw.y = 0;
	sdraw.xalign = surf->xalign;
	sdraw.yalign = surf->yalign;
	sdrawfn(&sdraw, height);

sddr_exit2:
	scrnmng_surfunlock(surf);

sddr_exit1:
	return ret;
}

void scrndrawva_redraw(void) {
	scrnmng_allflash();
	scrndrawva_makedrawcolor();
	scrndrawva_draw(1);
}

enum {
	INSIDE = 0,
	OUTSIDE = 1,
};

typedef struct {
	WORD *raster;  // 1ライン分の色データ(パレット番号orVA8bit/16bitカラーコード)
	BOOL mask[2];  // マスク領域の内側([0]),外側([1])をマスク
	int pixelmode; // ピクセルサイズ
	               // 0..1bpp, 1..4bpp, 2..8bpp, 3..16bpp

	// パレット指定画面の場合のみ
	BYTE palflip;   // パレット番号に XORするデータ
	                // パレットセット0使用時は0, 1使用時は0x10
	BYTE pixelmask; // xxxx_raster[]から読み出した値に適用するマスク
	                // 通常0x0f, 32色モード時のみ0x1f
	DWORD xpar;     // 透明色フラグ(bit0:パレット0～bit31:パレット31)

} _COMPSCRN, *COMPSCRN;

typedef struct {
	WORD *bp; // vabitmap
	int y;
	_COMPSCRN scrn[VIDEOVA_PALETTE_SCREENS + VIDEOVA_RGB_SCREENS];
} _COMPOSEWORK;

static _COMPOSEWORK work;

static WORD tsptext_raster[SURFACE_WIDTH];
static WORD tspspr_raster[SURFACE_WIDTH];
// 1ラスタ分のピクセルデータ
// 各ピクセルはパレット番号(0～15)

static BOOL layer_enabled[VAEG_VA_LAYER_COUNT] = {TRUE, TRUE, TRUE, TRUE};

void scrndrawva_set_layer_enabled(UINT layer, BOOL enabled) {
	if (layer >= VAEG_VA_LAYER_COUNT) {
		return;
	}
	layer_enabled[layer] = enabled ? TRUE : FALSE;
}

BOOL scrndrawva_layer_enabled(UINT layer) {
	if (layer >= VAEG_VA_LAYER_COUNT) {
		return FALSE;
	}
	return layer_enabled[layer];
}

void scrndrawva_compose_begin(void) {
	work.bp = vabitmap;
	work.y = 0;
}

void scrndrawva_compose_raster(void) {
	int x;
	int i;
	int palmode;
	int palset1scrn;
	int type;
	int side;
	int maskpos;
	WORD *bp;
	COMPSCRN scrn;
	BYTE palcode;
	WORD c;
	WORD tscr;
	WORD comp;
	BYTE defaultflip;
	WORD xparhigh;
	int blinkfreq;
	BOOL merge;

	// テキストとスプライトの出力を重ね合わせ、
	// テキスト/スプライト判別境界カラーで分離する
	tscr = videova.pagemsk >> 12; // テキスト/スプライト判別境界カラー
	merge = videova_textmerge() && layer_enabled[VAEG_VA_LAYER_GRAPHICS0] && !grph0_noraster;
	for (x = 0; x < SURFACE_WIDTH; x++) {
		palcode = sprraster[x];
		if (palcode == 0) {
			// Multiplane 1 bit/pixel graphics take the colour of the text cell.
			palcode = (merge && grph0_raster[x]) ? textcolorraster[x] : textraster[x];
		}
		if (palcode > tscr) {
			// テキスト
			tsptext_raster[x] = palcode;
			tspspr_raster[x] = 0;
		} else {
			// スプライト
			tsptext_raster[x] = 0;
			tspspr_raster[x] = palcode;
		}
	}

	palmode = videova_palettemode();
	defaultflip = palmode == 1 ? 0x10 : 0x00;
	palset1scrn = (videova.palmode >> 4) & 3;

	// パレットブリンク機能
	blinkfreq = (videova.palmode >> 2) & 3;
	if (palmode < 2 && blinkfreq > 0) {
		// ブリンクon
		// (ブリンクはパレットモード0,1でのみ有効)
		static UINT16 blinkontimetbl[4] = {1, 2, 4, 6}; // ポート10c bit1,0→ブリンクデューティー n/8
		static UINT16 cyclelentbl[4] = {0, 5, 6, 7};    // 1サイクル 2^n フレーム
		UINT16 current = (videova.blinkcnt >> (cyclelentbl[blinkfreq] - 3)) & 7;
		// blinkcnt の第cyclelentbl[blinkfreq] bitから3bit取得
		if (current >= blinkontimetbl[videova.palmode & 3]) {
			// onタイムでない→パレットセットを反転
			defaultflip ^= 0x10;
		}
	}

	scrn = work.scrn;
	comp = videova.colcomp;
	for (i = 0; i < VIDEOVA_PALETTE_SCREENS; i++, scrn++) {
		type = comp & 0x0f;
		comp >>= 4;
		if (type < 8) {
			scrn->raster = NULL;
			/*
			// raster == NULLのときは、mask==TRUEにして、描画されないようにする
			scrn->mask[OUTSIDE] = TRUE;
			scrn->mask[INSIDE] = TRUE;
			continue;
			*/
		} else {
			type &= 0x03;
			scrn->palflip = defaultflip;
			if (palmode == 2 && type == palset1scrn)
				scrn->palflip = 0x10;
			scrn->pixelmask = 0x0f;

			switch (type) {
			case VIDEOVA_TEXTSCREEN:
				scrn->raster = layer_enabled[VAEG_VA_LAYER_TEXT] ? tsptext_raster : NULL;
				scrn->xpar = videova.xpar_txtspr | ((DWORD)videova.xpar_txtspr << 16);
				break;
			case VIDEOVA_SPRITESCREEN:
				scrn->raster = layer_enabled[VAEG_VA_LAYER_SPRITE] ? tspspr_raster : NULL;
				scrn->xpar = videova.xpar_txtspr | ((DWORD)videova.xpar_txtspr << 16);
				break;
			case VIDEOVA_GRAPHICSCREEN0:
				if (merge) {
					// Drawn as part of the text screen.
					scrn->raster = NULL;
				} else if ((videova.grmode & 0x8000) && layer_enabled[VAEG_VA_LAYER_GRAPHICS0]) {
					// GDEN0 = 1 (グラフィック表示イネーブル)
					scrn->raster = (grph0_noraster) ? NULL : grph0_raster;
					scrn->pixelmode = videova.grres & 0x0003;
					xparhigh = videova.xpar_g0;
					if (palmode == 3 && scrn->pixelmode >= 2) {
						// 32色モード
						scrn->pixelmask = 0x1f;
						xparhigh = 0x0000;
					}
					scrn->xpar = videova.xpar_g0 | ((DWORD)xparhigh << 16);
				} else {
					// GDEN0 = 0 (グラフィック表示禁止)
					scrn->raster = NULL;
				}
				break;
			case VIDEOVA_GRAPHICSCREEN1:
				if ((videova.grmode & 0x8000) && layer_enabled[VAEG_VA_LAYER_GRAPHICS1]) {
					// GDEN0 = 1 (グラフィック表示イネーブル)
					scrn->raster = (grph1_noraster) ? NULL : grph1_raster;
					scrn->pixelmode = (videova.grres >> 8) & 0x0003;
					xparhigh = videova.xpar_g1;
					if (palmode == 3 && scrn->pixelmode >= 2) {
						// 32色モード (8bpp/16bpp, パレットモード3)
						scrn->pixelmask = 0x1f;
						xparhigh = 0x0000;
					}
					scrn->xpar = videova.xpar_g1 | ((DWORD)xparhigh << 16);
				} else {
					// GDEN0 = 0 (グラフィック表示禁止)
					scrn->raster = NULL;
				}
				break;
			}

			// screen mask
			maskpos = (videova.mskmode >> 4) & 3;
			if (i == maskpos + 1) {
				// 直後の低優先画面
				scrn->mask[OUTSIDE] = videova.mskmode & 0x04;
				scrn->mask[INSIDE] = videova.mskmode & 0x01;
			} else if (i > maskpos) {
				// 低優先画面
				scrn->mask[OUTSIDE] = (videova.mskmode & 0x0c) == 0x0c;
				scrn->mask[INSIDE] = (videova.mskmode & 0x03) == 0x03;
			} else {
				// 高優先画面
				scrn->mask[OUTSIDE] = (videova.mskmode & 0x0c) == 0x08;
				scrn->mask[INSIDE] = (videova.mskmode & 0x03) == 0x02;
			}
		}

		if (scrn->raster == NULL) {
			// raster == NULLのときは、mask==TRUEにして、描画されないようにする
			scrn->mask[OUTSIDE] = TRUE;
			scrn->mask[INSIDE] = TRUE;
		}
	}

	//scrn = &work.scrn[VIDEOVA_PALETTE_SCREENS];
	comp = videova.rgbcomp;
	for (i = 0; i < VIDEOVA_RGB_SCREENS; i++, scrn++) {
		type = comp & 0x0f;
		comp >>= 4;
		if (type < 8 || type > 9) {
			scrn->raster = NULL;
			/*
			// raster == NULLのときは、mask==TRUEにして、描画されないようにする
			scrn->mask[OUTSIDE] = TRUE;
			scrn->mask[INSIDE] = TRUE;
			continue;
			*/
		} else {
			type = (type & 0x01) + VIDEOVA_GRAPHICSCREEN0;

			if (videova.grmode & 0x8000) {
				// GDEN0 = 1 (グラフィック表示イネーブル)
				switch (type) {
				case VIDEOVA_GRAPHICSCREEN0:
					scrn->raster = layer_enabled[VAEG_VA_LAYER_GRAPHICS0] && !grph0_noraster
					                   ? grph0_raster
					                   : NULL;
					scrn->pixelmode = videova.grres & 0x0003;
					break;
				case VIDEOVA_GRAPHICSCREEN1:
					scrn->raster = layer_enabled[VAEG_VA_LAYER_GRAPHICS1] && !grph1_noraster
					                   ? grph1_raster
					                   : NULL;
					scrn->pixelmode = (videova.grres >> 8) & 0x0003;
					break;
				}
			} else {
				// GDEN0 = 0 (グラフィック表示禁止)
				scrn->raster = NULL;
			}

			// screen mask
			maskpos = (videova.mskmode >> 4) & 3;
			if ((i + 4) == maskpos + 1) {
				// 直後の低優先画面
				scrn->mask[OUTSIDE] = videova.mskmode & 0x04;
				scrn->mask[INSIDE] = videova.mskmode & 0x01;
			} else /*if ((i + 4) > maskpos)*/ {
				// 低優先画面
				scrn->mask[OUTSIDE] = (videova.mskmode & 0x0c) == 0x0c;
				scrn->mask[INSIDE] = (videova.mskmode & 0x03) == 0x03;
			}
			/*
			else {
				// 高優先画面
				scrn->mask[OUTSIDE] = (videova.mskmode & 0x0c) == 0x08;
				scrn->mask[INSIDE] = (videova.mskmode & 0x03) == 0x02;
			}
			*/
		}

		if (scrn->raster == NULL) {
			// raster == NULLのときは、mask==TRUEにして、描画されないようにする
			scrn->mask[OUTSIDE] = TRUE;
			scrn->mask[INSIDE] = TRUE;
		}
	}

	bp = work.bp;
	for (x = 0; x < SURFACE_WIDTH; x++) {
		if ((videova.grmode & 0x3000) == 0x3000) {
			// XVSP = 1 (ビデオ信号出力モード)
			// SYNCEN = 1 (水平同期信号出力モード)

			if (work.y < videova.msktop * 2 || work.y > videova.mskbot * 2 + 1 ||
			    x < videova.mskleft || x > videova.mskrit) {
				side = OUTSIDE;
			} else {
				side = INSIDE;
			}
			// パレット指定画面
			scrn = &work.scrn[0];
			for (i = 0; i < VIDEOVA_PALETTE_SCREENS; i++, scrn++) {
				if (!scrn->mask[side] /*&& scrn->raster != NULL*/) {
					palcode = (scrn->raster[x] & scrn->pixelmask) ^ scrn->palflip;
					if ((scrn->xpar & (1 << palcode)) == 0) {
						// 不透明色
						c = videova.palette[palcode];
						goto opaque;
					}
				}
			}
			// 直接色指定画面
			for (i = 0; i < VIDEOVA_RGB_SCREENS; i++, scrn++) {
				if (!scrn->mask[side] /*&& scrn->raster != NULL*/) {
					c = scrn->raster[x];
					switch (scrn->pixelmode) {
					case 2:
						c = rgb8to16[c];
						break;
					case 3:
						break;
					default:
						// 1bit/pixel, 4bit/pixelでは常に透明
						c = 0;
						break;
					}
					if (c != 0) {
						// 不透明色
						goto opaque;
					}
				}
			}

			c = videova.dropcol;
		opaque:;
		} else {
			// XVSP = 0 (ビデオ信号禁止モード)
			// バックドロップカラーも含めて出力禁止
			c = 0;
		}
		*bp = c;
		bp++;
	}

	work.bp = bp;
	work.y++;
}
