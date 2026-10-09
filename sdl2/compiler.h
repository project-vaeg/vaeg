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
#ifndef VAEG_SDL2_COMPILER_H
#define VAEG_SDL2_COMPILER_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <assert.h>
#include <limits.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#if defined(_WIN32) && !defined(WIN32)
#define WIN32
#endif

#if !defined(WIN32)
#define X11
#endif
#define OSLANG_UTF8
#define OSLINEBREAK_LF
#define SDL_MAIN_HANDLED

typedef int INT;
typedef unsigned int UINT;
typedef unsigned long ULONG;

typedef int8_t SINT8;
typedef uint8_t UINT8;
typedef int16_t SINT16;
typedef uint16_t UINT16;
typedef int32_t SINT32;
typedef uint32_t UINT32;
typedef int64_t SINT64;
typedef uint64_t UINT64;
typedef intptr_t VAEG_INTPTR;
typedef uintptr_t VAEG_UINTPTR;

typedef int BOOL;
typedef char CHAR;
typedef char TCHAR;
typedef uint8_t BYTE;
typedef uint16_t WORD;
#if defined(WIN32)
typedef unsigned long DWORD;
#else
typedef uint32_t DWORD;
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef MAX_PATH
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#define MAX_PATH PATH_MAX
#endif

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef ZeroMemory
#define ZeroMemory(d, n) memset((d), 0, (n))
#endif

#ifndef CopyMemory
#define CopyMemory(d, s, n) memcpy((d), (s), (n))
#endif

#ifndef FillMemory
#define FillMemory(a, b, c) memset((a), (c), (b))
#endif

#ifndef roundup
#define roundup(x, y) ((((x) + ((y) - 1)) / (y)) * (y))
#endif

#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define BYTESEX_BIG
#else
#define BYTESEX_LITTLE
#endif

#define UNUSED(v) ((void)(v))

#ifndef NELEMENTS
#define NELEMENTS(a) ((int)(sizeof(a) / sizeof(a[0])))
#endif

#ifndef INLINE
#define INLINE inline
#endif

#ifndef DMACCALL
#define DMACCALL
#endif

#ifndef MEMCALL
#define MEMCALL
#endif

#ifndef QWORD_CONST
#define QWORD_CONST(v) UINT64_C(v)
#endif

#ifndef SQWORD_CONST
#define SQWORD_CONST(v) INT64_C(v)
#endif

#define SIZE_VGA

#if !defined(SIZE_VGA)
#define RGB16 UINT32
#define SIZE_QVGA
#endif

#include "common.h"
#include "milstr.h"
#include "codecnv.h"
#include "_memory.h"
#include "rect.h"
#include "lstarray.h"
#include "trace.h"

static INLINE UINT32 vaeg_gettick(void) {
	struct timeval tv;
	UINT64 msec;

	if (gettimeofday(&tv, NULL) != 0) {
		return 0;
	}
	msec = ((UINT64)tv.tv_sec * 1000) + ((UINT64)tv.tv_usec / 1000);
	return (UINT32)msec;
}

#define GETTICK() vaeg_gettick()
#define SPRINTF sprintf
#define __ASSERT(s) assert(s)

#define msgbox(title, msg) fprintf(stderr, "%s: %s\n", (title), (msg))

#define VERMOUTH_LIB

#define SUPPORT_SJIS

#define MEMOPTIMIZE 2

#define SOUNDRESERVE 100

#endif
