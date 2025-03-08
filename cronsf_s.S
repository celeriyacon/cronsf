.global _slave_wdt_interrupt_S
.global _slave_vblank_interrupt_S

.section ".text"
_slave_wdt_interrupt_S:
	mov.l r0, @-r15
	mov.l r1, @-r15
	//
	mov r15, r1
	add #12 + 3, r1
	mov.b @r1, r0

	and #0x0F, r0
	or #0x50, r0
	mov.b r0, @r1
	//
	mov.l .L_wtcsr, r1
	mov.w @r1, r0
	mov.w .L_wtcsr_v, r0
	mov.w r0, @r1
	//
	//
	mov.l @r15+, r1
	mov.l @r15+, r0

	rte
	nop
	.align 2
.L_wtcsr:
	.long 0xFFFFFE80
.L_wtcsr_v:
	.word 0xA500

_slave_vblank_interrupt_S:
	mov.l r0, @-r15
	mov.l r1, @-r15
	//
	mov r15, r1
	add #12 + 3, r1
	mov.b @r1, r0

	and #0x0F, r0
	or #0xE0, r0
	mov.b r0, @r1
	//
	mov.l @r15+, r1
	mov.l .L_slave_vblank_interrupt, r0
	jmp @r0
	mov.l @r15+, r0

	.align 2
.L_slave_vblank_interrupt:
	.long _slave_vblank_interrupt
