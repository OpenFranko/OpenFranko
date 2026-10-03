	.gpu
	.org	$F03000

GPU_FLAGS	.equ	$F02100
HC_VC_REGISTER	.equ	$F00004
OBF_LONG	.equ	$F00024
CLUT_BASE	.equ	$F00400
LINE_MASK	.equ	$7FF
REGISTER_PAGE	.equ	$4000
OBJECT_ENABLE	.equ	$0080
IMASK_BIT	.equ	3
OBJECT_CLEAR_BIT	.equ	12
A1_BASE		.equ	$F02200
B_CMD		.equ	$F02238
QUEUE_MASK	.equ	63
INDEX_MASK	.equ	$FFFF
DIV_CONTROL	.equ	$F0211C
KIND_PIXELS	.equ	2
KIND_FILL	.equ	3
KIND_LZ4	.equ	4
KIND_OUTLINE	.equ	5
KIND_FLIP	.equ	6
EMPTY_SPAN	.equ	$7FFF8000
SPAN_MAX	.equ	$7FFF
SPAN_MIN	.equ	$FFFF8000
BYTE_LIMIT	.equ	255
MAX_ROWS	.equ	4000
ROOM		.equ	15984
PIXEL8		.equ	$18
WINDOW		.equ	$1800
XADDPIX		.equ	$10000
XSIGNSUB	.equ	$80000

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
	dc.l	queueRing
	dc.l	0

start:
	movei	#stackTop,r31
	movei	#GPU_FLAGS,r1
	movei	#REGISTER_PAGE|OBJECT_ENABLE,r0
	store	r0,(r1)
	nop
	nop
	movei	#DIV_CONTROL,r1
	moveq	#0,r0
	store	r0,(r1)
	movei	#queueWrite,r20
	movei	#queueRead,r21
	movei	#queueRing,r0
	load	(r0),r22
	moveq	#0,r23
	movei	#B_CMD,r24
	movei	#A1_BASE,r15
	movei	#QUEUE_MASK,r25
	movei	#INDEX_MASK,r26
	movei	#queueLoop,r27
queueLoop:
	load	(r20),r0
	cmp	r0,r23
	jr	eq,queueLoop
	nop
	move	r23,r14
	and	r25,r14
	shlq	#6,r14
	add	r22,r14
	load	(r14),r1
	load	(r14+1),r2
	load	(r14+2),r3
	load	(r14+3),r4
	load	(r14+4),r5
	load	(r14+5),r6
	load	(r14+6),r7
	load	(r14+7),r8
	load	(r14+8),r9
	load	(r14+9),r10
	moveq	#0,r28
	movei	#pixelsCommand,r0
	cmpq	#KIND_PIXELS,r1
	jump	eq,(r0)
	nop
	movei	#fillCommand,r0
	cmpq	#KIND_FILL,r1
	jump	eq,(r0)
	nop
	movei	#lz4Command,r0
	cmpq	#KIND_LZ4,r1
	jump	eq,(r0)
	nop
	movei	#outlineCommand,r0
	cmpq	#KIND_OUTLINE,r1
	jump	eq,(r0)
	nop
	movei	#flipCommand,r0
	cmpq	#KIND_FLIP,r1
	jump	eq,(r0)
	nop

phrasesCommand:
	move	r3,r0
	moveq	#1,r29
	neg	r29
	movei	#sourceCodeDone,r14
	cmpq	#8,r0
	jump	mi,(r14)
	moveq	#7,r13
	and	r0,r13
	jump	ne,(r14)
	moveq	#0,r11
	moveq	#0,r12
sourceCodeShift:
	cmpq	#8,r0
	jr	mi,sourceCodeShifted
	nop
	shrq	#1,r0
	addq	#1,r11
	shlq	#1,r12
	jr	sourceCodeShift
	bset	#0,r12
sourceCodeShifted:
	move	r3,r13
	and	r12,r13
	jump	ne,(r14)
	cmpq	#10,r11
	jump	pl,(r14)
	nop
	addq	#2,r11
	shlq	#2,r11
	moveq	#3,r13
	and	r13,r0
	move	r11,r29
	or	r0,r29
sourceCodeDone:
	move	r5,r0
	moveq	#1,r30
	neg	r30
	movei	#targetCodeDone,r14
	cmpq	#8,r0
	jump	mi,(r14)
	moveq	#7,r13
	and	r0,r13
	jump	ne,(r14)
	moveq	#0,r11
	moveq	#0,r12
