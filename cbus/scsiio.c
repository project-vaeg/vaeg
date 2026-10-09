#include "compiler.h"

#include "dosio.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "cbuscore.h"
#include "scsiio.h"
#include "scsiio.tbl"
#include "scsicmd.h"

#include "iocoreva.h"
#include "upd9002_trace.h"

_SCSIIO scsiio;

static const UINT8 scsiirq[] = {0x03, 0x05, 0x06, 0x09, 0x0c, 0x0d, 3, 3};
static BOOL scsi_trace_enabled;
static BOOL scsi_trace_compact;
static BOOL scsi_trace_census_only;
static UINT scsi_trace_completion_limit;
static UINT scsi_trace_completion_count;
static BOOL scsi_trace_stop;
static BOOL scsi_csr_latched;
static BOOL scsi_csr_event_active;
static REG8 scsi_csr_event_status;
static UINT scsi_trace_csr_sequence;
static UINT scsi_csr_event_sequence;
static const char *scsi_csr_event_origin;
static UINT scsi_csr_latched_sequence;
static const char *scsi_csr_latched_origin;
static UINT32 scsi_csr_event_clock;
static UINT32 scsi_csr_latched_clock;
static UINT32 scsi_trace_data_phase_decision_clock;
static BOOL scsi_trace_data_phase_pending;
static BOOL scsi_trace_event_watchdog_reported;
static BOOL scsi_trace_latched_watchdog_reported;
static BOOL scsi_trace_data_phase_watchdog_reported;
static BOOL scsi_trace_watchdog_scheduled;
static BOOL scsi_trace_jitter_enabled;
static UINT32 scsi_trace_jitter_state;
static UINT scsi_trace_jitter_span;
static UINT32 scsi_target_phase_delay_clock;
static BOOL scsi_trace_target_delay_watchdog_reported;
static UINT scsi_trace_data_phase_request_missing_count;
static BOOL scsi_target_selection_pending;
static REG8 scsi_target_selection_status;
static const char *scsi_target_selection_origin;
static BOOL scsi_target_phase_delay_pending;
static BOOL scsi_command_phase_pending;
static BOOL scsi_transfer_phase_pending;
static REG8 scsi_transfer_phase_status;
static BOOL scsi_target_phase_ready;
static UINT scsi_transfer_remaining;
typedef enum {
	SCSI_TRANSFER_IDLE = 0,
	SCSI_TRANSFER_WAIT_FOR_REQ,
	SCSI_TRANSFER_BYTE_PENDING,
	SCSI_TRANSFER_WAIT_FOR_POST_COUNT_REQ,
	SCSI_TRANSFER_COMPLETED_OR_TERMINATED
} SCSITRANSFERSTATE;
static SCSITRANSFERSTATE scsi_transfer_state;
static BOOL scsi_transfer_req_asserted;
static BOOL scsi_transfer_ack_asserted;
static BOOL scsi_transfer_req_retained;
static UINT scsi_transfer_retained_req_sequence;
static UINT scsi_transfer_retained_phase;
static BOOL scsi_transfer_retained_host_to_spc;
static BOOL scsi_transfer_single_byte;
static BOOL scsi_bus_free_pending;
static REG8 scsi_bus_free_status;
static BOOL scsi_transfer_selftest_mode;
static REG8 scsi_transfer_selftest_last_csr;
static UINT scsi_transfer_selftest_latch_count;
static UINT scsi_transfer_selftest_transferred_bytes;
static UINT scsi_transfer_req_sequence;
static UINT scsi_transfer_completion_status;
static UINT scsi_transfer_active_phase;
static void scsiio_command_write(REG8 command);
static REG8 scsiio_success_status_from_service(REG8 service_status);
static void scsiio_target_assert_req(const char *kind, REG8 status);
static void scsiio_target_negate_req(const char *reason);
static void scsiio_initiator_assert_ack(void);
static void scsiio_initiator_negate_ack(void);
static void scsiio_complete_byte_handshake(void);
static void scsiio_start_transfer(void);
static void scsiio_post_count_wait(REG8 next_status);
static BOOL scsiio_transfer_active(void);

static BOOL scsi_trace_transfer_active;
static UINT scsi_trace_transfer_phase;
static UINT scsi_trace_transfer_count;
static UINT scsi_trace_transfer_ar19_accesses;
static UINT scsi_trace_transfer_ar19_reads;
static UINT scsi_trace_transfer_ar19_writes;
static UINT scsi_trace_transfer_data_port_accesses;
static UINT scsi_trace_transfer_irq_requests;
static UINT scsi_trace_transfer_irq_assertions;
static BOOL scsi_trace_transfer_result_pending;
static REG8 scsi_trace_transfer_result_status;
static UINT scsi_trace_transfer_cdb_length;
static BYTE scsi_trace_transfer_cdb[12];
static const char *scsi_trace_transfer_source;
static BOOL scsi_trace_block_active;
static UINT32 scsi_trace_block_backend_bytes;
static UINT32 scsi_trace_block_staging_bytes;
static UINT32 scsi_trace_block_delivered_bytes;
static UINT32 scsi_trace_block_backend_digest;
static UINT32 scsi_trace_block_staging_digest;
static UINT32 scsi_trace_block_delivered_digest;

#define SCSI_CENSUS_RING_SIZE 32

typedef struct {
	UINT sequence;
	UINT target_id;
	UINT target_lun;
	UINT cdb_lun;
	BYTE cdb[12];
	UINT cdb_length;
	UINT32 lba;
	UINT32 block_count;
	UINT32 byte_count;
	char direction[12];
	REG8 backend_result;
	UINT32 transferred_bytes;
	UINT32 residual_bytes;
	REG8 status;
	REG8 sense_key;
	REG8 asc;
	REG8 ascq;
	char data_path[8];
	BOOL unsupported;
} SCSICENSUSRECORD;

static SCSICENSUSRECORD scsi_census_ring[SCSI_CENSUS_RING_SIZE];
static UINT scsi_census_ring_count;
static UINT scsi_census_ring_next;
static UINT scsi_census_sequence;
static UINT scsi_census_opcode_count[256];
static UINT scsi_census_good_count[256];
static UINT scsi_census_check_count[256];
static UINT scsi_census_first_failure_sequence_by_opcode[256];
static UINT scsi_census_first_failure_sequence;
static REG8 scsi_census_first_failure_key;
static REG8 scsi_census_first_failure_asc;
static REG8 scsi_census_first_failure_ascq;
static UINT scsi_census_first_unsupported_sequence;
static UINT scsi_census_first_short_sequence;
static UINT scsi_census_first_residual_sequence;
static UINT scsi_census_pending_failure_sequence;
static BOOL scsi_census_reported_ring;

static void scsi_tracef(const char *fmt, ...);
static UINT32 scsi_trace_clock(void);
static REG8 scsiio_auxstatus(void);
static void scsi_trace_watchdog(void);
static void scsi_target_publish(void);
static void scsi_target_schedule_after_consume(void);
static void scsi_trace_watchdog_schedule(void);
static UINT scsi_target_processing_clocks(void);

/* WD33C93 auxiliary-status bits.  The DATA window is PIO-only in M75. */
#define SCSI_AUX_INT 0x80
#define SCSI_AUX_LCI 0x40
#define SCSI_AUX_BSY 0x20
#define SCSI_AUX_CIP 0x10
#define SCSI_AUX_PE 0x02
#define SCSI_AUX_DBR 0x01

/* Target processing is a controller event, not a guest-tuned ISR delay. */
#define SCSI_TARGET_PROCESSING_CLOCKS 4000
#define SCSI_TRACE_WATCHDOG_CLOCKS 0x40000
#define SCSI_TRACE_JITTER_DEFAULT_SPAN 100

/* 0CC4h uses set/reset strobes for the controller transfer controls. */
#define SCSI_C4_TCMS 0x04
#define SCSI_C4_TCMR 0x08
#define SCSI_C4_TCIR 0x10
#define SCSI_C4_DMER 0x02
#define SCSI_C4_DMES 0x01

static void scsiintr(const char *origin, REG8 status);
static void scsiintr_transfer_complete(REG8 status);
static void scsiintr_enqueue(const char *origin, REG8 status, UINT clocks,
                             BOOL record_transfer_result, BOOL target_event);
static void scsiio_target_phase_ready_event(NEVENTITEM item);

static const char *scsi_trace_phase_direction(UINT phase) {
	if (scsicmd_phase_service_status(phase) == 0x42) {
		return "unknown";
	}
	return scsicmd_phase_host_to_spc(phase) ? "host-to-spc" : "spc-to-host";
}

static void scsi_trace_transfer_start(UINT phase, UINT count, const char *source) {
	if (!scsi_trace_enabled) {
		return;
	}
	if (scsi_trace_transfer_active) {
		scsi_tracef("scsitrace transfer-abandoned phase=%02x direction=%s "
		            "tc=%06x ar19_accesses=%u ar19_reads=%u ar19_writes=%u "
		            "data_port_accesses=%u irq_requests=%u irq_assertions=%u "
		            "source=%s",
		            scsi_trace_transfer_phase,
		            scsi_trace_phase_direction(scsi_trace_transfer_phase),
		            scsi_trace_transfer_count, scsi_trace_transfer_ar19_accesses,
		            scsi_trace_transfer_ar19_reads, scsi_trace_transfer_ar19_writes,
		            scsi_trace_transfer_data_port_accesses, scsi_trace_transfer_irq_requests,
		            scsi_trace_transfer_irq_assertions, scsi_trace_transfer_source);
	}
	scsi_trace_transfer_active = TRUE;
	scsi_trace_transfer_phase = phase;
	scsi_trace_transfer_count = count;
	scsi_trace_transfer_ar19_accesses = 0;
	scsi_trace_transfer_ar19_reads = 0;
	scsi_trace_transfer_ar19_writes = 0;
	scsi_trace_transfer_data_port_accesses = 0;
	scsi_trace_transfer_irq_requests = 0;
	scsi_trace_transfer_irq_assertions = 0;
	scsi_trace_transfer_result_pending = FALSE;
	scsi_trace_transfer_result_status = 0;
	scsi_trace_transfer_cdb_length = 0;
	ZeroMemory(scsi_trace_transfer_cdb, sizeof(scsi_trace_transfer_cdb));
	scsi_trace_transfer_source = source;
	scsi_tracef("scsitrace transfer-start phase=%02x direction=%s tc=%06x "
	            "source=%s cs=%04x ip=%04x",
	            phase, scsi_trace_phase_direction(phase), count, source, CPU_CS, CPU_IP);
}

static void scsi_trace_transfer_ar19_access(BOOL write) {
	if (scsi_trace_transfer_active) {
		scsi_trace_transfer_ar19_accesses++;
		if (write) {
			scsi_trace_transfer_ar19_writes++;
		} else {
			scsi_trace_transfer_ar19_reads++;
		}
	}
}

static void scsi_trace_transfer_data_port_access(void) {
	if (scsi_trace_transfer_active) {
		scsi_trace_transfer_data_port_accesses++;
	}
}

static void scsi_trace_transfer_result(REG8 status) {
	if (!scsi_trace_transfer_active) {
		return;
	}
	scsi_trace_transfer_result_pending = TRUE;
	scsi_trace_transfer_result_status = status;
	scsi_trace_transfer_irq_requests++;
	if (scsi_trace_transfer_phase == SCSIPH_COMMAND) {
		scsi_trace_transfer_cdb_length =
		    min(scsiio.wrdatpos, (UINT)sizeof(scsi_trace_transfer_cdb));
		if (scsi_trace_transfer_cdb_length) {
			CopyMemory(scsi_trace_transfer_cdb, scsiio.cmd, scsi_trace_transfer_cdb_length);
		}
	}
}

static void scsi_trace_transfer_event_result(void) {
	if (!scsi_trace_transfer_active || !scsi_trace_transfer_result_pending) {
		return;
	}
	scsi_tracef("scsitrace transfer-result phase=%02x direction=%s "
	            "tc=%06x ar19_accesses=%u ar19_reads=%u ar19_writes=%u "
	            "data_port_accesses=%u irq_requests=%u irq_assertions=%u "
	            "csr=%02x source=%s cdb_len=%u cdb0=%02x cdb1=%02x "
	            "cdb2=%02x cdb3=%02x cdb4=%02x cdb5=%02x cdb6=%02x "
	            "cdb7=%02x cdb8=%02x cdb9=%02x cdb10=%02x cdb11=%02x",
	            scsi_trace_transfer_phase, scsi_trace_phase_direction(scsi_trace_transfer_phase),
	            scsi_trace_transfer_count, scsi_trace_transfer_ar19_accesses,
	            scsi_trace_transfer_ar19_reads, scsi_trace_transfer_ar19_writes,
	            scsi_trace_transfer_data_port_accesses, scsi_trace_transfer_irq_requests,
	            scsi_trace_transfer_irq_assertions, scsi_trace_transfer_result_status,
	            scsi_trace_transfer_source, scsi_trace_transfer_cdb_length,
	            scsi_trace_transfer_cdb[0], scsi_trace_transfer_cdb[1], scsi_trace_transfer_cdb[2],
	            scsi_trace_transfer_cdb[3], scsi_trace_transfer_cdb[4], scsi_trace_transfer_cdb[5],
	            scsi_trace_transfer_cdb[6], scsi_trace_transfer_cdb[7], scsi_trace_transfer_cdb[8],
	            scsi_trace_transfer_cdb[9], scsi_trace_transfer_cdb[10],
	            scsi_trace_transfer_cdb[11]);
	scsi_trace_transfer_active = FALSE;
	scsi_trace_transfer_result_pending = FALSE;
	if (scsi_trace_completion_limit != 0) {
		scsi_trace_completion_count++;
		if (scsi_trace_completion_count >= scsi_trace_completion_limit) {
			scsi_trace_stop = TRUE;
		}
	}
}

