#pragma once

#include "arm/arm.h"
#include "arm/critical.h"

typedef u8 event_t;

#define EVENT_INITIALIZE	0

static inline void event_initialize(event_t *ev) {
	*ev = 0;
}

static inline void event_trigger(event_t *ev) {
	*ev = 1;
}

static inline bool event_test(event_t *ev) {
	return arm_swpb(0, ev) != 0;
}

#define event_clear	event_initialize

static inline void event_wait(event_t *ev) {
	need_sleep();

	while(!event_test(ev))
		arm_wait_for_interrupt();
}