targetCodeShift:
	cmpq	#8,r0
	jr	mi,targetCodeShifted
	nop
	shrq	#1,r0
	addq	#1,r11
	shlq	#1,r12
	jr	targetCodeShift
	bset	#0,r12
targetCodeShifted:
	move	r5,r13
	and	r12,r13
	jump	ne,(r14)
	cmpq	#10,r11
	jump	pl,(r14)
	nop
	addq	#2,r11
	shlq	#2,r11
	moveq	#3,r13
	and	r13,r0
	move	r11,r30
	or	r0,r30
targetCodeDone:
	movei	#PIXEL8,r11
	movei	#PIXEL8,r13
	move	r29,r0
	or	r30,r0
	jr	mi,phrasesWindow
	moveq	#0,r9
	moveq	#1,r9
	shlq	#9,r30
	or	r30,r11
	shlq	#9,r29
	jr	phrasesFlags
	or	r29,r13
phrasesWindow:
	movei	#WINDOW,r0
	or	r0,r11
	or	r0,r13
phrasesFlags:
	moveq	#7,r0
	move	r2,r19
	and	r0,r19
	move	r4,r17
	and	r0,r17
	move	r17,r30
	add	r6,r30
	addq	#7,r30
	not	r0
	and	r0,r30
	move	r30,r29
	cmp	r17,r19
	jr	mi,phrasesNoExtra
	nop
	jr	eq,phrasesNoExtra
	nop
	addq	#8,r29
	bset	#2,r8
phrasesNoExtra:
	movei	#phrasesLinear,r0
	cmpq	#0,r9
	jump	eq,(r0)
	nop
	move	r19,r18
	move	r17,r12
	sub	r30,r12
	move	r19,r16
	sub	r29,r16
	movei	#$FFFF,r0
	and	r0,r12
	and	r0,r16
	movei	#$10000,r0
	or	r0,r12
	or	r0,r16
	movei	#MAX_ROWS,r1
	movei	#phrasesLoop,r0
	jump	(r0)
	nop
phrasesLinear:
	move	r17,r12
	add	r5,r12
	sub	r30,r12
	move	r19,r16
	add	r3,r16
	sub	r29,r16
	movei	#$FFFF,r0
	and	r0,r12
	and	r0,r16
	move	r3,r29
	abs	r29
	cmp	r5,r29
	jr	pl,phrasesSpan
	nop
	move	r5,r29
phrasesSpan:
	movei	#MAX_ROWS,r1
	cmpq	#0,r29
	jr	eq,phrasesRowsDone
	nop
	movei	#ROOM,r0
	sub	r6,r0
	jr	mi,phrasesRowsOne
	nop
	div	r29,r0
	addq	#1,r0
	cmp	r0,r1
	jr	mi,phrasesRowsDone
	nop
	jr	phrasesRowsDone
	move	r0,r1
phrasesRowsOne:
	moveq	#1,r1
phrasesRowsDone:
phrasesLoop:
	move	r7,r29
	sub	r28,r29
	cmp	r1,r29
	jr	mi,phrasesRowsReady
	nop
	move	r1,r29
phrasesRowsReady:
	move	r29,r30
	subq	#1,r30
	imult	r3,r30
	move	r2,r31
	cmpq	#0,r3
	jr	pl,phrasesLowest
	nop
	add	r30,r31
phrasesLowest:
	cmpq	#0,r9
	jr	ne,phrasesPixelReady
	nop
	move	r2,r18
	sub	r31,r18
	add	r19,r18
phrasesPixelReady:
phrasesWait:
	load	(r24),r0
	btst	#0,r0
	jr	eq,phrasesWait
	nop
	moveq	#7,r0
	not	r0
	move	r4,r14
	and	r0,r14
	and	r0,r31
	store	r14,(r15)
	store	r11,(r15+1)
	moveq	#0,r0
	store	r0,(r15+2)
	store	r17,(r15+3)
	store	r12,(r15+4)
	store	r31,(r15+9)
	store	r13,(r15+10)
	store	r18,(r15+12)
	store	r16,(r15+13)
	store	r10,(r15+26)
	store	r10,(r15+27)
	move	r29,r0
	shlq	#16,r0
	or	r6,r0
	store	r0,(r15+15)
	store	r8,(r15+14)
	add	r30,r2
	add	r3,r2
	move	r29,r0
	imult	r5,r0
	add	r0,r4
	add	r29,r28
	cmp	r7,r28
	movei	#phrasesLoop,r0
	jump	mi,(r0)
	nop
	movei	#commandDone,r0
	jump	(r0)
	nop