static BOOL scsi_trace_compact_line(const char *fmt) {
	if (!scsi_trace_compact) {
		return (TRUE);
	}
	return (
	    strncmp(fmt, "scsitrace transfer-", 19) == 0 || strncmp(fmt, "scsitrace csr-", 14) == 0 ||
	    strncmp(fmt, "scsitrace data-read", 19) == 0 ||
	    strncmp(fmt, "scsitrace out port=0cc", 22) == 0 ||
	    strncmp(fmt, "scsitrace in port=0cc", 21) == 0 ||
	    strncmp(fmt, "scsitrace target-phase-wait", 27) == 0 ||
	    strncmp(fmt, "scsitrace M75c2", 15) == 0 || strncmp(fmt, "scsitrace warning", 17) == 0 ||
	    strncmp(fmt, "scsitrace command-", 18) == 0 ||
	    strncmp(fmt, "scsitrace target-selection", 25) == 0 ||
	    strncmp(fmt, "scsitrace cdb-result", 20) == 0 ||
	    strncmp(fmt, "scsitrace census", 16) == 0 || strncmp(fmt, "scsitrace census-", 17) == 0 ||
	    strncmp(fmt, "scsitrace block-", 16) == 0 || strncmp(fmt, "scsitrace req-", 14) == 0 ||
	    strncmp(fmt, "scsitrace ack-", 14) == 0 ||
	    strncmp(fmt, "scsitrace data-latched", 22) == 0 ||
	    strncmp(fmt, "scsitrace post-count", 20) == 0 ||
	    strncmp(fmt, "scsitrace data-phase-", 21) == 0 ||
	    strncmp(fmt, "scsitrace jitter", 16) == 0 || strncmp(fmt, "scsitrace watchdog", 18) == 0 ||
	    strncmp(fmt, "scsitrace invariant", 18) == 0);
}

static void scsi_tracef(const char *fmt, ...) {
	va_list ap;

	if (scsi_trace_census_only && (strncmp(fmt, "scsitrace census", 16) != 0) &&
	    (strncmp(fmt, "scsitrace census-", 17) != 0)) {
		return;
	}
	if (!scsi_trace_compact_line(fmt)) {
		return;
	}
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
}

#define SCSITRACEOUT(arg)                                                                          \
	do {                                                                                           \
		if (scsi_trace_enabled) {                                                                  \
			scsi_tracef arg;                                                                       \
		}                                                                                          \
	} while (0)
static UINT32 scsi_trace_clock(void) {
	return (UINT32)(CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK);
}

static UINT scsi_target_processing_clocks(void) {
	UINT32 sample;
	UINT clocks;

	clocks = SCSI_TARGET_PROCESSING_CLOCKS;
	if (!scsi_trace_jitter_enabled || (scsi_trace_jitter_span == 0)) {
		return (clocks);
	}
	/* Numerical Recipes LCG: deterministic and diagnostic-only. */
	scsi_trace_jitter_state = scsi_trace_jitter_state * 1664525U + 1013904223U;
	sample = scsi_trace_jitter_state % (scsi_trace_jitter_span + 1);
	clocks += sample;
	SCSITRACEOUT(("scsitrace jitter base=%u span=%u sample=%u clocks=%u seed=%u",
	              SCSI_TARGET_PROCESSING_CLOCKS, scsi_trace_jitter_span, sample, clocks,
	              scsi_trace_jitter_state));
	return (clocks);
}

static void scsi_trace_watchdog(void) {
	UINT32 now;

	if (!scsi_trace_enabled) {
		return;
	}
	now = scsi_trace_clock();
	if (scsi_csr_event_active && !scsi_trace_event_watchdog_reported &&
	    (UINT32)(now - scsi_csr_event_clock) >= SCSI_TRACE_WATCHDOG_CLOCKS) {
		scsi_trace_event_watchdog_reported = TRUE;
		SCSITRACEOUT(("scsitrace watchdog scheduled-unpublished status=%02x "
		              "seq=%u clocks=%u cs=%04x ip=%04x",
		              scsi_csr_event_status, scsi_csr_event_sequence,
		              (UINT32)(now - scsi_csr_event_clock), CPU_CS, CPU_IP));
	}
	if (scsi_csr_latched && !scsi_trace_latched_watchdog_reported &&
	    (UINT32)(now - scsi_csr_latched_clock) >= SCSI_TRACE_WATCHDOG_CLOCKS) {
		scsi_trace_latched_watchdog_reported = TRUE;
		SCSITRACEOUT(("scsitrace watchdog unconsumed-csr status=%02x "
		              "seq=%u clocks=%u cs=%04x ip=%04x",
		              scsiio.scsistatus, scsi_csr_latched_sequence,
		              (UINT32)(now - scsi_csr_latched_clock), CPU_CS, CPU_IP));
	}
	if (scsi_target_phase_delay_pending && !scsi_trace_target_delay_watchdog_reported &&
	    (UINT32)(now - scsi_target_phase_delay_clock) >= SCSI_TRACE_WATCHDOG_CLOCKS) {
		scsi_trace_target_delay_watchdog_reported = TRUE;
		SCSITRACEOUT(("scsitrace watchdog target-phase-delay clocks=%u "
		              "phase=%02x cs=%04x ip=%04x",
		              (UINT32)(now - scsi_target_phase_delay_clock), scsiio.phase, CPU_CS, CPU_IP));
	}
	if (scsi_trace_data_phase_pending && !scsi_trace_data_phase_watchdog_reported &&
	    (UINT32)(now - scsi_trace_data_phase_decision_clock) >= SCSI_TRACE_WATCHDOG_CLOCKS) {
		scsi_trace_data_phase_watchdog_reported = TRUE;
		scsi_trace_data_phase_request_missing_count++;
		SCSITRACEOUT(("scsitrace watchdog data-phase-request-missing "
		              "status=89 count=%u clocks=%u cs=%04x ip=%04x",
		              scsi_trace_data_phase_request_missing_count,
		              (UINT32)(now - scsi_trace_data_phase_decision_clock), CPU_CS, CPU_IP));
	}
}

static BOOL scsi_trace_watchdog_needed(void) {
	return scsi_trace_enabled &&
	       (scsi_csr_event_active || scsi_csr_latched || scsi_trace_data_phase_pending ||
	        scsi_target_selection_pending || scsi_command_phase_pending ||
	        scsi_transfer_phase_pending || scsi_target_phase_delay_pending);
}

static void scsi_trace_watchdog_schedule(void) {
	if (!scsi_trace_watchdog_needed()) {
		if (scsi_trace_watchdog_scheduled) {
			nevent_reset(NEVENT_SCSIWATCHDOG);
			scsi_trace_watchdog_scheduled = FALSE;
		}
		return;
	}
	nevent_set(NEVENT_SCSIWATCHDOG, SCSI_TRACE_WATCHDOG_CLOCKS, scsiio_watchdog_event,
	           NEVENT_ABSOLUTE);
	scsi_trace_watchdog_scheduled = TRUE;
}

void scsiio_watchdog_event(NEVENTITEM item) {
	scsi_trace_watchdog_scheduled = FALSE;
	scsi_trace_watchdog();
	scsi_trace_watchdog_schedule();
	(void)item;
}

static void scsi_trace_csr_record(const char *event, UINT sequence, REG8 status,
                                  const char *origin) {
	if (!scsi_trace_enabled) {
		return;
	}
	SCSITRACEOUT(("scsitrace csr-%s seq=%u status=%02x origin=%s "
	              "phase=%02x ar=%02x aux=%02x membank=%02x "
	              "event_active=%u event_status=%02x event_seq=%u event_origin=%s "
	              "latched=%u latched_status=%02x latched_seq=%u latched_origin=%s "
	              "selection_pending=%u command_pending=%u transfer_pending=%u "
	              "transfer_status=%02x target_ready=%u delay_pending=%u "
	              "cs=%04x ip=%04x",
	              event, sequence, status, origin ? origin : "none", scsiio.phase, scsiio.port,
	              scsiio_auxstatus(), scsiio.membank, scsi_csr_event_active, scsi_csr_event_status,
	              scsi_csr_event_sequence, scsi_csr_event_origin ? scsi_csr_event_origin : "none",
	              scsi_csr_latched, scsiio.scsistatus, scsi_csr_latched_sequence,
	              scsi_csr_latched_origin ? scsi_csr_latched_origin : "none",
	              scsi_target_selection_pending, scsi_command_phase_pending,
	              scsi_transfer_phase_pending, scsi_transfer_phase_status, scsi_target_phase_ready,
	              scsi_target_phase_delay_pending, CPU_CS, CPU_IP));
}

static REG8 scsiio_auxstatus(void) {
	REG8 ret;

	ret = scsiio.auxstatus &
	      (SCSI_AUX_LCI | SCSI_AUX_BSY | SCSI_AUX_CIP | SCSI_AUX_PE | SCSI_AUX_DBR);
	if (scsi_csr_latched) {
		ret |= SCSI_AUX_INT;
	}
	return ret;
}

static void scsiio_warn_reserved_register(const char *direction) {
	SCSITRACEOUT(("scsitrace warning reserved register range ar=%02x %s "
	              "hardware-pending cs=%04x ip=%04x",
	              scsiio.port, direction, CPU_CS, CPU_IP));
}

UINT scsiio_transfer_count(void) {
	/* WD33C93 exposes Transfer Count as high, middle, low (12h-14h). */
	return ((UINT)scsiio.reg[SCSICTR_TRANSCNT + 0] << 16) |
	       ((UINT)scsiio.reg[SCSICTR_TRANSCNT + 1] << 8) | (UINT)scsiio.reg[SCSICTR_TRANSCNT + 2];
}

static void scsiio_decrement_transfer_count(void) {
	if (scsi_transfer_single_byte) {
		return;
	}
	if (scsiio.reg[SCSICTR_TRANSCNT + 2]) {
		scsiio.reg[SCSICTR_TRANSCNT + 2]--;
	} else if (scsiio.reg[SCSICTR_TRANSCNT + 1]) {
		scsiio.reg[SCSICTR_TRANSCNT + 1]--;
		scsiio.reg[SCSICTR_TRANSCNT + 2] = 0xff;
	} else if (scsiio.reg[SCSICTR_TRANSCNT + 0]) {
		scsiio.reg[SCSICTR_TRANSCNT + 0]--;
		scsiio.reg[SCSICTR_TRANSCNT + 1] = 0xff;
		scsiio.reg[SCSICTR_TRANSCNT + 2] = 0xff;
	}
}

static const char *scsi_transfer_state_name(void) {
	switch (scsi_transfer_state) {
	case SCSI_TRANSFER_IDLE:
		return "idle";
	case SCSI_TRANSFER_WAIT_FOR_REQ:
		return "wait_for_req";
	case SCSI_TRANSFER_BYTE_PENDING:
		return "transfer_byte_pending";
	case SCSI_TRANSFER_WAIT_FOR_POST_COUNT_REQ:
		return "wait_for_post_count_req";
	case SCSI_TRANSFER_COMPLETED_OR_TERMINATED:
		return "completed_or_terminated";
	}
	return "unknown";
}

static BOOL scsiio_transfer_active(void) {
	return scsi_transfer_state == SCSI_TRANSFER_WAIT_FOR_REQ ||
	       scsi_transfer_state == SCSI_TRANSFER_BYTE_PENDING ||
	       scsi_transfer_state == SCSI_TRANSFER_WAIT_FOR_POST_COUNT_REQ;
}

static REG8 scsiio_success_status_from_service(REG8 service_status) {
	if ((service_status & 0xf8) != 0x88) {
		SCSITRACEOUT(("scsitrace warning invalid successful service status=%02x", service_status));
		return 0x42;
	}
	return (REG8)(0x18 | (service_status & 0x07));
}

static void scsiio_target_assert_req(const char *kind, REG8 status) {
	if (scsi_transfer_req_asserted) {
		SCSITRACEOUT(("scsitrace invariant req-overlap seq=%u kind=%s "
		              "status=%02x state=%s",
		              scsi_transfer_req_sequence, kind, status, scsi_transfer_state_name()));
		return;
	}
	scsi_transfer_req_asserted = TRUE;
	scsi_transfer_req_sequence++;
	SCSITRACEOUT(("scsitrace req-assert seq=%u kind=%s status=%02x "
	              "phase=%02x direction=%s state=%s tc=%06x",
	              scsi_transfer_req_sequence, kind, status, scsiio.phase,
	              scsi_trace_phase_direction(scsiio.phase), scsi_transfer_state_name(),
	              scsiio_transfer_count()));
}

static void scsiio_target_negate_req(const char *reason) {
	if (!scsi_transfer_req_asserted) {
		return;
	}
	scsi_transfer_req_asserted = FALSE;
	if (scsi_transfer_req_retained) {
		scsi_transfer_req_retained = FALSE;
	}
	SCSITRACEOUT(("scsitrace req-negate seq=%u reason=%s phase=%02x tc=%06x",
	              scsi_transfer_req_sequence, reason, scsiio.phase, scsiio_transfer_count()));
}

static void scsiio_initiator_assert_ack(void) {
	if (scsi_transfer_ack_asserted) {
		SCSITRACEOUT(("scsitrace invariant ack-overlap seq=%u", scsi_transfer_req_sequence));
		return;
	}
	scsi_transfer_ack_asserted = TRUE;
	SCSITRACEOUT(("scsitrace ack-assert seq=%u phase=%02x tc=%06x", scsi_transfer_req_sequence,
	              scsiio.phase, scsiio_transfer_count()));
}

