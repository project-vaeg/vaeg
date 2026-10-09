
#ifdef __cplusplus
extern "C" {
#endif

void timing_reset(void);
void timing_setrate(UINT lines, UINT crthz);
void timing_setcount(UINT value);
void timing_hosttick(void);
UINT timing_getcount(void);
void timing_setspeed(UINT percent);
UINT timing_addspan(UINT32 span);

#ifdef __cplusplus
}
#endif
