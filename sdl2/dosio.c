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
#include <sys/stat.h>
#include <time.h>
#include <limits.h>
#include "dosio.h"
#if defined(WIN32)
#include <direct.h>
#include <io.h>
#include "winutf.h"
#else
#include <dirent.h>
#include <unistd.h>
#endif

static char curpath[MAX_PATH];
static char *curfilep = curpath;

#if !defined(WIN32)
static int ascii_tolower(int c) {
	if ((c >= 'A') && (c <= 'Z')) {
		return (c + ('a' - 'A'));
	}
	return (c);
}

static BOOL ascii_case_equal(const char *str1, const char *str2) {
	while ((*str1 != '\0') && (*str2 != '\0')) {
		if (ascii_tolower((unsigned char)*str1) != ascii_tolower((unsigned char)*str2)) {
			return (FAILURE);
		}
		str1++;
		str2++;
	}
	return ((*str1 == '\0') && (*str2 == '\0'));
}

static const char *path_namepart(const char *path) {
	const char *ret;

	ret = path;
	while (*path != '\0') {
		if ((*path == '/') || (*path == '\\')) {
			ret = path + 1;
		}
		path++;
	}
	return (ret);
}

static BOOL path_is_rom_or_wav(const char *path) {
	const char *name;
	const char *ext;

	name = path_namepart(path);
	ext = NULL;
	while (*name != '\0') {
		if (*name == '.') {
			ext = name;
		}
		name++;
	}
	if (ext == NULL) {
		return (FAILURE);
	}
	return (ascii_case_equal(ext, ".rom") || ascii_case_equal(ext, ".wav"));
}
#endif

static BOOL resolve_casefold_asset_path(const char *path, char *resolved, int size) {
#if !defined(WIN32)
	const char *name;
	size_t dirlen;
	size_t namelen;
	char dir[MAX_PATH];
	DIR *dp;
	struct dirent *de;

	if (path_is_rom_or_wav(path) != SUCCESS) {
		return (FAILURE);
	}
	name = path_namepart(path);
	dirlen = (size_t)(name - path);
	if (dirlen >= sizeof(dir)) {
		return (FAILURE);
	}
	if (dirlen != 0) {
		memcpy(dir, path, dirlen);
		dir[dirlen] = '\0';
	} else {
		file_cpyname(dir, ".", sizeof(dir));
	}
	dp = opendir(dir);
	if (dp == NULL) {
		return (FAILURE);
	}
	while ((de = readdir(dp)) != NULL) {
		if (ascii_case_equal(de->d_name, name)) {
			namelen = strlen(de->d_name);
			if ((dirlen + namelen + 1) > (size_t)size) {
				closedir(dp);
				return (FAILURE);
			}
			if (dirlen != 0) {
				memcpy(resolved, path, dirlen);
				resolved[dirlen] = '\0';
				file_cpyname(resolved + dirlen, de->d_name, size - (int)dirlen);
			} else {
				file_cpyname(resolved, de->d_name, size);
			}
			closedir(dp);
			return (SUCCESS);
		}
	}
	closedir(dp);
#else
	(void)path;
	(void)resolved;
	(void)size;
#endif
	return (FAILURE);
}

static FILEH file_fopen_raw(const char *path, const char *mode) {
#if defined(WIN32)
	wchar_t *wpath;
	wchar_t *wmode;
	FILEH ret;

	wpath = winutf_from_utf8(path);
	wmode = winutf_from_utf8(mode);
	if ((wpath == NULL) || (wmode == NULL)) {
		winutf_free(wpath);
		winutf_free(wmode);
		return (FILEH_INVALID);
	}
	ret = _wfopen(wpath, wmode);
	winutf_free(wpath);
	winutf_free(wmode);
	return (ret);
#else
	return (fopen(path, mode));
#endif
}

static FILEH file_fopen_asset(const char *path, const char *mode) {
	FILEH ret;
	char resolved[MAX_PATH];

	ret = file_fopen_raw(path, mode);
	if (ret != FILEH_INVALID) {
		return (ret);
	}
	if (resolve_casefold_asset_path(path, resolved, sizeof(resolved)) == SUCCESS) {
		ret = file_fopen_raw(resolved, mode);
	}
	return (ret);
}

