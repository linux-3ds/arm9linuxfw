#include "common.h"

#include "arm/arm.h"
#include "arm/critical.h"

#include "hw/pxi.h"
#include "sys/event.h"
#include "virt/irq.h"

#define VIRQ_BANKS	(VIRQ_MAX / 32)

static event_t virq_ev;
static u32 virq_pending[VIRQ_MODES][VIRQ_BANKS];

void virtirq_reset(void)
{
	need_critical();

	event_initialize(&virq_ev);

	for (uint m = 0; m < VIRQ_MODES; m++) {
		for (uint b = 0; b < VIRQ_BANKS; b++) {
			virq_pending[m][b] = 0;
		}
	}
}

u32 virtirq_set(u32 dev, uint mode)
{
	need_critical();

	u32 ret = 0;
	uint bank = dev / 32;
	uint mask = BIT(dev % 32);

	if ((bank < VIRQ_BANKS) && (mode < VIRQ_MODES)) {
		ret = virq_pending[mode][bank];
		virq_pending[mode][bank] |= mask;
		event_trigger(&virq_ev);
	}

	return ret;
}

u32 virtirq_get(u32 bank, uint mode)
{
	if ((bank < VIRQ_BANKS) && (mode < VIRQ_MODES))
		return arm_swp(0, &virq_pending[mode][bank]);
	return 0;
}

/* only trigger the ARM11 if absolutely necessary */
void virtirq_sync(void)
{
	if (event_test(&virq_ev))
		pxi_sync_trigger();
}