pixelsCommand:
	move	r3,r29
	abs	r29
	cmp	r5,r29
	jr	pl,pixelsSpan
	nop
	move	r5,r29
pixelsSpan:
	movei	#MAX_ROWS,r1
	cmpq	#0,r29
	jr	eq,pixelsRowsDone
	nop
	movei	#ROOM,r0
	sub	r6,r0
	jr	mi,pixelsRowsOne
	nop
	div	r29,r0
	addq	#1,r0
	cmp	r0,r1
	jr	mi,pixelsRowsDone
	nop
	jr	pixelsRowsDone
	move	r0,r1
pixelsRowsOne:
	moveq	#1,r1
pixelsRowsDone:
	movei	#PIXEL8|WINDOW|XADDPIX,r11
	move	r11,r13
	move	r5,r12
	cmpq	#0,r9
	jr	eq,pixelsForward
	nop
	movei	#XSIGNSUB,r0
	or	r0,r11
	jr	pixelsSteps
	add	r6,r12
pixelsForward:
	sub	r6,r12
pixelsSteps:
	move	r3,r16
	sub	r6,r16
	movei	#$FFFF,r0
	and	r0,r12
	and	r0,r16
	moveq	#0,r19
	cmpq	#0,r9
	jr	eq,pixelsLoop
	nop
	move	r6,r19
	subq	#1,r19
pixelsLoop:
	move	r7,r29
	sub	r28,r29
	cmp	r1,r29
	jr	mi,pixelsRowsReady
	nop
	move	r1,r29
pixelsRowsReady:
	move	r29,r30
	subq	#1,r30
	imult	r3,r30
	move	r2,r31
	cmpq	#0,r3
	jr	pl,pixelsLowest
	nop
	add	r30,r31
pixelsLowest:
	moveq	#7,r0
	move	r4,r17
	and	r0,r17
	add	r19,r17
	move	r31,r18
	and	r0,r18
	add	r2,r18
	sub	r31,r18
pixelsWait:
	load	(r24),r0
	btst	#0,r0
	jr	eq,pixelsWait
	nop
	moveq	#7,r0
	not	r0
	move	r4,r14
	and	r0,r14
	and	r0,r31
	store	r14,(r15)
	store	r11,(r15+1)
	moveq	#0,r0
	store	r0,(r15+2)
	store	r17,(r15+3)
	store	r12,(r15+4)
	store	r31,(r15+9)
	store	r13,(r15+10)
	store	r18,(r15+12)
	store	r16,(r15+13)
	store	r10,(r15+26)
	store	r10,(r15+27)
	move	r29,r0
	shlq	#16,r0
	or	r6,r0
	store	r0,(r15+15)
	store	r8,(r15+14)
	add	r30,r2
	add	r3,r2
	move	r29,r0
	imult	r5,r0
	add	r0,r4
	add	r29,r28
	cmp	r7,r28
	movei	#pixelsLoop,r0
	jump	mi,(r0)
	nop
	movei	#commandDone,r0
	jump	(r0)
	nop

fillCommand:
	move	r5,r0
	moveq	#1,r29
	neg	r29
	movei	#fillCodeDone,r14
	cmpq	#8,r0
	jump	mi,(r14)
	moveq	#7,r13
	and	r0,r13
	jump	ne,(r14)
	moveq	#0,r11
	moveq	#0,r12
fillCodeShift:
	cmpq	#8,r0
	jr	mi,fillCodeShifted
	nop
	shrq	#1,r0
	addq	#1,r11
	shlq	#1,r12
	jr	fillCodeShift
	bset	#0,r12
fillCodeShifted:
	move	r5,r13
	and	r12,r13
	jump	ne,(r14)
	cmpq	#10,r11
	jump	pl,(r14)
	nop
	addq	#2,r11
	shlq	#2,r11
	moveq	#3,r13
	and	r13,r0
	move	r11,r29
	or	r0,r29
fillCodeDone:
	moveq	#7,r0
	move	r4,r17
	and	r0,r17
	move	r17,r30
	add	r6,r30
	addq	#7,r30
	not	r0
	and	r0,r30
	movei	#PIXEL8,r11
	movei	#fillLinear,r0
	btst	#31,r29
	jump	ne,(r0)
	nop
	shlq	#9,r29
	or	r29,r11
	move	r17,r12
	sub	r30,r12
	movei	#$FFFF,r0
	and	r0,r12
	movei	#$10000,r0
	or	r0,r12
	movei	#MAX_ROWS,r1
	movei	#fillLoop,r0
	jump	(r0)
	nop
