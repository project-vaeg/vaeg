
#include	"clockscale.h"

extern CLOCKSCALE pccore_cpu_scale;

#if defined(X11) && (defined(i386) || defined(__i386__))
#define	INHIBIT_WORDP(m)	((m) >= (UPD9002_MEMWRITEMAX - 1))
#elif (defined(ARM) || defined(X11)) && defined(BYTESEX_LITTLE)
#define	INHIBIT_WORDP(m)	(((m) & 1) || ((m) >= UPD9002_MEMWRITEMAX))
#else
#define	INHIBIT_WORDP(m)	(1)
#endif

#define	__CBW(src)		(UINT16)((SINT8)(src))
#define	__CBD(src)		((SINT8)(src))
#define	WORD2LONG(src)	((SINT16)(src))


#define	SEGMENTPTR(s)	(((UINT16 *)&UPD9002_SEGREG) + (s))

#define REAL_FLAGREG	(UINT16)((UPD9002_FLAG & 0x7ff) | (UPD9002_OV?O_FLAG:0))

#define	STRING_DIR		((UPD9002_FLAG & D_FLAG)?-1:1)
#define	STRING_DIRx2	((UPD9002_FLAG & D_FLAG)?-2:2)


// ---- flags

#if defined(UPD9002_TEST)

extern UINT8 BYTESZPF(UINT r);
extern UINT8 BYTESZPCF(UINT r);
#define	BYTESZPCF2(a)	BYTESZPCF((a) & 0x1ff)
extern UINT8 WORDSZPF(UINT32 r);
extern UINT8 WORDSZPCF(UINT32 r);

#elif !defined(MEMOPTIMIZE)

extern	UINT8	_szpflag16[0x10000];
#define	BYTESZPF(a)		(iflags[(a)])
#define	BYTESZPCF(a)	(iflags[(a)])
#define	BYTESZPCF2(a)	(iflags[(a) & 0x1ff])
#define	WORDSZPF(a)		(_szpflag16[(a)])
#define	WORDSZPCF(a)	(_szpflag16[LOW16(a)] + (((a) >> 16) & 1))

#else

#define	BYTESZPF(a)		(iflags[(a)])
#define	BYTESZPCF(a)	(iflags[(a)])
#define	BYTESZPCF2(a)	(iflags[(a) & 0x1ff])
#define	WORDSZPF(a)		((iflags[(a) & 0xff] & P_FLAG) + \
									(((a))?0:Z_FLAG) + (((a) >> 8) & S_FLAG))
#define	WORDSZPCF(a)	((iflags[(a) & 0xff] & P_FLAG) + \
							((LOW16(a))?0:Z_FLAG) + (((a) >> 8) & S_FLAG) + \
							(((a) >> 16) & 1))

#endif


// ---- reg position

#if !defined(MEMOPTIMIZE) || (MEMOPTIMIZE < 2)
extern	UINT8	*_reg8_b53[256];
extern	UINT8	*_reg8_b20[256];
#define	REG8_B53(op)		_reg8_b53[(op)]
#define	REG8_B20(op)		_reg8_b20[(op)]
#else
#if defined(BYTESEX_LITTLE)
#define	REG8_B53(op)		\
				(((UINT8 *)&UPD9002_REG) + (((op) >> 2) & 6) + (((op) >> 5) & 1))
#define	REG8_B20(op)		\
				(((UINT8 *)&UPD9002_REG) + (((op) & 3) * 2) + (((op) >> 2) & 1))
#else
#define	REG8_B53(op)		(((UINT8 *)&UPD9002_REG) + (((op) >> 2) & 6) +	\
													((((op) >> 5) & 1) ^ 1))
#define	REG8_B20(op)		(((UINT8 *)&UPD9002_REG) + (((op) & 3) * 2) +	\
													((((op) >> 2) & 1) ^ 1))
#endif
#endif

#if !defined(MEMOPTIMIZE) || (MEMOPTIMIZE < 2)
extern	UINT16	*_reg16_b53[256];
extern	UINT16	*_reg16_b20[256];
#define	REG16_B53(op)		_reg16_b53[(op)]
#define	REG16_B20(op)		_reg16_b20[(op)]
#else
#define	REG16_B53(op)		(((UINT16 *)&UPD9002_REG) + (((op) >> 3) & 7))
#define	REG16_B20(op)		(((UINT16 *)&UPD9002_REG) + ((op) & 7))
#endif


