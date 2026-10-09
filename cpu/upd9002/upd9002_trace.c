/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "compiler.h"
#include "cpucore.h"
#include "upd9002_trace.h"

typedef struct {
	FILE *stream;
	uint32_t remaining;
	uint32_t step;
	uint32_t event;
	int32_t before_clock;
	uint16_t before_cs;
	uint16_t before_ip;
} UPD9002_TRACE_STATE;

static UPD9002_TRACE_STATE trace_state;
typedef struct {
	FILE *stream;
	BOOL active;
	BOOL cmdreq_windows_only;
	UINT16 start_ip;
	UINT8 before_047e;
	uint32_t instruction;
	uint32_t window_count;
	uint32_t current_window;
	uint32_t presentation_instruction;
	uint32_t presentation_clock;
	uint32_t ba_instruction;
	uint32_t ba_clock;
	uint32_t presentation_wait_instruction;
	uint32_t presentation_wait_clock;
	BOOL presentation_wait_seen;
	uint32_t last_wait_instruction;
	uint32_t last_wait_clock;
	BOOL wait_seen;
	BOOL wait_logged;
	BOOL window_active;
	BOOL ba_seen;
	BOOL consumer_seen;
	UINT16 consumer_ip;
	BOOL capture_done;
} UPD9002_GUEST_TRACE_STATE;

static UPD9002_GUEST_TRACE_STATE guest_trace_state;

static BOOL guest_trace_watch_ip(UINT16 ip) {
	switch (ip) {
	case 0x1742:
	case 0x1747:
	case 0x1791:
	case 0x1972:
	case 0x1975:
	case 0x19a7:
	case 0x19bb:
	case 0x19c6:
	case 0x19c8:
	case 0x1b60:
	case 0x1ba1:
	case 0x1bfc:
	case 0x1c05:
	case 0x1c0e:
	case 0x1c14:
	case 0x1c32:
	case 0x1c34:
	case 0x1c37:
	case 0x1cbd:
	case 0x1ccd:
	case 0x1d67:
		return (TRUE);
	default:
		return (FALSE);
	}
}

static const char *guest_trace_label(UINT16 ip) {
	switch (ip) {
	case 0x19bb:
	case 0x19c6:
		return ("phase-compare");
	case 0x1b60:
		return ("transfer-path-1");
	case 0x1ba1:
		return ("transfer-path-2");
	case 0x1c14:
	case 0x1c37:
		return ("transfer-setup");
	case 0x1c32:
	case 0x1c34:
		return ("transfer-info-command");
	case 0x1742:
	case 0x1747:
	case 0x1791:
	case 0x19a7:
	case 0x1bfc:
	case 0x1c05:
	case 0x1c0e:
	case 0x1cbd:
	case 0x1ccd:
	case 0x1d67:
		return ("status-047e");
	default:
		return ("handoff");
	}
}

static BOOL guest_trace_047e_writer(UINT16 ip) {
	return (ip == 0x1747) || (ip == 0x1bfc) || (ip == 0x1ccd) || (ip == 0x1d67);
}

static void guest_trace_log(const char *event, UINT16 ip, UINT16 next_ip, UINT8 before_047e,
                            UINT8 after_047e) {
	if (guest_trace_state.stream == NULL) {
		return;
	}
	fprintf(guest_trace_state.stream,
	        "scsi-guest-trace event=%s label=%s ip=%04x next_ip=%04x "
	        "cs=%04x phys=%05x 047e_before=%02x 047e_after=%02x "
	        "ax=%04x bx=%04x cx=%04x dx=%04x si=%04x di=%04x "
	        "bp=%04x sp=%04x flags=%04x\n",

	        event, guest_trace_label(ip), ip, next_ip, CPU_CS,
	        (unsigned)((CS_BASE + 0x047e) & CPU_ADRSMASK), before_047e, after_047e, CPU_AX, CPU_BX,
	        CPU_CX, CPU_DX, CPU_SI, CPU_DI, CPU_BP, CPU_SP, CPU_FLAG);
}

static uint32_t guest_trace_clock(void) {
	return (uint32_t)(CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK);
}

static void guest_trace_cmdreq_log(const char *event, UINT16 ip, UINT16 next_ip, UINT8 before_047e,
                                   UINT8 after_047e) {
	if (guest_trace_state.stream == NULL) {
		return;
	}
	fprintf(guest_trace_state.stream,
	        "scsi-cmdreq-window event=%s window=%u step=%08x clock=%08x "
	        "label=%s ip=%04x next_ip=%04x cs=%04x phys=%05x "
	        "047e_before=%02x 047e_after=%02x "
	        "ax=%04x bx=%04x cx=%04x dx=%04x si=%04x di=%04x "
	        "bp=%04x sp=%04x flags=%04x\n",
	        event, guest_trace_state.current_window, guest_trace_state.instruction,
	        guest_trace_clock(), guest_trace_label(ip), ip, next_ip, CPU_CS,
	        (unsigned)((CS_BASE + 0x047e) & CPU_ADRSMASK), before_047e, after_047e, CPU_AX, CPU_BX,
	        CPU_CX, CPU_DX, CPU_SI, CPU_DI, CPU_BP, CPU_SP, CPU_FLAG);
}