static int file_stat_raw(const char *path, struct stat *sb) {
#if defined(WIN32)
	wchar_t *wpath;
	struct _stat wsb;
	int ret;

	wpath = winutf_from_utf8(path);
	if (wpath == NULL) {
		return (-1);
	}
	ret = _wstat(wpath, &wsb);
	winutf_free(wpath);
	if (ret != 0) {
		return (ret);
	}
	ZeroMemory(sb, sizeof(*sb));
	sb->st_dev = wsb.st_dev;
	sb->st_ino = wsb.st_ino;
	sb->st_mode = wsb.st_mode;
	sb->st_nlink = wsb.st_nlink;
	sb->st_uid = wsb.st_uid;
	sb->st_gid = wsb.st_gid;
	sb->st_rdev = wsb.st_rdev;
	sb->st_size = wsb.st_size;
	sb->st_atime = wsb.st_atime;
	sb->st_mtime = wsb.st_mtime;
	sb->st_ctime = wsb.st_ctime;
	return (0);
#else
	return (stat(path, sb));
#endif
}

static int file_stat_asset(const char *path, struct stat *sb) {
	char resolved[MAX_PATH];

	if (file_stat_raw(path, sb) == 0) {
		return (0);
	}
	if (resolve_casefold_asset_path(path, resolved, sizeof(resolved)) == SUCCESS) {
		return (file_stat_raw(resolved, sb));
	}
	return (-1);
}

void dosio_init(void) {
}

void dosio_term(void) {
}

/* ファイル操作 */
FILEH file_open(const char *path) {
	return (file_fopen_asset(path, "rb+"));
}

FILEH file_open_rb(const char *path) {
	return (file_fopen_asset(path, "rb+"));
}

FILEH file_create(const char *path) {
	return (file_fopen_raw(path, "wb+"));
}

long file_seek(FILEH handle, long pointer, int method) {
	fseek(handle, pointer, method);
	return (ftell(handle));
}

UINT file_read(FILEH handle, void *data, UINT length) {
	return ((UINT)fread(data, 1, length, handle));
}

UINT file_write(FILEH handle, const void *data, UINT length) {
	return ((UINT)fwrite(data, 1, length, handle));
}

short file_flush(FILEH handle) {
	if ((handle == FILEH_INVALID) || (fflush(handle) != 0)) {
		return (-1);
	}
#if defined(WIN32)
	if (FlushFileBuffers((HANDLE)_get_osfhandle(_fileno(handle))) == 0) {
		return (-1);
	}
#else
	if (fsync(fileno(handle)) != 0) {
		return (-1);
	}
#endif
	return (0);
}

short file_setsize(FILEH handle, UINT64 size) {
	if ((handle == FILEH_INVALID) || (size > (UINT64)LLONG_MAX)) {
		return (-1);
	}
	if (fflush(handle) != 0) {
		return (-1);
	}
#if defined(WIN32)
	if (_chsize_s(_fileno(handle), (__int64)size) != 0) {
		return (-1);
	}
#else
	if (ftruncate(fileno(handle), (off_t)size) != 0) {
		return (-1);
	}
#endif
	return (0);
}

short file_close(FILEH handle) {
	return (short)fclose(handle);
}

UINT file_getsize(FILEH handle) {
	UINT64 size = file_getsize64(handle);
	return ((size > UINT_MAX) ? 0 : (UINT)size);
}

UINT64 file_getsize64(FILEH handle) {
	struct stat sb;

	if (fstat(fileno(handle), &sb) == 0) {
		return ((UINT64)sb.st_size);
	}
	return (0);
}

