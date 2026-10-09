
#define INTR_FAST

// #define	UPD9002_TEST
#if defined(UPD9002_TEST)
#undef MEMOPTIMIZE
#endif

#define UPD9002_STAT upd9002_core_context.s.r

#define UPD9002_REG upd9002_core_context.s.r
#define UPD9002_SEGREG upd9002_core_context.s.r.w.es

#define UPD9002_AX upd9002_core_context.s.r.w.ax
#define UPD9002_BX upd9002_core_context.s.r.w.bx
#define UPD9002_CX upd9002_core_context.s.r.w.cx
#define UPD9002_DX upd9002_core_context.s.r.w.dx
#define UPD9002_SI upd9002_core_context.s.r.w.si
#define UPD9002_DI upd9002_core_context.s.r.w.di
#define UPD9002_BP upd9002_core_context.s.r.w.bp
#define UPD9002_SP upd9002_core_context.s.r.w.sp
#define UPD9002_CS upd9002_core_context.s.r.w.cs
#define UPD9002_DS upd9002_core_context.s.r.w.ds
#define UPD9002_ES upd9002_core_context.s.r.w.es
#define UPD9002_SS upd9002_core_context.s.r.w.ss
#define UPD9002_IP upd9002_core_context.s.r.w.ip

#define SEG_BASE upd9002_core_context.s.es_base
#define ES_BASE upd9002_core_context.s.es_base
#define CS_BASE upd9002_core_context.s.cs_base
#define SS_BASE upd9002_core_context.s.ss_base
#define DS_BASE upd9002_core_context.s.ds_base
#define SS_FIX upd9002_core_context.s.ss_fix
#define DS_FIX upd9002_core_context.s.ds_fix

#define UPD9002_AL upd9002_core_context.s.r.b.al
#define UPD9002_BL upd9002_core_context.s.r.b.bl
#define UPD9002_CL upd9002_core_context.s.r.b.cl
#define UPD9002_DL upd9002_core_context.s.r.b.dl
#define UPD9002_AH upd9002_core_context.s.r.b.ah
#define UPD9002_BH upd9002_core_context.s.r.b.bh
#define UPD9002_CH upd9002_core_context.s.r.b.ch
#define UPD9002_DH upd9002_core_context.s.r.b.dh

#define UPD9002_FLAG upd9002_core_context.s.r.w.flag
#define UPD9002_FLAGL upd9002_core_context.s.r.b.flag_l
#define UPD9002_FLAGH upd9002_core_context.s.r.b.flag_h
#define UPD9002_TRAP upd9002_core_context.s.trap
#define UPD9002_OV upd9002_core_context.s.ovflag

#define UPD9002_GDTR upd9002_core_context.s.GDTR
#define UPD9002_LDTRC upd9002_core_context.s.LDTRC
#define UPD9002_MSW upd9002_core_context.s.MSW

#define UPD9002_REMCLOCK upd9002_core_context.s.remainclock
#define UPD9002_BASECLOCK upd9002_core_context.s.baseclock
#define UPD9002_CLOCK upd9002_core_context.s.clock
#define UPD9002_ADRSMASK upd9002_core_context.s.adrsmask

#define UPD9002_PREFIX upd9002_core_context.s.prefix

#define UPD9002_INPADRS upd9002_core_context.e.inport

#define UPD9002FN static void
#define UPD9002EXT void

typedef void (*UPD9002OP)(void);

extern void CPUCALL upd9002_intnum(UINT vect, REG16 IP);
extern UINT32 upd9002_selector(UINT sel);

#if !defined(MEMOPTIMIZE) || (MEMOPTIMIZE < 2)
extern void upd9002_ea_initialize(void);
#endif

extern const UPD9002OP upd9002op[];
extern const UPD9002OP upd9002op_repe[];
extern const UPD9002OP upd9002op_repne[];
extern const UPD9002OP upd9002op_repnc[];
extern const UPD9002OP upd9002op_repc[];
extern UINT16 upd9002_step_start_cs;
extern UINT16 upd9002_step_start_ip;
extern UINT8 upd9002_current_opcode;

#define UPD9002_8X static void CPUCALL
typedef void(CPUCALL *UPD9002OP8XREG8)(UINT8 *p);
typedef void(CPUCALL *UPD9002OP8XEXT8)(UINT32 madr);
typedef void(CPUCALL *UPD9002OP8XREG16)(UINT16 *p, UINT32 src);
typedef void(CPUCALL *UPD9002OP8XEXT16)(UINT32 madr, UINT32 src);

