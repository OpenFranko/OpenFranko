	.dsp
	.org	$F1B000

DSP_FLAGS	.equ	$F1A100
SCLK		.equ	$F1A150
SMODE		.equ	$F1A154
RIGHT_DAC	.equ	$F1A148
LEFT_DAC	.equ	$F1A14C
I2S_ENABLE	.equ	$0020
IMASK_BIT	.equ	3
I2S_CLEAR_BIT	.equ	10
SERIAL_MODE	.equ	$15

RING_MASK	.equ	$3FC
BLOCK_FRAMES	.equ	64
BLOCK_BYTES	.equ	256
MUSIC_VOICES	.equ	4
SFX_VOICES	.equ	4
SFX_GAIN	.equ	112
FILTER_SHIFT	.equ	12
LAST_FRAMES_OFFSET	.equ	496

SHARED_GAIN		.equ	0
SHARED_FILTER		.equ	4
SHARED_TICK_WRITE	.equ	8
SHARED_FLUSH_SEQ	.equ	12
SHARED_FLUSH_INDEX	.equ	16
SHARED_LOOP_MASK	.equ	20
SHARED_SFX_SEQ		.equ	24
SHARED_SFX_COMMAND	.equ	40
SHARED_COEFFICIENTS	.equ	168
SHARED_TICK_READ	.equ	188
SHARED_SFX_ACTIVE	.equ	192
SHARED_FRAMES		.equ	196
SHARED_TICKS		.equ	256
TICK_MASK		.equ	31

cpuVector:
	movei	#ignoredInterrupt,r30
	jump	(r30)
	nop
	nop
	nop
	nop

i2sVector:
	movei	#i2sInterrupt,r30
	jump	(r30)
	nop
	nop
	nop
	nop

	.rept	4
	movei	#ignoredInterrupt,r30
	jump	(r30)
	nop
	nop
	nop
	nop
	.endr

	.long
header:
	dc.l	start
	dc.l	config
	dc.l	0
	dc.l	0

config:
sharedAddress:
	dc.l	0
clockDivider:
	dc.l	18
stepHigh:
	dc.l	0
stepLow:
	dc.l	0

i2sInterrupt:
	movei	#DSP_FLAGS,r30
	load	(r30),r29
	move	r20,r28
	add	r21,r28
	load	(r28),r28
	store	r28,(r23)
	sharq	#16,r28
	store	r28,(r22)
	addq	#4,r20
	and	r27,r20
	bclr	#IMASK_BIT,r29
	bset	#I2S_CLEAR_BIT,r29
	load	(r31),r28
	addq	#2,r28
	addq	#4,r31
	jump	(r28)
	store	r29,(r30)

ignoredInterrupt:
	movei	#DSP_FLAGS,r30
	load	(r30),r29
	bclr	#IMASK_BIT,r29
	movei	#$3E00,r28
	or	r28,r29
	load	(r31),r28
	addq	#2,r28
	addq	#4,r31
	jump	(r28)
	store	r29,(r30)

start:
	movei	#stackTop,r31
	movei	#ring,r21
	moveq	#0,r20
	movei	#RING_MASK,r27
	movei	#LEFT_DAC,r22
	movei	#RIGHT_DAC,r23
	movei	#dataStart,r0
	movei	#dataEnd,r1
	moveq	#0,r2
clearData:
	store	r2,(r0)
	addq	#4,r0
	cmp	r1,r0
	jr	mi,clearData
	nop
	movei	#sharedAddress,r0
	load	(r0),r10
	movei	#accumulator,r11
	moveq	#0,r13
	moveq	#0,r16
	move	r10,r14
	nop
	load	(r14+3),r19
	load	(r14+4),r24
	movei	#clockDivider,r0
	load	(r0),r1
	movei	#SCLK,r0
	store	r1,(r0)
	movei	#SMODE,r0
	movei	#SERIAL_MODE,r1
	store	r1,(r0)
	movei	#DSP_FLAGS,r0
	movei	#I2S_ENABLE,r1
	store	r1,(r0)
	nop
	nop

