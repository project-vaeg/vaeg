/*
 * cgromva.h: PC-88VA Character generator
 *
 */

typedef struct {
	WORD cgaddr; // 14Ch ハードウェア文字コード
	BYTE cgrow;  // 14Fh ラスタ番号/フォント左右
} _CGROMVA;

#ifdef __cplusplus
extern "C" {
#endif

extern _CGROMVA cgromva;

/* V1/V2 kanji font ports E8h/E9h (level 1) and ECh/EDh (level 2). */
typedef struct {
	UINT16 address[2]; /* word address per level */
} _CGROM88;

extern _CGROM88 cgrom88;

BYTE *cgromva_font(UINT16 hccode);
int cgromva_width(UINT16 hccode);
BYTE *cgromva_ank8(BYTE code);

void cgromva_reset(void);
void cgromva_bind(void);

#ifdef __cplusplus
}
#endif
