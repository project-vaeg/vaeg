/*
 * makegrphva.c: PC-88VA Graphics
 */

#include "compiler.h"
#include "scrndraw.h"
#include "gvramva.h"
#include "videova.h"
#include "makegrphva.h"

enum {
	GRPHVA_SCREENS = 2,
};
typedef struct {
	UINT16 y;      // 現在処理中のラスタ(グラフィック画面の座標系で)
	               //	BOOL	r200lines;				// 解像度200/204ラインならTRUE
	BOOL r320dots; // 解像度320ドットならTRUE
	int pixelmode; // 0..1bit, 1..4bit, 2..8bit, 3..16bit
	               // bit3 0..パレット 1..直接色指定
	WORD *rasterbuf;
	BOOL *noraster;
	UINT32 addrmask;
	UINT32 addrofs;

	UINT32 lineaddr;         // 次回表示GVRAMアドレス
	UINT32 wrappedaddr;      // 水平ラップアラウンド後のGVRAMアドレス
	UINT16 wrapcount;        // 水平ラップアラウンドするまでの残りバイト数
	UINT16 vwrapcount;       // 垂直ラップアラウンドするまでの残りライン数
	FRAMEBUFFER framebuffer; // 現在表示しているフレームバッファ
	int nextframebuffer;     // 次に使用するフレームバッファの番号。無い場合は-1
} _SCREEN, *SCREEN;

typedef struct {
	UINT16 screeny; // 現在処理中のラスタ(画面共通の座標系で)
	VIDEOVA_GRAPHICS_WINDOW window;
	UINT16 linesread;
	_SCREEN screen[GRPHVA_SCREENS];
} _GRPHVAWORK;

static _GRPHVAWORK work;

WORD grph0_raster[SURFACE_WIDTH + 64];
WORD grph1_raster[SURFACE_WIDTH + 64];
// 1ラスタ分のピクセルデータ
// 各ピクセルはカラーコード(16bit) または
// パレット番号
// 画面横幅 +
// 最大4バイト(ex.1bit/pixelなら32dot,
// 水平解像度320ならその倍)分
// 使用する
BOOL grph0_noraster;
BOOL grph1_noraster;
// 割り当てられた分割画面がない場合、
// グラフィック画面非表示の場合、
// true

/*
static	WORD byte2pixel[256][8];			// マルチプレーン
											// 1バイト→8ピクセル変換テーブル
*/
/*
static	WORD plane2pixel[4][4][4][4][2];	// マルチプレーン
											// 2bit*4plane→2ピクセル変換テーブル
*/
static BYTE byte2pixel[256][8]; // マルチプレーン
                                // 1バイト→8ピクセル変換テーブル

#define addr18(scrn, x) ((x) & ((scrn)->addrmask) | ((scrn)->addrofs))
#define issingleplane() (videova.grmode & 0x0400)

/*
 * 110h bit 7 (G3MSK) is the plane 3 screen switch of multiplane 4 bit/pixel
 * mode: clear means 3 bits/pixel with plane 3 not displayed (technical
 * manual ch. 4; BNN 8.3.2 requires it for V1/V2, and the VA2 ROM writes
 * 7F47h there).
 */
#define plane3mask() ((BYTE)((videova.pagemsk & 0x0080) ? 0xff : 0x00))

static void drawm4_pixels(SCREEN screen, UINT32 addr, UINT16 wrapcount, WORD *b, UINT count,
                          BOOL doublewidth) {
	UINT xp;
	UINT i;
	BYTE d0;
	BYTE d1;
	BYTE d2;
	BYTE d3;
	WORD pixel;

	for (xp = 0; xp < count; xp++) {
		if (wrapcount-- == 0) {
			addr = screen->wrappedaddr;
		}

		d0 = grphmem[addr + 0x00000];
		d1 = grphmem[addr + 0x10000];
		d2 = grphmem[addr + 0x20000];
		d3 = grphmem[addr + 0x30000] & plane3mask();
		addr = addr18(screen, addr + 1);

		for (i = 0; i < 8; i++) {
			pixel = (WORD)(byte2pixel[d0][i] | (byte2pixel[d1][i] << 1) | (byte2pixel[d2][i] << 2) |
			               (byte2pixel[d3][i] << 3));
			if (doublewidth) {
				b[0] = pixel;
				b[1] = pixel;
				b += 2;
			} else {
				*b++ = pixel;
			}
		}
	}
}

