/*
 * gs/vdp1.h
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

#ifndef GIGASANTA_VDP1_H
#define GIGASANTA_VDP1_H

#include "types.h"

enum { TVMR_8BPP   = 0x1 };
enum { TVMR_ROTATE = 0x2 };
enum { TVMR_HDTV   = 0x4 };
enum { TVMR_VBE    = 0x8 };
#define TVMR (*(volatile uint16*)0x25D00000)

enum { FBCR_FCT	   = 0x01 };	// Frame buffer change trigger
enum { FBCR_FCM	   = 0x02 };	// Frame buffer change mode
enum { FBCR_DIL	   = 0x04 };	// Double interlace draw line(0=even, 1=odd)
enum { FBCR_DIE	   = 0x08 };	// Double interlace enable
enum { FBCR_EOS	   = 0x10 };	// Even/Odd coordinate select(0=even, 1=odd, used with HSS)
#define FBCR (*(volatile uint16*)0x25D00002)

#define PTMR (*(volatile uint16*)0x25D00004)

#define ENDR (*(volatile uint16*)0x25D0000C)

enum { EDSR_BEF    = 0x1 };
enum { EDSR_CEF    = 0x2 };
#define EDSR (*(volatile uint16*)0x25D00010)

#define LOPR (*(volatile uint16*)0x25D00012)
#define COPR (*(volatile uint16*)0x25D00014)
#define MODR (*(volatile uint16*)0x25D00016)

static volatile uint16* const VDP1_VRAM = (volatile uint16*)0x25C00000;
static volatile uint16* const VDP1_FB = (volatile uint16*)0x25C80000;

#define EWDR (*(volatile uint16*)0x25D00006)
#define EWLR (*(volatile uint16*)0x25D00008)
#define EWRR (*(volatile uint16*)0x25D0000A)

#endif