fillLinear:
	movei	#WINDOW,r0
	or	r0,r11
	move	r17,r12
	add	r5,r12
	sub	r30,r12
	movei	#$FFFF,r0
	and	r0,r12
	move	r5,r29
	movei	#MAX_ROWS,r1
	cmpq	#0,r29
	jr	eq,fillRowsDone
	nop
	movei	#ROOM,r0
	sub	r6,r0
	jr	mi,fillRowsOne
	nop
	div	r29,r0
	addq	#1,r0
	cmp	r0,r1
	jr	mi,fillRowsDone
	nop
	jr	fillRowsDone
	move	r0,r1
fillRowsOne:
	moveq	#1,r1
fillRowsDone:
fillLoop:
	move	r7,r29
	sub	r28,r29
	cmp	r1,r29
	jr	mi,fillRowsReady
	nop
	move	r1,r29
fillRowsReady:
fillWait:
	load	(r24),r0
	btst	#0,r0
	jr	eq,fillWait
	nop
	moveq	#7,r0
	not	r0
	move	r4,r14
	and	r0,r14
	store	r14,(r15)
	store	r11,(r15+1)
	moveq	#0,r0
	store	r0,(r15+2)
	store	r17,(r15+3)
	store	r12,(r15+4)
	store	r10,(r15+26)
	store	r10,(r15+27)
	move	r29,r0
	shlq	#16,r0
	or	r6,r0
	store	r0,(r15+15)
	store	r8,(r15+14)
	move	r29,r0
	imult	r5,r0
	add	r0,r4
	add	r29,r28
	cmp	r7,r28
	movei	#fillLoop,r0
	jump	mi,(r0)
	nop
	movei	#commandDone,r0
	jump	(r0)
	nop

lz4Command:
	load	(r14+6),r13
	movei	#BYTE_LIMIT,r9
	movei	#lz4Token,r28
	movei	#lz4Done,r29
lz4Token:
	cmp	r3,r2
	jump	cc,(r29)
	nop
	cmp	r13,r4
	jump	cc,(r29)
	nop
	loadb	(r2),r6
	addq	#1,r2
	move	r6,r7
	shrq	#4,r7
	cmpq	#15,r7
	jr	ne,lz4Literals
	nop
lz4LiteralExtension:
	loadb	(r2),r8
	addq	#1,r2
	cmp	r9,r8
	jr	eq,lz4LiteralExtension
	add	r8,r7
lz4Literals:
	move	r4,r0
	add	r7,r0
	cmp	r0,r5
	jump	cs,(r29)
	cmpq	#0,r7
	jr	eq,lz4LiteralsDone
	nop
lz4LiteralCopy:
	loadb	(r2),r8
	addq	#1,r2
	storeb	r8,(r4)
	subq	#1,r7
	jr	ne,lz4LiteralCopy
	addq	#1,r4
lz4LiteralsDone:
	cmp	r3,r2
	jump	cc,(r29)
	nop
	loadb	(r2),r10
	addq	#1,r2
	loadb	(r2),r11
	addq	#1,r2
	shlq	#8,r11
	or	r11,r10
	move	r4,r12
	sub	r10,r12
	moveq	#15,r11
	and	r11,r6
	cmpq	#15,r6
	jr	ne,lz4MatchChecked
	addq	#4,r6
lz4MatchExtension:
	loadb	(r2),r8
	addq	#1,r2
	cmp	r9,r8
	jr	eq,lz4MatchExtension
	add	r8,r6
lz4MatchChecked:
	move	r4,r0
	add	r6,r0
	cmp	r0,r5
	jump	cs,(r29)
	nop
lz4MatchCopy:
	loadb	(r12),r8
	addq	#1,r12
	storeb	r8,(r4)
	subq	#1,r6
	jr	ne,lz4MatchCopy
	addq	#1,r4
	jump	(r28)
	nop
lz4Done:
	store	r4,(r14+5)
	store	r2,(r14+7)
	movei	#commandDone,r0
	jump	(r0)
	nop

outlineCommand:
	movei	#EMPTY_SPAN,r11
	movei	#SPAN_MAX,r12
	movei	#SPAN_MIN,r13
	moveq	#0,r16
	subq	#1,r16
	moveq	#0,r17
	moveq	#0,r18
	move	r5,r19
	movei	#outlineRow,r28
	movei	#outlineBox,r29
	movei	#outlineEmpty,r1
