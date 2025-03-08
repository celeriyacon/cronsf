/*
 * gs/scsp.h
 *
 * Copyright (C) 2021-2024 celeriyacon - https://github.com/celeriyacon
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

#ifndef GIGASANTA_SCSP_H
#define GIGASANTA_SCSP_H

#include "types.h"

#ifdef __m68k__
 #define SCSPVP(offset) ((volatile void*)(offset))
#else
 #define SCSPVP(offset) ((volatile void*)(0x25A00000 + (offset)))
#endif

#define SCSP(offset) (*(volatile uint16*)SCSPVP(offset))
#define SCSP16(offset) SCSP(offset)
#define SCSP8(offset) (*(volatile uint8*)SCSPVP(offset))
#define SCSP_MEM(offset) SCSP(offset)

#define SCSP_SREG(slot, offset)    SCSP16(0x100000 + (slot) * 0x20 + (offset))
#define SCSP_SREG_HI(slot, offset)  SCSP8(0x100000 + (slot) * 0x20 + (offset))
#define SCSP_SREG_LO(slot, offset)  SCSP8(0x100001 + (slot) * 0x20 + (offset))

#define SCSP_CREG(offset)    SCSP16(0x100400 + (offset))
#define SCSP_CREG_HI(offset)  SCSP8(0x100400 + (offset))
#define SCSP_CREG_LO(offset)  SCSP8(0x100401 + (offset))

#define SCSP_SCIEB SCSP_CREG(0x0F << 1)
#define SCSP_SCIPD SCSP_CREG(0x10 << 1)
#define SCSP_SCIRE SCSP_CREG(0x11 << 1)

#define SCSP_MCIEB SCSP_CREG(0x15 << 1)
#define SCSP_MCIPD SCSP_CREG(0x16 << 1)
#define SCSP_MCIRE SCSP_CREG(0x17 << 1)

#define SCSP_SCIEB_HI SCSP_CREG_HI(0x0F << 1)
#define SCSP_SCIPD_HI SCSP_CREG_HI(0x10 << 1)
#define SCSP_SCIRE_HI SCSP_CREG_HI(0x11 << 1)

#define SCSP_MCIEB_HI SCSP_CREG_HI(0x15 << 1)
#define SCSP_MCIPD_HI SCSP_CREG_HI(0x16 << 1)
#define SCSP_MCIRE_HI SCSP_CREG_HI(0x17 << 1)

#define SCSP_SCIEB_LO SCSP_CREG_LO(0x0F << 1)
#define SCSP_SCIPD_LO SCSP_CREG_LO(0x10 << 1)
#define SCSP_SCIRE_LO SCSP_CREG_LO(0x11 << 1)

#define SCSP_MCIEB_LO SCSP_CREG_LO(0x15 << 1)
#define SCSP_MCIPD_LO SCSP_CREG_LO(0x16 << 1)
#define SCSP_MCIRE_LO SCSP_CREG_LO(0x17 << 1)

static INLINE void scsp_wait_sample_irq(void)
{
 SCSP_SCIRE_HI = 0x4;
 while(!(SCSP_SCIPD_HI & 0x4));
}

// Waits for at least the number of specified full sample periods
// (may be up to 1 beyond the amount requested)
static INLINE void scsp_wait_full_samples(unsigned count)
{
 while(count-- != (unsigned)-1)
  scsp_wait_sample_irq();
}

void scsp_init(void);

#include "dsp-macros.h"

static volatile uint64* const MPROG = (volatile uint64*)SCSPVP(0x100800);	// 128
static volatile uint16* const MADRS = (volatile uint16*)SCSPVP(0x100780);	//  32
static volatile uint16* const COEF = (volatile uint16*)SCSPVP(0x100700);	//  64

static volatile uint32* const TEMP = (volatile uint32*)SCSPVP(0x100C00);	// 128
static volatile uint32* const MEMS = (volatile uint32*)SCSPVP(0x100E00);	//  32
static volatile uint32* const MIXS = (volatile uint32*)SCSPVP(0x100E80);	//  16
static volatile uint16* const EFREG = (volatile uint16*)SCSPVP(0x100EC0);	//  16

#define DSP_MAKE_MEMS(v) (((uint32)((v) & 0xFF) << 16) | (((v) >> 8) & 0xFFFF))
#define DSP_MAKE_TEMP(v) (((uint32)((v) & 0xFF) << 16) | (((v) >> 8) & 0xFFFF))
#define DSP_MAKE_MIXS(v) (((uint32)((v) & 0x0F) << 16) | (((v) >> 4) & 0xFFFF))
#define DSP_MAKE_COEF(v) ((v) << 3)

#define DSP_UNMAKE_MEMS(v) ((((v) >> 16) & 0xFF) | (((v) & 0xFFFF) << 8))
#define DSP_UNMAKE_TEMP(v) ((((v) >> 16) & 0xFF) | (((v) & 0xFFFF) << 8))

#endif