// ---- ea

#if !defined(MEMOPTIMIZE) || (MEMOPTIMIZE < 2)
typedef UINT32 (*CALCEA)(void);
typedef UINT16 (*CALCLEA)(void);
typedef UINT (*GETLEA)(UINT32 *seg);
extern	CALCEA	_calc_ea_dst[];
extern	CALCLEA	_calc_lea[];
extern	GETLEA	_get_ea[];
#define	CALC_EA(o)		(_calc_ea_dst[(o)]())
#define	CALC_LEA(o)		(_calc_lea[(o)]())
#define	GET_EA(o, s)	(_get_ea[(o)](s))
#else
extern UINT32 calc_ea_dst(UINT op);
extern UINT16 calc_lea(UINT op);
extern UINT calc_a(UINT op, UINT32 *seg);
#define	CALC_EA(o)		(calc_ea_dst(o))
#define	CALC_LEA(o)		(calc_lea(o))
#define	GET_EA(o, s)	(calc_a(o, s))
#endif


#define	SWAPBYTE(p, q) {											\
		REG8 tmp;													\
		tmp = (p);													\
		(p) = (q);													\
		(q) = tmp;													\
	}

#define	SWAPWORD(p, q) {											\
		REG16 tmp;													\
		tmp = (p);													\
		(p) = (q);													\
		(q) = tmp;													\
	}


#define	UPD9002_IRQCHECKTERM											\
		if (UPD9002_REMCLOCK > 0) {									\
			UPD9002_BASECLOCK -= UPD9002_REMCLOCK;						\
			UPD9002_REMCLOCK = 0;										\
		}


#define	REMOVE_PREFIX												\
		SS_FIX = SS_BASE;											\
		DS_FIX = DS_BASE;


#define	UPD9002_WORKCLOCK(c)	UPD9002_REMCLOCK -= (SINT32)clockscale_apply( \
											&pccore_cpu_scale, (UINT32)(c))


#define	GET_PCBYTE(b)												\
		(b) = upd9002_memoryread(CS_BASE + UPD9002_IP);					\
		UPD9002_IP++;


#define	GET_PCBYTES(b)												\
		(b) = __CBW(upd9002_memoryread(CS_BASE + UPD9002_IP));			\
		UPD9002_IP++;


#define	GET_PCBYTESD(b)												\
		(b) = __CBD(upd9002_memoryread(CS_BASE + UPD9002_IP));			\
		UPD9002_IP++;


#define	GET_PCWORD(b)												\
		(b) = upd9002_memoryread_w(CS_BASE + UPD9002_IP);					\
		UPD9002_IP += 2;


#define	PREPART_EA_REG8(b, d_s)										\
		GET_PCBYTE((b))												\
		(d_s) = *(REG8_B53(b));


#define	PREPART_EA_REG8P(b, d_s)									\
		GET_PCBYTE((b))												\
		(d_s) = REG8_B53(b);


#define	PREPART_EA_REG16(b, d_s)									\
		GET_PCBYTE((b))												\
		(d_s) = *(REG16_B53(b));


#define	PREPART_EA_REG16P(b, d_s)									\
		GET_PCBYTE((b))												\
		(d_s) = REG16_B53(b);


#define PREPART_REG8_EA(b, s, d, regclk, memclk)					\
		GET_PCBYTE((b))												\
		if ((b) >= 0xc0) {											\
			UPD9002_WORKCLOCK(regclk);									\
			(s) = *(REG8_B20(b));									\
		}															\
		else {														\
			UPD9002_WORKCLOCK(memclk);									\
			(s) = upd9002_memoryread(CALC_EA(b));						\
		}															\
		(d) = REG8_B53(b);


#define	PREPART_REG16_EA(b, s, d, regclk, memclk)					\
		GET_PCBYTE(b)												\
		if (b >= 0xc0) {											\
			UPD9002_WORKCLOCK(regclk);									\
			s = *(REG16_B20(b));									\
		}															\
		else {														\
			UPD9002_WORKCLOCK(memclk);									\
			s = upd9002_memoryread_w(CALC_EA(b));						\
		}															\
		d = REG16_B53(b);