static void selectframe(SCREEN screen, int no) {
	FRAMEBUFFER f;

	if (no < 0 || no >= VIDEOVA_FRAMEBUFFERS) {
		screen->framebuffer = NULL;
		return;
	}

	f = &videova.framebuffer[no];
	screen->framebuffer = f;
	if (issingleplane()) {
		// シングルプレーンモード
		screen->lineaddr = addr18(screen, f->dsa);
		screen->wrappedaddr = addr18(screen, f->dsa - f->ofx);
	} else {
		// マルチプレーンモード
		screen->addrofs = (f->fsa & 0x20000L) ? 0x00020000L / 4 : 0;
		screen->lineaddr = addr18(screen, f->dsa / 4);
		screen->wrappedaddr = addr18(screen, f->dsa / 4 - f->ofx / 4);
	}
	if (f->fbl == 0xffff) {
		// screen 1 (垂直ラップアラウンドなし)
		screen->vwrapcount = 0;
	} else {
		screen->vwrapcount = f->fbl + 1 - f->ofy;
	}

	if (no == 0) {
		screen->nextframebuffer = 2;
	} else if (no == 2) {
		screen->nextframebuffer = 3;
	} else {
		screen->nextframebuffer = -1;
	}
}

/*
次のフレームバッファを選択してscreen->framebufferに設定する。
以下の場合はscreen->framebuffer == NULLとなる。
  まだDSP > screen->y の場合
  DSP < screen->y の場合 (後続フレームバッファ全てが無効となる)
  後続のフレームバッファがない場合
*/
static void selectnextframe(SCREEN screen) {
	screen->framebuffer = NULL;
	if (screen->nextframebuffer < 0)
		return;

	if (videova.framebuffer[screen->nextframebuffer].dsp < screen->y) {
		// これより後のフレームバッファは表示しない
		screen->nextframebuffer = -1;
	} else if (videova.framebuffer[screen->nextframebuffer].dsp == screen->y) {
		selectframe(screen, screen->nextframebuffer);
	}
}

static void endraster(SCREEN screen) {
	screen->vwrapcount--;
	if (screen->vwrapcount == 0) {
		// 垂直ラップアラウンド
		screen->wrappedaddr = addr18(screen, screen->framebuffer->fsa);
		screen->lineaddr = addr18(screen, screen->wrappedaddr + screen->framebuffer->ofx);
	} else {
		screen->lineaddr = addr18(screen, screen->lineaddr + screen->framebuffer->fbw);
		screen->wrappedaddr = addr18(screen, screen->wrappedaddr + screen->framebuffer->fbw);
	}
}

static void endraster_m(SCREEN screen) {
	screen->vwrapcount--;
	if (screen->vwrapcount == 0) {
		// 垂直ラップアラウンド
		screen->wrappedaddr = addr18(screen, screen->framebuffer->fsa / 4);
		screen->lineaddr = addr18(screen, screen->wrappedaddr + screen->framebuffer->ofx / 4);
	} else {
		screen->lineaddr = addr18(screen, screen->lineaddr + screen->framebuffer->fbw / 4);
		screen->wrappedaddr = addr18(screen, screen->wrappedaddr + screen->framebuffer->fbw / 4);
	}
}

// シングルプレーン1bit/pixel
static void drawraster_s1(SCREEN screen) {
	UINT16 xp;
	UINT16 wrapcount;
	UINT32 addr;
	WORD *b;
	DWORD dd;
	BYTE fg;
	UINT16 i;

	addr = screen->lineaddr;
	b = screen->rasterbuf;
	if (screen->framebuffer->ofx == 0xffff) {
		// screen 1 (ラップアラウンドなし)
		wrapcount = 0;
	} else {
		wrapcount = screen->framebuffer->fbw - screen->framebuffer->ofx;
	}

	// フォアグラウンドカラーのパレット番号
	fg = (videova.pagemsk & 0x0f00) >> 8;

	if (screen->r320dots) {
		// 320 dots
		dd = ((DWORD)grphmem[addr + 0] << 24) | ((DWORD)grphmem[addr + 1] << 16) |
		     ((DWORD)grphmem[addr + 2] << 8) | grphmem[addr + 3];
		addr = addr18(screen, addr + 4);

		i = screen->framebuffer->dot & 0x1f;
		dd <<= i;
		for (; i < 32; i++) {
			b[0] = b[1] = (dd & 0x80000000L) ? fg : 0;
			b += 2;
			dd <<= 1;
		}
		for (xp = 0; xp < 320 / 32; xp++) {
			wrapcount -= 4;
			if (wrapcount == 0) {
				addr = screen->wrappedaddr;
			}

			dd = ((DWORD)grphmem[addr + 0] << 24) | ((DWORD)grphmem[addr + 1] << 16) |
			     ((DWORD)grphmem[addr + 2] << 8) | grphmem[addr + 3];
			addr = addr18(screen, addr + 4);

			for (i = 0; i < 32; i++) {
				b[0] = b[1] = (dd & 0x80000000L) ? fg : 0;
				b += 2;
				dd <<= 1;
			}
		}

	} else {
		// 640 dots

		dd = ((DWORD)grphmem[addr + 0] << 24) | ((DWORD)grphmem[addr + 1] << 16) |
		     ((DWORD)grphmem[addr + 2] << 8) | grphmem[addr + 3];
		addr = addr18(screen, addr + 4);

		i = screen->framebuffer->dot & 0x1f;
		dd <<= i;
		for (; i < 32; i++) {
			*b++ = (dd & 0x80000000L) ? fg : 0;
			dd <<= 1;
		}
		for (xp = 0; xp < 640 / 32; xp++) {
			wrapcount -= 4;
			if (wrapcount == 0) {
				addr = screen->wrappedaddr;
			}

			dd = ((DWORD)grphmem[addr + 0] << 24) | ((DWORD)grphmem[addr + 1] << 16) |
			     ((DWORD)grphmem[addr + 2] << 8) | grphmem[addr + 3];
			addr = addr18(screen, addr + 4);

			for (i = 0; i < 32; i++) {
				*b++ = (dd & 0x80000000L) ? fg : 0;
				dd <<= 1;
			}
		}
	}

	endraster(screen);
}

