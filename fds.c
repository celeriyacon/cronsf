/*
 * fds.c
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

#include "exchip.h"
#include "fds.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

typedef struct
{
 int16 divider;
 int8 wt_control;
 int8 mod_control;

 uint32 ph[2];
 uint32 ph_inc;
 uint32 eff_mod_ph_inc;

 uint32 mod_counter;

 int32 master_volume;

 int32 ww;

 int32 lpf;
 int32 lpf_coeff;
 //
 //
 int32 vol_latch;
 int32 eff_vol;

 int32 sample_latch;

 int32 env_level[2];

 int32 prev_timestamp;

 uint32 env_pre_divider;
 int16 env_pre_period;	// +1) << 3

 int8 env_divider[2];
 int8 env_period[2];	// +1
 int8 env_control[2];
 //
 //
 uint32 rb_rd;
 //
 uint32 resamp_divider;
 uint32 resamp_scale;
 uint32 sample_offs;

 uint32 source_pos;
 uint32 source_pos_inc;

 uint32 source_pos_f;
 uint32 source_pos_f_inc;

 uint32 buf_offs;

 volatile uint16* scsp_ptr;

 uint32 mod_ph_inc;

 int8 modram[0x20];
 int32 wtram[0x40];
 int16 buf[64 * 2];
 int32 wtx[0x40];
} fds_t;

static __attribute__((section(".fds_reloc"))) __attribute__((noinline,noclone)) void fds_run_(fds_t* f, int32 timestamp)
{
 f->divider += (uint16)(timestamp - f->prev_timestamp);
 f->prev_timestamp = timestamp;

 while(f->divider >= 0)
 {
  f->divider -= 16;
  //
  //
  uint32 av;

#if !EXCHIP_SINESWEEP
#if 1
  {
   uint32 tmp0, tmp1, tmp2, tmp3, tmp4;

   asm volatile(
	".align 2\n\t"

	"mov.l %[f_ph_0], %[tmp0]\n\t"
	"mov %[f], %[tmp4]\n\t"

	"mov.l %[f_eff_vol], %[tmp1]\n\t"
	"shlr16 %[tmp0]\n\t"

	"mov.l %[f_ww], %[tmp2]\n\t"
	"and #0xFC, %[tmp0]\n\t"

	"mov.l %[f_vol_latch], %[tmp3]\n\t"
	"cmp/eq #0, %[tmp0]\n\t"

	"bf/s 1f\n\t"
	"add %[wtram_offset_a], %[tmp4]\n\t"

	"mov.l %[tmp1], %[f_vol_latch]\n\t"
	"mov %[tmp1], %[tmp3]\n\t"

	"1:\n\t"
	"mov.l %[f_master_volume], %[tmp1]\n\t"
	"cmp/pl %[tmp2]\n\t"

	"muls.w %[tmp1], %[tmp3]\n\t"	// master_volume * vol_latch
	"bt/s 2f\n\t"

	"mov.l %[f_sample_latch], %[tmp1]\n\t"	// Delay slot
	"add %[wtram_offset_b], %[tmp4]\n\t"

	"mov.l @(%[tmp0], %[tmp4]), %[tmp1]\n\t"
	"nop\n\t"

	"2:\n\t"
	"mov.l %[tmp1], %[f_sample_latch]\n\t"
	"sts MACL, %[tmp3]\n\t"			// STALL

	"mul.l %[tmp1], %[tmp3]\n\t"		// sample_latch * (master_volume * vol_latch)
	"nop\n\t"

	"mov.l %[f_lpf], %[tmp1]\n\t"
	"nop\n\t"

	"mov.l %[f_lpf_coeff], %[tmp2]\n\t"
	"sts MACL, %[tmp3]\n\t"			// STALL

	"shlr8 %[tmp3]\n\t"
	"sub %[tmp1], %[tmp3]\n\t"

	"dmuls.l %[tmp2], %[tmp3]\n\t"
	"nop\n\t"

	"sts MACH, %[tmp3]\n\t"			// BIG STALL
	"add %[tmp3], %[tmp1]\n\t"		// STALL

	"mov.l %[tmp1], %[f_lpf]\n\t"
	"shlr8 %[tmp1]\n\t"

	: [tmp0] "=&z"(tmp0), [tmp1] "=&r"(tmp1), [tmp2] "=&r"(tmp2), [tmp3] "=&r"(tmp3), [tmp4] "=&r"(tmp4)
	: [f] "r"(f),
	  [f_ph_0] "m"(f->ph[0]),
	  [f_vol_latch] "m"(f->vol_latch),
	  [f_eff_vol] "m"(f->eff_vol),
	  [f_ww] "m"(f->ww),
	  [f_sample_latch] "m"(f->sample_latch),
	  [f_lpf] "m"(f->lpf),
	  [f_lpf_coeff] "m"(f->lpf_coeff),
	  [f_master_volume] "m"(f->master_volume),
	  [wtram_offset_a] "I08"((__builtin_offsetof(fds_t, wtram) > 127) ? 127 : __builtin_offsetof(fds_t, wtram)),
	  [wtram_offset_b] "I08"(__builtin_offsetof(fds_t, wtram) - ((__builtin_offsetof(fds_t, wtram) > 127) ? 127 : __builtin_offsetof(fds_t, wtram)))
	: "macl", "mach", "cc");

   av = tmp1;
  }
#else
  {
   uint32 t;
   unsigned wto = (f->ph[0] >> 18) & 0x3F;

   if(!wto)
    f->vol_latch = f->eff_vol;

   if(LIKELY(!f->ww))
    f->sample_latch = f->wtram[wto];

   t = f->sample_latch * f->vol_latch * f->master_volume;

   f->lpf += (((int32)(t >> 8) - f->lpf) * (int64)f->lpf_coeff) >> 32;
   av = (uint32)f->lpf >> 8;
  }
#endif
#endif
  //
  //
  //
  //
  EXCHIP_SINESWEEP_DO(av)
  //
  //
  //
  //
  {
   uint32 tmp0, tmp1, tmp2, tmp3, tmp4;
   uint32 resamp_scale_tmp;

   asm volatile(
	".align 2\n\t"

	"mov.l %[s_buf_offs], r0\n\t"
	"mov %[buf], %[tmp4]\n\t"

	"mov.w %[av], @(r0, %[tmp4])\n\t"
	"add #-128, %[tmp4]\n\t"

	"mov.w %[av], @(r0, %[tmp4])\n\t"
	"add #2, r0\n\t"

	"mov.l %[resamp_divider], %[tmp0]\n\t"
	"and #0x7E, r0\n\t"

	"mov.l %[s_resamp_scale], %[resamp_scale]\n\t"
	"mov %[tmp0], %[tmp2]\n\t"

	"mov.l r0, %[s_buf_offs]\n\t"
	"add %[resamp_scale], %[tmp0]\n\t"

	"mov.l %[tmp0], %[resamp_divider]\n\t"
	"cmp/hi %[tmp0], %[tmp2]\n\t"

	"bf/s skip_resample\n\t"
	"mov %[exchip_resamp], %[tmp1]\n\t"
	//
	//
	//
	"mov.l %[source_pos], r0\n\t"
	"clrmac\n\t"

	// (source_pos & 0xFF) * 2 * 40 -> tmp1
	"extu.b r0, %[tmp2]\n\t"
	"swap.b %[tmp2], %[tmp0]\n\t"

	"shlr2 %[tmp0]\n\t"
	"shll2 %[tmp2]\n\t"

	"shll2 %[tmp2]\n\t"
	"add %[tmp0], %[tmp1]\n\t"

	"add %[tmp2], %[tmp1]\n\t"
	"shlr8 r0\n\t"

	// (((source_pos >> 8) & 0x3F) * 2)
	"and #0x3F, r0\n\t"
	"add r0, r0\n\t"

	"mov.l %[sample_offs], %[tmp2]\n\t"
	"mov r0, %[tmp0]\n\t"

	"mov.l %[scsp_ptr], %[tmp3]\n\t"
	"add %[tmp4], %[tmp0]\n\t"

	// tmp0: waveform
	// tmp1: impulse
	// tmp2: sample_offs
	// tmp3: scsp_ptr
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"

	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"

	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"

	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"

	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"
	"mac.w @%[tmp0]+, @%[tmp1]+\n\t"

	"mov #0x10, %[tmp1]\n\t"
	"swap.b %[tmp1], %[tmp1]\n\t"

	"add #-1, %[tmp1]\n\t"
	"and %[tmp1], %[tmp2]\n\t"

	"sts macl, %[tmp0]\n\t"
	"add %[tmp2], %[tmp3]\n\t"

	"swap.w %[tmp0], %[tmp0]\n\t"
	"add #2, %[tmp2]\n\t"

	"mov.w %[tmp0], @%[tmp3]\n\t"	// accum -> scsp mem
	"mov.l %[tmp2], %[sample_offs]\n\t"	// 	STALL
	//
	//
	//
	"mov.l %[source_pos_f_inc], %[tmp2]\n\t"
	"mov.l %[source_pos_f], %[tmp3]\n\t"		// STALL

	"mov.l %[source_pos_inc], %[tmp0]\n\t"
	"add %[tmp2], %[tmp3]\n\t"

	"mov.l %[source_pos], %[tmp1]\n\t"
	"cmp/hs %[resamp_scale], %[tmp3]\n\t"

	"bf/s skip_f_adj\n\t"		//if(!(s->source_pos_f >= s->resamp_scale)) goto skip_f_adj;
	"add %[tmp0], %[tmp1]\n\t"
	//
	//
	"add #1, %[tmp1]\n\t"
	"sub %[resamp_scale], %[tmp3]\n\t"
	//
	//
	"skip_f_adj:\n\t"

	"mov.l %[tmp3], %[source_pos_f]\n\t"
	"mov.l %[tmp1], %[source_pos]\n\t"	// STALL
	//
	//
	//
	"skip_resample:\n\t"
	//
	//
	//
	//
	: [tmp0] "=&r"(tmp0), [tmp1] "=&r"(tmp1), [tmp2] "=&r"(tmp2), [tmp3] "=&r"(tmp3), [tmp4] "=&r"(tmp4), [resamp_scale] "=&r"(resamp_scale_tmp)
	: [av] "r"(av),
	  [exchip_resamp] "r"(exchip_resamp),
	  [buf] "r"((uint32)f->buf + 128),
	  [s_buf_offs] "m"(f->buf_offs),
	  [resamp_divider] "m"(f->resamp_divider),
	  [s_resamp_scale] "m"(f->resamp_scale),
	  [source_pos] "m"(f->source_pos),
	  [source_pos_inc] "m"(f->source_pos_inc),
	  [source_pos_f] "m"(f->source_pos_f),
	  [source_pos_f_inc] "m"(f->source_pos_f_inc),
	  [sample_offs] "m"(f->sample_offs),
	  [scsp_ptr] "m"(f->scsp_ptr)
	  //
	  //
	  //
	: "memory", "r0", "cc", "mach", "macl");
  }
  //
  //
  //
#if !EXCHIP_SINESWEEP
#if 1
  {
   uint32 tmp0, tmp1, tmp2, tmp3, tmp4;

   asm volatile(
	"mov.l %[f_mod_counter], %[tmp1]\n\t"
	"mov #0x10, %[tmp4]\n\t"

	"mov.l %[f_env_level_1], %[tmp2]\n\t"
	"add #-0x40, %[tmp1]\n\t"

	"muls.w %[tmp1], %[tmp2]\n\t"
	"shll16 %[tmp4]\n\t"

	"mov.b %[f_wt_control], %[tmp0]\n\t"
	"add #-1, %[tmp4]\n\t"

	"mov.l %[f_ph_inc_0], %[tmp3]\n\t"
	"tst #0x80, %[tmp0]\n\t"

	"sts MACL, %[tmp0]\n\t"
	"bf/s 1f\n\t"

	"tst #0x0F, %[tmp0]\n\t"	// Delay slot
	"shlr2 %[tmp0]\n\t"

	"bt/s 2f\n\t"
	"shlr2 %[tmp0]\n\t"		// Delay slot

	"tst #0x80, %[tmp0]\n\t"
	"bf/s 2f\n\t"

	"nop\n\t"	// Delay slot
	"add #0x02, %[tmp0]\n\t"

	"2:\n\t"
	"add #0x40, %[tmp0]\n\t"
	"and #0xFF, %[tmp0]\n\t"

	"muls.w %[tmp3], %[tmp0]\n\t"
	"nop\n\t"

	"mov.l %[f_ph_0], %[tmp2]\n\t"
	"nop\n\t"

	"sts MACL, %[tmp3]\n\t"
	"nop\n\t"

	"mov.l %[f_ph_inc_1], %[tmp0]\n\t"
	"and %[tmp4], %[tmp3]\n\t"

	"mov.l %[f_ph_1], %[tmp4]\n\t"
	"add %[tmp3], %[tmp2]\n\t"

	"mov.l %[tmp2], %[f_ph_0]\n\t"
	"mov %[tmp4], %[tmp2]\n\t"
	//
	//
	//
	"add %[tmp4], %[tmp0]\n\t"
	"xor %[tmp0], %[tmp2]\n\t"

	"mov.l %[tmp0], %[f_ph_1]\n\t"
	"exts.w %[tmp2], %[tmp2]\n\t"

	"cmp/pz %[tmp2]\n\t"
	"bt/s 1f\n\t"

	"swap.w %[tmp4], %[tmp0]\n\t"	// Delay slot
	"and #0x1F, %[tmp0]\n\t"

	"mov.b @(%[tmp0], %[modram]), %[tmp0]\n\t"
	"add #0x40, %[tmp1]\n\t"

	"cmp/pz %[tmp0]\n\t"
	"add %[tmp1], %[tmp0]\n\t"

	"bt/s 3f\n\t"
	"and #0x7F, %[tmp0]\n\t"		// Delay slot

	"mov #0x40, %[tmp0]\n\t"
	"3: mov.l %[tmp0], %[f_mod_counter]\n\t"
	//
	//
	"1:\n\t"
/*
*/

	: [tmp0] "=&z"(tmp0), [tmp1] "=&r"(tmp1), [tmp2] "=&r"(tmp2), [tmp3] "=&r"(tmp3), [tmp4] "=&r"(tmp4)
	: [f] "r"(f),
	  [f_wt_control] "m"(f->wt_control),
	  [f_mod_counter] "m"(f->mod_counter),
	  [f_env_level_1] "m"(f->env_level[1]),
	  [f_ph_inc_0] "m"(f->ph_inc),
	  [f_ph_0] "m"(f->ph[0]),
	  [f_mod_control] "m"(f->mod_control),
	  [f_ph_1] "m"(f->ph[1]),
	  [f_ph_inc_1] "m"(f->eff_mod_ph_inc),
	  [modram] "r"(f->modram)
	: "cc");
  }
