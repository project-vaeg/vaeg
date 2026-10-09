/*
 * PC-88VA C-bus device lifecycle owner.
 * This tier resets and binds live expansion devices (SASI, SCSI, MPU98II,
 * and BMS); each device ultimately registers its CPU-visible ports through
 * iocore. It is a hardware ownership boundary, not PC-9801 compatibility
 * residue. Evidence: docs/agents/reports/m96_va_only_structural_cleanup.md,
 * section 11.
 */

#include "compiler.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "cbuscore.h"
#include "sasiio.h"
#include "scsiio.h"
#include "mpu98ii.h"
#include "bmsio.h"

static const IOCBFN resetfn[] = {
    sasiio_reset,
    scsiio_reset,
    mpu98ii_reset,
    bmsio_reset,
};

static const IOCBFN bindfn[] = {
    sasiio_bind,
    scsiio_bind,
    mpu98ii_bind,
    bmsio_bind,
};

void cbuscore_reset(void) {
	iocore_cb(resetfn, sizeof(resetfn) / sizeof(IOCBFN));
}

void cbuscore_bind(void) {
	iocore_cb(bindfn, sizeof(bindfn) / sizeof(IOCBFN));
}
