	.text
	.globl	jaguarCopyForward
	.globl	jaguarCopyBackward
	.globl	jaguarFill

jaguarCopyForward:
	move.l	4(%sp),%a1
	move.l	8(%sp),%a0
	move.l	12(%sp),%d0
	beq	.Lforward_done
	move.l	%a0,%d1
	sub.l	%a1,%d1
	btst	#0,%d1
	bne	.Lforward_bytes
	move.w	%a0,%d1
	btst	#0,%d1
	beq	.Lforward_even
	move.b	(%a0)+,(%a1)+
	subq.l	#1,%d0
.Lforward_even:
	cmp.l	#64,%d0
	bcs	.Lforward_longs
	movem.l	%d2-%d7/%a2-%a6,-(%sp)
.Lforward_block:
	movem.l	(%a0)+,%d1-%d7/%a2-%a6
	movem.l	%d1-%d7/%a2-%a6,(%a1)
	lea	48(%a1),%a1
	sub.l	#48,%d0
	cmp.l	#48,%d0
	bcc	.Lforward_block
	movem.l	(%sp)+,%d2-%d7/%a2-%a6
.Lforward_longs:
	move.l	%d0,%d1
	lsr.l	#2,%d1
	beq	.Lforward_tail
	subq.w	#1,%d1
.Lforward_long:
	move.l	(%a0)+,(%a1)+
	dbra	%d1,.Lforward_long
.Lforward_tail:
	and.w	#3,%d0
	beq	.Lforward_done
	subq.w	#1,%d0
.Lforward_tail_byte:
	move.b	(%a0)+,(%a1)+
	dbra	%d0,.Lforward_tail_byte
.Lforward_done:
	rts
.Lforward_bytes:
	move.l	%d0,%d1
	lsr.l	#3,%d1
	beq	.Lforward_rest
	subq.l	#1,%d1
.Lforward_eight:
	move.b	(%a0)+,(%a1)+
	move.b	(%a0)+,(%a1)+
	move.b	(%a0)+,(%a1)+
	move.b	(%a0)+,(%a1)+
	move.b	(%a0)+,(%a1)+
	move.b	(%a0)+,(%a1)+
	move.b	(%a0)+,(%a1)+
	move.b	(%a0)+,(%a1)+
	subq.l	#1,%d1
	bcc	.Lforward_eight
.Lforward_rest:
	and.w	#7,%d0
	beq	.Lforward_done
	subq.w	#1,%d0
.Lforward_rest_byte:
	move.b	(%a0)+,(%a1)+
	dbra	%d0,.Lforward_rest_byte
	rts

jaguarCopyBackward:
	move.l	4(%sp),%a1
	move.l	8(%sp),%a0
	move.l	12(%sp),%d0
	beq	.Lbackward_done
	add.l	%d0,%a0
	add.l	%d0,%a1
	move.l	%a0,%d1
	sub.l	%a1,%d1
	btst	#0,%d1
	bne	.Lbackward_bytes
	move.w	%a0,%d1
	btst	#0,%d1
	beq	.Lbackward_even
	move.b	-(%a0),-(%a1)
	subq.l	#1,%d0
.Lbackward_even:
	move.l	%d0,%d1
	lsr.l	#2,%d1
	beq	.Lbackward_tail
	subq.l	#1,%d1
.Lbackward_long:
	move.l	-(%a0),-(%a1)
	subq.l	#1,%d1
	bcc	.Lbackward_long
.Lbackward_tail:
	and.w	#3,%d0
	beq	.Lbackward_done
	subq.w	#1,%d0
.Lbackward_tail_byte:
	move.b	-(%a0),-(%a1)
	dbra	%d0,.Lbackward_tail_byte
.Lbackward_done:
	rts
.Lbackward_bytes:
	subq.l	#1,%d0
.Lbackward_byte:
	move.b	-(%a0),-(%a1)
	subq.l	#1,%d0
	bcc	.Lbackward_byte
	rts

jaguarFill:
	move.l	4(%sp),%a1
	move.l	12(%sp),%d0
	beq	.Lfill_done
	move.b	11(%sp),%d1
	lsl.w	#8,%d1
	move.b	11(%sp),%d1
	move.l	%d1,%a0
	swap	%d1
	move.w	%a0,%d1
	exg	%d0,%a0
	move.w	%a1,%d0
	btst	#0,%d0
	exg	%d0,%a0
	beq	.Lfill_even
	move.b	%d1,(%a1)+
	subq.l	#1,%d0
.Lfill_even:
	cmp.l	#64,%d0
	bcs	.Lfill_longs
	movem.l	%d2-%d7/%a2-%a6,-(%sp)
	move.l	%d1,%d2
	move.l	%d1,%d3
	move.l	%d1,%d4
	move.l	%d1,%d5
	move.l	%d1,%d6
	move.l	%d1,%d7
	move.l	%d1,%a0
	move.l	%d1,%a2
	move.l	%d1,%a3
	move.l	%d1,%a4
	move.l	%d1,%a5
	move.l	%d1,%a6
.Lfill_block:
	movem.l	%d1-%d7/%a0/%a2-%a6,(%a1)
	lea	52(%a1),%a1
	sub.l	#52,%d0
	cmp.l	#52,%d0
	bcc	.Lfill_block
	movem.l	(%sp)+,%d2-%d7/%a2-%a6
.Lfill_longs:
	move.l	%d0,%a0
	lsr.l	#2,%d0
	beq	.Lfill_tail
	subq.l	#1,%d0
.Lfill_long:
	move.l	%d1,(%a1)+
	subq.l	#1,%d0
	bcc	.Lfill_long
.Lfill_tail:
	move.l	%a0,%d0
	and.w	#3,%d0
	beq	.Lfill_done
	subq.w	#1,%d0
.Lfill_byte:
	move.b	%d1,(%a1)+
	dbra	%d0,.Lfill_byte
.Lfill_done:
	rts
