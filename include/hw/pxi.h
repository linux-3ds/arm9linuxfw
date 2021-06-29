#pragma once

#include "common.h"

/** reset PXI hardware to a known state */
void pxi_reset(void);

/** check receive & send FIFO status */
uint pxi_is_rx_empty(void);
uint pxi_is_tx_full(void);

/** receive and send data through the PXI FIFO */
u32 pxi_recv(void);
void pxi_send(u32 msg);

/** manipulate the PXI SYNC block */
u8 pxi_sync_get(void);
void pxi_sync_set(u8 data);
void pxi_sync_trigger(void);