static void guest_trace_cmdreq_summary(const char *consumer, UINT16 consumer_ip, UINT16 next_ip) {
	if (guest_trace_state.stream == NULL || !guest_trace_state.ba_seen) {
		return;
	}
	fprintf(guest_trace_state.stream,
	        "scsi-cmdreq-window-summary window=%u consumer=%s consumer_ip=%04x "
	        "next_ip=%04x presentation_step=%08x presentation_clock=%08x "
	        "presentation_wait_seen=%u presentation_wait_step=%08x "
	        "presentation_wait_clock=%08x write_step=%08x write_clock=%08x "
	        "wait_after_presentation=%u wait_step=%08x wait_clock=%08x "
	        "write_to_consume_steps=%08x write_to_consume_clocks=%08x\n",
	        guest_trace_state.current_window, consumer, consumer_ip, next_ip,
	        guest_trace_state.presentation_instruction, guest_trace_state.presentation_clock,
	        guest_trace_state.presentation_wait_seen ? 1 : 0,
	        guest_trace_state.presentation_wait_instruction,
	        guest_trace_state.presentation_wait_clock, guest_trace_state.ba_instruction,
	        guest_trace_state.ba_clock, guest_trace_state.wait_seen ? 1 : 0,
	        guest_trace_state.last_wait_instruction, guest_trace_state.last_wait_clock,
	        guest_trace_state.instruction - guest_trace_state.ba_instruction,
	        guest_trace_clock() - guest_trace_state.ba_clock);
	guest_trace_state.window_active = FALSE;
	if (guest_trace_state.current_window >= 2) {
		guest_trace_state.capture_done = TRUE;
		fprintf(guest_trace_state.stream, "scsi-cmdreq-windows-complete windows=%u\n",
		        guest_trace_state.current_window);
	}
}

static const char *origin_name(uint32_t origin) {
	switch (origin) {
	case UPD9002_TRACE_ORIGIN_CPU:
		return "cpu";
	case UPD9002_TRACE_ORIGIN_DMA:
		return "dma";
	case UPD9002_TRACE_ORIGIN_DEVICE:
		return "device";
	default:
		return "invalid";
	}
}

void upd9002_trace_start(FILE *stream, uint32_t steps) {
	ZeroMemory(&trace_state, sizeof(trace_state));
	if ((stream != NULL) && (steps != 0)) {
		trace_state.stream = stream;
		trace_state.remaining = steps;
		fprintf(stream, "upd9002-trace-v1\n");
	}
}

void upd9002_trace_stop(void) {
	if (trace_state.stream != NULL) {
		fflush(trace_state.stream);
	}
	ZeroMemory(&trace_state, sizeof(trace_state));
}

void upd9002_guest_trace_start(FILE *stream) {
	ZeroMemory(&guest_trace_state, sizeof(guest_trace_state));
	if (stream != NULL) {
		guest_trace_state.stream = stream;
		fprintf(stream, "scsi-guest-trace-v1\n");
	}
}

void upd9002_guest_trace_start_cmdreq_windows(FILE *stream) {
	ZeroMemory(&guest_trace_state, sizeof(guest_trace_state));
	if (stream != NULL) {
		guest_trace_state.stream = stream;
		guest_trace_state.cmdreq_windows_only = TRUE;
		fprintf(stream, "scsi-cmdreq-windows-v1\n");
	}
}

void upd9002_guest_trace_stop(void) {
	if (guest_trace_state.stream != NULL) {
		fflush(guest_trace_state.stream);
	}
	ZeroMemory(&guest_trace_state, sizeof(guest_trace_state));
}

