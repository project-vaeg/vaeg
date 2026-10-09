/*
 * oprecord.c: User Operation Recorder / Player
 */

#include	"compiler.h"
#include	"dosio.h"
#include	"cpucore.h"
#include	"machine/nevent.h"
#include	"serial.h"
#include	"np2ver.h"

#include	"oprecord.h"

#if defined(SUPPORT_OPRECORD)


typedef struct {
	char	desc[16];
	char	vaegrel[16];
	BYTE	dmy[32];
} _OPRECORDHEADER;

typedef struct {
	UINT32	past;
	UINT16	type;
} _HEADER, *HEADER;

typedef struct {
	_HEADER	h;
	UINT8	drv;
	UINT8	readonly;
	char	filename[MAX_PATH];
} _E_FDD, *E_FDD;

typedef struct {
	_HEADER	h;
	UINT8	id;			// ジョイパッドの番号 0 or 1
	UINT8	arrow;
	UINT8	button;
	UINT8	dmy;
} _E_JOYPAD, *E_JOYPAD;

typedef struct {
	_HEADER	h;
	UINT8	code;
	UINT8	dmy;
} _E_KEY, *E_KEY;

enum {
	TYPE_END		= 0,
	TYPE_FDD,
	TYPE_JOYPAD,
	TYPE_KEY,
	TYPE_INVALID,
	STATE_UNKNOWN,

	BUFFER_SIZE		= 1024,
};

static const _OPRECORDHEADER oprecordheader = {
	"OPRECORD",
	VAEGREL_CORE
};

//		_OPRECORD oprecord = {0};

static	OPRECORD_SETFDD setfdd;
//static	OPRECORD_RESTARTPLAY restartplay;
static	OPRECORD_PLAYCOMPLETED playcompleted;

static	BYTE buffer[BUFFER_SIZE];
static	BYTE *bufferp;
static	UINT32 lasttime;
static	FILEH fh;
static	BOOL recording = FALSE;
static	BOOL playing = FALSE;

// デバイスの現在の状態
static _E_JOYPAD s_joypad;

// ---- 共通

static UINT32 now(void) {
	return CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
}

// ---- 共通 API

void oprecord_set_setfdd(OPRECORD_SETFDD _setfdd) {
	setfdd = _setfdd;
}

/*
void oprecord_set_restartplay(OPRECORD_RESTARTPLAY _restartplay) {
	restartplay = _restartplay;
}
*/

void oprecord_set_playcompleted(OPRECORD_PLAYCOMPLETED _playcompleted) {
	playcompleted = _playcompleted;
}

// ---- 記録

static void save(void) {
	if (bufferp == buffer) return;

	//bufferからbufferpまでを保存する。
	file_write(fh, buffer, bufferp - buffer);

	bufferp = buffer;
}

static HEADER new_event(UINT16 type, int size) {
	HEADER ret;
	if (buffer + BUFFER_SIZE - bufferp < size) {
		save();
	}

	ret = (HEADER)bufferp;
	bufferp += size;

	ret->type = type;
	ret->past = now() - lasttime;
	lasttime = now();

	return ret;
}

// ---- 記録 API

void oprecord_record_fdd(UINT8 drv, const char *fname, UINT8 readonly) {
	E_FDD e;

	if (!recording) return;

	e = (E_FDD)new_event(TYPE_FDD, sizeof(_E_FDD));
	e->drv = drv;
	e->readonly = readonly;
	ZeroMemory(e->filename, sizeof(e->filename));
	if (fname) {
		file_cpyname(e->filename, fname, sizeof(e->filename));
	}
}

void oprecord_record_joypad(UINT8 id, UINT8 arrow, UINT8 button) {
	E_JOYPAD e;

	if (!recording) return;

	// 現在の状態と比較
	if (s_joypad.h.type != STATE_UNKNOWN) {
		if (s_joypad.arrow == arrow &&
			s_joypad.button == button) {

			return;
		}
	}

	// 状態に変更がある
	e = (E_JOYPAD)new_event(TYPE_JOYPAD, sizeof(_E_JOYPAD));
	e->id = id;
	e->arrow = arrow;
	e->button = button;

	s_joypad = *e;
}

void oprecord_record_key(UINT8 code) {
	E_KEY e;

	if (!recording) return;

	e = (E_KEY)new_event(TYPE_KEY, sizeof(_E_KEY));
	e->code = code;
}


/* 使わない
int oprecord_start_record(const char *filename) {
	FILEH fh;

	if (recording) return -1;
	if (playing) {
		oprecord_stop_play();
	}

	// ファイルを開く
	fh = file_create(filename);
	if (fh == FILEH_INVALID) {
		// 作成失敗
		return -1;
	}

	return oprecord_start_record2(fh);
}
*/

