/*
 * sun5b.c
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
#include "nsfcore.h"
#include "sun5b.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

typedef struct
{
 uint32* volume_ptrs[4];

 int32 status;
 int32 divider[3];
 int32 period[3];

 int32 noise_tone_disable;

 struct
 {
  int32 divider;
  int32 period;
  int32 lfsr;
 } noise;

 struct
 {
  int32 level;
  //
  int32 divider;
  int32 period;
  int32 inc;
  int32 stop_v;
  int32 alt_v;
 } env;

 int32 prev_timestamp;
 uint32 rb_rd;

 uint32 resamp_divider;
 uint32 resamp_scale;
 int32 sample_offs;

 int32 source_pos;
 int32 source_pos_inc;

 uint32 source_pos_f;
 uint32 source_pos_f_inc;

 int32 buf_offs;

 uint32 lut[0x80];
 int16 db_lut[0x40]; // 0x20 mirrored
 //
 //
 volatile uint16* scsp_ptr;
 int16 buf[64 * 2];
 //
 //
 int8 shape_tab[0x10][2];
 uint32 volumes[4 + 1];
} sun5b_t;

static __attribute__((section(".sun5b_reloc"))) __attribute__((noinline,noclone)) void sun5b_run(sun5b_t* s, int32 timestamp)
{
 int count = (uint16)(timestamp - s->prev_timestamp) >> 4;

 s->prev_timestamp += count << 4;
 //
 //
 //
 while(count--)
 {
  uint32 av;
  uint32 status_save;
  {
   uint32 tmp0, tmp1, tmp2;

   asm volatile(
	".align 2\n\t"
	"mov.l %[s_status], %[status_save]\n\t"
	"mov %[lut_offset], r0\n\t"

	"mov.l %[s_noise_tone_disable], %[tmp0]\n\t"
	"shll2 r0\n\t"

	"or %[status_save], %[tmp0]\n\t"
	"add %[s], r0\n\t"

	"mov.l @(r0, %[tmp0]), %[tmp1]\n\t"
	"mov %[s], r0\n\t"

	"extu.b %[tmp1], %[tmp2]\n\t"
	"shlr8 %[tmp1]\n\t"

	"mov.l @(r0, %[tmp2]), %[tmp0]\n\t"
	"extu.b %[tmp1], %[tmp2]\n\t"

	"mov.l @%[tmp0], %[av]\n\t"
	"mov.l @(r0, %[tmp2]), %[tmp0]\n\t"	// STALL

	"mov.l @%[tmp0], %[tmp2]\n\t"
	"shlr8 %[tmp1]\n\t"

	"mov.l @(r0, %[tmp1]), %[tmp0]\n\t"
	"add %[tmp2], %[av]\n\t"

	"mov.l @%[tmp0], %[tmp2]\n\t"
	"add %[tmp2], %[av]\n\t"		// STALL

	: [av] "=&r"(av),
	  [tmp0] "=&r"(tmp0), [tmp1] "=&r"(tmp1), [tmp2] "=&r"(tmp2),
	  [status_save] "=&r"(status_save)
	: [s] "r"(s),
	  [s_status] "m"(s->status), [s_noise_tone_disable] "m"(s->noise_tone_disable),
	  [lut_offset] "I08"(((unsigned)s->lut - (unsigned)s) >> 2)
	: "r0", "cc");
  }

  EXCHIP_SINESWEEP_DO(av)

  {
   uint32 tmp0, tmp1 = (uint32)exchip_resamp, tmp2, tmp3;
   uint32 resamp_scale_tmp;
   uint32 s_buf = (uint32)s->buf + 128;

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
	//
	"mov.l %[s_divider0], %[tmp0]\n\t"
	"mov %[status_save], r0\n\t"

	"mov.l %[s_period0], %[tmp1]\n\t"
	"add #1, %[tmp0]\n\t"

	"mov.l %[s_divider1], %[tmp2]\n\t"
	"cmp/hs %[tmp1], %[tmp0]\n\t"

	"mov.l %[s_period1], %[tmp3]\n\t"
	"add #1, %[tmp2]\n\t"

	"bf/s skip_0_update\n\t"
	"cmp/hs %[tmp3], %[tmp2]\n\t"
	//
	"xor #0x08, r0\n\t"
	"mov #0, %[tmp0]\n\t"
	//
	"skip_0_update:\n\t"

	"mov.l %[tmp0], %[s_divider0]\n\t"
	"bf/s skip_1_update\n\t"

	"mov.l %[s_divider2], %[tmp0]\n\t"
	//
	"xor #0x10, r0\n\t"

	"mov #0, %[tmp2]\n\t"
	//
	"skip_1_update:\n\t"
	"add #1, %[tmp0]\n\t"

	"mov.l %[s_period2], %[tmp1]\n\t"
	"nop\n\t"

	"mov.l %[tmp2], %[s_divider1]\n\t"
	"cmp/hs %[tmp1], %[tmp0]\n\t"

	"bf/s skip_2_update\n\t"
	"nop\n\t"
	//
	"xor #0x20, r0\n\t"
	"mov #0, %[tmp0]\n\t"
	//
	"skip_2_update:\n\t"

	"mov.l %[s_noise_divider], %[tmp2]\n\t"
	"mov #1, %[tmp3]\n\t"

	"mov.l %[tmp0], %[s_divider2]\n\t"
	"add #1, %[tmp2]\n\t"

	"mov.l %[s_noise_period], %[tmp0]\n\t"
	"tst %[tmp3], %[tmp2]\n\t"

	"bf/s skip_noise_update\n\t"
	"cmp/hs %[tmp0], %[tmp2]\n\t"

	"bf/s skip_noise_update\n\t"
	"mov #4, %[tmp3]\n\t"
	//
	//
	"mov.l %[s_noise_lfsr], %[tmp0]\n\t"
	"mov #0, %[tmp2]\n\t"

	"mov %[tmp0], %[tmp1]\n\t"
	"shll2 %[tmp1]\n\t"

	"shll %[tmp1]\n\t"
	"xor %[tmp0], %[tmp1]\n\t"

	"shlr16 %[tmp1]\n\t"
	"and %[tmp3], %[tmp1]\n\t"

	"shll %[tmp0]\n\t"
	"or %[tmp1], %[tmp0]\n\t"

	"mov.l %[tmp0], %[s_noise_lfsr]\n\t"
	"and #0x78, r0\n\t"

	"or %[tmp1], r0\n\t"
	"nop\n\t"
	//
	//
	"skip_noise_update:\n\t"
	"mov.l %[tmp2], %[s_noise_divider]\n\t"
	"mov.l r0, %[s_status]\n\t"		// STALL
	//
	//
	//
	//
	: [tmp0] "=&r"(tmp0), [tmp1] "+&r"(tmp1) /*[tmp1] "=&r"(tmp1)*/, [tmp2] "=&r"(tmp2), [tmp3] "=&r"(tmp3), [resamp_scale] "=&r"(resamp_scale_tmp), [s_buf] "+&r"(s_buf)
	: [av] "r"(av),
	  [status_save] "r"(status_save),
	  [s_buf_offs] "m"(s->buf_offs),
	  [resamp_divider] "m"(s->resamp_divider),
	  [s_resamp_scale] "m"(s->resamp_scale),
	  [source_pos] "m"(s->source_pos),
	  [source_pos_inc] "m"(s->source_pos_inc),
	  [source_pos_f] "m"(s->source_pos_f),
	  [source_pos_f_inc] "m"(s->source_pos_f_inc),
	  [sample_offs] "m"(s->sample_offs),
	  [scsp_ptr] "m"(s->scsp_ptr),
	  //
	  [s_status] "m"(s->status),
	  [s_period0] "m"(s->period[0]),
	  [s_period1] "m"(s->period[1]),
	  [s_period2] "m"(s->period[2]),
	  [s_divider0] "m"(s->divider[0]),
	  [s_divider1] "m"(s->divider[1]),
	  [s_divider2] "m"(s->divider[2]),

	  [s_noise_divider] "m"(s->noise.divider),
	  [s_noise_period] "m"(s->noise.period),
	  [s_noise_lfsr] "m"(s->noise.lfsr)

