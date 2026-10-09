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
#ifndef VAEG_SDL2_DOSIO_H
#define VAEG_SDL2_DOSIO_H

#if defined(WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

typedef FILE *FILEH;
#define FILEH_INVALID NULL

#if defined(WIN32)
#define FLISTH HANDLE
#define FLISTH_INVALID (INVALID_HANDLE_VALUE)
#else
#define FLISTH long
#define FLISTH_INVALID 0
#endif

#define FSEEK_SET SEEK_SET
#define FSEEK_CUR SEEK_CUR
#define FSEEK_END SEEK_END

enum {
	FILEATTR_READONLY = 0x01,
	FILEATTR_HIDDEN = 0x02,
	FILEATTR_SYSTEM = 0x04,
	FILEATTR_VOLUME = 0x08,
	FILEATTR_DIRECTORY = 0x10,
	FILEATTR_ARCHIVE = 0x20
};

enum {
	FLICAPS_SIZE = 0x0001,
	FLICAPS_ATTR = 0x0002,
	FLICAPS_DATE = 0x0004,
	FLICAPS_TIME = 0x0008
};

typedef struct {
	UINT16 year;
	UINT8 month;
	UINT8 day;
} DOSDATE;

typedef struct {
	UINT8 hour;
	UINT8 minute;
	UINT8 second;
} DOSTIME;

typedef struct {
	UINT caps;
	UINT32 size;
	UINT32 attr;
	DOSDATE date;
	DOSTIME time;
	char path[MAX_PATH];
} FLINFO;

#ifdef __cplusplus
extern "C" {
#endif

void dosio_init(void);
void dosio_term(void);

FILEH file_open(const char *path);
FILEH file_open_rb(const char *path);
FILEH file_create(const char *path);
long file_seek(FILEH handle, long pointer, int method);
UINT file_read(FILEH handle, void *data, UINT length);
UINT file_write(FILEH handle, const void *data, UINT length);
short file_flush(FILEH handle);
short file_setsize(FILEH handle, UINT64 size);
short file_close(FILEH handle);
UINT file_getsize(FILEH handle);
UINT64 file_getsize64(FILEH handle);
short file_rename_atomic(const char *source, const char *destination, BOOL replace);
short file_getdatetime(FILEH handle, DOSDATE *dosdate, DOSTIME *dostime);
short file_delete(const char *path);
short file_attr(const char *path);
short file_dircreate(const char *path);

void file_setcd(const char *exepath);
char *file_getcd(const char *path);
FILEH file_open_c(const char *path);
FILEH file_open_rb_c(const char *path);
FILEH file_create_c(const char *path);
short file_delete_c(const char *path);
short file_attr_c(const char *path);
void file_getuserdir(char *path, int size);
void file_getstatepath(char *path, int size, const char *name);

FLISTH file_list1st(const char *dir, FLINFO *fli);
BOOL file_listnext(FLISTH hdl, FLINFO *fli);
void file_listclose(FLISTH hdl);

#define file_cpyname(p, n, m) milstr_ncpy(p, n, m)
#if defined(WIN32)
#define file_cmpname(p, n) milstr_cmp(p, n)
#else
#define file_cmpname(p, n) strcmp(p, n)
#endif
void file_catname(char *path, const char *name, int maxlen);
char *file_getname(const char *path);
void file_cutname(char *path);
char *file_getext(const char *path);
void file_cutext(char *path);
void file_cutseparator(char *path);
void file_setseparator(char *path, int maxlen);

#ifdef __cplusplus
}
#endif

#endif
