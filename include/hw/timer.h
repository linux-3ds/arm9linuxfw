#pragma once

#include "common.h"

#define TIMER_BASE_FREQ 67027964ULL

/** reset TIMER registers and state */
void timer_reset(bool irqen);

/** returns the number of ticks passed since startup */
u64 timer_get_ticks(void);

static inline u64 timer_ticks_to_ms(u64 ticks) {
	return (ticks * 1000ULL) / TIMER_BASE_FREQ;
}

static inline u64 timer_ms_to_ticks(u64 ms) {
	return (ms * TIMER_BASE_FREQ) / 1000ULL;
}
