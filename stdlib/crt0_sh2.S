/*
 * crt0_sh2.S
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

.section .init, "ax", @progbits

.global _start
.align 2
_start:
	mov #0xFFFFFFF0, r0
	ldc r0, sr
	//
	// Initialize .bss section(in high work RAM)
	//
	mov #0, r1
	mov.l .L_bss_start, r2
	mov.l .L_bss_bound, r3
	sub r2, r3

1:	mov.b r1, @r2
	add #1, r2
	dt r3
	bf 1b
	//
	// Initialize low work RAM, VDP1 VRAM, and VDP2 VRAM
	// (for custom bss sections, mainly):
	//
	mov.l .L_loram, r1
	mov.l .L_vdp1_vram, r2
	mov #2, r3
	swap.w r3, r3
	mov #0, r4
.align 4
1:	mov.l r4, @r2
	add #4, r2

	mov.l r4, @r1
	add #4, r1

	dt r3
	bf 1b

	mov.l .L_vdp2_vram, r2
	mov #2, r3
	swap.w r3, r3
.align 4
1:	mov.l r4, @r2
	add #4, r2

	mov.l r4, @r1
	add #4, r1

	dt r3
	bf 1b
	//
	//
	//
	mov #0x10, r0
	mov.l .L_ccr_addr, r1
	mov.b r0, @r1
	mov #0x01, r2
 
	mov #0, r4
	mov #0, r5
	mov.l .L_main_addr, r0
	jsr @r0
	mov.b r2, @r1

	/* Reset */
	mov #0x0D, r0
	mov.l .L_smpc_comreg, r2
1:
	bra 1b
	mov.b r0, @r2

.align 2
.L_ccr_addr:
	.long 0xFFFFFE92
.L_main_addr:
	.long _main
.L_smpc_comreg:
	.long 0x2010001F
.L_bss_start:
	.long bss_start
.L_bss_bound:
	.long bss_bound
.L_loram:
	.long 0x00200000
.L_vdp1_vram:
	.long 0x05C00000
.L_vdp2_vram:
	.long 0x05E00000
