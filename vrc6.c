/*
 * vrc6.c
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

//
// Simple, ugly mitigations are implemented here to handle the
// excessive distortion that can be caused by low/high period write pairs
// straddling 44.1KHz sample boundaries when a channel is audible.
// It would be better to run at 4x or higher and downsample to 44.1KHz, but
// there's barely enough CPU(and bus) time for even 2x.
//

#include "exchip.h"
#include "vrc6.h"

#include <stdio.h>

#include "exchip_lut.h"

typedef struct
{
 int8 enabled[3];
 int8 saw_volume;
 int8 pulse_duty[2];

 uint32 ph[3];
 uint32 ph_inc[3];
 const int16* wf[3];

 uint32 timestamp_scale;
 uint32 sample_offs;
 uint32 prev_sample_time;
 uint32 rb_rd;
 volatile uint16* scsp_ptr;
 int32 output_volume;

 int16 period[3];
 int8 pulse_volume[2];

 const int16* period_wf[2];

 bool delay_period_change[3];
 uint32 prev_fchange_stfxp[3];
 //
 //
 //
 int64 pulse_dividend;
 int64 saw_dividend;

 int16 saw_weights[6] __attribute__((aligned(4)));	// Derived from saw_volume
 int16 target_saw_weights[6] __attribute__((aligned(4)));;

 uint8 saw_accum[6] __attribute__((aligned(4)));
 uint8 target_saw_accum[6] __attribute__((aligned(4)));
 //
 const int16* pulse_constant;
 const int16* pulse_remap[48];
 const int16* saw_remap[48];

 bool saw_volume_pending;
} vrc6_t;

static unsigned period_to_fbin(const unsigned freq)
{
 unsigned fbi;

 if(freq & 0xFC0)
 {
  if(freq & 0xE00)
  {
   if(freq & 0x800)
    fbi = (0x10 + (0x7 << 2)) | ((freq >> 9) & 0x3);
   else if(freq & 0x400)
    fbi = (0x10 + (0x6 << 2)) | ((freq >> 8) & 0x3);
   else //if(freq & 0x200)
    fbi = (0x10 + (0x5 << 2)) | ((freq >> 7) & 0x3);
  }
  else
  {
   if(freq & 0x100)
    fbi = (0x10 + (0x4 << 2)) | ((freq >> 6) & 0x3);
   else if(freq & 0x080)
    fbi = (0x10 + (0x3 << 2)) | ((freq >> 5) & 0x3);
   else //if(freq & 0x040)
    fbi = (0x10 + (0x2 << 2)) | ((freq >> 4) & 0x3);
  }
 }
 else
 {
  if(freq & 0x020)
   fbi = (0x10 + (0x1 << 2)) | ((freq >> 3) & 0x3);
  else if(freq & 0x010)
   fbi = (0x10 + (0x0 << 2)) | ((freq >> 2) & 0x3);
  else
   fbi = (freq & 0xF);
 }

 return fbi;
}

static INLINE void pulse_duty_changed(vrc6_t* vrc6, unsigned w)
{
 const int16* wf = vrc6->period_wf[w] + ((vrc6->pulse_duty[w] & 0x7) * 28 * (1024 + 1));

 if(vrc6->pulse_duty[w] & 0x8)
  wf = vrc6->pulse_constant;

 vrc6->wf[w] = wf;
}

static INLINE void divu_s64_s32(int64 dividend, int32 divisor)
{
 DVCR = 0;
 DVSR = divisor;
 DVDNTH = (uint64)dividend >> 32;
 DVDNTL = (uint32)dividend;
}

static INLINE void pulse_period_changed(vrc6_t* vrc6, unsigned w)
{
 const unsigned period = vrc6->period[w];

 divu_s64_s32(vrc6->pulse_dividend, (period + 1) << 1);

 vrc6->period_wf[w] = vrc6->pulse_remap[period_to_fbin(period)];
 vrc6->wf[w] = vrc6->period_wf[w] + ((vrc6->pulse_duty[w] & 0x7) * 28 * (1024 + 1));
 //
 //
 vrc6->ph_inc[w] = DVDNTL;

 if(DVCR & 0x1)
  vrc6->ph_inc[w] = 0;
}

static INLINE void saw_period_changed(vrc6_t* vrc6)
{
 const unsigned period = vrc6->period[2];

 divu_s64_s32(vrc6->saw_dividend, (period + 1) << 1);

 vrc6->wf[2] = vrc6->saw_remap[period_to_fbin(period)];

 vrc6->ph_inc[2] = DVDNTL;

 if(DVCR & 0x1)
  vrc6->ph_inc[2] = 0;
}

static INLINE int32 pulse_sample(vrc6_t* vrc6, unsigned w)
{
 const int16* wf = vrc6->wf[w];
 const uint32 pulse_ph_i = vrc6->ph[w] >> 22;
 const uint8 pulse_ph_f = (uint8)(vrc6->ph[w] >> (22 - 8));
 const int16 a = wf[pulse_ph_i + 0];
 const int16 b = wf[pulse_ph_i + 1];

 vrc6->ph[w] += vrc6->ph_inc[w];
 //
 //
 if(!vrc6->enabled[w])
  return 0;

 return (a + (int16)((uint32)((b - a) * pulse_ph_f) >> 8)) * vrc6->pulse_volume[w];
}

enum { saw_part_len = 585 };
enum { saw_wf_len = saw_part_len * 7 };

static __attribute__((noinline)) int32 get_saw_sample(vrc6_t* vrc6)
{
 const uintptr_t wf_lbound = (uintptr_t)vrc6->wf[2];
 const int wf_part_adj = -((saw_part_len + 1) << 1);
 const int wf_adj = saw_wf_len << 1;
 uintptr_t wf = wf_lbound + (((vrc6->ph[2] >> (32 - 12))) << 1);
 int32 ret = 0;
 int16* saw_weights = vrc6->saw_weights;

 asm volatile(
	"clrmac\n\t"
	".align 2\n\t"

	// [0]
	"mac.w @%[wf]+, @%[saw_weights]+\n\t"
	"add %[wf_part_adj], %[wf]\n\t"

	"cmp/gt %[wf], %[wf_lbound]\n\t"
	"bf/s 1f\n\t"

	"nop\n\t"
	"add %[wf_adj], %[wf]\n\t"
	//
	// [1]
	"1:\n\t"
	"mac.w @%[wf]+, @%[saw_weights]+\n\t"
	"add %[wf_part_adj], %[wf]\n\t"

	"cmp/gt %[wf], %[wf_lbound]\n\t"
	"bf/s 1f\n\t"

	"nop\n\t"
	"add %[wf_adj], %[wf]\n\t"
	//
	// [2]
	"1:\n\t"
	"mac.w @%[wf]+, @%[saw_weights]+\n\t"
	"add %[wf_part_adj], %[wf]\n\t"

	"cmp/gt %[wf], %[wf_lbound]\n\t"
	"bf/s 1f\n\t"

	"nop\n\t"
	"add %[wf_adj], %[wf]\n\t"
	//
	// [3]
	"1:\n\t"
	"mac.w @%[wf]+, @%[saw_weights]+\n\t"
	"add %[wf_part_adj], %[wf]\n\t"

	"cmp/gt %[wf], %[wf_lbound]\n\t"
	"bf/s 1f\n\t"

	"nop\n\t"
	"add %[wf_adj], %[wf]\n\t"
	//
	// [4]
	"1:\n\t"
	"mac.w @%[wf]+, @%[saw_weights]+\n\t"
	"add %[wf_part_adj], %[wf]\n\t"

	"cmp/gt %[wf], %[wf_lbound]\n\t"
	"bf/s 1f\n\t"

	"nop\n\t"
	"add %[wf_adj], %[wf]\n\t"
	//
	// [5]
	"1:\n\t"
	"mac.w @%[wf]+, @%[saw_weights]+\n\t"
	"nop\n\t"

	"sts macl, %[ret]\n\t"

	: [ret] "=r"(ret), [wf] "+&r"(wf), [saw_weights] "+&r"(saw_weights)
	: [wf_adj] "r"(wf_adj), [wf_part_adj] "r"(wf_part_adj), [wf_lbound] "r"(wf_lbound)
	: "cc", "macl", "mach");
 //
 //

 return ret;
}

static INLINE void update_saw_target(vrc6_t* vrc6)
{
 __builtin_memcpy(vrc6->saw_weights, vrc6->target_saw_weights, sizeof(vrc6->target_saw_weights));
 __builtin_memcpy(vrc6->saw_accum, vrc6->target_saw_accum, sizeof(vrc6->target_saw_accum));
}

static INLINE int32 saw_sample(vrc6_t* vrc6)
{
 int32 ret = get_saw_sample(vrc6);

 {
  const uint32 prev_saw_ph = vrc6->ph[2];

  vrc6->ph[2] += vrc6->ph_inc[2];
  if(vrc6->ph[2] >= (saw_wf_len << (32 - 12)) || vrc6->ph[2] < prev_saw_ph)
  {
   vrc6->ph[2] -= saw_wf_len << (32 - 12);

   update_saw_target(vrc6);
  }
 }
 //
 //
 if(!vrc6->enabled[2])
  ret = 0;

 return ret;
}

static __attribute__((noinline)) void vrc6_run(vrc6_t* vrc6, const uint32 sample_time)
{
 if(UNLIKELY(vrc6->saw_volume_pending))
 {
  //
  // TODO: Handle saw_*_target->saw_* copying with ultrasonic playback
  // frequencies?
  //
  uint8 tmp = 0;
  const unsigned c = (vrc6->ph[2] >> (32 - 12)) / saw_part_len;

  for(unsigned i = 0; i < 6; i++)
  {
   tmp += vrc6->saw_volume;

   vrc6->target_saw_accum[i] = tmp;
   vrc6->target_saw_weights[i] = tmp >> 3;
  }

  tmp = c ? vrc6->saw_accum[c - 1] : 0;
  for(unsigned i = c; i < 6; i++)
  {
   tmp += vrc6->saw_volume;

   vrc6->saw_accum[i] = tmp;
   vrc6->saw_weights[i] = tmp >> 3;
  }
  vrc6->saw_volume_pending = false;
 }

 //printf("%03x %03x\n", vrc6->prev_sample_time, sample_time);
 while(vrc6->prev_sample_time != sample_time)
 {
  //
  //
  int32 accum = 0;

  accum  = pulse_sample(vrc6, 0);
  accum += pulse_sample(vrc6, 1);
  accum += saw_sample(vrc6);

  if(vrc6->delay_period_change[0])
  {
   pulse_period_changed(vrc6, 0);
   vrc6->delay_period_change[0] = false;
   //printf("d0\n");
  }

  if(vrc6->delay_period_change[1])
  {
   pulse_period_changed(vrc6, 1);
   vrc6->delay_period_change[1] = false;
   //printf("d1\n");
  }

  if(vrc6->delay_period_change[2])
  {
   saw_period_changed(vrc6);
   vrc6->delay_period_change[2] = false;
   //printf("d2\n");
  }
  //
  //
/*
  asm volatile(
	"mov.w %0, @%1\n\t"
	:
	: "r"(((uint32)(accum * 1198) >> 16)), "r"(&vrc6->scsp_ptr[vrc6->prev_sample_time]));
*/
  vrc6->scsp_ptr[vrc6->prev_sample_time] = ((uint32)(accum * vrc6->output_volume) >> 16);
  vrc6->prev_sample_time = (vrc6->prev_sample_time + 1) & 0x7FF;
 }
}

