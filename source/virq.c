#include "common.h"
#include "virq.h"

#include "arm/critical.h"

#include "hw/irq.h"
#include "hw/pxi.h"

static u32 virq_pending = 0;

void virq_reset(void)
{
	virq_pending = 0;
}

static void virq_handler(u32 irqn) {
	virq_pending |= BIT(irqn);
	pxi_sync_trigger();
}

void virq_unmask(u32 irqn)
{
	need_critical();

	irq_enable(irqn, virq_handler);
}

void virq_mask(u32 irqn)
{
	need_critical();

	irq_disable(irqn);
	virq_pending &= ~BIT(irqn);
}

u32 virq_get_pending(void)
{
	need_critical();

	if (virq_pending != 0) {
		u32 ret = TOP_BIT(virq_pending);
		virq_pending &= ~BIT(ret);
		return ret;
	} else {
		return 32; /* none pending */
	}
}
