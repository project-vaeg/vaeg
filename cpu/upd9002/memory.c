#include "compiler.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "memoryva.h"
#include "upd9002_trace.h"
#if defined(VAEG_UPD9002_SSTS_TESTING)
#include "tests/upd9002/direct_harness.h"
#endif

/* Emulator storage includes main/HMA backing and the host font range. */
BYTE mem[0x200000];

#if defined(VAEG_UPD9002_SSTS_TESTING)
static BOOL upd9002_test_flat_memory;
static UINT32 upd9002_test_flat_memory_reads;
static UINT32 upd9002_test_flat_memory_writes;

void upd9002_test_flat_memory_set(BOOL active) {
	upd9002_test_flat_memory = active;
}

void upd9002_test_flat_memory_reset_counters(void) {
	upd9002_test_flat_memory_reads = 0;
	upd9002_test_flat_memory_writes = 0;
}

UINT32 upd9002_test_flat_memory_read_count(void) {
	return upd9002_test_flat_memory_reads;
}

UINT32 upd9002_test_flat_memory_write_count(void) {
	return upd9002_test_flat_memory_writes;
}

static BOOL upd9002_test_flat_memory_active(void) {
	return upd9002_test_flat_memory || upd9002_ssts_io_active();
}
#endif

/*
 * These helpers expose only the storage behind VA main-memory map entries.
 * They deliberately do not perform VA bank decoding; memoryva owns that
 * decision. The first 64 KiB above 1 MiB is the HMA in mem[], while higher
 * addresses use the uPD9002 expansion-memory allocation.
 */
REG8 MEMCALL upd9002_mainram_read(UINT32 address) {
	if (address < USE_HIMEM) {
		if ((address < UPD9002_MAINRAM_LIMIT) && (address >= pccore_mainram_limit())) {
			return 0xff;
		}
		return mem[address];
	}
	address -= 0x100000;
	if ((CPU_EXTMEM != NULL) && (address < CPU_EXTMEMSIZE)) {
		return CPU_EXTMEM[address];
	}
	return 0xff;
}

REG16 MEMCALL upd9002_mainram_read_w(UINT32 address) {
	REG16 value;

	value = upd9002_mainram_read(address);
	value |= (REG16)upd9002_mainram_read(address + 1) << 8;
	return value;
}

void MEMCALL upd9002_mainram_write(UINT32 address, REG8 value) {
	if (address < USE_HIMEM) {
		if ((address < UPD9002_MAINRAM_LIMIT) && (address >= pccore_mainram_limit())) {
			return;
		}
		mem[address] = (BYTE)value;
		return;
	}
	address -= 0x100000;
	if ((CPU_EXTMEM != NULL) && (address < CPU_EXTMEMSIZE)) {
		CPU_EXTMEM[address] = (BYTE)value;
	}
}

void MEMCALL upd9002_mainram_write_w(UINT32 address, REG16 value) {
	upd9002_mainram_write(address, (REG8)value);
	upd9002_mainram_write(address + 1, (REG8)(value >> 8));
}

/*
 * Normal CPU and DMA accesses always enter the native VA decoder. The SST
 * instruction harness is intentionally isolated from machine devices and
 * therefore retains its explicit flat 1 MiB test-memory seam.
 */
REG8 MEMCALL upd9002_memoryread(UINT32 address) {
#if defined(VAEG_UPD9002_SSTS_TESTING)
	if (upd9002_test_flat_memory_active()) {
		upd9002_test_flat_memory_reads++;
		return mem[address & 0xfffff];
	}
#endif
	return upd9002_memoryread_va(address);
}

REG16 MEMCALL upd9002_memoryread_w(UINT32 address) {
#if defined(VAEG_UPD9002_SSTS_TESTING)
	if (upd9002_test_flat_memory_active()) {
		upd9002_test_flat_memory_reads += 2;
		return (REG16)(mem[address & 0xfffff] | (mem[(address + 1) & 0xfffff] << 8));
	}
#endif
	return upd9002_memoryread_va_w(address);
}