mainLoop:
	move	r20,r0
	sub	r13,r0
	subq	#4,r0
	and	r27,r0
	movei	#BLOCK_BYTES,r1
	cmp	r1,r0
	jr	mi,mainLoop
	nop

	move	r11,r0
	movei	#BLOCK_FRAMES*2,r1
	moveq	#0,r2
clearAccumulator:
	store	r2,(r0)
	subq	#1,r1
	jr	ne,clearAccumulator
	addq	#4,r0

	move	r10,r14
	nop
	load	(r14+3),r0
	cmp	r19,r0
	jr	eq,noFlush
	nop
	move	r0,r19
	nop
	load	(r14+4),r24
	moveq	#0,r16
	movei	#voices,r14
	moveq	#0,r1
	nop
	store	r1,(r14+7)
	store	r1,(r14+15)
	store	r1,(r14+23)
	store	r1,(r14+31)
noFlush:
	moveq	#0,r5
	movei	#sfxCommandLoop,r26
sfxCommandLoop:
	move	r5,r0
	shlq	#2,r0
	move	r10,r1
	addq	#SHARED_SFX_SEQ,r1
	add	r0,r1
	load	(r1),r2
	movei	#sfxSeqs,r3
	add	r0,r3
	load	(r3),r4
	movei	#sfxNoCommand,r6
	cmp	r4,r2
	jump	eq,(r6)
	nop
	store	r2,(r3)
	move	r5,r1
	shlq	#5,r1
	movei	#SHARED_SFX_COMMAND,r6
	add	r6,r1
	add	r10,r1
	move	r1,r15
	move	r5,r14
	addq	#MUSIC_VOICES,r14
	shlq	#5,r14
	movei	#voices,r6
	add	r6,r14
	load	(r15),r6
	store	r6,(r14)
	moveq	#0,r7
	nop
	store	r7,(r14+1)
	load	(r15+1),r7
	store	r7,(r14+4)
	load	(r15+2),r8
	store	r8,(r14+5)
	load	(r15+3),r8
	store	r8,(r14+2)
	load	(r15+4),r8
	store	r8,(r14+3)
	load	(r15+5),r8
	store	r8,(r14+6)
	move	r6,r8
	sub	r7,r8
	shrq	#31,r8
	store	r8,(r14+7)
sfxNoCommand:
	addq	#1,r5
	cmpq	#SFX_VOICES,r5
	jump	ne,(r26)
	nop

	move	r10,r14
	nop
	load	(r14+5),r0
	movei	#sfxVoices,r14
	moveq	#0,r1
	moveq	#SFX_VOICES,r2
sfxLoopMask:
	btst	#0,r0
	jr	ne,sfxKeepLoop
	nop
	move	r14,r3
	addq	#20,r3
	store	r1,(r3)
sfxKeepLoop:
	shrq	#1,r0
	addq	#32,r14
	subq	#1,r2
	jr	ne,sfxLoopMask
	nop

	movei	#BLOCK_FRAMES,r18
	moveq	#0,r17
chunkLoop:
	movei	#chunkMix,r0
	cmpq	#0,r16
	jr	eq,needTick
	nop
	jump	pl,(r0)
	nop
needTick:
	move	r10,r14
	nop
	load	(r14+2),r0
	shlq	#16,r0
	move	r24,r1
	shlq	#16,r1
	cmp	r1,r0
	jr	ne,haveTick
	nop
	move	r18,r16
	shlq	#16,r16
	movei	#chunkMix,r0
	jump	(r0)
	nop
haveTick:
	move	r24,r0
	moveq	#TICK_MASK,r1
	and	r1,r0
	move	r0,r1
	shlq	#7,r1
	shlq	#5,r0
	add	r1,r0
	movei	#SHARED_TICKS,r1
	add	r1,r0
	add	r10,r0
	move	r0,r15
	load	(r15),r1
	add	r1,r16
	addq	#1,r24
	addq	#32,r15
	movei	#voices,r14
	movei	#voicePeriods,r12
	moveq	#MUSIC_VOICES,r9
	movei	#applyVoice,r26
applyVoice:
	load	(r15),r1
	load	(r15+1),r2
	btst	#0,r1
	jr	eq,applyNoTrigger
	nop
	store	r2,(r14)
	moveq	#0,r3
	nop
	store	r3,(r14+1)