static void scsiio_initiator_negate_ack(void) {
	if (!scsi_transfer_ack_asserted) {
		return;
	}
	scsi_transfer_ack_asserted = FALSE;
	SCSITRACEOUT(("scsitrace ack-negate seq=%u phase=%02x tc=%06x", scsi_transfer_req_sequence,
	              scsiio.phase, scsiio_transfer_count()));
}

static void scsiio_complete_byte_handshake(void) {
	scsiio_target_negate_req("byte-handshake");
	scsiio_initiator_negate_ack();
	if (scsi_transfer_selftest_mode) {
		scsi_transfer_selftest_transferred_bytes++;
	}
}

/* Store payload bytes independently of the physical compatibility port.
 * AR19 owns REQ/ACK and WD transfer-count accounting; 0CC6 is a legacy
 * byte stream, but both paths must feed the same DATA OUT command state. */
static void scsiio_dataout_store_byte(REG8 dat, const char *source) {
	if (scsiio.wrdatpos < sizeof(scsiio.data)) {
		scsiio.data[scsiio.wrdatpos] = dat;
	} else {
		SCSITRACEOUT(("scsitrace invariant data-write-window-overrun "
		              "index=%u capacity=%u phase=%02x source=%s",
		              scsiio.wrdatpos, (UINT)sizeof(scsiio.data), scsiio.phase, source));
	}
	scsiio_trace_block_staging_data(&dat, 1);
	SCSITRACEOUT(
	    ("scsitrace dataout-accept source=%s index=%u data=%02x", source, scsiio.wrdatpos, dat));
	scsiio.wrdatpos++;
}

static void scsiio_legacy_dataout_complete(void) {
	REG8 next_status;

	if ((scsiio.phase != SCSIPH_DATAOUT) || (scsiio.wrdatpos < scsiio.cmdpos)) {
		return;
	}
	next_status = scsicmd_transinfo(scsiio.reg[SCSICTR_DSTID] & 7);
	if (next_status == scsicmd_phase_service_status(SCSIPH_DATAOUT)) {
		/* The command layer prepared another chunk and remains in DATA OUT. */
		return;
	}
	/* Backend commit and status selection are owned by transinfo(). */
	scsiintr("legacy-data-complete", next_status);
}

static void scsiio_start_transfer(void) {
	if (scsi_transfer_state != SCSI_TRANSFER_WAIT_FOR_REQ) {
		return;
	}
	if (scsi_transfer_req_retained) {
		SCSITRACEOUT(("scsitrace retained-req-consume seq=%u phase=%02x "
		              "direction=%s",
		              scsi_transfer_retained_req_sequence, scsi_transfer_retained_phase,
		              scsi_transfer_retained_host_to_spc ? "host-to-spc" : "spc-to-host"));
	}
	if (!scsi_transfer_req_asserted) {
		/* An accepted Level-II command waits for the target REQ.  It must
		 * not manufacture a service-required event or consume a phase. */
		SCSITRACEOUT(("scsitrace transfer-info-wait-for-req phase=%02x "
		              "tc=%06x state=%s",
		              scsiio.phase, scsiio_transfer_count(), scsi_transfer_state_name()));
		scsiio.auxstatus |= SCSI_AUX_BSY;
		scsiio.auxstatus &= (REG8) ~(SCSI_AUX_CIP | SCSI_AUX_DBR);
		return;
	}
	scsi_transfer_active_phase = scsiio.phase;
	scsi_transfer_state = SCSI_TRANSFER_BYTE_PENDING;
	scsiio.auxstatus |= SCSI_AUX_BSY;
	scsiio.auxstatus &= (REG8)~SCSI_AUX_CIP;
	scsiio.auxstatus |= SCSI_AUX_DBR;
	SCSITRACEOUT(("scsitrace command-active state=%s phase=%02x tc=%06x",
	              scsi_transfer_state_name(), scsiio.phase, scsiio_transfer_count()));
}

static void scsiio_post_count_wait(REG8 next_status) {
	scsi_transfer_state = SCSI_TRANSFER_WAIT_FOR_POST_COUNT_REQ;
	scsi_transfer_completion_status = scsiio_success_status_from_service(next_status);
	scsi_transfer_phase_status = next_status;
	scsi_transfer_phase_pending = TRUE;
	scsi_target_phase_ready = FALSE;
	scsi_target_phase_delay_pending = TRUE;
	scsi_target_phase_delay_clock = scsi_trace_clock();
	scsi_trace_target_delay_watchdog_reported = FALSE;
	SCSITRACEOUT(("scsitrace post-count-wait completion=%02x next=%02x "
	              "state=%s tc=%06x",
	              scsi_transfer_completion_status, next_status, scsi_transfer_state_name(),
	              scsiio_transfer_count()));
	if (!scsi_transfer_selftest_mode) {
		nevent_set(NEVENT_SCSIIO, 100, scsiio_target_phase_ready_event, NEVENT_ABSOLUTE);
	}
}

static void scsiio_data_write(REG8 dat) {
	REG8 next_status;

	scsi_trace_transfer_ar19_access(TRUE);
	if (scsicmd_direct_dataout_available()) {
		scsiio_dataout_store_byte(dat, "ar19-direct");
		scsiio_trace_block_delivered_byte(dat);
		SCSITRACEOUT(
		    ("scsitrace direct-data-write ar=19 data=%02x index=%u", dat, scsiio.wrdatpos - 1));
		scsicmd_direct_dataout_complete();
		return;
	}
	if (!scsicmd_phase_host_to_spc(scsiio.phase)) {
		SCSITRACEOUT(("scsitrace warning DATA write phase-direction-mismatch "
		              "phase=%02x cs=%04x ip=%04x",
		              scsiio.phase, CPU_CS, CPU_IP));
		return;
	}
	if (scsi_transfer_state != SCSI_TRANSFER_BYTE_PENDING || !scsi_transfer_req_asserted ||
	    !(scsiio.auxstatus & SCSI_AUX_DBR)) {
		SCSITRACEOUT(("scsitrace warning DATA write without pending REQ "
		              "state=%s req=%u dbr=%u data=%02x",
		              scsi_transfer_state_name(), scsi_transfer_req_asserted ? 1 : 0,
		              (scsiio.auxstatus & SCSI_AUX_DBR) ? 1 : 0, dat));
		return;
	}
	scsiio.auxstatus &= (REG8)~SCSI_AUX_DBR;
	scsiio_initiator_assert_ack();
	SCSITRACEOUT(("scsitrace data-latched direction=host-to-spc "
	              "seq=%u byte=%02x phase=%02x",
	              scsi_transfer_req_sequence, dat, scsiio.phase));
	scsiio_complete_byte_handshake();

	if (scsiio.phase == SCSIPH_COMMAND) {
		if (scsiio.wrdatpos < sizeof(scsiio.cmd)) {
			scsiio.cmd[scsiio.wrdatpos] = dat;
		}
		scsiio.wrdatpos++;
	} else {
		scsiio_dataout_store_byte(dat, "ar19");
	}
	if (!scsi_transfer_single_byte) {
		scsiio_decrement_transfer_count();
	}
	if (scsi_transfer_remaining) {
		scsi_transfer_remaining--;
	}
	if ((scsiio.phase == SCSIPH_DATAOUT) && scsi_transfer_remaining != 0 &&
	    scsicmd_block_dataout_ready()) {
		next_status = scsicmd_transinfo(scsiio.reg[SCSICTR_DSTID] & 7);
		if (next_status == scsicmd_phase_service_status(SCSIPH_DATAOUT)) {
			scsi_transfer_state = SCSI_TRANSFER_BYTE_PENDING;
			scsiio.auxstatus |= SCSI_AUX_DBR;
			scsiio_target_assert_req("chunk", next_status);
			return;
		}
	}
	if (scsi_transfer_remaining != 0) {
		scsi_transfer_state = SCSI_TRANSFER_BYTE_PENDING;
		scsiio.auxstatus |= SCSI_AUX_DBR;
		scsiio_target_assert_req("byte", 0);
		return;
	}

	if (scsiio.phase == SCSIPH_COMMAND) {
		SCSITRACEOUT(("scsitrace M75c2 CDB transfer complete count=%u", scsiio.wrdatpos));
		next_status = scsicmd_command(scsiio.reg[SCSICTR_DSTID] & 7);
		scsiio_post_count_wait(next_status);
		return;
	}
	next_status = scsicmd_transinfo(scsiio.reg[SCSICTR_DSTID] & 7);
	scsiio_post_count_wait(next_status);
}

static REG8 scsiio_data_read(void) {
	REG8 ret;
	REG8 next_status;

	scsi_trace_transfer_ar19_access(FALSE);
	/* Select-and-transfer READs use the raw AR19 data window.  They do not
	 * create the Level-II REQ/ACK state used by TRANSFER INFO. */
	if (scsicmd_direct_data_available()) {
		if (scsiio.rddatpos >= sizeof(scsiio.data)) {
			SCSITRACEOUT(("scsitrace invariant direct-data-window-overrun "
			              "index=%u capacity=%u",
			              scsiio.rddatpos, (UINT)sizeof(scsiio.data)));
			return 0xff;
		}
		ret = scsiio.data[scsiio.rddatpos++];
		scsiio_trace_block_delivered_byte(ret);
		SCSITRACEOUT(
		    ("scsitrace direct-data-read ar=19 data=%02x index=%u", ret, scsiio.rddatpos - 1));
		if (scsiio.rddatpos >= scsiio.cmdpos) {
			/* Select-and-transfer reports completion through its existing 16h
			 * result; the next CDB owns the controller phase reset. */
			scsicmd_direct_data_complete();
		}
		return ret;
	}
	if (scsicmd_phase_host_to_spc(scsiio.phase)) {
		SCSITRACEOUT(("scsitrace warning DATA read phase-direction-mismatch "
		              "phase=%02x cs=%04x ip=%04x",
		              scsiio.phase, CPU_CS, CPU_IP));
		return 0xff;
	}
	if (scsi_transfer_state != SCSI_TRANSFER_BYTE_PENDING || !scsi_transfer_req_asserted ||
	    !(scsiio.auxstatus & SCSI_AUX_DBR)) {
		SCSITRACEOUT(("scsitrace warning DATA read without pending REQ "
		              "state=%s req=%u dbr=%u",
		              scsi_transfer_state_name(), scsi_transfer_req_asserted ? 1 : 0,
		              (scsiio.auxstatus & SCSI_AUX_DBR) ? 1 : 0));
		return 0xff;
	}
	scsiio.auxstatus &= (REG8)~SCSI_AUX_DBR;
	scsiio_initiator_assert_ack();
	if (scsiio.rddatpos >= sizeof(scsiio.data)) {
		SCSITRACEOUT(("scsitrace invariant data-read-window-overrun "
		              "index=%u capacity=%u phase=%02x",
		              scsiio.rddatpos, (UINT)sizeof(scsiio.data), scsiio.phase));
		return 0xff;
	}
	ret = scsiio.data[scsiio.rddatpos];
	scsiio_trace_block_delivered_byte(ret);
	SCSITRACEOUT(("scsitrace data-read ar=19 data=%02x index=%u cs=%04x ip=%04x", ret,
	              scsiio.rddatpos, CPU_CS, CPU_IP));
	SCSITRACEOUT(("scsitrace data-latched direction=spc-to-host "
	              "seq=%u byte=%02x phase=%02x",
	              scsi_transfer_req_sequence, ret, scsiio.phase));
	scsiio.rddatpos++;

	if (scsiio.phase == SCSIPH_MSGIN && scsi_transfer_remaining == 1) {
		/* Message-In completes with 20h.  The target negates REQ,
		 * while ACK remains asserted until Negate ACK. */
		scsiio_target_negate_req("message-in-byte");
		if (scsi_transfer_selftest_mode) {
			scsi_transfer_selftest_transferred_bytes++;
		}
		if (!scsi_transfer_single_byte) {
			scsiio_decrement_transfer_count();
		}
		if (scsi_transfer_remaining) {
			scsi_transfer_remaining--;
		}
		next_status = scsicmd_transinfo(scsiio.reg[SCSICTR_DSTID] & 7);
		scsi_transfer_state = SCSI_TRANSFER_COMPLETED_OR_TERMINATED;
		scsi_transfer_phase_status = next_status;
		scsi_transfer_phase_pending = FALSE;
		scsi_target_phase_ready = FALSE;
		scsiio.auxstatus &= (REG8) ~(SCSI_AUX_BSY | SCSI_AUX_DBR);
		SCSITRACEOUT(("scsitrace message-in-complete status=20 "
		              "req=0 ack=%u next=%02x",
		              scsi_transfer_ack_asserted ? 1 : 0, next_status));
		scsiintr_transfer_complete(0x20);
		return ret;
	}

	if (scsiio.phase == SCSIPH_DATAIN && scsiio.rddatpos >= scsiio.cmdpos &&
	    scsi_transfer_remaining != 0) {
		REG8 short_status;
		BOOL chunk_handshake = FALSE;
		if (scsicmd_block_data_available()) {
			scsiio_complete_byte_handshake();
			if (!scsi_transfer_single_byte) {
				scsiio_decrement_transfer_count();
			}
			if (scsi_transfer_remaining) {
				scsi_transfer_remaining--;
			}
			chunk_handshake = TRUE;
			next_status = scsicmd_transinfo(scsiio.reg[SCSICTR_DSTID] & 7);
			if (next_status == scsicmd_phase_service_status(SCSIPH_DATAIN)) {
				scsi_transfer_state = SCSI_TRANSFER_BYTE_PENDING;
				scsiio.auxstatus |= SCSI_AUX_DBR;
				scsiio_target_assert_req("chunk", next_status);
				return ret;
			}
		}
		if (!chunk_handshake) {
			scsiio_complete_byte_handshake();
			if (!scsi_transfer_single_byte) {
				scsiio_decrement_transfer_count();
			}
			if (scsi_transfer_remaining) {
				scsi_transfer_remaining--;
			}
			next_status = scsicmd_transinfo(scsiio.reg[SCSICTR_DSTID] & 7);
		}
		if (scsi_transfer_remaining == 0) {
			scsiio_post_count_wait(next_status);
			return ret;
		}
		short_status = scsicmd_phase_unexpected_status(scsiio.phase);
		/* A terminated Transfer Info exposes the new phase REQ to the
		 * host with the 4MCI.  It is retained for the next Transfer Info;
		 * do not add a second 8MCI for the same request. */
		if (!scsi_transfer_req_asserted) {
			scsiio_target_assert_req("terminated-post-count", next_status);
		}
		scsi_transfer_req_retained = TRUE;
		scsi_transfer_retained_req_sequence = scsi_transfer_req_sequence;
		scsi_transfer_retained_phase = scsiio.phase;
		scsi_transfer_retained_host_to_spc = scsicmd_phase_host_to_spc(scsiio.phase);
		scsi_transfer_state = SCSI_TRANSFER_COMPLETED_OR_TERMINATED;
		scsi_transfer_phase_status = next_status;
		scsi_transfer_phase_pending = FALSE;
		scsi_target_phase_ready = FALSE;
		scsiio.auxstatus &= (REG8) ~(SCSI_AUX_BSY | SCSI_AUX_DBR);
		SCSITRACEOUT(("scsitrace short-data-phase completed=%u residual=%u "
		              "status=%02x",
		              scsiio.rddatpos, scsi_transfer_remaining, short_status));
		scsiintr_transfer_complete(short_status);
		return ret;
	}

	scsiio_complete_byte_handshake();
	if (!scsi_transfer_single_byte) {
		scsiio_decrement_transfer_count();
	}
	if (scsi_transfer_remaining) {
		scsi_transfer_remaining--;
	}
	if (scsi_transfer_remaining != 0) {
		scsi_transfer_state = SCSI_TRANSFER_BYTE_PENDING;
		scsiio.auxstatus |= SCSI_AUX_DBR;
		scsiio_target_assert_req("byte", 0);
		return ret;
	}
	next_status = scsicmd_transinfo(scsiio.reg[SCSICTR_DSTID] & 7);
	SCSITRACEOUT(("scsitrace data-read-next rddatpos=%u cmdpos=%u "
	              "remaining=%u next_status=%02x phase=%02x",
	              scsiio.rddatpos, scsiio.cmdpos, scsi_transfer_remaining, next_status,
	              scsiio.phase));
	/* Completion reports the phase of the distinct post-count REQ. */
	scsiio_post_count_wait(next_status);
	return ret;
}