// シングルプレーン4bit/pixel
static void drawraster_s4(SCREEN screen) {
	UINT16 xp;
	UINT16 wrapcount;
	UINT32 addr;
	WORD *b;

	WORD d, d2;

	addr = screen->lineaddr;
	b = screen->rasterbuf;
	if (screen->framebuffer->ofx == 0xffff) {
		// screen 1 (ラップアラウンドなし)
		wrapcount = 0;
	} else {
		wrapcount = screen->framebuffer->fbw - screen->framebuffer->ofx;
	}

	if (screen->r320dots) {
		// 320 dots
		d = LOADINTELWORD(grphmem + addr);
		d2 = LOADINTELWORD(grphmem + addr + 2);
		addr = addr18(screen, addr + 4);

		switch (screen->framebuffer->dot & 0x13) {
		case 0:
			b[0] = b[1] = (d >> 4) & 0x0f;
			b += 2;
		case 1:
			b[0] = b[1] = (d) & 0x0f;
			b += 2;
		case 2:
			b[0] = b[1] = (d >> 12) & 0x0f;
			b += 2;
		case 3:
			b[0] = b[1] = (d >> 8) & 0x0f;
			b += 2;
		case 0x10:
			b[0] = b[1] = (d2 >> 4) & 0x0f;
			b += 2;
		case 0x11:
			b[0] = b[1] = (d2) & 0x0f;
			b += 2;
		case 0x12:
			b[0] = b[1] = (d2 >> 12) & 0x0f;
			b += 2;
		case 0x13:
			b[0] = b[1] = (d2 >> 8) & 0x0f;
			b += 2;
		}
		for (xp = 0; xp < 320 / 8; xp++) {
			wrapcount -= 4;
			if (wrapcount == 0) {
				addr = screen->wrappedaddr;
			}

			d = LOADINTELWORD(grphmem + addr);
			d2 = LOADINTELWORD(grphmem + addr + 2);
			addr = addr18(screen, addr + 4);

			b[0] = b[1] = (d >> 4) & 0x0f;
			b += 2;
			b[0] = b[1] = (d) & 0x0f;
			b += 2;
			b[0] = b[1] = (d >> 12) & 0x0f;
			b += 2;
			b[0] = b[1] = (d >> 8) & 0x0f;
			b += 2;

			b[0] = b[1] = (d2 >> 4) & 0x0f;
			b += 2;
			b[0] = b[1] = (d2) & 0x0f;
			b += 2;
			b[0] = b[1] = (d2 >> 12) & 0x0f;
			b += 2;
			b[0] = b[1] = (d2 >> 8) & 0x0f;
			b += 2;
		}
	} else {
		// 640 dots

		d = LOADINTELWORD(grphmem + addr);
		d2 = LOADINTELWORD(grphmem + addr + 2);
		addr = addr18(screen, addr + 4);

		switch (screen->framebuffer->dot & 0x13) {
		case 0:
			*b++ = (d >> 4) & 0x0f;
		case 1:
			*b++ = (d) & 0x0f;
		case 2:
			*b++ = (d >> 12) & 0x0f;
		case 3:
			*b++ = (d >> 8) & 0x0f;
		case 0x10:
			*b++ = (d2 >> 4) & 0x0f;
		case 0x11:
			*b++ = (d2) & 0x0f;
		case 0x12:
			*b++ = (d2 >> 12) & 0x0f;
		case 0x13:
			*b++ = (d2 >> 8) & 0x0f;
		}
		for (xp = 0; xp < 640 / 8; xp++) {
			wrapcount -= 4;
			if (wrapcount == 0) {
				addr = screen->wrappedaddr;
			}

			d = LOADINTELWORD(grphmem + addr);
			d2 = LOADINTELWORD(grphmem + addr + 2);
			addr = addr18(screen, addr + 4);

			*b++ = (d >> 4) & 0x0f;
			*b++ = (d) & 0x0f;
			*b++ = (d >> 12) & 0x0f;
			*b++ = (d >> 8) & 0x0f;

			*b++ = (d2 >> 4) & 0x0f;
			*b++ = (d2) & 0x0f;
			*b++ = (d2 >> 12) & 0x0f;
			*b++ = (d2 >> 8) & 0x0f;
		}
	}

	endraster(screen);
}

