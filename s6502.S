/*
 * s6502.S
 *
 * Copyright (C) 2024-2025 celeriyacon - https://github.com/celeriyacon
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

// TODO: Try to ensure the lower 10 bits of the addresses of the hottest instructions
//	 are unique.


#define FLAG_C 0x01
#define FLAG_Z 0x02
#define FLAG_I 0x04
#define FLAG_D 0x08
#define FLAG_B 0x10
#define FLAG_U 0x20
#define FLAG_V 0x40
#define FLAG_N 0x80

#include "s6502.h"

.global _s6502_init
.global _s6502_power
.global _s6502_reset
//.global s6502_execute
.global _s6502_run

.global _s6502_write_shim_S

.global _nsf_read_frame_pending_S
.global _nsf_set_bank_S

.global _vrc6_write_func_S
.global _n163_write_f800_func_S
.global _n163_write_4800_func_S
.global _n163_read_4800_func_S
.global _sun5b_write0_func_S
.global _sun5b_write1_func_S

.global _mmc5_write0_func_S
.global _mmc5_write3_func_S
.global _mmc5_write4_func_S
.global _mmc5_write7_func_S

.global _mmc5_writeX_func_S

.global _mmc5_write_status_func_S
.global _mmc5_read_status_func_S

.global _mmc5_write_mult0_func_S
.global _mmc5_write_mult1_func_S
.global _mmc5_read_mult_res0_func_S
.global _mmc5_read_mult_res1_func_S

.global _bs2xxx_read_func_S
.global _bs6xxx_write_func_S
.global _bs7xxx_write_func_S
.global _bs6xxx_read_func_S
.global _bs7xxx_read_func_S
.global _bs8xxx_read_func_S
.global _bs9xxx_read_func_S
.global _bsAxxx_read_func_S
.global _bsBxxx_read_func_S
.global _bsCxxx_read_func_S
.global _bsDxxx_read_func_S
.global _bsExxx_read_func_S
.global _bsFxxx_read_func_S

.global _ob_rw_func_S
.global _ram_mirror_write_func_S
.global _ram_mirror_read_func_S
.global _ram_write_func_S
.global _ram_read_func_S

.global _apu_read_4015_S

.global _banks_4k
.global _ram
.global _rfap
.global _wfap

.global _timestamp_base
.global _apu_status
.global _fc_next_timestamp

// Inputs: r0 (pointer)
// Clobbers: r0 r1
.macro SET_BANK bank
	mov #(\bank * 16), r1
	swap.b r1, r1
	extu.w r1, r1
	sub r1, r0
	mov.l r0, @(\bank * 4, GBR)
.endm

.macro MOVIU v r
	.if \v < 256
		mov #v, \r
		extu.b \r, \r
	.elseif \v < 65536
		.if (\v & 255) == 0
			mov #(\v >> 8), \r
			extu.b \r, \r
			swap.b \r, \r
		.else
			mov.w @(6, PC), \r
			bra .+6
			extu.w \r, \r
			.short \v
		.endif
	.else
		.align 2
		mov.l @(8, PC), \r
		nop
		bra .+8
		nop
		.long \v
	.endif
.endm

//
//
//
.macro FROM_C_PROLOGUE
	mov.l r8, @-r15
	mov.l r9, @-r15
	mov.l r10, @-r15
	mov.l r11, @-r15
	mov.l r12, @-r15
	mov.l r13, @-r15
	mov.l r14, @-r15

	sts.l mach, @-r15
	sts.l macl, @-r15
	sts.l pr, @-r15

	stc.l sr, @-r15
	stc.l vbr, @-r15
	stc.l gbr, @-r15

	mov #0xFFFFFFF0, r0
	ldc r0, sr

	mov.l 2000f, r0
	ldc r0, gbr

	mov r15, r0
	mov.l r0, @(sp_save - gbr_area, GBR)
	//
	//
	//
	mov.l @(s_abi_spill_top_addr - gbr_area, GBR), r0

	mov.l @r0+, REG_Y
	mov.l @r0+, REG_X
	mov.l @r0+, REG_A
	mov.l @r0+, REG_P
	mov.l @r0+, REG_SP
	mov.l @r0+, REG_PC
	mov.l @r0+, REG_NZ
	mov.l @r0+, REG_RAM

	lds.l @r0+, SREG_WFAP
	lds.l @r0+, SREG_RFAP
	mov.l @r0+, REG_DATA_BUS
	lds.l @r0+, pr
	mov.l @r0+, REG_ADDR
	mov.l @r0+, REG_PC_BASE
	mov.l @r0+, REG_CYC_CNT
.endm

.macro FROM_C_EPILOGUE
	mov.l @(s_abi_spill_bottom_addr - gbr_area, GBR), r0

	mov.l REG_CYC_CNT, @-r0
	mov.l REG_PC_BASE, @-r0
	mov.l REG_ADDR, @-r0
	sts.l pr, @-r0
	mov.l REG_DATA_BUS, @-r0
	sts.l SREG_RFAP, @-r0
	sts.l SREG_WFAP, @-r0

	mov.l REG_RAM, @-r0
	mov.l REG_NZ, @-r0
	mov.l REG_PC, @-r0
	mov.l REG_SP, @-r0
	mov.l REG_P, @-r0
	mov.l REG_A, @-r0
	mov.l REG_X, @-r0
	mov.l REG_Y, @-r0
	//
	//
	//
	mov.l @(sp_save - gbr_area, GBR), r0
	mov r0, r15

	ldc.l @r15+, gbr
	ldc.l @r15+, vbr
	ldc.l @r15+, sr	// Load SR after GBR and VBR

	lds.l @r15+, pr
	lds.l @r15+, macl
	lds.l @r15+, mach

	mov.l @r15+, r14
	mov.l @r15+, r13
	mov.l @r15+, r12
	mov.l @r15+, r11
	mov.l @r15+, r10
	mov.l @r15+, r9
	rts
	mov.l @r15+, r8
.align 2
2000:	.long gbr_area
.endm
//
//
//
.global _s6502_step
_s6502_step:
	FROM_C_PROLOGUE

	mov.l 1f, r2
	mova s6502_run_, r0
	mov.l r0, @(event_handler_s - gbr_area, GBR)

	jmp @r2
	mov #-1, REG_CYC_CNT

_s6502_run:
	mov r4, r1
	FROM_C_PROLOGUE

	mov.l 1f, r2
	mova s6502_run_, r0
	mov.l r0, @(event_handler_s - gbr_area, GBR)

	jmp @r2
	sub r1, REG_CYC_CNT
.align 2
1: .long s6502_execute


s6502_run_:
	FROM_C_EPILOGUE
	rts
	nop
//
//
//
.section ".text.cold"
_s6502_init:
	FROM_C_PROLOGUE
	//
	//
	mov #0, REG_ADDR
	mov #0, REG_DATA_BUS
	mov #0, REG_RAM
	mov #0, REG_NZ
	mov #0, REG_PC
	mov #0, REG_SP
	mov #0, REG_P
	mov #0, REG_A
	mov #0, REG_X
	mov #0, REG_Y
	mov #0, REG_PC_BASE
	mov #0, REG_CYC_CNT
	//
	//
	mov.l ram_addr, r0
	mov r0, REG_RAM

	mov.l ram_addr, r0
	SET_BANK 0x0
	mov.l ram_addr, r0
	SET_BANK 0x1

	mov.l uc_offs, r1

	mov.l rfa_addr, r0
	or r1, r0
	lds r0, SREG_RFAP

	mov.l wfa_addr, r0
	or r1, r0
	lds r0, SREG_WFAP
	//
	//
	FROM_C_EPILOGUE

.align 2
ram_addr: .long _ram
rfa_addr: .long _rfap
wfa_addr: .long _wfap
uc_offs: .long 0x20000000

_s6502_power:
	FROM_C_PROLOGUE
	//
	//
	mov #0, r0
	mov r0, REG_DATA_BUS
	mov r0, REG_NZ
	mov r0, REG_PC
	mov r0, REG_SP
	mov r0, REG_P
	mov r0, REG_A
	mov r0, REG_X
	mov r0, REG_Y
	mov r0, REG_PC_BASE
	mov r0, REG_CYC_CNT
	ldc r0, CREG_SCRATCH

	mov #FLAG_U | FLAG_B, REG_P

	mov #0x08, r0
	swap.b r0, r2
	mov #0x00, r0
	mov REG_RAM, r1
	add REG_RAM, r2
	1:
	mov.b r0, @r1
	add #1, r1
	cmp/eq r1, r2
	bf 1b
	//
	//
	FROM_C_EPILOGUE

_s6502_reset:
	FROM_C_PROLOGUE
	//
	//
	mov #FLAG_I, r0
	or r0, REG_P

	mov.l @(0xF * 4, GBR), r0
	mov.l reset_vector_addr, r1

	mov.b @(r0, r1), r2
	add #1, r1
	mov.b @(r0, r1), r3

	extu.b r2, REG_PC
	extu.b r3, r3
	shll8 r3
	add r3, REG_PC

	stc GBR, r1
	swap.b REG_PC, r0
	shlr2 r0
	and #0x3C, r0

	mov.l @(r0, r1), REG_PC_BASE
	add REG_PC_BASE, REG_PC
	//
	//
	FROM_C_EPILOGUE

.align 2
reset_vector_addr:
	.long 0xFFFC

//
//
//
//
//
//
//
.section ".text"

.align 4
writehandler_region_start:

// exchip_rb must start on a 65536-byte boundary.
.align 2
_vrc6_write_func_S:
	mov.l @(GBR_OFFS_TIMESTAMP_BASE, GBR), r0
	mov REG_CYC_CNT, r3

	mov.l exchip_rb_ptr, r1
	add r0, r3

	mov.w @(GBR_OFFS_EXCHIP_RB_WR, GBR), r0
	extu.w REG_ADDR, r2

	extu.w r0, r0
	add r0, r1

	mov.w r3, @r1
	add #2, r1

	extu.b r2, r0
	and #0x03, r0

	mov r0, r3
	swap.b r2, r0

	and #0x30, r0
	shlr2 r0

	add r0, r3
	extu.b REG_DATA_BUS, r2

	shll8 r3
	add r2, r3

	mov.w r3, @r1
	add #2, r1

	mov r1, r0
	rts
	mov.w r0, @(GBR_OFFS_EXCHIP_RB_WR, GBR)
//
//
//
.align 2
_sun5b_write0_func_S:
	extu.b REG_DATA_BUS, r0
	rts
	mov.b r0, @(GBR_OFFS_SUN5B_ADDR, GBR)
sun5b_rts:
	rts
	nop

.align 2
_sun5b_write1_func_S:
	mov.b @(GBR_OFFS_SUN5B_ADDR, GBR), r0
	extu.b REG_DATA_BUS, r2

	tst #0xF0, r0
	bf/s sun5b_rts

	shll8 r0
	add r0, r2
	//
	//
	mov.l @(GBR_OFFS_TIMESTAMP_BASE, GBR), r0
	mov REG_CYC_CNT, r3

	mov.l exchip_rb_ptr, r1
	add r0, r3

	mov.w @(GBR_OFFS_EXCHIP_RB_WR, GBR), r0
	nop

	extu.w r0, r0
	add r0, r1

	mov.w r3, @r1
	add #2, r1

	mov.w r2, @r1
	add #2, r1

	mov r1, r0
	rts
	mov.w r0, @(GBR_OFFS_EXCHIP_RB_WR, GBR)
//
//
//
.align 2
_n163_write_f800_func_S:
	mov REG_DATA_BUS, r0
	and #0x7F, r0

	mov.b r0, @(GBR_OFFS_N163_ADDR, GBR)
	mov REG_DATA_BUS, r0

	shll r0
	shlr8 r0

	and #0x01, r0
	rts
	mov.b r0, @(GBR_OFFS_N163_ADDR_INC, GBR)

_n163_write_4800_func_S:
	mov.b @(GBR_OFFS_N163_ADDR, GBR), r0
	extu.b REG_DATA_BUS, r3

	mov r0, r1
	swap.b r0, r2

	mov.l @(GBR_OFFS_N163_RAM_PTR, GBR), r0
	add r3, r2

	mov.b REG_DATA_BUS, @(r0, r1)
	nop

	mov.b @(GBR_OFFS_N163_ADDR_INC, GBR), r0
	nop

	add r1, r0
	and #0x7F, r0

	mov.b r0, @(GBR_OFFS_N163_ADDR, GBR)
	nop
//
//	r2: (n163_addr << 8) | data
//
	mov.l @(GBR_OFFS_TIMESTAMP_BASE, GBR), r0
	mov REG_CYC_CNT, r3
	
	mov.l exchip_rb_ptr, r1
	add r0, r3

	mov.w @(GBR_OFFS_EXCHIP_RB_WR, GBR), r0
	nop

	extu.w r0, r0
	add r0, r1

	mov.w r3, @r1
	add #2, r1

	mov.w r2, @r1
	add #2, r1

	mov r1, r0
	rts
	mov.w r0, @(GBR_OFFS_EXCHIP_RB_WR, GBR)

.align 2
exchip_rb_ptr: .long _exchip_rb

.align 2
_mmc5_write_mult0_func_S:
	mov.w @(GBR_OFFS_MMC5_MASTER_MULT1, GBR), r0
	sts MACL, r2

	mov r0, r1
	extu.b REG_DATA_BUS, r0

	mov.b r0, @(GBR_OFFS_MMC5_MASTER_MULT0 + 1, GBR)
	muls.w r0, r1

	sts MACL, r0
	lds r2, MACL

	rts
	mov.w r0, @(GBR_OFFS_MMC5_MASTER_MULT_RES, GBR)

_mmc5_write_mult1_func_S:
	mov.w @(GBR_OFFS_MMC5_MASTER_MULT0, GBR), r0
	sts MACL, r2

	mov r0, r1
	extu.b REG_DATA_BUS, r0

	mov.b r0, @(GBR_OFFS_MMC5_MASTER_MULT1 + 1, GBR)
	muls.w r0, r1

	sts MACL, r0
	lds r2, MACL

	rts
	mov.w r0, @(GBR_OFFS_MMC5_MASTER_MULT_RES, GBR)

.align 2
mmc5_update_shim:
	//
	// Save S6502 registers:
	//
	mov.l @(s_abi_spill_bottom_addr - gbr_area, GBR), r0
	mov.l REG_CYC_CNT, @-r0

	mov.l REG_PC_BASE, @-r0
	mov.l REG_ADDR, @-r0

	sts.l pr, @-r0
	mov.l REG_DATA_BUS, @-r0
	//
	//
	//
	mov.l @(sp_save - gbr_area, GBR), r0
	nop
	//
	mov.l 1f, r1
	mov r0, r15
	//
	//
	mov.l @(GBR_OFFS_TIMESTAMP_BASE, GBR), r0
	extu.b REG_DATA_BUS, REG_DATA_BUS

	jsr @r1
	add r0, r4
	//
	// Restore S6502 registers:
	//
	mov.l 5f, r2
	mov.l 6f, r0

	lds.l @r2+, SREG_WFAP
	mov r0, REG_RAM

	lds.l @r2+, SREG_RFAP
	mov.l @r2+, REG_DATA_BUS

	lds.l @r2+, pr
	mov.l @r2+, REG_ADDR

	mov.l @r2+, REG_PC_BASE
	rts
	mov.l @r2+, REG_CYC_CNT
.align 2
1: .long _mmc5_master_update_counters
5: .long s_abi_spill_partial_top
6: .long _ram

.macro CHECK_UPDATE_MMC5_FRAME
	mov.l @(GBR_OFFS_TIMESTAMP_BASE, GBR), r0
	mov REG_CYC_CNT, r3

	add r0, r3
	mov.l @(GBR_OFFS_MMC5_MASTER_FRAME_DIVIDER, GBR), r0

	cmp/ge r0, r3
	bf/s 1f
	//
	//
	mova mmc5_pr_spill+4, r0
	sts.l pr, @-r0
	//
	//

	bsr mmc5_update_shim
	nop


	//
	//
	mova mmc5_pr_spill, r0
	lds.l @r0+, pr
	//
	//
	1:
.endm


.align 2
_mmc5_write0_func_S:
	CHECK_UPDATE_MMC5_FRAME
	//
	//
	//
	mov REG_DATA_BUS, r0
	bra _mmc5_writeX_func_S
	mov.b r0, @(GBR_OFFS_MMC5_MASTER_ENVMODE0, GBR)

_mmc5_write4_func_S:
	CHECK_UPDATE_MMC5_FRAME
	//
	//
	//
	mov REG_DATA_BUS, r0
	bra _mmc5_writeX_func_S
	mov.b r0, @(GBR_OFFS_MMC5_MASTER_ENVMODE1, GBR)

_mmc5_write3_func_S:
	CHECK_UPDATE_MMC5_FRAME
	//
	//
	//
	mov.b @(GBR_OFFS_MMC5_MASTER_LC_ENABLE, GBR), r0
	nop

	tst #0x01, r0
	bt/s _mmc5_writeX_func_S

	mov.b @(GBR_OFFS_MMC5_MASTER_STATUS, GBR), r0
	extu.b REG_DATA_BUS, r1

	or #0x01, r0
	shlr2 r1

	mov.b r0, @(GBR_OFFS_MMC5_MASTER_STATUS, GBR)
	shlr r1
	//
	//
	mova mmc5_length_table, r0
	nop

	mov.b @(r0, r1), r0
	bra _mmc5_writeX_func_S
	mov.b r0, @(GBR_OFFS_MMC5_MASTER_LENGTH_COUNTER0, GBR)

_mmc5_write7_func_S:
	CHECK_UPDATE_MMC5_FRAME
	//
	//
	//
	mov.b @(GBR_OFFS_MMC5_MASTER_LC_ENABLE, GBR), r0
	nop

	tst #0x02, r0
	bt/s _mmc5_writeX_func_S

	mov.b @(GBR_OFFS_MMC5_MASTER_STATUS, GBR), r0
	extu.b REG_DATA_BUS, r1

	or #0x02, r0
	shlr2 r1

	mov.b r0, @(GBR_OFFS_MMC5_MASTER_STATUS, GBR)
	shlr r1
	//
	//
	mova mmc5_length_table, r0
	nop

	mov.b @(r0, r1), r0
	bra _mmc5_writeX_func_S
	mov.b r0, @(GBR_OFFS_MMC5_MASTER_LENGTH_COUNTER1, GBR)

.align 2
mmc5_length_table:
	.byte 0x0A, 0xFE, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06, 0xA0, 0x08, 0x3C, 0x0A, 0x0E, 0x0C, 0x1A, 0x0E, 0x0C, 0x10, 0x18, 0x12, 0x30, 0x14, 0x60, 0x16, 0xC0, 0x18, 0x48, 0x1A, 0x10, 0x1C, 0x20, 0x1E

.align 2
_mmc5_writeX_func_S:
	mov.l @(GBR_OFFS_TIMESTAMP_BASE, GBR), r0
	mov REG_CYC_CNT, r3

	mov.l 1f, r1 //exchip_rb_ptr, r1
	add r0, r3

	mov.w @(GBR_OFFS_EXCHIP_RB_WR, GBR), r0
	extu.b REG_ADDR, r2

	extu.w r0, r0
	add r0, r1

	mov.w r3, @r1
	add #2, r1
	//
	swap.b r2, r2
	extu.b REG_DATA_BUS, r0

	or r0, r2
	nop

	mov.w r2, @r1
	add #2, r1

	mov r1, r0
	rts
	mov.w r0, @(GBR_OFFS_EXCHIP_RB_WR, GBR)

.align 2
1: .long _exchip_rb

.align 2
_mmc5_write_status_func_S:
	CHECK_UPDATE_MMC5_FRAME
	//
	//
	//
	mov REG_DATA_BUS, r0
	nop

	mov.b r0, @(GBR_OFFS_MMC5_MASTER_LC_ENABLE, GBR)
	nop

	mov.b @(GBR_OFFS_MMC5_MASTER_STATUS, GBR), r0
	nop

	and REG_DATA_BUS, r0
	bra _mmc5_writeX_func_S

	mov.b r0, @(GBR_OFFS_MMC5_MASTER_STATUS, GBR)

//
//
//
//
//
//
//
//
//
.align 8
memhandler_region_start:

.align 2
_mmc5_read_mult_res0_func_S:
	mov.b @(GBR_OFFS_MMC5_MASTER_MULT_RES + 1, GBR), R0
	rts
	mov r0, REG_DATA_BUS

.align 2
_mmc5_read_mult_res1_func_S:
	mov.b @(GBR_OFFS_MMC5_MASTER_MULT_RES + 0, GBR), R0
	rts
	mov r0, REG_DATA_BUS

.align 2
_mmc5_read_status_func_S:
	CHECK_UPDATE_MMC5_FRAME
	//
	//
	//
	mov.b @(GBR_OFFS_MMC5_MASTER_STATUS, GBR), r0
	rts
	mov r0, REG_DATA_BUS

.align 2
mmc5_pr_spill: .long 0

_n163_read_4800_func_S:
	mov.b @(GBR_OFFS_N163_ADDR, GBR), r0
	nop

	mov r0, r1
	nop

	mov.l @(GBR_OFFS_N163_RAM_PTR, GBR), r0
	nop

	mov.b @(r0, r1), REG_DATA_BUS
	nop

	mov.b @(GBR_OFFS_N163_ADDR_INC, GBR), r0
	nop

	add r1, r0
	and #0x7F, r0

	mov.b r0, @(GBR_OFFS_N163_ADDR, GBR)
	rts
	nop

.align 4
s6502_read_shim:
	//
	// Save S6502 registers:
	//
	mov.l @(s_abi_spill_bottom_addr - gbr_area, GBR), r0
	mov.l REG_CYC_CNT, @-r0

	mov.l REG_PC_BASE, @-r0
	mov.l REG_ADDR, @-r0

	sts.l pr, @-r0
	add #-4, r0	// skip REG_DATA_BUS
	//
	//
	//
	mov.l @(sp_save - gbr_area, GBR), r0
	extu.w REG_ADDR, REG_ADDR
	//
	//
	mov.l 1f, r1
	mov r0, r15

	mov.l @(GBR_OFFS_TIMESTAMP_BASE, GBR), r0
	nop

	jsr @r1
	add r0, r4

	//
	// Restore S6502 registers:
	//
	mov.l 5f, r2
	mov r0, REG_DATA_BUS

	lds.l @r2+, SREG_WFAP
	mova _ram, r0

	lds.l @r2+, SREG_RFAP
	add #4, r2	// Skip REG_DATA_BUS

	lds.l @r2+, pr
	mov r0, REG_RAM

	mov.l @r2+, REG_ADDR
	mov.l @r2+, REG_PC_BASE

	rts
	mov.l @r2+, REG_CYC_CNT
.align 2
1: .long _apu_read_4015
5: .long s_abi_spill_partial_top

.align 4
_s6502_write_shim_S:
	//
	// Save S6502 registers:
	//
	mov.l @(s_abi_spill_bottom_addr - gbr_area, GBR), r0
	mov.l REG_CYC_CNT, @-r0

	mov.l REG_PC_BASE, @-r0
	mov.l REG_ADDR, @-r0

	sts.l pr, @-r0
	mov.l REG_DATA_BUS, @-r0
	//
	//
	//
	mov.l @(sp_save - gbr_area, GBR), r0
	extu.w REG_ADDR, REG_ADDR
	//
	mov.l 1f, r1
	mov r0, r15
	//
	//
	mov.l @(GBR_OFFS_TIMESTAMP_BASE, GBR), r0
	extu.b REG_DATA_BUS, REG_DATA_BUS

	jsr @r1
	add r0, r4
	//
	// Restore S6502 registers:
	//
	mov.l 5f, r2
	mova _ram, r0

	lds.l @r2+, SREG_WFAP
	mov r0, REG_RAM

	lds.l @r2+, SREG_RFAP
	mov.l @r2+, REG_DATA_BUS

	lds.l @r2+, pr
	mov.l @r2+, REG_ADDR

	mov.l @r2+, REG_PC_BASE
	rts
	mov.l @r2+, REG_CYC_CNT
.align 2
1: .long _apu_write
5: .long s_abi_spill_partial_top
//
.align 2
_nsf_read_frame_pending_S:
	mov.b @(GBR_OFFS_NSF_FRAME_PENDING, GBR), r0
	mov r0, REG_DATA_BUS
	mov #0, r0
	rts
	mov.b r0, @(GBR_OFFS_NSF_FRAME_PENDING, GBR)

.align 2
_nsf_set_bank_S:
	extu.b REG_DATA_BUS, r2
	mov REG_ADDR, r0

	and #0x0F, r0
	sub r0, r2

	shll8 r2
	shll2 r2

	shll2 r2
	mov r0, r1

	mov.l @(GBR_OFFS_NSF_ROM_BASE, GBR), r0
	add r0, r2

	stc GBR, r0
	shll2 r1

	rts
	mov.l r2, @(r0, r1)

.align 2
_ram_mirror_write_func_S:
	mov.w ram_addr_mask, r1
	mov REG_ADDR, r0

	and r1, r0
	rts
	mov.b REG_DATA_BUS, @(r0, REG_RAM)
//
.align 4
_ram_mirror_read_func_S:
	mov.w ram_addr_mask, r1
	mov REG_ADDR, r0

	and r1, r0
	rts
	mov.b @(r0, REG_RAM), REG_DATA_BUS
ram_addr_mask: .short 0x07FF
//
.align 2
_ob_rw_func_S:
	rts
	nop
//
.align 4
_apu_read_4015_S:
	mov.l @(GBR_OFFS_TIMESTAMP_BASE, GBR), r0
	mov REG_CYC_CNT, r1

	add r0, r1
	nop

	mov.l @(GBR_OFFS_FC_NEXT_TIMESTAMP, GBR), r0
	nop

	cmp/hs r0, r1
	bt/s s6502_read_shim

	mov.l @(GBR_OFFS_DMC_END_TIMESTAMP, GBR), r0
	nop

	cmp/hs r0, r1
	bt/s s6502_read_shim

	mov.l @(GBR_OFFS_APU_STATUS, GBR), r0
	nop

	mov r0, REG_DATA_BUS
	and #~0x40, r0

	mov.l r0, @(GBR_OFFS_APU_STATUS, GBR)
	rts
	nop
//
//
.align 4
_ram_read_func_S:
	mov REG_ADDR, r0
	rts
	mov.b @(r0, REG_RAM), REG_DATA_BUS
//
.align 2
_ram_write_func_S:
	mov REG_ADDR, r0
	rts
	mov.b REG_DATA_BUS, @(r0, REG_RAM)
//
.align 2
_bs2xxx_read_func_S:
	mov.l @(0x2 * 4, GBR), r0
	rts
	mov.b @(r0, REG_ADDR), REG_DATA_BUS
//
.align 2
_bs6xxx_read_func_S:
	mov.l @(0x6 * 4, GBR), r0
	rts
	mov.b @(r0, REG_ADDR), REG_DATA_BUS
//
.align 2
_bs7xxx_read_func_S:
	mov.l @(0x7 * 4, GBR), r0
	rts
	mov.b @(r0, REG_ADDR), REG_DATA_BUS
//
.align 2
_bs6xxx_write_func_S:
	mov.l @(0x6 * 4, GBR), r0
	rts
	mov.b REG_DATA_BUS, @(r0, REG_ADDR)
//
.align 2
_bs7xxx_write_func_S:
	mov.l @(0x7 * 4, GBR), r0
	rts
	mov.b REG_DATA_BUS, @(r0, REG_ADDR)
//
.align 2
_bs8xxx_read_func_S:
	mov.l @(0x8 * 4, GBR), r0
	rts
	mov.b @(r0, REG_ADDR), REG_DATA_BUS
//
.align 2
_bs9xxx_read_func_S:
	mov.l @(0x9 * 4, GBR), r0
	rts
	mov.b @(r0, REG_ADDR), REG_DATA_BUS
//
.align 2
_bsAxxx_read_func_S:
	mov.l @(0xA * 4, GBR), r0
	rts
	mov.b @(r0, REG_ADDR), REG_DATA_BUS
//
.align 2
_bsBxxx_read_func_S:
	mov.l @(0xB * 4, GBR), r0
	rts
	mov.b @(r0, REG_ADDR), REG_DATA_BUS
//
.align 2
_bsCxxx_read_func_S:
	mov.l @(0xC * 4, GBR), r0
	rts
	mov.b @(r0, REG_ADDR), REG_DATA_BUS
//
.align 2
_bsDxxx_read_func_S:
	mov.l @(0xD * 4, GBR), r0
	rts
	mov.b @(r0, REG_ADDR), REG_DATA_BUS
//
.align 2
_bsExxx_read_func_S:
	mov.l @(0xE * 4, GBR), r0
	rts
	mov.b @(r0, REG_ADDR), REG_DATA_BUS
//
.align 2
_bsFxxx_read_func_S:
	mov.l @(0xF * 4, GBR), r0
	rts
	mov.b @(r0, REG_ADDR), REG_DATA_BUS
.align 8
_ram:
	.dcb.b 0x800
//
//
//
// Begin GBR area
//
//
//
.align 2
gbr_area:
_banks_4k:
bank0: .long 0
bank1: .long 0
bank2: .long 0
bank3: .long 0
bank4: .long 0
bank5: .long 0
bank6: .long 0
bank7: .long 0
bank8: .long 0
bank9: .long 0
bankA: .long 0
bankB: .long 0
bankC: .long 0
bankD: .long 0
bankE: .long 0
bankF: .long 0

ASSERT (bank0 - gbr_area) == GBR_OFFS_BANK0
ASSERT (bank1 - gbr_area) == GBR_OFFS_BANK1
ASSERT (bank2 - gbr_area) == GBR_OFFS_BANK2
ASSERT (bank3 - gbr_area) == GBR_OFFS_BANK3
ASSERT (bank4 - gbr_area) == GBR_OFFS_BANK4
ASSERT (bank5 - gbr_area) == GBR_OFFS_BANK5
ASSERT (bank6 - gbr_area) == GBR_OFFS_BANK6
ASSERT (bank7 - gbr_area) == GBR_OFFS_BANK7
ASSERT (bank8 - gbr_area) == GBR_OFFS_BANK8
ASSERT (bank9 - gbr_area) == GBR_OFFS_BANK9
ASSERT (bankA - gbr_area) == GBR_OFFS_BANKA
ASSERT (bankB - gbr_area) == GBR_OFFS_BANKB
ASSERT (bankC - gbr_area) == GBR_OFFS_BANKC
ASSERT (bankD - gbr_area) == GBR_OFFS_BANKD
ASSERT (bankE - gbr_area) == GBR_OFFS_BANKE
ASSERT (bankF - gbr_area) == GBR_OFFS_BANKF

_timestamp_base: .long 0
_fc_next_timestamp: .long 0
.global _dmc_end_timestamp
_dmc_end_timestamp: .long 0
_apu_status: .long 0

ASSERT (_timestamp_base - gbr_area) == GBR_OFFS_TIMESTAMP_BASE
ASSERT (_fc_next_timestamp - gbr_area) == GBR_OFFS_FC_NEXT_TIMESTAMP
ASSERT (_dmc_end_timestamp - gbr_area) == GBR_OFFS_DMC_END_TIMESTAMP
ASSERT (_apu_status - gbr_area) == GBR_OFFS_APU_STATUS

event_handler_s: .long 0
sp_save: .long 0	// C ABI r15/stack pointer

.global _nsf_rom_base
.global _nsf_frame_pending

_nsf_rom_base: .long 0
_nsf_frame_pending: .byte 0
.byte 0

.global _exchip_rb_wr
_exchip_rb_wr: .word 0

ASSERT (_nsf_rom_base - gbr_area) == GBR_OFFS_NSF_ROM_BASE
ASSERT (_nsf_frame_pending - gbr_area) == GBR_OFFS_NSF_FRAME_PENDING
ASSERT (_exchip_rb_wr - gbr_area) == GBR_OFFS_EXCHIP_RB_WR

_n163_ram_ptr: .long 0
_n163_addr: .byte 0
_n163_addr_inc: .byte 0

ASSERT (_n163_ram_ptr - gbr_area) == GBR_OFFS_N163_RAM_PTR
ASSERT (_n163_addr - gbr_area) == GBR_OFFS_N163_ADDR
ASSERT (_n163_addr_inc - gbr_area) == GBR_OFFS_N163_ADDR_INC

_sun5b_addr: .byte 0

ASSERT (_sun5b_addr - gbr_area) == GBR_OFFS_SUN5B_ADDR

.dcb.b (GBR_OFFS_MMC5_MASTER_FRAME_DIVIDER - (GBR_OFFS_SUN5B_ADDR + 1))

.global _mmc5_master
_mmc5_master:
_mmc5_master_frame_divider: .long 0
_mmc5_master_envmode0: .byte 0
_mmc5_master_envmode1: .byte 0
_mmc5_master_length_counter0: .byte 0
_mmc5_master_length_counter1: .byte 0
_mmc5_master_status: .byte 0
_mmc5_master_lc_enable: .byte 0
_mmc5_master_mult0: .word 0
_mmc5_master_mult1: .word 0
_mmc5_master_mult_res:	.word 0

ASSERT (_mmc5_master - gbr_area) == GBR_OFFS_MMC5_MASTER
ASSERT (_mmc5_master_frame_divider - gbr_area) == GBR_OFFS_MMC5_MASTER_FRAME_DIVIDER
ASSERT (_mmc5_master_envmode0 - gbr_area) == GBR_OFFS_MMC5_MASTER_ENVMODE0
ASSERT (_mmc5_master_envmode1 - gbr_area) == GBR_OFFS_MMC5_MASTER_ENVMODE1
ASSERT (_mmc5_master_length_counter0 - gbr_area) == GBR_OFFS_MMC5_MASTER_LENGTH_COUNTER0
ASSERT (_mmc5_master_length_counter1 - gbr_area) == GBR_OFFS_MMC5_MASTER_LENGTH_COUNTER1
ASSERT (_mmc5_master_status - gbr_area) == GBR_OFFS_MMC5_MASTER_STATUS
ASSERT (_mmc5_master_lc_enable - gbr_area) == GBR_OFFS_MMC5_MASTER_LC_ENABLE
ASSERT (_mmc5_master_mult0 - gbr_area) == GBR_OFFS_MMC5_MASTER_MULT0
ASSERT (_mmc5_master_mult1 - gbr_area) == GBR_OFFS_MMC5_MASTER_MULT1
ASSERT (_mmc5_master_mult_res - gbr_area) == GBR_OFFS_MMC5_MASTER_MULT_RES

.align 2
s_abi_spill_top_addr: .long s_abi_spill_top
s_abi_spill_bottom_addr: .long s_abi_spill_bottom

// If changing this spill area, be sure to update structure in s6502.h
.global _s6502
_s6502:
s_abi_spill_top:

s_abi_spill_Y: .long 0
s_abi_spill_X: .long 0
s_abi_spill_A: .long 0
s_abi_spill_P: .long 0
s_abi_spill_SP: .long 0
s_abi_spill_PC: .long 0
s_abi_spill_NZ: .long 0
s_abi_spill_RAM: .long 0
//
//
s_abi_spill_partial_top:

s_abi_spill_WFAP: .long 0
s_abi_spill_RFAP: .long 0
//
s_abi_spill_DATA_BUS: .long 0
s_abi_spill_PR: .long 0
s_abi_spill_ADDR: .long 0
s_abi_spill_PC_BASE: .long 0
s_abi_spill_CYC_CNT: .long 0
s_abi_spill_bottom:
//
//
//
// End GBR area
//
//
//




//
// Avoid using REG_DATA_BUS in the first instruction of OP_ADC, OP_SBC, OP_CMP, etc.
// if possible, to avoid a 1-cycle stall with some addressing mode combinations.
//

.macro OP_ADC
	exts.b REG_A, REG_A
	exts.b REG_DATA_BUS, REG_DATA_BUS
	rotcr REG_P
	addc REG_DATA_BUS, REG_A
	rotcl REG_P

	exts.b REG_A, REG_NZ
	cmp/eq REG_A, REG_NZ
	extu.b REG_A, REG_A

	mov #~FLAG_V, r0
	and r0, REG_P
	movt r0
	add #-1, r0
	and #FLAG_V, r0
	bra s6502_execute
	or r0, REG_P
.endm

.macro OP_SBC
	exts.b REG_A, REG_A
	exts.b REG_DATA_BUS, REG_DATA_BUS
	not REG_P, r0
	rotcr r0
	subc REG_DATA_BUS, REG_A
	rotcl r0
	not r0, REG_P

	exts.b REG_A, REG_NZ
	cmp/eq REG_A, REG_NZ
	extu.b REG_A, REG_A

	mov #~FLAG_V, r0
	and r0, REG_P
	movt r0
	add #-1, r0
	and #FLAG_V, r0
	bra s6502_execute
	or r0, REG_P
.endm

.macro OP_CMP_i i
	extu.b \i, r1
	extu.b REG_DATA_BUS, REG_DATA_BUS

	not REG_P, r0
	rotcr r0
	clrt
	subc REG_DATA_BUS, r1
	exts.b r1, REG_NZ
	rotcl r0
	bra s6502_execute
	not r0, REG_P
.endm

.macro OP_CMP
	OP_CMP_i REG_A
.endm

.macro OP_CPX
	OP_CMP_i REG_X
.endm

.macro OP_CPY
	OP_CMP_i REG_Y
.endm

.macro OP_BIT
	mov #~FLAG_V, r2
	mov REG_DATA_BUS, r0

	and #FLAG_V, r0
	and r2, REG_P

	or r0, REG_P
	exts.b REG_DATA_BUS, REG_NZ 

	and REG_A, REG_DATA_BUS
	shll8 REG_NZ

	bra s6502_execute
	or REG_DATA_BUS, REG_NZ
.endm

.macro OP_AND
	and REG_DATA_BUS, REG_A
	bra s6502_execute
	exts.b REG_A, REG_NZ
.endm

.macro OP_EOR
	extu.b REG_DATA_BUS, REG_DATA_BUS
	xor REG_DATA_BUS, REG_A
	bra s6502_execute
	exts.b REG_A, REG_NZ
.endm

.macro OP_ORA
	extu.b REG_DATA_BUS, REG_DATA_BUS
	or REG_DATA_BUS, REG_A
	bra s6502_execute
	exts.b REG_A, REG_NZ
.endm

//
// Don't clobber any of r0-r4 except r3 in RMW OP_ASL, OP_LSR, OP_ROL, OP_ROR, OP_DEC, and OP_INC
//

.macro OP_ASL
	shlr REG_P
	exts.b REG_DATA_BUS, REG_DATA_BUS

	shll REG_DATA_BUS
	rotcl REG_P	

	exts.b REG_DATA_BUS, REG_NZ
	extu.b REG_DATA_BUS, REG_DATA_BUS
.endm

.macro OP_LSR
	shlr REG_P
	extu.b REG_DATA_BUS, REG_DATA_BUS

	shlr REG_DATA_BUS
	rotcl REG_P	

	exts.b REG_DATA_BUS, REG_NZ
	nop
.endm

.macro OP_ROL
	shlr REG_P
	exts.b REG_DATA_BUS, REG_DATA_BUS

	rotcl REG_DATA_BUS
	rotcl REG_P	

	exts.b REG_DATA_BUS, REG_NZ
	extu.b REG_DATA_BUS, REG_DATA_BUS
.endm

.macro OP_ROR
	shlr REG_P
	extu.b REG_DATA_BUS, REG_DATA_BUS

	movt r3
	shll8 r3

	or r3, REG_DATA_BUS
	shlr REG_DATA_BUS

	rotcl REG_P
	exts.b REG_DATA_BUS, REG_NZ
.endm

.macro OP_DEC
	add #-1, REG_DATA_BUS
	exts.b REG_DATA_BUS, REG_NZ
.endm

.macro OP_INC
	add #1, REG_DATA_BUS
	exts.b REG_DATA_BUS, REG_NZ
.endm
//
//
.macro OP_ASL_A
	shlr REG_P
	exts.b REG_A, REG_A

	shll REG_A
	rotcl REG_P	

	exts.b REG_A, REG_NZ
	bra s6502_execute
	extu.b REG_A, REG_A
.endm

.macro OP_LSR_A
	shlr REG_P
	shlr REG_A

	rotcl REG_P	
	bra s6502_execute
	exts.b REG_A, REG_NZ
.endm

.macro OP_ROL_A
	shlr REG_P
	exts.b REG_A, REG_A

	rotcl REG_A
	rotcl REG_P	

	exts.b REG_A, REG_NZ
	bra s6502_execute
	extu.b REG_A, REG_A
.endm

.macro OP_ROR_A
	shlr REG_P
	movt r3

	shll8 r3
	or r3, REG_A

	shlr REG_A
	rotcl REG_P

	bra s6502_execute
	exts.b REG_A, REG_NZ
.endm

.macro OP_LDA
	exts.b REG_DATA_BUS, REG_NZ
	bra s6502_execute
	extu.b REG_DATA_BUS, REG_A
.endm

.macro OP_LDX
	exts.b REG_DATA_BUS, REG_NZ
	bra s6502_execute
	extu.b REG_DATA_BUS, REG_X
.endm

.macro OP_LDY
	exts.b REG_DATA_BUS, REG_NZ
	bra s6502_execute
	extu.b REG_DATA_BUS, REG_Y
.endm

//
//
//

.macro RD_ZP
	mov.b @REG_PC+, r0
	add #1, REG_CYC_CNT

	extu.b r0, r0
	nop

	mov.b @(r0,REG_RAM), REG_DATA_BUS
.endm

.macro WR_ZP sdreg
	mov.b @REG_PC+, r0
	add #1, REG_CYC_CNT

	extu.b r0, r0
	bra s6502_execute
	mov.b \sdreg, @(r0,REG_RAM)
.endm

.macro RD_ZP_i ireg
	mov.b @REG_PC+, r0
	add #2, REG_CYC_CNT

	add \ireg, r0
	extu.b r0, r0

	mov.b @(r0,REG_RAM), REG_DATA_BUS
.endm

.macro WR_ZP_i ireg sdreg
	mov.b @REG_PC+, r0
	add #2, REG_CYC_CNT

	add \ireg, r0
	extu.b r0, r0

	mov.b \sdreg, @(r0,REG_RAM)
	bra s6502_execute
	nop
.endm

.macro RD_ZP_X
	RD_ZP_i REG_X
.endm

.macro RD_ZP_Y
	RD_ZP_i REG_Y
.endm

.macro WR_ZP_X sdreg
	WR_ZP_i REG_X \sdreg
.endm

.macro WR_ZP_Y sdreg
	WR_ZP_i REG_Y \sdreg
.endm

//
// TODO: optimize RMW_ZP*/op combinations
//
.macro RMW_ZP op
	mov.b @REG_PC+, r0
	add #1, REG_CYC_CNT

	nop
	extu.b r0, r0

	mov.b @(r0,REG_RAM), REG_DATA_BUS
	nop

	\op

	nop
	bra s6502_execute
	mov.b REG_DATA_BUS, @(r0,REG_RAM)