void scsiio_trace_enable(BOOL enabled) {
	scsi_trace_enabled = enabled;
	if (enabled) {
		scsi_trace_watchdog_schedule();
	} else {
		nevent_reset(NEVENT_SCSIWATCHDOG);
		scsi_trace_watchdog_scheduled = FALSE;
	}
}

void scsiio_trace_compact(BOOL compact) {
	scsi_trace_compact = compact;
}

void scsiio_trace_census_only(BOOL census_only) {
	scsi_trace_census_only = census_only;
}

void scsiio_trace_limit(UINT limit) {
	scsi_trace_completion_limit = limit;
	scsi_trace_completion_count = 0;
	scsi_trace_data_phase_request_missing_count = 0;
	scsi_trace_stop = FALSE;
}

void scsiio_trace_jitter(BOOL enabled, UINT seed, UINT span) {
	scsi_trace_jitter_enabled = enabled;
	scsi_trace_jitter_state = seed;
	scsi_trace_jitter_span = span ? span : SCSI_TRACE_JITTER_DEFAULT_SPAN;
	if (enabled) {
		SCSITRACEOUT(("scsitrace jitter-config seed=%u span=%u base=%u", seed,
		              scsi_trace_jitter_span, SCSI_TARGET_PROCESSING_CLOCKS));
	}
}

BOOL scsiio_trace_stop_requested(void) {
	return scsi_trace_stop;
}

void scsiio_trace_pic_irq(REG8 irq, BOOL asserted) {
	if (scsi_trace_enabled && (irq == scsiirq[(scsiio.resent >> 3) & 7])) {
		SCSITRACEOUT(("scsitrace irq-%s line=%u cs=%04x ip=%04x", asserted ? "assert" : "clear",
		              irq, CPU_CS, CPU_IP));
	}
}

void scsiio_trace_target_selection(UINT target_id, UINT target_lun, UINT selected_index,
                                   REG8 status) {
	SCSITRACEOUT(("scsitrace target-selection target_id=%u target_lun=%u "
	              "selected_index=%u status=%02x",
	              target_id, target_lun, selected_index, status));
}

void scsiio_trace_bios_select_transfer(UINT target_id, UINT packet_lun, REG8 flags, REG8 cdb_opcode,
                                       REG8 cdb1, UINT transfer_bytes) {
	SCSITRACEOUT(("scsitrace bios-select-transfer target_id=%u packet_lun=%u "
	              "flags=%02x cdb_opcode=%02x cdb1=%02x transfer_bytes=%u "
	              "direction=%s",
	              target_id, packet_lun, flags, cdb_opcode, cdb1, transfer_bytes,
	              ((flags & 0x0c) == 0x08)   ? "OUT"
	              : ((flags & 0x0c) == 0x04) ? "IN"
	                                         : "none"));
}

static void scsiio_trace_census_record(const SCSICENSUSRECORD *record) {
	SCSITRACEOUT(("scsitrace census sequence=%u opcode=%02x cdb_len=%u "
	              "cdb=%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x "
	              "target_id=%u wd_target_lun=%u cdb_lun=%u lba=%u block_count=%u "
	              "byte_count=%u direction=%s backend_result=%02x transferred=%u "
	              "residual=%u STATUS=%02x sense_key=%02x ASC=%02x ASCQ=%02x "
	              "data_path=%s",
	              record->sequence, record->cdb[0], record->cdb_length, record->cdb[0],
	              record->cdb[1], record->cdb[2], record->cdb[3], record->cdb[4], record->cdb[5],
	              record->cdb[6], record->cdb[7], record->cdb[8], record->cdb[9], record->cdb[10],
	              record->cdb[11], record->target_id, record->target_lun, record->cdb_lun,
	              record->lba, record->block_count, record->byte_count, record->direction,
	              record->backend_result, record->transferred_bytes, record->residual_bytes,
	              record->status, record->sense_key, record->asc, record->ascq, record->data_path));
}

static void scsiio_trace_census_marker(const char *kind, const SCSICENSUSRECORD *record) {
	SCSITRACEOUT(("scsitrace census-%s sequence=%u opcode=%02x STATUS=%02x "
	              "sense_key=%02x ASC=%02x ASCQ=%02x",
	              kind, record->sequence, record->cdb[0], record->status, record->sense_key,
	              record->asc, record->ascq));
}

void scsiio_trace_census_command(UINT target_id, UINT target_lun, UINT cdb_lun, const BYTE *cdb,
                                 UINT cdb_length, UINT32 lba, UINT32 block_count, UINT32 byte_count,
                                 const char *direction, REG8 backend_result,
                                 UINT32 transferred_bytes, UINT32 residual_bytes, REG8 status,
                                 REG8 sense_key, REG8 asc, REG8 ascq, const char *data_path,
                                 BOOL unsupported) {
	SCSICENSUSRECORD record;
	UINT i;
	UINT ring_index;
	BOOL check;

	if (!scsi_trace_enabled || (cdb == NULL)) {
		return;
	}
	ZeroMemory(&record, sizeof(record));
	record.sequence = ++scsi_census_sequence;
	record.target_id = target_id;
	record.target_lun = target_lun;
	record.cdb_lun = cdb_lun;
	record.cdb_length = min(cdb_length, (UINT)sizeof(record.cdb));
	CopyMemory(record.cdb, cdb, sizeof(record.cdb));
	record.lba = lba;
	record.block_count = block_count;
	record.byte_count = byte_count;
	strncpy(record.direction, direction ? direction : "none", sizeof(record.direction) - 1);
	strncpy(record.data_path, data_path ? data_path : "none", sizeof(record.data_path) - 1);
	record.backend_result = backend_result;
	record.transferred_bytes = transferred_bytes;
	record.residual_bytes = residual_bytes;
	record.status = status;
	record.sense_key = sense_key;
	record.asc = asc;
	record.ascq = ascq;
	record.unsupported = unsupported;
	check = status != 0x00;
	scsi_census_opcode_count[record.cdb[0]]++;
	if (check) {
		scsi_census_check_count[record.cdb[0]]++;
		if (scsi_census_first_failure_sequence_by_opcode[record.cdb[0]] == 0) {
			scsi_census_first_failure_sequence_by_opcode[record.cdb[0]] = record.sequence;
		}
	} else {
		scsi_census_good_count[record.cdb[0]]++;
	}
	if (unsupported && (scsi_census_first_unsupported_sequence == 0)) {
		scsi_census_first_unsupported_sequence = record.sequence;
		scsiio_trace_census_marker("first-unsupported", &record);
	}
	if (check && (scsi_census_first_failure_sequence == 0)) {
		scsi_census_first_failure_sequence = record.sequence;
		scsi_census_first_failure_key = record.sense_key;
		scsi_census_first_failure_asc = record.asc;
		scsi_census_first_failure_ascq = record.ascq;
		scsi_census_pending_failure_sequence = record.sequence;
		scsiio_trace_census_marker("first-non-good", &record);
		if (!scsi_census_reported_ring) {
			for (i = 0; i < scsi_census_ring_count; i++) {
				ring_index =
				    (scsi_census_ring_next + SCSI_CENSUS_RING_SIZE - scsi_census_ring_count + i) %
				    SCSI_CENSUS_RING_SIZE;
				scsiio_trace_census_record(&scsi_census_ring[ring_index]);
			}
			scsi_census_reported_ring = TRUE;
		}
	}
	if ((residual_bytes != 0) && (scsi_census_first_residual_sequence == 0)) {
		scsi_census_first_residual_sequence = record.sequence;
		scsiio_trace_census_marker("first-nonzero-residual", &record);
	}
	if (((residual_bytes != 0) || (transferred_bytes < byte_count)) &&
	    (scsi_census_first_short_sequence == 0)) {
		scsi_census_first_short_sequence = record.sequence;
		scsiio_trace_census_marker("first-short-transfer", &record);
	}
	if ((record.cdb[0] == 0x03) && (scsi_census_pending_failure_sequence != 0)) {
		SCSITRACEOUT(("scsitrace sense-correlation failing_sequence=%u "
		              "request_sense_sequence=%u sense_key=%02x ASC=%02x ASCQ=%02x",
		              scsi_census_pending_failure_sequence, record.sequence,
		              scsi_census_first_failure_key, scsi_census_first_failure_asc,
		              scsi_census_first_failure_ascq));
		scsi_census_pending_failure_sequence = 0;
	}
	scsiio_trace_census_record(&record);
	CopyMemory(&scsi_census_ring[scsi_census_ring_next], &record, sizeof(record));
	scsi_census_ring_next = (scsi_census_ring_next + 1) % SCSI_CENSUS_RING_SIZE;
	if (scsi_census_ring_count < SCSI_CENSUS_RING_SIZE) {
		scsi_census_ring_count++;
	}
}

void scsiio_trace_census_report(void) {
	UINT opcode;

	if (!scsi_trace_enabled) {
		return;
	}
	for (opcode = 0; opcode < 256; opcode++) {
		if (scsi_census_opcode_count[opcode]) {
			SCSITRACEOUT(("scsitrace census-histogram opcode=%02x count=%u "
			              "GOOD=%u CHECK_CONDITION=%u first_failing_sequence=%u",
			              opcode, scsi_census_opcode_count[opcode], scsi_census_good_count[opcode],
			              scsi_census_check_count[opcode],
			              scsi_census_first_failure_sequence_by_opcode[opcode]));
		}
	}
	SCSITRACEOUT(("scsitrace census-summary first-unsupported-sequence=%u "
	              "first-non-good-sequence=%u first-short-sequence=%u "
	              "first-nonzero-residual-sequence=%u",
	              scsi_census_first_unsupported_sequence, scsi_census_first_failure_sequence,
	              scsi_census_first_short_sequence, scsi_census_first_residual_sequence));
}

void scsiio_trace_cdb_result(UINT target_id, UINT target_lun, UINT cdb_lun, UINT selected_index,
                             const BYTE *cdb, UINT cdb_length, REG8 inquiry_byte0,
                             UINT response_length, REG8 status, REG8 sense_key, REG8 asc,
                             REG8 ascq) {
	SCSITRACEOUT(("scsitrace cdb-result target_id=%u target_lun=%u cdb_lun=%u "
	              "opcode=%02x cdb_len=%u cdb0=%02x cdb1=%02x cdb2=%02x "
	              "cdb3=%02x cdb4=%02x cdb5=%02x cdb6=%02x cdb7=%02x "
	              "cdb8=%02x cdb9=%02x cdb10=%02x cdb11=%02x "
	              "selected_index=%u inquiry0=%02x response_length=%u "
	              "status=%02x sense=%02x asc=%02x ascq=%02x",
	              target_id, target_lun, cdb_lun, cdb[0], cdb_length, cdb[0], cdb[1], cdb[2],
	              cdb[3], cdb[4], cdb[5], cdb[6], cdb[7], cdb[8], cdb[9], cdb[10], cdb[11],
	              selected_index, inquiry_byte0, response_length, status, sense_key, asc, ascq));
}

