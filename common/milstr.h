
#ifdef __cplusplus
extern "C" {
#endif

// １文字分のサイズを取得
int milsjis_charsize(const char *str);
int milutf8_charsize(const char *str);

// 大文字小文字を同一視して比較
// ret 0:一致
int milsjis_cmp(const char *str, const char *cmp);
int milutf8_cmp(const char *str, const char *cmp);

// 大文字小文字を 同一視してcmpのヌルまで比較
// ret 0:一致
int milsjis_memcmp(const char *str, const char *cmp);
int milutf8_memcmp(const char *str, const char *cmp);

// str[pos]が漢字１バイト目かどうか…
int milsjis_kanji1st(const char *str, int pos);
int milutf8_kanji1st(const char *str, int pos);

// str[pos]が漢字２バイト目かどうか…
int milsjis_kanji2nd(const char *str, int pos);
int milutf8_kanji2nd(const char *str, int pos);

// maxlen分だけ文字列をコピー
void milsjis_ncpy(char *dst, const char *src, int maxlen);
void milutf8_ncpy(char *dst, const char *src, int maxlen);

// maxlen分だけ文字列をキャット
void milsjis_ncat(char *dst, const char *src, int maxlen);
void milutf8_ncat(char *dst, const char *src, int maxlen);

// 文字を検索
char *milsjis_chr(const char *str, int c);
char *milutf8_chr(const char *str, int c);

// 0~9, A~Z のみを大文字小文字を同一視して比較
// ret 0:一致
int milstr_extendcmp(const OEMCHAR *str, const OEMCHAR *cmp);

// 次の語を取得
char *milstr_nextword(const OEMCHAR *str);

// 文字列からARGの取得
int milstr_getarg(OEMCHAR *str, OEMCHAR *arg[], int maxarg);

// HEX2INT
long milstr_solveHEX(const OEMCHAR *str);

// STR2INT
long milstr_solveINT(const OEMCHAR *str);

// STRLIST
OEMCHAR *milstr_list(const OEMCHAR *lststr, UINT pos);

#ifdef __cplusplus
}
#endif

// ---- macros

#define milstr_charsize(s) milutf8_charsize(s)
#define milstr_cmp(s, c) milutf8_cmp(s, c)
#define milstr_memcmp(s, c) milutf8_memcmp(s, c)
#define milstr_kanji1st(s, p) milutf8_kanji1st(s, p)
#define milstr_kanji2nd(s, p) milutf8_kanji2nd(s, p)
#define milstr_ncpy(d, s, l) milutf8_ncpy(d, s, l)
#define milstr_ncat(d, s, l) milutf8_ncat(d, s, l)
#define milstr_chr(s, c) milutf8_chr(s, c)