.endm

.macro RMW_ZP_X op
	mov.b @REG_PC+, r0
	add #2, REG_CYC_CNT

	add REG_X, r0
	extu.b r0, r0

	mov.b @(r0,REG_RAM), REG_DATA_BUS
	nop

	\op

	nop
	bra s6502_execute
	mov.b REG_DATA_BUS, @(r0,REG_RAM)
.endm

.macro RMW_ZP_INC op
	mov.b @REG_PC+, r0
	add #1, REG_CYC_CNT

	nop
	extu.b r0, r0

	mov.b @(r0,REG_RAM), REG_DATA_BUS
	add #1, REG_DATA_BUS

	exts.b REG_DATA_BUS, REG_NZ
	bra s6502_execute

	mov.b REG_DATA_BUS, @(r0,REG_RAM)
.endm

.macro RMW_ZP_DEC op
	mov.b @REG_PC+, r0
	add #1, REG_CYC_CNT

	nop
	extu.b r0, r0

	mov.b @(r0,REG_RAM), REG_DATA_BUS
	add #-1, REG_DATA_BUS

	exts.b REG_DATA_BUS, REG_NZ
	bra s6502_execute

	mov.b REG_DATA_BUS, @(r0,REG_RAM)
.endm

.macro RMW_ZP_X_INC op
	mov.b @REG_PC+, r0
	add #1, REG_CYC_CNT

	add REG_X, r0
	extu.b r0, r0

	mov.b @(r0,REG_RAM), REG_DATA_BUS
	add #1, REG_DATA_BUS

	exts.b REG_DATA_BUS, REG_NZ
	bra s6502_execute

	mov.b REG_DATA_BUS, @(r0,REG_RAM)
