;
;  testapu.asm
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
.byte $00	; Expansion sound chips
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
	sta $4015

	lda #$7F
	sta $4011
	jsr delay_quick
	lda #$00
	sta $4011

	jmp *

rtable:
	.word test_cpu
	.word test_sweep
	.word test_sweep_silence
	.word test_sweep_nosilence
	.word test_relative_volume
	.word test_dsp_duty_volume_sync
	.word test_dsp_volume_phres_sync
	.word test_dmc_host_cpu
	;.word test_MAX_POWER_AHAHAHAHA
rtable_bound:
;
;
;
;
;
;
test_dsp_duty_volume_sync:
	lda #$01
	sta $4015

	lda #$30
	sta $4000
	lda #$08
	sta $4001
	lda #$FF
	sta $4002
	lda #$07
	sta $4003

	lda #$3F
	ldx #$10
test_dsp_duty_volume_sync_loop:
	sta $4000
	clc
	adc #$40
	eor #$08

	jsr delay_quick
	dex
	bne test_dsp_duty_volume_sync_loop
	rts

test_dsp_volume_phres_sync:

	lda #$01
	sta $4015

	ldx #$10
test_dsp_volume_phres_sync_loop:
	lda #$F0
	sta $4000
	jsr delay_quick
	lda #$08
	sta $4001
	lda #$FF
	sta $4002
	lda #$07
	sta $4003

	lda #$FF
	sta $4000
	jsr delay_quick

	dex
	bne test_dsp_volume_phres_sync_loop

	rts



sweep_testvals:
	.byte $80 | $00 | $71
	.byte $80 | $00 | $72
	.byte $80 | $00 | $73
	.byte $80 | $00 | $34
	.byte $80 | $00 | $35
	.byte $80 | $00 | $16
	.byte $80 | $00 | $07
	.byte $80 | $00 | $01

	.byte $80 | $08 | $71
	.byte $80 | $08 | $72
	.byte $80 | $08 | $33
	.byte $80 | $08 | $34
	.byte $80 | $08 | $15
	.byte $80 | $08 | $06
	.byte $80 | $08 | $07
	.byte $80 | $08 | $01

	.byte $00 | $00
	.byte $00 | $01
	.byte $00 | $07
	.byte $00 | $10
	.byte $00 | $11
	.byte $00 | $17
	.byte $80 | $00
	.byte $80 | $10

test_sweep:
	lda #$01
	sta $4015

	ldx #$00
sweep0_loop:
	lda #$00
	sta $4000
	sta $4001

	lda #$FF
	sta $4002

	lda sweep_testvals, X
	tay

	lda #$01
	sta $4003
	sty $4001
	lda #$BF
	sta $4000

	jsr delay_slow
	jsr delay_slow

	lda #$00
	sta $4000

	jsr delay_quick

	inx
	cpx #$18
	bne sweep0_loop
;
;
	lda #$02
	sta $4015

	ldx #$00
sweep1_loop:
	lda #$00
	sta $4004
	sta $4005

	lda #$FF
	sta $4006

	lda sweep_testvals, X
	tay

	lda #$01
	sta $4007
	sty $4005
	lda #$BF
	sta $4004

	jsr delay_slow
	jsr delay_slow

	lda #$00
	sta $4004

	jsr delay_quick

	inx
	cpx #$18
	bne sweep1_loop

;
;
;
	rts


;
;
;
;
;
test_sweep_silence:

	ldx #$00
sweep_silence_loop:
	lda #$00
	sta $4000
	sta $4001
	sta $4002
	sta $4003
	sta $4004
	sta $4005
	sta $4006
	sta $4007

	lda #$03
	sta $4015

	lda #$BF
	sta $4000, X

	lda #$00
	sta $4001, X

	lda #$00
	sta $4002, X

	lda #$04
	sta $4003, X

	jsr delay_quick

	lda #$00
	sta $4000, X

	lda #$07
	sta $4001, X

	lda #$F1
	sta $4002, X

	lda #$07
	sta $4003, X

	lda #$BF
	sta $4000, X

	jsr delay_quick

	lda #$00
	sta $4000, X

	lda #$FF
	sta $4002, X

	lda #$BF
	sta $4000, X

	jsr delay_quick

	inx
	inx
	inx
	inx

	cpx #$08
	bne sweep_silence_loop

	rts

test_sweep_nosilence:
	ldx #$00
