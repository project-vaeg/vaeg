#ifdef __cplusplus
extern "C" {
#endif

extern WORD vabitmap[];
//extern	BYTE	colorlevel5[];
//extern	BYTE	colorlevel6[];
extern RGB16 drawcolor16[];

enum {
	VAEG_VA_LAYER_TEXT = 0,
	VAEG_VA_LAYER_SPRITE,
	VAEG_VA_LAYER_GRAPHICS0,
	VAEG_VA_LAYER_GRAPHICS1,
	VAEG_VA_LAYER_COUNT
};

BYTE scrndrawva_draw(BYTE redraw);
void scrndrawva_redraw(void);
BYTE scrndrawva_outputlevel(UINT value, UINT bits, BOOL digital);

void scrndrawva_set_layer_enabled(UINT layer, BOOL enabled);
BOOL scrndrawva_layer_enabled(UINT layer);

void scrndrawva_compose_begin(void);
void scrndrawva_compose_raster(void);

//void scrndrawva_draw_sub(const SCRNSURF	*surf);

#ifdef __cplusplus
}
#endif