.endm

.macro RMW_ZP_X_DEC op
	mov.b @REG_PC+, r0
	add #1, REG_CYC_CNT

	add REG_X, r0
	extu.b r0, r0

	mov.b @(r0,REG_RAM), REG_DATA_BUS
	add #-1, REG_DATA_BUS

	exts.b REG_DATA_BUS, REG_NZ
	bra s6502_execute

	mov.b REG_DATA_BUS, @(r0,REG_RAM)
.endm


.macro RMW_ABS op
	mov.b @REG_PC+, r1
	add #2, REG_CYC_CNT

	mov.b @REG_PC+, REG_DATA_BUS
	extu.b r1, REG_ADDR

	sts SREG_RFAP, r0
	extu.b REG_DATA_BUS, r2

	shll8 r2
	add r2, REG_ADDR
	//
	//
	mov.b @(r0,REG_ADDR), r3	// Load read function pointer
	ldc REG_PC, CREG_SCRATCH	// BE EVIL YAAAY!

	sts SREG_WFAP, r0
	shll2 r3

	mov REG_ADDR, REG_PC
	shll2 REG_PC

	mov.l @(r0,REG_PC), REG_PC	// Load write function pointer
	add REG_RAM, r3

	jsr @r3				// Call read function
	nop
	//
	//
	jsr @REG_PC			// Call write function
	nop
	//
	//
	\op
	//
	//
	jsr @REG_PC			// Call write function
	stc CREG_SCRATCH, REG_PC	// END EVIL...FOR NOW!

	bra s6502_execute
	nop
