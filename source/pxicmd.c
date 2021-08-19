#include "common.h"

#include "virq.h"

#include "arm/critical.h"

#include "hw/irq.h"
#include "hw/pxi.h"

#define REG_IO(T, off)	(*(volatile T*)(off))

#define PXICMD_REG(c)	(((c) << 8) >> 8)	/* [23:0] */
#define PXICMD_SIZE(c)	(((c) << 4) >> 28)	/* [27:24] */
#define PXICMD_MODE(c)	((c) >> 28)		/* [31:28] */

#define PXICMD_MODE_WRITE	0
#define PXICMD_MODE_READ	1
#define PXICMD_MODE_IOSET	2
#define PXICMD_MODE_IOCLR	3
#define PXICMD_MODE_IRQGET	4
#define PXICMD_MODE_IRQMASK	5
#define PXICMD_MODE_IRQUNMASK	6

#define PXICMD_SIZE_BYTE	0
#define PXICMD_SIZE_HALF	1
#define PXICMD_SIZE_WORD	2

static void pxicmd_iowr(u32 addr, u32 size, u32 data) {
	switch(size) {
	case PXICMD_SIZE_WORD:
		REG_IO(u32, addr) = data;
		break;
	case PXICMD_SIZE_BYTE:
		REG_IO(u8, addr) = data;
		break;
	case PXICMD_SIZE_HALF:
		REG_IO(u16, addr) = data;
		break;
	}
}

static u32 pxicmd_iord(u32 addr, u32 size) {
	switch(size) {
	case PXICMD_SIZE_BYTE:
		return REG_IO(u8, addr);
	case PXICMD_SIZE_HALF:
		return REG_IO(u16, addr);
	case PXICMD_SIZE_WORD:
		return REG_IO(u32, addr);
	}

	return 0xDEADBEEF;
}

static void pxicmd_process_read(u32 addr, u32 size)
{
	pxi_send(pxicmd_iord(addr, size), true);
}

static void pxicmd_process_write(u32 addr, u32 size, u32 arg)
{
	pxicmd_iowr(addr, size, arg);
}

static void pxicmd_process_mask(u32 addr, u32 size, u32 set, u32 clr)
{
	u32 data = pxicmd_iord(addr, size);
	data = (data | set) & ~clr;
	pxicmd_iowr(addr, size, data);
}

static void pxicmd_process_one(u32 pxicmd)
{
	u32 reg, size, mode, addr;

	reg = PXICMD_REG(pxicmd);
	size = PXICMD_SIZE(pxicmd);
	mode = PXICMD_MODE(pxicmd);

	addr = reg + 0x10000000;

	switch(mode) {
	case PXICMD_MODE_WRITE:
		pxicmd_process_write(addr, size, pxi_recv(true));
		break;
	case PXICMD_MODE_READ:
		pxicmd_process_read(addr, size);
		break;
	case PXICMD_MODE_IOSET:
		pxicmd_process_mask(addr, size, pxi_recv(true), 0);
		break;
	case PXICMD_MODE_IOCLR:
		pxicmd_process_mask(addr, size, 0, pxi_recv(true));
		break;
	case PXICMD_MODE_IRQGET:
		CRITICAL_BLOCK( pxi_send(virq_get_pending(), true); );
		break;
	case PXICMD_MODE_IRQMASK:
		CRITICAL_BLOCK( virq_mask(reg & 0x1F); );
		break;
	case PXICMD_MODE_IRQUNMASK:
		CRITICAL_BLOCK( virq_unmask(reg & 0x1F); );
		break;
	}
}

void pxicmd_process(void)
{
	while(!pxi_is_rx_empty())
		pxicmd_process_one(pxi_recv(false));
}