// シングルプレーン8bit/pixel
static void drawraster_s8(SCREEN screen) {
	UINT16 xp;
	UINT16 wrapcount;
	UINT32 addr;
	WORD *b;

	WORD d, d2;

	addr = screen->lineaddr;
	b = screen->rasterbuf;
	if (screen->framebuffer->ofx == 0xffff) {
		// screen 1 (ラップアラウンドなし)
		wrapcount = 0;
	} else {
		wrapcount = screen->framebuffer->fbw - screen->framebuffer->ofx;
	}

	if (screen->r320dots) {
		// 320 dots
		d = LOADINTELWORD(grphmem + addr);
		d2 = LOADINTELWORD(grphmem + addr + 2);
		addr = addr18(screen, addr + 4);

		switch (screen->framebuffer->dot & 0x11) {
		case 0:
			b[0] = b[1] = (d) & 0xff;
			b += 2;
		case 1:
			b[0] = b[1] = (d >> 8) & 0xff;
			b += 2;
		case 0x10:
			b[0] = b[1] = (d2) & 0xff;
			b += 2;
		case 0x11:
			b[0] = b[1] = (d2 >> 8) & 0xff;
			b += 2;
		}
		for (xp = 0; xp < 320 / 4; xp++) {
			wrapcount -= 4;
			if (wrapcount == 0) {
				addr = screen->wrappedaddr;
			}

			d = LOADINTELWORD(grphmem + addr);
			d2 = LOADINTELWORD(grphmem + addr + 2);
			addr = addr18(screen, addr + 4);
			b[0] = b[1] = (d) & 0xff;
			b += 2;
			b[0] = b[1] = (d >> 8) & 0xff;
			b += 2;

			b[0] = b[1] = (d2) & 0xff;
			b += 2;
			b[0] = b[1] = (d2 >> 8) & 0xff;
			b += 2;
		}
	} else {
		// 640 dots

		d = LOADINTELWORD(grphmem + addr);
		d2 = LOADINTELWORD(grphmem + addr + 2);
		addr = addr18(screen, addr + 4);

		switch (screen->framebuffer->dot & 0x11) {
		case 0:
			*b++ = (d) & 0xff;
		case 1:
			*b++ = (d >> 8) & 0xff;
		case 0x10:
			*b++ = (d2) & 0xff;
		case 0x11:
			*b++ = (d2 >> 8) & 0xff;
		}
		for (xp = 0; xp < 640 / 4; xp++) {
			wrapcount -= 4;
			if (wrapcount == 0) {
				addr = screen->wrappedaddr;
			}

			d = LOADINTELWORD(grphmem + addr);
			d2 = LOADINTELWORD(grphmem + addr + 2);
			addr = addr18(screen, addr + 4);
			*b++ = (d) & 0xff;
			*b++ = (d >> 8) & 0xff;

			*b++ = (d2) & 0xff;
			*b++ = (d2 >> 8) & 0xff;
		}
	}
	endraster(screen);
}

// シングルプレーン16bit/pixel
static void drawraster_s16(SCREEN screen) {
	UINT16 xp;
	UINT16 wrapcount;
	UINT32 addr;
	WORD *b;

	WORD d, d2;

	addr = screen->lineaddr;
	b = screen->rasterbuf;
	if (screen->framebuffer->ofx == 0xffff) {
		// screen 1 (ラップアラウンドなし)
		wrapcount = 0;
	} else {
		wrapcount = screen->framebuffer->fbw - screen->framebuffer->ofx;
	}

	if (screen->r320dots) {
		// 320 dots
		d = LOADINTELWORD(grphmem + addr);
		d2 = LOADINTELWORD(grphmem + addr + 2);
		addr = addr18(screen, addr + 4);

		switch (screen->framebuffer->dot & 0x10) {
		case 0:
			b[0] = b[1] = d;
			b += 2;
		case 0x10:
			b[0] = b[1] = d2;
			b += 2;
		}

		for (xp = 0; xp < 320 / 2; xp++) {
			wrapcount -= 4;
			if (wrapcount == 0) {
				addr = screen->wrappedaddr;
			}

			d = LOADINTELWORD(grphmem + addr);
			d2 = LOADINTELWORD(grphmem + addr + 2);
			addr = addr18(screen, addr + 4);

			b[0] = b[1] = d;
			b += 2;
			b[0] = b[1] = d2;
			b += 2;
		}
	} else {
		// 640 dots

		d = LOADINTELWORD(grphmem + addr);
		d2 = LOADINTELWORD(grphmem + addr + 2);
		addr = addr18(screen, addr + 4);

		switch (screen->framebuffer->dot & 0x10) {
		case 0:
			*b++ = d;
		case 0x10:
			*b++ = d2;
		}
		for (xp = 0; xp < 640 / 2; xp++) {
			wrapcount -= 4;
			if (wrapcount == 0) {
				addr = screen->wrappedaddr;
			}

			d = LOADINTELWORD(grphmem + addr);
			d2 = LOADINTELWORD(grphmem + addr + 2);
			addr = addr18(screen, addr + 4);

			*b++ = d;
			*b++ = d2;
		}
	}
	endraster(screen);
}

