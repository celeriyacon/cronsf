/*
 * test_dsp.c
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

#include "config.h"

#include "gs/types.h"
#include "gs/scsp.h"
#include "gs/smpc.h"
#include "gs/bios.h"
#include "gs/sh2.h"

#include <stdio.h>
#include <stdlib.h>

#define CRA_NEG1	  0x0

#define DSP_CRA_NEG1		DSP_CRA(CRA_NEG1)

enum
{
 MIXS_PULSE0	= 0x20,
 MIXS_PULSE1	= 0x21,
 MIXS_TRIANGLE	= 0x22,
 MIXS_NOISE	= 0x23,
 //
 //
 MIXS_NEG4096_SL9	= 0x2D,
 MIXS_NEG4096_SL10 	= 0x2E,
 MIXS_NEG4096_SL11	= 0x2F,
};

static const uint8 Init68KProg[] =
{
 #include "init68k.bin.h"
};

// 0x0...
#define EFREG_PULSE0_ADDR 0x1
#define EFREG_PULSE1_ADDR 0x5
#define EFREG_TRI_ADDR	   0x9
#define EFREG_NOISE_ADDRHI 0xB
#define EFREG_NOISE_ADDRLO 0xC

#define EFREG_APU_OUTPUT 0xF

static void test_dsp_m68k_slot_sync(void)
{
 scsp_init();
 //
 SCSP_CREG(0x01 << 1) = (0x00 << 0) | (3 << 7); // RBP, RBL
 scsp_wait_full_samples(2);
 //
 //
 //
 //
 //
 smpc_sound_off();
 //
 //
 COEF[CRA_NEG1] = DSP_MAKE_COEF(-4096);

 MEMS[0] = DSP_MAKE_MEMS(-256);

 scsp_wait_full_samples(4);
 //
 //
 //
 //
 //
 const unsigned position_addr = 0x00000;
 const unsigned saw_bank64k = 1;
 const unsigned tri_bank64k = 2;
 const unsigned tri_map_addr = 0x30000;

 const unsigned base_slot = 4;

 for(unsigned slot = 0; slot < 32; slot++)
  SCSP_SREG(slot, 0x0C) = (1U << 9);

 // Triangle, pulse0a, pulse0b, pulse1a, pulse1b position reader slots.
 for(unsigned i = 0; i < 5; i++)
 {
  const unsigned slot = base_slot + 0 + i;
  uint32 pa = position_addr + (i << 1);

  SCSP_SREG(slot, 0x00) = (1U << 11) | (0x0 << 9) | (0x1 << 5) | (0U << 4) | ((pa >> 16) << 0);
  SCSP_SREG(slot, 0x02) = pa & 0xFFFF;

  SCSP_SREG(slot, 0x0C) = (1U << 8);
  SCSP_SREG(slot, 0x10) = (0x400 << 0);
 }

 // Triangle map slot
 {
  const unsigned slot = base_slot + 5;
  const unsigned mdxy = (0x40 - 5) & 0x3F;
  const unsigned mdl = 0xF - 5;

  SCSP_SREG(slot, 0x00) = (1U << 11) | (0x0 << 9) | (0x1 << 5) | (0U << 4) | (tri_map_addr >> 16);
  SCSP_SREG(slot, 0x02) = tri_map_addr & 0xFFFF;
  SCSP_SREG(slot, 0x04) = 0;
  SCSP_SREG(slot, 0x0C) = (1U << 8);
  SCSP_SREG(slot, 0x0E) = (mdl << 12) | (mdxy << 6) | (mdxy << 0);
  SCSP_SREG(slot, 0x10) = (0x400 << 0);
 }

 // Pulse wave slots
 for(unsigned i = 0; i < 4; i++)
 {
  const unsigned slot = base_slot + 6 + i;
  const unsigned sb = (i >= 4 || !(i & 1)) ? 0x0 : 0x3;

  SCSP_SREG(slot, 0x00) = (1U << 11) | (sb << 9) | (0x1 << 5) | (1U << 4) | (saw_bank64k << 0);
  SCSP_SREG(slot, 0x06) = 1024;
  SCSP_SREG(slot, 0x0C) = (1U << 9) | (1U << 8);
  //SCSP_SREG(slot, 0x0E) = (mdl << 12) | (mdxy << 6) | (mdxy << 0);
  SCSP_SREG(slot, 0x10) = (0x400 << 0) | (1U << 15);
  //
  SCSP_SREG(slot, 0x16) = (0x07 << 13) | (0x00 << 8);
 }

 // Triangle wave slot 
 {
  const unsigned slot = base_slot + 10;
  const unsigned isel = 2;
  const unsigned mdxy = (0x40 - 5) & 0x3F;
  const unsigned mdl = 0xF - 5;

  SCSP_SREG(slot, 0x00) = (1U << 11) | (0x0 << 9) | (0x1 << 5) | (1U << 4) | (tri_bank64k << 0);
  SCSP_SREG(slot, 0x0C) = (1U << 9) | (1U << 8);
  SCSP_SREG(slot, 0x0E) = (mdl << 12) | (mdxy << 6) | (mdxy << 0);
  SCSP_SREG(slot, 0x10) = (0x400 << 0);
  SCSP_SREG(slot, 0x14) = (isel << 3) | (0x6 << 0);
 }

 // Noise slot
 {
  const unsigned slot = base_slot + 11;
  const unsigned isel = 3;

  SCSP_SREG(slot, 0x00) = (1U << 11) | (0x0 << 9) | (0x1 << 5) | (1U << 4);

  //
  // Set LSA and LEA to 0xFFFF to work around a race condition with the M68K program writing
  // garbage to LPCTL, causing the effective fractional playback position to be inverted,
  // triggering a hardware bug that screws up FM linear interpolation on the previous
  // slot(triangle wave), resulting in audio glitches.
  //
  SCSP_SREG(slot, 0x04) = 0xFFFF;
  SCSP_SREG(slot, 0x06) = 0xFFFF;

  SCSP_SREG(slot, 0x0C) = (1U << 9) | (1U << 8);
  SCSP_SREG(slot, 0x10) = (0x400 << 0);
  SCSP_SREG(slot, 0x14) = (isel << 3) | (0x6 << 0);
 }

 // DSP PCM output slot
 {
  //const unsigned slot = EFREG_APU_OUTPUT;

  //SCSP_SREG(slot, 0x16) = (0x07 << 5) | (0x00 << 0);
 }

 // Value slots
 for(unsigned i = 0; i < 3; i++)
 {
  static const int8 isel[3] = { MIXS_NEG4096_SL9, MIXS_NEG4096_SL10, MIXS_NEG4096_SL11 };
  static const int8 imxl[3] = { 0x5, 0x6, 0x7 };
  const unsigned slot = 24 + i; //base_slot + 5;
  SCSP_SREG(slot, 0x02) = 0;
  SCSP_SREG(slot, 0x04) = 0;
  SCSP_SREG(slot, 0x06) = 1;

  SCSP_SREG(slot, 0x0C) = (1 << 8);
  SCSP_SREG(slot, 0x10) = 0x400;
  SCSP_SREG(slot, 0x14) = (isel[i] << 3) | imxl[i];
  SCSP_SREG(slot, 0x16) = (0 << 13);
  SCSP_SREG(slot, 0x00) = (0 << 12) |
	 		  (0 << 11) |
			  (2 << 9) |
 			  (2 << 7) |
			  ((0x00 & 0x3) << 5) |
			  ((0x00000 >> 16) & 0xF);
 }

 SCSP_SREG(0x1F, 0x00) |= 1U << 12;

 scsp_wait_full_samples(8);

 for(unsigned i = 0; i < sizeof(Init68KProg); i++)
  SCSP8(i) = Init68KProg[i];

 smpc_sound_on();

 // Before continuing, wait to ensure M68K execution has moved
 // to EFREG space.
 scsp_wait_full_samples(16);
 //
 //
 //

 for(unsigned i = 0; i < 65536; i++)
  SCSP8((saw_bank64k << 16) + i) = ((i & 1) ? -64 : 64);

 SCSP_CREG_LO(0x00 << 1) = (0x0F << 0);	// MVOL to max
 //
 //
 //
 scsp_wait_full_samples(8);


 // pulse0 0...12: bad...80...127 bad
 // pulse1 0...20: bad
 // slot*4 - 28
 for(unsigned i = 1; i < 128; i++) //16; i < 48; i++)
 {
  unsigned offs = i - 1;

  printf("%u\n", i);

  for(unsigned j = 0; j < 128; j++)
   MPROG[j] = 0;

  MPROG[offs++ & 0x7F] = DSP_BSEL_TEMP | DSP_TRA(0x01) |
	   		DSP_XSEL_INPUTS | DSP_IRA(0x00) |
	   		DSP_YSEL_COEF | DSP_CRA_NEG1;

  MPROG[offs++ & 0x7F] = DSP_SHFT0 | DSP_SHFT1 |
			DSP_TWT | DSP_TWA(0x00) |
			DSP_EWT | DSP_EWA(EFREG_PULSE0_ADDR);
  //
  //
  sh2_wait_approx(30000000);
 }

 for(;;)
 {
  //SCSP_SREG(10, 0x02) = rand();
 }
}

int main(void)
{
 printf("Start0\n");
 //bios_change_clock_speed(0x1);
 //
 printf("Start1\n");
 //
 //
 test_dsp_m68k_slot_sync();

 return 0;
}
