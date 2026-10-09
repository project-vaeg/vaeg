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
/*
 * Cassette tape for V1/V2 mode (vaeg extension, M103j).
 *
 * [VA-TM] The VA's uPD8251 serves RS-232C only and port 30h defines only
 * bits 0 and 1, so a real VA has no cassette. [ROM] Its N-88 BASIC still
 * carries the PC-8801 cassette code: port 30h bit 3 switches the motor and
 * bits 5-4 select the 8251 channel (00 = cassette 600 baud, 01 = 1200
 * baud, 1x = RS-232C); BASIC writes by polling TXRDY and reads through the
 * receive interrupt (8214 level 0). See docs/agents/tasks/M103j_*.md.
 *
 * Tape images are a raw byte stream, or T88 whose data blocks are played
 * back in order (blank, space and mark blocks are not timed). Saving
 * records the transmitted bytes as a raw image.
 */

#include "compiler.h"
#include "dosio.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "memoryva.h"
#include "sound.h"
#include "cmt.h"

#include <string.h>

enum {
	CMT_MAXSIZE = 0x400000,
	/* Received bytes keep the request asserted, like the RXRDY level. */
	CMT_REQUEST_HZ = 600,
	/* Leader before the data: 1200 Hz, then 2400 Hz, one second each. */
	CMT_LEADER_SECONDS = 2
};

static const char t88_signature[] = "PC-8801 Tape Image(T88)";

static struct {
	BYTE *tape;
	UINT32 size;
	UINT32 pos;
	BYTE *out;
	UINT32 outsize;
	UINT32 outcap;
	char outpath[MAX_PATH];
	BOOL saving;
	UINT8 control; /* 88-mode port 30h */
	UINT8 data;
	BOOL rxready;
	BOOL rxe;
	BOOL leader; /* leader sounding; no data until it ends */
} cmt;

/*
 * Tape sound (the loading "pi-gaa"): each byte as one start bit, eight data
 * bits LSB first and one stop bit, a 0 as 1200 Hz and a 1 as 2400 Hz
 * (PC-8801 cassette FSK), and 2400 Hz between bytes while the motor runs.
 * Played at the real baud rate: with fast load, bytes that arrive while
 * one is still sounding are skipped, and gaps while data flows are filled
 * with data rather than carrier, so the sound keeps its pitch, rhythm and
 * 1200/2400 Hz balance. Display/sound only; it does not affect the data path.
 */
static struct {
	UINT16 frame; /* bits still to play, LSB first */
	UINT8 bits;   /* number of bits left in frame */
	BOOL pending; /* a byte waits to be played */
	UINT8 recent; /* frames to keep sounding data after the last byte */
	UINT8 next;
	UINT32 bitpos; /* 16.16 position within the current bit */
	UINT32 phase;  /* 16.16 tone phase */
	SINT32 level;  /* amplitude */
	UINT32 leadsamples; /* samples of the leader played */
} cmtsnd;

void cmt_setvol(UINT vol) {
	if (vol > 128) {
		vol = 128;
	}
	cmtsnd.level = (SINT32)vol * 32;
}

static void cmtsnd_push(REG8 dat) {
	cmtsnd.next = (UINT8)dat;
	cmtsnd.pending = TRUE;
	cmtsnd.recent = 8;
}

static BOOL cmtsnd_carrier(void) {
	return (cmt_selected() && (cmt.control & 0x08) &&
	        ((cmt.pos < cmt.size) || cmt.rxready || cmt.saving))
	           ? TRUE
	           : FALSE;
}

