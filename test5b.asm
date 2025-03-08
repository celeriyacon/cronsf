;
;  test5b.asm
;
;  Copyright (C) 2025 celeriyacon - https://github.com/celeriyacon
;
;  This software is provided 'as-is', without any express or implied
;  warranty.  In no event will the authors be held liable for any damages
;  arising from the use of this software.
;
;  Permission is granted to anyone to use this software for any purpose,
;  including commercial applications, and to alter it and redistribute it
;  freely, subject to the following restrictions:
;
;  1. The origin of this software must not be misrepresented; you must not
;     claim that you wrote the original software. If you use this software
;     in a product, an acknowledgment in the product documentation would be
;     appreciated but is not required.
;  2. Altered source versions must be plainly marked as such, and must not be
;     misrepresented as being the original software.
;  3. This notice may not be removed or altered from any source distribution.
;

* = $8000
.aasc "NSFE"
.word (chunk_info_bound - chunk_info), $0000
.aasc "INFO"

chunk_info:
.word reset	; Load address
.word reset	; Init address
.word reset	; Play address
.byte $00	; NTSC/PAL bits
.byte $20	; Expansion sound chips
.byte (rtable_bound - rtable) / 2	; Number of songs
.byte $00	; Starting song
chunk_info_bound:
;
;
;
.word (chunk_data_bound - chunk_data) & $FFFF, (chunk_data_bound - chunk_data) / $10000
.aasc "DATA"

#define STA(s) pha : lda #(s) : sta $C0DE : pla : sta $F00D
#define STX(s) pha : lda #(s) : sta $C0DE : pla : stx $F00D

#define ST(s,v) pha : lda #(s) : sta $D00D : lda #(v) : sta $EDCB : pla


chunk_data:
reset:
	sei
	cld
	ldx #$FF
	txs

	pha

	lda #$00
	sta $4015

	lda #$00
	sta $4017

	pla
	asl
	tax
	lda rtable+0, X
	sta $00
	lda rtable+1, X
	sta $01

	lda #(ret - 1) >> 8
	pha
	lda #(ret - 1) & $FF
	pha

	lda #$7F
	sta $4011
	jsr delay_quick

	lda #$00
	sta $4011

	jmp ($0000)
ret:
	lda #$00
	STA($08)
	STA($09)
	STA($0A)
	sta $4015

	lda #$7F
	sta $4011
	jsr delay_quick
	lda #$00
	sta $4011

	jmp *

rtable:
	;.word test_apu_5b_sync
	.word test_apu_5b_rel_volume
	.word test_pulse_sweep
	.word test_pulse_hf
	.word test_noise_freqs
	.word test_multi
	.word test_env_shapes
	.word test_env_harm
	.word test_max_volume
	.word test_max_host_cpu
	.word test_max_host_cpu_mainbusy
	.word test_max_host_cpu_XTREME
rtable_bound:
;
;
;
;
;
;
test_apu_5b_sync:
/*
	lda #$FF
	sta $9001
	sta $9002

	lda #$00
tavs_loop:
	eor #$7F
	pha
	and #$0F
	ora #$F0
	tay
	pla
	sta $4011
	sty $9000

	ldx #$00
tavs_delay:
	dex
	bne tavs_delay
	jmp tavs_loop
*/
	rts

test_apu_5b_rel_volume:
	lda #$01
	sta $4015

	lda #$BF
	sta $4000

	lda #$80
	sta $4001

	lda #$FE
	sta $4002

	jsr delay_slow

	lda #$00
	sta $4003

	jsr delay_slow

	lda #$00
	sta $4015

	jsr delay_slow

	ST($00, $7F)
	ST($01, $00)

	ST($07, $3E)
	ST($08, $0C)

	jsr delay_slow

	ST($07, $3F)

	jsr delay_slow

	rts

test_pulse_sweep:
	jsr clear_zp

	ST($07, $38)
	ST($08, $0C)

tps_loop:
	ldx $00
	lda $01
	ora #$80
	STA($01)
	STX($00)


	ldy #$04
tps_delay0:
	ldx #$00
tps_delay1:
	dex
	bne tps_delay1
	dey
	bne tps_delay0

	clc
	lda $00
	adc #$01
	sta $00
	lda $01
	adc #$00
	tax
	and #$0F
	sta $01

	cpx #$10
	bne tps_loop

	ST($08, $00)

	rts

test_pulse_hf:
	ST($01, $00)
	ST($07, $38)
	ST($08, $0C)

	ldx #$00
tph_loop:
	txa
	STA($00)
	ST($08, $0C)

	jsr delay_quick
	jsr delay_quick

	ST($08, $00)

	jsr delay_quick

	inx
	cpx #$10
	bne tph_loop

	rts

clear_zp:
	pha
	txa
	pha
	tya
	pha

	lda #$00
	ldx #$00
clear_zp_loop:
	sta $00, X
	inx
	bne clear_zp_loop

	pla
	tay
	pla
	tax
	pla
	rts

;
;
;
;
;
test_noise_freqs:
	ST($07, $37)

	lda #$00
tnf_loop:
	ST($08, $0C)
	STA($06)

	jsr delay_quick

	ST($08, $00)

	jsr delay_quick

	sec
	adc #$00
	cmp #$20
	bne tnf_loop	
	rts