.endm

.macro RMW_ABS_X op
	mov.b @REG_PC+, r1
	add #2, REG_CYC_CNT

	mov.b @REG_PC+, REG_DATA_BUS
	extu.b r1, r1

	sts SREG_RFAP, r0
	extu.b REG_DATA_BUS, r2

	shll8 r2
	add REG_X, r1

	extu.b r1, REG_ADDR
	add r2, REG_ADDR

	add r2, r1
	extu.w r1, r1
	//
	//
	mov.b @(r0,REG_ADDR), r3	// Load read function pointer
	nop

	shll2 r3
	add REG_RAM, r3

	jsr @r3				// Call read function
	ldc r1, CREG_SCRATCH
	//
	//
	sts SREG_RFAP, r0
	stc CREG_SCRATCH, REG_ADDR

	mov.b @(r0,REG_ADDR), r3	// Load read function pointer
	ldc REG_PC, CREG_SCRATCH	// RETURN OF EVIL: EVIL BEYOND EVIL

	sts SREG_WFAP, r0
	shll2 r3

	mov REG_ADDR, REG_PC
	shll2 REG_PC

	mov.l @(r0,REG_PC), REG_PC	// Load write function pointer
	add REG_RAM, r3

	jsr @r3				// Call read function
	add #1, REG_CYC_CNT
	//
	//
	jsr @REG_PC			// Call write function
	add #1, REG_CYC_CNT

	\op

	jsr @REG_PC			// Call write function
	add #1, REG_CYC_CNT

	bra s6502_execute
	stc CREG_SCRATCH, REG_PC	// EVIL DEFEATED AGAIN!
