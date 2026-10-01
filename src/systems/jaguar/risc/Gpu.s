	.gpu
	.org	$F03000

GPU_FLAGS	.equ	$F02100
VC_REGISTER	.equ	$F00006
OBF_REGISTER	.equ	$F00026
CLUT_BASE	.equ	$F00400
LINE_MASK	.equ	$7FF
REGISTER_PAGE	.equ	$4000
OBJECT_ENABLE	.equ	$0080
IMASK_BIT	.equ	3
OBJECT_CLEAR_BIT	.equ	12

cpuVector:
	movei	#ignoredInterrupt,r30
	jump	(r30)
	nop
	nop
	nop
	nop

dspVector:
	movei	#ignoredInterrupt,r30
	jump	(r30)
	nop
	nop
	nop
	nop

timerVector:
	movei	#ignoredInterrupt,r30
	jump	(r30)
	nop
	nop
	nop
	nop

objectVector:
	movei	#copperInterrupt,r30
	jump	(r30)
	nop
	nop
	nop
	nop

blitterVector:
	movei	#ignoredInterrupt,r30
	jump	(r30)
	nop
	nop
	nop
	nop

	.long
header:
	dc.l	start
	dc.l	copperNext
	dc.l	0
	dc.l	0

start:
	movei	#stackTop,r31
	movei	#GPU_FLAGS,r1
	movei	#REGISTER_PAGE|OBJECT_ENABLE,r0
	store	r0,(r1)
	nop
	nop
idle:
	jr	idle
	nop

copperInterrupt:
	movei	#GPU_FLAGS,r30
	load	(r30),r29
	movei	#copperNext,r10
	load	(r10),r11
	movei	#VC_REGISTER,r12
	loadw	(r12),r13
	movei	#LINE_MASK,r14
	and	r14,r13
	movei	#CLUT_BASE,r15
	movei	#nextEntry,r20
	movei	#copperDone,r21
nextEntry:
	load	(r11),r16
	or	r16,r16
	move	r16,r17
	shrq	#16,r17
	cmp	r17,r13
	jump	mi,(r21)
	nop
	addq	#4,r11
	shlq	#16,r16
	shrq	#16,r16
applyEntry:
	load	(r11),r18
	addq	#4,r11
	move	r18,r19
	shrq	#16,r19
	add	r15,r19
	storew	r18,(r19)
	subq	#1,r16
	jr	ne,applyEntry
	nop
	jump	(r20)
	nop
copperDone:
	store	r11,(r10)
	movei	#OBF_REGISTER,r12
	storew	r13,(r12)
	bclr	#IMASK_BIT,r29
	bset	#OBJECT_CLEAR_BIT,r29
	load	(r31),r28
	addq	#2,r28
	addq	#4,r31
	jump	(r28)
	store	r29,(r30)

ignoredInterrupt:
	movei	#GPU_FLAGS,r30
	load	(r30),r29
	bclr	#IMASK_BIT,r29
	movei	#$3E00,r28
	or	r28,r29
	load	(r31),r28
	addq	#2,r28
	addq	#4,r31
	jump	(r28)
	store	r29,(r30)

	.long
copperNext:
	dc.l	copperEnd
copperEnd:
	dc.l	$FFFF0000
	.long
stack:
	dc.l	0,0,0,0,0,0,0,0
stackTop:
	dc.l	0