int oprecord_start_record2(FILEH _fh) {
	if (recording) return -1;
	if (playing) {
		oprecord_stop_play();
	}

	fh = _fh;

	// ヘッダ書き出し
	file_write(fh, &oprecordheader, sizeof(oprecordheader));

	// 変数初期化
	bufferp = buffer;
	lasttime = now();

	s_joypad.h.type = STATE_UNKNOWN;

	recording = TRUE;
	return 0;
}

void oprecord_stop_record(void) {
	if (!recording) return;

	new_event(TYPE_END, sizeof(_HEADER));
	save();

	// ファイルを閉じる
	file_close(fh);

	recording = FALSE;
}

BOOL oprecord_recording(void) {
	return recording;
}


// ---- 再生

static void load(void) {
	BYTE *dest;

	if (bufferp == buffer) return;

	// bufferp からバッファの最後までを、バッファの先頭に移動
	dest = buffer;
	while (bufferp < buffer + BUFFER_SIZE) {
		*dest++ = *bufferp++;
	}

	// あいているバッファに読み込み
	file_read(fh, dest, BUFFER_SIZE - (dest - buffer));

	bufferp = buffer;
}

static HEADER peek_next_event(void) {
	if (buffer + BUFFER_SIZE - bufferp < sizeof(_HEADER)) {
		load();
	}
	return (HEADER)bufferp;
}

static HEADER next_event(int size) {
	HEADER ret;

	if (buffer + BUFFER_SIZE - bufferp < size) {
		load();
	}

	ret = (HEADER)bufferp;
	bufferp += size;

	lasttime += ret->past;

	return ret;
}

static void update_fdd(void) {
	E_FDD e;
	const char *filename = NULL;

	e = (E_FDD)next_event(sizeof(_E_FDD));
	if (e->filename[0] != '\0') filename = e->filename;
	if (setfdd) {
		setfdd(e->drv, filename, e->readonly);
	}
}

static void update_joypad(void) {
	E_JOYPAD e;

	e = (E_JOYPAD)next_event(sizeof(_E_JOYPAD));
	s_joypad.arrow = e->arrow;
	s_joypad.button = e->button;
}

static void update_key(void) {
	E_KEY e;

	e = (E_KEY)next_event(sizeof(_E_KEY));
	keyboard_send(e->code);
}

// ---- 再生 API

void oprecord_play_update(void) {
	HEADER e;
	UINT32 n;

	if (!playing) return;

	n = now();
	e = peek_next_event();
	while (e->past <= n - lasttime) {
		switch(e->type) {
		case TYPE_END:
		default:
			oprecord_stop_play();
//			if (oprecord.repeat) restartplay();
			goto exit;
			break;
		case TYPE_FDD:
			update_fdd();
			break;
		case TYPE_JOYPAD:
			update_joypad();
			break;
		case TYPE_KEY:
			update_key();
			break;
		}
		e = peek_next_event();
	}
exit:;
}
/* 使わない
int oprecord_start_play(const char *filename) {
	if (playing) return -1;
	if (recording) {
		oprecord_stop_record();
	}
	// ファイルを開く
	fh = file_open_rb(filename);
	if (fh == FILEH_INVALID) {
		return -1;
	}
	return oprecord_start_play2(fh);
}
*/
int oprecord_start_play2(FILEH _fh) {
	_OPRECORDHEADER header;

	if (playing) return -1;
	if (recording) {
		oprecord_stop_record();
	}

	fh = _fh;	

	// ヘッダを読み込む
	file_read(fh, &header, sizeof(header));

	bufferp = buffer + BUFFER_SIZE;
	load();
	lasttime = now();

	playing = TRUE;
	return 0;
}

void oprecord_stop_play(void) {
	if (!playing) return;

	// ファイルを閉じる
	file_close(fh);

	playing = FALSE;

	if (playcompleted) playcompleted();
}

int oprecord_check_version(FILEH fh, char *ver, UINT size) {
	_OPRECORDHEADER header;

	// ヘッダを読み込む
	file_read(fh, &header, sizeof(header));

	if (milstr_cmp(header.vaegrel, VAEGREL_CORE)) {
		// リリース不一致
		milstr_ncpy(ver, header.vaegrel, size);
		return 1;
	}
	return 0;
}

BOOL oprecord_playing(void) {
	return playing;
}

int oprecord_play_joypad(UINT8 id, UINT8 *arrow, UINT8 *button) {
	if (!playing) return -1;

	oprecord_play_update();

	*arrow = s_joypad.arrow;
	*button = s_joypad.button;
	return 0;
}

#endif