short file_rename_atomic(const char *source, const char *destination, BOOL replace) {
#if defined(WIN32)
	wchar_t *wsource;
	wchar_t *wdestination;
	DWORD flags = MOVEFILE_WRITE_THROUGH;
	BOOL result;
	wsource = winutf_from_utf8(source);
	wdestination = winutf_from_utf8(destination);
	if ((wsource == NULL) || (wdestination == NULL)) {
		winutf_free(wsource);
		winutf_free(wdestination);
		return (-1);
	}
	if (replace) {
		flags |= MOVEFILE_REPLACE_EXISTING;
	}
	result = MoveFileExW(wsource, wdestination, flags);
	winutf_free(wsource);
	winutf_free(wdestination);
	return (result ? 0 : -1);
#else
	(void)replace;
	return (rename(source, destination));
#endif
}

short file_attr(const char *path) {
	struct stat sb;
	short attr;

	if (file_stat_asset(path, &sb) == 0) {
#if defined(WIN32)
		if (sb.st_mode & _S_IFDIR) {
			attr = FILEATTR_DIRECTORY;
		} else {
			attr = 0;
		}
		if (!(sb.st_mode & S_IWRITE)) {
			attr |= FILEATTR_READONLY;
		}
#else
		if (S_ISDIR(sb.st_mode)) {
			return (FILEATTR_DIRECTORY);
		}
		attr = 0;
		if (!(sb.st_mode & S_IWUSR)) {
			attr |= FILEATTR_READONLY;
		}
#endif
		return (attr);
	}
	return (-1);
}

static BOOL cnv_sttime(time_t *t, DOSDATE *dosdate, DOSTIME *dostime) {
	struct tm *ftime;

	ftime = localtime(t);
	if (ftime == NULL) {
		return (FAILURE);
	}
	if (dosdate) {
		dosdate->year = ftime->tm_year + 1900;
		dosdate->month = ftime->tm_mon + 1;
		dosdate->day = ftime->tm_mday;
	}
	if (dostime) {
		dostime->hour = ftime->tm_hour;
		dostime->minute = ftime->tm_min;
		dostime->second = ftime->tm_sec;
	}
	return (SUCCESS);
}

short file_getdatetime(FILEH handle, DOSDATE *dosdate, DOSTIME *dostime) {
	struct stat sb;

	if (fstat(fileno(handle), &sb) == 0) {
		if (cnv_sttime(&sb.st_mtime, dosdate, dostime) == SUCCESS) {
			return (0);
		}
	}
	return (-1);
}

short file_delete(const char *path) {
#if defined(WIN32)
	wchar_t *wpath;
	BOOL ret;

	wpath = winutf_from_utf8(path);
	if (wpath == NULL) {
		return (-1);
	}
	ret = DeleteFileW(wpath);
	winutf_free(wpath);
	return ((ret != 0) ? 0 : -1);
#else
	return (unlink(path));
#endif
}

short file_dircreate(const char *path) {
#if defined(WIN32)
	wchar_t *wpath;
	int ret;

	wpath = winutf_from_utf8(path);
	if (wpath == NULL) {
		return (-1);
	}
	ret = _wmkdir(wpath);
	winutf_free(wpath);
	return ((short)ret);
#else
	return ((short)mkdir(path, 0777));
#endif
}

static void file_append_userdir(char *path, const char *name, int size) {
	file_setseparator(path, size);
	file_catname(path, name, size);
	file_setseparator(path, size);
	(void)file_dircreate(path);
}

void file_getuserdir(char *path, int size) {
#if defined(WIN32)
	const char *base;

	base = getenv("APPDATA");
	if ((base == NULL) || (base[0] == '\0')) {
		file_cpyname(path, "./", size);
		return;
	}
	file_cpyname(path, base, size);
	file_append_userdir(path, "vaeg", size);
#elif defined(__APPLE__)
	const char *home;

	home = getenv("HOME");
	if ((home == NULL) || (home[0] == '\0')) {
		file_cpyname(path, "./", size);
		return;
	}
	file_cpyname(path, home, size);
	file_append_userdir(path, "Library", size);
	file_append_userdir(path, "Application Support", size);
	file_append_userdir(path, "vaeg", size);
#else
	const char *base;
	const char *home;

	base = getenv("XDG_CONFIG_HOME");
	if ((base == NULL) || (base[0] == '\0')) {
		home = getenv("HOME");
		if ((home == NULL) || (home[0] == '\0')) {
			file_cpyname(path, "./", size);
			return;
		}
		file_cpyname(path, home, size);
		file_append_userdir(path, ".config", size);
	} else {
		file_cpyname(path, base, size);
		file_setseparator(path, size);
		(void)file_dircreate(path);
	}
	file_append_userdir(path, "vaeg", size);
#endif
}