;
;
;
;
;
;
test_multi:
	lda #$00
	STA($00)
	STA($01)

	STA($02)
	STA($03)

	STA($04)
	STA($05)


	//ST($06, $1F)
	ST($06, $10)

	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop

	lda #$05
	sta $C000
	lda #$04
	sta $E000

	lda #$03
	sta $C000
	lda #$02
	sta $E000

	lda #$01
	sta $C000
	lda #$01
	sta $E000

	ST($07, $38)
	ST($08, $08)
	ST($09, $0A)
	ST($0A, $0C)

	jsr delay_slow

	ST($07, $3F)

	jsr delay_quick

	ST($07, $00)

	jsr delay_slow

	ST($07, $3F)

	jsr delay_quick

	rts
;
;
;
;
;
test_env_shapes:
	ST($00, $20)
	ST($01, $00)

	ST($07, $3E)

	ST($0B, $80)
	ST($0C, $00)

	lda #$00
tes_loop:
	ST($08, $10)
	STA($0D)

	jsr delay_quick

	ST($08, $00)

	jsr delay_quick

	sec
	adc #$00
	cmp #$10
	bne tes_loop

	rts

;
;
;
;
;
;
test_env_harm:
	ST($07, $3E)
	ST($08, $10)

	ST($0C, $00)
	ST($0D, $08)

	ST($00, $50)
	ST($01, $00)

	ST($0B, $06)

	jsr delay_quick

	rts
;
;
;
;
;
;
test_max_volume:
	ST($00, $20)
	ST($01, $00)

	ST($02, $23)
	ST($03, $00)

	ST($04, $27)
	ST($05, $00)

	ST($07, $38)

	ST($08, $0F)
	ST($09, $0F)
	ST($0A, $0F)

	jsr delay_quick

	//ST($07, $3F)
	ST($08, $00)
	ST($09, $00)
	ST($0A, $00)

	jsr delay_quick

	rts

;
;
;
;
;
;
test_max_host_cpu:
	ST($00, $00)
	ST($01, $00)

	ST($02, $00)
	ST($03, $00)

	ST($04, $00)
	ST($05, $00)

	ST($06, $00)

	ST($07, $00)

	ST($08, $FF)
	ST($09, $FF)
	ST($0A, $FF)

	ST($0B, $00)
	ST($0C, $00)
	ST($0D, $0A)

	//jsr delay_slow
	jmp *

	rts

;
;
;
;
;
;
test_max_host_cpu_mainbusy:
	ST($00, $00)
	ST($01, $00)

	ST($02, $00)
	ST($03, $00)

	ST($04, $00)
	ST($05, $00)

	ST($06, $00)

	ST($07, $00)

	ST($08, $FF)
	ST($09, $FF)
	ST($0A, $FF)

	ST($0B, $00)
	ST($0C, $00)
	ST($0D, $0A)

tmhcm_loop:
	inc $7FFF
	inc $7FFF
	inc $7FFF
	inc $7FFF
	jmp tmhcm_loop

	rts

;
;
;
;
;
;
test_max_host_cpu_XTREME:
	ST($00, $00)
	ST($01, $00)

	ST($02, $00)
	ST($03, $00)

	ST($04, $00)
	ST($05, $00)

	ST($06, $00)

	ST($07, $00)

	ST($08, $FF)
	ST($09, $FF)
	ST($0A, $FF)

	ST($0B, $00)
	ST($0C, $00)
	ST($0D, $0A)

	//jsr delay_slow
	lda #$00
tmhcx_loop:
	eor #$38
	STA($07)
	jmp tmhcx_loop

	rts

;
;
;
;
;
;
;
delay_slow:
	pha
	txa
	pha
	tya
	pha
	;
	lda #$F8
	delay1:
	ldx #$00
	delay2:
	ldy #$00
	delay3:
	iny
	bne delay3
	inx
	bne delay2
	clc
	adc #$01
	bne delay1
	;
	pla
	tay
	pla
	tax
	pla
	rts

delay_quick:
	pha
	txa
	pha
	tya
	pha
	;
	lda #$FC
	delay4:
	ldx #$00
	delay5:
	ldy #$00
	delay6:
	iny
	bne delay6
	inx
	bne delay5
	clc
	adc #$01
	bne delay4
	;
	pla
	tay
	pla
	tax
	pla
	rts

.dsb $C000 - *, $00
sample:
.dsb 16 + 1, $CC

.dsb $FFFC - *, $00
reset_vector: .word reset
.dsb $10000 - *, $00
chunk_data_bound:


.word (chunk_auth_bound - chunk_auth) & $FFFF, (chunk_auth_bound - chunk_auth) / $10000
.aasc "auth"
chunk_auth:
.aasc "Quick Sunsoft 5B Tester", $0
.aasc "Mr. X", $0
.aasc "20XX Wily Labs", $0
chunk_auth_bound:
;
;
;
.word (chunk_tlbl_bound - chunk_tlbl) & $FFFF, (chunk_tlbl_bound - chunk_tlbl) / $10000
.aasc "tlbl"
chunk_tlbl:
;.aasc "test_apu_5b_sync", $0
.aasc "test_apu_5b_rel_volume", $0
.aasc "test_pulse_sweep", $0
.aasc "test_pulse_hf", $0
.aasc "test_noise_freqs", $0
.aasc "test_multi", $0
.aasc "test_env_shapes", $0
.aasc "test_env_harm", $0
.aasc "test_max_volume", $0
.aasc "test_max_host_cpu", $0
.aasc "test_max_host_cpu_mainbusy", $0
.aasc "test_max_host_cpu_XTREME", $0
chunk_tlbl_bound:

.word $0, $0
.aasc "NEND"
