/*
 * fdsubsys.c: PC-88VA FD Sub System (mock-up type)
 */

#include "compiler.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "iocoreva.h"
#include "memoryva.h"
#include "fddfile.h"

enum {
	// Port-C handshake bits, viewed from the subsystem side.

	// Main CPU to subsystem.
	ATN_MAIN = 0x08, // Command phase; attention asserted.
	DAC_MAIN = 0x04, // Data accepted.
	RFD_MAIN = 0x02, // Ready for data.
	DAV_MAIN = 0x01, // Data valid.

	// Subsystem to main CPU.
	DAC_SUB = 0x40,
	RFD_SUB = 0x20,
	DAV_SUB = 0x10,

	DACBIT = 6,
	RFDBIT = 5,
	DAVBIT = 4,

	// Subsystem handshake states.
	HSST_STOPPED = 0,
	HSST_WAIT_ATN = 1,        // Wait for ATN.
	HSST_WAIT_CMD = 2,        // Wait for DAV and command reception.
	HSST_WAIT_DATA = 3,       // Wait for DAV and data reception.
	HSST_WAIT_DAV_RESET = 4,  // Wait for DAV deassertion.
	HSST_WAIT_RFD = 11,       // Wait for RFD before sending data.
	HSST_WAIT_DAC = 12,       // Wait for DAC.
	HSST_WAIT_DAC_RESET = 13, // Wait for DAC deassertion.

	// Subsystem command-cycle states.
	ST_RECV_CMD = 0,
	ST_RECV_DATA = 1,
	ST_EXEC_CMD = 2,
	ST_SEND_DATA = 3,
	ST_END_CYCLE = 4,

	WAITING = 0,
	GOAHEAD = 1,

	// Subsystem command codes.
	CMD_INITIALIZE = 0x00,
	CMD_WRITE_DATA = 0x01,
	CMD_READ_DATA = 0x02,
	CMD_SEND_DATA = 0x03,
	CMD_SEND_RESULT_STATUS = 0x06,
	CMD_RECEIVE_MEMORY = 0x0c,
	CMD_EXECUTE_COMMAND = 0x0d,
	CMD_LOAD_DATA = 0x0e,
	CMD_SET_SURFACE_MODE = 0x17,
	CMD_SET_DISK_MODE = 0x1f,
	CMD_SEND_DISK_MODE = 0x20,
	CMD_SET_BOUNDARY_MODE = 0x21,
	CMD_DRIVE_READY_CHECK = 0x23,
	CMD_SLEEP = 0x25,
	CMD_ACTIVE = 0x26,

	// Addresses in subsystem work memory.
	WORK_DATA_BUF = 0x4000,          // Read/write data buffer.
	WORK_READ_SECTOR_COUNT = 0x7f08, // Number of sectors completed by the read command.
	WORK_COMMAND_STATUS = 0x7f14,    // Command status returned by command 06H.
	WORK_DISK_MODE = 0x7f44,         // Per-drive disk mode, drive 0 then drive 1.
	WORK_LAST_DISK_MODE = 0x7f4f,    // Disk mode of the most recently accessed drive.
	WORK_DM_N = 0x7f52,              // FDC N field derived from the disk mode.

	// Other constants.
	DATA_BUF_SIZE = 0x2000, // Read/write data-buffer size.
	DRIVES = 2,             // Number of subsystem drives.
};