void cmt_getpcm(void *hdl, SINT32 *pcm, UINT count) {
	const UINT baud = (cmt.control & 0x10) ? 1200 : 600;
	UINT32 bitstep;
	UINT32 rate;

	(void)hdl;
	rate = soundcfg.rate;
	if ((rate == 0) || (cmtsnd.level == 0)) {
		cmtsnd.pending = FALSE;
		return;
	}
	if (!cmt_selected() || !(cmt.control & 0x08)) {
		/* Motor off: the tape stops, and so does its sound. */
		cmtsnd.bits = 0;
		cmtsnd.pending = FALSE;
		return;
	}
	bitstep = (UINT32)(((UINT64)baud << 16) / rate);
	while (count--) {
		int bit;
		UINT32 freq;
		SINT32 samp;

		if (cmt.leader) {
			/* Leader: low for the first second, then high. */
			freq = (cmtsnd.leadsamples < rate) ? 1200 : 2400;
			cmtsnd.leadsamples++;
			cmtsnd.bits = 0;
			cmtsnd.phase += (UINT32)(((UINT64)freq << 16) / rate);
			samp = (cmtsnd.phase & 0x8000) ? cmtsnd.level : -cmtsnd.level;
			pcm[0] += samp;
			pcm[1] += samp;
			pcm += 2;
			continue;
		}
		if (cmtsnd.bits == 0) {
			if (cmtsnd.pending) {
				cmtsnd.frame = (UINT16)(0x200 | ((UINT16)cmtsnd.next << 1));
				cmtsnd.bits = 10;
				cmtsnd.pending = FALSE;
			} else if (np2cfg.cmt_fast && cmtsnd.recent) {
				/* Fast load: between sampled bytes keep sounding data (the
				 * tape at its current position) rather than the carrier,
				 * whose 2400 Hz would otherwise dominate. */
				const UINT8 dat = (cmt.pos < cmt.size) ? cmt.tape[cmt.pos] : cmtsnd.next;

				cmtsnd.recent--;
				cmtsnd.frame = (UINT16)(0x200 | ((UINT16)dat << 1));
				cmtsnd.bits = 10;
			} else if (cmtsnd_carrier()) {
				cmtsnd.frame = 1;
				cmtsnd.bits = 1;
			} else {
				pcm += 2;
				continue;
			}
			cmtsnd.bitpos = 0;
		}
		bit = cmtsnd.frame & 1;
		freq = bit ? 2400 : 1200;
		cmtsnd.phase += (UINT32)(((UINT64)freq << 16) / rate);
		samp = (cmtsnd.phase & 0x8000) ? cmtsnd.level : -cmtsnd.level;
		pcm[0] += samp;
		pcm[1] += samp;
		pcm += 2;
		cmtsnd.bitpos += bitstep;
		if (cmtsnd.bitpos >= 0x10000) {
			cmtsnd.bitpos -= 0x10000;
			cmtsnd.frame >>= 1;
			cmtsnd.bits--;
		}
	}
}

void cmt_initialize(void) {
	ZeroMemory(&cmt, sizeof(cmt));
	ZeroMemory(&cmtsnd, sizeof(cmtsnd));
	cmt_setvol(np2cfg.cmt_vol);
}

void cmt_deinitialize(void) {
	cmt_save_end();
	cmt_eject();
	if (cmt.out != NULL) {
		_MFREE(cmt.out);
	}
	ZeroMemory(&cmt, sizeof(cmt));
}

void cmt_reset(void) {
	cmtsnd.bits = 0;
	cmtsnd.pending = FALSE;
	cmt.control = 0;
	cmt.rxready = FALSE;
	cmt.rxe = FALSE;
	cmt.leader = FALSE;
	nevent_reset(NEVENT_CMT);
}

BOOL cmt_selected(void) {
	return (memoryva_88_mode && !(cmt.control & 0x20)) ? TRUE : FALSE;
}

static BOOL cmt_running(void) {
	return (cmt_selected() && (cmt.control & 0x08) && cmt.rxe) ? TRUE : FALSE;
}

/* CPU clocks per tape byte: start, 8 data and stop bits; faster when the
 * cassette fast-load option is on. */
static SINT32 cmt_byteclock(void) {
	UINT baud = (cmt.control & 0x10) ? 1200 : 600;
	SINT32 clock = (SINT32)(pccore.realclock / (baud / 10));

	if (np2cfg.cmt_fast) {
		clock /= 16;
	}
	return (clock > 0) ? clock : 1;
}

/*
 * Tape leader (vaeg extension, maintainer's PC-8801 memory, [POLICY]): when
 * the motor starts for reading, a low then a high tone sound for one second
 * each before the data, and no byte is delivered until they end, so BASIC
 * prints "Found" after them. Not with fast load, and not while recording.
 */
