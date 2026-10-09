//----------------------------------------------------------------------------
//
//  i286c : 80286 Engine for Pentium  ver0.05
//
//                               Copyright by Yui/Studio Milmake 1999-2003
//
//----------------------------------------------------------------------------

#ifndef VAEG_UPD9002_CPUCORE_H
#define VAEG_UPD9002_CPUCORE_H

#include "memory.h"

/*
 * Native VA memory is always decoded by memoryva. Zero direct-access limits
 * keep legacy instruction helpers on the mapped accessors as well.
 */
enum {
	UPD9002_MEMREADMAX = 0,
	UPD9002_MEMWRITEMAX = 0
};
enum {
	C_FLAG = 0x0001,
	P_FLAG = 0x0004,
	A_FLAG = 0x0010,
	Z_FLAG = 0x0040,
	S_FLAG = 0x0080,
	T_FLAG = 0x0100,
	I_FLAG = 0x0200,
	D_FLAG = 0x0400,
	O_FLAG = 0x0800
};

enum {
	MSW_PE = 0x0001,
	MSW_MP = 0x0002,
	MSW_EM = 0x0004,
	MSW_TS = 0x0008
};

enum {
	CPUTYPE_V30 = 0x01
};

#ifndef CPUCALL
#define CPUCALL
#endif

#if defined(BYTESEX_LITTLE)

typedef struct {
	UINT8 al;
	UINT8 ah;
	UINT8 cl;
	UINT8 ch;
	UINT8 dl;
	UINT8 dh;
	UINT8 bl;
	UINT8 bh;
	UINT8 sp_l;
	UINT8 sp_h;
	UINT8 bp_l;
	UINT8 bp_h;
	UINT8 si_l;
	UINT8 si_h;
	UINT8 di_l;
	UINT8 di_h;
	UINT8 es_l;
	UINT8 es_h;
	UINT8 cs_l;
	UINT8 cs_h;
	UINT8 ss_l;
	UINT8 ss_h;
	UINT8 ds_l;
	UINT8 ds_h;
	UINT8 flag_l;
	UINT8 flag_h;
	UINT8 ip_l;
	UINT8 ip_h;
} Upd9002RegisterBytes;

#else

typedef struct {
	UINT8 ah;
	UINT8 al;
	UINT8 ch;
	UINT8 cl;
	UINT8 dh;
	UINT8 dl;
	UINT8 bh;
	UINT8 bl;
	UINT8 sp_h;
	UINT8 sp_l;
	UINT8 bp_h;
	UINT8 bp_l;
	UINT8 si_h;
	UINT8 si_l;
	UINT8 di_h;
	UINT8 di_l;
	UINT8 es_h;
	UINT8 es_l;
	UINT8 cs_h;
	UINT8 cs_l;
	UINT8 ss_h;
	UINT8 ss_l;
	UINT8 ds_h;
	UINT8 ds_l;
	UINT8 flag_h;
	UINT8 flag_l;
	UINT8 ip_h;
	UINT8 ip_l;
} Upd9002RegisterBytes;

#endif

typedef struct {
	UINT16 ax;
	UINT16 cx;
	UINT16 dx;
	UINT16 bx;
	UINT16 sp;
	UINT16 bp;
	UINT16 si;
	UINT16 di;
	UINT16 es;
	UINT16 cs;
	UINT16 ss;
	UINT16 ds;
	UINT16 flag;
	UINT16 ip;
} Upd9002RegisterWords;

typedef struct {
	UINT16 limit;
	UINT16 base;
	UINT8 base24;
	UINT8 reserved;
} Upd9002DescriptorImage;

typedef struct {
	union {
		Upd9002RegisterBytes b;
		Upd9002RegisterWords w;
	} r;
	UINT32 es_base;
	UINT32 cs_base;
	UINT32 ss_base;
	UINT32 ds_base;
	UINT32 ss_fix;
	UINT32 ds_fix;
	UINT32 adrsmask; // ver0.72
	UINT16 prefix;
	UINT8 trap;
	UINT8 resetreq; // ver0.72
	UINT32 ovflag;
	Upd9002DescriptorImage GDTR;
	UINT16 MSW;
	Upd9002DescriptorImage IDTR;
	UINT16 LDTR; // ver0.73
	Upd9002DescriptorImage LDTRC;
	UINT16 TR;
	Upd9002DescriptorImage TRC;
	UINT8 padding[2];

	UINT8 cpu_type;
	UINT8 itfbank; // ver0.72
	UINT16 ram_d0;
	SINT32 remainclock;
	SINT32 baseclock;
	UINT32 clock;
} Upd9002StateImage;

