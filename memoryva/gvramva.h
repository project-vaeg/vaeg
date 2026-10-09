/*
 *	GVAMVA.H: PC-88VA GVRAM
 */

#ifdef __cplusplus
extern "C" {
#endif

extern BYTE grphmem[0x40000];

/*
void MEMCALL _gvram_wt(UINT32 address, REG8 value);
*/
//void MEMCALL _gvramw_wt(UINT32 address, REG16 value);
REG8 MEMCALL _gvram_rd(UINT32 address);

#ifdef __cplusplus
}
#endif