void vrc6_slave_entry(uint32 timestamp_scale, volatile uint16* exchip_buffer, uint32 ref_volume)
{
 vrc6_t vrc6 = { { 0 } };
 //
 //
 vrc6.timestamp_scale = timestamp_scale;
 vrc6.scsp_ptr = exchip_buffer;
 vrc6.output_volume = -(((uint64)ref_volume * 65536) / (16384 * 15));

 vrc6.pulse_dividend = (((1LL << (32 + 21 + 1 + 1)) / timestamp_scale / 16) + 1) >> 1;
 vrc6.saw_dividend = ((( (int64)(585 * 7) << ((32 - 12) + 21 + 1 + 1)) / timestamp_scale / 14) + 1) >> 1;

 //printf("%016llx %016llx\n", vrc6.pulse_dividend, vrc6.saw_dividend);
 //printf("size: %zu\n", sizeof(vrc6));

 for(unsigned i = 0; i < 48; i++)
 {
  vrc6.pulse_remap[i] = (const int16*)exchip_tables.samples + (pulse_remap_lut_init[i] * (1024 + 1));
  vrc6.saw_remap[i] = (const int16*)exchip_tables.samples + ((28 * 8 + 1) * (1024 + 1)) + (saw_remap_lut_init[i] << 12);
 }

 vrc6.pulse_constant = (const int16*)exchip_tables.samples + ((28 * 8) * (1024 + 1));
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

  while(vrc6.rb_rd != rb_wr)
  {
   const uint32 rbd = exchip_rb[vrc6.rb_rd];
   uint32 timestamp = rbd >> 16;
   uint8 addr = rbd >> 8;
   uint8 data = rbd >> 0;

   vrc6.rb_rd = (vrc6.rb_rd + 1) & 0x3FFF;
   //
   //
   const uint32 sample_time_fxp = ((timestamp * vrc6.timestamp_scale) + vrc6.sample_offs);
   const uint32 sample_time = sample_time_fxp >> 21;

   vrc6_run(&vrc6, sample_time);
   //
   //

   switch(addr)
   {
    case 0x4:
    case 0x8:
	{
	 const size_t w = (bool)(addr & 0x8);

	 vrc6.pulse_volume[w] = data & 0x0F;
	 vrc6.pulse_duty[w] = (data >> 4) & 0xF;
	 //
	 pulse_duty_changed(&vrc6, w);
	}
	break;

    case 0x5:
    case 0x9:
	{
	 const size_t w = (bool)(addr & 0x8);
	 const unsigned old_period = vrc6.period[w];
	 const unsigned new_period = (vrc6.period[w] & 0xFF00) | data;

	 vrc6.period[w] = new_period;

	 if(new_period != old_period)
	 {
	  if((sample_time_fxp & (1U << 20)) && (sample_time_fxp - vrc6.prev_fchange_stfxp[w]) > (1U << 20))
	   vrc6.delay_period_change[w] = true;
	  else if(!vrc6.delay_period_change[w])
	   pulse_period_changed(&vrc6, w);
	 }
	}
	break;

    case 0x6:
    case 0xA:
	{
	 const size_t w = (bool)(addr & 0x8);
	 const unsigned old_period = vrc6.period[w];
	 const unsigned new_period = (vrc6.period[w] & 0x00FF) | ((data & 0xF) << 8);

	 vrc6.period[w] = new_period;

	 if((vrc6.enabled[w] ^ data) & data & 0x80)
	 {
	  //printf("e%u\n", w);
          vrc6.ph[w] &= 0x0FFFFFFF;
	  vrc6.enabled[w] = data & 0x80;
	  //
	  pulse_period_changed(&vrc6, w);

	  vrc6.delay_period_change[w] = false;
	  vrc6.prev_fchange_stfxp[w] = sample_time_fxp;
	 }
	 else
	 {
	  vrc6.enabled[w] = data & 0x80;
	  //
	  if(new_period != old_period)
	  {
	   if((sample_time_fxp & (1U << 20)) && (sample_time_fxp - vrc6.prev_fchange_stfxp[w]) > (1U << 20))
	    vrc6.delay_period_change[w] = true;
	   else if(!vrc6.delay_period_change[w])
	    pulse_period_changed(&vrc6, w);
	   //
	   vrc6.prev_fchange_stfxp[w] = sample_time_fxp;
	  }
	 }
	}
	break;
    //
    //
    case 0x7: // TODO
	break;
    //
    //
    case 0xC:
	vrc6.saw_volume = data & 0x3F;
	vrc6.saw_volume_pending = true;
	break;

    case 0xD:
	{
	 const unsigned old_period = vrc6.period[2];
	 const unsigned new_period = (vrc6.period[2] & 0xFF00) | data;

	 vrc6.period[2] = new_period;

	 if(new_period != old_period)
	 {
	  //printf("1: 0x%08x\n", (sample_time_fxp - vrc6.prev_fchange_stfxp[2]));

	  if((sample_time_fxp & (1U << 20)) && (sample_time_fxp - vrc6.prev_fchange_stfxp[2]) > (1U << 20))
	   vrc6.delay_period_change[2] = true;
	  else if(!vrc6.delay_period_change[2])
	   saw_period_changed(&vrc6);

	  vrc6.prev_fchange_stfxp[2] = sample_time_fxp;
	 }
	}
	break;

    case 0xE:
	{
	 const unsigned old_period = vrc6.period[2];
	 const unsigned new_period = (vrc6.period[2] & 0x00FF) | ((data & 0xF) << 8);

	 vrc6.period[2] = new_period;

	 if((vrc6.enabled[2] ^ data) & data & 0x80)
	 {
	  //printf("e2\n");

	  vrc6.ph[2] -= (((vrc6.ph[2] >> (32 - 12)) / saw_part_len) * saw_part_len) << (32 - 12);
	  vrc6.enabled[2] = data & 0x80;

 	  update_saw_target(&vrc6);
	  saw_period_changed(&vrc6);

	  vrc6.delay_period_change[2] = false;
	  vrc6.prev_fchange_stfxp[2] = sample_time_fxp;
	 }
	 else
	 {
	  vrc6.enabled[2] = data & 0x80;
	  //
	  if(new_period != old_period)
	  {
	   //printf("2: 0x%08x\n", (sample_time_fxp - vrc6.prev_fchange_stfxp[2]));
	   if((sample_time_fxp & (1U << 20)) && (sample_time_fxp - vrc6.prev_fchange_stfxp[2]) > (1U << 20))
	    vrc6.delay_period_change[2] = true;
	   else if(!vrc6.delay_period_change[2])
	    saw_period_changed(&vrc6);
	   //
	   vrc6.prev_fchange_stfxp[2] = sample_time_fxp;
	  }
	 }
	}

	break;
    //
    //
    case 0xFD: // Force-sync
	break;

    case 0xFE: // Reset timestamp
	EXCHIP_BENCH_END_FRAME

	{
	 vrc6.sample_offs += timestamp * vrc6.timestamp_scale;

	 for(unsigned i = 0; i < 3; i++)
	 {
          if((sample_time_fxp - vrc6.prev_fchange_stfxp[i]) >= (1U << 21))
           vrc6.prev_fchange_stfxp[i] = sample_time_fxp - (1U << 21);
	 }
	}
	break;

    case 0xFF: // Exit
	return;
   }
  }

  EXCHIP_BENCH_END
 }
}

/*
uint32 vrc6_get_max_ref_volume(void)
{
 // (32768 * 65536) / (((16384 * 15) / 65536.0) * 16384 * 1.5 * (15*2+31))
 return 5371;
}
*/
