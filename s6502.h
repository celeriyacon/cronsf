/*
 * s6502.h
 *
 * Copyright (C) 2024 celeriyacon - https://github.com/celeriyacon
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
extern uint8 wfap[0x10000];

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

extern uint8* nsf_rom_base;
extern uint8 nsf_frame_pending;
extern uint32 timestamp_base;
extern uint32 fc_next_timestamp;
extern uint32 dmc_end_timestamp;
extern uint32 apu_status;

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
