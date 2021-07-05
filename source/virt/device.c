#include "common.h"

#include "arm/critical.h"

#include "virt/device.h"
#include "virt/queue.h"

#define VIRTDEV_DEFAULT_VENDOR	0x464C4F57 // 'WOLF'

void vdev_init(vdev_s *vdev, uint i)
{
	/*
	 - correctly assign the internal ID
	 - for each virtqueue:
	  - initialize virtqueue
	 - reset device registers and hardware
	*/

	vdev->id = i;

	for (uint q = 0; q < vdev->vqn; q++)
		vqueue_init(&vdev->vqs[q], i, q);

	vdev_reset(vdev);
}

void vdev_reset(vdev_s *vdev)
{
	need_critical();

	/*
	 - reset all state values to their default
	 - for each virtqueue:
	  - reset virtqueue
	 - run hardware reset
	*/

	vdev->status = 0;
	vdev->cfg = 0;

	vdev->driver_feat.dword = 0;

	for (uint i = 0; i < vdev->vqn; i++)
		vqueue_reset(&vdev->vqs[i]);
	vdev_hard_reset(vdev);
}

vqueue_s *vdev_get_queue(vdev_s *vdev, uint i)
{
	if (i >= vdev->vqn)
		return NULL;
	return &vdev->vqs[i];
}

enum device_registers {
	DeviceID = 0x00,
	VendorID = 0x01,

	Status = 0x02,

	DeviceFeaturesLo = 0x03,
	DeviceFeaturesHi = 0x04,
	DriverFeaturesLo = 0x05,
	DriverFeaturesHi = 0x06,

	ConfigGeneration = 0x07,
};

u32 vdev_reg_read(vdev_s *v, uint reg)
{
	need_critical();

	switch(reg) {
	case DeviceID:
		return v->dev_id;
	case VendorID:
		return VIRTDEV_DEFAULT_VENDOR;
	case Status:
		return v->status;
	case DeviceFeaturesLo:
		return v->device_feat.lo;
	case DeviceFeaturesHi:
		return v->device_feat.hi;
	case DriverFeaturesLo:
		return v->driver_feat.lo;
	case DriverFeaturesHi:
		return v->driver_feat.hi;
	case ConfigGeneration:
		return v->cfg;
	default:
		return 0;
	}
}

void vdev_reg_write(vdev_s *v, uint reg, u32 val)
{
	need_critical();

	// certain register writes should also have some side effects
	switch(reg) {
	case Status:
		v->status = val;
		break;
	case DriverFeaturesLo:
		v->driver_feat.lo = val;
		break;
	case DriverFeaturesHi:
		v->driver_feat.hi = val;
		break;
	default:
		break;
	}
}

u32 vdev_queue_reg_read(vdev_s *v, uint vqn, u32 reg)
{
	vqueue_s *vq = vdev_get_queue(v, vqn);
	if (UNLIKELY(!vq))
		return 0;
	return vqueue_reg_read(vq, reg);
}

void vdev_queue_reg_write(vdev_s *v, uint vqn, u32 reg, u32 val)
{
	vqueue_s *vq = vdev_get_queue(v, vqn);
	if (LIKELY(vq != NULL))
		vqueue_reg_write(vq, reg, val);
}
