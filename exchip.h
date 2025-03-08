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
} exchip_tables_t;

extern exchip_tables_t exchip_tables LORAM_BSS_UNCACHED;

// Filled with data from resamp_div15 or resamp_div16 in *_slave_entry().
extern int16 exchip_resamp[256 * 40];

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
 extern const int16 exchip_sintab[4096];

 #define EXCHIP_SINESWEEP_DO(av)			\
 {							\
  static uint32 ph = 0;				\
  static uint32 ph_inc = 0;				\
  static uint32 ph_inc_inc = 1024;			\
  int32 a, b;						\
							\
  a = exchip_sintab[  ph >> (32 - 12)];			\
  b = exchip_sintab[((ph >> (32 - 12)) + 1) & 4095];	\
							\
  av = a + (((b - a) * ((ph >> 12) & 0xFF)) >> 8);	\
							\
  ph += ph_inc;					\
  ph_inc += ph_inc_inc;				\
 }
#else
 #define EXCHIP_SINESWEEP_DO(av) (void)av;
#endif

#endif
