#pragma once

#include "common.h"

#include "arm/bfn.h"

/** ARM assembly wrappers */
#define ARM_MCR(cp, op1, reg, crn, crm, op2) \
	asmv( \
	"MCR " #cp ", " #op1 ", %[R], " #crn ", " #crm ", " #op2 "\n\t" \
	:: [R] "r"(reg) : "memory", "cc")

#define ARM_MRC(cp, op1, reg, crn, crm, op2) \
	asmv( \
	"MRC " #cp ", " #op1 ", %[R], " #crn ", " #crm ", " #op2 "\n\t" \
	: [R] "=r"(reg) :: "memory", "cc")

/** ARM processor constants */

/** instruction cache size */
#define ARM_ICACHE_SIZE 4096

/** data cache size */
#define ARM_DCACHE_SIZE 4096

/** performs a data sync barrier and makes sure the write
 *  buffer is empty before proceeding to the next instruction */
static inline void arm_sync_barrier(void) {
	ARM_MCR(p15, 0, 0, c7, c10, 4);
}

/** invalidates the entire instruction cache */
static inline void arm_icache_inv(void) {
	ARM_MCR(p15, 0, 0, c7, c5, 0);
}

/** invalidates a range of the instruction cache */
static inline void arm_icache_inv_range(u32 addr, size_t len) {
	((void (*)(u32, u32))(BFN_INVALIDATE_ICACHE_RANGE))(addr, len);
}

/** invalidates the entire data cache */
static inline void arm_dcache_inv(void) {
	ARM_MCR(p15, 0, 0, c7, c6, 0);
}

/** invalidates a range of the data cache */
static inline void arm_dcache_inv_range(u32 addr, size_t len) {
	((void (*)(u32, u32))(BFN_INVALIDATE_DCACHE_RANGE))(addr, len);
}

/** writes back the entire data cache */
static inline void arm_dcache_wb(void) {
	((void (*)(void))(BFN_WRITEBACK_DCACHE))();
}

/** writes back a range of the data cache */
static inline void arm_dcache_wb_range(u32 addr, size_t len) {
	((void (*)(u32, u32))(BFN_WRITEBACK_DCACHE_RANGE))(addr, len);
}

/** writes back and invalidates the entire data cache */
static inline void arm_dcache_wb_inv(void) {
	((void (*)(void))(BFN_WRITEBACK_INVALIDATE_DCACHE))();
}

/** writes back and invalidates a range of the data cache */
static inline void arm_dcache_wb_inv_range(u32 addr, size_t len) {
	((void (*)(u32, u32))(BFN_WRITEBACK_INVALIDATE_DCACHE_RANGE))(addr, len);
}

/** delays program execution by a given number of cycles */
static inline void arm_delay_cycles(u32 cycles) {
	((void (*)(u32))(BFN_WAITCYCLES))(cycles);
}

/** sleeps until an interrupt triggers wakeup */
static inline void arm_wait_for_interrupt(void) {
	arm_sync_barrier();
	ARM_MCR(p15, 0, 0, c7, c0, 4);
}

/** process id register getter and setter */
static inline u32 arm_pid_get(void) {
	u32 pid;
	ARM_MRC(p15, 0, pid, c13, c0, 1);
	return pid;
}

static inline void arm_pid_set(u32 pid) {
	ARM_MCR(p15, 0, pid, c13, c0, 1);
}
