#include "common.h"

#include "arm/arm.h"

#include "hw/irq.h"
#include "hw/pxi.h"
#include "hw/timer.h"

#include "virt/irq.h"
#include "virt/manager.h"

#define PXIMSG_CMD_REG(m)	(((m) & (BIT(24) - 1)))
#define PXIMSG_CMD_DEV(m)	(((m) >> 24) & 0x7F)
#define PXIMSG_CMD_TYPE(m)	(((m) >> 31) & 0x01)

static void on_pxi_recv(u32 irqn) {
	while(!pxi_is_rx_empty()) {
		u32 msg, dev, cmd, reg, data;

		msg = pxi_recv();

		cmd = PXIMSG_CMD_TYPE(msg);
		dev = PXIMSG_CMD_DEV(msg);
		reg = PXIMSG_CMD_REG(msg);

		switch(cmd) {
		case 0: // READ REGISTER
			data = vman_reg_read(dev, reg);
			while(pxi_is_tx_full());
			pxi_send(data);
			break;

		case 1: // WRITE REGISTER
			while(pxi_is_rx_empty());
			data = pxi_recv();
			vman_reg_write(dev, reg, data);
			break;
		}
	}
}

void NORETURN arm9linuxfw_entry(void)
{
	irq_reset();

	pxi_reset();
	timer_reset(false);

	vman_init_all();

	// Enable interrupts, wait for new commands and process buffers
	irq_enable(IRQ_PXI_SYNC, NULL);
	irq_enable(IRQ_PXI_TX, NULL);
	irq_enable(IRQ_PXI_RX, on_pxi_recv);

	arm_interrupt_enable();

	while(1) {
		DBG_ASSERT(!arm_is_in_critical());

		if (!vman_process_pending())
			arm_wait_for_interrupt();
	}
}