void upd9002_guest_trace_step_begin(void) {
	UINT8 value;

	guest_trace_state.active = FALSE;
	if (guest_trace_state.cmdreq_windows_only) {
		if ((guest_trace_state.stream == NULL) || guest_trace_state.capture_done) {
			return;
		}
		if (CPU_IP == 0x1cbd) {
			guest_trace_state.last_wait_instruction = guest_trace_state.instruction;
			guest_trace_state.last_wait_clock = guest_trace_clock();
			guest_trace_state.wait_seen = TRUE;
			if (guest_trace_state.window_active && !guest_trace_state.wait_logged) {
				guest_trace_cmdreq_log("wait-enter", CPU_IP, CPU_IP,
				                       mem[(CS_BASE + 0x047e) & CPU_ADRSMASK],
				                       mem[(CS_BASE + 0x047e) & CPU_ADRSMASK]);
				guest_trace_state.wait_logged = TRUE;
			}
			return;
		}
		if ((CPU_IP != 0x1d67) &&
		    (!guest_trace_state.window_active || !guest_trace_watch_ip(CPU_IP))) {
			return;
		}
		value = mem[(CS_BASE + 0x047e) & CPU_ADRSMASK];
		guest_trace_state.active = TRUE;
		guest_trace_state.start_ip = CPU_IP;
		guest_trace_state.before_047e = value;
		guest_trace_cmdreq_log("entry", CPU_IP, CPU_IP, value, value);
		return;
	}
	if ((guest_trace_state.stream == NULL) || !guest_trace_watch_ip(CPU_IP)) {
		return;
	}
	value = mem[(CS_BASE + 0x047e) & CPU_ADRSMASK];
	guest_trace_state.active = TRUE;
	guest_trace_state.start_ip = CPU_IP;
	guest_trace_state.before_047e = value;
	guest_trace_log("entry", CPU_IP, CPU_IP, value, value);
}

void upd9002_guest_trace_step_end(void) {
	UINT8 after_047e;

	if (guest_trace_state.cmdreq_windows_only) {
		if ((guest_trace_state.stream == NULL) || guest_trace_state.capture_done) {
			return;
		}
		guest_trace_state.instruction++;
		if (!guest_trace_state.active) {
			return;
		}
		after_047e = mem[(CS_BASE + 0x047e) & CPU_ADRSMASK];
		if ((guest_trace_state.start_ip == 0x1d67) && (after_047e == 0xba) &&
		    guest_trace_state.window_active) {
			guest_trace_state.ba_seen = TRUE;
			guest_trace_state.ba_instruction = guest_trace_state.instruction;
			guest_trace_state.ba_clock = guest_trace_clock();
			guest_trace_cmdreq_log("047e-write-bah", guest_trace_state.start_ip, CPU_IP,
			                       guest_trace_state.before_047e, after_047e);
			fprintf(guest_trace_state.stream,
			        "scsi-cmdreq-window-context window=%u "
			        "presentation_to_wait=%u wait_step=%08x wait_clock=%08x\n",
			        guest_trace_state.current_window, guest_trace_state.wait_seen ? 1 : 0,
			        guest_trace_state.last_wait_instruction, guest_trace_state.last_wait_clock);
		}
		if ((guest_trace_state.start_ip == 0x1ccd) && guest_trace_state.ba_seen) {
			guest_trace_cmdreq_log("wait-consume", guest_trace_state.start_ip, CPU_IP,
			                       guest_trace_state.before_047e, after_047e);
			guest_trace_cmdreq_summary("1ccd", guest_trace_state.start_ip, CPU_IP);
		}
		if ((guest_trace_state.start_ip == 0x1747) && guest_trace_state.ba_seen) {
			guest_trace_state.consumer_seen = TRUE;
			guest_trace_state.consumer_ip = guest_trace_state.start_ip;
			guest_trace_cmdreq_log("main-pump-consume", guest_trace_state.start_ip, CPU_IP,
			                       guest_trace_state.before_047e, after_047e);
		}
		if ((guest_trace_state.start_ip == 0x1791) && guest_trace_state.consumer_seen) {
			guest_trace_cmdreq_log("main-pump-exit", guest_trace_state.start_ip, CPU_IP,
			                       guest_trace_state.before_047e, after_047e);
			guest_trace_cmdreq_summary("1747", guest_trace_state.consumer_ip, CPU_IP);
		}
		if (guest_trace_state.window_active) {
			guest_trace_cmdreq_log("exit", guest_trace_state.start_ip, CPU_IP,
			                       guest_trace_state.before_047e, after_047e);
		}
		guest_trace_state.active = FALSE;
		return;
	}
	if (!guest_trace_state.active) {
		return;
	}
	after_047e = mem[(CS_BASE + 0x047e) & CPU_ADRSMASK];
	guest_trace_log(guest_trace_047e_writer(guest_trace_state.start_ip) ? "047e-write" : "exit",
	                guest_trace_state.start_ip, CPU_IP, guest_trace_state.before_047e, after_047e);
	guest_trace_state.active = FALSE;
}

