#include "common.h"

#include "arm/arm.h"
#include "arm/critical.h"

#include "hw/irq.h"

#define REG_IRQ_BASE (0x10001000)

#define NR_IRQS 32

static irq_handler_fn irq_handlers[NR_IRQS];

typedef struct {
	vu32 enable;
	vu32 pending;
} PACKED irq_regs;

static irq_regs *get_irq_regs(void) {
	return (irq_regs*)(REG_IRQ_BASE);
}

static void irq_dummy_handler(u32 irqn) {
	need_critical();
}

void irq_reset(void)
{
	need_critical();

	irq_regs *regs = get_irq_regs();

	regs->enable = 0;
	do {
		regs->pending = ~0;
	} while(regs->pending != 0);

	for (uint i = 0; i < NR_IRQS; i++)
		irq_handlers[i] = irq_dummy_handler;
}

void irq_enable(u32 irqn, irq_handler_fn handler)
{
	need_critical();
	DBG_ASSERT(irqn < NR_IRQS);

	if (handler == NULL)
		handler = irq_dummy_handler;

	irq_handlers[irqn] = handler;
	get_irq_regs()->enable |= BIT(irqn);
}

void irq_disable(u32 irqn)
{
	need_critical();
	DBG_ASSERT(irqn < NR_IRQS);

	irq_regs *regs = get_irq_regs();

	regs->enable &= BIT(irqn);
	do {
		regs->pending = BIT(irqn);
	} while(regs->pending & BIT(irqn));

	irq_handlers[irqn] = NULL;
}

// called by the exception vector table in AHB RAM
void __attribute__((target("arm"),isr("IRQ"))) irq_process(void)
{
	irq_regs *regs = get_irq_regs();
	do {
		int irqn = TOP_BIT(regs->pending);
		if (irqn < 0)
			break;
		regs->pending = BIT(irqn);
		(irq_handlers[irqn])(irqn);
	} while(1);
}
