/*
 * tsp.h: PC-88VA Text Sprite Processor
 *
 */

enum {
	TSP_F_LINESCHANGED = 1,
};

typedef struct {
	BOOL dspon; // TSPの表示ON
	BOOL spron; // スプライトの表示ON

	UINT16 texttable;  // テキスト画面制御テーブル(TVRAM先頭からのオフセット)
	UINT16 attroffset; // 文字コード領域とアトリビュート領域の差
	UINT8 lineheight;  // 1行のラスタ数
	UINT8 hlinepos;    // ホリゾンタルラインを表示するラスタ
	UINT8 blink;       // ブリンク文字とカーソルの点滅速度

	UINT8 curn; // カーソルとして使用するスプライトの番号
	BOOL be;    // カーソルブリンク

	UINT16 sprtable; // スプライト制御テーブル(TVRAM先頭からのオフセット)
	BOOL mg;         // 縦2倍拡大
	UINT8 hspn;      // 1ラスタ中に同時に表示できるスプライトの最大数
	BOOL gr;         // グルーピングモード

	UINT8 syncparam[14]; // SYNCコマンドパラメータ
	UINT16 screenlines;  // 画面ライン数(SYNCコマンドで指定されたもの)
	BOOL textmg;         // テキスト縦2倍拡大
	//	BOOL	hsync15khz;			// true.15KHz, false.24KHz(SYNCコマンドで指定されたもの)

	UINT16 flag; // TSP_F_***

	UINT8 blinkcnt;  // ブリンク用カウンタ。画面表示1サイクルごとにデクリメント
	UINT8 blinkcnt2; // ブリンク用カウンタ。blinkcnt==0になるたびにインクリメント

	UINT8 cmd;    // コマンド
	UINT8 status; // ステータス(ポート142h)
	//void	(*paramfunc)(REG8 dat);
	UINT16 paramfunc; // 受信パラメータ処理ルーチン

	UINT32 dispclock;   // 表示期間の時間(CPUクロック数)
	UINT32 vsyncclock;  // 垂直同期信号(VSYNC)の出力時間(CPUクロック数)
	UINT32 rasterclock; // 1ラスタ表示の時間(CPUクロック数)

	UINT32 sysp4vsyncextension; // TSP VSYNC終了後 システムポート4 VSYNCが1であり
	                            // 続ける期間(CPUクロック数)
	UINT32 sysp4dispclock;      // システムポート4 VSYNC の表示期間(CPUクロック数)

	UINT8 vsync;      // bit5: 1.VSYNC期間
	UINT8 sysp4vsync; // bit5: 1.システムポート4 VSYNC期間

	BYTE dmy1[64];

	// paramfunc_generic 用

	UINT8 recvdatacnt; // パラメータ残り受信バイト数
	//BYTE	senddatacnt;		// パラメータ残り送信バイト数
	//BYTE	*datap;				// 次に送信する/受信するデータ
	UINT16 paramindex; // 次に送受信するデータの位置(parambuf内)
	BYTE parambuf[16]; // 送受信バッファ
	//void	(*endparamfunc)(void);
	// パラメータ受信終了時ルーチン

	// paramfunc_generic 用
	UINT16 execfunc; // コマンド実行ルーチン

	// paramfunc_sprdef 用
	BYTE sprdef_offset;

	UINT8 ce; // CURDEF cursor enable; stored in the existing reserved state byte.

	/* uPD3301 emulation (EMUL 8Ch), held in former reserved bytes so the
	 * state layout is unchanged; zero (old states) means not emulating. */
	UINT8 emul;        // nonzero while emulating
	UINT8 emul_frame;  // split screen number
	UINT8 emul_chars;  // characters per row
	UINT8 emul_attrs;  // attribute pairs per row
	UINT8 emul_rows;   // rows
	UINT8 tvw_addr[3]; // TVRAM byte address for the 97h data stream (8Eh)
	UINT8 actscr;      // ACTSCR split screen number (0-3), from a reserved byte
	BYTE dmy2[118];
} _TSP, *TSP;

#ifdef __cplusplus
extern "C" {
#endif

void tsp_reset(void);
void tsp_bind(void);

void tsp_updateclock(void);

extern _TSP tsp;
extern BOOL tsp_dirty;

#ifdef __cplusplus
}
#endif
