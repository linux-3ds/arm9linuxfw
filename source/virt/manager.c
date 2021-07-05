#include "common.h"

#include "arm/arm.h"
#include "arm/critical.h"

#include "hw/pxi.h"

#include "virt/irq.h"
#include "virt/manager.h"

/* these two symbols get filled in at link time */
extern u32 __vdev_s;
extern u32 __vdev_cnt;

/* list of queues that MIGHT have pending jobs */
static list_head_s vq_pending;

static vdev_s *vman_get_dev_unsafe(uint n) {
	return ((vdev_s *const *)(&__vdev_s))[n];
}

uint vman_dev_count(void)
{
	return (uint)(&__vdev_cnt);
}

vdev_s *vman_get_dev(uint n)
{
	if (n >= vman_dev_count())
		return NULL;
	return vman_get_dev_unsafe(n);
}

void vman_init_all(void)
{
	need_critical();

	/*
	 - initialize virtual IRQ subsystem
	 - initialize global pending vq list
	 - initialize each device
	*/
	virtirq_reset();
	list_head_init(&vq_pending);
	for (uint i = 0; i < vman_dev_count(); i++)
		vdev_init(vman_get_dev_unsafe(i), i);
}

enum register_type {
	DeviceReg = 0,
	ConfigReg = 1,
	QueueReg = 2,
	ManagerReg = 3,
};

static u32 vman_internal_reg_read(uint reg) {
	switch(reg) {
	case 0:
		return VIRT_MANAGER_VERSION; // VERSION
	case 1:
		return vman_dev_count(); // DEVCOUNT
	case 8: case 9: case 10: case 11:
		return virtirq_get(reg - 8, VIRQ_VQUEUE); // INTBANK0
	case 12: case 13: case 14: case 15:
		return virtirq_get(reg - 12, VIRQ_CONFIG); // INTBANK1
	default:
		return 0;
	}
}

static void vman_internal_reg_write(uint reg, u32 val) {
	// ignore all register writes for now
}

static void vman_decode_reg(uint *reg, uint *rtype) {
	*rtype = *reg & 0x03;
	*reg = *reg >> 2;
}

u32 vman_reg_read(uint dev, uint reg)
{
	uint rtype;
	vdev_s *vdev;

	need_critical();

	vman_decode_reg(&reg, &rtype);

	// manager registers ignore the device field
	if (rtype == ManagerReg)
		return vman_internal_reg_read(reg);

	vdev = vman_get_dev(dev);
	if (UNLIKELY(!vdev))
		return 0;

	switch(rtype) {
	case DeviceReg:
		return vdev_reg_read(vdev, reg);
	case ConfigReg:
		return vdev_cfg_read(vdev, reg);
	case QueueReg:
		return vdev_queue_reg_read(vdev, reg & 0x7F, reg >> 7);
	default: return 0;
	}
}

void vman_reg_write(uint dev, uint reg, u32 val)
{
	uint rtype;
	vdev_s *vdev;

	need_critical();

	vman_decode_reg(&reg, &rtype);

	if (rtype == ManagerReg) {
		vman_internal_reg_write(reg, val);
		return;
	}

	vdev = vman_get_dev(dev);
	if (UNLIKELY(!vdev))
		return;

	switch(rtype) {
	case DeviceReg:
		vdev_reg_write(vdev, reg, val);
		break;
	case ConfigReg:
		vdev_cfg_write(vdev, reg, val);
		break;
	case QueueReg:
		vdev_queue_reg_write(vdev, reg & 0x7F, reg >> 7, val);
		break;
	}
}

vdev_s *vqueue_owner(vqueue_s *vq)
{
	return vman_get_dev_unsafe(vq->owner);
}

bool vman_process_pending(void)
{
	bool ret = true;
	vqueue_s *vq;
	list_node_s *next_vq;

	CRITICAL_BLOCK(
		if (list_empty(&vq_pending)) {
			// no queues left to process
			// let the system sleep a bit
			ret = false;
			break;
		}

		next_vq = list_head(&vq_pending);
		list_remove(next_vq);

		vq = CONTAINER_OF(next_vq, vqueue_s, node);

		// alert the device that there MIGHT be pending
		// descriptors in this virtqueue
		vdev_process_vqueue(vqueue_owner(vq), vq);
	);

	return ret;
}

bool vman_add_pending(vqueue_s *vq)
{
	return list_append_if_not_embedded(&vq->node, &vq_pending);
}
