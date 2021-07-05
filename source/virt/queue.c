#include "common.h"

#include "arm/arm.h"
#include "arm/critical.h"

#include "virt/queue.h"
#include "virt/manager.h"

void vqueue_init(vqueue_s *vq, uint owner, uint i)
{
	list_node_init(&vq->node);
	vq->owner = owner;
	vq->id = i;
	vqueue_reset(vq);
}

void vqueue_reset(vqueue_s *vq)
{
	need_critical();

	list_remove(&vq->node);
	vq->ready = 0;
	vq->size_mask = 0;
	vq->avail_idx = 0;
	vq->used_idx = 0;
	vq->q_desc = 0;
	vq->q_avail = 0;
	vq->q_used = 0;
}

enum queue_regs {
	QueueNumMax = 0x00,
	QueueNumCurrent = 0x01,

	QueueReady = 0x02,
	QueueNotify = 0x03,

	QueueDesc = 0x04,
	QueueAvail = 0x05,
	QueueUsed = 0x06,
};

u32 vqueue_reg_read(vqueue_s *vq, uint reg)
{
	need_critical();

	switch(reg) {
	case QueueNumMax:
		return VIRTQUEUE_MAX_DESC;
	case QueueNumCurrent:
		return vq->size_mask + 1;
	case QueueReady:
		return vq->ready;
	case QueueDesc:
		return vq->q_desc;
	case QueueAvail:
		return vq->q_avail;
	case QueueUsed:
		return vq->q_used;
	default: return 0;
	}
}

static void vqueue_set_desc_count(vqueue_s *vq, u32 val) {
	/* make sure the size is in range and is a power of two */
	vq->size_mask = ((val <= VIRTQUEUE_MAX_DESC) && INT_IS_POW2(val)) ?
			val - 1 : 0;
}

void vqueue_reg_write(vqueue_s *vq, uint reg, u32 val)
{
	need_critical();

	switch(reg) {
	case QueueNumCurrent:
		vqueue_set_desc_count(vq, val);
		break;
	case QueueReady:
		if (vq->size_mask > 0)
			vq->ready = val;
		break;
	case QueueNotify: /* new buffers are present */
		vman_add_pending(vq);
		break;
	case QueueDesc:
		vq->q_desc = val;
		break;
	case QueueAvail:
		vq->q_avail = val;
		break;
	case QueueUsed:
		vq->q_used = val;
		break;
	default: break;
	}
}

enum {
	VQ_DESC_F_NEXT = BIT(0),
	VQ_DESC_F_WRITE = BIT(1),
	VQ_DESC_F_INDIRECT = BIT(2),
};

typedef struct vqDesc_s {
	u64 addr; // high 32 bits of address are always ignored
	u32 len;
	u16 flags;
	u16 next;
} PACKED vqDesc_s;

typedef struct vqAvail_s {
	u16 flags;
	u16 last;
	u16 ring[0];
} PACKED vqAvail_s;

typedef struct vqUsed_s {
	u16 flags;
	u16 last;
	struct { u32 id; u32 len; } PACKED ring[0];
} PACKED vqUsed_s;

int vqueue_fetch_avail_first(vqueue_s *vq)
{
	need_critical();

	uint index, avail;
	const vqAvail_s *vqA = (const vqAvail_s*)vq->q_avail;

	if (UNLIKELY(!vq->ready))
		return -1;

	index = vq->avail_idx & vq->size_mask;
	avail = vqA->last & vq->size_mask;

	if (index == avail)
		return -1;

	vq->avail_idx = index + 1;
	return vqA->ring[index];
}

int vqueue_fetch_avail_next(vqueue_s *vq, u16 prev)
{
	need_critical();

	const vqDesc_s *vqD = (const vqDesc_s*)vq->q_desc;
	if (vqD[prev].flags & VQ_DESC_F_NEXT)
		return vqD[prev].next;
	return -1;
}

void vqueue_push_used(vqueue_s *vq, u16 first, u32 len)
{
	need_critical();

	vqUsed_s *vqU = (vqUsed_s*)vq->q_used;
	unsigned index = vq->used_idx & vq->size_mask;

	vqU->ring[index].id = first;
	vqU->ring[index].len = len;
	arm_sync_barrier();

	/*
	 * the first data sync barrier ensures that
	 * any buffer data and the pushed index
	 * are written back to main memory
	 */

	vq->used_idx = index + 1;
	vqU->last += 1;
	arm_sync_barrier();
}

void vqueue_get_desc(vqueue_s *vq, u16 index, vdesc_s *desc)
{
	need_critical();

	const vqDesc_s *vqD = (const vqDesc_s*)vq->q_desc;

	if (index <= vq->size_mask) {
		desc->data = (u8*)((u32)vqD[index].addr);
		desc->length = vqD[index].len;
		desc->dir = (vqD[index].flags & VQ_DESC_F_WRITE) ?
			VDEV_TO_HOST : HOST_TO_VDEV;
	} else {
		desc->data = NULL;
	}
}