/*
 * Multiplane 1 bit/pixel: each output pixel is the OR of the planes whose
 * screen switch (110h GnSW) is on. The compositor merges it into the text.
 *
 * With 110h 88MD set and 400 lines (V1/V2 640x400 monochrome), the VA2 ROM
 * switches on planes 0 and 1 and leaves frame buffer 0 at 200 lines of
 * 16 KiB per plane, so the PC-8801 layout must come from the display
 * circuit: plane 0 shows the upper 200 lines and plane 1 the lower 200.
 */
static BOOL n88_400lines(void) {
	return (videova.pagemsk & 0x40) && !(videova.grmode & 0x0400) && !(videova.grres & 0x0003) &&
	       !(videova.grmode & 0x0002);
}

static void drawraster_m1(SCREEN screen) {
	UINT sw = videova.pagemsk & 0x0f;
	UINT16 wrapcount;
	UINT32 addr;
	WORD *b;
	UINT xp;
	UINT i;

	addr = screen->lineaddr;
	if (n88_400lines()) {
		if (screen->y < 200) {
			sw &= 0x01;
		} else {
			sw &= 0x02;
			addr = addr18(screen, addr - 200 * (screen->framebuffer->fbw / 4));
		}
	}
	b = screen->rasterbuf;
	ZeroMemory(b, sizeof(grph0_raster));
	wrapcount = screen->framebuffer->fbw / 4 - screen->framebuffer->ofx / 4;
	for (xp = 0; xp < SURFACE_WIDTH / 8; xp++) {
		BYTE d = 0;

		if (wrapcount-- == 0) {
			addr = screen->wrappedaddr;
		}
		for (i = 0; i < 4; i++) {
			if (sw & (1 << i)) {
				d |= grphmem[addr + ((UINT32)i << 16)];
			}
		}
		addr = addr18(screen, addr + 1);
		for (i = 0; i < 8; i++) {
			*b++ = (d & (0x80 >> i)) ? 1 : 0;
		}
	}
	endraster_m(screen);
}

