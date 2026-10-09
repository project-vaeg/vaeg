
enum {
	SXSIBIOS_SASI = 0,
	SXSIBIOS_SCSI = 2
};

#ifdef __cplusplus
extern "C" {
#endif

REG8 sasibios_operate(void);

REG8 scsibios_operate(void);

void np2sysp_sasi(const void *arg1, long arg2);

void np2sysp_scsi(const void *arg1, long arg2);
void np2sysp_scsidev(const void *arg1, long arg2);

#ifdef __cplusplus
}
#endif