#else
  if(!(f->wt_control & 0x80))
  {
   {
    uint32 tphi = (int32)(f->mod_counter - 0x40) * (int32)f->env_level[1];
    int r = (bool)(tphi & 0xF) << 1;

    tphi >>= 4;

    if(!(tphi & 0x80))
     tphi += r;

    tphi = (tphi + 0x40) & 0xFF;
    tphi = (f->ph_inc * tphi) & 0xFFFFF;

    f->ph[0] += tphi;
   }

   //if(!(f->mod_control & 0x80))
   {
    const uint32 prev_mph = f->ph[1];

    f->ph[1] += f->eff_mod_ph_inc; //f->ph_inc[1];
    if((prev_mph ^ f->ph[1]) & 0x8000)
    {
     int8 m = f->modram[(prev_mph >> 16) & 0x1F];

     f->mod_counter = (f->mod_counter + m) & 0x7F;
     if(m < 0)
      f->mod_counter = 0x40;
    }
   }
  }
#endif
#endif
 }
}

static INLINE void fds_run(fds_t* f, int32 timestamp)
{
 while(timestamp >= f->env_pre_divider)
 {
  fds_run_(f, f->env_pre_divider);
  //
  //
  f->env_pre_divider += f->env_pre_period;
  //
  if(!(f->wt_control & 0x40))
  {
   for(unsigned i = 0; i < 2; i++)
   {
    f->env_divider[i]--;

    if(!f->env_divider[i])
    {
     f->env_divider[i] = f->env_period[i];
     //
     if(!(f->env_control[i] & 0x80))
     {
      if(f->env_control[i] & 0x40)
      {
       if(f->env_level[i] < 0x20)
        f->env_level[i]++;
      }
      else
      {
       if(f->env_level[i] > 0x00)
        f->env_level[i]--;
      }
      f->eff_vol = ((f->env_level[0] > 0x20) ? 0x20 : f->env_level[0]);
      if(!f->eff_vol)
       f->vol_latch = f->eff_vol;
     }
    }
   }
  }
 }

 fds_run_(f, timestamp);
}

