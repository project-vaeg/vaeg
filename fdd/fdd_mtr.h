
typedef struct {
	int busy;
	UINT8 head[4];
	UINT nextevent;
	UINT8 curevent;
} _FDDMTR, *FDDMTR;

#ifdef __cplusplus
extern "C" {
#endif

extern _FDDMTR fddmtr;

void fdbiosout(NEVENTITEM item);

void fddmtr_initialize(void);
void fddmtr_callback(UINT time);
void fddmtr_seek(REG8 drv, REG8 c, UINT size);

void fddmtrsnd_initialize(UINT rate);
void fddmtrsnd_bind(void);
void fddmtrsnd_deinitialize(void);
void fddmtrsnd_volume(UINT volume);
void fddmtrsnd_stop(void);
void fddmtrsnd_seek(BOOL one_track, UINT duration_ms);
void fddmtrsnd_headload(BOOL one_track);
void fddmtrsnd_headunload(void);

#ifdef __cplusplus
}
#endif
