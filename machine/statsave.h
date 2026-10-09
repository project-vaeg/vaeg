#include "dosio.h"

enum {
	STATFLAG_SUCCESS = 0,
	STATFLAG_DISKCHG = 0x0001,
	STATFLAG_VERCHG = 0x0002,
	STATFLAG_WARNING = 0x0080,
	STATFLAG_VERSION = 0x0100,
	STATFLAG_FAILURE = -1
};

typedef struct {
	char index[10];
	UINT16 ver;
	UINT32 size;
} STFLAGHDR;

typedef struct {
	STFLAGHDR hdr;
	UINT pos;
	char *err;
	int errlen;
} _STFLAGH, *STFLAGH;

typedef struct {
	char index[10];
	UINT16 ver;
	UINT16 type;
	void *arg1;
	UINT arg2;
} SFENTRY;

#ifdef __cplusplus
extern "C" {
#endif

int statflag_read(STFLAGH sfh, void *ptr, UINT size);
int statflag_write(STFLAGH sfh, const void *ptr, UINT size);
void statflag_seterr(STFLAGH sfh, const char *str);

int statsave_save(const char *filename);
int statsave_check(const char *filename, char *buf, int size);
/* Explicit UI-confirmed override; all non-HOSTFAT checks remain active. */
int statsave_check_hostfat_override(const char *filename, char *buf, int size);
int statsave_load(const char *filename);
/* Retains the current HOSTFAT mount state and read-only snapshot. */
int statsave_load_hostfat_override(const char *filename);

#if defined(SUPPORT_OPRECORD)
typedef void (*STATSAVE_LOAD_DISK_HOOK)(int drv, char *path, UINT size, int *readonly);

void statsave_set_load_disk_hook(STATSAVE_LOAD_DISK_HOOK hook);

#endif

#ifdef __cplusplus
}
#endif