extern const UPD9002OP8XREG8 c_op8xreg8_table[];
extern const UPD9002OP8XEXT8 c_op8xext8_table[];
extern const UPD9002OP8XREG16 c_op8xreg16_table[];
extern const UPD9002OP8XEXT16 c_op8xext16_table[];

#define UPD9002_SFT static void CPUCALL
typedef void(CPUCALL *UPD9002OPSFTR8)(UINT8 *p);
typedef void(CPUCALL *UPD9002OPSFTE8)(UINT32 madr);
typedef void(CPUCALL *UPD9002OPSFTR16)(UINT16 *p);
typedef void(CPUCALL *UPD9002OPSFTE16)(UINT32 madr);
typedef void(CPUCALL *UPD9002OPSFTR8CL)(UINT8 *p, REG8 cl);
typedef void(CPUCALL *UPD9002OPSFTE8CL)(UINT32 madr, REG8 cl);
typedef void(CPUCALL *UPD9002OPSFTR16CL)(UINT16 *p, REG8 cl);
typedef void(CPUCALL *UPD9002OPSFTE16CL)(UINT32 madr, REG8 cl);

extern const UPD9002OPSFTR8 sft_r8_table[];
extern const UPD9002OPSFTE8 sft_e8_table[];
extern const UPD9002OPSFTR16 sft_r16_table[];
extern const UPD9002OPSFTE16 sft_e16_table[];
extern const UPD9002OPSFTR8CL sft_r8cl_table[];
extern const UPD9002OPSFTE8CL sft_e8cl_table[];
extern const UPD9002OPSFTR16CL sft_r16cl_table[];
extern const UPD9002OPSFTE16CL sft_e16cl_table[];

#define UPD9002_F6 static void CPUCALL
typedef void(CPUCALL *UPD9002OPF6)(UINT op);

extern const UPD9002OPF6 c_ope0xf6_table[];
extern const UPD9002OPF6 c_ope0xf7_table[];

extern const UPD9002OPF6 c_ope0xfe_table[];
extern const UPD9002OPF6 c_ope0xff_table[];

extern UPD9002EXT upd9002_rep_insb(void);
extern UPD9002EXT upd9002_rep_insw(void);
extern UPD9002EXT upd9002_rep_outsb(void);
extern UPD9002EXT upd9002_rep_outsw(void);
extern UPD9002EXT upd9002_rep_movsb(void);
extern UPD9002EXT upd9002_rep_movsw(void);
extern UPD9002EXT upd9002_rep_lodsb(void);
extern UPD9002EXT upd9002_rep_lodsw(void);
extern UPD9002EXT upd9002_rep_stosb(void);
extern UPD9002EXT upd9002_rep_stosw(void);
extern UPD9002EXT upd9002_repe_cmpsb(void);
extern UPD9002EXT upd9002_repne_cmpsb(void);
extern UPD9002EXT upd9002_repe_cmpsw(void);
extern UPD9002EXT upd9002_repne_cmpsw(void);
extern UPD9002EXT upd9002_repe_scasb(void);
extern UPD9002EXT upd9002_repne_scasb(void);
extern UPD9002EXT upd9002_repe_scasw(void);
extern UPD9002EXT upd9002_repne_scasw(void);
extern UPD9002EXT upd9002_repnc_movsb(void);
extern UPD9002EXT upd9002_repc_movsb(void);
extern UPD9002EXT upd9002_repnc_movsw(void);
extern UPD9002EXT upd9002_repc_movsw(void);
extern UPD9002EXT upd9002_repnc_cmpsb(void);
extern UPD9002EXT upd9002_repc_cmpsb(void);
extern UPD9002EXT upd9002_repnc_cmpsw(void);
extern UPD9002EXT upd9002_repc_cmpsw(void);
extern UPD9002EXT upd9002_repnc_stosb(void);
extern UPD9002EXT upd9002_repc_stosb(void);
extern UPD9002EXT upd9002_repnc_stosw(void);
extern UPD9002EXT upd9002_repc_stosw(void);
extern UPD9002EXT upd9002_repnc_lodsb(void);
extern UPD9002EXT upd9002_repc_lodsb(void);
extern UPD9002EXT upd9002_repnc_lodsw(void);
extern UPD9002EXT upd9002_repc_lodsw(void);
extern UPD9002EXT upd9002_repnc_scasb(void);
extern UPD9002EXT upd9002_repc_scasb(void);
extern UPD9002EXT upd9002_repnc_scasw(void);
extern UPD9002EXT upd9002_repc_scasw(void);