sweep_nosilence_loop:
	lda #$00
	sta $4001
	sta $4005

	lda #$03
	sta $4015

	lda #$BF
	sta $4000, X

	lda #$00
	sta $4001, X

	lda #$FF
	sta $4002, X

	lda #$03
	sta $4003, X

	jsr delay_quick

	lda #$07
	sta $4001, X

	lda #$F0
	sta $4002, X

	lda #$07
	sta $4003, X

	jsr delay_quick

	lda #$09
	sta $4001, X

	lda #$FF
	sta $4002, X

	jsr delay_quick

	inx
	inx
	inx
	inx

	cpx #$08
	bne sweep_nosilence_loop

	rts


test_dmc_host_cpu:
	lda #$4F
	sta $4010
	lda #$80
	sta $4011
	lda #$00
	sta $4012
	lda #$00
	sta $4013

	lda #$10
	sta $4015

	jmp *

	lda #$00
	ldx #$10
	ldy #$1F
pcm_loop:
	sta $4011
	stx $4011
	sty $4011

	sta $4011
	stx $4011
	sty $4011

	sta $4011
	stx $4011
	sty $4011

	sta $4011
	stx $4011
	sty $4011

	jmp pcm_loop
	rts


test_MAX_POWER_AHAHAHAHA:
	jsr delay_slow
	;
	;
	;
	lda #$0F
	sta $4015

	lda #$00
	sta $4001
	sta $4005

	lda #$3F | $80
	sta $4000
	sta $4004
	sta $400C

	lda #$80
	sta $4002
	sta $4006

	lda #$01
	sta $4003
	sta $4007

	lda #$FF
	sta $4008

	lda #$03
	sta $400A

	lda #$02
	sta $400B

	lda #$01
	sta $400E
	sta $400F


	lda #$00
	mploop0:
	sta $4011
	eor #$7F

	ldx #$00
	mploop1:
	dex
	bne mploop1

	ldx #$40
	mploop2:
	dex
	bne mploop2

	jmp mploop0


test_relative_volume:
	lda #$0D
	sta $4015

	lda #$3F | $80
	sta $4000

	lda #$00
	sta $4001

	lda #$80
	sta $4002

	lda #$01
	sta $4003

	;jsr delay_slow

	;lda #$00
	;sta $4003
	;sta $4015
	;jsr delay_quick

	;lda #$04
	;sta $4015

	lda #$FF
	sta $4008

	lda #$00
	sta $400A

	lda #$01
	sta $400B
	;jsr delay_slow
	;lda #$00
	;sta $400B
	;sta $4015
	;jsr delay_quick

	;lda #$08
	;sta $4015

	lda #$37
	sta $400C

	lda #$00
	sta $400E

	lda #$00
	sta $400F

	jsr delay_slow
	jsr delay_slow
	jsr delay_slow
	jsr delay_slow
	jsr delay_slow
	jsr delay_slow
	jsr delay_slow
	jsr delay_slow

	lda #$00
	sta $400C

	lda #$00
	sta $4015
	jsr delay_quick

	lda #$00
	sta $4011

	jsr delay_quick

	lda #$7F
	sta $4011

	jsr delay_quick

	lda #$00
	sta $4011

	jsr delay_quick

	lda #$7F
	sta $4011

	jsr delay_quick

	lda #$00
	sta $4011

	rts

test_noise:
	lda #$08
	sta $4015

	lda #$3F
	sta $400C
	lda #$00
	sta $400F

	ldx #$00
	noise_loop:
	stx $400E
	lda #$3F
	sta $400C

	jsr delay_slow

	lda #$30
	sta $400C

	jsr delay_quick

	inx
	cpx #$10
	bne noise_loop

	ldx #$80
	noise_loop2:
	stx $400E
	lda #$3F
	sta $400C

	jsr delay_slow

	lda #$30
	sta $400C

	jsr delay_quick

	inx
	cpx #$90
	bne noise_loop2
	rts
;
;
;
;
;
;
	.dsb 256 - (* & $FF), $00
test_cpu_data0:
	.dsb 255, $00
	.byte $CB
	.byte $73
	.dsb 255, $00

#define FLAG_C $01
#define FLAG_Z $02
#define FLAG_I $04
#define FLAG_D $08
#define FLAG_B $10
#define FLAG_U $20
#define FLAG_V $40
#define FLAG_N $80

#define BCC_F bcs *+5 : jmp
#define BCS_F bcc *+5 : jmp
#define BVC_F bvs *+5 : jmp
#define BVS_F bvc *+5 : jmp
#define BNE_F beq *+5 : jmp
#define BEQ_F bne *+5 : jmp

#define REQF(v) pha : php : pla : eor #((v) | FLAG_U | FLAG_B | FLAG_I) : beq *+6 : pla : jmp test_cpu_fail : pla

