/*
 * gactrlva.h: PC-88VA GVRAM access control
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

// Single Plane
typedef struct {
	UINT8 writemode;   // 0580h ライトモードレジスタ
	UINT16 pattern[2]; // 0590h, 0592h パターンレジスタ
	UINT8 rop[2];      // 05a0h, 05a2h ROPコードレジスタ
} _GACTRLVA_SINGLE, *GACTRLVA_SINGLE;

// Multi Plane
typedef struct {
	UINT8 accessmode;         // 0510h アクセスモード bit0 1..拡張アクセス 0..独立アクセス
	UINT8 accessblock;        // 0512h アクセスブロック bit0 0..ブロック0 1..ブロック1
	UINT8 readplane;          // 0514h 読み出しプレーン bit0-3 0..読み出す 1..読み出さない
	UINT8 writeplane;         // 0516h 書き込みプレーン bit0-3 0..書き込む 1..書き込まない
	UINT8 advancedaccessmode; // 0518h 拡張アクセスモード
	                          // bit 1-0
	                          //　　０　　　０　　　固定モード（リード／ライト時更新しない）
	                          //　　０　　　１　　　リード時のみ更新
	                          //　　１　　　０　　　ライト時のみ更新
	                          //　　１　　　１　　　リード／ライト時更新
	                          // bit 2 0..8bit 1..16bit
	                          // bit 4-3
	                          //     00 LU出力
	                          //     01 パターンレジスタ
	                          //     10 CPU
	                          //     11 ノーオペレーション
	                          // bit 5 1..比較読み出しをする
	                          // bit 7 1..マルチプレーンモード制御レジスタ リード禁止
	UINT8 cmpdata[4];         // 0520h, 0522h, 0524h, 0526h 比較データレジスタ
	UINT8 dmy; //cmpdatacontrol;	// 0528h 比較データレジスタに設定 bit3-0 1..0ffh設定 0..0設定
	UINT8 pattern[4][2];       // [][0] 0530h, 0532h, 0534h, 0536h パターンレジスタ下位
	                           // [][1] 0540h, 0542h, 0544h, 0546h パターンレジスタ上位
	UINT8 patternreadpointer;  // 0550h パターンレジスタリードポインタ bit3-0 1..上位バイトから使用
	UINT8 patternwritepointer; // 0552h パターンレジスタライトポインタ bit3-0 1..上位バイトから使用
	UINT8 rop[4];              // 0560h,0562h,0564h,0566h ROPレジスタ
} _GACTRLVA_MULTI, *GACTRLVA_MULTI;

typedef struct {
	UINT8 gmsp;         // 描画モード 0x10..シングルプレーン 0x00..マルチプレーン
	_GACTRLVA_SINGLE s; // シングルプレーン
	_GACTRLVA_MULTI m;  // マルチプレーン
} _GACTRLVA, *GACTRLVA;

extern _GACTRLVA gactrlva;

void gactrlva_reset(void);
void gactrlva_bind(void);

#ifdef __cplusplus
}
#endif
