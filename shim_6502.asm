;
;  shim_6502.asm
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

* = $2000

INIT_ADDR	= $3EF8
PLAY_ADDR	= $3EFC
SONG_NUMBER	= $3EFE
FRAME_PENDING	= $3EFF

.dsb $2800 - *, $00
reset:
	sei
	cld
	ldx #$FF
	txs

	lda #$0F
	sta $4015

song_lda:lda #$00
pal_ldx:ldx #$00
	ldy #$00
init_jsr:jsr $0000
	lda FRAME_PENDING
	bpl play_loop
	.byte $F2
	jmp special_lda
play_loop:
	.byte $F2
	lda FRAME_PENDING
	beq play_loop
	cmp #$80
	beq special_lda
	;
	pha
	lda #$00
	tax
	tay
play_jsr:jsr $0000
	pla
	bpl play_loop
special_lda: lda #$00
	ldx #$00
	ldy #$00
	clc
	clv
special_jsr: jsr $0000
	jmp play_loop

.dsb $2FF0 - *, $00
.word reset
.word song_lda + 1
.word pal_ldx + 1
.word init_jsr + 1
.word play_jsr + 1
.dsb $2FFC - *, $00
.word special_lda + 1
.word special_jsr + 1
.dsb $3000 - *, $00
