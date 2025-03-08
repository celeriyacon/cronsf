/*
 * s6502.h
 *
 * Copyright (C) 2024-2025 celeriyacon - https://github.com/celeriyacon
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 */

#ifndef CRONSF_S6502_H
#define CRONSF_S6502_H

#if __ASSEMBLER__
//
//
//
.macro ASSERT v
	.if (\v)
	.else
		.error "Assertion \v failed!"
	.endif
.endm
//#define ASSERT(v)

#define REG_CYC_CNT r4		// Cycle counter
#define REG_ADDR r5		// REG_ADDR is passed to external read/write handlers and must not be modified.
				// May also be used as a scratch register inside S6502 and the event handler.
#define REG_DATA_BUS r6		// Data bus
#define REG_RAM	r7		// Pointer to start of 2KiB RAM and upper bound of 64KiB instruction handler area.

#define REG_NZ	r8		// N/Z flag value.  N = (REG_NZ < 0), Z = ((REG_NZ & 0xFF) == 0)
#define REG_PC	r9		// PC (pointer)
#define REG_SP	r10		// SP
#define REG_P	r11		// P (Carry, I, D, V)
#define REG_A	r12		// A
#define REG_X	r13		// X
#define REG_Y	r14		// Y
#define REG_PC_BASE r15		// PC (base pointer)

#define CREG_SCRATCH VBR	// Scratch register to save data across calls to read/write handlers.

#define SREG_RFAP MACL	// STS, MA/IF contention
#define SREG_WFAP MACH	// STS, MA/IF contention

#endif
//
//
//
#define GBR_OFFS_BANK0 0x0 * 4
#define GBR_OFFS_BANK1 0x1 * 4
#define GBR_OFFS_BANK2 0x2 * 4
#define GBR_OFFS_BANK3 0x3 * 4
#define GBR_OFFS_BANK4 0x4 * 4
#define GBR_OFFS_BANK5 0x5 * 4
#define GBR_OFFS_BANK6 0x6 * 4
#define GBR_OFFS_BANK7 0x7 * 4

#define GBR_OFFS_BANK8 0x8 * 4
#define GBR_OFFS_BANK9 0x9 * 4
#define GBR_OFFS_BANKA 0xA * 4
#define GBR_OFFS_BANKB 0xB * 4
#define GBR_OFFS_BANKC 0xC * 4
#define GBR_OFFS_BANKD 0xD * 4
#define GBR_OFFS_BANKE 0xE * 4
#define GBR_OFFS_BANKF 0xF * 4
//
//
//
#define GBR_OFFS_TIMESTAMP_BASE 	0x10 * 4
#define GBR_OFFS_FC_NEXT_TIMESTAMP	0x11 * 4
#define GBR_OFFS_DMC_END_TIMESTAMP	0x12 * 4
#define GBR_OFFS_APU_STATUS		0x13 * 4

#define GBR_OFFS_NSF_ROM_BASE    0x58
#define GBR_OFFS_NSF_FRAME_PENDING 0x5C
//
//
#define GBR_OFFS_EXCHIP_RB_WR 0x5E
//
//
//
//
#define GBR_OFFS_N163_RAM_PTR 0x60
#define GBR_OFFS_N163_ADDR 0x64
#define GBR_OFFS_N163_ADDR_INC 0x65
//
#define GBR_OFFS_SUN5B_ADDR 0x66
//
#define GBR_OFFS_MMC5_MASTER 0x70
#define GBR_OFFS_MMC5_MASTER_FRAME_DIVIDER (GBR_OFFS_MMC5_MASTER + 0x0)
#define GBR_OFFS_MMC5_MASTER_ENVMODE0 (GBR_OFFS_MMC5_MASTER + 0x4)
#define GBR_OFFS_MMC5_MASTER_ENVMODE1 (GBR_OFFS_MMC5_MASTER + 0x5)
#define GBR_OFFS_MMC5_MASTER_LENGTH_COUNTER0 (GBR_OFFS_MMC5_MASTER + 0x6)
#define GBR_OFFS_MMC5_MASTER_LENGTH_COUNTER1 (GBR_OFFS_MMC5_MASTER + 0x7)
#define GBR_OFFS_MMC5_MASTER_STATUS (GBR_OFFS_MMC5_MASTER + 0x8)
#define GBR_OFFS_MMC5_MASTER_LC_ENABLE (GBR_OFFS_MMC5_MASTER + 0x9)
#define GBR_OFFS_MMC5_MASTER_MULT0 (GBR_OFFS_MMC5_MASTER + 0xA)
#define GBR_OFFS_MMC5_MASTER_MULT1 (GBR_OFFS_MMC5_MASTER + 0xC)
#define GBR_OFFS_MMC5_MASTER_MULT_RES (GBR_OFFS_MMC5_MASTER + 0xE)

