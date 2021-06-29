#include "asm.h"

#include "arm/bfn.h"

#define IRQ_STACK_SIZE (1024)
#define SYS_STACK_SIZE (2048)

ASM_FUNCTION start_itcm
	@ Setup stacks
	msr cpsr_c, #0xD2 @ IRQ
	ldr sp, =irq_stack

	msr cpsr_c, #0xDF @ SYS
	ldr sp, =sys_stack


	@ MPU Regions:
	@ N | Name    | Start      | End        | DataP | InstP | IC | DC | DB
	@ 0 | ITCM    | 0x01FF8000 | 0x01FFFFFF | RW_NA | RO_NA | n  | n  | n
	@ 1 | AHBRAM  | 0x08000000 | 0x08FFFFFF | RW_NA | RO_NA | y  | y  | y
	@ 2 | MMIO    | 0x10000000 | 0x101FFFFF | RW_NA | NA_NA | n  | n  | n
	@ 3 | VRAM    | 0x18000000 | 0x187FFFFF | RW_NA | NA_NA | n  | n  | n
	@ 4 | AXIRAM  | 0x1FF00000 | 0x1FFFFFFF | RW_NA | NA_NA | n  | n  | n
	@ 5 | FCRAM   | 0x20000000 | 0x2FFFFFFF | RW_NA | NA_NA | n  | n  | y
	@ 6 | DTCM    | 0x40000000 | 0x40003FFF | RW_NA | NA_NA | n  | n  | n
	@ 7 | BootROM | 0xFFFF0000 | 0xFFFF7FFF | RO_NA | RO_NA | y  | y  | n

	mov r0, #0b10000010 @ Instruction Cacheable
	mov r1, #0b10000010 @ Data Cacheable
	mov r2, #0b00100010 @ Data Bufferable

	ldr r3, =0x51111111 @ Data Access Permissions
	ldr r4, =0x50000055 @ Instruction Access Permissions

	mcr p15, 0, r0, c2, c0, 1
	mcr p15, 0, r1, c2, c0, 0
	mcr p15, 0, r2, c3, c0, 0

	mcr p15, 0, r3, c5, c0, 2
	mcr p15, 0, r4, c5, c0, 3

	ldr r8, =mpu_regions
	ldmia r8, {r0-r7}
	mcr p15, 0, r0, c6, c0, 0
	mcr p15, 0, r1, c6, c1, 0
	mcr p15, 0, r2, c6, c2, 0
	mcr p15, 0, r3, c6, c3, 0
	mcr p15, 0, r4, c6, c4, 0
	mcr p15, 0, r5, c6, c5, 0
	mcr p15, 0, r6, c6, c6, 0
	mcr p15, 0, r7, c6, c7, 0


	@ Enable MPU and caches, use high vectors
	mrc p15, 0, r0, c1, c0, 0
	ldr r1, =0x3005
	orr r0, r0, r1
	mcr p15, 0, r0, c1, c0, 0


	@ Branch to C code
	mov lr, #0
	b arm9linuxfw_entry


.section .bss.stacks
.align 4

.global irq_stack
irq_stack_bottom:
	.space IRQ_STACK_SIZE
irq_stack:

.global sys_stack
sys_stack_bottom:
	.space SYS_STACK_SIZE
sys_stack:


.section .rodata.mpu_regions
.align 3

.global mpu_regions
mpu_regions:
	.word 0x01FF801D @ ITCM
	.word 0x08000027 @ AHBRAM
	.word 0x10000029 @ MMIO
	.word 0x1800002D @ VRAM
	.word 0x1FF00027 @ AXIRAM
	.word 0x20000037 @ FCRAM
	.word 0x4000001B @ DTCM
	.word 0xFFFF001D @ BootROM