static INLINE void fds_write(fds_t* f, uint32 timestamp, uint8 addr, uint8 data)
{
 //printf("%04x %02x\n", addr, data);

 if(addr & 0x40)
 {
  // No need to check f->ww, as blocked writes are already filtered out on
  // the master CPU side.
  f->wtram[addr & 0x3F] = f->wtx[data & 0x3F];
 }
 else switch(addr & 0x1F)
 {
  case 0x0:
	//printf("%02x\n", data);

	f->env_control[0] = data;

	if(data & 0x80)
	{
	 f->env_level[0] = data & 0x3F;
	 f->eff_vol = ((f->env_level[0] > 0x20) ? 0x20 : f->env_level[0]);

         if(!f->eff_vol)
          f->vol_latch = f->eff_vol;
	}
	//
	f->env_period[0] = (data & 0x3F) + 1;
	f->env_divider[0] = f->env_period[0];
	break;

  case 0x2:
	f->ph_inc &= 0xFF00;
	f->ph_inc |= data;
	break;

  case 0x3:
	//printf("wtc: %02x\n", data);

	f->ph_inc &= 0x00FF;
	f->ph_inc |= (data & 0xF) << 8;

	f->wt_control = data & 0xC0;
	if(f->wt_control & 0x80)
	 f->ph[0] = 0;

	if(f->wt_control & 0x40)
	{
	 f->env_divider[0] = f->env_period[0];
	 f->env_divider[1] = f->env_period[1];
	}
	break;
  //
  //
  case 0x4:
	f->env_control[1] = data;

	if(data & 0x80)
	 f->env_level[1] = data & 0x3F;
	//
	f->env_period[1] = (data & 0x3F) + 1;
	f->env_divider[1] = f->env_period[1];
	break;

  case 0x5:
	f->mod_counter = (data & 0x7F) ^ 0x40;
	break;

  case 0x6:
	f->mod_ph_inc &= 0xFF00;
	f->mod_ph_inc |= data;

	f->eff_mod_ph_inc = ((f->mod_control & 0x80) ? 0 : (f->mod_ph_inc << 3));
	break;

  case 0x7:
	f->mod_ph_inc &= 0x00FF;
	f->mod_ph_inc |= (data & 0xF) << 8;

	f->mod_control = data & 0xC0;
	if(f->mod_control & 0x80)
	 f->ph[1] &= ~0xFFFF;
	//
	//
	f->eff_mod_ph_inc = ((f->mod_control & 0x80) ? 0 : (f->mod_ph_inc << 3));
	break;

  case 0x8:
	if(f->mod_control & 0x80)
	{
	 static const uint8 mod_table[8] = { 0, 1, 2, 4, 0x80, 0x80 - 4, 0x80 - 2, 0x80 - 1 };

	 f->modram[(f->ph[1] >> 16) & 0x1F] = mod_table[data & 0x7];
	 f->ph[1] += (1U << 16);
	}
	break;
  //
  //
  case 0x9:
	f->master_volume = 60 / (2 + (data & 0x3));
	f->ww = data & 0x80;
	break;

  case 0xA:
	f->env_pre_period = (data + 1) << 3;

	if(!data)
	 f->env_pre_divider = (uint32)-1;
	else
	 f->env_pre_divider = timestamp + f->env_pre_period;
	break;

 }
}