void file_getstatepath(char *path, int size, const char *name) {
	file_getuserdir(path, size);
	file_catname(path, name, size);
}

/* カレントファイル操作 */
void file_setcd(const char *exepath) {
	file_cpyname(curpath, exepath, sizeof(curpath));
	curfilep = file_getname(curpath);
	*curfilep = '\0';
}

char *file_getcd(const char *path) {
	file_cpyname(curfilep, path, sizeof(curpath) - (curfilep - curpath));
	return (curpath);
}

FILEH file_open_c(const char *path) {
	file_cpyname(curfilep, path, sizeof(curpath) - (curfilep - curpath));
	return (file_open(curpath));
}

FILEH file_open_rb_c(const char *path) {
	file_cpyname(curfilep, path, sizeof(curpath) - (curfilep - curpath));
	return (file_open_rb(curpath));
}

FILEH file_create_c(const char *path) {
	file_cpyname(curfilep, path, sizeof(curpath) - (curfilep - curpath));
	return (file_create(curpath));
}

short file_delete_c(const char *path) {
	file_cpyname(curfilep, path, sizeof(curpath) - (curfilep - curpath));
	return (file_delete(curpath));
}

short file_attr_c(const char *path) {
	file_cpyname(curfilep, path, sizeof(curpath) - (curfilep - curpath));
	return (file_attr(curpath));
}

#if defined(WIN32)
static BOOL cnvdatetime(FILETIME *file, DOSDATE *dosdate, DOSTIME *dostime) {
	FILETIME localtime;
	SYSTEMTIME systime;

	if ((FileTimeToLocalFileTime(file, &localtime) == 0) ||
	    (FileTimeToSystemTime(&localtime, &systime) == 0)) {
		return (FAILURE);
	}
	if (dosdate) {
		dosdate->year = (UINT16)systime.wYear;
		dosdate->month = (UINT8)systime.wMonth;
		dosdate->day = (UINT8)systime.wDay;
	}
	if (dostime) {
		dostime->hour = (UINT8)systime.wHour;
		dostime->minute = (UINT8)systime.wMinute;
		dostime->second = (UINT8)systime.wSecond;
	}
	return (SUCCESS);
}