void MEMCALL upd9002_memorywrite(UINT32 address, REG8 value) {
#if defined(VAEG_UPD9002_SSTS_TESTING)
	if (upd9002_test_flat_memory_active()) {
		upd9002_test_flat_memory_writes++;
		mem[address & 0xfffff] = (BYTE)value;
		return;
	}
#endif
	upd9002_trace_event(UPD9002_TRACE_ORIGIN_CPU, "memory-write", (uint32_t)address,
	                    (uint32_t)value, 1);
	upd9002_memorywrite_va(address, value);
}

void MEMCALL upd9002_memorywrite_w(UINT32 address, REG16 value) {
#if defined(VAEG_UPD9002_SSTS_TESTING)
	if (upd9002_test_flat_memory_active()) {
		upd9002_test_flat_memory_writes += 2;
		mem[address & 0xfffff] = (BYTE)value;
		mem[(address + 1) & 0xfffff] = (BYTE)(value >> 8);
		return;
	}
#endif
	upd9002_trace_event(UPD9002_TRACE_ORIGIN_CPU, "memory-write", (uint32_t)address,
	                    (uint32_t)value, 2);
	upd9002_memorywrite_va_w(address, value);
}

REG16 MEMCALL upd9002_memoryread_seg_w(UINT32 segment_base, UINT off) {
	UINT32 address;
	UINT32 high_address;

	address = segment_base + LOW16(off);
	high_address = segment_base + LOW16(off + 1);
	/*
     * A 16-bit offset wraps within its segment. Contiguous pairs may use the
     * VA word decoder; the wrap case is two independently mapped byte reads.
     */
	if (high_address == (address + 1)) {
		return upd9002_memoryread_w(address);
	}
	return (REG16)(upd9002_memoryread(address) | (upd9002_memoryread(high_address) << 8));
}

void MEMCALL upd9002_memorywrite_seg_w(UINT32 segment_base, UINT off, REG16 value) {
	UINT32 address;
	UINT32 high_address;

	address = segment_base + LOW16(off);
	high_address = segment_base + LOW16(off + 1);
	if (high_address == (address + 1)) {
		upd9002_memorywrite_w(address, value);
		return;
	}
	upd9002_memorywrite(address, (REG8)value);
	upd9002_memorywrite(high_address, (REG8)(value >> 8));
}

REG8 MEMCALL meml_read8(UINT seg, UINT off) {
	return upd9002_memoryread(((UINT32)seg << 4) + LOW16(off));
}

REG16 MEMCALL meml_read16(UINT seg, UINT off) {
	return upd9002_memoryread_seg_w((UINT32)seg << 4, off);
}

void MEMCALL meml_write8(UINT seg, UINT off, REG8 value) {
	upd9002_memorywrite(((UINT32)seg << 4) + LOW16(off), value);
}

void MEMCALL meml_write16(UINT seg, UINT off, REG16 value) {
	upd9002_memorywrite_seg_w((UINT32)seg << 4, off, value);
}

void MEMCALL meml_readstr(UINT seg, UINT off, void *dat, UINT leng) {
	BYTE *out;
	UINT32 segment_base;

	out = (BYTE *)dat;
	segment_base = (UINT32)seg << 4;
	off = LOW16(off);
	while (leng--) {
		*out++ = upd9002_memoryread(segment_base + off);
		off = LOW16(off + 1);
	}
}

void MEMCALL meml_writestr(UINT seg, UINT off, const void *dat, UINT leng) {
	const BYTE *in;
	UINT32 segment_base;

	in = (const BYTE *)dat;
	segment_base = (UINT32)seg << 4;
	off = LOW16(off);
	while (leng--) {
		upd9002_memorywrite(segment_base + off, *in++);
		off = LOW16(off + 1);
	}
}

void MEMCALL meml_read(UINT32 address, void *dat, UINT leng) {
	BYTE *out;

	out = (BYTE *)dat;
	while (leng--) {
		*out++ = upd9002_memoryread(address++);
	}
}

void MEMCALL meml_write(UINT32 address, const void *dat, UINT leng) {
	const BYTE *in;

	in = (const BYTE *)dat;
	while (leng--) {
		upd9002_memorywrite(address++, *in++);
	}
}
