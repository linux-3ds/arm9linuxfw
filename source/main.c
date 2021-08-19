#include "common.h"
#include "pxicmd.h"

#include "arm/arm.h"

#include "hw/irq.h"
#include "hw/pxi.h"

#include "sys/event.h"

static event_t pxi_fifo_available;

static void on_pxi_fifo(u32 irqn) {
	event_trigger(&pxi_fifo_available);
}

void NORETURN arm9linuxfw_entry(void)
{
	irq_reset();
	pxi_reset();

	event_initialize(&pxi_fifo_available);

	/*
	 * Enable interrupts, wait for new commands and process buffers
	 * also prevents the ARM11 from getting interrupts related to these
	 */
	irq_enable(IRQ_PXI_SYNC, NULL);
	irq_enable(IRQ_PXI_TX, on_pxi_fifo);
	irq_enable(IRQ_PXI_RX, on_pxi_fifo);
	arm_interrupt_enable();

	while(1) {
		event_wait(&pxi_fifo_available);
		pxicmd_process();
	}
}