// マルチプレーン4bit/pixel
static void drawraster_m4(SCREEN screen) {
	//	UINT16		xp;
	UINT16 wrapcount;
	UINT32 addr;
	WORD *b;
	//	DWORD		dd;
	BYTE d0, d1, d2, d3;
	//	BYTE		fg;
	UINT16 i;

	addr = screen->lineaddr;
	b = screen->rasterbuf;
	ZeroMemory(b, sizeof(grph0_raster));
	if (screen->framebuffer->ofx == 0xffff) {
		// screen 1 (ラップアラウンドなし)
		wrapcount = 0;
	} else {
		wrapcount = screen->framebuffer->fbw / 4 - screen->framebuffer->ofx / 4;
	}

	// フォアグラウンドカラーのパレット番号
	//fg = (videova.pagemsk & 0x0f00) >> 8;

	if (screen->r320dots) {
		i = screen->framebuffer->dot & 0x07;
		if (i > 0) {
			d0 = grphmem[addr];
			d1 = grphmem[addr + 0x10000];
			d2 = grphmem[addr + 0x20000];
			d3 = grphmem[addr + 0x30000] & plane3mask();
			addr = addr18(screen, addr + 1);

			d0 <<= i;
			d1 <<= i;
			d2 <<= i;
			d3 <<= i;
			for (; i < 8; i++) {
				b[0] = b[1] = ((d0 & 0x80) >> 7) | ((d1 & 0x80) >> 6) | ((d2 & 0x80) >> 5) |
				              ((d3 & 0x80) >> 4);
				d0 <<= 1;
				d1 <<= 1;
				d2 <<= 1;
				d3 <<= 1;
				b += 2;
			}
			wrapcount--;
		}

		drawm4_pixels(screen, addr, wrapcount, b, 320 / 8, TRUE);

		/*
		// 320 dots
		dd = ((DWORD)grphmem[addr+0] << 24) | 
			 ((DWORD)grphmem[addr+1] << 16) | 
			 ((DWORD)grphmem[addr+2] << 8) | 
			 grphmem[addr+3];
		addr = addr18(screen, addr + 4);

		i = screen->framebuffer->dot & 0x1f;
		dd <<= i;
		for (; i < 32; i++) {
			b[0] = b[1] = (dd & 0x80000000L) ? fg : 0;
			b += 2;
			dd <<= 1;
		}
		for (xp = 0; xp < 320/32; xp++) {
			wrapcount -= 4;
			if (wrapcount == 0) {
				addr = screen->wrappedaddr;
			}

			dd = ((DWORD)grphmem[addr+0] << 24) | 
				 ((DWORD)grphmem[addr+1] << 16) | 
				 ((DWORD)grphmem[addr+2] << 8) | 
				 grphmem[addr+3];
			addr = addr18(screen, addr + 4);

			for (i = 0; i < 32; i++) {
				b[0] = b[1] = (dd & 0x80000000L) ? fg : 0;
				b += 2;
				dd <<= 1;
			}
		}
*/

	} else {
		// 640 dots
		i = screen->framebuffer->dot & 0x07;
		if (i > 0) {
			d0 = grphmem[addr];
			d1 = grphmem[addr + 0x10000];
			d2 = grphmem[addr + 0x20000];
			d3 = grphmem[addr + 0x30000] & plane3mask();
			addr = addr18(screen, addr + 1);

			d0 <<= i;
			d1 <<= i;
			d2 <<= i;
			d3 <<= i;
			for (; i < 8; i++) {
				*b++ = ((d0 & 0x80) >> 7) | ((d1 & 0x80) >> 6) | ((d2 & 0x80) >> 5) |
				       ((d3 & 0x80) >> 4);
				d0 <<= 1;
				d1 <<= 1;
				d2 <<= 1;
				d3 <<= 1;
			}
			wrapcount--;
		}
#if 1
		drawm4_pixels(screen, addr, wrapcount, b, 640 / 8, FALSE);
#else
		for (xp = 0; xp < 640 / 8; xp++) {
			//wrapcount -= 1;
			if (wrapcount-- == 0) {
				addr = screen->wrappedaddr;
			}

			/*
			d0 = grphmem[addr];
			d1 = grphmem[addr + 0x10000];
			d2 = grphmem[addr + 0x20000];
			d3 = grphmem[addr + 0x30000];
			addr = addr18(screen, addr + 1);
			*/

			__asm {
				mov		edi, addr
				movzx	ebx, grphmem[edi + 0x30000]
				mov		eax, dword ptr byte2pixel[ebx*8]
				mov		edx, dword ptr byte2pixel[ebx*8+4]
				shl		eax, 1
				shl		edx, 1
				movzx	ebx, grphmem[edi + 0x20000]
				or		eax, dword ptr byte2pixel[ebx*8]
				or		edx, dword ptr byte2pixel[ebx*8+4]
				shl		eax, 1
				shl		edx, 1
				movzx	ebx, grphmem[edi + 0x10000]
				or		eax, dword ptr byte2pixel[ebx*8]
				or		edx, dword ptr byte2pixel[ebx*8+4]
				shl		eax, 1
				shl		edx, 1
				movzx	ebx, grphmem[edi + 0x00000]
				or		eax, dword ptr byte2pixel[ebx*8]
				or		edx, dword ptr byte2pixel[ebx*8+4]
				mov		ebx, b
				mov		[ebx+0], al
				;mov		[ebx+1], byte ptr 0
				mov		[ebx+2], ah
				;mov		[ebx+3], byte ptr 0
				shr		eax, 16
				mov		[ebx+4], al
				;mov		[ebx+5], byte ptr 0
				mov		[ebx+6], ah
				;mov		[ebx+7], byte ptr 0
				mov		[ebx+8], dl
				;mov		[ebx+9], byte ptr 0
				mov		[ebx+10], dh
				;mov		[ebx+11], byte ptr 0
				shr		edx, 16
				mov		[ebx+12], dl
				;mov		[ebx+13], byte ptr 0
				mov		[ebx+14], dh
				;mov		[ebx+15], byte ptr 0
			}

			/*
			__asm {
				mov		edi, addr
				movzx	ebx, grphmem[edi + 0x30000]
				shl		ebx, 3
				mov		eax, dword ptr byte2pixel[ebx]
				mov		edx, dword ptr byte2pixel[ebx+4]
				shl		eax, 1
				shl		edx, 1
				movzx	ebx, grphmem[edi + 0x20000]
				shl		ebx, 3
				or		eax, dword ptr byte2pixel[ebx]
				or		edx, dword ptr byte2pixel[ebx+4]
				shl		eax, 1
				shl		edx, 1
				movzx	ebx, grphmem[edi + 0x10000]
				shl		ebx, 3
				or		eax, dword ptr byte2pixel[ebx]
				or		edx, dword ptr byte2pixel[ebx+4]
				shl		eax, 1
				shl		edx, 1
				movzx	ebx, grphmem[edi + 0x00000]
				shl		ebx, 3
				or		eax, dword ptr byte2pixel[ebx]
				or		edx, dword ptr byte2pixel[ebx+4]
				mov		ebx, b
				mov		[ebx+0], al
				mov		[ebx+2], ah
				shr		eax, 16
				mov		[ebx+4], al
				mov		[ebx+6], ah
				mov		[ebx+8], dl
				mov		[ebx+10], dh
				shr		edx, 16
				mov		[ebx+12], dl
				mov		[ebx+14], dh
			}
			*/
			addr = addr18(screen, addr + 1);
			b += 8;

			/*
			for (i = 0; i < 8; i++) {
				*b++ = ((d0 & 0x80) >> 7) | 
					   ((d1 & 0x80) >> 6) | 
					   ((d2 & 0x80) >> 5) | 
					   ((d3 & 0x80) >> 4);
				d0 <<= 1;
				d1 <<= 1;
				d2 <<= 1;
				d3 <<= 1;
			}
*/
			/*
			{
				DWORD p;

				p = *((DWORD *)&byte2pixel[d0][0]);
				p |= *((DWORD *)&byte2pixel[d1][0]) << 1;
				p |= *((DWORD *)&byte2pixel[d2][0]) << 2;
				p |= *((DWORD *)&byte2pixel[d3][0]) << 3;
				*((DWORD *)b) = p;
				b += 2;

				p = *((DWORD *)&byte2pixel[d0][2]);
				p |= *((DWORD *)&byte2pixel[d1][2]) << 1;
				p |= *((DWORD *)&byte2pixel[d2][2]) << 2;
				p |= *((DWORD *)&byte2pixel[d3][2]) << 3;
				*((DWORD *)b) = p;
				b += 2;

				p = *((DWORD *)&byte2pixel[d0][4]);
				p |= *((DWORD *)&byte2pixel[d1][4]) << 1;
				p |= *((DWORD *)&byte2pixel[d2][4]) << 2;
				p |= *((DWORD *)&byte2pixel[d3][4]) << 3;
				*((DWORD *)b) = p;
				b += 2;

				p = *((DWORD *)&byte2pixel[d0][6]);
				p |= *((DWORD *)&byte2pixel[d1][6]) << 1;
				p |= *((DWORD *)&byte2pixel[d2][6]) << 2;
				p |= *((DWORD *)&byte2pixel[d3][6]) << 3;
				*((DWORD *)b) = p;
				b += 2;
			}
*/
			/*
			{
				for (i = 6; i < 8 ; i-=2) {
					*((DWORD *)&b[i]) = 
						*((DWORD *)&plane2pixel[d0&3][d1&3][d2&3][d3&3][0]);
					d0 >>= 2;
					d1 >>= 2;
					d2 >>= 2;
					d3 >>= 2;
				}
				b += 8;
			}
*/
		}
#endif
	}

	endraster_m(screen);
}

