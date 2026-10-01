	.text
	.globl	jaguarLevel0Interrupt
	.globl	jaguarExceptionStubs

jaguarLevel0Interrupt:
	movem.l	%d0-%d1/%a0-%a1,-(%sp)
	moveq	#0,%d0
	move.w	0xF000E0,%d0
	move.l	18(%sp),-(%sp)
	move.l	%d0,-(%sp)
	jsr	jaguarDispatchInterrupt
	addq.l	#8,%sp
	movem.l	(%sp)+,%d0-%d1/%a0-%a1
	rte

	.macro	exceptionStub number
	.balign	16
	move.l	#\number,-(%sp)
	jmp	jaguarExceptionEntry
	.endm

	.balign	16
jaguarExceptionStubs:
	.irp	number,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
	exceptionStub \number
	.endr
	.irp	number,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63
	exceptionStub \number
	.endr

jaguarExceptionEntry:
	move.w	#0x2700,%sr
	movem.l	%d0-%d7/%a0-%a7,-(%sp)
	move.l	%sp,-(%sp)
	jsr	jaguarFatalException
.Lstop:
	bra.s	.Lstop