static UINT32 scsi_trace_digest_update(UINT32 digest, const BYTE *data, UINT32 count) {
	UINT32 i;

	for (i = 0; i < count; i++) {
		digest ^= data[i];
		digest *= 16777619U;
	}
	return digest;
}

void scsiio_trace_block_program(UINT sequence, REG8 opcode, UINT32 cdb_transfer_length,
                                UINT32 decoded_blocks, UINT32 decoded_bytes, REG8 ar12, REG8 ar13,
                                REG8 ar14, UINT32 transfer_count) {
	SCSITRACEOUT(("scsitrace block-transfer-program sequence=%u "
	              "opcode=%02x cdb_transfer_length=%u decoded_blocks=%u "
	              "decoded_bytes=%u ar12=%02x ar13=%02x ar14=%02x tc=%06x",
	              sequence, opcode, cdb_transfer_length, decoded_blocks, decoded_bytes, ar12, ar13,
	              ar14, transfer_count));
}

void scsiio_trace_block_chunk(UINT sequence, UINT chunk_index, UINT32 lba, UINT32 block_count,
                              UINT32 byte_offset, UINT32 byte_count) {
	SCSITRACEOUT(("scsitrace block-chunk sequence=%u chunk=%u lba=%u "
	              "block_count=%u byte_offset=%u byte_count=%u",
	              sequence, chunk_index, lba, block_count, byte_offset, byte_count));
}

void scsiio_trace_block_backend_data(const BYTE *data, UINT32 count) {
	if (!scsi_trace_block_active || (data == NULL)) {
		return;
	}
	scsi_trace_block_backend_digest =
	    scsi_trace_digest_update(scsi_trace_block_backend_digest, data, count);
	scsi_trace_block_backend_bytes += count;
}

void scsiio_trace_block_staging_data(const BYTE *data, UINT32 count) {
	if (!scsi_trace_block_active || (data == NULL)) {
		return;
	}
	scsi_trace_block_staging_digest =
	    scsi_trace_digest_update(scsi_trace_block_staging_digest, data, count);
	scsi_trace_block_staging_bytes += count;
}

void scsiio_trace_block_delivered_data(const BYTE *data, UINT32 count) {
	if (!scsi_trace_block_active || (data == NULL)) {
		return;
	}
	scsi_trace_block_delivered_digest =
	    scsi_trace_digest_update(scsi_trace_block_delivered_digest, data, count);
	scsi_trace_block_delivered_bytes += count;
}

void scsiio_trace_block_delivered_byte(REG8 data) {
	if (!scsi_trace_block_active) {
		return;
	}
	scsi_trace_block_delivered_digest =
	    scsi_trace_digest_update(scsi_trace_block_delivered_digest, &data, 1);
	scsi_trace_block_delivered_bytes++;
}

void scsiio_trace_block_start(UINT sequence, UINT target_id, UINT target_lun, UINT cdb_lun,
                              const BYTE *cdb, UINT32 lba, UINT32 block_count, UINT sector_size,
                              UINT32 byte_count, UINT backend_index, BOOL backend_read_only) {
	scsi_trace_block_active = scsi_trace_enabled;
	scsi_trace_block_backend_bytes = 0;
	scsi_trace_block_staging_bytes = 0;
	scsi_trace_block_delivered_bytes = 0;
	scsi_trace_block_backend_digest = 2166136261U;
	scsi_trace_block_staging_digest = 2166136261U;
	scsi_trace_block_delivered_digest = 2166136261U;

	SCSITRACEOUT(("scsitrace block-start sequence=%u target_id=%u "
	              "wd_target_lun=%u cdb_lun=%u opcode=%02x cdb0=%02x cdb1=%02x "
	              "cdb2=%02x cdb3=%02x cdb4=%02x cdb5=%02x cdb6=%02x cdb7=%02x "
	              "cdb8=%02x cdb9=%02x cdb10=%02x cdb11=%02x lba=%u block_count=%u sector_size=%u "
	              "byte_count=%u backend_device=%u backend_read_only=%u",
	              sequence, target_id, target_lun, cdb_lun, cdb[0], cdb[0], cdb[1], cdb[2], cdb[3],
	              cdb[4], cdb[5], cdb[6], cdb[7], cdb[8], cdb[9], cdb[10], cdb[11], lba,
	              block_count, sector_size, byte_count, backend_index, backend_read_only ? 1 : 0));
}

void scsiio_trace_block_complete(UINT sequence, REG8 opcode, UINT32 transferred_bytes,
                                 UINT32 residual_bytes, UINT32 backend_blocks, REG8 backend_result,
                                 REG8 status, REG8 sense_key, REG8 asc, REG8 ascq,
                                 UINT commit_count) {
	BOOL equal;

	equal = (scsi_trace_block_backend_bytes == scsi_trace_block_staging_bytes) &&
	        (scsi_trace_block_staging_bytes == scsi_trace_block_delivered_bytes) &&
	        (scsi_trace_block_backend_digest == scsi_trace_block_staging_digest) &&
	        (scsi_trace_block_staging_digest == scsi_trace_block_delivered_digest);
	SCSITRACEOUT(("scsitrace block-complete sequence=%u opcode=%02x "
	              "transferred_bytes=%u residual_bytes=%u backend_blocks=%u "
	              "backend_result=%02x status=%02x sense=%02x asc=%02x ascq=%02x "
	              "commit_count=%u backend_bytes=%u staging_bytes=%u delivered_bytes=%u "
	              "backend_digest=%08x staging_digest=%08x delivered_digest=%08x "
	              "digest_equal=%u",
	              sequence, opcode, transferred_bytes, residual_bytes, backend_blocks,
	              backend_result, status, sense_key, asc, ascq, commit_count,
	              scsi_trace_block_backend_bytes, scsi_trace_block_staging_bytes,
	              scsi_trace_block_delivered_bytes, scsi_trace_block_backend_digest,
	              scsi_trace_block_staging_digest, scsi_trace_block_delivered_digest,
	              equal ? 1 : 0));
	scsi_trace_block_active = FALSE;
}

static void scsi_target_publish(void) {
	REG8 status;

	if (scsi_csr_event_active || scsi_csr_latched || scsi_target_phase_delay_pending ||
	    scsi_transfer_req_retained) {
		return;
	}
	if (scsi_bus_free_pending) {
		status = scsi_bus_free_status;
		scsi_bus_free_pending = FALSE;
		scsiintr_enqueue("target-bus-free", status, scsi_target_processing_clocks(), FALSE, TRUE);
		return;
	}
	if (scsi_target_selection_pending) {
		const char *origin = scsi_target_selection_origin;
		status = scsi_target_selection_status;

		scsi_target_selection_pending = FALSE;
		if (!scsi_transfer_req_asserted) {
			scsiio_target_assert_req("service", status);
		}
		scsiintr_enqueue(origin ? origin : "select-result", status, scsi_target_processing_clocks(),
		                 FALSE, TRUE);
		return;
	}
	if (scsi_command_phase_pending) {
		scsi_command_phase_pending = FALSE;
		status = 0x8a;
		if (!scsi_transfer_req_asserted) {
			scsiio_target_assert_req("service", status);
		}
		scsiintr_enqueue("select-command-phase", status, scsi_target_processing_clocks(), FALSE,
		                 TRUE);
		return;
	}
	if (scsi_transfer_phase_pending && scsi_target_phase_ready) {
		status = scsi_transfer_phase_status;
		/* MESSAGE IN completion releases BUS FREE (85h/80h) without
		 * another TRANSFER INFO command. */
		if ((status == 0x85) || (status == 0x80)) {
			scsi_target_phase_ready = TRUE;
		}
		scsi_transfer_phase_pending = FALSE;
		scsi_target_phase_ready = FALSE;
		if (!scsi_transfer_req_asserted) {
			scsiio_target_assert_req("service", status);
		}
		scsiintr_enqueue("target-phase-ready", status, scsi_target_processing_clocks(), FALSE,
		                 TRUE);
	}
}

static void scsi_target_schedule_after_consume(void) {
	if (scsi_transfer_selftest_mode) {
		return;
	}
	if (scsi_csr_event_active || scsi_csr_latched || scsi_target_phase_delay_pending ||
	    scsi_transfer_req_retained) {
		return;
	}
	if (scsi_bus_free_pending || scsi_target_selection_pending || scsi_command_phase_pending ||
	    (scsi_transfer_phase_pending && !scsi_target_phase_ready)) {
		scsi_target_phase_delay_pending = TRUE;
		scsi_target_phase_delay_clock = scsi_trace_clock();
		scsi_trace_target_delay_watchdog_reported = FALSE;
		nevent_set(NEVENT_SCSIIO, scsi_target_processing_clocks(), scsiio_target_phase_ready_event,
		           NEVENT_ABSOLUTE);
	} else {
		scsi_target_publish();
	}
	scsi_trace_watchdog_schedule();
}

void scsiioint(NEVENTITEM item) {
	scsi_csr_event_active = FALSE;
	scsi_trace_watchdog();
	scsiio.scsistatus = scsi_csr_event_status;
	scsi_csr_latched_sequence = scsi_csr_event_sequence;
	scsi_csr_latched_origin = scsi_csr_event_origin;
	upd9002_guest_trace_scsi_status(scsi_csr_event_status);
	scsi_csr_latched = TRUE;
	scsi_csr_latched_clock = scsi_trace_clock();
	scsi_trace_latched_watchdog_reported = FALSE;
	scsi_trace_watchdog_schedule();
	scsi_trace_csr_record("latch", scsi_csr_latched_sequence, scsiio.scsistatus,
	                      scsi_csr_latched_origin);
	scsiio.auxstatus &= (REG8)~SCSI_AUX_CIP;
	if ((scsi_csr_event_status & 0x80) && !scsicmd_phase_host_to_spc(scsiio.phase)) {
		/* DBR is asserted only after the service request is visible. */
		scsiio.auxstatus |= SCSI_AUX_DBR;
	}
	SCSITRACEOUT(("scsitrace event irq=%u cs=%04x ip=%04x aux=%02x "
	              "status=%02x phase=%02x membank=%02x",
	              scsiirq[(scsiio.resent >> 3) & 7], CPU_CS, CPU_IP, scsiio.auxstatus,
	              scsiio.scsistatus, scsiio.phase, scsiio.membank));
	TRACEOUT(("scsiioint"));
	if (scsiio.membank & 4) {
		if (scsi_trace_transfer_active && scsi_trace_transfer_result_pending) {
			scsi_trace_transfer_irq_assertions++;
		}
		pic_setirq(scsiirq[(scsiio.resent >> 3) & 7]);
		TRACEOUT(("scsi intr"));
	}
	scsi_trace_transfer_event_result();
	(void)item;
}

static void scsiio_target_phase_ready_event(NEVENTITEM item) {
	REG8 status;

	scsi_target_phase_delay_pending = FALSE;
	scsi_trace_target_delay_watchdog_reported = FALSE;
	if (scsi_bus_free_pending) {
		scsi_target_publish();
		(void)item;
		return;
	}
	if (scsi_transfer_state == SCSI_TRANSFER_WAIT_FOR_POST_COUNT_REQ) {
		/* The post-count REQ is distinct from the last byte REQ.  It
		 * generates the successful completion MCI, not a service request.
		 * The following phase remains target-owned but is not published
		 * until the host accepts its next Transfer Info command. */
		status = scsi_transfer_phase_status;
		if (!scsi_transfer_req_asserted) {
			scsiio_target_assert_req("post-count", status);
		}
		/* Keep this target REQ asserted.  It is the retained request for
		 * the next Transfer Info command; no ACK has occurred yet. */
		scsi_transfer_req_retained = TRUE;
		scsi_transfer_retained_req_sequence = scsi_transfer_req_sequence;
		scsi_transfer_retained_phase = scsiio.phase;
		scsi_transfer_retained_host_to_spc = scsicmd_phase_host_to_spc(scsiio.phase);
		scsi_transfer_state = SCSI_TRANSFER_COMPLETED_OR_TERMINATED;
		scsi_target_phase_ready = FALSE;
		scsiio.auxstatus &= (REG8) ~(SCSI_AUX_BSY | SCSI_AUX_CIP | SCSI_AUX_DBR);
		SCSITRACEOUT(("scsitrace post-count-retained req=%u phase=%02x "
		              "direction=%s ack=%u",
		              scsi_transfer_req_sequence, scsi_transfer_retained_phase,
		              scsi_transfer_retained_host_to_spc ? "host-to-spc" : "spc-to-host",
		              scsi_transfer_ack_asserted ? 1 : 0));
		scsiintr_transfer_complete((REG8)scsi_transfer_completion_status);
		return;
	}
	scsi_target_phase_ready = TRUE;
	if (scsi_transfer_state == SCSI_TRANSFER_WAIT_FOR_REQ) {
		if (scsi_transfer_phase_pending) {
			scsi_transfer_phase_pending = FALSE;
			scsi_target_phase_ready = FALSE;
		}
		if (!scsi_transfer_req_asserted) {
			scsiio_target_assert_req("active", scsi_transfer_phase_status
			                                       ? scsi_transfer_phase_status
			                                       : scsicmd_phase_service_status(scsiio.phase));
		}
		scsiio_start_transfer();
		return;
	}
	scsi_target_publish();
	(void)item;
}