typedef struct {
	union {
		Upd9002RegisterBytes b;
		Upd9002RegisterWords w;
	} r;
	UINT32 es_base;
	UINT32 cs_base;
	UINT32 ss_base;
	UINT32 ds_base;
	UINT32 ss_fix;
	UINT32 ds_fix;
	UINT32 adrsmask; // ver0.72
	UINT16 prefix;
	UINT8 trap;
	UINT8 resetreq; // ver0.72
	UINT32 ovflag;
	Upd9002DescriptorImage GDTR;
	UINT16 MSW;
	Upd9002DescriptorImage IDTR;
	UINT16 LDTR; // ver0.73
	Upd9002DescriptorImage LDTRC;
	UINT16 TR;
	Upd9002DescriptorImage TRC;
	UINT8 padding[2];

	UINT8 cpu_type;
	UINT8 itfbank; // ver0.72
	UINT16 ram_d0;
	SINT32 remainclock;
	SINT32 baseclock;
	UINT32 clock;
} Upd9002RuntimeState;

typedef struct {
	UINT8 bytes[112];
} Upd9002StateOpaqueImage;
typedef struct {
	/* for ver0.73 */
	BYTE *ext;
	UINT32 extsize;
	BYTE *ems[4];
	UINT32 inport;
} Upd9002ExtendedState;

typedef struct {
	Upd9002RuntimeState s;
	Upd9002ExtendedState e;
} Upd9002CoreContext;

#ifdef __cplusplus
extern "C" {
#endif

extern Upd9002CoreContext upd9002_core_context;
extern const UINT8 iflags[];

typedef struct {
	void (*reset)(void);
	void (*enter)(void);
	void (*step)(void);
	void (*sync_to_native)(void);
	void (*leave)(void);
	void (*resume)(void);
	int (*state_save)(UINT8 *buffer, UINT size);
	int (*state_load)(const UINT8 *buffer, UINT size);
	void (*irq)(BOOL asserted);
} Upd9002CompatHooks;

enum {
	UPD9002_COMPAT_NATIVE = 0,
	UPD9002_COMPAT_UPD70008 = 1
};

#define UPD9002_COMPAT_STATE_SIZE 68
#define UPD9002_COMPAT_STATE_SECTION "UPD9Z80"
/*
 * Mode-transition state that UPD9CPU does not carry: execution mode, the
 * CALLN/interrupt return-pending flag and the native SP that identifies the
 * returning IRET. Without it, a state saved while compatible code waits in a
 * native handler resumes that handler but never returns to compatible mode.
 */
#define UPD9002_MODE_STATE_SECTION "UPD9MODE"
#define UPD9002_MODE_STATE_SIZE 4

/*
 * M103a: the Z80 emulation mode's alternate register set (AF', BC', DE',
 * HL'). Native mode cannot address these eight bytes (see
 * docs/modernization/upd9002-upd70008-mode.md 3.1 and 11-4), so they are
 * machine-owned storage beside the native register file rather than part
 * of it. The uPD70008-compatible adapter copies them in on mode entry and
 * resume and out after every compatible-mode step, exactly like the main
 * set aliases. Set to FFFFh only by the hardware reset path (upd9002_core_reset;
 * M103c, after a real PC-88VA2 register dump);
 * CALLN round trips, RETEM/BRKEM re-entry and adapter resets preserve them.
 * Saved as the "UPD9ALT" state section, which precedes the compat blob;
 * its loader sets upd9002_alt_regs_loaded so that the adapter's StateLoad
 * lets the storage win over the blob's working copy. A state file written
 * before M103a has no such section and keeps the blob's values.
 */
typedef struct {
	UINT16 af;
	UINT16 bc;
	UINT16 de;
	UINT16 hl;
} Upd9002AltRegs;

#define UPD9002_ALT_STATE_SECTION "UPD9ALT"

extern Upd9002AltRegs upd9002_alt_regs;
extern BOOL upd9002_alt_regs_loaded;

void upd9002_core_initialize(void);
void upd9002_core_deinitialize(void);
void upd9002_core_reset(void);
void upd9002_core_shut(void);
void upd9002_core_set_ext_size(UINT32 size);
void upd9002_core_set_emm(UINT frame, UINT32 addr);
void upd9002_core_set_compat_hooks(const Upd9002CompatHooks *hooks);
void CPUCALL upd9002_core_brkem(REG8 vect);
void CPUCALL upd9002_core_compat_calln(REG8 vect, REG16 return_ip);
void CPUCALL upd9002_core_compat_iotrap(REG8 vect, REG16 instruction_ip);
/* 8214 mode: drive the compatible core's INT line (vector on acknowledge). */
void CPUCALL upd9002_core_compat_irq(BOOL asserted);
BOOL CPUCALL upd9002_core_compat_iret_is_return(void);
void CPUCALL upd9002_core_compat_retem(void);
void CPUCALL upd9002_core_compat_iret_resume(void);
int upd9002_core_compat_state_save(UINT8 *buffer, UINT size);
int upd9002_core_compat_state_load(const UINT8 *buffer, UINT size);
void upd9002_core_mode_state_save(UINT8 *buffer);
BOOL upd9002_core_mode_state_valid(const UINT8 *buffer);
int upd9002_core_mode_state_load(const UINT8 *buffer);

void CPUCALL upd9002_core_interrupt(REG8 vect);

void upd9002_core_step(void);
#if defined(VAEG_UPD9002_M46_TESTING)
int upd9002_dispatch_test_verify(void);
void upd9002_dispatch_test_require_immutable(void);
UINT upd9002_dispatch_test_construction_count(void);
UINT upd9002_dispatch_test_rejected_count(void);
#endif

#ifdef __cplusplus
}
#endif

