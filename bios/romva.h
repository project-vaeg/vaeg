
enum {
	ROMVA_N80_NAMES = 4
};

#ifdef __cplusplus
extern "C" {
#endif

extern const char *const romva_n80_names[ROMVA_N80_NAMES];

void romva_initialize(void);
const char *romva_default_font_filename(void);
BOOL romva_load_default_font(void);
BOOL romva_load_pc98_font(const char *filename);

#ifdef __cplusplus
}
#endif