static const BYTE ntobit[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

static BYTE porta_main;
static BYTE portb_main;
static BYTE portc_main;

// ---- FDD subsystem.

BYTE subsysmem[0x10000]; // 64 KiB subsystem memory.

static int hsstate; // Current handshake state.
static int state;

static BYTE cmd;
static BOOL cmdrecvd; // A complete command has been received.

static int recvdatacnt; // Remaining receive-byte count.
static BYTE *recvbuf;   // Current receive destination.
static int senddatacnt; // Remaining transmit-byte count.
static BYTE *sendbuf;   // Current transmit source.

static BYTE parambuf[8]; // Command parameter and result buffer.

typedef struct {
	UINT8 drive;
	UINT8 C;
	UINT8 H;
	UINT8 R;
	UINT8 N;
	UINT8 st0;
	UINT8 st1;
	UINT8 st2;
	UINT32 req_len;
	UINT32 xfer_len;
	UINT32 range_start;
	UINT32 range_end;
} FDSUBTRACE;

static FDSUBTRACE fdsubtrace;

static void fdsubsys_trace_bytes(const char *dir, const UINT8 *data, UINT length) {
	char prefix[64];

	(void)snprintf(prefix, sizeof(prefix), "fdsubtrace mode=%02x %s", fdc.fddifmode, dir);
	fdc_trace_bytes(prefix, data, length);
}

static UINT fdsubsys_trace_param_len(BYTE command) {
	switch (command) {
	case CMD_INITIALIZE:
	case CMD_SEND_DATA:
	case CMD_SEND_RESULT_STATUS:
	case CMD_SLEEP:
		return 0;
	case CMD_WRITE_DATA:
	case CMD_READ_DATA:
	case CMD_RECEIVE_MEMORY:
		return 4;
	case CMD_EXECUTE_COMMAND:
	case CMD_SET_DISK_MODE:
	case CMD_ACTIVE:
		return 2;
	case CMD_LOAD_DATA:
		return 6;
	case CMD_SET_SURFACE_MODE:
	case CMD_SET_BOUNDARY_MODE:
	case CMD_DRIVE_READY_CHECK:
	case CMD_SEND_DISK_MODE:
		return 1;
	default:
		return 0;
	}
}

static void fdsubsys_trace_main_sequence(void) {
	UINT len;
	UINT i;
	UINT8 bytes[9];

	len = fdsubsys_trace_param_len(cmd);
	bytes[0] = cmd;
	for (i = 0; i < len; i++) {
		bytes[i + 1] = parambuf[i];
	}
	fdsubsys_trace_bytes("main2sub", bytes, len + 1);
}

static void fdsubsys_trace_response_sequence(void) {
	if (senddatacnt > 0) {
		fdsubsys_trace_bytes("sub2main", sendbuf, (UINT)senddatacnt);
	} else {
		fdsubsys_trace_bytes("sub2main", NULL, 0);
	}
}

static void subsys_outportb(REG8 dat) {
	porta_main = dat;
}

static REG8 subsys_inporta(void) {
	return portb_main;
}

static REG8 subsys_inportc(void) {
	return (portc_main << 4) | ((portc_main >> 4) & 0x0f);
}

static void subsys_setportc(int bitnum) {
	if (bitnum >= 4) {
		portc_main |= ntobit[bitnum - 4];
	}
}

static void subsys_resetportc(int bitnum) {
	if (bitnum >= 4) {
		portc_main &= ~ntobit[bitnum - 4];
	}
}

static int subsys_wait_atn(void) {
	if (subsys_inportc() & ATN_MAIN) {
		subsys_setportc(RFDBIT);
		hsstate = HSST_WAIT_CMD;
		return GOAHEAD;
	}
	return WAITING;
}

static int subsys_wait_cmd(void) {
	if (subsys_inportc() & DAV_MAIN) {
		subsys_resetportc(RFDBIT);
		cmd = subsys_inporta();
		cmdrecvd = TRUE;
		subsys_setportc(DACBIT);
		hsstate = HSST_WAIT_DAV_RESET;
		return GOAHEAD;
	}
	return WAITING;
}

static int subsys_wait_data(void) {
	BYTE data;
	if (subsys_inportc() & DAV_MAIN) {
		subsys_resetportc(RFDBIT);
		data = subsys_inporta();
		//		TRACEOUT(("fdsubsys: recv data 0x%02x", data));
		*recvbuf++ = data;
		recvdatacnt--;
		subsys_setportc(DACBIT);
		hsstate = HSST_WAIT_DAV_RESET;
		return GOAHEAD;
	}
	return WAITING;
}

static int subsys_wait_dav_reset(void) {
	if (!(subsys_inportc() & DAV_MAIN)) {
		subsys_resetportc(DACBIT);
		hsstate = HSST_STOPPED;
		return GOAHEAD;
	}
	return WAITING;
}

static int subsys_wait_rfd(void) {
	BYTE data;
	if (subsys_inportc() & RFD_MAIN) {
		data = *sendbuf++;
		senddatacnt--;
		subsys_outportb(data);
		//		TRACEOUT(("fdsubsys: send data 0x%02x", data));
		subsys_setportc(DAVBIT);
		hsstate = HSST_WAIT_DAC;
		return GOAHEAD;
	}
	return WAITING;
}

static int subsys_wait_dac(void) {
	if (subsys_inportc() & DAC_MAIN) {
		subsys_resetportc(DAVBIT);
		hsstate = HSST_WAIT_DAC_RESET;
		return GOAHEAD;
	}
	return WAITING;
}

static int subsys_wait_dac_reset(void) {
	if (!(subsys_inportc() & DAC_MAIN)) {
		hsstate = HSST_STOPPED;
		return GOAHEAD;
	}
	return WAITING;
}

static int subsys_receive_cmd(void) {
	int result;

	result = GOAHEAD;
	while (result == GOAHEAD) {
		switch (hsstate) {
		case HSST_WAIT_ATN:
			result = subsys_wait_atn();
			break;
		case HSST_WAIT_CMD:
			result = subsys_wait_cmd();
			break;
		case HSST_WAIT_DAV_RESET:
			result = subsys_wait_dav_reset();
			break;
		case HSST_STOPPED:
			if (cmdrecvd) {
				return GOAHEAD;
			} else {
				hsstate = HSST_WAIT_ATN;
			}
		}
	}
	return WAITING;
}

static int subsys_receive_data(void) {
	int result;

	result = GOAHEAD;
	while (result == GOAHEAD) {
		switch (hsstate) {
		case HSST_WAIT_DATA:
			result = subsys_wait_data();
			break;
		case HSST_WAIT_DAV_RESET:
			result = subsys_wait_dav_reset();
			break;
		case HSST_STOPPED:
			if (recvdatacnt > 0) {
				subsys_setportc(RFDBIT);
				hsstate = HSST_WAIT_DATA;
			} else {
				// The complete scheduled receive payload has arrived.
				return GOAHEAD;
			}
		}
	}
	return WAITING;
}

static int subsys_send_data(void) {
	int result;

	result = GOAHEAD;
	while (result == GOAHEAD) {
		switch (hsstate) {
		case HSST_WAIT_RFD:
			result = subsys_wait_rfd();
			break;
		case HSST_WAIT_DAC:
			result = subsys_wait_dac();
			break;
		case HSST_WAIT_DAC_RESET:
			result = subsys_wait_dac_reset();
			break;
		case HSST_STOPPED:
			if (senddatacnt > 0) {
				hsstate = HSST_WAIT_RFD;
			} else {
				// The complete scheduled transmit payload has been sent.
				return GOAHEAD;
			}
		}
	}
	return WAITING;
}

/*
Prepare a command after reception and before receiving its parameters.
This primarily selects the command parameter length.
	Output:
		recvdatacnt
		recvbuf
*/
static void subsys_cmd_received(void) {
	recvdatacnt = 0;
	switch (cmd) {
	case CMD_INITIALIZE:
	case CMD_SEND_DATA:
	case CMD_SEND_RESULT_STATUS:
	case CMD_SLEEP:
		break;
	case CMD_WRITE_DATA:
		recvdatacnt = 4;
		recvbuf = parambuf;
		parambuf[5] = 0;
		break;
	case CMD_READ_DATA:
		recvdatacnt = 4;
		recvbuf = parambuf;
		break;
	case CMD_RECEIVE_MEMORY:
		recvdatacnt = 4;
		recvbuf = parambuf;
		parambuf[5] = 0;
		break;
	case CMD_EXECUTE_COMMAND:
		recvdatacnt = 2;
		recvbuf = parambuf;
		break;
	case CMD_SET_DISK_MODE:
		recvdatacnt = 2;
		recvbuf = parambuf;
		break;
	case CMD_LOAD_DATA:
		recvdatacnt = 6;
		recvbuf = parambuf;
		break;
	case CMD_SET_SURFACE_MODE:
	case CMD_SET_BOUNDARY_MODE:
	case CMD_DRIVE_READY_CHECK:
	case CMD_SEND_DISK_MODE:
		recvdatacnt = 1;
		recvbuf = parambuf;
		break;
	case CMD_ACTIVE:
		recvdatacnt = 2;
		recvbuf = parambuf;
		break;
	default:
		break;
	}
}

/*
Disk-control helpers.
*/

static void config_fdc_by_disk_mode(int drv, int track) {
	BYTE mode;

	mode = subsysmem[WORK_DISK_MODE + drv];

	fdc.rpm[drv] = 0; // The subsystem command set does not select 1.44 MB media here.
	switch ((mode >> 4) & 0x03) {
	case 0: // 1D/2D
		CTRL_FDMEDIA[drv] = DISKTYPE_2DD;
		fdc.trackdensity[drv] = FDC_TRACKDENSITY_48TPI;
		break;
	case 1: // 1DD/2DD
		CTRL_FDMEDIA[drv] = DISKTYPE_2DD;
		fdc.trackdensity[drv] = FDC_TRACKDENSITY_96TPI;
		break;
	case 2: // 1HD/2HD
		CTRL_FDMEDIA[drv] = DISKTYPE_2HD;
		fdc.trackdensity[drv] = FDC_TRACKDENSITY_96TPI;
		break;
	}

	if ((mode & 0x38) == 0x28 && track == 0) {
		// In 1HD/2HD mode, SPC remaps logical track zero.
		fdc.mf = 0x00; // FM
	} else {
		fdc.mf = 0x40; // MFM
	}

	fdc.N = mode & 0x03; // Set the FDC sector-size code from the subsystem disk mode.
	subsysmem[WORK_DM_N] = fdc.N;
	subsysmem[WORK_LAST_DISK_MODE] = mode;
}

static void set_command_status(BYTE status) {
	subsysmem[WORK_COMMAND_STATUS] = status;
	fdsubtrace.st0 = status;
	TRACEOUT(("fdsubsys: command_status=0x%02x", status));
}

static const char *fdsubsys_trace_cmdname(BYTE command) {
	switch (command) {
	case CMD_INITIALIZE:
		return ("FDSubInitialize");
	case CMD_WRITE_DATA:
		return ("FDSubWriteData");
	case CMD_READ_DATA:
		return ("FDSubReadData");
	case CMD_SEND_DATA:
		return ("FDSubSendData");
	case CMD_SEND_RESULT_STATUS:
		return ("FDSubSendResultStatus");
	case CMD_RECEIVE_MEMORY:
		return ("FDSubReceiveMemory");
	case CMD_EXECUTE_COMMAND:
		return ("FDSubExecuteCommand");
	case CMD_LOAD_DATA:
		return ("FDSubLoadData");
	case CMD_SET_SURFACE_MODE:
		return ("FDSubSetSurfaceMode");
	case CMD_SET_DISK_MODE:
		return ("FDSubSetDiskMode");
	case CMD_SEND_DISK_MODE:
		return ("FDSubSendDiskMode");
	case CMD_SET_BOUNDARY_MODE:
		return ("FDSubSetBoundaryMode");
	case CMD_DRIVE_READY_CHECK:
		return ("FDSubDriveReadyCheck");
	case CMD_SLEEP:
		return ("FDSubSleep");
	case CMD_ACTIVE:
		return ("FDSubActive");
	default:
		return ("FDSubUnknown");
	}
}

static void fdsubsys_trace_begin(void) {
	ZeroMemory(&fdsubtrace, sizeof(fdsubtrace));
	fdsubtrace.drive = 0xff;
	fdsubtrace.st0 = 0xff;
	fdsubtrace.st1 = 0xff;
	fdsubtrace.st2 = 0xff;
	fdsubtrace.range_start = 0xffffffffUL;
	fdsubtrace.range_end = 0xffffffffUL;
}

static void fdsubsys_trace_set_range(UINT32 start, UINT32 length) {
	fdsubtrace.range_start = start;
	if (length) {
		fdsubtrace.range_end = start + length - 1;
	} else {
		fdsubtrace.range_end = start;
	}
}

static void fdsubsys_trace_set_chrn(int drv, int track, int sector) {
	fdsubtrace.drive = (UINT8)drv;
	fdsubtrace.C = (UINT8)(track >> 1);
	fdsubtrace.H = (UINT8)(track & 1);
	fdsubtrace.R = (UINT8)sector;
	fdsubtrace.N = fdc.N;
}

static void fdsubsys_trace_emit(void) {
	fdc_trace_log(cmd, fdsubsys_trace_cmdname(cmd), fdsubtrace.drive, fdsubtrace.C, fdsubtrace.H,
	              fdsubtrace.R, fdsubtrace.N, fdsubtrace.req_len, fdsubtrace.st0, fdsubtrace.st1,
	              fdsubtrace.st2, fdsubtrace.xfer_len, 0xff, memoryva.dma_access,
	              memoryva.dma_sysm_bank, memoryva.sysm_bank, 0, fdsubtrace.range_start,
	              fdsubtrace.range_end);
}

/*
Execute subsystem commands.
	Output:
		senddatacnt
		sendbuf
*/

/*
00H: initialize.
*/
static void subsys_exec_initialize(void) {
	TRACEOUT(("fdsubsys: initialize command"));
	/* These FFH values make an original VA mis-detect readable 2D media and attempt V1/V2 mode.
	subsysmem[WORK_DISK_MODE + 0] = 0xff;
	subsysmem[WORK_DISK_MODE + 1] = 0xff;
	*/
	subsysmem[WORK_DISK_MODE + 0] = 0x01;
	subsysmem[WORK_DISK_MODE + 1] = 0x01;
}

/*
01H: write data.
*/
static void subsys_exec_write_data(void) {
	if (parambuf[5] == 0) {
		BYTE sectorcnt = parambuf[0];
		BYTE drv = parambuf[1];
		BYTE track = parambuf[2];
		BYTE sector = parambuf[3];

		TRACEOUT(
		    ("fdsubsys: write data (not implemented): sectorcnt=%d, drv=%d, track=%d, sector=%d",
		     sectorcnt, drv, track, sector));
		fdsubsys_trace_set_chrn(drv, track, sector);
		fdsubtrace.req_len = 256UL * sectorcnt;

		recvdatacnt = 256 * sectorcnt;
		if (recvdatacnt) {
			recvbuf = &subsysmem[WORK_DATA_BUF];
			state = ST_RECV_DATA;
			parambuf[5] = 1; // Mark the payload receive phase as started.
		}
	} else {
	}
}

/*
02H: read sectors from disk.
*/
static void subsys_exec_read_data(void) {
	int drv;
	int sectorcnt;
	int original_sectorcnt;
	int track;
	int sector;
	int readbufaddr;
	int sectorsize;
	UINT32 totalbytes;

	sectorcnt = parambuf[0];
	original_sectorcnt = sectorcnt;
	drv = parambuf[1];
	track = parambuf[2];
	sector = parambuf[3];
	fdsubsys_trace_set_chrn(drv, track, sector);

	TRACEOUT(("fdsubsys: read_data: drv=%d, sectorcnt=%d, track=%d, sector=%d", drv, sectorcnt,
	          track, sector));

	/*
	fdc
		us: drive number, starting at zero.
		ctrlfd (CTRL_FDMEDIA) DISKTYPE_2HD or DISKTYPE_2DD
		treg: current cylinder; not required by this path.
		hd: physical head number.
		C,H,R,N
		mf		0:fm 0x40:mfm 0xff:??
		rpm		?	0:1.2  1:1.44

		ncn: target seek cylinder.

	
	
	*/
	if (drv >= DRIVES)
		goto failed;

	fdc.us = drv;
	config_fdc_by_disk_mode(drv, track);
	fdsubsys_trace_set_chrn(drv, track, sector);
	sectorsize = 128 << subsysmem[WORK_DM_N];
	totalbytes = (UINT32)original_sectorcnt * sectorsize;
	fdsubtrace.req_len = totalbytes;
	fdsubsys_trace_set_range(WORK_DATA_BUF, totalbytes);

	fdc.ncn = track >> 1;
	fdc.hd = track & 1;
	if (fdd_seek())
		goto failed;

	subsysmem[WORK_READ_SECTOR_COUNT] = 0;
	readbufaddr = WORK_DATA_BUF;
	while (sectorcnt > 0) {
		fdc.C = track >> 1;
		fdc.H = track & 1;
		fdc.R = sector;
		// config_fdc_by_disk_mode() has already selected fdc.N.
		fdc.hd = track & 1;

		if (fdd_read())
			goto failed;

		CopyMemory(&subsysmem[readbufaddr], fdc.buf, sectorsize);
		fdsubtrace.xfer_len += sectorsize;

		sectorcnt--;
		sector++;
		subsysmem[WORK_READ_SECTOR_COUNT]++;
		readbufaddr += sectorsize;
		if (readbufaddr >= WORK_DATA_BUF + DATA_BUF_SIZE)
			goto failed;
	}
	set_command_status(0x40);
	return;

failed:
	set_command_status(0x01);
	return;
}

/*
03H: return data from the read buffer.
*/
static void subsys_exec_send_data(void) {
	senddatacnt = (128 << subsysmem[WORK_DM_N]) * subsysmem[WORK_READ_SECTOR_COUNT];
	sendbuf = &subsysmem[WORK_DATA_BUF];
	fdsubtrace.req_len = senddatacnt;
	fdsubtrace.xfer_len = senddatacnt;
	fdsubsys_trace_set_range(WORK_DATA_BUF, (UINT32)senddatacnt);
	TRACEOUT(("fdsubsys: send data: count=%d", senddatacnt));
}

/*
06H: return command status.
*/
static void subsys_exec_send_result_status(void) {
	parambuf[0] = subsysmem[WORK_COMMAND_STATUS];
	fdsubtrace.st0 = parambuf[0];

	senddatacnt = 1;
	sendbuf = parambuf;

	TRACEOUT(("fdsubsys: send result status: return 0x%02x", parambuf[0]));
}

/*
0CH: transfer data into subsystem memory.
*/
static void subsys_exec_receive_memory(void) {
	if (parambuf[5] == 0) {
		// Payload reception has not started.
		WORD addr = (parambuf[0] << 8) | parambuf[1];
		recvdatacnt = (parambuf[2] << 8) | parambuf[3];
		recvbuf = &subsysmem[addr];
		fdsubtrace.req_len = recvdatacnt;
		fdsubsys_trace_set_range(addr, (UINT32)recvdatacnt);
		state = ST_RECV_DATA;
		parambuf[5] = 1; // Mark the payload receive phase as started.

		TRACEOUT(("fdsubsys: receive_memory: addr=0x%04x, bytes=%d", addr, recvdatacnt));
	} else {
		// Payload reception is complete.
	}

	/*
	The VA2 bootstrap writes 01H or 09H to 7F4BH, the read/write retry-count byte.
	The current model stores it without interpreting the value.
	*/
}

/*
0DH: execute command.
*/
static void subsys_exec_execute_command(void) {
	WORD addr = (parambuf[0] << 8) | parambuf[1];

	TRACEOUT(("fdsubsys: execute command (not implemented): address=0x%02x", addr));
}

/*
0EH: load data.
*/
static void subsys_exec_load_data(void) {
	BYTE sectorcnt = parambuf[0];
	BYTE drv = parambuf[1];
	BYTE track = parambuf[2];
	BYTE sector = parambuf[3];
	WORD addr = (parambuf[4] << 8) | parambuf[5];

	TRACEOUT((
	    "fdsubsys: load data (not implemented): sectorcount=%d, drv=%d, track=%d, sector=%d, address=0x%04x",
	    sectorcnt, drv, track, sector, addr));
	fdsubsys_trace_set_chrn(drv, track, sector);
	fdsubtrace.req_len = 256UL * sectorcnt;
	fdsubsys_trace_set_range(addr, fdsubtrace.req_len);
}

/*
17H: set surface mode.
*/
static void subsys_exec_set_surface_mode(void) {
	TRACEOUT(("fdsubsys: set surface mode: mode=0x%02x", parambuf[0]));
}

/*
1FH: set disk mode.
*/
static void subsys_exec_set_disk_mode(void) {
	int drv;
	BYTE mode;

	drv = parambuf[0];
	mode = parambuf[1];
	fdsubtrace.drive = (UINT8)drv;

	TRACEOUT(("fdsubsys: set_disk_mode: drive=%d, mode=0x%02x", drv, mode));

	if (drv < DRIVES) {
		subsysmem[WORK_DISK_MODE + drv] = mode;
	}
}

/*
20H: return disk mode.
*/
static void subsys_exec_send_disk_mode(void) {
	int drv;
	BYTE mode = 0xff;

	drv = parambuf[0];
	fdsubtrace.drive = (UINT8)drv;

	if (drv < DRIVES) {
		mode = subsysmem[WORK_DISK_MODE + drv];
	}

	TRACEOUT(("fdsubsys: send_disk_mode: drive=%d, mode=0x%02x", drv, mode));

	parambuf[0] = mode;
	senddatacnt = 1;
	sendbuf = parambuf;
}

/*
21H: set boundary mode.
*/
static void subsys_exec_set_boundary_mode(void) {
	TRACEOUT(("fdsubsys: set boundary mode: mode=%d", parambuf[0]));
}

/*
23H: check drive-ready state.
*/
static void subsys_exec_drive_ready_check(void) {
	REG8 drv;

	drv = parambuf[0];
	fdsubtrace.drive = drv;

	if (fdd_diskready(drv)) {
		parambuf[0] = 0x00;
	} else {
		parambuf[0] = 0xff; // No disk is inserted.
	}

	TRACEOUT(("fdsubsys: drive_ready_check: drive=%d, return=%d", drv, parambuf[0]));

	senddatacnt = 1;
	sendbuf = parambuf;
}

/*
25H: sleep.
*/
static void subsys_exec_sleep(void) {
	TRACEOUT(("fdsubsys: sleep: return 0, 0"));
	parambuf[0] = 0; // Value visible at main port 1B2H / subsystem port F4H.
	parambuf[1] = 0; // Motor state: 00H off, FFH on.

	senddatacnt = 2;
	sendbuf = parambuf;
}

/*
26H: activate.
*/
static void subsys_exec_active(void) {
	TRACEOUT(("fdsubsys: active: port1b2h=0x%0x, moter=%d", parambuf[0], parambuf[1]));
}

/*
	Output:
		state
		senddatacnt
		sendbuf
*/
static void subsys_exec_cmd(void) {
	fdsubsys_trace_begin();
	state = ST_SEND_DATA;
	senddatacnt = 0;

	switch (cmd) {
	case CMD_INITIALIZE:
		subsys_exec_initialize();
		break;
	case CMD_WRITE_DATA:
		subsys_exec_write_data();
		break;
	case CMD_READ_DATA:
		subsys_exec_read_data();
		break;
	case CMD_SEND_DATA:
		subsys_exec_send_data();
		break;
	case CMD_SEND_RESULT_STATUS:
		subsys_exec_send_result_status();
		break;
	case CMD_RECEIVE_MEMORY:
		subsys_exec_receive_memory();
		break;
	case CMD_EXECUTE_COMMAND:
		subsys_exec_execute_command();
		break;
	case CMD_LOAD_DATA:
		subsys_exec_load_data();
		break;
	case CMD_SET_SURFACE_MODE:
		subsys_exec_set_surface_mode();
		break;
	case CMD_SET_DISK_MODE:
		subsys_exec_set_disk_mode();
		break;
	case CMD_SEND_DISK_MODE:
		subsys_exec_send_disk_mode();
		break;
	case CMD_SET_BOUNDARY_MODE:
		subsys_exec_set_boundary_mode();
		break;
	case CMD_DRIVE_READY_CHECK:
		subsys_exec_drive_ready_check();
		break;
	case CMD_SLEEP:
		subsys_exec_sleep();
		break;
	case CMD_ACTIVE:
		subsys_exec_active();
		break;
	}
	if (state != ST_RECV_DATA) {
		fdsubsys_trace_response_sequence();
		fdsubsys_trace_emit();
	}
}

static void subsys_exec(void) {
	int result;

	result = GOAHEAD;
	while (result == GOAHEAD) {
		switch (state) {
		case ST_RECV_CMD:
			result = subsys_receive_cmd();
			if (result == GOAHEAD) {
				TRACEOUT(("fdsubsys: recv cmd 0x%02x", cmd));
				subsys_cmd_received();
				state = ST_RECV_DATA;
			}
			break;
		case ST_RECV_DATA:
			result = subsys_receive_data();
			if (result == GOAHEAD) {
				fdsubsys_trace_main_sequence();
				state = ST_EXEC_CMD;
			}
			break;
		case ST_EXEC_CMD:
			subsys_exec_cmd();
			break;
		case ST_SEND_DATA:
			result = subsys_send_data();
			if (result == GOAHEAD) {
				state = ST_END_CYCLE;
			}
			break;
		case ST_END_CYCLE:
			state = ST_RECV_CMD;
			cmdrecvd = FALSE;
			break;
		}
	}
}

static void subsys_reset(void) {
	state = ST_RECV_CMD;
	hsstate = HSST_STOPPED;
	cmdrecvd = FALSE;
	subsys_resetportc(DAVBIT);
	subsys_resetportc(RFDBIT);
	subsys_resetportc(DACBIT);
}

// ---- I/O

static void IOOUTCALL fdsubsys_o0fd(UINT port, REG8 dat) {
	portb_main = dat;
	subsys_exec();
	(void)port;
}

static void IOOUTCALL fdsubsys_o0fe(UINT port, REG8 dat) {
	portc_main = (portc_main & 0x0f) | (dat & 0xf0);
	subsys_exec();
	(void)port;
}

static void IOOUTCALL fdsubsys_o0ff(UINT port, REG8 dat) {
	if (dat & 0x80) {
		// 8255 mode-set command.
		// The subsystem handshake uses its fixed mode; ignore this write.
	} else {
		int bitnum;

		bitnum = (dat >> 1) & 0x07;
		if (bitnum >= 4) {
			if (dat & 1) {
				// Set the selected port-C bit.
				portc_main |= ntobit[bitnum];
			} else {
				// Clear the selected port-C bit.
				portc_main &= ~ntobit[bitnum];
			}
			subsys_exec();
		}
	}
	(void)port;
}

static REG8 IOINPCALL fdsubsys_i0fc(UINT port) {
	(void)port;
	return porta_main;
}

static REG8 IOINPCALL fdsubsys_i0fe(UINT port) {
	(void)port;
	return portc_main;
}

// ---- I/F

void fdsubsys_reset(void) {
	subsys_reset();
	fdsubsys_o0fe(0, 0);
}

void fdsubsys_bind(void) {
	iocore_attachout(0x0fd, fdsubsys_o0fd);
	iocore_attachout(0x0fe, fdsubsys_o0fe);
	iocore_attachout(0x0ff, fdsubsys_o0ff);

	iocore_attachinp(0x0fc, fdsubsys_i0fc);
	iocore_attachinp(0x0fe, fdsubsys_i0fe);
}
