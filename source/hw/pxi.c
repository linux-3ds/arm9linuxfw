#include "common.h"

#include "arm/arm.h"
#include "arm/critical.h"

#include "hw/pxi.h"

#define REG_PXI_BASE	(0x10008000)
#define PXI_FIFO_WIDTH	(16)

#define PXI_CNT_TX_FULL		BIT(1)
#define PXI_CNT_TX_FLUSH	BIT(3)

#define PXI_CNT_RX_EMPTY	BIT(8)
#define PXI_CNT_RX_FULL		BIT(9)

#define PXI_CNT_TX_AVAIL_IRQ	BIT(2)
#define PXI_CNT_RX_AVAIL_IRQ	BIT(10)

#define PXI_CNT_FIFO_ERRACK	BIT(14)
#define PXI_CNT_FIFO_ENABLE	BIT(15)

#define PXI_SYNC_CNT_IRQ_TRIG	BIT(5)
#define PXI_SYNC_CNT_IRQ_ENABLE	BIT(7)

typedef struct {
	vu8 sync_rx;
	vu8 sync_tx;
	u8 unused;
	vu8 sync_cnt;

	vu32 cnt;
	vu32 tx;
	vu32 rx;
} PACKED pxi_regs;

static pxi_regs *get_pxi_regs(void) {
	return (pxi_regs*)(REG_PXI_BASE);
}

void pxi_reset(void)
{
	need_critical();

	pxi_regs *regs = get_pxi_regs();

	regs->sync_cnt = 0;
	regs->cnt = PXI_CNT_TX_FLUSH | \
		PXI_CNT_FIFO_ERRACK | PXI_CNT_FIFO_ENABLE;

	for (uint i = 0; i < PXI_FIFO_WIDTH; i++)
		regs->rx;

	regs->cnt = PXI_CNT_RX_AVAIL_IRQ | \
		PXI_CNT_FIFO_ERRACK | PXI_CNT_FIFO_ENABLE;
	regs->sync_cnt = PXI_SYNC_CNT_IRQ_ENABLE;
}

uint pxi_is_rx_empty(void)
{
	return get_pxi_regs()->cnt & PXI_CNT_RX_EMPTY;
}

uint pxi_is_tx_full(void)
{
	return get_pxi_regs()->cnt & PXI_CNT_TX_FULL;
}

u32 pxi_recv(void)
{
	return get_pxi_regs()->rx;
}

void pxi_send(u32 msg)
{
	get_pxi_regs()->tx = msg;
}

u8 pxi_sync_get(void)
{
	return get_pxi_regs()->sync_rx;
}

void pxi_sync_set(u8 data)
{
	get_pxi_regs()->sync_tx = data;
}

void pxi_sync_trigger(void)
{
	get_pxi_regs()->sync_cnt = PXI_SYNC_CNT_IRQ_ENABLE | PXI_SYNC_CNT_IRQ_TRIG;
}
