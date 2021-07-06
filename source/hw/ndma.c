#include "common.h"

#include "arm/critical.h"

#include "hw/irq.h"
#include "hw/ndma.h"

#include "sys/event.h"

#define NUM_CHANNELS	(8)

#define NDMA_CNT_IRQEN	BIT(30)
#define NDMA_CNT_START	BIT(31)

#define REG_NDMA_BASE	(0x10002000)

typedef struct {
	vu32 src;
	vu32 dst;
	vu32 tcnt;
	vu32 wcnt;
	vu32 clk_cnt;
	vu32 fill_data;
	vu32 cnt;
} PACKED ndma_regs;

static event_t ndma_xfer_ev[NUM_CHANNELS];

static vu32 *get_ndma_cnt(void) {
	return (vu32*)(REG_NDMA_BASE);
}

static ndma_regs *get_ndma_regs(u32 chan) {
	return &((ndma_regs*)(REG_NDMA_BASE + 0x04))[chan];
}

static void ndmaIrqHandler(u32 irqn) {
	u32 channel = irqn - IRQ_NDMA0;
	event_trigger(&ndma_xfer_ev[channel]);
}

void ndma_reset(u32 arbitration_flags)
{
	need_critical();

	vu32 *global_cnt = get_ndma_cnt();

	*global_cnt = 0;
	for (uint i = 0; i < NUM_CHANNELS; i++) {
		ndma_regs *regs = get_ndma_regs(i);
		regs->cnt = 0;
		regs->clk_cnt = 0;
	}
	*global_cnt = arbitration_flags | BIT(0); // enable
}

void ndma_setclk(u32 chan, u32 control)
{
	DBG_ASSERT(chan < NUM_CHANNELS);
	get_ndma_regs(chan)->clk_cnt = control;
}

void ndma_xfer_async(u32 chan, u32 dst, u32 src, u32 len, u32 flags)
{
	ndma_regs *regs;
	DBG_ASSERT(chan < NUM_CHANNELS);

	regs = get_ndma_regs(chan);
	event_clear(&ndma_xfer_ev[chan]);

	regs->src = src;
	regs->dst = dst;
	regs->fill_data = src;

	regs->tcnt = 0; // total length for repeats
	regs->wcnt = len / 4; // number of physical blocks
	regs->cnt = flags | NDMA_CNT_IRQEN | NDMA_CNT_START;
}

bool ndma_is_busy(u32 chan)
{
	DBG_ASSERT(chan < NUM_CHANNELS);
	return event_test(&ndma_xfer_ev[chan]);
}

void ndma_wait_done(u32 chan)
{
	DBG_ASSERT(chan < NUM_CHANNELS);
	event_wait(&ndma_xfer_ev[chan]);
}