/*
	  [s_env_divider] "m"(s->env.divider),
	  [s_env_period] "m"(s->env.period),
	  [s_env_level] "m"(s->env.level),
	  [s_env_stop_v] "m"(s->env.stop_v),
	  [s_env_inc] "m"(s->env.inc),
*/
	: "memory", "r0", "cc", "mach", "macl");
  }
  //
  //
  //
  s->env.divider++;
  if(LIKELY(s->env.divider >= s->env.period))
  {
   s->env.divider = 0;
   //
   if(LIKELY(s->env.level != s->env.stop_v))
   {
    s->env.level = (s->env.level + s->env.inc) & 0x3F;

    if(LIKELY(s->env.level == s->env.alt_v))
    {
     s->env.inc = -s->env.inc;
     s->env.alt_v ^= 0x1F;
     s->env.level ^= 0x3F;
    }

    s->volumes[3] = s->db_lut[s->env.level];
   }
  }
 }
}

static INLINE void sun5b_write(sun5b_t* s, uint8 addr, uint8 data)
{
 switch(addr)
 {
  case 0x0:
  case 0x2:
  case 0x4:
	{
	 const size_t w = (addr >> 1);

	 s->period[w] &= 0xFF00;
         s->period[w] |= data;
	}
	break;

  case 0x1:
  case 0x3:
  case 0x5:
	{
	 const size_t w = (addr >> 1);

	 s->period[w] &= 0x00FF;
         s->period[w] |= (data & 0x0F) << 8;
	}
	break;

  case 0x6:
	s->noise.period = ((data & 0x1F) ? (data & 0x1F) : 0x01) << 1;
	break;

  case 0x7:
	s->noise_tone_disable = (data & 0x3F) << 3;
	break;

  case 0x8:
  case 0x9:
  case 0xA:
	{
	 const size_t w = addr & 0x3;

	 s->volume_ptrs[w] = &s->volumes[(data & 0x10) ? 3 : w];
	 s->volumes[w] = s->db_lut[((data & 0x1F) << 1) + 1];
	}
	break;

  case 0xB:
	s->env.period &= 0xFF00;
	s->env.period |= data;
	break;

  case 0xC:
	s->env.period &= 0x00FF;
	s->env.period |= data << 8;
	break;

  case 0xD:
	s->env.level = ((data & 0x04) ? 0x00 : 0x1F);
	s->volumes[3] = s->db_lut[s->env.level];
	s->env.divider = 0;	// Correct?
	s->env.inc = ((data & 0x04) >> 1) - 1;
	s->env.stop_v = s->shape_tab[data & 0x0F][0];
	s->env.alt_v  = s->shape_tab[data & 0x0F][1];
	break;

  case 0xE:
  case 0xF:
	asm volatile("");
	break;
 }
}

