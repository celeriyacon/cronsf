/*
 * gs/vdp2.h
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

#ifndef GIGASANTA_VDP2_H
#define GIGASANTA_VDP2_H

#include "types.h"

static volatile uint16* const CRAM = (volatile uint16*)0x25F00000;
static volatile uint16* const VDP2_VRAM = (volatile uint16*)0x25E00000;

typedef struct
{
 int32 Xst, Yst, Zst;
 int32 DXst, DYst;
 int32 DX, DY;
 int32 A, B, C, D, E, F;
 int16 Px, Py, Pz;

 uint16 dummy0;

 int16 Cx, Cy, Cz;

 uint16 dummy1;

 int32 Mx, My;

 int32 kx, ky;

 int32 KAst;
 int32 DKAst;
 int32 DKAx;

 uint16 dummy2[0x10];
} vdp2_raw_rot_param_t;

#define TVMD	(*(volatile uint16*)0x25F80000)
#define EXTEN   (*(volatile uint16*)0x25F80002)
#define TVSTAT	(*(volatile uint16*)0x25F80004)
#define VRSIZE	(*(volatile uint16*)0x25F80006)
#define HCNT	(*(volatile uint16*)0x25F80008)
#define VCNT	(*(volatile uint16*)0x25F8000A)
#define RAMCTL	(*(volatile uint16*)0x25F8000E)

enum
{
 CRAM_MODE_RGB555_1024	= 0,
 CRAM_MODE_RGB555_2048	= 1,
 CRAM_MODE_RGB888_1024	= 2,
 CRAM_MODE_ILLEGAL	= 3
};

#define CYCA0	(*(volatile uint32*)0x25F80010)
#define CYCA1	(*(volatile uint32*)0x25F80014)
#define CYCB0	(*(volatile uint32*)0x25F80018)
#define CYCB1	(*(volatile uint32*)0x25F8001C)

enum
{
 RDBS_UNUSED = 0x0,
 RDBS_COEFF  = 0x1,
 RDBS_NAME   = 0x2,
 RDBS_CHAR   = 0x3
};

enum
{
 VCP_NBG0_NT = 0x0,
 VCP_NBG1_NT = 0x1,
 VCP_NBG2_NT = 0x2,
 VCP_NBG3_NT = 0x3,

 VCP_NBG0_CG = 0x4,
 VCP_NBG1_CG = 0x5,
 VCP_NBG2_CG = 0x6,
 VCP_NBG3_CG = 0x7,

 VCP_NBG0_VCS = 0xC,
 VCP_NBG1_VCS = 0xD,
 VCP_CPU = 0xE,
 VCP_NOP = 0xF
};


// BG screen display enable
#define BGON	(*(volatile uint16*)0x25F80020)

// Mosaic Control
#define MZCTL	(*(volatile uint16*)0x25F80022)

// Special function code select
#define SFSEL	(*(volatile uint16*)0x25F80024)

// Special function code
#define SFCODE	(*(volatile uint16*)0x25F80026)

// Character control(NBG0, NBG1)
#define CHCTLA	(*(volatile uint16*)0x25F80028)

// Character control(NBG2, NBG3)
#define CHCTLB	(*(volatile uint16*)0x25F8002A)

// Bitmap palette number(NBG0, NBG1)
#define BMPNA	(*(volatile uint16*)0x25F8002C)

#define PNCN0	(*(volatile uint16*)0x25F80030)
#define PNCN1	(*(volatile uint16*)0x25F80032)
#define PNCN2	(*(volatile uint16*)0x25F80034)
#define PNCN3	(*(volatile uint16*)0x25F80036)

// Plane size
#define PLSZ	(*(volatile uint16*)0x25F8003A)

// Map offset(NBG0~NBG3)
#define MPOFN	(*(volatile uint16*)0x25F8003C)

// Map offset(RBG A and B params)
#define MPOFR	(*(volatile uint16*)0x25F8003E)

#define MPABRA	(*(volatile uint16*)0x25F80050)

#define SCXN0	(*(volatile uint32*)0x25F80070) // Screen scroll(NBG0, horizontal)
#define SCYN0	(*(volatile uint32*)0x25F80074) // Screen scroll(NBG0, vertical)

#define ZMXN0	(*(volatile uint32*)0x25F80078) // Coordinate increment(NBG0, horizontal)
#define ZMYN0	(*(volatile uint32*)0x25F8007C) // Coordinate increment(NBG0, vertical)

#define SCXN1	(*(volatile uint32*)0x25F80080) // Screen scroll(NBG1, horizontal)
#define SCYN1	(*(volatile uint32*)0x25F80084) // Screen scroll(NBG1, vertical)

#define ZMXN1	(*(volatile uint32*)0x25F80088) // Coordinate increment(NBG1, horizontal)
#define ZMYN1	(*(volatile uint32*)0x25F8008C) // Coordinate increment(NBG1, vertical)

#define SCXN2	(*(volatile uint16*)0x25F80090)	// X scroll, NBG2
#define SCYN2	(*(volatile uint16*)0x25F80092)	// Y scroll, NBG2

#define SCXN3	(*(volatile uint16*)0x25F80094)	// X scroll, NBG3
#define SCYN3	(*(volatile uint16*)0x25F80096)	// Y scroll, NBG3

#define ZMCTL	(*(volatile uint16*)0x25F80098)

// Line and Vertical Cell Scroll Control Register
#define SCRCTL	(*(volatile uint16*)0x25F8009A)

#define VCSTA	(*(volatile uint32*)0x25F8009C)

#define SCR_N0_VCSC_EN	0x0001	// Vertical cell scroll
#define SCR_N0_LSCX_EN	0x0002	// Line scroll, X
#define SCR_N0_LSCY_EN	0x0004	// Line scroll, Y
#define SCR_N0_LZMX_EN	0x0008	// Line zoom X
#define SCR_N0_LSS_0	(0 << 4)
#define SCR_N0_LSS_1	(1 << 4)
#define SCR_N0_LSS_2	(2 << 4)
#define SCR_N0_LSS_3	(3 << 4)

// Line scroll table address registers
#define LSTA0	(*(volatile uint32*)0x25F800A0)	// NBG0
#define LSTA1	(*(volatile uint32*)0x25F800A4) // NBG1

// Line color screen table address register
#define LCTA	(*(volatile uint32*)0x25F800A8)

// Back screen table address register
#define BKTA	(*(volatile uint32*)0x25F800AC)

// Rotation
#define RPMD	(*(volatile uint16*)0x25F800B0)	// Rotation parameter mode
#define RPRCTL	(*(volatile uint16*)0x25F800B2)	// Rotation parameter read control
#define KTCTL	(*(volatile uint16*)0x25F800B4)	// Coefficient table control
#define KTAOF	(*(volatile uint16*)0x25F800B6)	// Coefficient table address offset

#define RPTA	(*(volatile uint32*)0x25F800BC)	// Rotation parameter table address

// Window Coordinates
#define WPSX0	(*(volatile uint16*)0x25F800C0)	// Window 0 X Start
#define WPSY0	(*(volatile uint16*)0x25F800C2)	// Window 0 Y Start
#define WPEX0	(*(volatile uint16*)0x25F800C4)	// Window 0 X End
#define WPEY0	(*(volatile uint16*)0x25F800C6)	// Window 0 Y End

#define WPSX1	(*(volatile uint16*)0x25F800C8)	// Window 1 X Start
#define WPSY1	(*(volatile uint16*)0x25F800CA)	// Window 1 Y Start
#define WPEX1	(*(volatile uint16*)0x25F800CC)	// Window 1 X End
#define WPEY1	(*(volatile uint16*)0x25F800CE)	// Window 1 Y End


#define WCTLA	(*(volatile uint16*)0x25F800D0)	// NBG0, NBG1
#define WCTLC	(*(volatile uint16*)0x25F800D4)	// RBG0, Sprite
#define WCTLD	(*(volatile uint16*)0x25F800D6)	// RP, CC

#define LWTA0	(*(volatile uint32*)0x25F800D8) // Line window table address(W0)
#define LWTA1	(*(volatile uint32*)0x25F800DC) // Line window table address(W1)

// Sprite control
#define SPCTL	(*(volatile uint16*)0x25F800E0)

// Shadow control
#define SDCTL	(*(volatile uint16*)0x25F800E2)

// NBG color RAM offsets
#define CRAOFA	(*(volatile uint16*)0x25F800E4)

// Sprite, RBG0 color ram offsets
#define CRAOFB	(*(volatile uint16*)0x25F800E6)

// Line color screen enable
#define LNCLEN	(*(volatile uint16*)0x25F800E8)

// Special priority mode
#define SFPRMD	(*(volatile uint16*)0x25F800EA)

// Color calculation control
#define CCCTL	(*(volatile uint16*)0x25F800EC)

// Special color calculation control
#define SFCCMD	(*(volatile uint16*)0x25F800EE)

// Sprite priorities
#define PRISA	(*(volatile uint16*)0x25F800F0)
#define PRISB	(*(volatile uint16*)0x25F800F2)
#define PRISC	(*(volatile uint16*)0x25F800F4)
#define PRISD	(*(volatile uint16*)0x25F800F6)

// NBG priorities
#define PRINA	(*(volatile uint16*)0x25F800F8)
#define PRINB	(*(volatile uint16*)0x25F800FA)

// RBG0 priority
#define PRIR	(*(volatile uint16*)0x25F800FC)

// Reserved?
#define RESERVE	(*(volatile uint16*)0x25F800FE)

// Sprite color calculation ratios
#define CCRSA	(*(volatile uint16*)0x25F80100)
#define CCRSB	(*(volatile uint16*)0x25F80102)
#define CCRSC	(*(volatile uint16*)0x25F80104)
#define CCRSD	(*(volatile uint16*)0x25F80106)

// NBG color calculation ratios
#define CCRNA	(*(volatile uint16*)0x25F80108)
#define CCRNB	(*(volatile uint16*)0x25F8010A)

// RBG0 color calculation ratio
#define CCRR	(*(volatile uint16*)0x25F8010C)

// Back color screen and line color screen color calculation ratios
#define CCRLB	(*(volatile uint16*)0x25F8010E)

// Color offset enable
#define CLOFEN	(*(volatile uint16*)0x25F80110)

// Color offset select
#define CLOFSL	(*(volatile uint16*)0x25F80112)

// Color offset A R/G/B
#define COAR	(*(volatile uint16*)0x25F80114)
#define COAG	(*(volatile uint16*)0x25F80116)
#define COAB	(*(volatile uint16*)0x25F80118)

// Color offset B R/G/B
#define COBR	(*(volatile uint16*)0x25F8011A)
#define COBG	(*(volatile uint16*)0x25F8011C)
#define COBB	(*(volatile uint16*)0x25F8011E)

#endif
