/*
 * mmc5.c
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
#include "mmc5.h"

#include <stdio.h>

#include "exchip_lut.h"

typedef struct
{
 uint32 ph[2];
 uint32 ph_inc[2];
 const int16* wf[2];

 uint32 timestamp_scale;
 uint32 sample_offs;
 uint32 prev_sample_time;
 uint32 rb_rd;
 volatile uint16* scsp_ptr;
 int32 output_volume;

 int32 volume[2];

 int32 pcm;
 uint32 length_counter[2];
 //
 //
 uint32 lc_enable;
 int32 period[2];
 int32 duty[2];
 const int16* period_wf[2];

 uint32 frame_divider;
 bool frame_divider_alt;

 struct envelope_t
 {
  int8 period;
  int8 divider;
  int8 level;

  int8 mode;

  int8 reload;
 } env[2];
 size_t duty_offs[4];
 //
 //
 //
 int64 pulse_dividend;
 const int16* pulse_remap[48];
} mmc5_t;

static unsigned period_to_fbin(const unsigned freq)
{
 unsigned fbi;

 if(freq & 0x7C0)
 {
  if(freq & 0x600)
  {
   if(freq & 0x400)
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

static INLINE void pulse_duty_changed(mmc5_t* mmc5, unsigned w)
{
 const int16* wf = mmc5->period_wf[w] + mmc5->duty_offs[mmc5->duty[w]];

 mmc5->wf[w] = wf;
}

static INLINE void divu_s64_s32(int64 dividend, int32 divisor)
{
 DVCR = 0;
 DVSR = divisor;
 DVDNTH = (uint64)dividend >> 32;
 DVDNTL = (uint32)dividend;
}

static INLINE void pulse_period_changed(mmc5_t* mmc5, unsigned w)
{
 const unsigned period = mmc5->period[w];

 divu_s64_s32(mmc5->pulse_dividend, (period + 1) << 1);

 mmc5->period_wf[w] = mmc5->pulse_remap[period_to_fbin(period)];
 mmc5->wf[w] = mmc5->period_wf[w] + mmc5->duty_offs[mmc5->duty[w]]; //((mmc5->pulse_duty[w] & 0x7) * 28 * (1024 + 1));
 //
 //
 mmc5->ph_inc[w] = -DVDNTL;

 if(DVCR & 0x1)
  mmc5->ph_inc[w] = 0;
}

static INLINE int32 pulse_sample(mmc5_t* mmc5, unsigned w)
{
 const int16* wf = mmc5->wf[w];
 const uint32 pulse_ph_i = mmc5->ph[w] >> 22;
 const uint8 pulse_ph_f = (uint8)(mmc5->ph[w] >> (22 - 8));
 const int16 a = wf[pulse_ph_i + 0];
 const int16 b = wf[pulse_ph_i + 1];

 mmc5->ph[w] += mmc5->ph_inc[w];
 //
 //
 int32 s = (a + (int16)((uint32)((b - a) * pulse_ph_f) >> 8));

 if(mmc5->duty[w] == 0x3)
  s = 16384 - s;

 if(!mmc5->length_counter[w])
  s = 0;

 return s * mmc5->volume[w];
}

static __attribute__((noinline)) void mmc5_run(mmc5_t* mmc5, const uint32 sample_time)
{
 while(mmc5->prev_sample_time != sample_time)
 {
  int32 accum = 0;

  accum  = pulse_sample(mmc5, 0);
  accum += pulse_sample(mmc5, 1);
  accum += mmc5->pcm * 2896;

  mmc5->scsp_ptr[mmc5->prev_sample_time] = ((uint32)(accum * mmc5->output_volume) >> 16);
  mmc5->prev_sample_time = (mmc5->prev_sample_time + 1) & 0x7FF;
 }
}

static const uint8 length_table[0x20] =
{
 0x0A, 0xFE,
 0x14, 0x02,
 0x28, 0x04,
 0x50, 0x06,
 0xA0, 0x08,
 0x3C, 0x0A,
 0x0E, 0x0C,
 0x1A, 0x0E,
 0x0C, 0x10,
 0x18, 0x12,
 0x30, 0x14,
 0x60, 0x16,
 0xC0, 0x18,
 0x48, 0x1A,
 0x10, 0x1C,
 0x20, 0x1E
};

enum { frame_divider_reload = 7457 };

void mmc5_slave_entry(uint32 timestamp_scale, volatile uint16* exchip_buffer, uint32 ref_volume)
{
 mmc5_t mmc5 = { { 0 } };
 //
 //
 mmc5.timestamp_scale = timestamp_scale;
 mmc5.scsp_ptr = exchip_buffer;
 mmc5.output_volume = -(((uint64)ref_volume * 65536) / (16384 * 15));

 mmc5.pulse_dividend = (((1LL << (32 + 21 + 1 + 1)) / timestamp_scale / 16) + 1) >> 1;

 //printf("%016llx %016llx\n", mmc5.pulse_dividend);
 //printf("size: %zu\n", sizeof(mmc5));

 for(unsigned i = 0; i < 4; i++)
 {
  static const uint8 rem[4] = { 1, 3, 7, 3 };

  mmc5.duty_offs[i] = rem[i] * 28 * (1024 + 1);
 }

 for(unsigned i = 0; i < 48; i++)
  mmc5.pulse_remap[i] = (const int16*)exchip_tables.samples + (pulse_remap_lut_init[i] * (1024 + 1));

 //mmc5.pulse_constant = (const int16*)exchip_tables.samples + ((28 * 8) * (1024 + 1));

 mmc5.frame_divider = frame_divider_reload;
 mmc5.frame_divider_alt = false;
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

  while(mmc5.rb_rd != rb_wr)
  {
   const uint32 rbd = exchip_rb[mmc5.rb_rd];
   uint32 timestamp = rbd >> 16;
   uint8 addr = rbd >> 8;
   uint8 data = rbd >> 0;

   mmc5.rb_rd = (mmc5.rb_rd + 1) & 0x3FFF;
   //
   //

   if(timestamp >= mmc5.frame_divider)
   {
    do
    {
     const uint32 sample_time_fxp = ((mmc5.frame_divider * mmc5.timestamp_scale) + mmc5.sample_offs);
     const uint32 sample_time = sample_time_fxp >> 21;

     mmc5_run(&mmc5, sample_time);
     //
     //
     for(unsigned i = 0; i < 2; i++)
     {
      if(mmc5.env[i].reload)
      {
       mmc5.env[i].level = 0xF;
       mmc5.env[i].divider = mmc5.env[i].period;

       mmc5.env[i].reload = false;
      }
      else
      {
       mmc5.env[i].divider--;
       if(mmc5.env[i].divider < 0)
       {
        mmc5.env[i].divider = mmc5.env[i].period;

        if(mmc5.env[i].level || (mmc5.env[i].mode & 0x2))
         mmc5.env[i].level = (mmc5.env[i].level - 1) & 0xF;
       }
      }

      mmc5.volume[i] = (mmc5.env[i].mode & 0x1) ? mmc5.env[i].period : mmc5.env[i].level;
 
      if(!(mmc5.env[i].mode & 0x2) && mmc5.length_counter[i])
      {
       mmc5.length_counter[i]--;
      }
     }

     mmc5.frame_divider += frame_divider_reload + mmc5.frame_divider_alt;
     mmc5.frame_divider_alt = !mmc5.frame_divider_alt;
    } while(timestamp >= mmc5.frame_divider);
   }
   //
   //
   const uint32 sample_time_fxp = ((timestamp * mmc5.timestamp_scale) + mmc5.sample_offs);
   const uint32 sample_time = sample_time_fxp >> 21;

   mmc5_run(&mmc5, sample_time);
   //
   //
   switch(addr)
   {
    case 0x0:
    case 0x4:
	{
	 const size_t w = (addr >> 2) & 0x1;

	 mmc5.env[w].period = (data & 0xF);
	 mmc5.env[w].mode = (data >> 4) & 0x3;

	 mmc5.duty[w] = (data >> 6) & 0x3;
	 //
	 pulse_duty_changed(&mmc5, w);
	 //
	 mmc5.volume[w] = (mmc5.env[w].mode & 0x1) ? mmc5.env[w].period : mmc5.env[w].level;
	}
	break;

    case 0x2:
    case 0x6:
	{
	 const size_t w = (addr >> 2) & 0x1;
	 //const unsigned old_period = mmc5.period[w];
	 const unsigned new_period = (mmc5.period[w] & 0xFF00) | (data << 0);

	 mmc5.period[w] = new_period;
	 //
	 pulse_period_changed(&mmc5, w);
	}
	break;

    case 0x3:
    case 0x7:
	{
	 const size_t w = (addr >> 2) & 0x1;
	 //const unsigned old_period = mmc5.period[w];
	 const unsigned new_period = (mmc5.period[w] & 0x00FF) | ((data & 0x7) << 8);

	 mmc5.period[w] = new_period;
	 mmc5.ph[w] = ~((~mmc5.ph[w] & 0x1FFFFFFF) | 0xE0000000);
	 //
	 pulse_period_changed(&mmc5, w);

	 if(mmc5.lc_enable & (0x01 << w))
	 {
	  mmc5.length_counter[w] = length_table[(data >> 3) & 0x1F];
	  //apu_status |= 0x01 << w;
	 }

	 mmc5.env[w].reload = true;
	}
	break;

    case 0x11:
	if(data)
	 mmc5.pcm = data;
	break;

    case 0x15:
	if(!(data & 0x01))
	 mmc5.length_counter[0] = 0;

	if(!(data & 0x02))
	 mmc5.length_counter[1] = 0;

	mmc5.lc_enable = (data & 0x03);
	break;
    //
    //
    case 0xFD: // Force-sync
	break;

    case 0xFE: // Reset timestamp
	EXCHIP_BENCH_END_FRAME

	{
	 mmc5.sample_offs += timestamp * mmc5.timestamp_scale;

	 mmc5.frame_divider -= timestamp;
	}
	break;

    case 0xFF: // Exit
	return;
   }
  }

  EXCHIP_BENCH_END
 }
}


//
// Called from the 6502 emulator running on the master SH-2
//
extern mmc5_master_t mmc5_master;
static bool mmc5_master_alt;

void mmc5_master_update_counters(uint32 timestamp)
{
 //printf("%u\n", timestamp);
 while(timestamp >= mmc5_master.frame_divider)
 {
  if(!(mmc5_master.envmode0 & 0x20) && mmc5_master.length_counter0)
  {
   mmc5_master.length_counter0--;
   if(!mmc5_master.length_counter0)
    mmc5_master.status &= ~0x01;
  }

  if(!(mmc5_master.envmode1 & 0x20) && mmc5_master.length_counter1)
  {
   mmc5_master.length_counter1--;
   if(!mmc5_master.length_counter1)
    mmc5_master.status &= ~0x02;
  }
  //
  //
  mmc5_master.frame_divider += 7457 + mmc5_master_alt;
  mmc5_master_alt = !mmc5_master_alt;
 }
}

void mmc5_master_power(void)
{
 mmc5_master.frame_divider = frame_divider_reload;
 mmc5_master_alt = false;
 mmc5_master.envmode0 = 0x00;
 mmc5_master.envmode1 = 0x00;
 mmc5_master.length_counter0 = 0;
 mmc5_master.length_counter1 = 0;
 mmc5_master.status = 0x00;
 mmc5_master.lc_enable = 0x00;

 mmc5_master.mult0 = 0xFF;
 mmc5_master.mult1 = 0xFF;
 mmc5_master.mult_res = mmc5_master.mult0 * mmc5_master.mult1;
}

void mmc5_master_frame(uint32 timestamp)
{
 if(timestamp >= mmc5_master.frame_divider)
  mmc5_master_update_counters(timestamp);

 mmc5_master.frame_divider -= timestamp;
}