#define REQFA(v,va) php : REQF(v) : cmp #(va) : beq *+6 : plp : jmp test_cpu_fail : plp

test_cpu:
	lda #$0F
	sta $4015
	lda #$40
	sta $4017

	;
	;
	;
	jsr clear_zp
	ldy #$80
	lda #$C0
	sta $FF
	lda #$FF
	sta $00
	lda #$55
	sta $40

	lda ($FF), Y
	cmp #$55
	BNE_F test_cpu_fail
	eor #$55
	BNE_F test_cpu_fail

	lda #$AA
	sta ($FF), Y
	lda $40
	cmp #$AA
	BNE_F test_cpu_fail
	eor #$AA
	BNE_F test_cpu_fail

	;
	;
	;
	jsr clear_zp

	lda #((test_cpu_data0 + 255) & $FF)
	sta $00
	lda #((test_cpu_data0 + 255) / 256)
	sta $01

	ldx #$03
	lda ($FD, X)

	cmp #$CB
	BNE_F test_cpu_fail
	eor #$CB
	BNE_F test_cpu_fail

	sta $0207
	lda #$07
	sta $00
	lda #$02
	sta $01

	ldx #$80
	lda #$BE
	sta ($80, X)

	ldy $0207
	cpy #$BE
	BNE_F test_cpu_fail
	tya
	eor #$BE
	BNE_F test_cpu_fail

	lda #$00
	sta $0207
	;
	;
	;
	jsr clear_zp

	ldx #$FF
	lda test_cpu_data0, X

	cmp #$CB
	BNE_F test_cpu_fail

	lda test_cpu_data0 + 1, X

	cmp #$73
	BNE_F test_cpu_fail

	lda #$C0
	ldx #$FF
	sta $0108, X

	lda $0207
	cmp #$C0
	BNE_F test_cpu_fail

	;
	; ADC test
	;
	clc
	lda #$00
	adc #$00
	REQFA(FLAG_Z, $00)

	adc #$7F
	REQFA(0, $7F)

	adc #$01
	REQFA(FLAG_N | FLAG_V, $80)

	adc #$7F
	REQFA(FLAG_N, $FF)

	adc #$01
	REQFA(FLAG_Z | FLAG_C, $00)

	adc #$00
	REQFA(0, $01)

	sec
	lda #$7F
	adc #$7F
	REQFA(FLAG_N | FLAG_V, $FF)

	sec
	lda #$80
	adc #$80
	REQFA(FLAG_V | FLAG_C, $01)

	sec
	lda #$80
	adc #$7F
	REQFA(FLAG_C | FLAG_Z, $00)

	;
	; SBC test
	;
	sec
	lda #$00
	sbc #$00
	REQFA(FLAG_Z | FLAG_C, $00)

	sec
	sbc #$7F
	REQFA(FLAG_N, $81)

	sbc #$00
	REQFA(FLAG_N | FLAG_C, $80)

	sbc #$01
	REQFA(FLAG_C | FLAG_V, $7F)

	sbc #$FF
	REQFA(FLAG_N | FLAG_V, $80)

	sec
	sbc #$80
	REQFA(FLAG_Z | FLAG_C, $00)

	sbc #$80
	REQFA(FLAG_N | FLAG_V, $80)

	clc
	sbc #$7F
	REQFA(FLAG_Z | FLAG_V | FLAG_C, $00)
	;
	;Success
	;
	lda #$BF
	sta $4000

	lda #$8B
	sta $4001

	lda #$80
	sta $4002
	lda #$00
	sta $4003

	jsr delay_quick

	lda #$00
	sta $4015

	rts

test_cpu_fail:
	lda #$BF
	sta $4000
	lda #$08
	sta $4001
	lda #$FF
	sta $4002
	sta $4003
	jmp *

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
.aasc "Quick APU Tester", $0
.aasc "Mr. X", $0
.aasc "20XX Wily Labs", $0
chunk_auth_bound:
;
;
;
.word (chunk_tlbl_bound - chunk_tlbl) & $FFFF, (chunk_tlbl_bound - chunk_tlbl) / $10000
.aasc "tlbl"
chunk_tlbl:
.aasc "test_cpu", $0
.aasc "test_sweep", $0
.aasc "test_sweep_silence", $0
.aasc "test_sweep_nosilence", $0
.aasc "test_relative_volume", $0
.aasc "test_dsp_duty_volume_sync", $0
.aasc "test_dsp_volume_phres_sync", $0
.aasc "test_dmc_host_cpu", $0
;.aasc "test_MAX_POWER_AHAHAHAHA", $0
chunk_tlbl_bound:

.word $0, $0
.aasc "NEND"