.endm
//
//
.macro RD_ABS
	mov.b @REG_PC+, r1
	add #2, REG_CYC_CNT

	mov.b @REG_PC+, REG_DATA_BUS
	extu.b r1, REG_ADDR

	sts SREG_RFAP, r0
	extu.b REG_DATA_BUS, r2

	shll8 r2
	add r2, REG_ADDR
	//
	//
	mov.b @(r0,REG_ADDR), r2	// Load read function pointer
	nop

	shll2 r2
	add REG_RAM, r2

	jsr @r2			// Call read function
	nop
.endm

.macro RD_ABS_OP_BIT
	mov.b @REG_PC+, r1
	add #2, REG_CYC_CNT

	mov.b @REG_PC+, REG_DATA_BUS
	extu.b r1, REG_ADDR

	sts SREG_RFAP, r0
	extu.b REG_DATA_BUS, r2

	shll8 r2
	add r2, REG_ADDR
	//
	//
	mov.b @(r0,REG_ADDR), r2	// Load read function pointer
	mov #~FLAG_V, r3	// OP_BIT

	shll2 r2
	add REG_RAM, r2

	jsr @r2			// Call read function
	and r3, REG_P		// OP_BIT
	//
	// OP_BIT:
	//
	mov REG_DATA_BUS, r0
	and #FLAG_V, r0

	or r0, REG_P
	exts.b REG_DATA_BUS, REG_NZ

	and REG_A, REG_DATA_BUS
	shll8 REG_NZ

	bra s6502_execute
	or REG_DATA_BUS, REG_NZ
.endm


.macro WR_ABS sdreg
	mov.b @REG_PC+, r1
	add #2, REG_CYC_CNT

	mov.b @REG_PC+, REG_DATA_BUS
	extu.b r1, REG_ADDR

	sts SREG_WFAP, r0
	extu.b REG_DATA_BUS, r2

	shll8 r2
	add r2, REG_ADDR
	//
	//
	mov REG_ADDR, r2
	shll2 r2

	mov.l @(r0,r2), r2	// Load write function pointer
	nop

	jsr @r2			// Call write function
	mov \sdreg, REG_DATA_BUS

	bra s6502_execute
	nop
.endm

.macro RD_ABS_i i
	mov.b @REG_PC+, r1
	mov \i, REG_ADDR

	mov.b @REG_PC+, REG_DATA_BUS
	extu.b r1, r1

	extu.b REG_DATA_BUS, r2
	add r1, REG_ADDR

	sts SREG_RFAP, r0
	extu.b REG_ADDR, REG_ADDR

	shll8 r2
	cmp/hs \i, REG_ADDR

	bt/s 1f
	add r2, REG_ADDR

	mov.b @(r0,REG_ADDR), r2	// Load read function pointer
	nop

	shll2 r2
	add REG_RAM, r2

	jsr @r2			// Call read function
	add #2, REG_CYC_CNT

	mov #1, r0
	swap.b r0, r0

	add r0, REG_ADDR
	extu.w REG_ADDR, REG_ADDR

	sts SREG_RFAP, r0
	add #-1, REG_CYC_CNT
	1:

	mov.b @(r0,REG_ADDR), r2	// Load read function pointer
	nop

	shll2 r2
	add REG_RAM, r2

	jsr @r2			// Call read function
	add #2, REG_CYC_CNT
.endm

.macro RD_ABS_X
	RD_ABS_i REG_X
.endm

.macro RD_ABS_Y
	RD_ABS_i REG_Y
.endm

.macro WR_ABS_i i sdreg
	mov.b @REG_PC+, r1
	add #2, REG_CYC_CNT

	mov.b @REG_PC+, REG_DATA_BUS
	extu.b r1, r1

	sts SREG_RFAP, r0
	extu.b REG_DATA_BUS, r2

	shll8 r2
	add \i, r1

	extu.b r1, REG_ADDR
	add r2, REG_ADDR

	add r2, r1
	extu.w r1, r1
	//
	//
	mov.b @(r0,REG_ADDR), r3	// Load read function pointer
	nop

	shll2 r3
	add REG_RAM, r3

	jsr @r3			// Call read function
	ldc r1, CREG_SCRATCH
	//
	//
	sts SREG_WFAP, r0
	stc CREG_SCRATCH, REG_ADDR

	mov REG_ADDR, r2
	shll2 r2

	mov.l @(r0,r2), r2	// Load write function pointer
	add #1, REG_CYC_CNT

	jsr @r2			// Call write function
	mov \sdreg, REG_DATA_BUS

	bra s6502_execute
	nop
.endm

.macro WR_ABS_X sdreg
	WR_ABS_i REG_X \sdreg
.endm

.macro WR_ABS_Y sdreg
	WR_ABS_i REG_Y \sdreg
.endm