#define	ADDBYTE(r, d, s)											\
		(r) = (s) + (d);											\
		UPD9002_OV = ((r) ^ (s)) & ((r) ^ (d)) & 0x80;					\
		UPD9002_FLAGL = (BYTE)(((r) ^ (d) ^ (s)) & A_FLAG);			\
		UPD9002_FLAGL |= BYTESZPCF(r);

#define	ADDWORD(r, d, s)											\
		(r) = (s) + (d);											\
		UPD9002_OV = ((r) ^ (s)) & ((r) ^ (d)) & 0x8000;				\
		UPD9002_FLAGL = (UINT8)(((r) ^ (d) ^ (s)) & A_FLAG);			\
		UPD9002_FLAGL |= WORDSZPCF(r);


// flag no check
#define	ORBYTE(d, s)												\
		(d) |= (s);													\
		UPD9002_OV = 0;												\
		UPD9002_FLAGL = BYTESZPF(d);

#define	ORWORD(d, s)												\
		(d) |= (s);													\
		UPD9002_OV = 0;												\
		UPD9002_FLAGL = WORDSZPF(d);


#define	ADCBYTE(r, d, s) 											\
		(r) = (UPD9002_FLAGL & 1) + (s) + (d);							\
		UPD9002_OV = ((r) ^ (s)) & ((r) ^ (d)) & 0x80;					\
		UPD9002_FLAGL = (UINT8)(((r) ^ (d) ^ (s)) & A_FLAG);			\
		UPD9002_FLAGL |= BYTESZPCF(r);

#define	ADCWORD(r, d, s) 											\
		(r) = (UPD9002_FLAGL & 1) + (s) + (d);							\
		UPD9002_OV = ((r) ^ (s)) & ((r) ^ (d)) & 0x8000;				\
		UPD9002_FLAGL = (UINT8)(((r) ^ (d) ^ (s)) & A_FLAG);			\
		UPD9002_FLAGL |= WORDSZPCF(r);


// flag no check
#define	SBBBYTE(r, d, s) 											\
		(r) = (d) - (s) - (UPD9002_FLAGL & 1);							\
		UPD9002_OV = ((d) ^ (r)) & ((d) ^ (s)) & 0x80;					\
		UPD9002_FLAGL = (UINT8)(((r) ^ (d) ^ (s)) & A_FLAG);			\
		UPD9002_FLAGL |= BYTESZPCF2(r);

#define	SBBWORD(r, d, s) 											\
		(r) = (d) - (s) - (UPD9002_FLAGL & 1);							\
		UPD9002_OV = ((d) ^ (r)) & ((d) ^ (s)) & 0x8000;				\
		UPD9002_FLAGL = (UINT8)(((r) ^ (d) ^ (s)) & A_FLAG);			\
		UPD9002_FLAGL |= WORDSZPCF(r);


// flag no check
#define	ANDBYTE(d, s)												\
		(d) &= (s);													\
		UPD9002_OV = 0;												\
		UPD9002_FLAGL = BYTESZPF(d);

#define	ANDWORD(d, s)												\
		(d) &= (s);													\
		UPD9002_OV = 0;												\
		UPD9002_FLAGL = WORDSZPF(d);


// flag no check
#define	SUBBYTE(r, d, s) 											\
		(r) = (d) - (s);											\
		UPD9002_OV = ((d) ^ (r)) & ((d) ^ (s)) & 0x80;					\
		UPD9002_FLAGL = (UINT8)(((r) ^ (d) ^ (s)) & A_FLAG);			\
		UPD9002_FLAGL |= BYTESZPCF2(r);

#define	SUBWORD(r, d, s) 											\
		(r) = (d) - (s);											\
		UPD9002_OV = ((d) ^ (r)) & ((d) ^ (s)) & 0x8000;				\
		UPD9002_FLAGL = ((r) ^ (d) ^ (s)) & A_FLAG;					\
		UPD9002_FLAGL |= WORDSZPCF(r);


// flag no check
#define	XORBYTE(d, s)												\
		(d) ^= s;													\
		UPD9002_OV = 0;												\
		UPD9002_FLAGL = BYTESZPF(d);

#define	XORWORD(d, s)												\
		(d) ^= (s);													\
		UPD9002_OV = 0;												\
		UPD9002_FLAGL = WORDSZPF(d);


