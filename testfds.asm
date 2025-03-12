;
;  testfds.asm
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
.byte $04	; Expansion sound chips
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

	lda #$02
	sta $4023

	lda #$80
	sta $4080
	sta $4084
	sta $4089

	lda #$7F
	sta $4011
	jsr delay_quick

	lda #$00
	sta $4011

	jmp ($0000)
ret:
	lda #$00
	sta $4015
	lda #$80
	sta $4080
	sta $4084

	lda #$7F
	sta $4011
	jsr delay_quick
	lda #$00
	sta $4011

	jmp *

rtable:
	.word test_apu_fds_sync
	.word test_apu_fds_rel_volume
	.word test_apu_fds_max_volume
	.word test_fds_saw
rtable_bound:
;
;
;
;
;
;

load_saw:
	php
	pha
	txa
	pha
	;
	;
	;
	ldx #$00
load_saw_loop:
	txa
	sta $4040, X
	inx
	cpx #$40
	bne load_saw_loop
	;
	;
	;
	pla
	tax
	pla
	plp
	rts

load_square:
	php
	pha
	txa
	pha
	;
	;
	;
	lda #$00
	ldx #$00
load_square_loop0:
	sta $4040, X
	inx
	cpx #$20
	bne load_square_loop0

	lda #$3F
load_square_loop1:
	sta $4040, X
	inx
	cpx #$40
	bne load_square_loop1
	;
	;
	;
	pla
	tax
	pla
	plp
	rts

test_apu_fds_sync:
	ldx #$00
tafs_ll:
	lda #$3F
	sta $4040, X
	inx
	cpx #$40
	bne tafs_ll

	lda #$00
	sta $4082
	lda #$00
	sta $4083

	lda #$00
	sta $4089
	;
	;
	;
	jsr clear_zp

	lda #$00
	sta $00

	lda #$00
	sta $01

	lda #$00
tams_loop:
	lda $00
	clc
	adc #$01
	sta $00
	and #$3F
	ora #$80
	tay

	lda $01
	eor #$7F
	sta $01

	sta $4011
	sty $4080

	ldx #$00
tams_delay:
	dex
	bne tams_delay
	jmp tams_loop

	rts

test_apu_fds_rel_volume:
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
	jsr load_square

	lda #$00
	sta $4082
	lda #$04
	sta $4083

	ldx #$00
frvl:
	stx $4089

	lda #$BF
	sta $4080

	jsr delay_slow

	lda #$80
	sta $4080

	jsr delay_slow

	inx
	cpx #$04
	bne frvl

	rts
;
;
;
;
;
test_apu_fds_max_volume:
	jsr delay_slow
	;
	;
	;
	jsr load_square

	lda #$21
	sta $4082
	lda #$03
	sta $4083

	lda #$00
	sta $4089

	lda #$BF
	sta $4080
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

	rts
;
;
;
;
;
test_fds_saw:
	jsr load_saw

	lda #$80
	sta $4082
	lda #$00
	sta $4083

	lda #$00
	sta $4089

	lda #$BF
	sta $4080

	jsr delay_slow

	jsr delay_slow

	rts
;
;
;
;
;

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
.aasc "Quick FDS Tester", $0
.aasc "Mr. X", $0
.aasc "20XX Wily Labs", $0
chunk_auth_bound:
;
;
;
.word (chunk_tlbl_bound - chunk_tlbl) & $FFFF, (chunk_tlbl_bound - chunk_tlbl) / $10000
.aasc "tlbl"
chunk_tlbl:
.aasc "test_apu_fds_sync", $0
.aasc "test_apu_fds_rel_volume", $0
.aasc "test_apu_fds_max_volume", $0
.aasc "test_fds_saw", $0
chunk_tlbl_bound:

.word $0, $0
.aasc "NEND"
