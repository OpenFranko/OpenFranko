	.section .text.boot,"ax"
	.globl	_start
_start:
	move.w	#0x2700,%sr
	move.l	#0x00070007,0xF0210C
	move.l	#0x00070007,0xF1A10C
	move.l	#0,0xF02114
	move.l	#0,0xF1A114
	move.w	#0xFFFF,0xF0004E
	move.w	#0x1F00,0xF000E0
	lea	__stack_top,%sp
	lea	__text_load,%a0
	lea	__text_start,%a1
	lea	__text_end,%a2
.Ltext:
	cmp.l	%a2,%a1
	bhs.s	.Ldata
	move.l	(%a0)+,(%a1)+
	bra.s	.Ltext
.Ldata:
	lea	__data_load,%a0
	lea	__data_start,%a1
	lea	__data_end,%a2
.Lcopy:
	cmp.l	%a2,%a1
	bhs.s	.Lclear
	move.l	(%a0)+,(%a1)+
	bra.s	.Lcopy
.Lclear:
	lea	__bss_start,%a1
	lea	__bss_end,%a2
	moveq	#0,%d0
.Lzero:
	cmp.l	%a2,%a1
	bhs.s	.Lrun
	move.l	%d0,(%a1)+
	bra.s	.Lzero
.Lrun:
	jsr	__libc_init_array
	clr.l	-(%sp)
	clr.l	-(%sp)
	clr.l	-(%sp)
	jsr	main
	move.l	%d0,(%sp)
	jsr	exit
.Lhalt:
	bra.s	.Lhalt