void upd9002_guest_trace_scsi_status(uint8_t status) {
	if (!guest_trace_state.cmdreq_windows_only || guest_trace_state.stream == NULL ||
	    guest_trace_state.capture_done) {
		return;
	}
	if (status == 0x8a) {
		if (guest_trace_state.window_active) {
			fprintf(guest_trace_state.stream,
			        "scsi-cmdreq-window event=window-overlap window=%u "
			        "ba_seen=%u step=%08x clock=%08x\n",
			        guest_trace_state.current_window, guest_trace_state.ba_seen ? 1 : 0,
			        guest_trace_state.instruction, guest_trace_clock());
		}
		guest_trace_state.window_count++;
		guest_trace_state.current_window = guest_trace_state.window_count;
		guest_trace_state.window_active = TRUE;
		guest_trace_state.presentation_instruction = guest_trace_state.instruction;
		guest_trace_state.presentation_clock = guest_trace_clock();
		guest_trace_state.presentation_wait_instruction = guest_trace_state.last_wait_instruction;
		guest_trace_state.presentation_wait_clock = guest_trace_state.last_wait_clock;
		guest_trace_state.presentation_wait_seen = guest_trace_state.wait_seen;
		guest_trace_state.wait_seen = FALSE;
		guest_trace_state.wait_logged = FALSE;
		guest_trace_state.ba_seen = FALSE;
		guest_trace_state.consumer_seen = FALSE;
		fprintf(guest_trace_state.stream,
		        "scsi-cmdreq-window event=csr-present raw=8a window=%u "
		        "step=%08x clock=%08x cs=%04x ip=%04x\n",
		        guest_trace_state.current_window, guest_trace_state.presentation_instruction,
		        guest_trace_state.presentation_clock, CPU_CS, CPU_IP);
	} else {
		fprintf(guest_trace_state.stream,
		        "scsi-cmdreq-window event=csr-present raw=%02x window=%u "
		        "step=%08x clock=%08x cs=%04x ip=%04x\n",
		        status, guest_trace_state.current_window, guest_trace_state.instruction,
		        guest_trace_clock(), CPU_CS, CPU_IP);
	}
}

int upd9002_trace_active(void) {
	return (trace_state.stream != NULL) && (trace_state.remaining != 0);
}

void upd9002_trace_event(uint32_t origin, const char *kind, uint32_t address, uint32_t value,
                         uint32_t width) {
	if (!upd9002_trace_active()) {
		return;
	}
	fprintf(trace_state.stream,
	        "event step=%08x seq=%08x origin=%s kind=%s address=%08x value=%08x width=%02x\n",
	        trace_state.step, trace_state.event++, origin_name(origin),
	        (kind != NULL) ? kind : "invalid", address, value, width);
}

void upd9002_trace_step_begin(void) {
	uint32_t address;
	uint32_t index;

	if (!upd9002_trace_active()) {
		return;
	}
	trace_state.before_clock = CPU_REMCLOCK;
	trace_state.before_cs = CPU_CS;
	trace_state.before_ip = CPU_IP;
	trace_state.event = 0;
	address = (CS_BASE + CPU_IP) & CPU_ADRSMASK;
	fprintf(trace_state.stream, "begin step=%08x cs=%04x ip=%04x bytes=", trace_state.step, CPU_CS,
	        CPU_IP);
	for (index = 0; index < 8; index++) {
		fprintf(trace_state.stream, "%02x", mem[(address + index) & CPU_ADRSMASK]);
	}
	fputc('\n', trace_state.stream);
	upd9002_trace_event(UPD9002_TRACE_ORIGIN_CPU, "fetch", address, mem[address], 1);
	upd9002_trace_event(UPD9002_TRACE_ORIGIN_DMA, "scheduler-checkpoint", 0, 0, 0);
	upd9002_trace_event(UPD9002_TRACE_ORIGIN_DEVICE, "device-checkpoint", 0, 0, 0);
}

void upd9002_trace_step_end(void) {
	int32_t consumed;

	if (!upd9002_trace_active()) {
		return;
	}
	consumed = trace_state.before_clock - CPU_REMCLOCK;
	fprintf(
	    trace_state.stream,
	    "end step=%08x ax=%04x bx=%04x cx=%04x dx=%04x si=%04x di=%04x bp=%04x sp=%04x es=%04x cs=%04x ss=%04x ds=%04x ip=%04x flags=%04x esbase=%08x csbase=%08x ssbase=%08x dsbase=%08x consumed=%08x remain=%08x\n",
	    trace_state.step, CPU_AX, CPU_BX, CPU_CX, CPU_DX, CPU_SI, CPU_DI, CPU_BP, CPU_SP, CPU_ES,
	    CPU_CS, CPU_SS, CPU_DS, CPU_IP, CPU_FLAG, ES_BASE, CS_BASE, SS_BASE, DS_BASE,
	    (uint32_t)consumed, (uint32_t)CPU_REMCLOCK);
	trace_state.step++;
	trace_state.remaining--;
	if (trace_state.remaining == 0) {
		fflush(trace_state.stream);
	}
}
