#ifndef VAEG_IO_IOCORE_H
#define VAEG_IO_IOCORE_H

#ifndef IOOUTCALL
#define IOOUTCALL
#endif
#ifndef IOINPCALL
#define IOINPCALL
#endif

typedef void(IOOUTCALL *IOOUT)(UINT port, REG8 val);
typedef REG8(IOINPCALL *IOINP)(UINT port);
typedef void (*IOCBFN)(void);

#include "lsidef.h"

#include "dmac.h"
#include "emsio.h"
#include "fdc.h"
#include "mouseif.h"
#include "np2sysp.h"
#include "pic.h"
#include "pit.h"
#include "serial.h"
#include "upd4990.h"

#ifdef __cplusplus
extern "C" {
#endif

extern _DMAC dmac;
extern _EMSIO emsio;
extern _FDC fdc;
extern _KEYBRD keybrd;
extern _MOUSEIF mouseif;
extern _NP2SYSP np2sysp;
extern _PIC pic;
extern _PIT pit;
extern _RS232C rs232c;
extern _UPD4990 uPD4990;

/* Register an exact port in the native VA 16-bit I/O address space. */
BOOL iocore_attachout(UINT port, IOOUT func);
BOOL iocore_attachinp(UINT port, IOINP func);

void iocore_create(void);
void iocore_destroy(void);
BOOL iocore_build(void);

void iocore_cb(const IOCBFN *cbfn, UINT count);
void iocore_reset(void);
void iocore_bind(void);

void IOOUTCALL iocore_out8(UINT port, REG8 dat);
REG8 IOINPCALL iocore_inp8(UINT port);

void IOOUTCALL iocore_out16(UINT port, REG16 dat);
REG16 IOINPCALL iocore_inp16(UINT port);

void IOOUTCALL iocore_out32(UINT port, UINT32 dat);
UINT32 IOINPCALL iocore_inp32(UINT port);

#ifdef __cplusplus
}
#endif

#endif
