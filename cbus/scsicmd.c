#include "compiler.h"

#include "dosio.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "cbuscore.h"
#include "scsiio.h"
#include "scsiio.tbl"
#include "scsicmd.h"
#include "sxsi.h"

#if defined(_WIN32) && defined(TRACE)
#define SCSICMD_ERR                                                                                \
	MessageBox(NULL, "SCSI error", "?", MB_OK);                                                    \
	exit(1);
#else
#define SCSICMD_ERR
#endif

typedef struct {
	UINT phase;
	REG8 service_status;
	BOOL host_to_spc;
} SCSIPHASECONTRACT;

/* One table owns the target phase and the WD33C93 service-request code. */
static const SCSIPHASECONTRACT scsi_phase_contract[] = {
    {SCSIPH_DATAOUT, 0x88, TRUE}, {SCSIPH_DATAIN, 0x89, FALSE}, {SCSIPH_COMMAND, 0x8a, TRUE},
    {SCSIPH_STATUS, 0x8b, FALSE}, {SCSIPH_INFOOUT, 0x8c, TRUE}, {SCSIPH_INFOIN, 0x8d, FALSE},
    {SCSIPH_MSGOUT, 0x8e, TRUE},  {SCSIPH_MSGIN, 0x8f, FALSE}};

REG8 scsicmd_phase_service_status(UINT phase) {
	UINT i;

	for (i = 0; i < (UINT)(sizeof(scsi_phase_contract) / sizeof(scsi_phase_contract[0])); i++) {
		if (scsi_phase_contract[i].phase == phase) {
			return scsi_phase_contract[i].service_status;
		}
	}
	return 0x42;
}

REG8 scsicmd_phase_unexpected_status(UINT phase) {
	/* 48h-4Fh reports an information-phase change before TC expires. */
	return (REG8)(0x48 | (phase & 7));
}

BOOL scsicmd_phase_host_to_spc(UINT phase) {
	UINT i;

	for (i = 0; i < (UINT)(sizeof(scsi_phase_contract) / sizeof(scsi_phase_contract[0])); i++) {
		if (scsi_phase_contract[i].phase == phase) {
			return scsi_phase_contract[i].host_to_spc;
		}
	}
	return FALSE;
}

static const BYTE hdd_inquiry[36] = {0x00, 0x00, 0x02, 0x02, 0x1f, 0x00, 0x00, 0x18, 'N',
                                     'E',  'C',  0x20, 0x20, 0x20, 0x20, 0x20, 'N',  'P',
                                     '2',  '-',  'H',  'D',  'D',  0x20, 0x20, 0x20, 0x20,
                                     0x20, 0x20, 0x20, 0x20, 0x20, '1',  '.',  '0',  '0'};

