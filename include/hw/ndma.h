#pragma once

#include "common.h"

#define NDMA_ARBITRATION_PRIORITY	0
#define NDMA_ARBITRATION_ROUNDROBIN	BIT(31)
#define NDMA_ARBITRATION_CYCLES(n)	(((n) & 0xF) << 16)

enum {
	NDMA_STARTUP_TIMER0 = 0,
	NDMA_STARTUP_TIMER1 = 1,
	NDMA_STARTUP_TIMER2 = 2,
	NDMA_STARTUP_TIMER3 = 3,

	NDMA_STARTUP_CTRCARD0 = 4,
	NDMA_STARTUP_CTRCARD1 = 5,

	NDMA_STARTUP_SDIO1 = 6,
	NDMA_STARTUP_SDIO2 = 7,

	NDMA_STARTUP_AES_IN = 8,
	NDMA_STARTUP_AES_OUT = 9,

	NDMA_STARTUP_SHA_IN = 10,
	NDMA_STARTUP_SHA_OUT = 11,
};

#define NDMA_DST_INC	(0 << 10)
#define NDMA_DST_DEC	(1 << 10)
#define NDMA_DST_FIXED	(2 << 10)
#define NDMA_DST_RELOAD	BIT(12)

#define NDMA_SRC_INC	(0 << 13)
#define NDMA_SRC_DEC	(1 << 13)
#define NDMA_SRC_FIXED	(2 << 13)
#define NDMA_SRC_FILL	(3 << 13)
#define NDMA_SRC_RELOAD	BIT(15)

#define NDMA_STARTUP_MODE(x)	((x) << 24)

#define NDMA_IMMEDIATE_MODE	BIT(28)
#define NDMA_REPEATING_MODE	BIT(29)

#define NDMA_COPY_FLAGS	(NDMA_DST_INC | NDMA_SRC_INC | NDMA_IMMEDIATE_MODE)
#define NDMA_FILL_FLAGS	(NDMA_DST_INC | NDMA_SRC_FILL | NDMA_IMMEDIATE_MODE)

#define NDMA_CLK_INTERVAL(x)	((x) & 0xFFFF)
#define NDMA_CLK_PRESCALER_33	(0 << 16)
#define NDMA_CLK_PRESCALER_8	(1 << 16)
#define NDMA_CLK_PRESCALER_2	(2 << 16)
#define NDMA_CLK_PRESCALER_0_5	(3 << 16)

enum { // pre-reserved channels
	NDMA_CHANNEL_SDMC = 0,
	NDMA_CHANNEL_EMMC = 1,
	NDMA_CHANNEL_CART = 2,
	NDMA_CHANNEL_HWRNG = 7,
};

/* reset the entire NDMA hardware block */
void ndma_reset(u32 arbitration_flags);

/* set up timing parameters of a channel */
void ndma_setclk(u32 chan, u32 control);

/* set up an async DMA transfer */
void ndma_xfer_async(u32 chan, u32 dst, u32 src, u32 len, u32 flags);

/* test whether a transfer has finished */
bool ndma_is_busy(u32 chan);

/* block execution until the transfer has finished */
void ndma_wait_done(u32 chan);

/* set up a synchronous DMA transfer */
static inline void ndma_xfer(u32 chan, u32 dst, u32 src, u32 len, u32 flags) {
	ndma_xfer_async(chan, dst, src, len, flags);
	ndma_wait_done(chan);
}