static void scsiintr_enqueue(const char *origin, REG8 status, UINT clocks,
                             BOOL record_transfer_result, BOOL target_event) {
	UINT sequence;

	scsi_trace_watchdog();
	if (status == 0x89 && scsiio_transfer_active()) {
		SCSITRACEOUT(("scsitrace invariant service-required-active status=89 "
		              "state=%s req=%u tc=%06x",
		              scsi_transfer_state_name(), scsi_transfer_req_asserted ? 1 : 0,
		              scsiio_transfer_count()));
		return;
	}
	if (status == 0x89 && scsi_trace_data_phase_pending) {
		SCSITRACEOUT(("scsitrace data-phase-request status=89 delta=%u "
		              "cs=%04x ip=%04x",
		              (UINT32)(scsi_trace_clock() - scsi_trace_data_phase_decision_clock), CPU_CS,
		              CPU_IP));
		scsi_trace_data_phase_pending = FALSE;
	}

	if (record_transfer_result) {
		scsi_trace_transfer_result(status);
	}
	if (scsi_transfer_selftest_mode) {
		if (scsi_csr_event_active || scsi_csr_latched) {
			return;
		}
		scsiio.scsistatus = status;
		scsi_csr_latched = TRUE;
		scsiio.auxstatus &= (REG8)~SCSI_AUX_CIP;
		scsi_transfer_selftest_last_csr = status;
		scsi_transfer_selftest_latch_count++;
		return;
	}
	sequence = scsi_trace_enabled ? ++scsi_trace_csr_sequence : 0;
	scsi_trace_csr_record("request", sequence, status, origin);
	if (scsi_csr_event_active || scsi_csr_latched) {
		scsi_trace_csr_record("overrun", sequence, status, origin);
		SCSITRACEOUT(("scsitrace invariant %s-overlap seq=%u status=%02x "
		              "active=%u active_status=%02x latched=%u cs=%04x ip=%04x",
		              target_event ? "target" : "host", sequence, status, scsi_csr_event_active,
		              scsi_csr_event_status, scsi_csr_latched, CPU_CS, CPU_IP));
		scsi_trace_csr_record("drop", sequence, status, origin);
		/* WD33C93 exposes one CSR; a second event is not queued. */
		return;
	}
	scsi_csr_event_active = TRUE;
	scsi_csr_event_status = status;
	scsi_csr_event_sequence = sequence;
	scsi_csr_event_origin = origin;
	scsi_csr_event_clock = scsi_trace_clock();
	scsi_trace_event_watchdog_reported = FALSE;
	nevent_set(NEVENT_SCSIIO, clocks, scsiioint, NEVENT_ABSOLUTE);
	scsi_trace_watchdog_schedule();
	SCSITRACEOUT(("scsitrace request status=%02x phase=%02x cs=%04x ip=%04x", status, scsiio.phase,
	              CPU_CS, CPU_IP));
	TRACEOUT(("scsi schedule intr"));
}

static void scsiintr_transfer_complete(REG8 status) {
	scsiintr_enqueue("transfer-complete", status, 100, TRUE, FALSE);
}

static void scsiintr(const char *origin, REG8 status) {
	scsiintr_enqueue(origin, status, 4000, TRUE, FALSE);
}

static void scsicmd(REG8 cmd) {
	REG8 ret;
	UINT8 id;

	id = scsiio.reg[SCSICTR_DSTID] & 7;
	switch (cmd) {
	case SCSICMD_RESET:
		scsiio.phase = 0;
		scsiio.cmdpos = 0;
		scsiio.rddatpos = 0;
		scsiio.wrdatpos = 0;
		scsiio.auxstatus = 0;
		scsi_command_phase_pending = FALSE;
		scsi_transfer_remaining = 0;
		scsi_transfer_state = SCSI_TRANSFER_IDLE;
		scsi_transfer_req_asserted = FALSE;
		scsi_transfer_ack_asserted = FALSE;
		scsi_transfer_req_retained = FALSE;
		scsi_transfer_single_byte = FALSE;
		scsi_bus_free_pending = FALSE;
		scsiintr("reset", SCSISTAT_RESET);
		break;

	case SCSICMD_NEGATE:
		/* Negate ACK is Level I: only ACK changes here.  The later
			 * bus-free/disconnect event owns its own CSR, if any. */
		(void)scsicmd_negate(id);
		scsiio_initiator_negate_ack();
		scsiio.auxstatus &= (REG8) ~(SCSI_AUX_BSY | SCSI_AUX_DBR);
		scsi_transfer_state = SCSI_TRANSFER_COMPLETED_OR_TERMINATED;
		scsi_transfer_req_retained = FALSE;
		scsi_bus_free_status = (scsiio.reg[SCSICTR_CONTROL] & 0x08) ? 0x85 : 0x80;
		scsi_bus_free_pending = TRUE;
		SCSITRACEOUT(("scsitrace target-disconnect pending status=%02x", scsi_bus_free_status));
		scsi_target_schedule_after_consume();
		break;

	case SCSICMD_SEL:
		scsiio.auxstatus |= SCSI_AUX_BSY;
		ret = scsicmd_select(id);
		if (ret & 0x80) {
			scsi_command_phase_pending = TRUE;
			scsi_target_selection_origin = "select-complete";
		} else {
			scsi_target_selection_origin = "select-error";
		}
		scsi_target_selection_status = (ret & 0x80) ? 0x11 : ret;
		scsi_target_selection_pending = TRUE;
		scsi_target_schedule_after_consume();
		break;

	case SCSICMD_SEL_TR:
		ret = scsicmd_transfer(id, scsiio.reg + SCSICTR_CDB);
		if (ret != 0xff) {
			scsiintr("select-transfer", ret);
		}
		break;

	case SCSICMD_TRANS_INFO:
		scsi_trace_transfer_start(scsiio.phase, scsiio_transfer_count(),
		                          scsiio.phase == SCSIPH_COMMAND ? "m75c2-ar19-pio"
		                                                         : "level2-transfer-info");
		scsi_transfer_remaining = scsi_transfer_single_byte ? 1 : scsiio_transfer_count();
		scsi_transfer_active_phase = scsiio.phase;
		SCSITRACEOUT(("scsitrace transfer-info-accept tc=%06x sbt=%u "
		              "logical_count=%u",
		              scsiio_transfer_count(), scsi_transfer_single_byte ? 1 : 0,
		              scsi_transfer_remaining));
		scsi_transfer_state = SCSI_TRANSFER_WAIT_FOR_REQ;
		scsiio.auxstatus |= SCSI_AUX_BSY;
		scsiio.auxstatus &= (REG8) ~(SCSI_AUX_CIP | SCSI_AUX_DBR);
		if (scsiio.phase == SCSIPH_COMMAND) {
			scsiio.wrdatpos = 0;
			if (scsi_transfer_remaining == 0) {
				SCSITRACEOUT(("scsitrace warning M75c2 Transfer Info "
				              "with TC=0 hardware-pending"));
				scsi_transfer_state = SCSI_TRANSFER_COMPLETED_OR_TERMINATED;
				scsiio.auxstatus &= (REG8)~SCSI_AUX_BSY;
				break;
			}
			SCSITRACEOUT(("scsitrace M75c2 accumulates CDB through DATA "
			              "window count=%u",
			              scsi_transfer_remaining));
		} else {
			if (scsi_transfer_phase_pending && !scsi_target_phase_ready &&
			    !scsi_transfer_req_asserted) {
				SCSITRACEOUT(("scsitrace target-phase-wait phase=%02x tc=%06x "
				              "state=%s",
				              scsiio.phase, scsi_transfer_remaining, scsi_transfer_state_name()));
				if (!scsi_target_phase_delay_pending) {
					scsi_target_phase_delay_pending = TRUE;
					scsi_target_phase_delay_clock = scsi_trace_clock();
					scsi_trace_target_delay_watchdog_reported = FALSE;
					nevent_set(NEVENT_SCSIIO, scsi_target_processing_clocks(),
					           scsiio_target_phase_ready_event, NEVENT_ABSOLUTE);
				}
				return;
			}
			if (scsiio.phase == SCSIPH_STATUS) {
				scsiio.data[0] = scsiio.reg[SCSICTR_STATUS];
			} else if (scsiio.phase == SCSIPH_MSGIN) {
				scsiio.data[0] = 0x00;
			}
		}
		if (scsi_transfer_remaining == 0) {
			SCSITRACEOUT(("scsitrace warning Transfer Info with TC=0 "
			              "phase=%02x hardware-pending",
			              scsiio.phase));
			scsi_transfer_state = SCSI_TRANSFER_COMPLETED_OR_TERMINATED;
			scsiio.auxstatus &= (REG8)~SCSI_AUX_BSY;
			break;
		}
		if (scsi_transfer_phase_pending) {
			scsi_transfer_phase_pending = FALSE;
			scsi_target_phase_ready = FALSE;
		}
		scsiio_start_transfer();
		break;
	}
}

// ----

static BOOL scsiio_is_level2_command(REG8 command) {
	switch (command & 0x7f) {
	case SCSICMD_SEL_ATN:
	case SCSICMD_SEL:
	case SCSICMD_SEL_ATNTR:
	case SCSICMD_SEL_TR:
	case SCSICMD_RESEL_RECV:
	case SCSICMD_RESEL_SEND:
	case SCSICMD_TRANS_INFO:
	case SCSICMD_TRANS_PAD:
		return TRUE;
	}
	return FALSE;
}

static BOOL scsiio_is_information_command(REG8 command) {
	command &= 0x7f;
	return command >= SCSICMD_RECV_CMD && command <= SCSICMD_SEND_INFO;
}

static void scsiio_command_write(REG8 command) {
	REG8 aux;
	BOOL int_pending;
	BOOL level2;
	REG8 command_code;
	BOOL msg = FALSE;
	BOOL cd = FALSE;
	BOOL io = FALSE;

	switch (scsiio.phase) {
	case SCSIPH_DATAIN:
		io = TRUE;
		break;
	case SCSIPH_COMMAND:
		cd = TRUE;
		break;
	case SCSIPH_STATUS:
		cd = TRUE;
		io = TRUE;
		break;
	case SCSIPH_MSGOUT:
		msg = TRUE;
		break;
	case SCSIPH_MSGIN:
		msg = TRUE;
		io = TRUE;
		break;
	}
	aux = scsiio_auxstatus();
	command_code = command & 0x7f;
	level2 = scsiio_is_level2_command(command_code);
	int_pending = (scsi_csr_event_active || scsi_csr_latched);
	SCSITRACEOUT(("scsitrace command-write-pre command=%02x sbt=%u level=%u int=%u lci=%u "
	              "bsy=%u cip=%u dbr=%u csr_pending=%u req=%u ack=%u "
	              "msg=%u cd=%u io=%u tc=%06x state=%s cs=%04x ip=%04x",
	              command, (command & 0x80) ? 1 : 0, level2 ? 2 : 1, (aux & SCSI_AUX_INT) ? 1 : 0,
	              (aux & SCSI_AUX_LCI) ? 1 : 0, (aux & SCSI_AUX_BSY) ? 1 : 0,
	              (aux & SCSI_AUX_CIP) ? 1 : 0, (aux & SCSI_AUX_DBR) ? 1 : 0, int_pending ? 1 : 0,
	              scsi_transfer_req_asserted ? 1 : 0, scsi_transfer_ack_asserted ? 1 : 0,
	              msg ? 1 : 0, cd ? 1 : 0, io ? 1 : 0, scsiio_transfer_count(),
	              scsi_transfer_state_name(), CPU_CS, CPU_IP));
	if (int_pending) {
		scsiio.auxstatus |= SCSI_AUX_LCI;
		SCSITRACEOUT(("scsitrace command-ignored reason=int-pending "
		              "command=%02x state=%s tc=%06x",
		              command, scsi_transfer_state_name(), scsiio_transfer_count()));
		return;
	}
	if (scsiio_transfer_active() && level2) {
		scsiio.auxstatus |= SCSI_AUX_LCI;
		SCSITRACEOUT(("scsitrace command-ignored reason=active-level2 "
		              "command=%02x state=%s tc=%06x",
		              command, scsi_transfer_state_name(), scsiio_transfer_count()));
		return;
	}
	scsiio.reg[SCSICTR_CMD] = command;
	scsi_transfer_single_byte = (command & 0x80) && (command_code == SCSICMD_TRANS_INFO ||
	                                                 scsiio_is_information_command(command_code));
	scsiio.auxstatus |= SCSI_AUX_CIP;
	SCSITRACEOUT(("scsitrace command-accepted command=%02x sbt=%u state=%s "
	              "tc=%06x",
	              command, scsi_transfer_single_byte ? 1 : 0, scsi_transfer_state_name(),
	              scsiio_transfer_count()));
	if (scsi_transfer_state == SCSI_TRANSFER_COMPLETED_OR_TERMINATED) {
		scsi_transfer_state = SCSI_TRANSFER_IDLE;
	}
	scsicmd(command_code);
	scsiio.auxstatus &= (REG8)~SCSI_AUX_CIP;
}

static void IOOUTCALL scsiio_occ0(UINT port, REG8 dat) {
	scsi_trace_watchdog();
	scsiio.port = dat;
	SCSITRACEOUT(("scsitrace out port=0cc0 ar=%02x cs=%04x ip=%04x", dat, CPU_CS, CPU_IP));
	(void)port;
}

