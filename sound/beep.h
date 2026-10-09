

enum {
	/* beepcfg.vol is the legacy 0-3 volume in units of 1 << BEEPVOL_SHIFT. */
	BEEPVOL_SHIFT = 7,
	BEEPEVENT_MAXBIT = 8,
	BEEPEVENT_MAX = (1 << BEEPEVENT_MAXBIT)
};

typedef struct {
	SINT32 clock;
	int enable;
} BPEVENT;

typedef struct {
	UINT16 cnt;
	UINT16 hz;
	int buz;
	int __puchi;
	BYTE mode;
	BYTE padding[3];

	int low;
	int enable;
	int lastenable;
	SINT32 clock;
	UINT events;
	BPEVENT event[BEEPEVENT_MAX];
} _BEEP, *BEEP;

typedef struct {
	UINT rate;
	UINT vol;
	UINT __puchibase;
	UINT samplebase;
} BEEPCFG;

#ifdef __cplusplus
extern "C" {
#endif

extern _BEEP beep;
extern _BEEP fbeep;

void beep_initialize(UINT rate);
void beep_deinitialize(void);
void beep_setvol(UINT vol);
void beep_setlevel(UINT level);
void beep_changeclock(void);

void beep_reset(void);
void beep_hzset(UINT16 cnt, UINT beepclock);
void beep_modeset(void);
void beep_eventinit(void);
void beep_eventreset(void);
void beep_lheventset(int beep_low);
void beep_oneventset(void);
void beep_portsoundset(void);

void SOUNDCALL beep_getpcm(BEEP bp, SINT32 *pcm, UINT count);

#ifdef __cplusplus
}
#endif
