/*
 * subsystemmx.h: PC-88VA FD Sub System (multiplexer)
 *
 */

#if !defined(_SUBSYSTEMMX_H_)
#define _SUBSYSTEMMX_H_

typedef struct {
	BOOL mockup;
} _SUBSYSTEMMXCFG, *SUBSYSTEMMXCFG;

#ifdef __cplusplus
extern "C" {
#endif

extern _SUBSYSTEMMXCFG subsystemmxcfg;

void subsystemmx_initialize(void);
void subsystemmx_reset(void);
void subsystemmx_bind(void);
void subsystemmx_exec(void);

#ifdef __cplusplus
}
#endif

#endif /* _SUBSYSTEMMX_H_ */