.macro RD_IND_X
	mov.b @REG_PC+, r0
	add #4, REG_CYC_CNT

	add REG_X, r0
	extu.b r0, r0

	mov.b @(r0,REG_RAM), REG_ADDR
	add #1, r0

	extu.b REG_ADDR, REG_ADDR
	extu.b r0, r0

	mov.b @(r0,REG_RAM), r2
	nop

	sts SREG_RFAP, r0
	extu.b r2, r2

	shll8 r2
	add r2, REG_ADDR

	mov.b @(r0,REG_ADDR), r2	// Load read function pointer
	nop

	shll2 r2
	add REG_RAM, r2

	jsr @r2			// Call read function
	nop
.endm

.macro WR_IND_X sdreg
	mov.b @REG_PC+, r0
	add #4, REG_CYC_CNT

	add REG_X, r0
	extu.b r0, r0

	mov.b @(r0,REG_RAM), REG_ADDR
	add #1, r0

	extu.b REG_ADDR, REG_ADDR
	extu.b r0, r0

	mov.b @(r0,REG_RAM), r2
	nop

	sts SREG_WFAP, r0
	extu.b r2, r2

	shll8 r2
	add r2, REG_ADDR

	mov REG_ADDR, r2
	shll2 r2

	mov.l @(r0,r2), r2	// Load write function pointer
	nop

	jsr @r2			// Call write function
	mov \sdreg, REG_DATA_BUS

	bra s6502_execute
	nop
.endm

.macro RD_IND_Y
	mov.b @REG_PC+, r0
	add #4, REG_CYC_CNT

	mov REG_Y, REG_ADDR
	extu.b r0, r0

	mov.b @(r0,REG_RAM), r1
	add #1, r0

	extu.b r1, r1
	extu.b r0, r0

	mov.b @(r0,REG_RAM), r2
	nop

	extu.b r2, r2
	add r1, REG_ADDR

	sts SREG_RFAP, r0
	extu.b REG_ADDR, REG_ADDR

	shll8 r2
	cmp/hs REG_Y, REG_ADDR

	bt/s 1f
	add r2, REG_ADDR

	mov.b @(r0,REG_ADDR), r2	// Load read function pointer
	nop

	shll2 r2
	add REG_RAM, r2

	jsr @r2			// Call read function
	nop

	add #1, REG_CYC_CNT
	mov #1, r0

	swap.b r0, r0
	add r0, REG_ADDR

	sts SREG_RFAP, r0
	extu.w REG_ADDR, REG_ADDR
	1:

	mov.b @(r0,REG_ADDR), r2	// Load read function pointer
	nop

	shll2 r2
	add REG_RAM, r2

	jsr @r2			// Call read function
	nop
.endm

.macro WR_IND_Y sdreg
	mov.b @REG_PC+, r0
	extu.b r0, r0		// Stall

	mov.b @(r0,REG_RAM), r1
	add #1, r0

	extu.b r1, r1
	extu.b r0, r0

	mov.b @(r0,REG_RAM), r2
	add REG_Y, r1

	sts SREG_RFAP, r0
	extu.b r1, REG_ADDR

	extu.b r2, r2
	shll8 r2

	add r2, REG_ADDR
	add r2, r1	

	extu.w r1, r1
	nop
	//
	//
	mov.b @(r0,REG_ADDR), r2	// Load read function pointer
	add #3, REG_CYC_CNT

	shll2 r2
	add REG_RAM, r2

	jsr @r2			// Call read function
	ldc r1, CREG_SCRATCH
	//
	//
	sts SREG_WFAP, r0
	stc CREG_SCRATCH, REG_ADDR

	mov REG_ADDR, r2
	shll2 r2

	mov.l @(r0,r2), r2	// Load write function pointer
	add #1, REG_CYC_CNT

	jsr @r2			// Call write function
	mov \sdreg, REG_DATA_BUS

	bra s6502_execute
	nop
.endm

.macro RD_IMM
	mov.b @REG_PC+, REG_DATA_BUS
.endm

//
//
//
//
//
//
//

#define CASE(n) .align 2 ; Op_##n

// ASL
CASE(0x0E): RMW_ABS   OP_ASL
CASE(0x1E): RMW_ABS_X OP_ASL

// DEC
CASE(0xCE): RMW_ABS   OP_DEC
CASE(0xDE): RMW_ABS_X OP_DEC

// INC
CASE(0xEE): RMW_ABS   OP_INC
CASE(0xFE): RMW_ABS_X OP_INC

// LSR
CASE(0x4E): RMW_ABS   OP_LSR
CASE(0x5E): RMW_ABS_X OP_LSR

// ROL
CASE(0x2E): RMW_ABS   OP_ROL
CASE(0x3E): RMW_ABS_X OP_ROL

// ROR
CASE(0x6E): RMW_ABS   OP_ROR
CASE(0x7E): RMW_ABS_X OP_ROR

// ADC
CASE(0x61): RD_IND_X ; OP_ADC
CASE(0x71): RD_IND_Y ; OP_ADC

// AND
CASE(0x21): RD_IND_X ; OP_AND
CASE(0x31): RD_IND_Y ; OP_AND

// CMP
CASE(0xC1): RD_IND_X ; OP_CMP
CASE(0xD1): RD_IND_Y ; OP_CMP

// EOR
CASE(0x41): RD_IND_X ; OP_EOR
CASE(0x51): RD_IND_Y ; OP_EOR

// LDA
CASE(0xA1): RD_IND_X ; OP_LDA
CASE(0xB1): RD_IND_Y ; OP_LDA

// ORA
CASE(0x01): RD_IND_X ; OP_ORA
CASE(0x11): RD_IND_Y ; OP_ORA

// SBC
CASE(0xE1): RD_IND_X ; OP_SBC
CASE(0xF1): RD_IND_Y ; OP_SBC

// STA
CASE(0x81): WR_IND_X REG_A
CASE(0x91): WR_IND_Y REG_A

// STA
CASE(0x8D): WR_ABS REG_A
CASE(0x9D): WR_ABS_X REG_A
CASE(0x99): WR_ABS_Y REG_A

// STX
CASE(0x8E): WR_ABS REG_X

// STY
CASE(0x8C): WR_ABS REG_Y

.align 4 // Align to 16-byte boundary
//.org (cpu_base + 0x1000 - 8)
s6502_call_event:
	mov.l @(event_handler_s - gbr_area, GBR), r0
	nop
	jmp @r0
	add #-1, REG_PC
//.org (cpu_base + 0x1000)
s6502_execute:
	mov.b @REG_PC+, r1
	mova op_table_middle, r0

	add r1, r1
	cmp/pz REG_CYC_CNT

	mov.w @(r0, r1), r3
	bt s6502_call_event

	braf r3
	add #2, REG_CYC_CNT
	1:
.align 2 // Align to 4-byte boundary
op_table:
	.short (Op_0xEA - 1b) //	.short (Op_0x80 - 1b)
	.short (Op_0x81 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x82 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x83 - 1b)
	.short (Op_0x84 - 1b)
	.short (Op_0x85 - 1b)
	.short (Op_0x86 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x87 - 1b)
	.short (Op_0x88 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x89 - 1b)
	.short (Op_0x8A - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x8B - 1b)
	.short (Op_0x8C - 1b)
	.short (Op_0x8D - 1b)
	.short (Op_0x8E - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x8F - 1b)
	.short (Op_0x90 - 1b)
	.short (Op_0x91 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x92 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x93 - 1b)
	.short (Op_0x94 - 1b)
	.short (Op_0x95 - 1b)
	.short (Op_0x96 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x97 - 1b)
	.short (Op_0x98 - 1b)
	.short (Op_0x99 - 1b)
	.short (Op_0x9A - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x9B - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x9C - 1b)
	.short (Op_0x9D - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x9E - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x9F - 1b)
	.short (Op_0xA0 - 1b)
	.short (Op_0xA1 - 1b)
	.short (Op_0xA2 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xA3 - 1b)
	.short (Op_0xA4 - 1b)
	.short (Op_0xA5 - 1b)
	.short (Op_0xA6 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xA7 - 1b)
	.short (Op_0xA8 - 1b)
	.short (Op_0xA9 - 1b)
	.short (Op_0xAA - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xAB - 1b)
	.short (Op_0xAC - 1b)
	.short (Op_0xAD - 1b)
	.short (Op_0xAE - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xAF - 1b)
	.short (Op_0xB0 - 1b)
	.short (Op_0xB1 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xB2 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xB3 - 1b)
	.short (Op_0xB4 - 1b)
	.short (Op_0xB5 - 1b)
	.short (Op_0xB6 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xB7 - 1b)
	.short (Op_0xB8 - 1b)
	.short (Op_0xB9 - 1b)
	.short (Op_0xBA - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xBB - 1b)
	.short (Op_0xBC - 1b)
	.short (Op_0xBD - 1b)
	.short (Op_0xBE - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xBF - 1b)
	.short (Op_0xC0 - 1b)
	.short (Op_0xC1 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xC2 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xC3 - 1b)
	.short (Op_0xC4 - 1b)
	.short (Op_0xC5 - 1b)
	.short (Op_0xC6 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xC7 - 1b)
	.short (Op_0xC8 - 1b)
	.short (Op_0xC9 - 1b)
	.short (Op_0xCA - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xCB - 1b)
	.short (Op_0xCC - 1b)
	.short (Op_0xCD - 1b)
	.short (Op_0xCE - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xCF - 1b)
	.short (Op_0xD0 - 1b)
	.short (Op_0xD1 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xD2 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xD3 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xD4 - 1b)
	.short (Op_0xD5 - 1b)
	.short (Op_0xD6 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xD7 - 1b)
	.short (Op_0xD8 - 1b)
	.short (Op_0xD9 - 1b)
	.short (Op_0xEA - 1b) // 0xDA, alternate 1-byte NOP
	.short (Op_0xEA - 1b) //	.short (Op_0xDB - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xDC - 1b)
	.short (Op_0xDD - 1b)
	.short (Op_0xDE - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xDF - 1b)
	.short (Op_0xE0 - 1b)
	.short (Op_0xE1 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xE2 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xE3 - 1b)
	.short (Op_0xE4 - 1b)
	.short (Op_0xE5 - 1b)
	.short (Op_0xE6 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xE7 - 1b)
	.short (Op_0xE8 - 1b)
	.short (Op_0xE9 - 1b)
	.short (Op_0xEA - 1b)
	.short (Op_0xEB - 1b)
	.short (Op_0xEC - 1b)
	.short (Op_0xED - 1b)
	.short (Op_0xEE - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xEF - 1b)
	.short (Op_0xF0 - 1b)
	.short (Op_0xF1 - 1b)
	.short (Op_0xF2 - 1b) // Fake //	.short (Op_0xF2 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xF3 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xF4 - 1b)
	.short (Op_0xF5 - 1b)
	.short (Op_0xF6 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xF7 - 1b)
	.short (Op_0xF8 - 1b)
	.short (Op_0xF9 - 1b)
	.short (Op_0xEA - 1b) // 0xFA, alternate 1-byte NOP
	.short (Op_0xEA - 1b) //	.short (Op_0xFB - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xFC - 1b)
	.short (Op_0xFD - 1b)
	.short (Op_0xFE - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0xFF - 1b)