void sun5b_slave_entry(uint32 timestamp_scale, volatile uint16* exchip_buffer, uint32 ref_volume)
{
 sun5b_t sun5b = { { 0 } };

 {
  extern uint8 sun5b_reloc_dest[];
  extern uint8 sun5b_reloc_start[];
  extern uint8 sun5b_reloc_bound[];
  volatile uint8* s = sun5b_reloc_start;
  volatile uint8* d = sun5b_reloc_dest;
  const size_t reloc_size = (sun5b_reloc_bound - sun5b_reloc_start);

  //const size_t total = sizeof(sun5b) + (sun5b_reloc_bound - sun5b_reloc_start)
  //printf("%08x %08x %08x\n", (unsigned)sun5b_reloc_dest, (unsigned)sun5b_reloc_start, (unsigned)sun5b_reloc_bound);
  //printf("Total: %zu\n", total);
  //assert(total <= 1280);
  uint32 sp;
  asm volatile("mov r15, %0\n\t" :"=r"(sp));

  //printf("%08x, %08zx\n", sp, reloc_size);

  assert((sp - ((unsigned)sun5b_reloc_dest + reloc_size)) >= 0x140);
  while(s != sun5b_reloc_bound)
  {
   //printf("0x%04x: %02x\n", (unsigned)(s - sun5b_reloc_start), *s);
   *d = *s;
   s++;
   d++;
  }
 }

 TCR = 0x0;
 TOCR = 0;

 sh2_set_sr_s(true);

 sun5b.resamp_scale = timestamp_scale << (32 - 17);
 sun5b.scsp_ptr = exchip_buffer;
 sun5b.sample_offs = (-8 & 0x7FF) << 1;	// Approximate, 44100 * (40 / 2 + 0.5) / (1789772.727272/16)

 sun5b.source_pos_inc = (256LL * (1U << (21 - 4))) / timestamp_scale;
 sun5b.source_pos_f_inc = ((256LL * (1U << (21 - 4))) % timestamp_scale) << (32 - 17);

 //printf("%08x %08x\n", sun5b.resamp_scale, sun5b.source_pos_f_inc);
 //printf("%u\n", sun5b.source_pos_inc / 256);

 sun5b.buf_offs = 40 << 1;

 memcpy(exchip_resamp, exchip_tables.resamp_div16, sizeof(exchip_tables.resamp_div16));

 for(int i = 0; i < 0x20; i++)
 {
  int tmp = -(ref_volume * exp2_fxp_16_16((i - 0x19) << (16 - 2)) >> 16);

  if(i < 0x02)
   tmp = 0;
  //
  //
  //printf("ref: %u, % 6d --- 0x%02x\n", ref_volume, tmp, i);

  assert(tmp <= 0);
  assert((tmp * 3) >= -32768);
  //
  //
  sun5b.db_lut[0x00 + i] = tmp;
  sun5b.db_lut[0x20 + i] = tmp;
 }

 assert((sun5b.db_lut[0x00] | sun5b.db_lut[0x01] | sun5b.db_lut[0x20] | sun5b.db_lut[0x21]) == 0);

 for(unsigned i = 0; i < 0x10; i++)
 {
  // stop_v, alt_v
  static const int8 shape_tab[0x10][2] =
  {
   // 0x0 ... 0x3
   { 0x00, -1 },
   { 0x00, -1 },
   { 0x00, -1 },
   { 0x00, -1 },

   // 0x4 ... 0x7
   { 0x20, -1 },
   { 0x20, -1 },
   { 0x20, -1 },
   { 0x20, -1 },

   // 0x8
   { -1, -1 },

   // 0x9
   { 0x00, -1 },

   // 0xA
   { -1, 0x3F },

   // 0xB
   { 0x3F, -1 },

   // 0xC
   { -1, -1 },

   // 0xD
   { 0x1F, -1 },

   // 0xE
   { -1, 0x20 },

   // 0xF
   { 0x20, -1 }
  };

  sun5b.shape_tab[i][0] = shape_tab[i][0];
  sun5b.shape_tab[i][1] = shape_tab[i][1];
 }

 sun5b.volume_ptrs[0] = &sun5b.volumes[0];
 sun5b.volume_ptrs[1] = &sun5b.volumes[1];
 sun5b.volume_ptrs[2] = &sun5b.volumes[2];
 sun5b.volume_ptrs[3] = &sun5b.volumes[4];

 for(unsigned i = 0x00; i < 0x80; i++)
 {
  uint32 l = 0;

  if((i & 0x02) && ((i & 0x10) || (i & 0x01)))
   l |= 0 << 2;
  else
   l |= 3 << 2;

  if((i & 0x04) && ((i & 0x20) || (i & 0x01)))
   l |= 1 << 10;
  else
   l |= 3 << 10;

  if((i & 0x08) && ((i & 0x40) || (i & 0x01)))
   l |= 2 << 18;
  else
   l |= 3 << 18;

  sun5b.lut[i] = l;
 }


 sun5b.noise.lfsr = 1 << 2;

 //printf("%016llx %016llx\n", vrc6.pulse_dividend, vrc6.saw_dividend);
 //printf("size: %zu\n", sizeof(vrc6));
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

  while(sun5b.rb_rd != rb_wr)
  {
   const uint32 rbd = exchip_rb[sun5b.rb_rd];
   uint32 timestamp = rbd >> 16;
   uint8 addr = rbd >> 8;
   uint8 data = rbd >> 0;

   sun5b.rb_rd = (sun5b.rb_rd + 1) & 0x3FFF;
   //
   //
   sun5b_run(&sun5b, timestamp);
   //
   //
   if(LIKELY(addr < 0x10))
    sun5b_write(&sun5b, addr, data);
   else if(addr == 0xFD) // Force-sync
   {

   }
   else if(addr == 0xFE) // Reset timestamp
   {
    EXCHIP_BENCH_END_FRAME
    //
    //
    sun5b.prev_timestamp -= timestamp;
   }
   else if(addr == 0xFF)	// Exit
    return;
  }

  EXCHIP_BENCH_END
 }
}

/*
uint32 sun5b_get_max_ref_volume(void)
{
 // 32768 / (3 * (2**1.5))
 return (((uint32)32768 << 16) / (3 * (exp2_fxp_16_16(0x00018000)))) >> 16;
}
*/
