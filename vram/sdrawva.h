
typedef struct {
	BYTE *dst;
	int width;  // 画面幅(pixel)
	int xbytes; // 画面幅(byte)
	int y;
	int xalign; // 1ピクセルのバイト数
	int yalign; // 1ラスタのバイト数
	BYTE dirty[SURFACE_WIDTH];
} _SDRAWVA, *SDRAWVA;

typedef void(SCRNCALL *SDRAWFNVA)(SDRAWVA sdraw, int maxy);

#ifdef __cplusplus
extern "C" {
#endif

const SDRAWFNVA sdrawva_getproctbl(const SCRNSURF *surf);

#ifdef __cplusplus
}
#endif
