	.section .rodata
	.balign	8
	.globl	jaguarGpuProgram
	.globl	jaguarGpuProgramEnd
jaguarGpuProgram:
	.incbin	"gpu.bin"
jaguarGpuProgramEnd:
	.balign	8
	.globl	jaguarDspProgram
	.globl	jaguarDspProgramEnd
jaguarDspProgram:
	.incbin	"dsp.bin"
jaguarDspProgramEnd:
	.balign	8