// ---- macros

#define CPU_STATSAVE upd9002_core_context.s

#define CPU_AX upd9002_core_context.s.r.w.ax
#define CPU_BX upd9002_core_context.s.r.w.bx
#define CPU_CX upd9002_core_context.s.r.w.cx
#define CPU_DX upd9002_core_context.s.r.w.dx
#define CPU_SI upd9002_core_context.s.r.w.si
#define CPU_DI upd9002_core_context.s.r.w.di
#define CPU_BP upd9002_core_context.s.r.w.bp
#define CPU_SP upd9002_core_context.s.r.w.sp
#define CPU_CS upd9002_core_context.s.r.w.cs
#define CPU_DS upd9002_core_context.s.r.w.ds
#define CPU_ES upd9002_core_context.s.r.w.es
#define CPU_SS upd9002_core_context.s.r.w.ss
#define CPU_IP upd9002_core_context.s.r.w.ip

#define ES_BASE upd9002_core_context.s.es_base
#define CS_BASE upd9002_core_context.s.cs_base
#define SS_BASE upd9002_core_context.s.ss_base
#define DS_BASE upd9002_core_context.s.ds_base

#define CPU_AL upd9002_core_context.s.r.b.al
#define CPU_BL upd9002_core_context.s.r.b.bl
#define CPU_CL upd9002_core_context.s.r.b.cl
#define CPU_DL upd9002_core_context.s.r.b.dl
#define CPU_AH upd9002_core_context.s.r.b.ah
#define CPU_BH upd9002_core_context.s.r.b.bh
#define CPU_CH upd9002_core_context.s.r.b.ch
#define CPU_DH upd9002_core_context.s.r.b.dh

#define CPU_FLAG upd9002_core_context.s.r.w.flag
#define CPU_FLAGL upd9002_core_context.s.r.b.flag_l

#define CPU_REMCLOCK upd9002_core_context.s.remainclock
#define CPU_BASECLOCK upd9002_core_context.s.baseclock
#define CPU_CLOCK upd9002_core_context.s.clock
#define CPU_ADRSMASK upd9002_core_context.s.adrsmask
#define CPU_MSW upd9002_core_context.s.MSW
#define CPU_RESETREQ upd9002_core_context.s.resetreq
#define CPU_ITFBANK upd9002_core_context.s.itfbank
#define CPU_RAM_D000 upd9002_core_context.s.ram_d0

#define CPU_EXTMEM upd9002_core_context.e.ext
#define CPU_EXTMEMSIZE upd9002_core_context.e.extsize
#define CPU_INPADRS upd9002_core_context.e.inport
#define CPU_COMPAT_MODE upd9002_core_context.s.padding[0]
#define CPU_COMPAT_RETURN_PENDING upd9002_core_context.s.padding[1]
#define CPU_EMSPTR upd9002_core_context.e.ems

#define CPU_isDI (!(upd9002_core_context.s.r.w.flag & I_FLAG))
#define CPU_isEI (upd9002_core_context.s.r.w.flag & I_FLAG)
#define CPU_CLI                                                                                    \
	upd9002_core_context.s.r.w.flag &= ~I_FLAG;                                                    \
	upd9002_core_context.s.trap = 0;
#define CPU_STI                                                                                    \
	upd9002_core_context.s.r.w.flag |= I_FLAG;                                                     \
	upd9002_core_context.s.trap = (upd9002_core_context.s.r.w.flag >> 8) & 1;
#define CPU_A20EN(en) CPU_ADRSMASK = (en) ? 0xfffffff : 0x000fffff;

#define CPU_INITIALIZE upd9002_core_initialize
#define CPU_DEINITIALIZE upd9002_core_deinitialize
#define CPU_RESET upd9002_core_reset
#define CPU_CLEARPREFETCH()
#define CPU_INTERRUPT(vect, soft) upd9002_core_interrupt(vect)
#define CPU_SHUT upd9002_core_shut
#define CPU_SETEXTSIZE(size) upd9002_core_set_ext_size((UINT32)(size) << 20)
#define CPU_SETEMM(frame, addr) upd9002_core_set_emm(frame, addr)

#endif
