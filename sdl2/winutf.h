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
#ifndef VAEG_SDL2_WINUTF_H
#define VAEG_SDL2_WINUTF_H

#if defined(WIN32)
#include <stdlib.h>
#include <wchar.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

static inline wchar_t *winutf_from_utf8(const char *src) {
	int len;
	wchar_t *dst;

	if (src == NULL) {
		return NULL;
	}
	len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, src, -1, NULL, 0);
	if (len <= 0) {
		return NULL;
	}
	dst = (wchar_t *)malloc(sizeof(wchar_t) * (size_t)len);
	if (dst == NULL) {
		return NULL;
	}
	if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, src, -1, dst, len) <= 0) {
		free(dst);
		return NULL;
	}
	return dst;
}

static inline int winutf_to_utf8(char *dst, int size, const wchar_t *src) {
	int ret;

	if ((dst == NULL) || (size <= 0) || (src == NULL)) {
		return -1;
	}
	ret = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, src, -1, dst, size, NULL, NULL);
	if (ret <= 0) {
		dst[0] = '\0';
		return -1;
	}
	return 0;
}

static inline void winutf_free(wchar_t *ptr) {
	free(ptr);
}
#endif

#endif