#if !__ASSEMBLER__
//
//
//
#include <assert.h>

void s6502_init(void); //uint8* ram_ptr);
void s6502_power(void);
void s6502_reset(void);
void s6502_run(unsigned cycles);
void s6502_step(void);

extern uint8 ram[0x800];
//
//
//
extern uint8 rfap[0x10000];
extern uintptr_t wfap[0x10000];

extern void s6502_write_shim_S(void);

extern void ob_rw_func_S(void);
extern void ram_mirror_write_func_S(void);
extern void ram_mirror_read_func_S(void);
extern void ram_write_func_S(void);
extern void ram_read_func_S(void);

extern void apu_read_4015_S(void);
//extern void apu_write_4010_S(void);
//extern void apu_write_4011_S(void);
//extern void apu_write_4012_S(void);
//extern void apu_write_4013_S(void);

// Bank pointers *must* be aligned to 256-byte boundaries,
// i.e. the lower 8 bits of banks_4k[*] must be all 0!
//
// (Otherwise, the cycle overhead calculation for relative
// branches will be broken).
extern uintptr_t banks_4k[0x10];
void bs2xxx_read_func_S(void);
extern void bs6xxx_write_func_S(void);
extern void bs7xxx_write_func_S(void);
extern void bs6xxx_read_func_S(void);
extern void bs7xxx_read_func_S(void);
extern void bs8xxx_read_func_S(void);
extern void bs9xxx_read_func_S(void);
extern void bsAxxx_read_func_S(void);
extern void bsBxxx_read_func_S(void);
extern void bsCxxx_read_func_S(void);
extern void bsDxxx_read_func_S(void);
extern void bsExxx_read_func_S(void);
extern void bsFxxx_read_func_S(void);

extern void vrc6_write_func_S(void);
extern void n163_write_f800_func_S(void);
extern void n163_write_4800_func_S(void);
extern void n163_read_4800_func_S(void);
extern void sun5b_write0_func_S(void);
extern void sun5b_write1_func_S(void);

extern void mmc5_write0_func_S(void);
extern void mmc5_write3_func_S(void);
extern void mmc5_write4_func_S(void);
extern void mmc5_write7_func_S(void);
extern void mmc5_writeX_func_S(void);

extern void mmc5_write_status_func_S(void);
extern void mmc5_read_status_func_S(void);

extern void mmc5_write_mult0_func_S(void);
extern void mmc5_write_mult1_func_S(void);
extern void mmc5_read_mult_res0_func_S(void);
extern void mmc5_read_mult_res1_func_S(void);


extern uint8* nsf_rom_base;
extern uint8 nsf_frame_pending;
extern uint32 timestamp_base;
extern uint32 fc_next_timestamp;
extern uint32 dmc_end_timestamp;
extern uint32 apu_status;
extern volatile uint16 exchip_rb_wr;

