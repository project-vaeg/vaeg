
typedef struct {
	UINT port;
	UINT phase;
	BYTE reg[0x30];
	UINT8 auxstatus;
	UINT8 scsistatus;
	UINT8 membank;
	UINT8 memwnd;
	UINT8 resent;
	UINT8 datmap;
	UINT cmdpos;
	UINT wrdatpos;
	UINT rddatpos;
	BYTE cmd[12];
	BYTE data[0x10000];
	/* Keep the historical serialized image size without owning board ROM. */
	BYTE reserved[2][0x2000];

} _SCSIIO, *SCSIIO;

#ifdef __cplusplus
extern "C" {
#endif

extern _SCSIIO scsiio;

void scsiioint(NEVENTITEM item);
void scsiio_watchdog_event(NEVENTITEM item);

void scsiio_reset(void);
void scsiio_bind(void);
void scsiio_trace_enable(BOOL enabled);
void scsiio_trace_compact(BOOL compact);
void scsiio_trace_census_only(BOOL census_only);
void scsiio_trace_limit(UINT limit);
void scsiio_trace_jitter(BOOL enabled, UINT seed, UINT span);
BOOL scsiio_trace_stop_requested(void);
void scsiio_trace_pic_irq(REG8 irq, BOOL asserted);
void scsiio_trace_target_selection(UINT target_id, UINT target_lun, UINT selected_index,
                                   REG8 status);
void scsiio_trace_bios_select_transfer(UINT target_id, UINT packet_lun, REG8 flags, REG8 cdb_opcode,
                                       REG8 cdb1, UINT transfer_bytes);
void scsiio_trace_cdb_result(UINT target_id, UINT target_lun, UINT cdb_lun, UINT selected_index,
                             const BYTE *cdb, UINT cdb_length, REG8 inquiry_byte0,
                             UINT response_length, REG8 status, REG8 sense_key, REG8 asc,
                             REG8 ascq);
void scsiio_trace_census_command(UINT target_id, UINT target_lun, UINT cdb_lun, const BYTE *cdb,
                                 UINT cdb_length, UINT32 lba, UINT32 block_count, UINT32 byte_count,
                                 const char *direction, REG8 backend_result,
                                 UINT32 transferred_bytes, UINT32 residual_bytes, REG8 status,
                                 REG8 sense_key, REG8 asc, REG8 ascq, const char *data_path,
                                 BOOL unsupported);
void scsiio_trace_census_report(void);
void scsiio_trace_block_start(UINT sequence, UINT target_id, UINT target_lun, UINT cdb_lun,
                              const BYTE *cdb, UINT32 lba, UINT32 block_count, UINT sector_size,
                              UINT32 byte_count, UINT backend_index, BOOL backend_read_only);
UINT scsiio_transfer_count(void);

void scsiio_trace_block_program(UINT sequence, REG8 opcode, UINT32 cdb_transfer_length,
                                UINT32 decoded_blocks, UINT32 decoded_bytes, REG8 ar12, REG8 ar13,
                                REG8 ar14, UINT32 transfer_count);

void scsiio_trace_block_chunk(UINT sequence, UINT chunk_index, UINT32 lba, UINT32 block_count,
                              UINT32 byte_offset, UINT32 byte_count);
void scsiio_trace_block_backend_data(const BYTE *data, UINT32 count);
void scsiio_trace_block_staging_data(const BYTE *data, UINT32 count);
void scsiio_trace_block_delivered_data(const BYTE *data, UINT32 count);
void scsiio_trace_block_delivered_byte(REG8 data);
void scsiio_trace_block_complete(UINT sequence, REG8 opcode, UINT32 transferred_bytes,
                                 UINT32 residual_bytes, UINT32 backend_blocks, REG8 backend_result,
                                 REG8 status, REG8 sense_key, REG8 asc, REG8 ascq,
                                 UINT commit_count);
BOOL scsiio_transfer_selftest(void);
void scsiio_legacy_dataout_selftest_byte(REG8 dat);

#ifdef __cplusplus
}
#endif
