/*
 * init68k.c
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

#include "gs/types.h"
#include "gs/scsp.h"

void _start(void)
{
 asm volatile(
	"movea.l %6, %%a7\n\t"
	"move.l 1f + 0x00, %%a7@(0x00)\n\t"
	"move.l 1f + 0x04, %%a7@(0x04)\n\t"
	"move.l 1f + 0x08, %%a7@(0x08)\n\t"
	"move.l 1f + 0x0C, %%a7@(0x0C)\n\t"
	"move.l 1f + 0x10, %%a7@(0x10)\n\t"
	"move.l 1f + 0x14, %%a7@(0x14)\n\t"
	"move.l 1f + 0x18, %%a7@(0x18)\n\t"
	"jmp %%a7@\n\t"

	"1:\n\t"
	// Pulse0
	"move.w #0xDEAD, %%d0\n\t"
	"move.w %%d0, %0@\n\t"
	"move.w %%d0, %1@\n\t"

	// Pulse1
	"move.w #0xBEEF, %%d0\n\t"
	"move.w %%d0, %2@\n\t"
	"move.w %%d0, %3@\n\t"

	// Triangle
	"move.w #0xF00D, %4@\n\t"

	// Noise
	"move.l #0xCAFEBABE, %5@\n\t"

	"jmp %%a7@\n\t"

	:
	: "a"(&SCSP_SREG(10 + 0, 0x02)), "a"(&SCSP_SREG(10 + 1, 0x02)),	// Pulse0
	  "a"(&SCSP_SREG(10 + 2, 0x02)), "a"(&SCSP_SREG(10 + 3, 0x02)),	// Pulse1
	  "a"(&SCSP_SREG(14, 0x02)),	// Triangle
	  "a"(&SCSP_SREG(15, 0x00)), // Noise
	  "d"(EFREG)	// Jump target
	: "memory", "cc");
}
