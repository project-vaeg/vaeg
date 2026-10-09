
enum {
	FDCEVENT_NEUTRAL,
	FDCEVENT_CMDRECV,
	FDCEVENT_DATSEND,
	FDCEVENT_BUFRECV,
	FDCEVENT_BUFSEND,
	FDCEVENT_BUFSEND2,
	FDCEVENT_NEXTDATA,
	FDCEVENT_BUSY,
	FDCEVENT_FIRSTDATA,
	FDCEVENT_STARTBUFSEND2,
	FDCEVENT_FIRSTSTARTBUFSEND2,
	FDCEVENT_STARTBUFRECV,
	FDCEVENT_FIRSTSTARTBUFRECV,
	FDCEVENT_TC,
};

enum {
	LENGTH_GAP0 = 80,
	LENGTH_SYNC = 12,
	LENGTH_IM = 4,
	LENGTH_GAP1 = 50,
	LENGTH_IAM = 4,
	LENGTH_ID = 4,
	LENGTH_CRC = 2,
	LENGTH_GAP2 = 22,
	LENGTH_DAM = 4,

	LENGTH_PRIAMP = LENGTH_GAP0 + LENGTH_SYNC + LENGTH_IM + LENGTH_GAP1,
	LENGTH_SECTOR = LENGTH_SYNC + LENGTH_IAM + LENGTH_ID + LENGTH_CRC + LENGTH_GAP2 + LENGTH_SYNC +
	                LENGTH_DAM + LENGTH_CRC,

	LENGTH_TRACK = 10600 // normal: (+-2%)
};

// real sector size = LENGTH_SECTOR + (128 << n) + gap3

enum {
	FDCRLT_NR = 0x000008,
	FDCRLT_EC = 0x000010,
	FDCRLT_SE = 0x000020,
	FDCRLT_IC0 = 0x000040,
	FDCRLT_IC1 = 0x000080,

	FDCRLT_MA = 0x000100,
	FDCRLT_NW = 0x000200,
	FDCRLT_ND = 0x000400,
	FDCRLT_OR = 0x001000,
	FDCRLT_DE = 0x002000,
	FDCRLT_EN = 0x008000,

	FDCRLT_MD = 0x010000,
	FDCRLT_BC = 0x020000,
	FDCRLT_SN = 0x040000,
	FDCRLT_SH = 0x080000,
	FDCRLT_NC = 0x100000,
	FDCRLT_DD = 0x200000,
	FDCRLT_CM = 0x400000,

	FDCRLT_NT = 0x80000000,
	FDCRLT_AT = 0x00000040,
	FDCRLT_IC = 0x00000080,
	FDCRLT_AI = 0x000000c0,

	FDCSTAT_CB = 0x10,
	FDCSTAT_NDM = 0x20,
	FDCSTAT_DIO = 0x40,
	FDCSTAT_RQM = 0x80
};

enum {
	FDC_TRACKDENSITY_48TPI = 0,
	FDC_TRACKDENSITY_96TPI = 1
};

typedef struct {
	UINT8 equip;
	UINT8 support144;
	UINT8 ctrlfd[4];
	UINT8 trackdensity[4]; // FDC_TRACKDENSITY_48TPI or FDC_TRACKDENSITY_96TPI

	UINT8 us, hd;
	UINT8 mt, mf, sk;
	UINT8 eot, gpl, dtl;
	UINT8 C, H, R, N;
	UINT8 srt, hut, hlt, nd;
	UINT8 stp, ncn, sc, d;

	UINT8 status;
	UINT8 intreq;
	UINT8 lastdata;
	UINT8 dummy;

	UINT8 crcn;
	UINT8 ctrlreg;
	UINT8 chgreg;
	UINT8 reg144;

	UINT32 stat[4];
	UINT8 treg[4];
	UINT8 rpm[4];

	int event;
	int cmdp;
	int cmdcnt;
	int datp;
	int datcnt;
	int bufp;
	int bufcnt;

	UINT8 tcreserved;
	UINT8 rqm;
	SINT32 rqmlastclock;
	SINT32 rqminterval;
	//	int		priampcnt;
	UINT8 reserved[128];
	UINT32 clock; // 動作周波数(Hz)
	UINT8 motor[4];
	UINT8 fddifmode; // 1B0h FDDインタフェースモード
	                 //      0..インテリジェント 1..DMA

	UINT8 cmd;
	BYTE cmds[15];
	BYTE data[16];

	BYTE buf[0x8000];
} _FDC, *FDC;

#ifdef __cplusplus
extern "C" {
#endif

#define CTRL_FDMEDIA fdc.ctrlfd

void fdc_intwait(NEVENTITEM item);
void fdc_timer(NEVENTITEM item);
void fdc_fddmotor(NEVENTITEM item);
void fdc_statewatch(NEVENTITEM item);

void fdc_interrupt(void);

void DMACCALL fdc_datawrite(REG8 data);
REG8 DMACCALL fdc_dataread(void);
REG8 DMACCALL fdc_dmafunc(REG8 func);

void fdcsend_error7(void);
void fdcsend_success7(void);

void fdc_trace_enable(BOOL enable);
void fdc_trace_text(const char *fmt, ...);
void fdc_trace_bytes(const char *prefix, const UINT8 *data, UINT length);
void fdc_trace_iova_unhandled(UINT port);
void fdc_trace_log(REG8 cmd, const char *name, UINT8 drive, UINT8 C, UINT8 H, UINT8 R, UINT8 N,
                   UINT32 req_len, UINT8 st0, UINT8 st1, UINT8 st2, UINT32 xfer_len, UINT8 dma_ch,
                   UINT8 dma_access, UINT8 dma_sysm_bank, UINT8 sysm_bank, UINT32 dma_len,
                   UINT32 dma_start, UINT32 dma_end);

void fdc_reset(void);
void fdc_bind(void);

BYTE fdcsubsys_i_fdc0(void);
BYTE fdcsubsys_i_fdc1(void);
void fdcsubsys_o_fdc1(BYTE dat);
void fdcsubsys_o_mtrctl(BYTE dat);
void fdcsubsys_o_dskctl(BYTE dat);
void fdcsubsys_o_tc(void);

#ifdef __cplusplus
}
#endif
