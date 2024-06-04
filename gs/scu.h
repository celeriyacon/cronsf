/*
 * gs/scu.h
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

#ifndef GIGASANTA_SCU_H
#define GIGASANTA_SCU_H

#define SCU_T0C		(*(volatile uint32*)0x25FE0090)
#define SCU_T1S		(*(volatile uint32*)0x25FE0094)
#define SCU_T1MD	(*(volatile uint32*)0x25FE0098)

#define SCU_IMS		(*(volatile uint32*)0x25FE00A0)
#define SCU_IST		(*(volatile uint32*)0x25FE00A4)
#define SCU_AIACK	(*(volatile uint32*)0x25FE00A8)

#define SCU_ASR0	(*(volatile uint32*)0x25FE00B0)
#define SCU_ASR1	(*(volatile uint32*)0x25FE00B4)
#define SCU_AREF	(*(volatile uint32*)0x25FE00B8)

#define SCU_RSEL	(*(volatile uint32*)0x25FE00C4)
#define SCU_VER		(*(volatile uint32*)0x25FE00C8)

#define SCU_DMA_RA(level)	(*(volatile uint32*)(0x25FE0000 + ((level) << 5)))
#define SCU_DMA_WA(level)	(*(volatile uint32*)(0x25FE0004 + ((level) << 5)))

#define SCU_DMA_CNT(level)	(*(volatile uint32*)(0x25FE0008 + ((level) << 5)))
#define SCU_DMA_ADD(level)	(*(volatile uint32*)(0x25FE000C + ((level) << 5)))

#define SCU_DMA_EN(level)	(*(volatile uint32*)(0x25FE0010 + ((level) << 5)))
#define SCU_DMA_MODE(level)	(*(volatile uint32*)(0x25FE0014 + ((level) << 5)))

#define SCU_DMA_STOP	(*(volatile uint32*)0x25FE0060)
#define SCU_DMA_STATUS	(*(volatile uint32*)0x25FE007C)

#define SCU_DSP_PPAF	(*(volatile uint32*)(0x25FE0080))	// Program control port
#define SCU_DSP_PPD	(*(volatile uint32*)(0x25FE0084))	// Program RAM data port
#define SCU_DSP_PDA	(*(volatile uint32*)(0x25FE0088))	// Data RAM address port
#define SCU_DSP_PDD	(*(volatile uint32*)(0x25FE008C))	// Data RAM data port

#endif
