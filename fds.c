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
#include <assert.h>
#include <string.h>

typedef struct
{
 int32 divider;

 uint32 ph[2];
 uint32 ph_inc[2];

 uint32 mod_counter;

 int32 master_volume;

 int32 ww;

 int32 wt_control;
 int32 mod_control;

 int32 lpf;
 int32 lpf_coeff;
 //
 //
 int32 vol_latch;
 int32 eff_vol;

 int32 sample_latch;

 int32 env_level[2];

 int32 env_pre_divider;
 int16 env_pre_period;	// +1) << 3

 int8 env_divider[2];
 int8 env_period[2];	// +1
 int8 env_control[2];
 //
 //
 int32 prev_timestamp;
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

 int8 modram[0x20];
 int32 wtram[0x40];
 int16 buf[64 * 2];
 int32 wtx[0x40];
} fds_t;

static /*__attribute__((section(".fds_reloc")))*/ __attribute__((noinline,noclone)) void fds_run_(fds_t* f, int32 timestamp)
{
 f->divider += (uint16)(timestamp - f->prev_timestamp);
 f->prev_timestamp = timestamp;

 while(f->divider >= 0)
 {
  f->divider -= 16;
  //
  //
  uint32 av;

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
   uint32 tmp0, tmp1 = (uint32)exchip_resamp, tmp2, tmp3;
   uint32 resamp_scale_tmp;
   uint32 s_buf = (uint32)f->buf + 128;

   asm volatile(
	".align 2\n\t"

	"mov.l %[s_buf_offs], r0\n\t"
	"nop\n\t"

	"mov.w %[av], @(r0, %[s_buf])\n\t"
	"add #-128, %[s_buf]\n\t"

	"mov.w %[av], @(r0, %[s_buf])\n\t"
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
	"nop\n\t"
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
	"add %[s_buf], %[tmp0]\n\t"

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
	: [tmp0] "=&r"(tmp0), [tmp1] "+&r"(tmp1) /*[tmp1] "=&r"(tmp1)*/, [tmp2] "=&r"(tmp2), [tmp3] "=&r"(tmp3), [resamp_scale] "=&r"(resamp_scale_tmp), [s_buf] "+&r"(s_buf)
	: [av] "r"(av),
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
  if(!(f->wt_control & 0x80))
  {
   {
    uint32 tphi = (int32)(f->mod_counter - 0x40) * (int32)f->env_level[1];
    int r = (bool)(tphi & 0xF) << 1;

    tphi >>= 4;

    if(!(tphi & 0x80))
     tphi += r;

    tphi = (tphi + 0x40) & 0xFF;
    tphi = (f->ph_inc[0] * tphi) & 0xFFFFF;

    f->ph[0] += tphi;
   }

   if(!(f->mod_control & 0x80))
   {
    const uint32 prev_mph = f->ph[1];

    f->ph[1] += f->ph_inc[1];
    if((prev_mph ^ f->ph[1]) & 0x8000)
    {
     int8 m = f->modram[(prev_mph >> 16) & 0x1F];

     f->mod_counter = (f->mod_counter + m) & 0x7F;
     if(m < 0)
      f->mod_counter = 0x40;
    }
   }
  }
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

static INLINE void fds_write(fds_t* f, uint8 addr, uint8 data)
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
	f->ph_inc[0] &= 0xFF00;
	f->ph_inc[0] |= data;
	break;

  case 0x3:
	//printf("wtc: %02x\n", data);

	f->ph_inc[0] &= 0x00FF;
	f->ph_inc[0] |= (data & 0xF) << 8;

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
	f->ph_inc[1] &= 0xFF00 << 3;
	f->ph_inc[1] |= data << 3;
	break;

  case 0x7:
	f->ph_inc[1] &= 0x00FF << 3;
	f->ph_inc[1] |= ((data & 0xF) << 8) << 3;

	f->mod_control = data & 0xC0;
	if(f->mod_control & 0x80)
	 f->ph[1] &= ~0xFFFF;

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
	break;

 }
}

void fds_slave_entry(uint32 timestamp_scale, volatile uint16* exchip_buffer, uint32 ref_volume)
{
 fds_t fds = { 0 };

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

 fds.lpf_coeff = 413013050;
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
    fds_write(&fds, addr, data);
   }
   else if(addr == 0xFD) // Force-sync
   {

   }
   else if(addr == 0xFE) // Reset timestamp
   {
    EXCHIP_BENCH_END_FRAME
    //
    //
    fds.env_pre_divider -= timestamp;
    fds.prev_timestamp -= timestamp;
   }
   else if(addr == 0xFF)	// Exit
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
     }
    }
   }
  }
 }
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

 fds_master.env_pre_divider -= timestamp;
}

