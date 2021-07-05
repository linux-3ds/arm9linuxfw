#pragma once

#include "common.h"

#define ARM_MSR(cp, reg) \
	asmv( \
	"MSR " #cp ", %[R]\n\t" \
	:: [R] "r"(reg) : "memory", "cc")

#define ARM_MRS(reg, cp) \
	asmv( \
	"MRS %[R], " #cp "\n\t" \
	: [R] "=r"(reg) :: "memory")

/** CPSR interrupt mask - disables interrupts when set */
#define ARM_INT_MASK	0xC0

/** critical section wrapper, any code inside is safely executed */
#define CRITICAL_BLOCK(_x) ({ \
	u32 _sr = arm_enter_critical(); \
	do { _x } while(0); \
	arm_leave_critical(_sr); \
})

/** debug-only function that aborts when called from a non-critical context */
static inline void need_critical(void) {
	#ifndef NDEBUG
	u32 sr;
	ARM_MRS(sr, cpsr);
	DBG_ASSERT(sr & ARM_INT_MASK);
	#endif
}

/** same as above but the other way around, this function might sleep */
static inline void need_sleep(void) {
	#ifndef NDEBUG
	u32 sr;
	ARM_MRS(sr, cpsr);
	DBG_ASSERT(!(sr & ARM_INT_MASK));
	#endif
}

/** preserves the interrupt state and disables interrupts */
static inline u32 arm_enter_critical(void) {
	u32 sr;
	ARM_MRS(sr, cpsr);
	ARM_MSR(cpsr_c, sr | ARM_INT_MASK);
	return sr & ARM_INT_MASK;
}

/** restores the interrupt state after a critical section */
static inline void arm_leave_critical(u32 stat) {
	u32 sr;
	ARM_MRS(sr, cpsr);
	sr = (sr & ~ARM_INT_MASK) | stat;
	ARM_MSR(cpsr_c, sr);
}

/** enables processor interrupts */
static inline void arm_interrupt_enable(void) {
	u32 sr;
	ARM_MRS(sr, cpsr);
	ARM_MSR(cpsr_c, sr & ~ARM_INT_MASK);
}

/** disables processor interrupts */
static inline void arm_interrupt_disable(void) {
	u32 sr;
	ARM_MRS(sr, cpsr);
	ARM_MSR(cpsr_c, sr | ARM_INT_MASK);
}

/** returns non-zero if processor interrupts are disabled */
static inline uint arm_is_in_critical(void) {
	u32 sr;
	ARM_MRS(sr, cpsr);
	return sr & ARM_INT_MASK;
}

/** performs an atomic word swap */
static inline u32 arm_swp(u32 val, u32 *addr) {
	u32 old;
	asmv(
		"swp %0, %1, [%2]\n\t"
		: "=&r"(old) : "r"(val), "r"(addr) : "memory"
	);
	return old;
}

/** atomic byte swap */
static inline u32 arm_swpb(u8 val, u8 *addr) {
	u32 old;
	asmv(
		"swpb %0, %1, [%2]\n\t"
		: "=&r"(old) : "r"(val), "r"(addr) : "memory"
	);
	return old;
}