static void cmt_leader_start(void) {
	cmt.leader = (cmt_selected() && !cmt.saving && !np2cfg.cmt_fast && (cmt.tape != NULL) &&
	              (cmt.pos < cmt.size))
	                 ? TRUE
	                 : FALSE;
	cmtsnd.leadsamples = 0;
}

static void cmt_schedule(void) {
	if (cmt.leader) {
		if (cmt_selected() && (cmt.control & 0x08)) {
			if (!nevent_iswork(NEVENT_CMT)) {
				nevent_set(NEVENT_CMT, (SINT32)(pccore.realclock * CMT_LEADER_SECONDS), cmt_event,
				           NEVENT_RELATIVE);
			}
			return;
		}
		cmt.leader = FALSE;
	}
	if (cmt_running() && (cmt.rxready || (cmt.pos < cmt.size))) {
		if (!nevent_iswork(NEVENT_CMT)) {
			nevent_set(NEVENT_CMT,
			           cmt.rxready ? (SINT32)(pccore.realclock / CMT_REQUEST_HZ) : cmt_byteclock(),
			           cmt_event, NEVENT_RELATIVE);
		}
	} else if (nevent_iswork(NEVENT_CMT)) {
		nevent_reset(NEVENT_CMT);
	}
}

void cmt_event(NEVENTITEM item) {
	(void)item;
	if (cmt.leader) {
		cmt.leader = FALSE;
		cmt_schedule();
		return;
	}
	if (!cmt_running()) {
		return;
	}
	if (!cmt.rxready && (cmt.pos < cmt.size)) {
		cmt.data = cmt.tape[cmt.pos++];
		cmt.rxready = TRUE;
		cmtsnd_push(cmt.data);
	}
	if (cmt.rxready) {
		pic8214_request(PIC8214_RXRDY);
	}
	cmt_schedule();
}

/* Carrier on the tape: [ROM] BASIC waits for port 40h bit 2 before it
 * starts reading (7EDFh). */
BOOL cmt_carrier(void) {
	return (cmt_selected() && (cmt.control & 0x08) && (cmt.pos < cmt.size)) ? TRUE : FALSE;
}

void cmt_port30(REG8 dat) {
	const BOOL motor = (cmt.control & 0x08) ? TRUE : FALSE;

	cmt.control = (UINT8)(dat & 0x3c);
	if (!motor && (cmt.control & 0x08)) {
		cmt_leader_start();
	}
	cmt_schedule();
}

void cmt_command(REG8 cmd) {
	cmt.rxe = (cmd & 0x04) ? TRUE : FALSE;
	cmt_schedule();
}

/* 8251 status: DSR, TXE and TXRDY always set; RXRDY while a byte waits. */
REG8 cmt_status(REG8 rxe) {
	(void)rxe;
	return (REG8)(0x85 | (cmt.rxready ? 0x02 : 0));
}

REG8 cmt_read(void) {
	cmt.rxready = FALSE;
	if (!cmt.leader && nevent_iswork(NEVENT_CMT)) {
		nevent_reset(NEVENT_CMT);
	}
	cmt_schedule();
	return cmt.data;
}

void cmt_write(REG8 dat) {
	BYTE *grown;

	if (!cmt.saving || !(cmt.control & 0x08)) {
		return;
	}
	if (cmt.outsize >= cmt.outcap) {
		if (cmt.outcap >= CMT_MAXSIZE) {
			return;
		}
		grown = (BYTE *)_MALLOC(cmt.outcap ? cmt.outcap * 2 : 0x4000, "cmt-save");
		if (grown == NULL) {
			return;
		}
		if (cmt.out != NULL) {
			CopyMemory(grown, cmt.out, cmt.outsize);
			_MFREE(cmt.out);
		}
		cmt.out = grown;
		cmt.outcap = cmt.outcap ? cmt.outcap * 2 : 0x4000;
	}
	cmt.out[cmt.outsize++] = (BYTE)dat;
	cmtsnd_push(dat);
}

/* T88: 24-byte signature, then tags (type, length, body); data tags carry
 * begin and length ticks, data size and type, then the bytes. */
