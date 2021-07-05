#include "common.h"

#include "arm/critical.h"

#include "hw/timer.h"

#define REG_TIMER_BASE	(0x10003000)

#define CNT_START	BIT(7)
#define CNT_IRQEN	BIT(6)
#define CNT_COUNTUP	BIT(2)

#define CNT_NO_PRESCALER	0
#define CNT_PRESCALER_64	1
#define CNT_PRESCALER_256	2
#define CNT_PRESCALER_1024	3

typedef struct {
	vu16 val;
	vu16 cnt;
} PACKED timer_regs;

static timer_regs *get_timer_regs(u32 tmr) {
	return &((timer_regs*)(REG_TIMER_BASE))[tmr];
}

void timer_reset(bool irqen)
{
	need_critical();

	for (uint i = 0; i < 4; i++) {
		timer_regs *regs = get_timer_regs(i);
		regs->cnt = 0;
		regs->val = 0;
	}

	for (uint i = 3; i >= 1; i--)
		get_timer_regs(i)->cnt = CNT_START | CNT_COUNTUP | CNT_NO_PRESCALER;
	get_timer_regs(0)->cnt = CNT_START |
		(irqen ? CNT_IRQEN : 0) | CNT_NO_PRESCALER;
}

u64 timer_get_ticks(void)
{
	u32 hi;
	u16 quart, lo_start, lo_end;
	timer_regs *tmr0 = get_timer_regs(0);

	do { // retry on overflow, hopefully it only needs to retry once
		lo_start = tmr0->val;
		quart = get_timer_regs(1)->val;
		hi = get_timer_regs(2)->val | (get_timer_regs(3)->val << 16);
		lo_end = tmr0->val;
	} while(UNLIKELY(lo_start > lo_end));

	return ((u64)hi << 32) | (quart << 16) | lo_end;
}