static void IOOUTCALL scsiio_occ2(UINT port, REG8 dat) {
	scsi_trace_watchdog();
	UINT8 bit;

	if (scsiio.port < 0x40) {
		SCSITRACEOUT(("scsitrace out port=0cc2 ar=%02x data=%02x cs=%04x ip=%04x", scsiio.port, dat,
		              CPU_CS, CPU_IP));
		TRACEOUT(("scsi ctrl write %s(%.2x) %.2x", scsictr[scsiio.port], scsiio.port, dat));
	}
	if (scsiio.port == SCSICTR_DATA) {
		scsiio_data_write(dat);
		return;
	}
	if (scsiio.port == SCSICTR_CMD) {
		scsiio_command_write(dat);
		return;
	}
	if (scsiio.port <= 0x19) {
		if (scsiio.port >= SCSICTR_TRANSCNT && scsiio.port <= SCSICTR_TRANSCNT + 2) {
			UINT tc_before = scsiio_transfer_count();
			scsiio.reg[scsiio.port] = dat;
			SCSITRACEOUT(("scsitrace tc-write ar=%02x data=%02x before=%06x "
			              "after=%06x sbt=%u cs=%04x ip=%04x",
			              scsiio.port, dat, tc_before, scsiio_transfer_count(),
			              scsi_transfer_single_byte ? 1 : 0, CPU_CS, CPU_IP));
		} else {
			scsiio.reg[scsiio.port] = dat;
		}
		/* COMMAND and DATA are fixed windows; all other registers advance. */
		if (scsiio.port != SCSICTR_CMD && scsiio.port != SCSICTR_DATA) {
			scsiio.port++;
		}
	} else {
		SCSITRACEOUT(("scsitrace out port=0cc2 ar=%02x data=%02x cs=%04x ip=%04x", scsiio.port, dat,
		              CPU_CS, CPU_IP));
		switch (scsiio.port) {
		case SCSICTR_MEMBANK:
			scsiio.membank = dat;
			break;
		case SCSICTR_MEMWND:
			scsiio.memwnd = dat;
			break;
		case SCSICTR_PKGID:
		case SCSICTR_RESENT:
		case SCSICTR_FIFO_CTRL:
		case SCSICTR_FIFO_STATUS:
			SCSITRACEOUT(("scsitrace warning AR=%02x write is hardware-pending "
			              "data=%02x cs=%04x ip=%04x",
			              scsiio.port, dat, CPU_CS, CPU_IP));
			break;

		case 0x3f:
			bit = 1 << (dat & 7);
			if (dat & 8) {
				scsiio.datmap |= bit;
			} else {
				if (scsiio.datmap & bit) {
					scsiio.datmap &= ~bit;
					if (bit == (1 << 1)) {
						scsiio.wrdatpos = 0;
					} else if (bit == (1 << 5)) {
						scsiio.rddatpos = 0;
					}
				}
			}
			break;

		default:
			/* Undefined AR values are held, not auto-incremented. */
			if (scsiio.port >= 0x1a && scsiio.port < 0x30) {
				scsiio_warn_reserved_register("write");
			}
			break;
		}
	}
	(void)port;
}

static void IOOUTCALL scsiio_occ4(UINT port, REG8 dat) {
	SCSITRACEOUT(("scsitrace out port=0cc4 data=%02x cs=%04x ip=%04x", dat, CPU_CS, CPU_IP));
	TRACEOUT(("scsiio_occ4 %.2x", dat));
	if (dat & SCSI_C4_DMER) {
		/* PCPLUS selects polled I/O; DMA remains deliberately disabled. */
		SCSITRACEOUT(("scsitrace 0cc4 DMER reset (PIO-only)"));
	}
	if (dat & (SCSI_C4_TCMS | SCSI_C4_TCMR | SCSI_C4_TCIR | SCSI_C4_DMES)) {
		SCSITRACEOUT(("scsitrace warning 0cc4 bits=%02x hardware-pending",
		              dat & (SCSI_C4_TCMS | SCSI_C4_TCMR | SCSI_C4_TCIR | SCSI_C4_DMES)));
	}
	(void)port;
}

static void IOOUTCALL scsiio_occ6(UINT port, REG8 dat) {
	scsi_trace_transfer_data_port_access();

	SCSITRACEOUT(("scsitrace out port=0cc6 data=%02x ar=%02x cs=%04x ip=%04x", dat, scsiio.port,
	              CPU_CS, CPU_IP));
	if (scsiio.phase == SCSIPH_DATAOUT) {
		scsiio_dataout_store_byte(dat, "0cc6");
		scsiio_legacy_dataout_complete();
	}
	(void)port;
}

void scsiio_legacy_dataout_selftest_byte(REG8 dat) {
	scsiio_occ6(0, dat);
}

static REG8 IOINPCALL scsiio_icc0(UINT port) {
	scsi_trace_watchdog();
	REG8 ret;

	ret = scsiio_auxstatus();
	SCSITRACEOUT(("scsitrace in port=0cc0 aux=%02x ar=%02x cs=%04x ip=%04x", ret, scsiio.port,
	              CPU_CS, CPU_IP));
	(void)port;
	return (ret);
}

static REG8 IOINPCALL scsiio_icc2(UINT port) {
	scsi_trace_watchdog();
	REG8 ret;

	switch (scsiio.port) {
	case SCSICTR_STATUS:
		if (scsi_csr_latched) {
			REG8 consumed = scsiio.scsistatus;
			BOOL repeated_data = scsi_transfer_state == SCSI_TRANSFER_COMPLETED_OR_TERMINATED &&
			                     scsi_transfer_phase_pending && consumed == 0x19 &&
			                     scsi_transfer_phase_status == 0x89;

			scsi_trace_csr_record("hostread", scsi_csr_latched_sequence, consumed,
			                      scsi_csr_latched_origin);
			SCSITRACEOUT(("scsitrace csr-hostread-state status=%02x req=%u "
			              "req_id=%u retained=%u ack=%u",
			              consumed, scsi_transfer_req_asserted ? 1 : 0, scsi_transfer_req_sequence,
			              scsi_transfer_req_retained ? 1 : 0, scsi_transfer_ack_asserted ? 1 : 0));
			scsi_csr_latched = FALSE;
			scsi_trace_latched_watchdog_reported = FALSE;
			scsiio.auxstatus &= (REG8)~SCSI_AUX_INT;
			scsi_transfer_state = SCSI_TRANSFER_IDLE;
			if (!repeated_data) {
				scsi_target_schedule_after_consume();
			}
		}
		SCSITRACEOUT(("scsitrace in port=0cc2 ar=%02x status=%02x cs=%04x ip=%04x", scsiio.port,
		              scsiio.scsistatus, CPU_CS, CPU_IP));
		scsiio.port++;
		return (scsiio.scsistatus);

	case SCSICTR_DATA:
		SCSITRACEOUT(("scsitrace in port=0cc2 ar=%02x data-window "
		              "cs=%04x ip=%04x",
		              scsiio.port, CPU_CS, CPU_IP));
		return scsiio_data_read();

	case SCSICTR_MEMBANK:
		SCSITRACEOUT(("scsitrace in port=0cc2 ar=%02x data=%02x cs=%04x ip=%04x", scsiio.port,
		              scsiio.membank, CPU_CS, CPU_IP));
		return (scsiio.membank);

	case SCSICTR_MEMWND:
		SCSITRACEOUT(("scsitrace in port=0cc2 ar=%02x data=%02x cs=%04x ip=%04x", scsiio.port,
		              scsiio.memwnd, CPU_CS, CPU_IP));
		return (scsiio.memwnd);

	case SCSICTR_RESENT:
		SCSITRACEOUT(("scsitrace in port=0cc2 ar=%02x data=%02x cs=%04x ip=%04x", scsiio.port,
		              scsiio.resent, CPU_CS, CPU_IP));
		return (scsiio.resent);

	case SCSICTR_PKGID:
	case SCSICTR_FIFO_CTRL:
	case SCSICTR_FIFO_STATUS:
		SCSITRACEOUT(("scsitrace warning AR=%02x read is hardware-pending "
		              "cs=%04x ip=%04x",
		              scsiio.port, CPU_CS, CPU_IP));
		return (0xff);

	case 0x36:
		return (0); // Reject a second controller claiming the same target.
	}
	if (scsiio.port >= 0x1a && scsiio.port < 0x30) {
		scsiio_warn_reserved_register("read");
	}
	if (scsiio.port <= 0x19) {
		ret = scsiio.reg[scsiio.port];
		SCSITRACEOUT(("scsitrace in port=0cc2 ar=%02x data=%02x cs=%04x ip=%04x", scsiio.port, ret,
		              CPU_CS, CPU_IP));
		TRACEOUT(("scsi ctrl read %s %.2x [%.4x:%.4x]", scsictr[scsiio.port], ret, CPU_CS, CPU_IP));
		scsiio.port++;
		return (ret);
	}
	SCSITRACEOUT(
	    ("scsitrace in port=0cc2 ar=%02x data=ff cs=%04x ip=%04x", scsiio.port, CPU_CS, CPU_IP));
	(void)port;
	return (0xff);
}

static REG8 IOINPCALL scsiio_icc4(UINT port) {
	SCSITRACEOUT(("scsitrace in port=0cc4 data=00 cs=%04x ip=%04x", CPU_CS, CPU_IP));
	TRACEOUT(("scsiio_icc4"));
	(void)port;
	return (0x00);
}

static REG8 IOINPCALL scsiio_icc6(UINT port) {
	REG8 ret;

	scsi_trace_transfer_data_port_access();
	if (scsiio.rddatpos >= sizeof(scsiio.data)) {
		SCSITRACEOUT(("scsitrace invariant data-read-window-overrun "
		              "source=0cc6 index=%u capacity=%u phase=%02x",
		              scsiio.rddatpos, (UINT)sizeof(scsiio.data), scsiio.phase));
		return 0xff;
	}
	ret = scsiio.data[scsiio.rddatpos];
	scsiio_trace_block_delivered_byte(ret);
	SCSITRACEOUT(("scsitrace in port=0cc6 data=%02x ar=%02x cs=%04x ip=%04x", ret, scsiio.port,
	              CPU_CS, CPU_IP));
	scsiio.rddatpos++;
	if ((scsiio.phase == SCSIPH_DATAIN) && (scsiio.rddatpos >= scsiio.cmdpos)) {
		scsiio.phase = SCSIPH_STATUS;
		scsiintr("legacy-data-complete", 0x8b);
	}
	(void)port;
	return (ret);
}

// ----

void scsiio_reset(void) {
	nevent_reset(NEVENT_SCSIWATCHDOG);
	scsi_trace_watchdog_scheduled = FALSE;
	ZeroMemory(&scsiio, sizeof(scsiio));
	scsicmd_block_reset_state();
	scsi_csr_latched = FALSE;
	scsi_csr_event_active = FALSE;
	scsi_csr_event_status = 0;
	scsi_csr_event_clock = 0;
	scsi_csr_latched_clock = 0;
	scsi_trace_data_phase_decision_clock = 0;
	scsi_trace_data_phase_pending = FALSE;
	scsi_trace_event_watchdog_reported = FALSE;
	scsi_trace_latched_watchdog_reported = FALSE;
	scsi_trace_data_phase_watchdog_reported = FALSE;
	scsi_trace_target_delay_watchdog_reported = FALSE;
	scsi_target_phase_delay_clock = 0;
	scsi_trace_data_phase_request_missing_count = 0;
	scsi_trace_csr_sequence = 0;
	scsi_csr_event_sequence = 0;
	scsi_csr_event_origin = NULL;
	scsi_csr_latched_sequence = 0;
	scsi_csr_latched_origin = NULL;
	scsi_target_selection_pending = FALSE;
	scsi_target_selection_status = 0;
	scsi_target_selection_origin = NULL;
	scsi_target_phase_delay_pending = FALSE;
	scsi_command_phase_pending = FALSE;
	scsi_transfer_phase_pending = FALSE;
	scsi_transfer_phase_status = 0;
	scsi_target_phase_ready = FALSE;
	scsi_transfer_state = SCSI_TRANSFER_IDLE;
	scsi_transfer_req_asserted = FALSE;
	scsi_transfer_ack_asserted = FALSE;
	scsi_transfer_req_retained = FALSE;
	scsi_transfer_retained_req_sequence = 0;
	scsi_transfer_retained_phase = 0;
	scsi_transfer_retained_host_to_spc = FALSE;
	scsi_transfer_single_byte = FALSE;
	scsi_bus_free_pending = FALSE;
	scsi_bus_free_status = 0;
	scsi_transfer_req_sequence = 0;
	scsi_transfer_completion_status = 0;
	scsi_transfer_active_phase = 0;
	scsi_transfer_remaining = 0;
	scsi_transfer_selftest_last_csr = 0;
	scsi_transfer_selftest_latch_count = 0;
	scsi_transfer_selftest_transferred_bytes = 0;
	scsi_trace_transfer_active = FALSE;
	scsi_trace_transfer_phase = 0;
	scsi_trace_transfer_count = 0;
	scsi_trace_transfer_ar19_accesses = 0;
	scsi_trace_transfer_ar19_reads = 0;
	scsi_trace_transfer_ar19_writes = 0;
	scsi_trace_transfer_data_port_accesses = 0;
	scsi_trace_transfer_irq_requests = 0;
	scsi_trace_transfer_irq_assertions = 0;
	scsi_trace_transfer_result_pending = FALSE;
	scsi_trace_transfer_result_status = 0;
	scsi_trace_transfer_cdb_length = 0;
	ZeroMemory(scsi_trace_transfer_cdb, sizeof(scsi_trace_transfer_cdb));
	scsi_trace_transfer_source = NULL;
	if (pccore.hddif & PCHDD_SCSI) {
		/* INT2/IRQ6 is the VA bus choice that does not collide with SASI. */
		scsiio.resent = (2 << 3) + (7 << 0);
		/*
		 * PCPLUS.SYS supplies the $SCSIBIOS service through the board I/O
		 * interface.  The PC-88VA SCSI55 guidance permits the board ROM to
		 * be disconnected, so do not claim a VA system-memory window for it.
		 */
		TRACEOUT(("SCSI board ROM detached; use PCPLUS $SCSIBIOS"));
	}
}

