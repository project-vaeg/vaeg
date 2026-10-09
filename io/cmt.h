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
#ifndef VAEG_IO_CMT_H
#define VAEG_IO_CMT_H

/*
 * Cassette tape (vaeg extension, M103j): the PC-8801 cassette channel of
 * the uPD8251, which the VA does not have. Active only in 88 mode, when
 * port 30h bits 5-4 select the cassette.
 */

#ifdef __cplusplus
extern "C" {
#endif

void cmt_initialize(void);
void cmt_deinitialize(void);
void cmt_reset(void);

/* Tape images. */
BOOL cmt_open(const char *path); /* raw byte image or T88 */
BOOL cmt_open_memory(const BYTE *data, UINT size);
void cmt_eject(void);
void cmt_rewind(void);
BOOL cmt_inserted(void);
UINT32 cmt_position(void);
UINT32 cmt_length(void);
BOOL cmt_save_begin(const char *path); /* record to a raw image file */
BOOL cmt_save_end(void);               /* write and close it */
void cmt_save_discard(void);           /* stop without writing */
BOOL cmt_saving(void);
UINT32 cmt_saved_bytes(void);
const BYTE *cmt_saved_data(void);

/* Machine side. */
void cmt_port30(REG8 dat);
BOOL cmt_selected(void);
BOOL cmt_carrier(void);
REG8 cmt_status(REG8 rxe);
REG8 cmt_read(void);
void cmt_write(REG8 dat);
void cmt_command(REG8 cmd);
void cmt_setvol(UINT vol); /* tape sound, 0-128 */
void cmt_getpcm(void *hdl, SINT32 *pcm, UINT count);
void cmt_event(struct _neventitem *item); /* NEVENTCB */

#ifdef __cplusplus
}
#endif

#endif
