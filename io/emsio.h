#ifndef VAEG_IO_EMSIO_H
#define VAEG_IO_EMSIO_H

enum {
	EMSIO_DEFAULT_MEGABYTES = 13,
	EMSIO_MIN_MEGABYTES = 1,
	EMSIO_MAX_MEGABYTES = 13
};

typedef struct {
	UINT8 maxmem;
	UINT8 target;
	UINT16 padding;
	UINT32 addr[4];
} _EMSIO, *EMSIO;

#ifdef __cplusplus
extern "C" {
#endif

extern _EMSIO emsio;

void emsio_reset(void);
void emsio_bind(void);

#ifdef __cplusplus
}
#endif

#endif