static void drawraster(SCREEN screen) {
	//	if (!screen->r200lines || (work.screeny & 1) == 0) {
	if (screen->framebuffer != NULL) {
		// The 88MD 400-line layout shows the 200-line frame from two planes.
		const UINT dsh = screen->framebuffer->dsh * (n88_400lines() ? 2 : 1);

		if (screen->framebuffer->dsp + dsh == screen->y) {
			screen->framebuffer = NULL;
		}
	}

	// 必要なら、次のフレームバッファに切り替える
	if (screen->framebuffer == NULL) {
		do {
			selectnextframe(screen);
		} while (!(screen->framebuffer == NULL || screen->framebuffer->dsh > 0));
	}

	//if (screen->rasterbuf) {
	*screen->noraster = FALSE;
	if (screen->framebuffer == NULL) {
		// 何も表示しない
		/*
				UINT16		xp;
				WORD		*b;
				b = screen->rasterbuf;
				for (xp = 0; xp < 640; xp++) *b++ = 0;
				*/
		*screen->noraster = TRUE;
	} else if (issingleplane()) {
		// シングルプレーンモード
		switch (screen->pixelmode) {
		case 0:
			drawraster_s1(screen);
			break;
		case 1:
			drawraster_s4(screen);
			break;
		case 2:
			drawraster_s8(screen);
			break;
		case 3:
			drawraster_s16(screen);
			break;
		}
	} else {
		// マルチプレーンモード
		switch (screen->pixelmode) {
		case 0:
			drawraster_m1(screen);
			break;
		case 1:
			drawraster_m4(screen);
			break;
		default: {
			// 何も表示しない (透明ではなく、0を出力)
			UINT16 xp;
			WORD *b;
			b = screen->rasterbuf;
			for (xp = 0; xp < 640; xp++)
				*b++ = 0;
		} break;
		}
	}
	//}
	screen->y++;

	//	}
}

