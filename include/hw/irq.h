#pragma once

#include "common.h"

/** interrupt callback funcptr type */
typedef void (*irq_handler_fn)(u32);

/** resets all interrupt callbacks and registers */
void irq_reset(void);

/** enable interrupt line and set a handler (stubbed if handler is NULL) */
void irq_enable(u32 irqn, irq_handler_fn handler);

/** disables an interrupt line */
void irq_disable(u32 irqn);

#define IRQ_NDMA(n)	(IRQ_NDMA0 + (n))
#define IRQ_TIMER(n)	(IRQ_TIMER0 + (n))

enum {
	IRQ_NDMA0 = 0,
	IRQ_NDMA1 = 1,
	IRQ_NDMA2 = 2,
	IRQ_NDMA3 = 3,	
	IRQ_NDMA4 = 4,
	IRQ_NDMA5 = 5,
	IRQ_NDMA6 = 6,
	IRQ_NDMA7 = 7,

	IRQ_TIMER0 = 8,
	IRQ_TIMER1 = 9,
	IRQ_TIMER2 = 10,
	IRQ_TIMER3 = 11,

	IRQ_PXI_SYNC = 12,
	IRQ_PXI_TX = 13,
	IRQ_PXI_RX = 14,

	IRQ_AES = 15,

	IRQ_SDIO_1 = 16,
	IRQ_SDIO_1_ASYNC = 17,

	IRQ_SDIO_3 = 18,
	IRQ_SDIO_3_ASYNC = 18,

	IRQ_DEBUG_RECV = 20,
	IRQ_DEBUG_SEND = 21,

	IRQ_RSA = 22,

	IRQ_CTR_CARD_1 = 23,
	IRQ_CTR_CARD_2 = 24,

	IRQ_CGC = 25,
	IRQ_CGC_DET = 26,

	IRQ_DS_CARD = 27,

	IRQ_XDMA = 28,
	IRQ_XDMA_ABORT = 29,
};