static UINT32 cmt_t88_data(const BYTE *src, UINT size, BYTE *dst) {
	UINT32 p = sizeof(t88_signature);
	UINT32 n = 0;

	while (p + 4 <= size) {
		const UINT type = LOADINTELWORD(src + p);
		const UINT len = LOADINTELWORD(src + p + 2);

		p += 4;
		if ((type == 0x0000) || (p + len > size)) {
			break;
		}
		if ((type == 0x0101) && (len >= 12)) {
			UINT bytes = LOADINTELWORD(src + p + 8);

			if (bytes > len - 12) {
				bytes = len - 12;
			}
			if (dst != NULL) {
				CopyMemory(dst + n, src + p + 12, bytes);
			}
			n += bytes;
		}
		p += len;
	}
	return n;
}

BOOL cmt_open_memory(const BYTE *data, UINT size) {
	BOOL t88;
	UINT32 bytes;
	BYTE *tape;

	t88 = (size >= sizeof(t88_signature)) &&
	      (memcmp(data, t88_signature, sizeof(t88_signature)) == 0);
	bytes = t88 ? cmt_t88_data(data, size, NULL) : size;
	if ((bytes == 0) || (bytes > CMT_MAXSIZE)) {
		return FAILURE;
	}
	tape = (BYTE *)_MALLOC(bytes, "cmt-tape");
	if (tape == NULL) {
		return FAILURE;
	}
	if (t88) {
		cmt_t88_data(data, size, tape);
	} else {
		CopyMemory(tape, data, bytes);
	}
	cmt_eject();
	cmt.tape = tape;
	cmt.size = bytes;
	cmt.pos = 0;
	cmt_schedule();
	return SUCCESS;
}

BOOL cmt_open(const char *path) {
	FILEH fh;
	UINT size;
	BYTE *buf;
	BOOL ret;

	fh = file_open_rb(path);
	if (fh == FILEH_INVALID) {
		return FAILURE;
	}
	size = file_getsize(fh);
	if ((size == 0) || (size > CMT_MAXSIZE)) {
		file_close(fh);
		return FAILURE;
	}
	buf = (BYTE *)_MALLOC(size, "cmt-file");
	if (buf == NULL) {
		file_close(fh);
		return FAILURE;
	}
	ret = (file_read(fh, buf, size) == size) ? cmt_open_memory(buf, size) : FAILURE;
	_MFREE(buf);
	file_close(fh);
	return ret;
}

void cmt_eject(void) {
	if (cmt.tape != NULL) {
		_MFREE(cmt.tape);
	}
	cmt.tape = NULL;
	cmt.size = 0;
	cmt.pos = 0;
	cmt.rxready = FALSE;
	cmt.leader = FALSE;
	cmt_schedule();
}

void cmt_rewind(void) {
	cmt.pos = 0;
	cmt.rxready = FALSE;
	cmt_schedule();
}

BOOL cmt_inserted(void) {
	return (cmt.tape != NULL) ? TRUE : FALSE;
}

UINT32 cmt_position(void) {
	return cmt.pos;
}

UINT32 cmt_length(void) {
	return cmt.size;
}

BOOL cmt_save_begin(const char *path) {
	cmt_save_end();
	if ((path == NULL) || (path[0] == '\0')) {
		return FAILURE;
	}
	milstr_ncpy(cmt.outpath, path, sizeof(cmt.outpath));
	cmt.outsize = 0;
	cmt.saving = TRUE;
	return SUCCESS;
}

BOOL cmt_save_end(void) {
	FILEH fh;
	BOOL ret = SUCCESS;

	if (!cmt.saving) {
		return SUCCESS;
	}
	cmt.saving = FALSE;
	if (cmt.outpath[0] != '\0') {
		fh = file_create(cmt.outpath);
		if (fh == FILEH_INVALID) {
			ret = FAILURE;
		} else {
			if (file_write(fh, cmt.out, cmt.outsize) != cmt.outsize) {
				ret = FAILURE;
			}
			file_close(fh);
		}
	}
	return ret;
}

void cmt_save_discard(void) {
	cmt.saving = FALSE;
	cmt.outpath[0] = '\0';
	cmt.outsize = 0;
}

BOOL cmt_saving(void) {
	return cmt.saving;
}

UINT32 cmt_saved_bytes(void) {
	return cmt.outsize;
}

const BYTE *cmt_saved_data(void) {
	return cmt.out;
}