void makegrphva_initialize(void) {
	/*
	// マルチプレーン 1バイト→8ピクセル 変換テーブル
	{
		int d;
		int i;

		for (d = 0; d < 256; d++) {
			int dat = d;
			for (i = 7; i >= 0; i--) {
				byte2pixel[d][i] = dat & 1;
				dat >>= 1;
			}
		}
	}
*/
	/*
	// マルチプレーン 2bit*4plane→2ピクセル変換テーブル
	{
		int p0,p1,p2,p3;
		for (p0 = 0; p0 < 4; p0++) {
			for (p1 = 0; p1 < 4; p1++) {
				for (p2 = 0; p2 < 4; p2++) {
					for (p3 = 0; p3 < 4; p3++) {
						plane2pixel[p0][p1][p2][p3][1] = 
							(p0 & 1) | 
							((p1 & 1) << 1) |
							((p2 & 1) << 2) |
							((p3 & 1) << 3);
						plane2pixel[p0][p1][p2][p3][0] = 
							((p0 & 2) >> 1) | 
							((p1 & 2)     ) |
							((p2 & 2) << 1) |
							((p3 & 2) << 2);
					}
				}
			}
		}
	}
*/
	// マルチプレーン 1バイト→8ピクセル 変換テーブル
	{
		int d;
		int i;

		for (d = 0; d < 256; d++) {
			int dat = d;
			for (i = 7; i >= 0; i--) {
				byte2pixel[d][i] = dat & 1;
				dat >>= 1;
			}
		}
	}
}

void makegrphva_begin(BOOL *scrn200) {
	work.screeny = 0;
	ZeroMemory(&work.window, sizeof(work.window));
	work.window.linelimit = 0xffff;
	work.linesread = 0;

	work.screen[0].pixelmode = videova.grres & 0x0003;
	work.screen[1].pixelmode = (videova.grres >> 8) & 0x0003;
	work.screen[0].rasterbuf = grph0_raster;
	work.screen[1].rasterbuf = grph1_raster;
	work.screen[0].noraster = &grph0_noraster;
	work.screen[1].noraster = &grph1_noraster;

	work.screen[0].r320dots = videova.grres & 0x0010;
	work.screen[1].r320dots = videova.grres & 0x1000;
	//	work.screen[0].r200lines = videova.grmode & 0x0002;
	//	work.screen[1].r200lines = videova.grmode & 0x0002;
	if (issingleplane()) {
		if (videova.grmode & 0x0800) {
			// 2画面モード
			work.screen[0].addrmask = 0x0001ffffL;
		} else {
			// 1画面モード
			work.screen[0].addrmask = 0x0003ffffL;
		}
		work.screen[0].addrofs = 0x00000000L;
	} else {
		work.screen[0].addrmask = 0x0001ffffL / 4;
	}
	work.screen[1].addrmask = 0x0001ffffL;
	work.screen[1].addrofs = 0x00020000L;
	work.screen[0].nextframebuffer = 0;
	work.screen[1].nextframebuffer = 1;
	work.screen[0].y = 0;
	work.screen[1].y = 0;
	selectframe(&work.screen[0], -1);
	selectframe(&work.screen[1], -1);

	*scrn200 = videova.grmode & 0x0002;
}

void makegrphva_blankraster(void) {
	int i;
	//	UINT16 xp;

	for (i = 0; i < GRPHVA_SCREENS; i++) {
		/*
		for (xp = 0; xp < SURFACE_WIDTH; xp++) {
			work.screen[i].rasterbuf[xp] = 0;
		}
		*/
		*work.screen[i].noraster = TRUE;
	}
}

/* Call after makegrphva_begin; see videova_graphics_window. */
void makegrphva_setwindow(const struct videova_graphics_window *window) {
	work.window = *window;
}

static void readline(void) {
	if (!(videova.grmode & 0x8000)) {
		// グラフィック表示禁止
		makegrphva_blankraster();
	} else if (videova.grmode & 0x0400) {
		// シングルプレーンモード
		drawraster(&work.screen[0]);
		drawraster(&work.screen[1]);
	} else {
		// マルチプレーンモード
		drawraster(&work.screen[0]);
	}
}

void makegrphva_raster(void) {
	if (work.window.startdelay > 0) {
		work.window.startdelay--;
		makegrphva_blankraster();
		work.screeny++;
		return;
	}
	while (work.window.startskip > 0) {
		// Read before the visible area: the lines are not shown.
		work.window.startskip--;
		readline();
		work.linesread++;
	}
	if (work.linesread >= work.window.linelimit) {
		// The line address no longer advances.
		if (work.window.blankafterlimit) {
			makegrphva_blankraster();
		}
		work.screeny++;
		return;
	}
	work.linesread++;
	readline();
	work.screeny++;
}