extern struct
{
 uint32 Y;
 uint32 X;
 uint32 A;
 uint32 P;
 uint32 SP;
 uintptr_t PC;
 uint32 NZ;
 uint32 RAM;
 //
 //
 //
 uint32 WFAP;
 uint32 RFAP;
 //
 uint32 DATA_BUS;	// !
 uint32 PR;
 uint32 ADDR;
 uint32 PC_BASE;
 uint32 CYC_CNT;
} s6502;

typedef struct
{
 uint32 frame_divider;
 uint8 envmode0;
 uint8 envmode1;
 uint8 length_counter0;
 uint8 length_counter1;
 uint8 status;
 uint8 lc_enable;
 uint16 mult0;	// Upper 8 bits must be zero.
 uint16 mult1;  // Upper 8 bits must be zero.
 uint16 mult_res;
} mmc5_master_t;

_Static_assert(__builtin_offsetof(mmc5_master_t, frame_divider) == (GBR_OFFS_MMC5_MASTER_FRAME_DIVIDER - GBR_OFFS_MMC5_MASTER), "Structure malformed.");
_Static_assert(__builtin_offsetof(mmc5_master_t, envmode0) == (GBR_OFFS_MMC5_MASTER_ENVMODE0 - GBR_OFFS_MMC5_MASTER), "Structure malformed.");
_Static_assert(__builtin_offsetof(mmc5_master_t, envmode1) == (GBR_OFFS_MMC5_MASTER_ENVMODE1 - GBR_OFFS_MMC5_MASTER), "Structure malformed.");
_Static_assert(__builtin_offsetof(mmc5_master_t, length_counter0) == (GBR_OFFS_MMC5_MASTER_LENGTH_COUNTER0 - GBR_OFFS_MMC5_MASTER), "Structure malformed.");
_Static_assert(__builtin_offsetof(mmc5_master_t, length_counter1) == (GBR_OFFS_MMC5_MASTER_LENGTH_COUNTER1 - GBR_OFFS_MMC5_MASTER), "Structure malformed.");
_Static_assert(__builtin_offsetof(mmc5_master_t, status) == (GBR_OFFS_MMC5_MASTER_STATUS - GBR_OFFS_MMC5_MASTER), "Structure malformed.");
_Static_assert(__builtin_offsetof(mmc5_master_t, lc_enable) == (GBR_OFFS_MMC5_MASTER_LC_ENABLE - GBR_OFFS_MMC5_MASTER), "Structure malformed.");
_Static_assert(__builtin_offsetof(mmc5_master_t, mult0) == (GBR_OFFS_MMC5_MASTER_MULT0 - GBR_OFFS_MMC5_MASTER), "Structure malformed.");
_Static_assert(__builtin_offsetof(mmc5_master_t, mult1) == (GBR_OFFS_MMC5_MASTER_MULT1 - GBR_OFFS_MMC5_MASTER), "Structure malformed.");
_Static_assert(__builtin_offsetof(mmc5_master_t, mult_res) == (GBR_OFFS_MMC5_MASTER_MULT_RES - GBR_OFFS_MMC5_MASTER), "Structure malformed.");

static INLINE void set_bank_4k(unsigned w, uint8* p)
{
 uintptr_t tmp = (uintptr_t)p - (w << 12);

 assert(!(tmp & 0xFF));

 banks_4k[w] = tmp;
}

static INLINE void set_bank_8k(unsigned w, uint8* p)
{
 set_bank_4k(w + 0, p + 0x0000);
 set_bank_4k(w + 1, p + 0x1000);
}

static INLINE void set_bank_16k(unsigned w, uint8* p)
{
 set_bank_8k(w + 0, p + 0x0000);
 set_bank_8k(w + 2, p + 0x2000);
}

static INLINE void set_bank_32k(unsigned w, uint8* p)
{
 set_bank_16k(w + 0, p + 0x0000);
 set_bank_16k(w + 4, p + 0x4000);
}

#endif

#endif
