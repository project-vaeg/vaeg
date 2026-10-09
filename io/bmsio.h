/*
 * bmsio.h: I-O Bank Memory
 */

enum {
	BMSIO_DEFAULT_ENABLED = TRUE,
	BMSIO_PORT_DEFAULT = 0x01d0,
	BMSIO_PORT_COMPAT = 0x00ec,
	BMSIO_PORT_MASK = 0xffff,
	BMSIO_BANK_BYTES = 0x20000,
	BMSIO_DEFAULT_BANKS = 0x80,
	BMSIO_MAX_BANKS = 0xff
};

// 構成設定
typedef struct {
	BOOL enabled;    // IOバンクメモリを使用する
	UINT16 port;     // ポート番号
	UINT16 portmask; // (予約)
	UINT8 numbanks;  // バンク数
} _BMSIOCFG;

// 動作時の構成と状態 (STATSAVEの対象)
typedef struct { // memory.x86内の構造体に影響
	             // 状態
	UINT8 nomem; // 現在選択されているバンクにメモリがある
	UINT8 bank;  // 現在選択されているバンク

	_BMSIOCFG cfg; // 構成
} _BMSIO, *BMSIO;

// ワーク
typedef struct { // memory.x86内の構造体に影響
	BYTE *bmsmem;
	UINT32 bmsmemsize;
} _BMSIOWORK;

#ifdef __cplusplus
extern "C" {
#endif

extern _BMSIOCFG bmsiocfg;
extern _BMSIO bmsio;
extern _BMSIOWORK bmsiowork;

void bmsio_set(void);
void bmsio_reset(void);
void bmsio_bind(void);

#ifdef __cplusplus
}
#endif
