/*
 * subsystem.h: PC-88VA FD Sub System
 */

#include "i8255.h"

typedef struct {
	BYTE romexist;  // ROMをロードした
	BYTE intopcode; // 割り込みオペコード
	_I8255 i8255;
	BYTE rom[0x2000];
	BYTE ram[0x4000];
} _SUBSYSTEM, *SUBSYSTEM;

#ifdef __cplusplus
extern "C" {
#endif

extern _SUBSYSTEM subsystem;

void subsystem_initialize(void);
void subsystem_exec(void);
BYTE subsystem_readmem(WORD addr);
const struct UPD780Reg *subsystem_getcpureg(void);
enum {
	SUBSYSTEM_DISASSEMBLY_CAPACITY = 64
};
WORD subsystem_disassemble_bounded(WORD pc, char *str, UINT capacity);
WORD subsystem_disassemble(WORD pc, char *str);
UINT subsystem_getcpustatussize(void);
BOOL subsystem_savecpustatus(UINT8 *buf);
BOOL subsystem_loadcpustatus(const UINT8 *buf);

void subsystem_businporta(BYTE dat);
void subsystem_businportb(BYTE dat);
void subsystem_businportc(BYTE dat);

void subsystem_reset(void);
void subsystem_irq(BOOL irq);

#if defined(VAEG_UPD780_INTEGRATION_TESTING)
typedef struct {
	UINT f4_count;
	BYTE f4_last_value;
	UINT acknowledge_count;
	UINT fe_read_count;
	UINT wait_assert_count;
	UINT wait_release_count;
	WORD sleep_live_pc;
	WORD sleep_public_pc;
	BYTE sleep_path;
	BYTE sleep_port_value;
	BYTE sleep_memory_value;
	BOOL wait_active;
} VAEG_UPD780_INTEGRATION_TRACE_STATE;

typedef struct {
	UINT16 af;
	UINT16 bc;
	UINT16 de;
	UINT16 hl;
	UINT16 sp;
	UINT16 live_pc;
	UINT16 public_pc;
	UINT8 irq;
	UINT8 wait_flags;
	SINT32 remainclock;
	SINT32 lastclock;
} VAEG_UPD780_INTEGRATION_CPU_STATE;

void subsystem_upd780_test_reset(void);
void subsystem_upd780_test_install(WORD address, const UINT8 *data, UINT size);
void subsystem_upd780_test_set_pc(WORD pc);
void subsystem_upd780_test_set_clock(UINT32 now);
void subsystem_upd780_test_set_wait(BOOL wait);
void subsystem_upd780_test_reset_trace(void);
void subsystem_upd780_test_get_trace(VAEG_UPD780_INTEGRATION_TRACE_STATE *trace);
BOOL subsystem_upd780_test_get_state(VAEG_UPD780_INTEGRATION_CPU_STATE *state);
#endif

#ifdef __cplusplus
}
#endif
