/*
 * memctrlva.h: PC-88VA memory control
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

void memctrlva_reset(void);
void memctrlva_bind(void);
BOOL memctrlva_nmode_active(void);
BOOL memctrlva_nmode_boot_search(void);
void memctrlva_nmode_reset(void);
void memctrlva_nmode_compat_entry(void);

#ifdef __cplusplus
}
#endif
