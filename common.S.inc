/*
 * common.S.inc
 *
 * Copyright (C) 2024 celeriyacon - https://github.com/celeriyacon
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

.macro ASSERT v
	.if (\v)
	.else
		.error "Assertion \v failed!"
	.endif
.endm
//#define ASSERT(v)

#define REG_CYC_CNT r4		// Cycle counter
#define REG_ADDR r5		// REG_ADDR is passed to external read/write handlers and must not be modified.
				// May also be used as a scratch register inside S6502 and the event handler.
#define REG_DATA_BUS r6		// Data bus
#define REG_RAM	r7		// Pointer to start of 2KiB RAM and upper bound of 64KiB instruction handler area.

#define REG_NZ	r8		// N/Z flag value.  N = (REG_NZ < 0), Z = ((REG_NZ & 0xFF) == 0)
#define REG_PC	r9		// PC (pointer)
#define REG_SP	r10		// SP
#define REG_P	r11		// P (Carry, I, D, V)
#define REG_A	r12		// A
#define REG_X	r13		// X
#define REG_Y	r14		// Y
#define REG_PC_BASE r15		// PC (base pointer)

#define CREG_SCRATCH VBR	// Scratch register to save data across calls to read/write handlers.

#define SREG_RFAP MACL	// STS, MA/IF contention
#define SREG_WFAP MACH	// STS, MA/IF contention

#define GBR_OFFS_BANK0 0x0 * 4
#define GBR_OFFS_BANK1 0x1 * 4
#define GBR_OFFS_BANK2 0x2 * 4
#define GBR_OFFS_BANK3 0x3 * 4
#define GBR_OFFS_BANK4 0x4 * 4
#define GBR_OFFS_BANK5 0x5 * 4
#define GBR_OFFS_BANK6 0x6 * 4
#define GBR_OFFS_BANK7 0x7 * 4

#define GBR_OFFS_BANK8 0x8 * 4
#define GBR_OFFS_BANK9 0x9 * 4
#define GBR_OFFS_BANKA 0xA * 4
#define GBR_OFFS_BANKB 0xB * 4
#define GBR_OFFS_BANKC 0xC * 4
#define GBR_OFFS_BANKD 0xD * 4
#define GBR_OFFS_BANKE 0xE * 4
#define GBR_OFFS_BANKF 0xF * 4
//
//
//
#define GBR_OFFS_TIMESTAMP_BASE 	0x10 * 4
#define GBR_OFFS_FC_NEXT_TIMESTAMP	0x11 * 4
#define GBR_OFFS_DMC_END_TIMESTAMP	0x12 * 4
#define GBR_OFFS_APU_STATUS		0x13 * 4

#define GBR_OFFS_NSF_ROM_BASE    0x58
#define GBR_OFFS_NSF_FRAME_PENDING 0x5C


