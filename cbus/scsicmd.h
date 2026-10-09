
#ifdef __cplusplus
extern "C" {
#endif

REG8 scsicmd_negate(REG8 id);
REG8 scsicmd_select(REG8 id);
REG8 scsicmd_transfer(REG8 id, BYTE *cdb);
REG8 scsicmd_command(REG8 id);
REG8 scsicmd_transinfo(REG8 id);
BOOL scsicmd_block_data_available(void);
BOOL scsicmd_block_dataout_ready(void);
void scsicmd_block_reset_state(void);
BOOL scsicmd_send(void);
BOOL scsicmd_backend_selftest(void);
BOOL scsicmd_direct_data_available(void);
void scsicmd_direct_data_complete(void);
BOOL scsicmd_direct_dataout_available(void);
void scsicmd_direct_dataout_complete(void);
REG8 scsicmd_phase_service_status(UINT phase);
REG8 scsicmd_phase_unexpected_status(UINT phase);
BOOL scsicmd_phase_host_to_spc(UINT phase);

void scsicmd_bios(void);

#ifdef __cplusplus
}
#endif