BOOL scsiio_transfer_selftest(void) {
	REG8 value;
	UINT retained_sequence;
	UINT latch_count;
	UINT index;
	BYTE boundary_value;
	BOOL ok = TRUE;

#define SCSI_SELFTEST_CHECK(name, expression)                                                      \
	do {                                                                                           \
		if (!(expression)) {                                                                       \
			fprintf(stderr, "selftest: %s FAIL\n", name);                                          \
			ok = FALSE;                                                                            \
		} else {                                                                                   \
			fprintf(stderr, "selftest: %s PASS\n", name);                                          \
		}                                                                                          \
	} while (0)
#define SCSI_SELFTEST_RESET()                                                                      \
	do {                                                                                           \
		scsiio_reset();                                                                            \
		scsi_transfer_selftest_last_csr = 0;                                                       \
		scsi_transfer_selftest_latch_count = 0;                                                    \
		scsi_transfer_selftest_transferred_bytes = 0;                                              \
	} while (0)

	scsi_transfer_selftest_mode = TRUE;
	SCSI_SELFTEST_RESET();
	SCSI_SELFTEST_CHECK("success_status_encodes_19_1b_1f",
	                    scsiio_success_status_from_service(0x89) == 0x19 &&
	                        scsiio_success_status_from_service(0x8b) == 0x1b &&
	                        scsiio_success_status_from_service(0x8f) == 0x1f);
	SCSI_SELFTEST_CHECK("transfer_count_register_order", (scsiio.reg[SCSICTR_TRANSCNT + 0] = 0x01,
	                                                      scsiio.reg[SCSICTR_TRANSCNT + 1] = 0x02,
	                                                      scsiio.reg[SCSICTR_TRANSCNT + 2] = 0x03,
	                                                      scsiio_transfer_count() == 0x010203));

	/* The PIO window is exactly 64 KiB.  Keep 65535 and 65536 valid,
	 * and reject an attempted access at 65537 instead of wrapping. */
	SCSI_SELFTEST_RESET();
	scsiio.phase = SCSIPH_DATAIN;
	scsiio.cmdpos = 65535;
	scsiio.rddatpos = 65534;
	scsiio.data[65534] = 0x5e;
	scsi_transfer_state = SCSI_TRANSFER_BYTE_PENDING;
	scsi_transfer_remaining = 1;
	scsi_transfer_req_asserted = TRUE;
	scsiio.auxstatus |= SCSI_AUX_DBR;
	boundary_value = scsiio_data_read();
	SCSI_SELFTEST_CHECK("read_65535_bytes_boundary",
	                    boundary_value == 0x5e && scsi_transfer_selftest_transferred_bytes == 1 &&
	                        scsi_transfer_state == SCSI_TRANSFER_WAIT_FOR_POST_COUNT_REQ);

	SCSI_SELFTEST_RESET();
	scsiio.phase = SCSIPH_DATAIN;
	scsiio.cmdpos = sizeof(scsiio.data);
	scsiio.rddatpos = 0;
	for (index = 0; index < sizeof(scsiio.data); index++) {
		scsiio.data[index] = (BYTE)((index + (index >> 8) + 0x17) & 0xff);
	}
	scsi_transfer_state = SCSI_TRANSFER_BYTE_PENDING;
	scsi_transfer_remaining = sizeof(scsiio.data);
	scsi_transfer_req_asserted = TRUE;
	scsiio.auxstatus |= SCSI_AUX_DBR;
	boundary_value = 0;
	for (index = 0; index < sizeof(scsiio.data); index++) {
		boundary_value = scsiio_data_read();
	}
	SCSI_SELFTEST_CHECK("read_65536_bytes_boundary",
	                    scsi_transfer_selftest_transferred_bytes == sizeof(scsiio.data) &&
	                        boundary_value == scsiio.data[sizeof(scsiio.data) - 1]);

	SCSI_SELFTEST_RESET();
	scsiio.phase = SCSIPH_DATAIN;
	scsiio.cmdpos = sizeof(scsiio.data) + 1;
	scsiio.rddatpos = sizeof(scsiio.data);
	scsi_transfer_state = SCSI_TRANSFER_BYTE_PENDING;
	scsi_transfer_remaining = 1;
	scsi_transfer_req_asserted = TRUE;
	scsiio.auxstatus |= SCSI_AUX_DBR;
	boundary_value = scsiio_data_read();
	SCSI_SELFTEST_CHECK("read_65537_bytes_boundary",
	                    boundary_value == 0xff && scsiio.rddatpos == sizeof(scsiio.data) &&
	                        scsi_transfer_selftest_transferred_bytes == 0);

	SCSI_SELFTEST_RESET();
	scsiio.phase = SCSIPH_STATUS;
	scsi_transfer_state = SCSI_TRANSFER_WAIT_FOR_REQ;
	scsi_transfer_remaining = 1;
	scsi_transfer_req_asserted = FALSE;
	scsiio_start_transfer();
	SCSI_SELFTEST_CHECK("transfer_info_waits_for_req",
	                    scsi_transfer_state == SCSI_TRANSFER_WAIT_FOR_REQ &&
	                        !scsi_transfer_req_asserted && (scsiio.auxstatus & SCSI_AUX_BSY) &&
	                        !(scsiio.auxstatus & SCSI_AUX_DBR));

	SCSI_SELFTEST_RESET();
	scsi_transfer_state = SCSI_TRANSFER_WAIT_FOR_REQ;
	scsi_transfer_req_asserted = TRUE;
	scsiintr_enqueue("selftest-service", 0x89, 0, FALSE, TRUE);
	SCSI_SELFTEST_CHECK("transfer_info_does_not_raise_service_required_while_active",
	                    scsi_transfer_selftest_latch_count == 0 && !scsi_csr_latched);
	SCSI_SELFTEST_CHECK("service_required_requires_no_active_level2_command",
	                    scsi_transfer_state == SCSI_TRANSFER_WAIT_FOR_REQ && !scsi_csr_latched);

	SCSI_SELFTEST_RESET();
	scsiio.phase = SCSIPH_DATAIN;
	scsiio.cmdpos = 1;
	scsiio.rddatpos = 0;
	scsiio.data[0] = 0xaa;
	scsi_transfer_state = SCSI_TRANSFER_BYTE_PENDING;
	scsi_transfer_remaining = 2;
	scsi_transfer_req_asserted = TRUE;
	scsiio.auxstatus |= SCSI_AUX_DBR;
	value = scsiio_data_read();
	SCSI_SELFTEST_CHECK("transfer_info_phase_change_before_tc_zero_returns_4mci",
	                    value == 0xaa && scsi_transfer_selftest_last_csr == 0x4b &&
	                        scsi_transfer_remaining == 1 && scsi_transfer_req_asserted &&
	                        scsi_transfer_req_retained &&
	                        scsi_transfer_selftest_transferred_bytes == 1);

	SCSI_SELFTEST_RESET();
	scsiio.phase = SCSIPH_STATUS;
	scsiio.data[0] = 0x00;
	scsiio.rddatpos = 0;
	scsi_transfer_state = SCSI_TRANSFER_BYTE_PENDING;
	scsi_transfer_remaining = 1;
	scsi_transfer_req_asserted = TRUE;
	scsiio.auxstatus |= SCSI_AUX_DBR;
	value = scsiio_data_read();
	SCSI_SELFTEST_CHECK("transfer_info_tc_zero_waits_for_next_req",
	                    value == 0x00 &&
	                        scsi_transfer_state == SCSI_TRANSFER_WAIT_FOR_POST_COUNT_REQ &&
	                        scsi_transfer_selftest_latch_count == 0 && !scsi_transfer_req_asserted);
	scsiio_target_phase_ready_event(NULL);
	retained_sequence = scsi_transfer_retained_req_sequence;
	SCSI_SELFTEST_CHECK("transfer_info_completion_uses_next_req_mci",
	                    scsi_transfer_selftest_last_csr == 0x1f &&
	                        scsi_transfer_selftest_latch_count == 1 && scsi_transfer_req_retained &&
	                        scsi_transfer_req_asserted &&
	                        scsi_transfer_req_sequence == retained_sequence &&
	                        !scsi_transfer_ack_asserted && !(scsiio.auxstatus & SCSI_AUX_BSY));
	SCSI_SELFTEST_CHECK("status_to_message_in_returns_1f",
	                    scsi_transfer_phase_status == 0x8f &&
	                        scsi_transfer_completion_status == 0x1f);

	scsiio.port = SCSICTR_STATUS;
	value = scsiio_icc2(0);
	SCSI_SELFTEST_CHECK("csr_read_preserves_retained_post_count_req",
	                    value == 0x1f && !scsi_csr_latched && scsi_transfer_req_retained &&
	                        scsi_transfer_req_asserted &&
	                        scsi_transfer_req_sequence == retained_sequence);

	scsi_transfer_state = SCSI_TRANSFER_WAIT_FOR_REQ;
	scsi_transfer_phase_pending = FALSE;
	scsi_transfer_remaining = 1;
	scsiio.phase = SCSIPH_MSGIN;
	scsiio.rddatpos = 0;
	scsiio.data[0] = 0x00;
	scsiio.auxstatus &= (REG8) ~(SCSI_AUX_BSY | SCSI_AUX_DBR);
	scsi_transfer_selftest_transferred_bytes = 0;
	scsiio_start_transfer();
	SCSI_SELFTEST_CHECK("post_count_req_survives_completion_interrupt",
	                    scsi_transfer_req_asserted &&
	                        scsi_transfer_req_sequence == retained_sequence &&
	                        scsi_transfer_state == SCSI_TRANSFER_BYTE_PENDING);
	value = scsiio_data_read();
	SCSI_SELFTEST_CHECK("next_transfer_uses_same_req_id",
	                    value == 0x00 && scsi_transfer_selftest_last_csr == 0x20 &&
	                        scsi_transfer_selftest_transferred_bytes == 1);
	SCSI_SELFTEST_CHECK("message_in_transfer_returns_20",
	                    scsi_transfer_selftest_last_csr == 0x20 && !scsi_transfer_req_asserted &&
	                        !scsi_transfer_req_retained && scsi_transfer_ack_asserted &&
	                        scsi_transfer_selftest_latch_count == 2);
	SCSI_SELFTEST_CHECK("message_in_does_not_wait_for_additional_req",
	                    !scsi_target_phase_delay_pending && !scsi_transfer_phase_pending);
	SCSI_SELFTEST_CHECK("message_in_holds_ack_until_negate_ack", scsi_transfer_ack_asserted);

	scsiio.port = SCSICTR_STATUS;
	(void)scsiio_icc2(0);
	latch_count = scsi_transfer_selftest_latch_count;
	scsiio_command_write(SCSICMD_NEGATE);
	SCSI_SELFTEST_CHECK("negate_ack_clears_ack_without_direct_interrupt",
	                    !scsi_transfer_ack_asserted &&
	                        scsi_transfer_selftest_latch_count == latch_count && !scsi_csr_latched);

	SCSI_SELFTEST_RESET();
	scsi_transfer_state = SCSI_TRANSFER_WAIT_FOR_REQ;
	scsi_transfer_req_asserted = TRUE;
	scsi_transfer_ack_asserted = TRUE;
	scsiio.auxstatus |= SCSI_AUX_BSY;
	scsiio_command_write(SCSICMD_NEGATE);
	SCSI_SELFTEST_CHECK("level1_command_is_not_rejected_only_because_level2_was_active",
	                    !scsi_transfer_ack_asserted && !(scsiio.auxstatus & SCSI_AUX_LCI));

	SCSI_SELFTEST_RESET();
	scsiintr_enqueue("selftest-first", 0x1b, 0, FALSE, FALSE);
	scsiintr_enqueue("selftest-second", 0x1f, 0, FALSE, FALSE);
	SCSI_SELFTEST_CHECK("csr_latch_is_stable_while_int_pending",
	                    scsi_transfer_selftest_last_csr == 0x1b &&
	                        scsi_transfer_selftest_latch_count == 1 && scsi_csr_latched);
	SCSI_SELFTEST_RESET();
	scsiintr_enqueue("selftest-pending", 0x1b, 0, FALSE, FALSE);
	scsiio_command_write(SCSICMD_TRANS_INFO);
	SCSI_SELFTEST_CHECK("command_during_int_pending_is_ignored",
	                    scsi_transfer_selftest_last_csr == 0x1b &&
	                        (scsiio.auxstatus & SCSI_AUX_LCI) &&
	                        scsi_transfer_selftest_latch_count == 1);
	SCSI_SELFTEST_CHECK("ignored_command_sets_lci", (scsiio_auxstatus() & SCSI_AUX_LCI) != 0);

	SCSI_SELFTEST_RESET();
	scsiio.phase = SCSIPH_COMMAND;
	scsiio.reg[SCSICTR_TRANSCNT + 2] = 0x24;
	scsi_transfer_req_asserted = TRUE;
	scsiio_command_write((REG8)(0x80 | SCSICMD_TRANS_INFO));
	SCSI_SELFTEST_CHECK("single_byte_transfer_command_semantics",
	                    scsi_transfer_single_byte && scsi_transfer_remaining == 1 &&
	                        scsiio_transfer_count() == 0x24);

	scsi_transfer_selftest_mode = FALSE;
	scsiio_reset();
#undef SCSI_SELFTEST_CHECK
#undef SCSI_SELFTEST_RESET
	return ok ? SUCCESS : FAILURE;
}

void scsiio_bind(void) {
	if (pccore.hddif & PCHDD_SCSI) {
		iocore_attachout(0x0cc0, scsiio_occ0);
		iocore_attachout(0x0cc2, scsiio_occ2);
		iocore_attachout(0x0cc4, scsiio_occ4);
		iocore_attachout(0x0cc6, scsiio_occ6);
		iocore_attachinp(0x0cc0, scsiio_icc0);
		iocore_attachinp(0x0cc2, scsiio_icc2);
		iocore_attachinp(0x0cc4, scsiio_icc4);
		iocore_attachinp(0x0cc6, scsiio_icc6);
	}
}
