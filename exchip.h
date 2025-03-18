/*
 * exchip.h
 *
 * Copyright (C) 2025 celeriyacon - https://github.com/celeriyacon
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

#ifndef EXCHIP_H
#define EXCHIP_H

#include "config.h"

#include "gs/types.h"
#include "gs/sh2.h"

#include "s6502.h"

typedef struct
{
 uint16 samples[715202 / 2];
 int16 resamp_n163[256 * 40];
 int16 resamp_div16[256 * 40];
 int16 sine_half[4096 / 2];
} exchip_tables_t;

extern exchip_tables_t exchip_tables LORAM_BSS_UNCACHED;

// Filled with data from resamp_div15 or resamp_div16 in *_slave_entry().
extern int16 exchip_resamp[256 * 40];

enum
{
 SLAVECMD_FORCE_UPDATE = 0xFD,
 SLAVECMD_FRAME = 0xFE,
 SLAVECMD_STOP = 0xFF
};

extern volatile int16 nsfcore_slave_ready;

extern volatile uint32 exchip_rb[16384];
extern volatile uint16 exchip_rb_wr;

extern volatile uint32 exchip_bench[8];
extern volatile uint32 exchip_bench_rb_wr;

#if EXCHIP_BENCH
 //
 //
 #define EXCHIP_BENCH_PROLOGUE	\
  uint32 bench_prev_time;	\
  uint32 bench_time_taken = 0;	\
  TCR = 0;			\
  TOCR = 0;			\

#define EXCHIP_BENCH_BEGIN	\
  bench_prev_time = FRCH << 8;	\
  bench_prev_time |= FRCL << 0;

 #define EXCHIP_BENCH_END	\
  {				\
   uint32 bench_cur_time;	\
   bench_cur_time = FRCH << 8;	\
   bench_cur_time |= FRCL << 0;	\
   bench_time_taken += ((bench_cur_time - bench_prev_time) & 0xFFFF);	\
   bench_prev_time = bench_cur_time;	\
  }

 #define EXCHIP_BENCH_END_FRAME	\
  {				\
   uint32 bench_cur_time;	\
   bench_cur_time = FRCH << 8;	\
   bench_cur_time |= FRCL << 0;	\
   bench_time_taken += ((bench_cur_time - bench_prev_time) & 0xFFFF);	\
   bench_prev_time = bench_cur_time;		\
   /**/						\
   {						\
    uint32 ebrbwr = exchip_bench_rb_wr;		\
    exchip_bench[ebrbwr] = bench_time_taken << 3;	\
    ebrbwr = (ebrbwr + 1) & 7;			\
    exchip_bench_rb_wr = ebrbwr;		\
    bench_time_taken = 0;			\
   }						\
  }
 //
 //
#else
 #define EXCHIP_BENCH_PROLOGUE ((void)0);
 #define EXCHIP_BENCH_BEGIN ((void)0);
 #define EXCHIP_BENCH_END ((void)0);
 #define EXCHIP_BENCH_END_FRAME ((void)0);
#endif

#if EXCHIP_SINESWEEP
 #define EXCHIP_SINESWEEP_DO(av)			\
 {							\
  static uint32 ph = 0;					\
  static uint32 ph_inc = 0;				\
  static uint32 ph_inc_inc = 1024;			\
  const unsigned idx_a = ph >> (32 - 12);		\
  const unsigned idx_b = (idx_a + 1) & 4095;		\
  int32 a, b;						\
							\
  a = exchip_tables.sine_half[idx_a & 2047];		\
  b = exchip_tables.sine_half[idx_b & 2047];		\
							\
  if(idx_a & 2048)					\
   a = -a;						\
							\
  if(idx_b & 2048)					\
   b = -b;						\
							\
  av = a + (((b - a) * ((ph >> 12) & 0xFF)) >> 8);	\
							\
  ph += ph_inc;					\
  ph_inc += ph_inc_inc;				\
 }
#else
 #define EXCHIP_SINESWEEP_DO(av) (void)av;
#endif

#define EXCHIP_RELOC(chip_, objp_)						\
 {										\
  extern uint8 chip_##_reloc_dest[];						\
  extern uint8 chip_##_reloc_start[];						\
  extern uint8 chip_##_reloc_bound[];						\
  volatile uint8* s = chip_##_reloc_start;					\
  volatile uint8* d = chip_##_reloc_dest;					\
  const size_t reloc_size = (chip_##_reloc_bound - chip_##_reloc_start);	\
  uint32 sp;									\
  int32 ss;									\
  asm volatile("mov r15, %0\n\t" :"=r"(sp):"r"(objp_));				\
										\
  ss = (sp - (((unsigned)chip_##_reloc_dest & 0xC0000FFF) + reloc_size));	\
  /*printf("%08x, %08zx, %08x\n", sp, reloc_size, ss);*/			\
										\
  assert(ss >= 0x140);								\
  while(s != chip_##_reloc_bound)						\
  {										\
   *d = *s;									\
   s++;										\
   d++;										\
  }										\
 }


#endif