applyNoTrigger:
	nop
	load	(r15+2),r3
	store	r3,(r14+4)
	load	(r15+3),r3
	store	r3,(r14+5)
	load	(r15+4),r3
	load	(r12),r5
	cmp	r3,r5
	movei	#applySameStep,r0
	jump	eq,(r0)
	cmpq	#0,r3
	jump	eq,(r0)
	nop
	store	r3,(r12)
	movei	#stepHigh,r0
	load	(r0),r5
	movei	#stepLow,r0
	load	(r0),r6
	moveq	#0,r0
	moveq	#0,r7
	moveq	#0,r8
	movei	#64,r2
	movei	#divideStep,r25
divideStep:
	move	r5,r4
	shrq	#31,r4
	shlq	#1,r0
	or	r4,r0
	move	r6,r4
	shrq	#31,r4
	shlq	#1,r5
	or	r4,r5
	shlq	#1,r6
	move	r8,r4
	shrq	#31,r4
	shlq	#1,r7
	or	r4,r7
	shlq	#1,r8
	cmp	r3,r0
	jr	cs,divideNext
	nop
	sub	r3,r0
	addq	#1,r8
divideNext:
	subq	#1,r2
	jump	ne,(r25)
	nop
	store	r7,(r14+2)
	store	r8,(r14+3)
applySameStep:
	load	(r15+6),r3
	store	r3,(r14+6)
	move	r1,r4
	shrq	#1,r4
	moveq	#1,r5
	and	r5,r4
	btst	#0,r1
	jr	ne,applyActive
	nop
	load	(r14+7),r6
	and	r6,r4
applyActive:
	nop
	store	r4,(r14+7)
	addq	#4,r12
	addq	#32,r15
	addq	#32,r14
	subq	#1,r9
	jump	ne,(r26)
	nop
	movei	#chunkLoop,r0
	jump	(r0)
	nop

chunkMix:
	move	r16,r0
	movei	#$FFFF,r1
	add	r1,r0
	shrq	#16,r0
	cmp	r18,r0
	jr	mi,chunkSmaller
	nop
	move	r18,r0
chunkSmaller:
	movei	#chunkFrames,r1
	store	r0,(r1)
	movei	#voices,r1
	movei	#voiceCursor,r2
	store	r1,(r2)
	moveq	#MUSIC_VOICES,r1
	movei	#voiceCounter,r2
	store	r1,(r2)

musicVoiceLoop:
	movei	#voiceCursor,r2
	load	(r2),r14
	nop
	load	(r14+7),r25
	cmpq	#0,r25
	movei	#musicVoiceNext,r0
	jump	eq,(r0)
	nop
	load	(r14),r0
	load	(r14+1),r1
	load	(r14+2),r2
	load	(r14+3),r3
	load	(r14+4),r4
	load	(r14+5),r5
	load	(r14+6),r6
	movei	#chunkFrames,r7
	load	(r7),r7
	move	r17,r8
	shlq	#3,r8
	add	r11,r8
	movei	#musicFrame,r26
musicFrame:
	loadb	(r0),r9
	move	r0,r14
	addq	#1,r14
	loadb	(r14),r12
	shlq	#24,r9
	sharq	#24,r9
	shlq	#24,r12
	sharq	#24,r12
	sub	r9,r12
	move	r1,r14
	shrq	#17,r14
	imult	r14,r12
	sharq	#7,r12
	shlq	#8,r9
	add	r12,r9
	load	(r8),r12
	move	r6,r14
	shrq	#16,r14
	imult	r9,r14
	add	r14,r12
	store	r12,(r8)
	addq	#4,r8
	load	(r8),r12
	move	r6,r14
	imult	r9,r14
	add	r14,r12
	store	r12,(r8)
	addq	#4,r8
	add	r3,r1
	addc	r2,r0
	cmp	r4,r0
	jr	mi,musicNoEnd
	nop
	cmpq	#0,r5
	jr	eq,musicStop
	nop
	sub	r5,r0
musicNoEnd:
	subq	#1,r7
	jump	ne,(r26)
	nop
	jr	musicVoiceStore
	nop
musicStop:
	moveq	#0,r25
musicVoiceStore:
	movei	#voiceCursor,r9
	load	(r9),r14
	nop
	store	r0,(r14)
	store	r1,(r14+1)
	store	r25,(r14+7)
