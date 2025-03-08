;
;  testmmc5.asm
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
.byte $08	; Expansion sound chips
.byte (rtable_bound - rtable) / 2	; Number of songs
.byte $00	; Starting song
chunk_info_bound:
;
;
;
.word (chunk_data_bound - chunk_data) & $FFFF, (chunk_data_bound - chunk_data) / $10000
.aasc "DATA"

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
	sta $5015
	sta $4015

	lda #$7F
	sta $4011
	jsr delay_quick
	lda #$00
	sta $4011

	jmp *

rtable:
	.word test_apu_mmc5_sync
	.word test_apu_mmc5_rel_volume
	.word test_length_counter_sync
	.word test_pulse_sweep
rtable_bound:
;
;
;
;
;
;
test_apu_mmc5_sync:


	jsr clear_zp

	lda #$00
	sta $00

	lda #$00
	sta $01

	lda #$00
tams_loop:
	lda $00
	clc
	adc #$20
	sta $00
	and #$7F
	tay
	iny

	lda $01
	eor #$7F
	sta $01

	sta $4011
	sty $5011

	ldx #$00
tams_delay:
	dex
	bne tams_delay
	jmp tams_loop

	rts

test_apu_mmc5_rel_volume:
	lda #$01
	sta $4015

	lda #$BF
	sta $4000

	lda #$80
	sta $4001

	lda #$FF
	sta $4002

	jsr delay_slow

	lda #$00
	sta $4003

	jsr delay_slow

	lda #$00
	sta $4015

	jsr delay_slow
	;
	;
	;
	lda #$01
	sta $5015

	lda #$BF
	sta $5000

	lda #$FF
	sta $5002

	jsr delay_slow

	lda #$00
	sta $5003

	jsr delay_slow

	lda #$00
	sta $5015

	jsr delay_slow
	;
	;
	;
	lda #$00
	sta $00
	sta $01
	sta $02

apu_pcm_loop:
	lda $02
	eor #$7F
	sta $02
	tay
	iny
	sta $4011

	ldy #$C0
apld_loop:
	iny
	bne apld_loop

	clc
	lda $00
	adc #$08
	sta $00
	lda $01
	adc #$00
	sta $01
	bcc apu_pcm_loop

	;
	;
	;
	lda #$00
	sta $00
	sta $01
	sta $02


	lda #$00
	sta $4011
	jsr delay_quick

mmc5_pcm_loop:
	lda $02
	eor #$7F
	sta $02
	tay
	iny
	sty $5011

	ldy #$C0
mpld_loop:
	iny
	bne mpld_loop

	clc
	lda $00
	adc #$08
	sta $00
	lda $01
	adc #$00
	sta $01
	bcc mmc5_pcm_loop

	rts
;
;
;
;
;
test_length_counter_sync:
	jsr clear_zp

	jsr delay_quick

	ldx #$00
	ldy #$01
tlcs_biggerloop:
	sty $5015

	lda #$1F
	sta $5000, X

	lda #$10
	sta $5002, X

tlcs_bigloop:
	lda $00
	sta $5003, X

	lda #$7F
	sta $5011
tlcs_loop0:
	tya
	and $5015
	bne tlcs_loop0

	lda #$01
	sta $5011

	jsr delay_quick

	clc
	lda $00
	adc #$08
	sta $00
	bne tlcs_bigloop
	;
	;
	;
	;
	inx
	inx
	inx
	inx

	tya
	asl
	tay

	cpy #$04
	bne tlcs_biggerloop
	;
	;
	;
#define BNE_F beq *+5 : jmp
#define BEQ_F bne *+5 : jmp

	lda #$00
	sta $5000
	sta $5002
	sta $5004
	sta $5006

	sta $5015
	lda $5015
	cmp #$00
	BNE_F tlcs_fail
	sta $5003
	sta $5007
	sta $5015
	lda $5015
	cmp #$00
	BNE_F tlcs_fail

	lda #$20
	sta $5000	
	lda #$20
	sta $5002
	lda #$00
	lda #$01
	sta $5015
	sta $5003
	lda $5015
	cmp #$01
	BNE_F tlcs_fail
	;
	;
	;
	;
	;
	lda #$00
	sta $5015
	lda $5015
	cmp #$00
	BNE_F tlcs_fail
	sta $5000
	sta $5002
	sta $5003
	sta $5004
	sta $5006
	sta $5007
	sta $5015
	lda $5015
	cmp #$00
	BNE_F tlcs_fail

	lda #$20
	sta $5004	
	lda #$20
	sta $5006
	lda #$02
	sta $5015
	lda #$00
	sta $5007
	lda $5015
	cmp #$02
	BNE_F tlcs_fail

	lda #$00
	sta $5015
	lda $5015
	cmp #$00
	BNE_F tlcs_fail
	sta $5003
	sta $5007
	sta $5015
	lda $5015
	cmp #$00
	BNE_F tlcs_fail

	rts

tlcs_fail:
	lda #$03
	sta $5015

	lda #$BF
	sta $5000
	sta $5004

	lda #$FF
	sta $5002
	sta $5006

	lda #$07
	sta $5003
	sta $5007

	jmp *

	rts
;
;
;
;
;
test_pulse_sweep:
	jsr clear_zp

	lda #$3F
	sta $02

	lda #$01
	sta $5015

	lda #$10
	sta $5003

tps_loop:
	ldx $00
	lda $01
	ldy $02

	cpx #$00
	bne tps_partial_period
	stx $5002
	sta $5003
	sty $5000
	jmp tps_full_period_cont

tps_partial_period:
	stx $5002
	stx $5002
	sty $5000
	nop

tps_full_period_cont:

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
	sta $01

	and #$08
	asl
	asl
	asl
	clc
	adc $02
	sta $02
	tax

	lda $01
	and #$07
	sta $01

	bcc tps_loop
	
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
.aasc "Quick MMC5 Tester", $0
.aasc "Mr. X", $0
.aasc "20XX Wily Labs", $0
chunk_auth_bound:
;
;
;
.word (chunk_tlbl_bound - chunk_tlbl) & $FFFF, (chunk_tlbl_bound - chunk_tlbl) / $10000
.aasc "tlbl"
chunk_tlbl:
.aasc "test_apu_mmc5_sync", $0
.aasc "test_apu_mmc5_rel_volume", $0
.aasc "test_length_counter_sync", $0
.aasc "test_pulse_sweep", $0
chunk_tlbl_bound:

.word $0, $0
.aasc "NEND"