outlineRow:
	cmpq	#0,r4
	jump	eq,(r29)
	move	r2,r8
	move	r2,r7
	add	r3,r7
outlineLeft:
	cmp	r8,r7
	jump	eq,(r1)
	nop
	loadb	(r8),r10
	cmpq	#0,r10
	jr	eq,outlineLeft
	addq	#1,r8
	move	r7,r9
outlineRight:
	subq	#1,r9
	loadb	(r9),r10
	cmpq	#0,r10
	jr	eq,outlineRight
	nop
	subq	#1,r8
	sub	r2,r8
	sub	r2,r9
	cmp	r12,r8
	jr	pl,outlineKeepLeft
	nop
	move	r8,r12
outlineKeepLeft:
	cmp	r9,r13
	jr	pl,outlineKeepRight
	nop
	move	r9,r13
outlineKeepRight:
	cmpq	#0,r16
	jr	pl,outlineHaveTop
	nop
	move	r18,r16
outlineHaveTop:
	move	r18,r17
	addq	#1,r17
	shlq	#16,r8
	or	r9,r8
	store	r8,(r19)
	jr	outlineNext
	addq	#4,r19
outlineEmpty:
	store	r11,(r19)
	addq	#4,r19
outlineNext:
	move	r7,r2
	addq	#1,r18
	jump	(r28)
	subq	#1,r4
outlineBox:
	cmpq	#0,r16
	jr	pl,outlineBoxFound
	nop
	moveq	#0,r12
	moveq	#0,r13
	moveq	#0,r16
	jr	outlineBoxStore
	moveq	#0,r17
outlineBoxFound:
	addq	#1,r13
outlineBoxStore:
	store	r12,(r14+6)
	store	r16,(r14+7)
	store	r13,(r14+8)
	store	r17,(r14+9)
	load	(r14+3),r4
	move	r5,r8
	movei	#outlineBandRow,r28
	movei	#commandDone,r29
	movei	#outlineBandInner,r16
outlineBandRow:
	cmpq	#0,r4
	jump	eq,(r29)
	moveq	#4,r10
	cmp	r4,r10
	jr	mi,outlineBandCount
	nop
	move	r4,r10
outlineBandCount:
	movei	#SPAN_MAX,r12
	movei	#SPAN_MIN,r13
	move	r8,r9
outlineBandInner:
	load	(r9),r0
	addq	#4,r9
	move	r0,r1
	sharq	#16,r1
	shlq	#16,r0
	sharq	#16,r0
	cmp	r12,r1
	jr	pl,outlineBandKeepFirst
	nop
	move	r1,r12
outlineBandKeepFirst:
	cmp	r0,r13
	jr	pl,outlineBandKeepLast
	nop
	move	r0,r13
outlineBandKeepLast:
	subq	#1,r10
	jump	ne,(r16)
	nop
	shlq	#16,r12
	shlq	#16,r13
	shrq	#16,r13
	or	r13,r12
	store	r12,(r6)
	addq	#4,r6
	addq	#4,r8
	jump	(r28)
	subq	#1,r4

flipCommand:
	movei	#commandDone,r29
	cmpq	#0,r4
	jump	eq,(r29)
	nop
flipLoop:
	load	(r2),r7
	addq	#4,r2
	and	r6,r7
	xor	r5,r7
	subq	#1,r4
	store	r7,(r3)
	jr	ne,flipLoop
	addq	#4,r3
	jump	(r29)
	nop

commandDone:
	addq	#1,r23
	and	r26,r23
	jump	(r27)
	store	r23,(r21)

copperInterrupt:
	movei	#GPU_FLAGS,r30
	load	(r30),r29
	movei	#copperNext,r10
	load	(r10),r11
	movei	#HC_VC_REGISTER,r12
	load	(r12),r13
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
	load	(r11),r19
	addq	#4,r11
	load	(r11),r18
	addq	#4,r11
	add	r15,r19
	or	r18,r18
	store	r18,(r19)
	subq	#1,r16
	jr	ne,applyEntry
	nop
	jump	(r20)
	nop
copperDone:
	store	r11,(r10)
	movei	#OBF_LONG,r12
	store	r13,(r12)
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
queueRing:
	dc.l	0
queueWrite:
	dc.l	0
queueRead:
	dc.l	0

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