musicVoiceNext:
	movei	#voiceCursor,r9
	load	(r9),r14
	addq	#32,r14
	store	r14,(r9)
	movei	#voiceCounter,r9
	load	(r9),r0
	subq	#1,r0
	store	r0,(r9)
	movei	#musicVoiceLoop,r1
	jump	ne,(r1)
	nop

	movei	#chunkFrames,r0
	load	(r0),r0
	move	r0,r1
	shlq	#16,r1
	sub	r1,r16
	add	r0,r17
	sub	r0,r18
	movei	#chunkLoop,r1
	jump	ne,(r1)
	nop

	move	r11,r8
	movei	#BLOCK_FRAMES*2,r7
	load	(r10),r6
musicLevel:
	load	(r8),r0
	sharq	#11,r0
	sat16s	r0
	imult	r6,r0
	sharq	#6,r0
	store	r0,(r8)
	subq	#1,r7
	jr	ne,musicLevel
	addq	#4,r8

	movei	#sfxVoices,r1
	movei	#voiceCursor,r2
	store	r1,(r2)
	moveq	#SFX_VOICES,r1
	movei	#voiceCounter,r2
	store	r1,(r2)
sfxVoiceLoop:
	movei	#voiceCursor,r2
	load	(r2),r14
	nop
	load	(r14+7),r25
	cmpq	#0,r25
	movei	#sfxVoiceNext,r0
	jump	eq,(r0)
	nop
	load	(r14),r0
	load	(r14+1),r1
	load	(r14+2),r2
	load	(r14+3),r3
	load	(r14+4),r4
	load	(r14+5),r5
	load	(r14+6),r8
	add	r11,r8
	movei	#BLOCK_FRAMES,r7
	movei	#SFX_GAIN,r6
	movei	#sfxFrame,r26
sfxFrame:
	loadb	(r0),r9
	shlq	#24,r9
	sharq	#24,r9
	imult	r6,r9
	load	(r8),r12
	add	r9,r12
	store	r12,(r8)
	addq	#8,r8
	add	r3,r1
	addc	r2,r0
	cmp	r4,r0
	jr	mi,sfxNoEnd
	nop
	cmpq	#0,r5
	jr	eq,sfxStop
	nop
	sub	r5,r0
sfxNoEnd:
	subq	#1,r7
	jump	ne,(r26)
	nop
	jr	sfxVoiceStore
	nop
sfxStop:
	moveq	#0,r25
sfxVoiceStore:
	movei	#voiceCursor,r9
	load	(r9),r14
	nop
	store	r0,(r14)
	store	r1,(r14+1)
	store	r25,(r14+7)
sfxVoiceNext:
	movei	#voiceCursor,r9
	load	(r9),r14
	addq	#32,r14
	store	r14,(r9)
	movei	#voiceCounter,r9
	load	(r9),r0
	subq	#1,r0
	store	r0,(r9)
	movei	#sfxVoiceLoop,r1
	jump	ne,(r1)
	nop

	move	r10,r14
	nop
	load	(r14+1),r0
	cmpq	#0,r0
	movei	#filterOff,r1
	jump	eq,(r1)
	nop
	movei	#filterState,r9
	move	r11,r8
	movei	#filterChannel,r26
	moveq	#2,r0
	movei	#filterChannels,r1
	store	r0,(r1)
filterChannel:
	move	r10,r15
	movei	#SHARED_COEFFICIENTS,r0
	add	r0,r15
	load	(r15),r1
	addq	#4,r15
	load	(r15),r2
	addq	#4,r15
	load	(r15),r3
	addq	#4,r15
	load	(r15),r4
	addq	#4,r15
	load	(r15),r5
	move	r9,r15
	load	(r15),r6
	addq	#4,r15
	load	(r15),r7
	addq	#4,r15
	load	(r15),r12
	addq	#4,r15
	load	(r15),r14
	movei	#BLOCK_FRAMES,r25
	movei	#filterFrame,r17
