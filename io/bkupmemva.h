/*
 *	BKMEMVA.H: PC-88VA Backup memory
 */

#ifdef __cplusplus
extern "C" {
#endif

void bkupmemva_load(void);
void bkupmemva_sync_mainram(void);
void bkupmemva_save(void);
void bkupmemva_setpath(const char *path);
void bkupmemva_setenabled(BOOL enabled);
BOOL bkupmemva_get_88v1(void);
void bkupmemva_set_88v1(BOOL v1);

#ifdef __cplusplus
}
#endif