static BOOL setflist(WIN32_FIND_DATAW *w32fd, FLINFO *fli) {
	if ((w32fd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
	    ((!wcscmp(w32fd->cFileName, L".")) || (!wcscmp(w32fd->cFileName, L"..")))) {
		return (FAILURE);
	}
	fli->caps = FLICAPS_SIZE | FLICAPS_ATTR;
	fli->size = w32fd->nFileSizeLow;
	fli->attr = w32fd->dwFileAttributes;
	if (cnvdatetime(&w32fd->ftLastWriteTime, &fli->date, &fli->time) == SUCCESS) {
		fli->caps |= FLICAPS_DATE | FLICAPS_TIME;
	}
	if (winutf_to_utf8(fli->path, sizeof(fli->path), w32fd->cFileName) != 0) {
		return (FAILURE);
	}
	return (SUCCESS);
}

FLISTH file_list1st(const char *dir, FLINFO *fli) {
	char path[MAX_PATH];
	wchar_t *wpath;
	HANDLE hdl;
	WIN32_FIND_DATAW w32fd;

	file_cpyname(path, dir, sizeof(path));
	file_setseparator(path, sizeof(path));
	file_catname(path, "*.*", sizeof(path));
	wpath = winutf_from_utf8(path);
	if (wpath == NULL) {
		return (FLISTH_INVALID);
	}
	hdl = FindFirstFileW(wpath, &w32fd);
	winutf_free(wpath);
	if (hdl != INVALID_HANDLE_VALUE) {
		do {
			if (setflist(&w32fd, fli) == SUCCESS) {
				return (hdl);
			}
		} while (FindNextFileW(hdl, &w32fd));
		FindClose(hdl);
	}
	return (FLISTH_INVALID);
}

BOOL file_listnext(FLISTH hdl, FLINFO *fli) {
	WIN32_FIND_DATAW w32fd;

	while (FindNextFileW(hdl, &w32fd)) {
		if (setflist(&w32fd, fli) == SUCCESS) {
			return (SUCCESS);
		}
	}
	return (FAILURE);
}

void file_listclose(FLISTH hdl) {
	FindClose(hdl);
}
#else
FLISTH file_list1st(const char *dir, FLINFO *fli) {
	DIR *ret;

	ret = opendir(dir);
	if (ret == NULL) {
		goto ff1_err;
	}
	if (file_listnext((FLISTH)ret, fli) == SUCCESS) {
		return ((FLISTH)ret);
	}
	closedir(ret);

ff1_err:
	return (FLISTH_INVALID);
}

BOOL file_listnext(FLISTH hdl, FLINFO *fli) {
	struct dirent *de;
	struct stat sb;
	UINT32 attr;

	de = readdir((DIR *)hdl);
	if (de == NULL) {
		return (FAILURE);
	}
	if (fli) {
		if (stat(de->d_name, &sb) == 0) {
			fli->caps = FLICAPS_SIZE | FLICAPS_ATTR;
			fli->size = sb.st_size;
			attr = 0;
			if (S_ISDIR(sb.st_mode)) {
				attr = FILEATTR_DIRECTORY;
			} else if (!(sb.st_mode & S_IWUSR)) {
				attr = FILEATTR_READONLY;
			}
			fli->attr = attr;
			if (cnv_sttime(&sb.st_mtime, &fli->date, &fli->time) == SUCCESS) {
				fli->caps |= FLICAPS_DATE | FLICAPS_TIME;
			}
		} else {
			fli->caps = 0;
			fli->size = 0;
			fli->attr = 0;
		}
		file_cpyname(fli->path, de->d_name, sizeof(fli->path));
	}
	return (SUCCESS);
}

void file_listclose(FLISTH hdl) {
	closedir((DIR *)hdl);
}
#endif

void file_catname(char *path, const char *name, int maxlen) {
	int csize;

	while (maxlen > 0) {
		if (*path == '\0') {
			break;
		}
		path++;
		maxlen--;
	}
	file_cpyname(path, name, maxlen);
	while ((csize = milstr_charsize(path)) != 0) {
		if ((csize == 1) && (*path == '\\')) {
			*path = '/';
		}
		path += csize;
	}
}

char *file_getname(const char *path) {
	const char *ret;
	int csize;

	ret = path;
	while ((csize = milstr_charsize(path)) != 0) {
		if ((csize == 1) && (*path == '/')) {
			ret = path + 1;
		}
		path += csize;
	}
	return ((char *)ret);
}

void file_cutname(char *path) {
	char *p;

	p = file_getname(path);
	*p = '\0';
}

char *file_getext(const char *path) {
	const char *p;
	const char *q;

	p = file_getname(path);
	q = NULL;
	while (*p != '\0') {
		if (*p == '.') {
			q = p + 1;
		}
		p++;
	}
	if (q == NULL) {
		q = p;
	}
	return ((char *)q);
}

void file_cutext(char *path) {
	char *p;
	char *q;

	p = file_getname(path);
	q = NULL;
	while (*p != '\0') {
		if (*p == '.') {
			q = p;
		}
		p++;
	}
	if (q != NULL) {
		*q = '\0';
	}
}

void file_cutseparator(char *path) {
	int pos;

	pos = strlen(path) - 1;
	if ((pos > 0) &&                        // 2文字以上でー
	    (path[pos] == '/') &&               // ケツが \ でー
	    ((pos != 1) || (path[0] != '.'))) { // './' ではなかったら
		path[pos] = '\0';
	}
}

void file_setseparator(char *path, int maxlen) {
	int pos;

	pos = strlen(path);
	if ((pos) && (path[pos - 1] != '/') && ((pos + 2) < maxlen)) {
		path[pos++] = '/';
		path[pos] = '\0';
	}
}