void fds_slave_entry(uint32 timestamp_scale, volatile uint16* exchip_buffer, uint32 ref_volume)
{
 fds_t fds = { 0 };

 EXCHIP_RELOC(fds, &fds)
 //
 //
 TCR = 0x0;
 TOCR = 0;

 sh2_set_sr_s(true);

 {
  int32 output_volume = ((uint64)ref_volume * 65536) / ((63 * 32 * (60 / 2)));

  //printf("%d\n", output_volume);

  for(int i = 0x3F; i >= 0x00; i--)
  {
   int32 s = 1 << 24;
   int64 v = (i & 0x20) * s;

   for(int j = 0x10; j; j >>= 1)
   {
    s = ((int64)s * 17196646) >> 24;
    v += (i & j) * s;
   }

   if(i == 0x3F)
    output_volume = ((uint64)ref_volume * 65536 * (1 << 24)) / ((v * 0x20 * 30));

   fds.wtx[i] = ((int64)v * output_volume) >> 24;
   //printf("0x%02x: %u\n", i, fds.wtx[i]);
  }

  assert(((uint64)fds.wtx[0x3F] * 0x20 * 30) < (1ULL << 32));
 }

 fds.resamp_scale = timestamp_scale << (32 - 17);
 fds.scsp_ptr = exchip_buffer;
 fds.sample_offs = (-8 & 0x7FF) << 1;	// Approximate, 44100 * (40 / 2 + 0.5) / (1789772.727272/16)

 fds.source_pos_inc = (256LL * (1U << (21 - 4))) / timestamp_scale;
 fds.source_pos_f_inc = ((256LL * (1U << (21 - 4))) % timestamp_scale) << (32 - 17);
 //printf("%u %u\n", fds.source_pos_inc, fds.source_pos_f_inc);

 fds.buf_offs = 40 << 1;

 memcpy(exchip_resamp, exchip_tables.resamp_div16, sizeof(exchip_tables.resamp_div16));

 fds.lpf_coeff = 456378560;
 //
 //
 fds.env_pre_period = (0xE8 + 1) << 3;
 fds.env_pre_divider = fds.env_pre_period;
 //
 //
 nsfcore_slave_ready = true;
 //
 //
 //
 EXCHIP_BENCH_PROLOGUE

 for(;;)
 {
  while(!(FTCSR & 0x80))
  {
   sh2_wait_approx(100);
   //
  }
  FTCSR = 0x00;

  EXCHIP_BENCH_BEGIN
  //
  //
  const uint32 rb_wr = *(volatile uint16*)(0x20000000 | (uint32)&exchip_rb_wr) >> 2;

  while(fds.rb_rd != rb_wr)
  {
   const uint32 rbd = exchip_rb[fds.rb_rd];
   uint32 timestamp = rbd >> 16;
   uint8 addr = rbd >> 8;
   uint8 data = rbd >> 0;

   fds.rb_rd = (fds.rb_rd + 1) & 0x3FFF;
   //
   //
   fds_run(&fds, timestamp);
   //
   //
   if(LIKELY(addr < 0xC0))
   {
    //printf("%02x %02x\n", addr, data);
    fds_write(&fds, timestamp, addr, data);
   }
#if EXCHIP_CONSISTENCY_CHECK
   else if(addr == 0xC0 || addr == 0xC1)
   {
    const unsigned w = (addr & 1);
    const unsigned el = fds.env_level[w];

    if(data != el)
    {
     printf("E%u: M=%02x S=%02x\n", w, data, el);
     abort();
    }
   }
#endif
   else if(addr == SLAVECMD_FORCE_UPDATE)
   {

   }
   else if(addr == SLAVECMD_FRAME)
   {
    EXCHIP_BENCH_END_FRAME
    //
    //
    fds.prev_timestamp -= timestamp;

    if(fds.env_pre_divider != (uint32)-1)    
     fds.env_pre_divider -= timestamp;
   }
   else if(addr == SLAVECMD_STOP)
    return;
  }

  EXCHIP_BENCH_END
 }
}


