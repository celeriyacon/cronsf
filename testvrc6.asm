;
;  testvrc6.asm
;
;  Copyright (C) 2024 celeriyacon - https://github.com/celeriyacon
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
.byte $01	; Expansion sound chips
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
	sta $9002
	sta $A002
	sta $B002
	sta $4015

	lda #$7F
	sta $4011
	jsr delay_quick
	lda #$00
	sta $4011

	jmp *

rtable:
	.word test_apu_vrc6_sync
	.word test_apu_vrc6_rel_volume
	.word test_pulse_sweep
	.word test_saw_volume_mod
	.word test_saw_sweep
	.word test_saw_freq_race
rtable_bound:
;
;
;
;
;
;
test_apu_vrc6_sync:
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

	rts

test_apu_vrc6_rel_volume:
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

	lda #$7F
	sta $9000
	lda #$FF
	sta $9001
	lda #$80
	sta $9002

	jsr delay_slow

	lda #$00
	sta $9002

	jsr delay_slow

	rts

test_saw_volume_mod:
	lda #$00
	sta $B001
	lda #$88
	sta $B002

	lda #$01
tsvmloop0:
	eor #$27
	sta $B000

	ldx #$FF
tsvmloop1:
	dex
	bne tsvmloop1
	jmp tsvmloop0
	
	rts

test_pulse_sweep:
	jsr clear_zp

	lda #$0F
	sta $02

tps_loop:
	ldx $00
	lda $01
	ldy $02
	ora #$80
	stx $9001
	sta $9002
	sty $9000


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

	and #$10
	clc
	adc $02
	sta $02
	tax

	lda $01
	and #$0F
	sta $01

	cpx #$8F
	bne tps_loop
	
	rts
;
;
;
;
;
test_saw_sweep
	jsr clear_zp

	lda #$26
	sta $B000

tss_loop:
	ldx $00
	lda $01
	ora #$80
	stx $B001
	sta $B002


	ldy #$04
tss_delay0:
	ldx #$00
tss_delay1:
	dex
	bne tss_delay1
	dey
	bne tss_delay0

	clc
	lda $00
	adc #$01
	sta $00
	lda $01
	adc #$00
	sta $01
	cmp #$10
	bne tss_loop	
	
	rts
;
;
;
test_saw_freq_race:
	jsr clear_zp

	lda #$26
	sta $B000

tsfg_loop:
	lda #$80
	sta $B002
	lda #$FF
	sta $B001
	;
	;
	ldy #$FF
tsfg_delay0a:
	ldx #$00
tsfg_delay0b:
	dex
	bne tsfg_delay0b
	dey
	bne tsfg_delay0a
	;
	;
	lda #$07
	sta $B001
	lda #$81
	sta $B002
	
	ldy #$FF
tsfg_delay1a:
	ldx #$00
tsfg_delay1b:
	dex
	bne tsfg_delay1b
	dey
	bne tsfg_delay1a

	jmp tsfg_loop

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
.aasc "Quick VRC6 Tester", $0
.aasc "Mr. X", $0
.aasc "20XX Wily Labs", $0
chunk_auth_bound:
;
;
;
.word (chunk_tlbl_bound - chunk_tlbl) & $FFFF, (chunk_tlbl_bound - chunk_tlbl) / $10000
.aasc "tlbl"
chunk_tlbl:
.aasc "test_apu_vrc6_sync", $0
.aasc "test_apu_vrc6_rel_volume", $0
.aasc "test_pulse_sweep", $0
.aasc "test_saw_volume_mod", $0
.aasc "test_saw_sweep", $0
.aasc "test_saw_freq_race", $0
chunk_tlbl_bound:

.word $0, $0
.aasc "NEND"