op_table_middle:
	.short (Op_0xEA - 1b) //	.short (Op_0x00 - 1b)		// TODO BRK!
	.short (Op_0x01 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x02 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x03 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x04 - 1b)
	.short (Op_0x05 - 1b)
	.short (Op_0x06 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x07 - 1b)
	.short (Op_0x08 - 1b)
	.short (Op_0x09 - 1b)
	.short (Op_0x0A - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x0B - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x0C - 1b)
	.short (Op_0x0D - 1b)
	.short (Op_0x0E - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x0F - 1b)
	.short (Op_0x10 - 1b)
	.short (Op_0x11 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x12 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x13 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x14 - 1b)
	.short (Op_0x15 - 1b)
	.short (Op_0x16 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x17 - 1b)
	.short (Op_0x18 - 1b)
	.short (Op_0x19 - 1b)
	.short (Op_0xEA - 1b) // 0x1A, alternate 1-byte NOP
	.short (Op_0xEA - 1b) //	.short (Op_0x1B - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x1C - 1b)
	.short (Op_0x1D - 1b)
	.short (Op_0x1E - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x1F - 1b)
	.short (Op_0x20 - 1b)
	.short (Op_0x21 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x22 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x23 - 1b)
	.short (Op_0x24 - 1b)
	.short (Op_0x25 - 1b)
	.short (Op_0x26 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x27 - 1b)
	.short (Op_0x28 - 1b)
	.short (Op_0x29 - 1b)
	.short (Op_0x2A - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x2B - 1b)
	.short (Op_0x2C - 1b)
	.short (Op_0x2D - 1b)
	.short (Op_0x2E - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x2F - 1b)
	.short (Op_0x30 - 1b)
	.short (Op_0x31 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x32 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x33 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x34 - 1b)
	.short (Op_0x35 - 1b)
	.short (Op_0x36 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x37 - 1b)
	.short (Op_0x38 - 1b)
	.short (Op_0x39 - 1b)
	.short (Op_0xEA - 1b) // 0x3A, alternate 1-byte NOP
	.short (Op_0xEA - 1b) //	.short (Op_0x3B - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x3C - 1b)
	.short (Op_0x3D - 1b)
	.short (Op_0x3E - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x3F - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x40 - 1b)		// TODO RTI!
	.short (Op_0x41 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x42 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x43 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x44 - 1b)
	.short (Op_0x45 - 1b)
	.short (Op_0x46 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x47 - 1b)
	.short (Op_0x48 - 1b)
	.short (Op_0x49 - 1b)
	.short (Op_0x4A - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x4B - 1b)
	.short (Op_0x4C - 1b)
	.short (Op_0x4D - 1b)
	.short (Op_0x4E - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x4F - 1b)
	.short (Op_0x50 - 1b)
	.short (Op_0x51 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x52 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x53 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x54 - 1b)
	.short (Op_0x55 - 1b)
	.short (Op_0x56 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x57 - 1b)
	.short (Op_0x58 - 1b)
	.short (Op_0x59 - 1b)
	.short (Op_0xEA - 1b) // 0x5A, alternate 1-byte NOP
	.short (Op_0xEA - 1b) //	.short (Op_0x5B - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x5C - 1b)
	.short (Op_0x5D - 1b)
	.short (Op_0x5E - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x5F - 1b)
	.short (Op_0x60 - 1b)
	.short (Op_0x61 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x62 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x63 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x64 - 1b)
	.short (Op_0x65 - 1b)
	.short (Op_0x66 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x67 - 1b)
	.short (Op_0x68 - 1b)
	.short (Op_0x69 - 1b)
	.short (Op_0x6A - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x6B - 1b)
	.short (Op_0x6C - 1b)
	.short (Op_0x6D - 1b)
	.short (Op_0x6E - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x6F - 1b)
	.short (Op_0x70 - 1b)
	.short (Op_0x71 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x72 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x73 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x74 - 1b)
	.short (Op_0x75 - 1b)
	.short (Op_0x76 - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x77 - 1b)
	.short (Op_0x78 - 1b)
	.short (Op_0x79 - 1b)
	.short (Op_0xEA - 1b) // 0x7A, alternate 1-byte NOP
	.short (Op_0xEA - 1b) //	.short (Op_0x7B - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x7C - 1b)
	.short (Op_0x7D - 1b)
	.short (Op_0x7E - 1b)
	.short (Op_0xEA - 1b) //	.short (Op_0x7F - 1b)//
//
//
//
//
//

//
// NOP
//
Op_0xEA:
	bra s6502_execute
	nop

//
// CLI (TODO)
// Maybe... alter CREG_IHBASE ?
Op_0x58:
	//tst #FLAG_I, r0
	//bt/s 1337f

	mov #~FLAG_I, r0
	bra s6502_execute
	and r0, REG_P

//
// SEI (TODO)
//
Op_0x78:
	mov #FLAG_I, r0
	bra s6502_execute
	or r0, REG_P

//
// CLC
//
Op_0x18:
	mov #~FLAG_C, r0
	bra s6502_execute
	and r0, REG_P

//
// CLD
//
Op_0xD8:
	mov #~FLAG_D, r0
	bra s6502_execute
	and r0, REG_P

//
// CLV
//
Op_0xB8:
	mov #~FLAG_V, r0
	bra s6502_execute
	and r0, REG_P

//
// SEC
//
Op_0x38:
	mov #FLAG_C, r0
	bra s6502_execute
	or r0, REG_P

//
// SED
//
Op_0xF8:
	mov #FLAG_D, r0
	bra s6502_execute
	or r0, REG_P

//
// DEX
//
Op_0xCA:
	add #-1, REG_X
	extu.b REG_X, REG_X

	bra s6502_execute
	exts.b REG_X, REG_NZ

//
// DEY
//
Op_0x88:
	add #-1, REG_Y
	extu.b REG_Y, REG_Y

	bra s6502_execute
	exts.b REG_Y, REG_NZ

//
// INX
//
Op_0xE8:
	add #1, REG_X
	extu.b REG_X, REG_X

	bra s6502_execute
	exts.b REG_X, REG_NZ

//
// INY
//
Op_0xC8:
	add #1, REG_Y
	extu.b REG_Y, REG_Y

	bra s6502_execute
	exts.b REG_Y, REG_NZ

//
// TAX
//
Op_0xAA:
	mov REG_A, REG_X
	bra s6502_execute
	exts.b REG_A, REG_NZ

//
// TAY
//
Op_0xA8:
	mov REG_A, REG_Y
	bra s6502_execute
	exts.b REG_A, REG_NZ

//
// TXA
//
Op_0x8A:
	mov REG_X, REG_A
	bra s6502_execute
	exts.b REG_X, REG_NZ

//
// TYA
//
Op_0x98:
	mov REG_Y, REG_A
	bra s6502_execute
	exts.b REG_Y, REG_NZ

//
// TSX
//
Op_0xBA:
	mov REG_SP, REG_X
	bra s6502_execute
	exts.b REG_SP, REG_NZ

//
// TXS
//
Op_0x9A:
	bra s6502_execute
	mov REG_X, REG_SP

//
// BEQ
//
.align 2
Op_0xF0:
	mov.b @REG_PC+, r1
	mov REG_NZ, r0

	tst #0xFF, r0
	bt/s rel_branch
	mov REG_PC, r2
	bra s6502_execute
	nop

//
// BNE
//
.align 2
Op_0xD0:
	mov.b @REG_PC+, r1
	mov REG_NZ, r0

	tst #0xFF, r0
	bf/s rel_branch
	mov REG_PC, r2
	bra s6502_execute
	nop

//
// BMI
//
.align 2
Op_0x30:
	mov.b @REG_PC+, r1
	cmp/pz REG_NZ

	bf/s rel_branch
	mov REG_PC, r2
	bra s6502_execute
	nop
//
// BPL
//
.align 2
Op_0x10:
	mov.b @REG_PC+, r1
	cmp/pz REG_NZ

	bt/s rel_branch
	mov REG_PC, r2
	bra s6502_execute
	nop

rel_branch:
#if 1
	add r1, REG_PC
	xor REG_PC, r2
	swap.b r2, r0
	and #1, r0
	add r0, REG_CYC_CNT
	bra s6502_execute
	add #1, REG_CYC_CNT
#else
	add r1, REG_PC
	xor REG_PC, r2

	swap.b r2, r0
	rotcr r0

	bf/s s6502_execute
	add #1, REG_CYC_CNT

	add #1, REG_CYC_CNT
	stc GBR, r2

	sub REG_PC_BASE, REG_PC
	swap.b REG_PC, r0

	shlr2 r0
	and #0x3C, r0

	mov.l @(r0, r2), REG_PC_BASE
	bra s6502_execute
	add REG_PC_BASE, REG_PC
#endif

//
// BVC
//
.align 2
Op_0x50:
	mov.b @REG_PC+, r1
	mov REG_P, r0

	tst #FLAG_V, r0
	bt/s rel_branch
	mov REG_PC, r2

	bra s6502_execute
	nop

//
// BVS
//
.align 2
Op_0x70:
	mov.b @REG_PC+, r1
	mov REG_P, r0

	tst #FLAG_V, r0
	bf/s rel_branch
	mov REG_PC, r2

	bra s6502_execute
	nop

//
// BCC
//
.align 2
Op_0x90:
	mov.b @REG_PC+, r1
	mov REG_P, r0

	tst #FLAG_C, r0
	bt/s rel_branch
	mov REG_PC, r2

	bra s6502_execute
	nop

//
// BCS
//
.align 2
Op_0xB0:
	mov.b @REG_PC+, r1
	mov REG_P, r0

	tst #FLAG_C, r0
	bf/s rel_branch
	mov REG_PC, r2

	bra s6502_execute
	nop