filterFrame:
	load	(r8),r0
	sat16s	r0
	imultn	r1,r0
	imacn	r2,r6
	imacn	r3,r7
	imacn	r4,r12
	imacn	r5,r14
	resmac	r15
	sharq	#FILTER_SHIFT,r15
	sat16s	r15
	move	r6,r7
	move	r0,r6
	move	r12,r14
	move	r15,r12
	store	r15,(r8)
	subq	#1,r25
	jump	ne,(r17)
	addq	#8,r8
	move	r9,r15
	store	r6,(r15)
	addq	#4,r15
	store	r7,(r15)
	addq	#4,r15
	store	r12,(r15)
	addq	#4,r15
	store	r14,(r15)
	addq	#16,r9
	move	r11,r8
	addq	#4,r8
	movei	#filterChannels,r1
	load	(r1),r0
	subq	#1,r0
	store	r0,(r1)
	jump	ne,(r26)
	nop
	movei	#packOutput,r0
	jump	(r0)
	nop

filterOff:
	movei	#filterState,r15
	move	r11,r8
	movei	#LAST_FRAMES_OFFSET,r0
	add	r0,r8
	load	(r8),r1
	sat16s	r1
	addq	#8,r8
	load	(r8),r2
	sat16s	r2
	store	r2,(r15)
	nop
	store	r1,(r15+1)
	store	r2,(r15+2)
	store	r1,(r15+3)
	addq	#4,r8
	load	(r8),r2
	sat16s	r2
	subq	#8,r8
	load	(r8),r1
	sat16s	r1
	nop
	store	r2,(r15+4)
	store	r1,(r15+5)
	store	r2,(r15+6)
	store	r1,(r15+7)

packOutput:
	move	r11,r8
	movei	#ring,r9
	add	r13,r9
	movei	#BLOCK_FRAMES,r7
packFrame:
	load	(r8),r0
	addq	#4,r8
	load	(r8),r1
	addq	#4,r8
	sat16s	r0
	sat16s	r1
	shlq	#16,r0
	shlq	#16,r1
	shrq	#16,r1
	or	r1,r0
	store	r0,(r9)
	subq	#1,r7
	jr	ne,packFrame
	addq	#4,r9

	movei	#sfxActiveFields,r9
	moveq	#0,r2
	moveq	#1,r3
	moveq	#SFX_VOICES,r4
activeMask:
	load	(r9),r0
	cmpq	#0,r0
	jr	eq,activeNext
	nop
	or	r3,r2
activeNext:
	shlq	#1,r3
	subq	#1,r4
	jr	ne,activeMask
	addq	#32,r9

	move	r10,r14
	nop
	load	(r14+2),r0
	or	r0,r0
	movei	#SHARED_TICK_READ+2,r1
	add	r10,r1
	storew	r24,(r1)
	load	(r14+2),r0
	or	r0,r0
	movei	#SHARED_SFX_ACTIVE+2,r1
	add	r10,r1
	storew	r2,(r1)
	movei	#blockCount,r1
	load	(r1),r2
	addq	#1,r2
	store	r2,(r1)
	load	(r14+2),r0
	or	r0,r0
	movei	#SHARED_FRAMES+2,r1
	add	r10,r1
	storew	r2,(r1)

	movei	#BLOCK_BYTES,r0
	add	r0,r13
	and	r27,r13
	movei	#mainLoop,r0
	jump	(r0)
	nop

	.long
dataStart:
voices:
	.rept	4
	dc.l	0,0,0,0,0,0,0,0
	.endr
sfxVoices:
	dc.l	0,0,0,0,0,0,0
sfxActiveFields:
	dc.l	0
	.rept	3
	dc.l	0,0,0,0,0,0,0,0
	.endr
sfxSeqs:
	dc.l	0,0,0,0
filterState:
	dc.l	0,0,0,0,0,0,0,0
chunkFrames:
	dc.l	0
voiceCursor:
	dc.l	0
voiceCounter:
	dc.l	0
filterChannels:
	dc.l	0
voicePeriods:
	dc.l	0,0,0,0
blockCount:
	dc.l	0
accumulator:
	.rept	BLOCK_FRAMES*2
	dc.l	0
	.endr
ring:
	.rept	256
	dc.l	0
	.endr
dataEnd:
stack:
	dc.l	0,0,0,0,0,0,0,0
stackTop:
	dc.l	0
