/*
 * makesprva.c: PC-88VA Sprite
 */

#ifdef __cplusplus
extern "C" {
#endif

extern BYTE sprraster[];

void makesprva_initialize(void);

void makesprva_begin(void);
void makesprva_raster(void);
void makesprva_blankraster(void);

#ifdef __cplusplus
}
#endif