//
//
//
extern fds_master_t fds_master;
static uint8 master_wtram[0x40];


void fds_master_update_counters(uint32 timestamp)
{
 fds_master_t* f = &fds_master;

 while(timestamp >= f->env_pre_divider)
 {
  f->env_pre_divider += f->env_pre_period;
  //
  if(!(f->wt_control & 0x40))
  {
   f->env_divider[0]--;
   if(!f->env_divider[0])
   {
    f->env_divider[0] = f->env_period[0];
    //
    if(!(f->env_control[0] & 0x80))
    {
     if(f->env_control[0] & 0x40)
      f->env_level[0] += (f->env_level[0] < 0x20);
     else
      f->env_level[0] -= (f->env_level[0] > 0x00);
    }
   }
   //
   //
   f->env_divider[1]--;
   if(!f->env_divider[1])
   {
    f->env_divider[1] = f->env_period[1];
    //
    if(!(f->env_control[1] & 0x80))
    {
     if(f->env_control[1] & 0x40)
      f->env_level[1] += (f->env_level[1] < 0x20);
     else
      f->env_level[1] -= (f->env_level[1] > 0x00);
    }
   }
  }
 }

#if EXCHIP_CONSISTENCY_CHECK
 {
  *(volatile uint32*)((volatile uint8*)exchip_rb + exchip_rb_wr) = (timestamp << 16) | (0xC0 << 8) | f->env_level[0];
  exchip_rb_wr += 4;

  *(volatile uint32*)((volatile uint8*)exchip_rb + exchip_rb_wr) = (timestamp << 16) | (0xC1 << 8) | f->env_level[1];
  exchip_rb_wr += 4;
 }
#endif
}

void fds_master_power(void)
{
 fds_master_t* f = &fds_master;

 f->wtram_ptr = (uintptr_t)master_wtram - 0x4040;
 //
 //
 f->env_pre_period = (0xE8 + 1) << 3;
 f->env_pre_divider = fds_master.env_pre_period;
 f->ww_mvol = 0x00;

 f->wt_control = 0x00;

 for(unsigned i = 0; i < 2; i++)
 {
  f->env_control[i] = 0x00;
  f->env_period[i] = 0x00 + 1;
  f->env_divider[i] = f->env_period[i];

  f->env_level[i] = 0x00;
 }
}

void fds_master_frame(uint32 timestamp)
{
 if(timestamp >= fds_master.env_pre_divider)
  fds_master_update_counters(timestamp);

 if(fds_master.env_pre_divider != (uint32)-1)
  fds_master.env_pre_divider -= timestamp;
}

