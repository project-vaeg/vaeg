/*
 *	GVAMVA.C: PC-88VA GVRAM
 */

#include "compiler.h"
#include "cpucore.h"
#include "gactrlva.h"

enum {
	MEMWAITVA_VRAM_SINGLE_R = 4,
	MEMWAITVA_VRAM_MULTI_R = 7,
	MEMWAITVA_VRAM_SINGLE_W = 1, // 1.5くらいにしたい・・・
	MEMWAITVA_VRAM_MULTI_W = 4,
};

BYTE grphmem[0x40000];
_GACTRLVA gactrlva;

static UINT16 lu(UINT8 rop, UINT16 pat, UINT16 cpu, UINT16 mem) {
	UINT16 out;

	out = 0;
	if (rop & 0x01) {
		out |= ~pat & ~cpu & ~mem;
	}
	if (rop & 0x02) {
		out |= ~pat & ~cpu & mem;
	}
	if (rop & 0x04) {
		out |= ~pat & cpu & ~mem;
	}
	if (rop & 0x08) {
		out |= ~pat & cpu & mem;
	}
	if (rop & 0x10) {
		out |= pat & ~cpu & ~mem;
	}
	if (rop & 0x20) {
		out |= pat & ~cpu & mem;
	}
	if (rop & 0x40) {
		out |= pat & cpu & ~mem;
	}
	if (rop & 0x80) {
		out |= pat & cpu & mem;
	}
	return out;
}

static int addr2page(UINT32 address) {
	return (address >> 17) & 1;
}

static void memwait_r(void) {
	int wait;
	if (gactrlva.gmsp) {
		// シングル
		wait = MEMWAITVA_VRAM_SINGLE_R;
	} else {
		// マルチ
		wait = MEMWAITVA_VRAM_MULTI_R;
	}
	CPU_REMCLOCK -= wait;
}

static void memwait_w(void) {
	int wait;
	if (gactrlva.gmsp) {
		// シングル
		wait = MEMWAITVA_VRAM_SINGLE_W;
	} else {
		// マルチ
		wait = MEMWAITVA_VRAM_MULTI_W;
	}
	CPU_REMCLOCK -= wait;
}

/*
GVRAMへwriteするデータを求める(シングルプレーン)
	入力:
		address		アドレス(偶数)
		value		データ
*/
static UINT16 writevalue_s(UINT32 address, REG16 value) {
	UINT16 mem;
	UINT16 pat;
	int page;

	mem = *(UINT16 *)(grphmem + address);
	//	if (gactrlva.gmsp) {
	// シングルプレーン
	page = addr2page(address);
	pat = gactrlva.s.pattern[page];
	switch ((gactrlva.s.writemode >> 3) & 3) {
	case 0: // LU出力
		return lu(gactrlva.s.rop[page], pat, value, mem);
	case 1: // パターンレジスタ
		return pat;
	case 2: // CPUライトデータ
		return value;
	case 3: // ノーオペレーション
	default:
		return mem;
	}
	/*	}
	else {
		// マルチプレーン
		page = address >> 16;
		pat = gactrlva.m.pattern[page];
		switch ((gactrlva.m.advancedaccessmode >> 3) & 3) {
		case 0:	// LU出力
			return lu(gactrlva.m.rop[page], pat, value, mem);
		case 1:	// パターンレジスタ
			return pat;
		case 2:	// CPUライトデータ
			return value;
		case 3:	// ノーオペレーション
		default:
			return mem;

		}
	}
*/
}

/*
GVRAMへwriteするデータを求める(マルチプレーン)
	入力:
		address		アドレス
		value		データ
*/
static UINT8 writevalue_m(UINT32 address, REG8 value) {
	UINT8 mem;
	UINT8 pat;
	int plane;
	int patternindex;

	mem = grphmem[address];
	plane = address >> 16;
	patternindex = (gactrlva.m.patternreadpointer >> plane) & 1;
	pat = gactrlva.m.pattern[plane][patternindex];
	switch ((gactrlva.m.advancedaccessmode >> 3) & 3) {
	case 0: // LU出力
		return (UINT8)lu(gactrlva.m.rop[plane], pat, value, mem);
	case 1: // パターンレジスタ
		return pat;
	case 2: // CPUライトデータ
		return value;
	case 3: // ノーオペレーション
	default:
		return mem;
	}
}

// ---- write byte

