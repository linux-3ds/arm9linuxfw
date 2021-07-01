#include "asm.h"

.section .bootstrap, "ax"
.global __start
.align 5
__start:
	@ Switch to supervisor mode and disable interrupts
	msr cpsr_c, #0xD3

	@ Make sure the binary is written back to main memory and out of caches
	@ forces cache thrashing on a 32 byte boundary (line size)
	bic r0, pc, #0x1F
	add r1, r0, #0x1000
	1:
		ldr r2, [r0], #0x20
		cmp r0, r1
		blo 1b

	@ Cache maintenance operations
	mov r0, #0
	mcr p15, 0, r0, c7, c10, 4  @ drain write buffer (data memory barrier)
	mcr p15, 0, r0, c7, c6, 0   @ flush data cache
	mcr p15, 0, r0, c7, c5, 0   @ flush instruction cache


	@ Set the control register to its reset value (+ high vectors)
	@ Disables the MPU, caches, TCMs
	ldr r0, =0x2078
	mcr p15, 0, r0, c1, c0, 0


	@ Setup Tightly Coupled Memory
	ldr r1, =0x4000000A @ DTCM @ 0x40000000 / 16KB (16KB mirror)
	ldr r2, =0x00000024 @ ITCM @ 0x00000000 / 32KB (128MB mirror)
	mcr p15, 0, r1, c9, c1, 0
	mcr p15, 0, r2, c9, c1, 1


	@ Enable TCMs
	orr r0, r0, #0x50000
	mcr p15, 0, r0, c1, c0, 0


	@ Get relocation base
	ldr r11, =__bootstrap_start
	adr r12, __start
	sub r12, r12, r11


	@ Relocate vectors
	ldr r0, =__vector_lma
	ldr r1, =__vector_s
	ldr r2, =__vector_e
	add r0, r0, r12
	bl boot_reloc

	@ Relocate executable code
	ldr r0, =__text_lma
	ldr r1, =__text_s
	ldr r2, =__text_e
	add r0, r0, r12
	bl boot_reloc

	@ Relocate data and rodata
	ldr r0, =__data_lma
	ldr r1, =__data_s
	ldr r2, =__data_e
	add r0, r0, r12
	bl boot_reloc


	@ Clear BSS
	ldr r0, =__bss_s
	ldr r1, =__bss_e
	mov r2, #0
	1:
		cmp r0, r1
		strlo r2, [r0], #4
		blo 1b


	@ Branch to main startup code
	ldr pc, =start_itcm


@ equivalent to memcpy(vma_start, lma, vma_end - vma_start)
@ assumes all pointers are aligned to a 4 byte boundary
@ and the length to copy is aligned to 16 bytes
boot_reloc:
	cmp r1, r2
	ldmloia r0!, {r3-r6}
	stmloia r1!, {r3-r6}
	blo boot_reloc
	bx lr

.pool