#define	NEGBYTE(d, s) 												\
		(d) = 0 - (s);												\
		UPD9002_OV = ((d) & (s)) & 0x80;								\
		UPD9002_FLAGL = (UINT8)(((d) ^ (s)) & A_FLAG);					\
		UPD9002_FLAGL |= BYTESZPCF2(d);

#define	NEGWORD(d, s) 												\
		(d) = 0 - (s);												\
		UPD9002_OV = ((d) & (s)) & 0x8000;								\
		UPD9002_FLAGL = (UINT8)(((d) ^ (s)) & A_FLAG);					\
		UPD9002_FLAGL |= WORDSZPCF(d);


#define	BYTE_MUL(r, d, s)											\
		UPD9002_FLAGL &= (Z_FLAG | S_FLAG | A_FLAG | P_FLAG);			\
		(r) = (UINT8)(d) * (UINT8)(s);								\
		UPD9002_OV = (r) >> 8;											\
		if (UPD9002_OV) {												\
			UPD9002_FLAGL |= C_FLAG;									\
		}

#define	WORD_MUL(r, d, s)											\
		UPD9002_FLAGL &= (Z_FLAG | S_FLAG | A_FLAG | P_FLAG);			\
		(r) = (UINT16)(d) * (UINT16)(s);							\
		UPD9002_OV = (r) >> 16;										\
		if (UPD9002_OV) {												\
			UPD9002_FLAGL |= C_FLAG;									\
		}


#define	BYTE_IMUL(r, d, s)											\
		UPD9002_FLAGL &= (Z_FLAG | S_FLAG | A_FLAG | P_FLAG);			\
		(r) = (SINT8)(d) * (SINT8)(s);								\
		UPD9002_OV = ((r) + 0x80) & 0xffffff00;						\
		if (UPD9002_OV) {												\
			UPD9002_FLAGL |= C_FLAG;									\
		}

#define	WORD_IMUL(r, d, s)											\
		UPD9002_FLAGL &= (Z_FLAG | S_FLAG | A_FLAG | P_FLAG);			\
		(r) = (SINT16)(d) * (SINT16)(s);							\
		UPD9002_OV = ((r) + 0x8000) & 0xffff0000;						\
		if (UPD9002_OV) {												\
			UPD9002_FLAGL |= C_FLAG;									\
		}


// flag no check
#define	INCBYTE(s) {												\
		UINT b = (s);												\
		(s)++;														\
		UPD9002_OV = (s) & (b ^ (s)) & 0x80;							\
		UPD9002_FLAGL &= C_FLAG;										\
		UPD9002_FLAGL |= (UINT8)((b ^ (s)) & A_FLAG);					\
		UPD9002_FLAGL |= BYTESZPF((UINT8)(s));							\
	}

#define	INCWORD(s) {												\
		UINT32 b = (s);												\
		(s)++;														\
		UPD9002_OV = (s) & (b ^ (s)) & 0x8000;							\
		UPD9002_FLAGL &= C_FLAG;										\
		UPD9002_FLAGL |= (UINT8)((b ^ (s)) & A_FLAG);					\
		UPD9002_FLAGL |= WORDSZPF((UINT16)(s));						\
	}


// flag no check
#define	DECBYTE(s) {												\
		UINT b = (s);												\
		b--;														\
		UPD9002_OV = (s) & (b ^ (s)) & 0x80;							\
		UPD9002_FLAGL &= C_FLAG;										\
		UPD9002_FLAGL |= (UINT8)((b ^ (s)) & A_FLAG);					\
		UPD9002_FLAGL |= BYTESZPF((UINT8)b);							\
		(s) = b;													\
	}

#define	DECWORD(s) {												\
		UINT32 b = (s);												\
		b--;														\
		UPD9002_OV = (s) & (b ^ (s)) & 0x8000;							\
		UPD9002_FLAGL &= C_FLAG;										\
		UPD9002_FLAGL |= (UINT8)((b ^ (s)) & A_FLAG);					\
		UPD9002_FLAGL |= WORDSZPF((UINT16)b);							\
		(s) = b;													\
	}


