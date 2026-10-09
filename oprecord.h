/*
 * oprecord.h: User Operation Recorder / Player
 */

#if !defined(_OPRECORD_H_)
#define _OPRECORD_H_

#if defined(SUPPORT_OPRECORD)

/*
typedef struct {
	BOOL	repeat;

	int		playno;
} _OPRECORD, *OPRECORD;
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*OPRECORD_SETFDD)(UINT8 drv, const char *filename, int readonly);
//typedef void (*OPRECORD_RESTARTPLAY)(void);
typedef void (*OPRECORD_PLAYCOMPLETED)(void);

void oprecord_set_setfdd(OPRECORD_SETFDD setfdd);
//void oprecord_set_restartplay(OPRECORD_RESTARTPLAY restartplay);
void oprecord_set_playcompleted(OPRECORD_PLAYCOMPLETED playcompleted);

//int oprecord_start_record(const char *filename);
int oprecord_start_record2(FILEH fh);
void oprecord_stop_record(void);
BOOL oprecord_recording(void);

void oprecord_record_fdd(UINT8 drv, const char *fname, UINT8 readonly);
void oprecord_record_joypad(UINT8 id, UINT8 arrow, UINT8 button);
void oprecord_record_key(UINT8 code);

//int oprecord_start_play(const char *filename);
void oprecord_stop_play(void);
int oprecord_start_play2(FILEH _fh);
int oprecord_check_version(FILEH fh, char *ver, UINT size);
BOOL oprecord_playing(void);
void oprecord_play_update(void);

int oprecord_play_joypad(UINT8 id, UINT8 *arrow, UINT8 *button);

//extern	_OPRECORD	oprecord;

#ifdef __cplusplus
}
#endif

#endif

#endif /* _OPRECORD_H_ */