static void _gvram_wt(UINT32 address, REG8 value) {
	if (gactrlva.gmsp) {
		// シングルプレーン
		UINT16 out;

		if (address & 1) {
			out = writevalue_s(address & 0xfffffffe, (REG16)(((REG16)value) << 8));
			grphmem[address] = out >> 8;
		} else {
			out = writevalue_s(address, (REG16)value);
			grphmem[address] = (BYTE)out;
		}
	} else {
		// マルチプレーン
		if (gactrlva.m.accessmode) {
			// 拡張アクセス
			int i;
			UINT8 mask;
			UINT8 out;
			UINT8 mem;

			address &= 0x7fff;
			if (gactrlva.m.accessblock)
				address |= 0x8000;
			mask = gactrlva.m.writeplane;
			for (i = 0; i < 4; i++) {
				if (!(mask & 1)) {
					out = writevalue_m(address, value);
					mem = grphmem[address];
					grphmem[address] = out;

					if (gactrlva.m.advancedaccessmode & 0x02) {
						// パターンレジスタ更新
						int patternindex;
						patternindex = (gactrlva.m.patternwritepointer >> i) & 1;
						gactrlva.m.pattern[i][patternindex] = mem;
					}
				}
				mask >>= 1;
				address += 0x10000;
			}

			if (gactrlva.m.advancedaccessmode & 0x04) {
				// パターンレジスタ16bit
				gactrlva.m.patternreadpointer ^= 0x0f;
				if (gactrlva.m.advancedaccessmode & 0x02) {
					// ライト時にパターンレジスタ更新
					gactrlva.m.patternwritepointer ^= 0x0f;
				}
			}

		} else {
			// 独立アクセス
			grphmem[address] = value;
		}
	}
}

void MEMCALL gvram_wt(UINT32 address, REG8 value) {
	memwait_w();
	_gvram_wt(address, value);
}

// ---- read byte

static REG8 MEMCALL _gvram_rd(UINT32 address) {
	if (gactrlva.gmsp || !gactrlva.m.accessmode) {
		// シングルプレーンモード、または、
		// マルチプレーンモード　独立アクセスモード
		return (grphmem[address]);
	} else {
		// マルチプレーンモード  拡張アクセスモード
		int i;
		UINT8 mask;
		UINT8 dat;
		UINT8 ret;

		address &= 0x7fff;
		if (gactrlva.m.accessblock)
			address |= 0x8000;
		mask = gactrlva.m.readplane;
		ret = 0xff;
		for (i = 0; i < 4; i++) {
			if (!(mask & 1)) {
				dat = grphmem[address];

				if (gactrlva.m.advancedaccessmode & 0x20) {
					// 比較読み出し
					ret &= ~(dat ^ gactrlva.m.cmpdata[i]);
				} else {
					// 通常読み出し
					ret &= dat;
				}

				if (gactrlva.m.advancedaccessmode & 0x01) {
					// パターンレジスタ更新
					int patternindex;
					patternindex = (gactrlva.m.patternwritepointer >> i) & 1;
					gactrlva.m.pattern[i][patternindex] = dat;
				}
			}
			mask >>= 1;
			address += 0x10000;
		}
		if (gactrlva.m.advancedaccessmode & 0x04) {
			// パターンレジスタ16bit
			if (gactrlva.m.advancedaccessmode & 0x01) {
				// リード時にパターンレジスタ更新
				gactrlva.m.patternwritepointer ^= 0x0f;
			}
		}
		return ret;
	}
}

REG8 MEMCALL gvram_rd(UINT32 address) {
	memwait_r();
	return _gvram_rd(address);
}

// ---- write word

/*
	入力:
		address		アドレス(偶数)
*/
/*
void MEMCALL _gvramw_wt(UINT32 address, REG16 value) {
	*(UINT16 *)(grphmem + address) = value;
}
*/

void MEMCALL gvramw_wt(UINT32 address, REG16 value) {
	UINT16 out;

	if (address & 1) {
		gvram_wt(address, (REG8)(value & 0x00ff));
		gvram_wt(address + 1, (REG8)(value >> 8));
	} else {
		memwait_w();
		if (gactrlva.gmsp) {
			// シングルプレーン
			out = writevalue_s(address, value);
			*(UINT16 *)(grphmem + address) = out;
		} else {
			_gvram_wt(address, (REG8)(value & 0x00ff));
			_gvram_wt(address + 1, (REG8)(value >> 8));
		}
	}
}

// ---- read word

REG16 MEMCALL gvramw_rd(UINT32 address) {
	REG8 l, h;

	memwait_r();
	l = _gvram_rd(address);
	h = _gvram_rd(address + 1);
	return l | (((REG16)h) << 8);
}