// flag no check
#define	INCWORD2(r, clock) {										\
		REG16 s = (r);												\
		REG16 d = (r);												\
		d++;														\
		(r) = (UINT16)d;											\
		UPD9002_OV = d & (d ^ s) & 0x8000;								\
		UPD9002_FLAGL &= C_FLAG;										\
		UPD9002_FLAGL |= (UINT8)((d ^ s) & A_FLAG);					\
		UPD9002_FLAGL |= WORDSZPF((UINT16)d);							\
		UPD9002_WORKCLOCK(clock);										\
	}

#define	DECWORD2(r, clock) {										\
		REG16 s = (r);												\
		REG16 d = (r);												\
		d--;														\
		(r) = (UINT16)d;											\
		UPD9002_OV = s & (d ^ s) & 0x8000;								\
		UPD9002_FLAGL &= C_FLAG;										\
		UPD9002_FLAGL |= (UINT8)((d ^ s) & A_FLAG);					\
		UPD9002_FLAGL |= WORDSZPF((UINT16)d);							\
		UPD9002_WORKCLOCK(clock);										\
	}


// ---- stack

#define	REGPUSH0(reg)												\
		UPD9002_SP -= 2;												\
		upd9002_memorywrite_w(UPD9002_SP + SS_BASE, reg);

#define	REGPOP0(reg) 												\
		reg = upd9002_memoryread_w(UPD9002_SP + SS_BASE);					\
		UPD9002_SP += 2;

#if (defined(ARM) || defined(X11)) && defined(BYTESEX_LITTLE)

#define	REGPUSH(reg, clock)	{										\
		UINT32 addr;												\
		UPD9002_WORKCLOCK(clock);										\
		UPD9002_SP -= 2;												\
		addr = UPD9002_SP + SS_BASE;									\
		if (INHIBIT_WORDP(addr)) {									\
			upd9002_memorywrite_w(addr, reg);							\
		}															\
		else {														\
			*(UINT16 *)(mem + addr) = (reg);						\
		}															\
	}

#define	REGPOP(reg, clock) {										\
		UINT32 addr;												\
		UPD9002_WORKCLOCK(clock);										\
		addr = UPD9002_SP + SS_BASE;									\
		if (INHIBIT_WORDP(addr)) {									\
			(reg) = upd9002_memoryread_w(addr);						\
		}															\
		else {														\
			(reg) = *(UINT16 *)(mem + addr);						\
		}															\
		UPD9002_SP += 2;												\
	}

#else

#define	REGPUSH(reg, clock)	{										\
		UPD9002_WORKCLOCK(clock);										\
		UPD9002_SP -= 2;												\
		upd9002_memorywrite_w(UPD9002_SP + SS_BASE, reg);					\
	}

#define	REGPOP(reg, clock) {										\
		UPD9002_WORKCLOCK(clock);										\
		reg = upd9002_memoryread_w(UPD9002_SP + SS_BASE);					\
		UPD9002_SP += 2;												\
	}

#endif

#define	SP_PUSH(reg, clock)	{										\
		REG16 sp = (reg);											\
		UPD9002_SP -= 2;												\
		upd9002_memorywrite_w(UPD9002_SP + SS_BASE, sp);					\
		UPD9002_WORKCLOCK(clock);										\
	}

#define	SP_POP(reg, clock) {										\
		UPD9002_WORKCLOCK(clock);										\
		reg = upd9002_memoryread_w(UPD9002_SP + SS_BASE);					\
	}


#define	JMPSHORT(clock) {											\
		UPD9002_WORKCLOCK(clock);										\
		UPD9002_IP += __CBW(upd9002_memoryread(CS_BASE + UPD9002_IP));		\
		UPD9002_IP++;													\
	}


#define	JMPNOP(clock) {												\
		UPD9002_WORKCLOCK(clock);										\
		UPD9002_IP++;													\
	}


#define	MOVIMM8(reg) {												\
		UPD9002_WORKCLOCK(2);											\
		GET_PCBYTE(reg)												\
	}


#define	MOVIMM16(reg) {												\
		UPD9002_WORKCLOCK(2);											\
		GET_PCWORD(reg)												\
	}


#define	SEGSELECT(c)	((UPD9002_MSW & MSW_PE)?upd9002_selector(c):((c) << 4))

#define	INT_NUM(a, b)	upd9002_intnum((a), (REG16)(b))
