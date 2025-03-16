/*
 * n163.c
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
#include "n163.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

typedef struct
{
 uint32 ph;
 uint32 ph_inc;
 uint32 length;
 uint32 wf_offs;
 int32 volume;
 int32 padding[3];
} n163_channel_t;

typedef struct
{
 int32 divider;
 uint32 ch_counter;
 uint32 ch_limit;

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

 int32 output_volume;

 int16 buf[64 * 2];
 int8 wf[256];
 n163_channel_t channels[8];
} n163_t;

static __attribute__((section(".n163_reloc"))) __attribute__((noinline,noclone)) void n163_run(n163_t* n, int32 timestamp)
{
 n->divider += (uint16)(timestamp - n->prev_timestamp);
 n->prev_timestamp = timestamp;

 while(n->divider >= 0)
 {
  n->divider -= 15;
  //
  //
  n163_channel_t* ch = &n->channels[n->ch_counter];
  uint32 av;

  av = n->wf[(ch->ph + ch->wf_offs) >> 24] * (int16)ch->volume;

  ch->ph += ch->ph_inc;
  if(ch->ph >= ch->length)
   ch->ph -= ch->length;

  n->ch_counter++;
  if(n->ch_counter >= n->ch_limit)
   n->ch_counter = 0;
  //
  //
  EXCHIP_SINESWEEP_DO(av)

  n->buf[n->buf_offs +  0] = av;
  n->buf[n->buf_offs + 64] = av;
  n->buf_offs = (n->buf_offs + 1) & 0x3F;

  n->resamp_divider += n->resamp_scale;
  if(n->resamp_divider >= (1U << 21))
  {
   n->resamp_divider -= (1U << 21);
   //
   int32 accum = 0;
   int16* wf = &n->buf[(n->source_pos >> 8) & 0x3F];
   int16* imp = &exchip_resamp[(n->source_pos & 0xFF) * 40];
   {
     asm volatile(
	"clrmac\n\t"

	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"

	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"

	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"

	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"

	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"
	"mac.w @%[wf]+, @%[imp]+\n\t"

	"sts macl, %[accum]\n\t"
	: [accum] "=&r"(accum), [wf] "+&r"(wf), [imp] "+&r"(imp)
	: 
	: "cc", "mach", "macl");
   }

   n->scsp_ptr[n->sample_offs] = ((int16)(accum >> 16) * n->output_volume) >> 16;
   n->sample_offs = (n->sample_offs + 1) & 0x7FF;
   //
   n->source_pos += n->source_pos_inc;
   n->source_pos_f += n->source_pos_f_inc;
   if(n->source_pos_f >= n->resamp_scale)
   {
    n->source_pos++;
    n->source_pos_f -= n->resamp_scale;
   }
  }
 }
}

static INLINE void n163_write(n163_t* n, uint8 addr, uint8 data)
{
 n->wf[(addr << 1) + 0] = (((data >> 0) & 0xF) - 8);
 n->wf[(addr << 1) + 1] = (((data >> 4) & 0xF) - 8);

 if(addr >= 0x40)
 {
  n163_channel_t* ch = &n->channels[7 - ((addr & 0x3F) >> 3)];

  switch(addr & 0x7)
  {
   case 0x0:
	ch->ph_inc &= ~(0xFF << 8);
	ch->ph_inc |= data << 8;
	break;

   case 0x1:
	ch->ph &= ~(0xFF << 8);
	ch->ph |= data << 8;
	break;

   case 0x2:
	ch->ph_inc &= ~(0xFF << 16);
	ch->ph_inc |= data << 16;
	break;

   case 0x3:
	ch->ph &= ~(0xFF << 16);
	ch->ph |= data << 16;
	break;

   case 0x4:
	ch->ph_inc &= ~(0xFF << 24);
	ch->ph_inc |= (data & 0x3) << 24;
	ch->length = (0x100 - (data & 0xFC)) << 24;
	break;

   case 0x5:
	ch->ph &= ~(0xFF << 24);
	ch->ph |= data << 24;
	break;

   case 0x6:
	ch->wf_offs = data << 24;
	break;

   case 0x7:
	ch->volume = (data & 0xF) << 7;

	if(addr == 0x7F)
	{
	 n->ch_limit = 1 + ((data >> 4) & 0x7);
	 //printf("%d\n", n->ch_limit);
	}
	break;
  }
 }
}

void n163_slave_entry(uint32 timestamp_scale, volatile uint16* exchip_buffer, uint32 ref_volume)
{
 n163_t n163 = { 0 };

 EXCHIP_RELOC(n163, &n163)
 //
 //
 TCR = 0x0;
 TOCR = 0;

 sh2_set_sr_s(true);

 //n163.output_volume = (21903 * /*ref_volume*/4); // >> 16; // * 1; //-8192; //-19168;
 n163.output_volume = (((uint64)ref_volume * 65536) / (64 * 15 * 15));

 n163.resamp_scale = (15 * timestamp_scale);
 n163.scsp_ptr = exchip_buffer;
 n163.sample_offs = (-8 & 0x7FF);	// Approximate, 44100 * (40 / 2 + 0.5) / (1789772.727272/15)

 //sun5b.source_pos_inc = (256LL * (1U << (21 - 4))) / timestamp_scale;
 //sun5b.source_pos_f_inc = ((256LL * (1U << (21 - 4))) % timestamp_scale) << (32 - 17);

 n163.source_pos_inc = (256LL * (1U << 21)) / n163.resamp_scale;
 n163.source_pos_f_inc = (256LL * (1U << 21)) % n163.resamp_scale;

 //printf("%u %u\n", n163.source_pos_inc, n163.source_pos_f_inc);

 n163.buf_offs = 40;

 memcpy(exchip_resamp, exchip_tables.resamp_n163, sizeof(exchip_tables.resamp_n163));
 //
 //
 n163.ch_limit = 1;
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

  while(n163.rb_rd != rb_wr)
  {
   const uint32 rbd = exchip_rb[n163.rb_rd];
   uint32 timestamp = rbd >> 16;
   uint8 addr = rbd >> 8;
   uint8 data = rbd >> 0;

   n163.rb_rd = (n163.rb_rd + 1) & 0x3FFF;
   //
   //
   n163_run(&n163, timestamp);
   //
   //
   if(LIKELY(addr < 0x80))
   {
    //printf("%02x %02x\n", addr, data);
    n163_write(&n163, addr, data);
   }
   else if(addr == 0xFD) // Force-sync
   {

   }
   else if(addr == 0xFE) // Reset timestamp
   {
    EXCHIP_BENCH_END_FRAME
    //
    //
    n163.prev_timestamp -= timestamp;
   }
   else if(addr == 0xFF)	// Exit
    return;
  }

  EXCHIP_BENCH_END
 }
}