static const BYTE hdd_inquiry_unsupported_lun[36] = {
    0x7f, 0x00, 0x02, 0x02, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static BYTE hdd_sense[18] = {0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static BOOL scsicmd_check_condition;
static REG8 scsicmd_last_request_sense_key;
static REG8 scsicmd_last_request_sense_asc;
static REG8 scsicmd_last_request_sense_ascq;

static void scsicmd_putbe32(BYTE *dst, UINT32 value) {
	dst[0] = (BYTE)(value >> 24);
	dst[1] = (BYTE)(value >> 16);
	dst[2] = (BYTE)(value >> 8);
	dst[3] = (BYTE)value;
}

static void scsicmd_putbe24(BYTE *dst, UINT32 value) {
	dst[0] = (BYTE)(value >> 16);
	dst[1] = (BYTE)(value >> 8);
	dst[2] = (BYTE)value;
}

static UINT scsicmd_cdb_lun(const BYTE *cdb) {
	return cdb ? ((cdb[1] >> 5) & 7) : 0;
}

static UINT scsicmd_target_lun(void) {
	return scsiio.reg[SCSICTR_TARGETLUN] & 7;
}

static UINT scsicmd_backend_index(UINT id) {
	return 2 + (id & 7);
}

/* The SCSI target backend has one logical unit per target ID: LUN 0. */
static BOOL scsicmd_lun_supported(const BYTE *cdb) {
	return (scsicmd_target_lun() == 0) && (cdb == NULL || scsicmd_cdb_lun(cdb) == 0);
}

static UINT scsicmd_cdb_length(const BYTE *cdb) {
	if (cdb == NULL) {
		return 0;
	}
	switch (cdb[0] >> 5) {
	case 0:
		return 6;
	case 1:
	case 2:
		return 10;
	default:
		return 12;
	}
}

static void scsicmd_trace_cdb_result(UINT id, const BYTE *cdb, SXSIDEV sxsi, REG8 status) {
	REG8 inquiry_byte0;
	REG8 sense_key;
	REG8 asc;
	REG8 ascq;
	UINT selected_index;

	sense_key = (cdb && cdb[0] == 0x03) ? scsicmd_last_request_sense_key : hdd_sense[2];
	asc = (cdb && cdb[0] == 0x03) ? scsicmd_last_request_sense_asc : hdd_sense[12];
	ascq = (cdb && cdb[0] == 0x03) ? scsicmd_last_request_sense_ascq : hdd_sense[13];
	inquiry_byte0 = (cdb && cdb[0] == 0x12 && scsiio.cmdpos) ? scsiio.data[0] : 0xff;
	selected_index = (sxsi && scsicmd_lun_supported(cdb)) ? scsicmd_backend_index(id) : 0xff;
	scsiio_trace_cdb_result(id, scsicmd_target_lun(), scsicmd_cdb_lun(cdb), selected_index, cdb,
	                        scsicmd_cdb_length(cdb), inquiry_byte0, scsiio.cmdpos, status,
	                        sense_key, asc, ascq);
	scsiio_trace_census_command(
	    id, scsicmd_target_lun(), scsicmd_cdb_lun(cdb), cdb, scsicmd_cdb_length(cdb), 0, 0,
	    scsiio.cmdpos,
	    (cdb[0] == 0x03 || cdb[0] == 0x12 || cdb[0] == 0x1a || cdb[0] == 0x25) ? "IN" : "none", 0,
	    scsiio.cmdpos, 0, status, sense_key, asc, ascq, "none",
	    (status == 0x02) && (sense_key == 0x05) && (asc == 0x20));
}

static BOOL scsicmd_geometry_valid(SXSIDEV sxsi) {
	UINT64 expected;

	if ((sxsi == NULL) || (sxsi->totals <= 0) || (sxsi->cylinders == 0) || (sxsi->surfaces == 0) ||
	    (sxsi->sectors == 0) || (sxsi->size == 0)) {
		return FALSE;
	}
	expected = (UINT64)sxsi->cylinders * sxsi->surfaces * sxsi->sectors;
	return expected == (UINT64)sxsi->totals;
}

static void scsicmd_set_sense(BYTE key, BYTE asc, BYTE ascq) {
	hdd_sense[2] = key;
	hdd_sense[12] = asc;
	hdd_sense[13] = ascq;
}

typedef enum {
	SCSIBLOCK_NONE = 0,
	SCSIBLOCK_READ,
	SCSIBLOCK_WRITE
} SCSIBLOCKKIND;

typedef struct {
	SCSIBLOCKKIND kind;
	REG8 id;
	REG8 opcode;
	SXSIDEV sxsi;
	UINT32 lba;
	UINT32 remaining_blocks;
	UINT32 chunk_blocks;
	UINT32 chunk_bytes;
	UINT32 total_blocks;
	UINT32 start_lba;
	UINT32 transferred_bytes;
	UINT32 backend_blocks;
	UINT32 commit_count;
	UINT32 chunk_index;
	BYTE cdb[12];
	UINT cdb_length;
	BOOL active;
	REG8 backend_status;
} SCSIBLOCKTRANSFER;

static SCSIBLOCKTRANSFER scsi_block;
static UINT scsi_block_sequence;
static void scsicmd_trace_block_complete(REG8 opcode, REG8 status);

void scsicmd_block_reset_state(void) {
	ZeroMemory(&scsi_block, sizeof(scsi_block));
	scsi_block.kind = SCSIBLOCK_NONE;
}

static BOOL scsicmd_block_decode(const BYTE *cdb, UINT32 *lba, UINT32 *blocks) {
	UINT32 count;

	if ((cdb == NULL) || (lba == NULL) || (blocks == NULL)) {
		return FALSE;
	}
	if (cdb[0] == 0x08 || cdb[0] == 0x0a) {
		*lba = ((UINT32)(cdb[1] & 0x1f) << 16) | ((UINT32)cdb[2] << 8) | cdb[3];
		count = cdb[4];
		*blocks = count ? count : 256;
		return TRUE;
	}
	if (cdb[0] == 0x28 || cdb[0] == 0x2a) {
		*lba = ((UINT32)cdb[2] << 24) | ((UINT32)cdb[3] << 16) | ((UINT32)cdb[4] << 8) | cdb[5];
		/* Unlike READ/WRITE(6), zero is a successful zero-block command. */
		*blocks = ((UINT32)cdb[7] << 8) | cdb[8];
		return TRUE;
	}
	return FALSE;
}

static BOOL scsicmd_block_range_valid(SXSIDEV sxsi, UINT32 lba, UINT32 blocks) {
	UINT64 total;

	if ((sxsi == NULL) || (sxsi->totals <= 0)) {
		return FALSE;
	}
	total = (UINT64)(UINT32)sxsi->totals;
	return ((UINT64)lba < total) && ((UINT64)blocks <= total - lba);
}

static UINT32 scsicmd_block_capacity(SXSIDEV sxsi) {
	UINT64 capacity;

	if ((sxsi == NULL) || (sxsi->size == 0)) {
		return 0;
	}
	capacity = sizeof(scsiio.data) / sxsi->size;
	return capacity > 0xffffffffU ? 0xffffffffU : (UINT32)capacity;
}

static BOOL scsicmd_block_prepare_read(void) {
	UINT32 blocks;
	REG8 ret;

	if (!scsi_block.active || (scsi_block.kind != SCSIBLOCK_READ) ||
	    scsi_block.remaining_blocks == 0) {
		return FALSE;
	}
	blocks = min(scsi_block.remaining_blocks, scsicmd_block_capacity(scsi_block.sxsi));
	if ((blocks == 0) || ((UINT64)blocks * scsi_block.sxsi->size > sizeof(scsiio.data))) {
		return FALSE;
	}
	scsi_block.chunk_blocks = blocks;
	scsi_block.chunk_bytes = blocks * scsi_block.sxsi->size;
	scsiio_trace_block_chunk(scsi_block_sequence, scsi_block.chunk_index++, scsi_block.lba, blocks,
	                         scsi_block.transferred_bytes, scsi_block.chunk_bytes);
	ret = sxsi_read((REG8)(0x20 + scsi_block.id), (long)scsi_block.lba, scsiio.data,
	                scsi_block.chunk_bytes);
	if (ret != 0) {
		scsi_block.backend_status = ret;
		scsicmd_set_sense(0x03, 0x11, 0x00);
		scsicmd_check_condition = TRUE;
		scsi_block.active = FALSE;
		return FALSE;
	}
	scsiio_trace_block_backend_data(scsiio.data, scsi_block.chunk_bytes);
	scsiio_trace_block_staging_data(scsiio.data, scsi_block.chunk_bytes);
	ZeroMemory(scsiio.data + scsi_block.chunk_bytes, sizeof(scsiio.data) - scsi_block.chunk_bytes);
	scsi_block.remaining_blocks -= blocks;
	scsi_block.lba += blocks;
	scsi_block.backend_blocks += blocks;
	scsi_block.transferred_bytes += scsi_block.chunk_bytes;
	scsiio.cmdpos = scsi_block.chunk_bytes;
	scsiio.rddatpos = 0;
	return TRUE;
}

static BOOL scsicmd_block_prepare_write(void) {
	UINT32 blocks;

	if (!scsi_block.active || (scsi_block.kind != SCSIBLOCK_WRITE) ||
	    scsi_block.remaining_blocks == 0) {
		return FALSE;
	}
	blocks = min(scsi_block.remaining_blocks, scsicmd_block_capacity(scsi_block.sxsi));
	if ((blocks == 0) || ((UINT64)blocks * scsi_block.sxsi->size > sizeof(scsiio.data))) {
		return FALSE;
	}
	scsi_block.chunk_blocks = blocks;
	scsi_block.chunk_bytes = blocks * scsi_block.sxsi->size;
	scsiio_trace_block_chunk(scsi_block_sequence, scsi_block.chunk_index++, scsi_block.lba, blocks,
	                         scsi_block.transferred_bytes, scsi_block.chunk_bytes);
	ZeroMemory(scsiio.data, scsi_block.chunk_bytes);
	scsiio.cmdpos = scsi_block.chunk_bytes;
	scsiio.wrdatpos = 0;
	return TRUE;
}

static BOOL scsicmd_block_commit_write(void) {
	REG8 ret;

	if (!scsi_block.active || (scsi_block.kind != SCSIBLOCK_WRITE) ||
	    (scsi_block.chunk_blocks == 0) || (scsiio.wrdatpos != scsi_block.chunk_bytes)) {
		return FALSE;
	}
	ret = sxsi_write((REG8)(0x20 + scsi_block.id), (long)(scsi_block.lba), scsiio.data,
	                 scsi_block.chunk_bytes);
	if (ret != 0) {
		scsi_block.backend_status = ret;
		if (ret == 0x70) {
			scsicmd_set_sense(0x07, 0x27, 0x00);
		} else {
			scsicmd_set_sense(0x03, 0x0c, 0x02);
		}
		scsicmd_check_condition = TRUE;
		scsi_block.active = FALSE;
		return FALSE;
	}
	scsi_block.lba += scsi_block.chunk_blocks;
	scsi_block.remaining_blocks -= scsi_block.chunk_blocks;
	scsi_block.backend_blocks += scsi_block.chunk_blocks;
	scsi_block.transferred_bytes += scsi_block.chunk_bytes;
	scsi_block.commit_count++;
	return TRUE;
}

/* Called after a completed PIO DATA OUT chunk. */
static REG8 scsicmd_block_dataout_complete(void) {
	if (!scsicmd_block_commit_write()) {
		scsiio.reg[SCSICTR_STATUS] = 0x02;
		scsiio.data[0] = 0x02;
		scsiio.cmdpos = 1;
		scsiio.rddatpos = 0;
		scsiio.phase = SCSIPH_STATUS;
		scsicmd_trace_block_complete(scsi_block.opcode, 0x02);
		return scsicmd_phase_service_status(SCSIPH_STATUS);
	}
	if (scsi_block.remaining_blocks != 0) {
		if (!scsicmd_block_prepare_write()) {
			scsicmd_set_sense(0x03, 0x0c, 0x02);
			scsicmd_check_condition = TRUE;
			scsi_block.active = FALSE;
			scsiio.reg[SCSICTR_STATUS] = 0x02;
			scsiio.data[0] = 0x02;
			scsiio.cmdpos = 1;
			scsiio.rddatpos = 0;
			scsiio.phase = SCSIPH_STATUS;
			scsicmd_trace_block_complete(scsi_block.opcode, 0x02);
			return scsicmd_phase_service_status(SCSIPH_STATUS);
		}
		return scsicmd_phase_service_status(SCSIPH_DATAOUT);
	}
	scsi_block.active = FALSE;
	scsiio.reg[SCSICTR_STATUS] = scsicmd_check_condition ? 0x02 : 0x00;
	scsicmd_trace_block_complete(scsi_block.opcode, scsiio.reg[SCSICTR_STATUS]);
	return scsicmd_phase_service_status(SCSIPH_STATUS);
}

static REG8 scsicmd_block_datain_complete(void) {
	if ((scsi_block.opcode != 0x08) && (scsi_block.opcode != 0x28)) {
		return scsicmd_phase_service_status(SCSIPH_STATUS);
	}
	if (scsi_block.active && (scsi_block.kind == SCSIBLOCK_READ) &&
	    scsi_block.remaining_blocks != 0) {
		if (!scsicmd_block_prepare_read()) {
			return scsicmd_phase_service_status(SCSIPH_STATUS);
		}
		return scsicmd_phase_service_status(SCSIPH_DATAIN);
	}
	scsi_block.active = FALSE;
	scsiio.reg[SCSICTR_STATUS] = scsicmd_check_condition ? 0x02 : 0x00;
	scsicmd_trace_block_complete(scsi_block.opcode, scsiio.reg[SCSICTR_STATUS]);
	return scsicmd_phase_service_status(SCSIPH_STATUS);
}

BOOL scsicmd_block_data_available(void) {
	return scsi_block.active && (scsi_block.kind == SCSIBLOCK_READ) &&
	       scsi_block.remaining_blocks != 0;
}

BOOL scsicmd_block_dataout_ready(void) {
	return scsi_block.active && (scsi_block.kind == SCSIBLOCK_WRITE) &&
	       scsiio.wrdatpos >= scsiio.cmdpos;
}

static void scsicmd_trace_block_start(REG8 id, const BYTE *cdb, UINT32 lba, UINT32 blocks,
                                      UINT32 bytes, SXSIDEV sxsi) {
	UINT32 cdb_transfer_length;

	if ((cdb[0] == 0x08) || (cdb[0] == 0x0a)) {
		cdb_transfer_length = cdb[4];
	} else {
		cdb_transfer_length = ((UINT32)cdb[7] << 8) | cdb[8];
	}
	CopyMemory(scsi_block.cdb, cdb, sizeof(scsi_block.cdb));
	scsi_block.cdb_length = scsicmd_cdb_length(cdb);
	scsiio_trace_block_start(++scsi_block_sequence, id, scsicmd_target_lun(), scsicmd_cdb_lun(cdb),
	                         cdb, lba, blocks, sxsi ? sxsi->size : 0, bytes,
	                         scsicmd_backend_index(id),
	                         sxsi ? ((sxsi->type & SXSITYPE_DEVMASK) == SXSITYPE_CDROM) : FALSE);
	scsiio_trace_block_program(scsi_block_sequence, cdb[0], cdb_transfer_length, blocks, bytes,
	                           scsiio.reg[SCSICTR_TRANSCNT + 0], scsiio.reg[SCSICTR_TRANSCNT + 1],
	                           scsiio.reg[SCSICTR_TRANSCNT + 2], scsiio_transfer_count());
}

static void scsicmd_trace_block_complete(REG8 opcode, REG8 status) {
	UINT64 residual;
	UINT32 residual_bytes;

	residual = (UINT64)scsi_block.remaining_blocks * (scsi_block.sxsi ? scsi_block.sxsi->size : 0);
	if (residual > 0xffffffffU) {
		residual_bytes = 0xffffffffU;
	} else {
		residual_bytes = (UINT32)residual;
	}
	scsiio_trace_block_complete(scsi_block_sequence, opcode, scsi_block.transferred_bytes,
	                            residual_bytes, scsi_block.backend_blocks,
	                            scsi_block.backend_status, status, hdd_sense[2], hdd_sense[12],
	                            hdd_sense[13], scsi_block.commit_count);
	scsiio_trace_census_command(
	    scsi_block.id, scsicmd_target_lun(), scsicmd_cdb_lun(scsi_block.cdb), scsi_block.cdb,
	    scsi_block.cdb_length, scsi_block.start_lba, scsi_block.total_blocks,
	    scsi_block.total_blocks * (scsi_block.sxsi ? scsi_block.sxsi->size : 0),
	    (scsi_block.kind == SCSIBLOCK_READ) ? "IN" : "OUT", scsi_block.backend_status,
	    scsi_block.transferred_bytes, residual_bytes, status, hdd_sense[2], hdd_sense[12],
	    hdd_sense[13], "AR19", FALSE);
}

static REG8 scsicmd_block_status_phase(REG8 id, REG8 status) {
	REG8 service_status;

	scsiio.reg[SCSICTR_STATUS] = status;
	scsiio.data[0] = status;
	scsiio.cmdpos = 1;
	scsiio.rddatpos = 0;
	scsiio.phase = SCSIPH_STATUS;
	service_status = scsicmd_phase_service_status(SCSIPH_STATUS);
	scsicmd_trace_block_complete(scsi_block.opcode, status);
	return service_status;
}

static REG8 scsicmd_block_start(REG8 id, SXSIDEV sxsi, BYTE *cdb) {
	UINT32 lba;
	UINT32 blocks;
	UINT32 byte_count;
	UINT64 total_bytes;
	BOOL is_read;

	if (!scsicmd_block_decode(cdb, &lba, &blocks)) {
		scsicmd_set_sense(0x05, 0x20, 0x00);
		return scsicmd_block_status_phase(id, 0x02);
	}
	scsicmd_block_reset_state();
	scsi_block.id = id;
	scsi_block.opcode = cdb[0];
	scsi_block.sxsi = sxsi;
	scsi_block.lba = lba;
	scsi_block.start_lba = lba;
	scsi_block.remaining_blocks = blocks;
	scsi_block.total_blocks = blocks;
	scsi_block.chunk_index = 0;
	scsi_block.active = FALSE;
	scsi_block.backend_status = 0;
	is_read = (cdb[0] == 0x08 || cdb[0] == 0x28);
	total_bytes = (UINT64)blocks * sxsi->size;
	if (total_bytes > 0xffffffffU || total_bytes > 0xffffffU) {
		scsicmd_set_sense(0x05, 0x24, 0x00);
		scsicmd_trace_block_start(id, cdb, lba, blocks, 0, sxsi);
		return scsicmd_block_status_phase(id, 0x02);
	}
	byte_count = (UINT32)total_bytes;
	scsicmd_trace_block_start(id, cdb, lba, blocks, byte_count, sxsi);
	/* READ/WRITE(10) zero length is a successful no-data command. */
	if ((cdb[0] == 0x28 || cdb[0] == 0x2a) && blocks == 0) {
		if ((UINT64)lba > (UINT64)(UINT32)sxsi->totals) {
			scsicmd_set_sense(0x05, 0x21, 0x00);
			scsicmd_check_condition = TRUE;
			return scsicmd_block_status_phase(id, 0x02);
		}
		return scsicmd_block_status_phase(id, 0x00);
	}
	if (!scsicmd_block_range_valid(sxsi, lba, blocks)) {
		scsicmd_set_sense(0x05, 0x21, 0x00);
		scsicmd_check_condition = TRUE;
		return scsicmd_block_status_phase(id, 0x02);
	}
	scsi_block.active = TRUE;
	scsi_block.kind = is_read ? SCSIBLOCK_READ : SCSIBLOCK_WRITE;
	if (is_read) {
		if (!scsicmd_block_prepare_read()) {
			return scsicmd_block_status_phase(id, 0x02);
		}
		scsiio.phase = SCSIPH_DATAIN;
		scsiio.rddatpos = 0;
		return scsicmd_phase_service_status(SCSIPH_DATAIN);
	}
	if ((sxsi->type & SXSITYPE_DEVMASK) == SXSITYPE_CDROM) {
		scsicmd_set_sense(0x07, 0x27, 0x00);
		scsi_block.active = FALSE;
		scsicmd_check_condition = TRUE;
		return scsicmd_block_status_phase(id, 0x02);
	}
	if (!scsicmd_block_prepare_write()) {
		scsicmd_set_sense(0x03, 0x0c, 0x02);
		scsicmd_check_condition = TRUE;
		return scsicmd_block_status_phase(id, 0x02);
	}
	scsiio.phase = SCSIPH_DATAOUT;
	scsiio.wrdatpos = 0;
	return scsicmd_phase_service_status(SCSIPH_DATAOUT);
}

static REG8 scsicmd_direct_block_transfer(REG8 id, SXSIDEV sxsi, BYTE *cdb, UINT transfer_bytes) {
	UINT32 lba;
	UINT32 blocks;
	UINT32 byte_count;
	UINT64 total_bytes;
	BOOL is_read;
	REG8 ret;

	scsicmd_check_condition = FALSE;
	if (!scsicmd_block_decode(cdb, &lba, &blocks)) {
		scsicmd_set_sense(0x05, 0x20, 0x00);
		scsiio.reg[SCSICTR_STATUS] = 0x02;
		scsicmd_check_condition = TRUE;
		return 0x16;
	}
	scsicmd_block_reset_state();
	scsi_block.id = id;
	scsi_block.opcode = cdb[0];
	scsi_block.sxsi = sxsi;
	scsi_block.lba = lba;
	scsi_block.start_lba = lba;
	scsi_block.remaining_blocks = blocks;
	scsi_block.total_blocks = blocks;
	scsi_block.kind = (cdb[0] == 0x08 || cdb[0] == 0x28) ? SCSIBLOCK_READ : SCSIBLOCK_WRITE;
	is_read = (scsi_block.kind == SCSIBLOCK_READ);
	total_bytes = (UINT64)blocks * sxsi->size;
	byte_count = ((total_bytes <= 0xffffffffU) && (total_bytes <= 0xffffffU) &&
	              (total_bytes <= sizeof(scsiio.data)))
	                 ? (UINT32)total_bytes
	                 : 0;
	scsicmd_trace_block_start(id, cdb, lba, blocks, byte_count, sxsi);
	if ((byte_count == 0 && blocks != 0) || (transfer_bytes != byte_count)) {
		scsicmd_set_sense(0x05, 0x24, 0x00);
		goto fail;
	}
	/* READ/WRITE(10) zero length is a successful no-data command. */
	if ((blocks == 0) && ((cdb[0] == 0x28) || (cdb[0] == 0x2a))) {
		if ((UINT64)lba > (UINT64)(UINT32)sxsi->totals) {
			scsicmd_set_sense(0x05, 0x21, 0x00);
			goto fail;
		}
		scsiio.reg[SCSICTR_STATUS] = 0x00;
		scsicmd_trace_block_complete(cdb[0], 0x00);
		return 0x16;
	}
	if (!scsicmd_block_range_valid(sxsi, lba, blocks)) {
		scsicmd_set_sense(0x05, 0x21, 0x00);
		goto fail;
	}
	if (!is_read && ((sxsi->type & SXSITYPE_DEVMASK) == SXSITYPE_CDROM)) {
		scsicmd_set_sense(0x07, 0x27, 0x00);
		goto fail;
	}
	scsi_block.active = TRUE;
	scsi_block.chunk_blocks = blocks;
	scsi_block.chunk_bytes = byte_count;
	scsiio_trace_block_chunk(scsi_block_sequence, 0, lba, blocks, 0, byte_count);
	if (is_read) {
		ret = sxsi_read((REG8)(0x20 + id), (long)lba, scsiio.data, byte_count);
		if (ret != 0) {
			scsi_block.backend_status = ret;
			scsicmd_set_sense(0x03, 0x11, 0x00);
			goto fail;
		}
		scsiio_trace_block_backend_data(scsiio.data, byte_count);
		scsiio_trace_block_staging_data(scsiio.data, byte_count);
		scsi_block.backend_blocks = blocks;
		scsi_block.transferred_bytes = byte_count;
		scsi_block.remaining_blocks = 0;
		scsiio.cmdpos = byte_count;
		scsiio.rddatpos = 0;
		scsi_block.active = TRUE;
		scsiio.phase = SCSIPH_DATAIN;
		scsiio.reg[SCSICTR_STATUS] = 0x00;
		return 0x16;
	}
	/* Select-and-transfer WRITE exposes the command-completion result before
	 * the guest drains its DATA OUT window.  Keep the command active until
	 * every byte has arrived; committing scsiio.data here would commit stale
	 * data from the preceding command. */
	scsiio.phase = SCSIPH_DATAOUT;
	scsiio.cmdpos = byte_count;
	scsiio.wrdatpos = 0;
	scsiio.rddatpos = 0;
	scsi_block.active = TRUE;
	scsiio.reg[SCSICTR_STATUS] = 0x00;
	return 0x16;

fail:
	scsiio.reg[SCSICTR_STATUS] = 0x02;
	scsicmd_check_condition = TRUE;
	scsi_block.active = FALSE;
	scsicmd_trace_block_complete(cdb[0], 0x02);
	return 0x16;
}

static UINT scsicmd_datain(SXSIDEV sxsi, BYTE *cdb) {
	UINT length;
	UINT copylen;
	UINT32 last_lba;
	UINT page;
	UINT page_offset;
	UINT response_length;
	UINT geometry_offset;
	BOOL dbd;

	ZeroMemory(scsiio.data, sizeof(scsiio.data));
	scsiio.cmdpos = 0;
	if (cdb[0] != 0x03) {
		scsicmd_check_condition = FALSE;
	}
	if (!scsicmd_lun_supported(cdb) && cdb[0] != 0x03 && cdb[0] != 0x12) {
		/* Unsupported LUNs never reach the mounted LUN-0 backend. */
		scsicmd_set_sense(0x05, 0x25, 0x00);
		scsicmd_check_condition = TRUE;
		return 0;
	}
	switch (cdb[0]) {
	case 0x03: // Request Sense
		TRACEOUT(("Request Sense"));
		scsicmd_last_request_sense_key = hdd_sense[2];
		scsicmd_last_request_sense_asc = hdd_sense[12];
		scsicmd_last_request_sense_ascq = hdd_sense[13];
		length = cdb[4];
		copylen = min(length, (UINT)sizeof(hdd_sense));
		if (copylen) {
			CopyMemory(scsiio.data, hdd_sense, copylen);
		}
		scsiio.cmdpos = copylen;
		scsicmd_set_sense(0x00, 0x00, 0x00);
		scsicmd_check_condition = FALSE;
		break;

	case 0x12: // Inquiry
	{
		const BYTE *inquiry;

		TRACEOUT(("Inquiry LUN=%u", scsicmd_cdb_lun(cdb)));
		/* Unsupported LUN inquiry is GOOD with peripheral qualifier 011b. */
		inquiry = scsicmd_lun_supported(cdb) ? hdd_inquiry : hdd_inquiry_unsupported_lun;
		length = cdb[4];
		copylen = min(length, 36U);
		if (copylen) {
			CopyMemory(scsiio.data, inquiry, copylen);
		}
		scsiio.cmdpos = copylen;
	} break;

	case 0x25: // Read Capacity (10)
		TRACEOUT(("Read Capacity"));
		last_lba = (sxsi->totals > 0) ? (UINT32)(sxsi->totals - 1) : 0;
		scsicmd_putbe32(scsiio.data + 0, last_lba);
		scsicmd_putbe32(scsiio.data + 4, sxsi->size);
		scsiio.cmdpos = 8;
		break;

	case 0x1a: // Mode Sense (6)
		TRACEOUT(("Mode Sense (6)"));
		page = cdb[2] & 0x3f;
		if ((page != 0x00) && (page != 0x04) && (page != 0x3f)) {
			/* Invalid page code: CHECK CONDITION / ILLEGAL REQUEST. */
			scsicmd_set_sense(0x05, 0x24, 0x00);
			scsicmd_check_condition = TRUE;
			break;
		}
		if (!scsicmd_geometry_valid(sxsi)) {
			/* A contradictory image geometry is not a usable target. */
			scsicmd_set_sense(0x05, 0x24, 0x00);
			scsicmd_check_condition = TRUE;
			break;
		}
		/*
			 * Page 04h is the rigid-disk geometry page.  Page 00h is
			 * the supported empty page, and page 3fh returns both pages.
			 * The DBD bit selects the optional descriptor.
			 */
		dbd = (cdb[1] & 0x08) != 0;
		page_offset = dbd ? 4 : 12;
		geometry_offset = page_offset;
		if (page == 0x3f) {
			scsiio.data[page_offset + 0] = 0x00;
			scsiio.data[page_offset + 1] = 0x00;
			geometry_offset += 2;
		}
		response_length = geometry_offset + ((page == 0x00) ? 2 : 24);
		length = cdb[4];
		scsiio.data[1] = 0x00;
		scsiio.data[2] = 0x00;
		scsiio.data[3] = dbd ? 0 : 8;
		if (!dbd) {
			scsiio.data[4] = 0;
			scsicmd_putbe24(scsiio.data + 5, (UINT32)sxsi->totals);
			scsicmd_putbe24(scsiio.data + 9, (UINT32)sxsi->size);
		}
		if (page != 0x00) {
			scsiio.data[geometry_offset + 0] = 0x04;
			scsiio.data[geometry_offset + 1] = 0x16;
			scsicmd_putbe24(scsiio.data + geometry_offset + 2, (UINT32)sxsi->cylinders);
			scsiio.data[geometry_offset + 5] = sxsi->surfaces;
			/* Write-precomp, reduced-current, step-rate, RPL and
				 * rotational fields remain zero for the emulated disk. */
			scsicmd_putbe24(scsiio.data + geometry_offset + 14, (UINT32)sxsi->cylinders);
		}
		copylen = min(length, response_length);
		scsiio.data[0] = (BYTE)(response_length - 1);
		scsiio.cmdpos = copylen;
		break;

	default:
		scsicmd_set_sense(0x05, 0x20, 0x00);
		scsicmd_check_condition = TRUE;
		break;
	}
	return (scsiio.cmdpos);
}

// ----

REG8 scsicmd_negate(REG8 id) {
	scsiio.phase = 0;
	(void)id;
	return (0x85); // disconnect
}

REG8 scsicmd_select(REG8 id) {
	SXSIDEV sxsi;
	REG8 status;
	UINT selected_index;

	TRACEOUT(("scsicmd_select"));
	sxsi = sxsi_getptr((REG8)(0x20 + id));
	selected_index = (sxsi && sxsi->type) ? scsicmd_backend_index(id) : 0xff;
	if (scsicmd_target_lun() != 0) {
		TRACEOUT(("LUN = %d", scsicmd_target_lun()));
		status = 0x42;
	} else if ((sxsi) && (sxsi->type)) {
		scsiio.phase = SCSIPH_COMMAND;
		status = 0x8a;
	} else {
		status = 0x42;
	}
	scsiio_trace_target_selection(id, scsicmd_target_lun(), selected_index, status);
	return status;
}

REG8 scsicmd_transfer(REG8 id, BYTE *cdb) {
	SXSIDEV sxsi;
	REG8 status;

	if (cdb == NULL) {
		return 0x42;
	}
	sxsi = sxsi_getptr((REG8)(0x20 + id));
	if ((sxsi == NULL) || (sxsi->type == 0)) {
		return 0x42;
	}

	TRACEOUT(("sel ope code = %.2x lun=%u", cdb[0], scsicmd_cdb_lun(cdb)));
	switch (cdb[0]) {
	case 0x00: // Test Unit Ready
		if (!scsicmd_lun_supported(cdb)) {
			scsicmd_set_sense(0x05, 0x25, 0x00);
			scsiio.reg[SCSICTR_STATUS] = 0x02;
		} else {
			scsiio.reg[SCSICTR_STATUS] = 0x00;
		}
		status = 0x16;
		scsicmd_trace_cdb_result(id, cdb, scsicmd_lun_supported(cdb) ? sxsi : NULL,
		                         scsiio.reg[SCSICTR_STATUS]);
		return status;

	case 0x03: // Request Sense
	case 0x12: // Inquiry
	case 0x1a: // Mode Sense (6)
	case 0x25: // Read Capacity (10)
		scsicmd_datain(sxsi, cdb);
		scsiio.reg[SCSICTR_STATUS] = scsicmd_check_condition ? 0x02 : 0x00;
		scsicmd_trace_cdb_result(id, cdb, (scsicmd_lun_supported(cdb) ? sxsi : NULL),
		                         scsiio.reg[SCSICTR_STATUS]);
		return status = 0x16;
	case 0x08:
	case 0x0a:
	case 0x28:
	case 0x2a:
		return scsicmd_direct_block_transfer(id, sxsi, cdb, scsiio_transfer_count());
	}

	scsicmd_set_sense(0x05, 0x20, 0x00);
	scsiio.reg[SCSICTR_STATUS] = 0x02;
	scsicmd_trace_cdb_result(id, cdb, NULL, scsiio.reg[SCSICTR_STATUS]);
	return 0x16;
}

BOOL scsicmd_direct_data_available(void) {
	return (scsi_block.active && (scsi_block.kind == SCSIBLOCK_READ) &&
	        (scsiio.phase == SCSIPH_DATAIN) && (scsiio.rddatpos < scsiio.cmdpos));
}

void scsicmd_direct_data_complete(void) {
	if ((scsi_block.kind == SCSIBLOCK_READ) && (scsiio.reg[SCSICTR_STATUS] == 0x00)) {
		scsi_block.active = FALSE;
		scsicmd_trace_block_complete(scsi_block.opcode, 0x00);
	}
}

BOOL scsicmd_direct_dataout_available(void) {
	return (scsi_block.active && (scsi_block.kind == SCSIBLOCK_WRITE) &&
	        (scsiio.phase == SCSIPH_DATAOUT) && (scsiio.wrdatpos < scsiio.cmdpos));
}

void scsicmd_direct_dataout_complete(void) {
	REG8 ret;

	if (!scsi_block.active || (scsi_block.kind != SCSIBLOCK_WRITE) ||
	    (scsiio.phase != SCSIPH_DATAOUT) || (scsiio.wrdatpos < scsiio.cmdpos)) {
		return;
	}
	ret = sxsi_write((REG8)(0x20 + scsi_block.id), (long)scsi_block.start_lba, scsiio.data,
	                 scsi_block.chunk_bytes);
	if (ret != 0) {
		scsi_block.backend_status = ret;
		scsicmd_set_sense((ret == 0x70) ? 0x07 : 0x03, (ret == 0x70) ? 0x27 : 0x0c,
		                  (ret == 0x70) ? 0x00 : 0x02);
		scsiio.reg[SCSICTR_STATUS] = 0x02;
		scsi_block.active = FALSE;
		scsicmd_trace_block_complete(scsi_block.opcode, 0x02);
		return;
	}
	scsiio_trace_block_backend_data(scsiio.data, scsi_block.chunk_bytes);
	scsi_block.backend_blocks = scsi_block.total_blocks;
	scsi_block.transferred_bytes = scsi_block.chunk_bytes;
	scsi_block.remaining_blocks = 0;
	scsi_block.commit_count = 1;
	scsi_block.active = FALSE;
	scsiio.reg[SCSICTR_STATUS] = 0x00;
	scsicmd_trace_block_complete(scsi_block.opcode, 0x00);
}

REG8 scsicmd_command(REG8 id) {
	SXSIDEV sxsi;
	REG8 service_status;
	BOOL lun_supported;
	SXSIDEV trace_sxsi;

	TRACEOUT(("scsicmd_cmd = %.2x lun=%u", scsiio.cmd[0], scsicmd_cdb_lun(scsiio.cmd)));
	sxsi = sxsi_getptr((REG8)(0x20 + id));
	if ((sxsi == NULL) || (sxsi->type == 0)) {
		return 0x42;
	}
	lun_supported = scsicmd_lun_supported(scsiio.cmd);
	scsiio.reg[SCSICTR_STATUS] = 0x00;
	if (scsiio.cmd[0] != 0x03) {
		scsicmd_check_condition = FALSE;
	}
	scsicmd_block_reset_state();

	/* Unsupported LUN INQUIRY is GOOD and is represented by byte 0=7fh. */
	if (!lun_supported && scsiio.cmd[0] != 0x03 && scsiio.cmd[0] != 0x12) {
		scsicmd_set_sense(0x05, 0x25, 0x00);
		scsiio.reg[SCSICTR_STATUS] = 0x02;
		scsiio.data[0] = 0x02;
		scsiio.cmdpos = 1;
		scsiio.rddatpos = 0;
		scsiio.phase = SCSIPH_STATUS;
		service_status = scsicmd_phase_service_status(SCSIPH_STATUS);
		scsicmd_trace_cdb_result(id, scsiio.cmd, NULL, scsiio.reg[SCSICTR_STATUS]);
		return service_status;
	}

	switch (scsiio.cmd[0]) {
	case 0x00:
		scsiio.phase = SCSIPH_STATUS;
		service_status = scsicmd_phase_service_status(SCSIPH_STATUS);
		scsicmd_trace_cdb_result(id, scsiio.cmd, sxsi, scsiio.reg[SCSICTR_STATUS]);
		return service_status;

	case 0x03: // Request Sense
	case 0x12: // Inquiry
	case 0x1a: // Mode Sense (6)
	case 0x25: // Read Capacity (10)
		scsicmd_datain(sxsi, scsiio.cmd);
		if (!scsicmd_check_condition) {
			scsiio.phase = SCSIPH_DATAIN;
			scsiio.rddatpos = 0;
			service_status = scsicmd_phase_service_status(SCSIPH_DATAIN);
			trace_sxsi = lun_supported ? sxsi : NULL;
			scsicmd_trace_cdb_result(id, scsiio.cmd, trace_sxsi, scsiio.reg[SCSICTR_STATUS]);
			return service_status;
		}
		/* Data-command validation failures complete in STATUS. */
		scsiio.reg[SCSICTR_STATUS] = 0x02;
		scsiio.data[0] = 0x02;
		scsiio.cmdpos = 1;
		scsiio.rddatpos = 0;
		scsiio.phase = SCSIPH_STATUS;
		service_status = scsicmd_phase_service_status(SCSIPH_STATUS);
		scsicmd_trace_cdb_result(id, scsiio.cmd, NULL, scsiio.reg[SCSICTR_STATUS]);
		return service_status;

	case 0x08: // Read (6)
	case 0x0a: // Write (6)
	case 0x28: // Read (10)
	case 0x2a: // Write (10)
		return scsicmd_block_start(id, sxsi, scsiio.cmd);
	}

	scsicmd_set_sense(0x05, 0x20, 0x00);
	scsiio.reg[SCSICTR_STATUS] = 0x02;
	scsiio.data[0] = 0x02;
	scsiio.cmdpos = 1;
	scsiio.rddatpos = 0;
	scsiio.phase = SCSIPH_STATUS;
	service_status = scsicmd_phase_service_status(SCSIPH_STATUS);
	scsicmd_trace_cdb_result(id, scsiio.cmd, NULL, scsiio.reg[SCSICTR_STATUS]);
	return service_status;
}

BOOL scsicmd_backend_selftest(void) {
	_SXSIDEV saved_slots[SCSIHDD_MAX];
	SXSIDEV slot;
	BYTE saved_reg[0x30];
	BYTE saved_cmd[12];
	BYTE saved_data[36];
	BYTE saved_sense[sizeof(hdd_sense)];
	char saved_test_fname[MAX_PATH];
	UINT saved_phase;
	UINT saved_cmdpos;
	UINT saved_rddatpos;
	UINT saved_port;
	REG8 saved_scsistatus;
	BOOL saved_check_condition;
	BYTE cdb[12];
	BYTE expected[36];
	UINT id;
	UINT visible;
	UINT media_operations;
	long saved_remclock;
	REG8 service;
	BOOL ok = TRUE;
	static BYTE test_media[512 * 256];
	BYTE readback[256];
	BYTE legacy_readback[256];
	BYTE readback_multi[512];
	FILEH test_fh;
	const char *test_path = "m75-scsi-block-selftest.raw";
	UINT test_i;

#define SCMD_SELFTEST_CHECK(name, expression)                                                      \
	do {                                                                                           \
		if (!(expression)) {                                                                       \
			fprintf(stderr, "selftest: %s FAIL\n", name);                                          \
			ok = FALSE;                                                                            \
		} else {                                                                                   \
			fprintf(stderr, "selftest: %s PASS\n", name);                                          \
		}                                                                                          \
	} while (0)
#define SCMD_SELFTEST_COMMAND(bytes)                                                               \
	do {                                                                                           \
		CopyMemory(scsiio.cmd, (bytes), sizeof(scsiio.cmd));                                       \
		scsiio.reg[SCSICTR_TARGETLUN] = 0;                                                         \
		scsiio.cmdpos = 0;                                                                         \
		scsiio.rddatpos = 0;                                                                       \
		service = scsicmd_command(0);                                                              \
	} while (0)

	for (id = 0; id < SCSIHDD_MAX; id++) {
		slot = sxsi_getptr((REG8)(0x20 + id));
		saved_slots[id] = *slot;
		ZeroMemory(slot, sizeof(*slot));
		slot->fh = FILEH_INVALID;
	}
	CopyMemory(saved_reg, scsiio.reg, sizeof(saved_reg));
	CopyMemory(saved_cmd, scsiio.cmd, sizeof(saved_cmd));
	CopyMemory(saved_data, scsiio.data, sizeof(saved_data));
	CopyMemory(saved_sense, hdd_sense, sizeof(saved_sense));
	saved_phase = scsiio.phase;
	saved_cmdpos = scsiio.cmdpos;
	saved_rddatpos = scsiio.rddatpos;
	saved_port = scsiio.port;
	saved_scsistatus = scsiio.scsistatus;
	saved_check_condition = scsicmd_check_condition;

	slot = sxsi_getptr(0x20);
	slot->totals = 640L * 32L * 8L;
	slot->cylinders = 640;
	slot->size = 256;
	slot->sectors = 32;
	slot->surfaces = 8;
	slot->type = SXSITYPE_SCSI | SXSITYPE_HDD;

	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x12;
	cdb[4] = 36;
	SCMD_SELFTEST_COMMAND(cdb);
	ZeroMemory(expected, sizeof(expected));
	expected[0] = 0x00;
	expected[2] = 0x02;
	expected[3] = 0x02;
	expected[4] = 0x1f;
	expected[7] = 0x18;
	CopyMemory(expected + 8, "NEC     ", 8);
	CopyMemory(expected + 16, "NP2-HDD         ", 16);
	CopyMemory(expected + 32, "1.00", 4);
	SCMD_SELFTEST_CHECK("inquiry_lun0_returns_36_bytes",
	                    service == scsicmd_phase_service_status(SCSIPH_DATAIN) &&
	                        scsiio.cmdpos == 36);
	SCMD_SELFTEST_CHECK("inquiry_lun0_additional_length_is_1f", scsiio.data[4] == 0x1f);
	SCMD_SELFTEST_CHECK("inquiry_lun0_vendor_product_revision_offsets",
	                    memcmp(scsiio.data, expected, sizeof(expected)) == 0);

	cdb[4] = 4;
	SCMD_SELFTEST_COMMAND(cdb);
	SCMD_SELFTEST_CHECK("inquiry_allocation_length_truncates_response",
	                    service == scsicmd_phase_service_status(SCSIPH_DATAIN) &&
	                        scsiio.cmdpos == 4 && memcmp(scsiio.data, expected, 4) == 0);

	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x12;
	cdb[1] = 0x20;
	cdb[4] = 36;
	SCMD_SELFTEST_COMMAND(cdb);
	SCMD_SELFTEST_CHECK("inquiry_unsupported_lun_returns_7f",
	                    service == scsicmd_phase_service_status(SCSIPH_DATAIN) &&
	                        scsiio.cmdpos == 36 && scsiio.data[0] == 0x7f &&
	                        scsiio.data[8] == 0x00);
	SCMD_SELFTEST_CHECK("inquiry_unsupported_lun_does_not_alias_lun0",
	                    scsiio.data[0] != expected[0] && scsiio.data[8] != expected[8]);

	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x00;
	cdb[1] = 0x20;
	SCMD_SELFTEST_COMMAND(cdb);
	SCMD_SELFTEST_CHECK("tur_unsupported_lun_returns_check_condition",
	                    service == scsicmd_phase_service_status(SCSIPH_STATUS) &&
	                        scsiio.reg[SCSICTR_STATUS] == 0x02 && scsiio.phase == SCSIPH_STATUS);
	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x03;
	cdb[1] = 0x20;
	cdb[4] = sizeof(hdd_sense);
	SCMD_SELFTEST_COMMAND(cdb);
	SCMD_SELFTEST_CHECK("request_sense_unsupported_lun_returns_05_25_00",
	                    service == scsicmd_phase_service_status(SCSIPH_DATAIN) &&
	                        scsiio.data[2] == 0x05 && scsiio.data[12] == 0x25 &&
	                        scsiio.data[13] == 0x00);

	media_operations = 0;
	saved_remclock = CPU_REMCLOCK;
	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x25;
	cdb[1] = 0x20;
	SCMD_SELFTEST_COMMAND(cdb);
	media_operations = (UINT)(saved_remclock - CPU_REMCLOCK);
	SCMD_SELFTEST_CHECK("read_capacity_unsupported_lun_does_not_access_lun0",
	                    media_operations == 0 && scsiio.reg[SCSICTR_STATUS] == 0x02);
	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x1a;
	cdb[1] = 0x20;
	cdb[2] = 0x04;
	cdb[4] = 36;
	SCMD_SELFTEST_COMMAND(cdb);
	media_operations = (UINT)(saved_remclock - CPU_REMCLOCK);
	SCMD_SELFTEST_CHECK("mode_sense_unsupported_lun_does_not_access_lun0",
	                    media_operations == 0 && scsiio.reg[SCSICTR_STATUS] == 0x02);
	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x28;
	cdb[1] = 0x20;
	cdb[7] = 0;
	cdb[8] = 1;
	SCMD_SELFTEST_COMMAND(cdb);
	SCMD_SELFTEST_CHECK("unsupported_lun_read_does_not_access_lun0",
	                    service == scsicmd_phase_service_status(SCSIPH_STATUS) &&
	                        hdd_sense[2] == 0x05 && hdd_sense[12] == 0x25);
	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x2a;
	cdb[1] = 0x20;
	cdb[7] = 0;
	cdb[8] = 1;
	SCMD_SELFTEST_COMMAND(cdb);
	SCMD_SELFTEST_CHECK("unsupported_lun_write_does_not_access_lun0",
	                    service == scsicmd_phase_service_status(SCSIPH_STATUS) &&
	                        hdd_sense[2] == 0x05 && hdd_sense[12] == 0x25);

	/* Select-and-transfer uses the target-LUN register as well as CDB LUN.
	 * An unsupported LUN must still complete with the protocol-level result,
	 * not be turned into a selection timeout. */
	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x12;
	cdb[4] = 36;
	scsiio.reg[SCSICTR_TARGETLUN] = 1;
	service = scsicmd_transfer(0, cdb);
	SCMD_SELFTEST_CHECK("select_transfer_unsupported_lun_inquiry_is_good",
	                    service == 0x16 && scsiio.reg[SCSICTR_STATUS] == 0x00 &&
	                        scsiio.data[0] == 0x7f && scsiio.cmdpos == 36);
	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x00;
	service = scsicmd_transfer(0, cdb);
	SCMD_SELFTEST_CHECK("select_transfer_unsupported_lun_tur_checks",
	                    service == 0x16 && scsiio.reg[SCSICTR_STATUS] == 0x02 &&
	                        hdd_sense[2] == 0x05 && hdd_sense[12] == 0x25 && hdd_sense[13] == 0x00);

	scsiio.reg[SCSICTR_TARGETLUN] = 0;
	visible = 0;
	for (id = 0; id < 8; id++) {
		if (scsicmd_select((REG8)id) == 0x8a) {
			visible++;
		}
	}
	SCMD_SELFTEST_CHECK("one_configured_target_enumerates_one_disk", visible == 1);
	SCMD_SELFTEST_CHECK("absent_target_id_times_out",
	                    scsicmd_select(1) == 0x42 && scsicmd_select(2) == 0x42);
	SCMD_SELFTEST_CHECK("different_luns_do_not_share_device_identity",
	                    scsiio.reg[SCSICTR_TARGETLUN] == 0);

	/* Exercise the production SXSIDEV-backed block path with a disposable
	 * raw image.  The target slot uses a zero header so sxsi_read/write are
	 * tested without inventing a second image format. */
	for (test_i = 0; test_i < sizeof(test_media); test_i++) {
		test_media[test_i] = (BYTE)((test_i / 256 + test_i) & 0xff);
	}
	test_fh = file_create(test_path);
	SCMD_SELFTEST_CHECK("block_selftest_media_create", test_fh != FILEH_INVALID);
	if (test_fh != FILEH_INVALID) {
		SCMD_SELFTEST_CHECK("block_selftest_media_seed",
		                    file_write(test_fh, test_media, sizeof(test_media)) ==
		                        sizeof(test_media));
		file_close(test_fh);
	}
	file_cpyname(slot->fname, test_path, sizeof(slot->fname));
	file_cpyname(saved_test_fname, slot->fname, sizeof(saved_test_fname));
	slot->fh = FILEH_INVALID;
	slot->headersize = 0;
	slot->totals = 512;
	slot->cylinders = 2;
	slot->size = 256;
	slot->sectors = 32;
	slot->surfaces = 8;
	slot->type = SXSITYPE_SCSI | SXSITYPE_HDD;

	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x28;
	cdb[7] = 0;
	cdb[8] = 1;
	SCMD_SELFTEST_COMMAND(cdb);
	SCMD_SELFTEST_CHECK("read10_one_block_lba0",
	                    service == scsicmd_phase_service_status(SCSIPH_DATAIN) &&
	                        scsiio.cmdpos == 256 && scsiio.data[0] == test_media[0]);
	SCMD_SELFTEST_CHECK("read10_one_block_backend_count",
	                    scsi_block.backend_blocks == 1 && scsi_block.transferred_bytes == 256);

	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x28;
	cdb[5] = 3;
	cdb[7] = 0;
	cdb[8] = 2;
	SCMD_SELFTEST_COMMAND(cdb);
	SCMD_SELFTEST_CHECK("read10_multi_block_big_endian_lba",
	                    service == scsicmd_phase_service_status(SCSIPH_DATAIN) &&
	                        scsiio.cmdpos == 512 && scsiio.data[0] == test_media[3 * 256] &&
	                        scsiio.data[256] == test_media[4 * 256]);

	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x28;
	cdb[5] = 0; /* zero-block LBA may equal the end */
	cdb[7] = 0;
	cdb[8] = 0;
	SCMD_SELFTEST_COMMAND(cdb);
	SCMD_SELFTEST_CHECK("read10_zero_blocks_is_good_without_data",
	                    service == scsicmd_phase_service_status(SCSIPH_STATUS) &&
	                        scsiio.reg[SCSICTR_STATUS] == 0 && scsi_block.backend_blocks == 0);

	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x28;
	cdb[4] = 1;
	cdb[5] = 0xff;
	cdb[7] = 0;
	cdb[8] = 1;
	SCMD_SELFTEST_COMMAND(cdb);
	SCMD_SELFTEST_CHECK("read10_last_valid_block",
	                    service == scsicmd_phase_service_status(SCSIPH_DATAIN) &&
	                        scsiio.cmdpos == 256);

	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x28;
	cdb[4] = 2;
	cdb[5] = 0;
	cdb[7] = 0;
	cdb[8] = 1;
	SCMD_SELFTEST_COMMAND(cdb);
	SCMD_SELFTEST_CHECK("read10_out_of_range_returns_05_21_00",
	                    service == scsicmd_phase_service_status(SCSIPH_STATUS) &&
	                        scsiio.reg[SCSICTR_STATUS] == 2 && hdd_sense[2] == 5 &&
	                        hdd_sense[12] == 0x21 && hdd_sense[13] == 0);

	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x2a;
	cdb[5] = 5;
	cdb[7] = 0;
	cdb[8] = 1;
	SCMD_SELFTEST_COMMAND(cdb);
	for (test_i = 0; test_i < 256; test_i++) {
		scsiio.data[test_i] = (BYTE)(0xa0 + test_i);
	}
	scsiio.wrdatpos = scsiio.cmdpos - 1;
	service = scsicmd_transinfo(0);
	SCMD_SELFTEST_CHECK("write_does_not_commit_before_complete_data_out",
	                    scsi_block.commit_count == 0 &&
	                        service == scsicmd_phase_service_status(SCSIPH_DATAOUT));
	SCMD_SELFTEST_CHECK("short_data_out_does_not_commit_as_complete",
	                    service == scsicmd_phase_service_status(SCSIPH_DATAOUT) &&
	                        scsi_block.commit_count == 0);
	SCMD_SELFTEST_CHECK("aborted_write_does_not_report_good",
	                    service == scsicmd_phase_service_status(SCSIPH_DATAOUT) &&
	                        scsi_block.commit_count == 0);

	SCMD_SELFTEST_COMMAND(cdb);
	for (test_i = 0; test_i < 256; test_i++) {
		scsiio.data[test_i] = (BYTE)(0xa0 + test_i);
	}
	scsiio.wrdatpos = scsiio.cmdpos;
	service = scsicmd_transinfo(0);
	SCMD_SELFTEST_CHECK("write10_one_block_persists",
	                    service == scsicmd_phase_service_status(SCSIPH_STATUS) &&
	                        scsi_block.commit_count == 1 &&
	                        sxsi_read(0x20, 5, readback, sizeof(readback)) == 0 &&
	                        readback[0] == 0xa0 && readback[255] == 0x9f);
	SCMD_SELFTEST_CHECK("write_backend_commit_occurs_once", scsi_block.commit_count == 1);

	/* Exercise the compatibility 0CC6h DATA OUT path.  It must feed the
	 * same block completion routine instead of fabricating STATUS. */
	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x2a;
	cdb[5] = 7;
	cdb[7] = 0;
	cdb[8] = 1;
	SCMD_SELFTEST_COMMAND(cdb);
	for (test_i = 0; test_i < 256; test_i++) {
		scsiio_legacy_dataout_selftest_byte((BYTE)(0x50 + test_i));
	}
	SCMD_SELFTEST_CHECK("legacy_0cc6_write_reaches_backend_commit",
	                    scsi_block.commit_count == 1 && scsiio.reg[SCSICTR_STATUS] == 0x00 &&
	                        sxsi_read(0x20, 7, legacy_readback, sizeof(legacy_readback)) == 0 &&
	                        legacy_readback[0] == 0x50 && legacy_readback[255] == 0x4f);
	SCMD_SELFTEST_CHECK("legacy_0cc6_does_not_directly_complete_status",
	                    scsiio.phase == SCSIPH_STATUS && scsi_block.commit_count == 1);

	/* A backend failure must remain CHECK CONDITION through STATUS. */
	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x2a;
	cdb[5] = 9;
	cdb[7] = 0;
	cdb[8] = 1;
	SCMD_SELFTEST_COMMAND(cdb);
	for (test_i = 0; test_i < 256; test_i++) {
		scsiio.data[test_i] = (BYTE)(0x70 + test_i);
	}
	if (slot->fh != FILEH_INVALID) {
		file_close(slot->fh);
		slot->fh = FILEH_INVALID;
	}
	file_cpyname(slot->fname, "m75-missing-write-backend.raw", sizeof(slot->fname));
	scsiio.wrdatpos = scsiio.cmdpos;
	service = scsicmd_transinfo(0);
	SCMD_SELFTEST_CHECK("failed_backend_write_does_not_return_good",
	                    service == scsicmd_phase_service_status(SCSIPH_STATUS) &&
	                        scsiio.reg[SCSICTR_STATUS] == 0x02 && hdd_sense[2] == 0x03 &&
	                        hdd_sense[12] == 0x0c);
	service = scsicmd_transinfo(0);
	SCMD_SELFTEST_CHECK("check_condition_survives_status_transfer",
	                    service == scsicmd_phase_service_status(SCSIPH_MSGIN) &&
	                        scsiio.reg[SCSICTR_STATUS] == 0x02 && hdd_sense[2] == 0x03 &&
	                        hdd_sense[12] == 0x0c);
	file_cpyname(slot->fname, saved_test_fname, sizeof(slot->fname));
	slot->fh = FILEH_INVALID;

	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x2a;
	cdb[5] = 20;
	cdb[7] = 0;
	cdb[8] = 2;
	SCMD_SELFTEST_COMMAND(cdb);
	for (test_i = 0; test_i < sizeof(readback_multi); test_i++) {
		scsiio.data[test_i] = (BYTE)(0x30 + (test_i & 0x7f));
	}
	scsiio.wrdatpos = scsiio.cmdpos;
	service = scsicmd_transinfo(0);
	SCMD_SELFTEST_CHECK("write10_multi_block_round_trip",
	                    service == scsicmd_phase_service_status(SCSIPH_STATUS) &&
	                        scsi_block.commit_count == 1 &&
	                        sxsi_read(0x20, 20, readback_multi, sizeof(readback_multi)) == 0 &&
	                        memcmp(scsiio.data, readback_multi, sizeof(readback_multi)) == 0);

	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x2a;
	cdb[2] = 0x02;
	cdb[5] = 0;
	cdb[7] = 0;
	cdb[8] = 1;
	SCMD_SELFTEST_COMMAND(cdb);
	SCMD_SELFTEST_CHECK("write10_out_of_range_does_not_modify_media",
	                    service == scsicmd_phase_service_status(SCSIPH_STATUS) &&
	                        scsiio.reg[SCSICTR_STATUS] == 0x02 && hdd_sense[2] == 0x05 &&
	                        hdd_sense[12] == 0x21 && hdd_sense[13] == 0x00 &&
	                        sxsi_read(0x20, 20, readback_multi, sizeof(readback_multi)) == 0 &&
	                        readback_multi[0] == 0x30 && readback_multi[127] == 0xaf &&
	                        readback_multi[128] == 0x30 && readback_multi[511] == 0xaf);

	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x28;
	cdb[5] = 5;
	cdb[7] = 0;
	cdb[8] = 1;
	SCMD_SELFTEST_COMMAND(cdb);
	SCMD_SELFTEST_CHECK("read_after_write_matches_byte_for_byte",
	                    service == scsicmd_phase_service_status(SCSIPH_DATAIN) &&
	                        memcmp(scsiio.data, readback, sizeof(readback)) == 0);

	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x2a;
	cdb[5] = 6;
	cdb[7] = 0;
	cdb[8] = 0;
	SCMD_SELFTEST_COMMAND(cdb);
	SCMD_SELFTEST_CHECK("write10_zero_blocks_is_good_without_write",
	                    service == scsicmd_phase_service_status(SCSIPH_STATUS) &&
	                        scsiio.reg[SCSICTR_STATUS] == 0 && scsi_block.commit_count == 0);

	slot->type = SXSITYPE_SCSI | SXSITYPE_CDROM;
	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x2a;
	cdb[5] = 1;
	cdb[7] = 0;
	cdb[8] = 1;
	SCMD_SELFTEST_COMMAND(cdb);
	SCMD_SELFTEST_CHECK("write10_read_only_returns_07_27_00",
	                    service == scsicmd_phase_service_status(SCSIPH_STATUS) &&
	                        scsiio.reg[SCSICTR_STATUS] == 2 && hdd_sense[2] == 7 &&
	                        hdd_sense[12] == 0x27 && hdd_sense[13] == 0);
	slot->type = SXSITYPE_SCSI | SXSITYPE_HDD;

	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x08;
	cdb[1] = 0x00;
	cdb[2] = 0x00;
	cdb[3] = 7;
	cdb[4] = 1;
	SCMD_SELFTEST_COMMAND(cdb);
	SCMD_SELFTEST_CHECK("read6_decodes_21_bit_lba",
	                    service == scsicmd_phase_service_status(SCSIPH_DATAIN) &&
	                        scsi_block.lba == 8 && scsiio.data[0] == 0x50);

	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x08;
	cdb[1] = 0;
	cdb[2] = 0;
	cdb[3] = 0;
	cdb[4] = 0;
	SCMD_SELFTEST_COMMAND(cdb);
	SCMD_SELFTEST_CHECK("read6_zero_transfer_length_means_256",
	                    service == scsicmd_phase_service_status(SCSIPH_DATAIN) &&
	                        scsiio.cmdpos == sizeof(scsiio.data));
	SCMD_SELFTEST_CHECK("read6_256_blocks_programs_tc_010000",
	                    scsi_block.total_blocks == 256 && scsi_block.chunk_bytes == 65536 &&
	                        scsiio.cmdpos == 65536 && scsiio.data[0] == test_media[0] &&
	                        scsiio.data[65535] == test_media[65535]);

	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x0a;
	cdb[1] = 0;
	cdb[2] = 0;
	cdb[3] = 9;
	cdb[4] = 0;
	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x28;
	cdb[7] = 1;
	cdb[8] = 0;
	SCMD_SELFTEST_COMMAND(cdb);
	SCMD_SELFTEST_CHECK("read10_256_blocks_exact_65536",
	                    service == scsicmd_phase_service_status(SCSIPH_DATAIN) &&
	                        scsi_block.total_blocks == 256 && scsi_block.chunk_bytes == 65536 &&
	                        scsiio.cmdpos == 65536 && scsiio.data[65535] == test_media[65535]);
	SCMD_SELFTEST_CHECK("read_65536_backend_staging_match",
	                    scsiio.data[0] == test_media[0] && scsiio.data[65535] != scsiio.data[0]);

	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x28;
	cdb[7] = 1;
	cdb[8] = 1;
	SCMD_SELFTEST_COMMAND(cdb);
	SCMD_SELFTEST_CHECK("read10_65537_chunk_boundary",
	                    service == scsicmd_phase_service_status(SCSIPH_DATAIN) &&
	                        scsi_block.total_blocks == 257 && scsiio.cmdpos == 65536);
	if (service == scsicmd_phase_service_status(SCSIPH_DATAIN)) {
		service = scsicmd_block_datain_complete();
	}
	SCMD_SELFTEST_CHECK("read10_65537_second_chunk",
	                    service == scsicmd_phase_service_status(SCSIPH_DATAIN) &&
	                        scsi_block.chunk_bytes == 256 && scsi_block.lba == 257 &&
	                        scsiio.cmdpos == 256);

	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x0a;
	cdb[5] = 6;
	cdb[7] = 0;
	cdb[8] = 0;
	SCMD_SELFTEST_COMMAND(cdb);
	SCMD_SELFTEST_CHECK("write6_zero_transfer_length_means_256",
	                    service == scsicmd_phase_service_status(SCSIPH_DATAOUT) &&
	                        scsiio.cmdpos == sizeof(scsiio.data) && scsi_block.commit_count == 0);

	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x0a;
	cdb[1] = 0;
	cdb[2] = 0;
	cdb[3] = 8;
	cdb[4] = 1;
	SCMD_SELFTEST_COMMAND(cdb);
	ZeroMemory(scsiio.data, 256);
	scsiio.wrdatpos = scsiio.cmdpos;
	service = scsicmd_transinfo(0);
	SCMD_SELFTEST_CHECK("write6_decodes_lba",
	                    service == scsicmd_phase_service_status(SCSIPH_STATUS) &&
	                        scsi_block.commit_count == 1);

	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x28;
	cdb[5] = 0;
	cdb[7] = 1;
	cdb[8] = 1;
	SCMD_SELFTEST_COMMAND(cdb);
	SCMD_SELFTEST_CHECK("request_crossing_internal_buffer_boundary",
	                    service == scsicmd_phase_service_status(SCSIPH_DATAIN) &&
	                        scsiio.cmdpos == sizeof(scsiio.data));
	if (service == scsicmd_phase_service_status(SCSIPH_DATAIN)) {
		service = scsicmd_block_datain_complete();
	}
	SCMD_SELFTEST_CHECK("chunked_read_has_no_duplicate_or_missing_bytes",
	                    service == scsicmd_phase_service_status(SCSIPH_DATAIN) &&
	                        scsiio.cmdpos == 256 && scsiio.data[0] == test_media[256 * 256]);

	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x2a;
	cdb[5] = 0;
	cdb[7] = 1;
	cdb[8] = 1;
	SCMD_SELFTEST_COMMAND(cdb);
	for (test_i = 0; test_i < sizeof(scsiio.data); test_i++) {
		scsiio.data[test_i] = (BYTE)(test_i ^ 0x5a);
	}
	scsiio.wrdatpos = scsiio.cmdpos;
	service = scsicmd_block_dataout_complete();
	if (service == scsicmd_phase_service_status(SCSIPH_DATAOUT)) {
		for (test_i = 0; test_i < 256; test_i++) {
			scsiio.data[test_i] = (BYTE)(test_i ^ 0xa5);
		}
		scsiio.wrdatpos = scsiio.cmdpos;
		service = scsicmd_block_dataout_complete();
	}
	SCMD_SELFTEST_CHECK("chunked_write_round_trip",
	                    service == scsicmd_phase_service_status(SCSIPH_STATUS) &&
	                        scsi_block.commit_count == 2);

	if (slot->fh != FILEH_INVALID) {
		file_close(slot->fh);
		slot->fh = FILEH_INVALID;
	}
	file_delete(test_path);

	for (id = 0; id < SCSIHDD_MAX; id++) {
		slot = sxsi_getptr((REG8)(0x20 + id));
		*slot = saved_slots[id];
	}
	CopyMemory(scsiio.reg, saved_reg, sizeof(saved_reg));
	CopyMemory(scsiio.cmd, saved_cmd, sizeof(saved_cmd));
	CopyMemory(scsiio.data, saved_data, sizeof(saved_data));
	CopyMemory(hdd_sense, saved_sense, sizeof(saved_sense));
	scsiio.phase = saved_phase;
	scsiio.cmdpos = saved_cmdpos;
	scsiio.rddatpos = saved_rddatpos;
	scsiio.port = saved_port;
	scsiio.scsistatus = saved_scsistatus;
	scsicmd_check_condition = saved_check_condition;

#undef SCMD_SELFTEST_COMMAND
#undef SCMD_SELFTEST_CHECK
	return ok ? SUCCESS : FAILURE;
}

BOOL scsicmd_send(void) {
	switch (scsiio.phase) {
	case SCSIPH_COMMAND:
		scsiio.cmdpos = 0;
		return (SUCCESS);
	}
	return (FAILURE);
}

REG8 scsicmd_transinfo(REG8 id) {
	REG8 ret;

	switch (scsiio.phase) {
	case SCSIPH_COMMAND:
		CopyMemory(scsiio.cmd, scsiio.reg + SCSICTR_CDB, sizeof(scsiio.cmd));
		ret = scsicmd_command(id);
		return (ret);

	case SCSIPH_DATAIN:
		if (scsiio.rddatpos >= scsiio.cmdpos) {
			ret = scsicmd_block_datain_complete();
			if (ret == scsicmd_phase_service_status(SCSIPH_DATAIN)) {
				return ret;
			}
			scsiio.phase = SCSIPH_STATUS;
			/* The next phase starts a fresh PIO data window. */
			scsiio.rddatpos = 0;
			return (scsicmd_phase_service_status(SCSIPH_STATUS));
		}
		return (scsicmd_phase_service_status(SCSIPH_DATAIN));

	case SCSIPH_DATAOUT:
		if (scsiio.cmdpos && (scsiio.wrdatpos >= scsiio.cmdpos)) {
			ret = scsicmd_block_dataout_complete();
			if (ret == scsicmd_phase_service_status(SCSIPH_DATAOUT)) {
				return ret;
			}
			scsiio.phase = SCSIPH_STATUS;
			/* The next phase starts a fresh PIO data window. */
			scsiio.rddatpos = 0;
			return (scsicmd_phase_service_status(SCSIPH_STATUS));
		}
		return (scsicmd_phase_service_status(SCSIPH_DATAOUT));

	case SCSIPH_STATUS:
		/* Preserve the command result selected by the target command
			 * layer.  A failed DATA OUT backend must remain CHECK CONDITION
			 * through STATUS transfer. */
		scsiio.phase = SCSIPH_MSGIN;
		/* The next phase starts a fresh PIO data window. */
		scsiio.rddatpos = 0;
		return (scsicmd_phase_service_status(SCSIPH_MSGIN));

	case SCSIPH_MSGIN:
		scsiio.phase = 0;
		scsiio.rddatpos = 0;
		return ((scsiio.reg[SCSICTR_CONTROL] & 0x08) ? 0x85 : 0x80);
	}
	return (0x42);
}

// ---- BIOS から

static const UINT8 stat2ret[16] = {0x40, 0x00, 0x10, 0x00, 0x20, 0x00, 0x10, 0x00,
                                   0x30, 0x00, 0x10, 0x00, 0x20, 0x00, 0x10, 0x00};

static REG8 bios1bc_seltrans(REG8 id) {
	BYTE cdb[16];
	REG8 ret;

	MEML_READSTR(CPU_DS, CPU_DX, cdb, 16);
	scsiio.reg[SCSICTR_TARGETLUN] = cdb[0];
	scsiio_trace_bios_select_transfer(id, cdb[0], cdb[1], cdb[4], cdb[5], CPU_CX);
	if ((cdb[1] & 0x0c) == 0x08) { // OUT
		MEML_READSTR(CPU_ES, CPU_BX, scsiio.data, CPU_CX);
	}
	ret = scsicmd_transfer(id, cdb + 4);
	if ((cdb[1] & 0x0c) == 0x04) { // IN
		MEML_WRITESTR(CPU_ES, CPU_BX, scsiio.data, CPU_CX);
		scsicmd_direct_data_complete();
	}
	return (ret);
}

void scsicmd_bios(void) {
	UINT8 flag;
	UINT8 ret;
	REG8 stat;
	UINT cmd;
	REG8 dstid;

	TRACEOUT(("BIOS 1B-C* CPU_AH %.2x", CPU_AH));

	if (CPU_AH & 0x80) { // エラーぽ
		return;
	}

	flag = MEML_READ8(CPU_SS, CPU_SP + 4) & 0xbe;
	ret = mem[0x0483];
	cmd = CPU_AH & 0x1f;
	dstid = CPU_AL & 7;
	if (ret & 0x80) {
		mem[0x0483] &= 0x7f;
	} else if (cmd < 0x18) {
		switch (cmd) {
		case 0x00: // reset
			stat = 0x00;
			break;

		case 0x03: // Negate ACK
			stat = scsicmd_negate(dstid);
			break;

		case 0x07: // Select Without AMN
			stat = scsicmd_select(dstid);
			break;

		case 0x09: // Select Without AMN and Transfer
			stat = bios1bc_seltrans(dstid);
			break;

		default:
			TRACEOUT(("cmd = %.2x", CPU_AH));
			SCSICMD_ERR
			stat = 0x42;
			break;
		}
		ret = stat2ret[stat >> 4] + (stat & 0x0f);
		TRACEOUT(("BIOS 1B-C* CPU_AH %.2x ret = %.2x", CPU_AH, ret));
		mem[0x0483] = ret;
	} else {
		if ((ret ^ cmd) & 0x0f) {
			ret = cmd | 0x80;
		} else {
			switch (cmd) {
			case 0x19: // Data In
				MEML_WRITESTR(CPU_ES, CPU_BX, scsiio.data, CPU_CX);
				scsiio.phase = SCSIPH_STATUS;
				stat = 0x8b;
				break;

			case 0x1a: // Transfer command
				MEML_READSTR(CPU_ES, CPU_BX, scsiio.cmd, 12);
				stat = scsicmd_command(dstid);
				break;

			case 0x1b: // Status In
				scsiio.phase = SCSIPH_MSGIN;
				stat = 0x8f;
				break;

			case 0x1f: // Message In
				scsiio.phase = 0;
				stat = 0x80;
				break;

			default:
				TRACEOUT(("cmd = %.2x", CPU_AH));
				SCSICMD_ERR
				stat = 0x42;
				break;
			}
			ret = stat2ret[stat >> 4] + (stat & 0x0f);
		}
		TRACEOUT(("BIOS 1B-C* CPU_AH %.2x ret = %.2x", CPU_AH, ret));
		mem[0x0483] = ret;
	}
	flag |= ret & Z_FLAG;
	if (ret & 0x80) {
		flag |= C_FLAG;
		ret &= 0x7f;
	}
	CPU_AH = ret;
	MEML_WRITE8(CPU_SS, CPU_SP + 4, flag);
}