//
// JMP indirect
//
.align 2
Op_0x6C:
	mov.b @REG_PC+, r1
	add #2, REG_CYC_CNT

	mov.b @REG_PC+, REG_DATA_BUS
	extu.b r1, r1

	sts SREG_RFAP, r0
	extu.b REG_DATA_BUS, r2

	shll8 r2
	mov r2, REG_ADDR

	add r1, REG_ADDR
	add #1, r1

	extu.b r1, r1
	add r2, r1
	//
	//
	mov.b @(r0,REG_ADDR), r3	// Load read function pointer
	nop

	shll2 r3
	add REG_RAM, r3

	jsr @r3			// Call read function
	ldc r1, CREG_SCRATCH
	//
	//
	sts SREG_RFAP, r0
	stc CREG_SCRATCH, REG_ADDR

	mov.b @(r0,REG_ADDR), r3	// Load read function pointer
	add #1, REG_CYC_CNT

	shll2 r3
	add REG_RAM, r3

	jsr @r3			// Call read function
	ldc REG_DATA_BUS, CREG_SCRATCH
	//
	//
	stc CREG_SCRATCH, r1
	extu.b r1, r1

	extu.b REG_DATA_BUS, REG_PC
	shll8 REG_PC

	add r1, REG_PC
	stc GBR, r2

	bra set_pc
	swap.b REG_PC, r0

//
// RTI (TODO)
//

//
// RTS
//
.align 2
Op_0x60:
	mov.l ram_stack_addr, r0
	add #1, REG_SP

	extu.b REG_SP, REG_SP
	stc GBR, r2

	mov.b @(r0, REG_SP), r1
	add #1, REG_SP

	extu.b REG_SP, REG_SP
	add #4, REG_CYC_CNT

	mov.b @(r0, REG_SP), REG_PC
	extu.b r1, r1

	shll8 REG_PC
	add r1, REG_PC

	add #1, REG_PC
	extu.w REG_PC, REG_PC

	bra set_pc
	swap.b REG_PC, r0

//
// JSR
//
.align 2
Op_0x20:
	mov REG_PC, r1
	add #1, r1

	sub REG_PC_BASE, r1
	add #3, REG_CYC_CNT

	mov.l ram_stack_addr, r0
	swap.b r1, r3

	mov.b r3, @(r0, REG_SP)
	add #-1, REG_SP

	extu.b REG_SP, REG_SP
	nop

	mov.b r1, @(r0, REG_SP)
	add #-1, REG_SP

	bra Op_0x4C
	extu.b REG_SP, REG_SP
ram_stack_addr:
	.long _ram + 0x100
//
// JMP
//
.align 2
Op_0x4C:
	mov.b @REG_PC+, r1
	stc GBR, r2

	mov.b @REG_PC+, r0
	extu.b r1, r1

	extu.b r0, REG_PC
	shll8 REG_PC

	add r1, REG_PC
	add #1, REG_CYC_CNT
//
set_pc:
	shlr2 r0
	and #0x3C, r0

	mov.l @(r0, r2), REG_PC_BASE
	bra s6502_execute
	add REG_PC_BASE, REG_PC


//
// PLP (TODO: I flag special handling)
//
.align 2
Op_0x28:
	mov.l ram_stack_addr2, r0
	add #1, REG_SP

	extu.b REG_SP, REG_SP
	add #2, REG_CYC_CNT

	mov.b @(r0, REG_SP), r0
	nop
	//
	or #FLAG_U | FLAG_B, r0
	exts.b r0, REG_NZ

	and #0x7D, r0
	extu.b r0, REG_P

	mov REG_NZ, r0
	and #FLAG_Z, r0

	mov r0, r1
	mova plp_z_table, r0

	mov.w @(r0, r1), r1
	bra s6502_execute
	and r1, REG_NZ
.align 2
plp_z_table:
	.short 0xFFFF
	.short 0xFF00


//
// PHP
//
Op_0x08:
	mov REG_P, r1
	cmp/pz REG_NZ
	movt r0
	add #-1, r0
	and #FLAG_N, r0
	or r0, r1

	extu.b REG_NZ, r0
	cmp/pl r0
	movt r0
	add #-1, r0
	and #FLAG_Z, r0
	or r0, r1

	//
	//
	.align 2
	mov.l ram_stack_addr2, r0
	add #1, REG_CYC_CNT

	mov.b r1, @(r0, REG_SP)
	add #-1, REG_SP

	bra s6502_execute
	extu.b REG_SP, REG_SP

//
// PLA
//
.align 2
Op_0x68:
	mov.l ram_stack_addr2, r0
	add #1, REG_SP

	extu.b REG_SP, REG_SP
	add #2, REG_CYC_CNT

	mov.b @(r0, REG_SP), REG_NZ
	bra s6502_execute
	extu.b REG_NZ, REG_A

//
// PHA
//
.align 2
Op_0x48:
	mov.l ram_stack_addr2, r0
	add #1, REG_CYC_CNT

	mov.b REG_A, @(r0, REG_SP)
	add #-1, REG_SP

	bra s6502_execute
	extu.b REG_SP, REG_SP

ram_stack_addr2:
	.long _ram + 0x100


//
//


// ADC
CASE(0x69): RD_IMM  ; OP_ADC

// AND
CASE(0x29): RD_IMM  ; OP_AND

// CMP
CASE(0xC9): RD_IMM  ; OP_CMP

// EOR
CASE(0x49): RD_IMM  ; OP_EOR

// LDA
CASE(0xA9): RD_IMM  ; OP_LDA

// LDX
CASE(0xA2): RD_IMM  ; OP_LDX

// LDY
CASE(0xA0): RD_IMM  ; OP_LDY

// ORA
CASE(0x09): RD_IMM  ; OP_ORA

// SBC
CASE(0xEB): //
CASE(0xE9): RD_IMM  ; OP_SBC

// CPX
CASE(0xE0): RD_IMM  ; OP_CPX

// CPY
CASE(0xC0): RD_IMM  ; OP_CPY

//
//

// ASL
CASE(0x0A): OP_ASL_A

// LSR
CASE(0x4A): OP_LSR_A

// ROL
CASE(0x2A): OP_ROL_A

// ROR
CASE(0x6A): OP_ROR_A

//
//

// ADC
CASE(0x6D): RD_ABS   ; OP_ADC
CASE(0x7D): RD_ABS_X ; OP_ADC
CASE(0x79): RD_ABS_Y ; OP_ADC

// AND
CASE(0x2D): RD_ABS   ; OP_AND
CASE(0x3D): RD_ABS_X ; OP_AND
CASE(0x39): RD_ABS_Y ; OP_AND

// CMP
CASE(0xCD): RD_ABS   ; OP_CMP
CASE(0xDD): RD_ABS_X ; OP_CMP
CASE(0xD9): RD_ABS_Y ; OP_CMP

// EOR
CASE(0x4D): RD_ABS   ; OP_EOR
CASE(0x5D): RD_ABS_X ; OP_EOR
CASE(0x59): RD_ABS_Y ; OP_EOR

// LDA
CASE(0xAD): RD_ABS   ; OP_LDA
CASE(0xBD): RD_ABS_X ; OP_LDA
CASE(0xB9): RD_ABS_Y ; OP_LDA

// LDX
CASE(0xAE): RD_ABS   ; OP_LDX
CASE(0xBE): RD_ABS_Y ; OP_LDX

// LDY
CASE(0xAC): RD_ABS   ; OP_LDY
CASE(0xBC): RD_ABS_X ; OP_LDY

// ORA
CASE(0x0D): RD_ABS   ; OP_ORA
CASE(0x1D): RD_ABS_X ; OP_ORA
CASE(0x19): RD_ABS_Y ; OP_ORA

// SBC
CASE(0xED): RD_ABS   ; OP_SBC
CASE(0xFD): RD_ABS_X ; OP_SBC
CASE(0xF9): RD_ABS_Y ; OP_SBC

// BIT
CASE(0x2C): RD_ABS_OP_BIT //RD_ABS   ; OP_BIT

// CPX
CASE(0xEC): RD_ABS   ; OP_CPX

// CPY
CASE(0xCC): RD_ABS   ; OP_CPY


//
//

// ASL
CASE(0x06): RMW_ZP   OP_ASL
CASE(0x16): RMW_ZP_X OP_ASL

// DEC
CASE(0xC6): RMW_ZP_DEC
CASE(0xD6): RMW_ZP_X_DEC

// INC
CASE(0xE6): RMW_ZP_INC
CASE(0xF6): RMW_ZP_X_INC

// LSR
CASE(0x46): RMW_ZP   OP_LSR
CASE(0x56): RMW_ZP_X OP_LSR

// ROL
CASE(0x26): RMW_ZP   OP_ROL
CASE(0x36): RMW_ZP_X OP_ROL

// ROR
CASE(0x66): RMW_ZP   OP_ROR
CASE(0x76): RMW_ZP_X OP_ROR

// ADC
CASE(0x65): RD_ZP   ; OP_ADC
CASE(0x75): RD_ZP_X ; OP_ADC

// AND
CASE(0x25): RD_ZP   ; OP_AND
CASE(0x35): RD_ZP_X ; OP_AND

// CMP
CASE(0xC5): RD_ZP   ; OP_CMP
CASE(0xD5): RD_ZP_X ; OP_CMP

// EOR
CASE(0x45): RD_ZP   ; OP_EOR
CASE(0x55): RD_ZP_X ; OP_EOR

// LDA
CASE(0xA5): RD_ZP   ; OP_LDA
CASE(0xB5): RD_ZP_X ; OP_LDA

// LDX
CASE(0xA6): RD_ZP   ; OP_LDX
CASE(0xB6): RD_ZP_Y ; OP_LDX

// LDY
CASE(0xA4): RD_ZP   ; OP_LDY
CASE(0xB4): RD_ZP_X ; OP_LDY

// ORA
CASE(0x05): RD_ZP   ; OP_ORA
CASE(0x15): RD_ZP_X ; OP_ORA

// SBC
CASE(0xE5): RD_ZP   ; OP_SBC
CASE(0xF5): RD_ZP_X ; OP_SBC

// BIT
CASE(0x24): RD_ZP   ; OP_BIT

// CPX/CPY
CASE(0xE4): RD_ZP   ; OP_CPX
CASE(0xC4): RD_ZP   ; OP_CPY

// STA
CASE(0x85): WR_ZP REG_A
CASE(0x95): WR_ZP_X REG_A

// STX
CASE(0x86): WR_ZP REG_X
CASE(0x96): WR_ZP_Y REG_X

// STY
CASE(0x84): WR_ZP REG_Y
CASE(0x94): WR_ZP_X REG_Y

CASE(0xF2):
	mov.b @(GBR_OFFS_NSF_FRAME_PENDING, GBR), r0
	tst #0xFF, r0
	bf 1f
	mov #0, REG_CYC_CNT
	1:
	bra s6502_execute
	nop
//
//
//
.section ".loram.bss.uncached"
.align 2
_rfap: .dcb.b 0x10000

.section ".hiram.bss.uncached"
.align 2
_wfap: .dcb.b 0x10000*4
