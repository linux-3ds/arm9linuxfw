#pragma once

#include "common.h"

#include "sys/list.h"

typedef struct vdev_s vdev_s;
typedef struct vqueue_s vqueue_s;

/** VirtIO device feature bits */
enum vdev_feature_bits {
	/**< This feature enables the used_event and the avail_event fields */
	VDEV_F_RING_EVENT_IDX = BIT(29),

	/**< Indicates compliance with the v1 specification */
	VDEV_F_VERSION_1 = BIT(32),

	/**< Indicates the ARM9 is behind an IOMMU */
	VDEV_F_ACCESS_PLATFORM = BIT(33),

	/**< Indicates support for packed virtqueue layout */
	VDEV_F_RING_PACKED = BIT(34),

	/**< All buffers are used up in the same order as they're provided */
	VDEV_F_IN_ORDER = BIT(35),

	/**< Forces stronger memory and cache barriers */
	VDEV_F_ORDER_PLATFORM = BIT(36),

	/**< Flags that are always set by default */
	VDEV_F_DEFAULT = VDEV_F_VERSION_1 |
		VDEV_F_ACCESS_PLATFORM | VDEV_F_ORDER_PLATFORM,
};

/** VirtIO device status bits */
enum vdev_status_bits {
	/**< The guest recognizes the device as a valid VirtIO device */
	VDEV_S_ACK = BIT(0),

	/**< The guest knows how to drive the device */
	VDEV_S_DRIVER = BIT(1),

	/**< The driver is ready */
	VDEV_S_DRIVEROK = BIT(2),

	/**< The guest has completed feature negotiation */
	VDEV_S_FEATURESOK = BIT(3),

	/**< Something happened and the device needs a reset */
	VDEV_S_NEEDRESET = BIT(6),

	/**< The guest has given up on the device */
	VDEV_S_FAILED = BIT(7),
};

/** VirtIO device types */
enum vdev_type {
	VDEV_T_BLOCK = 2,	/**< Block device */
	VDEV_T_HWRNG = 4,	/**< Entropy source */
	VDEV_T_CRYPTO = 20,	/**< Hardware cryptography module */
};

/**
 Main VirtIO device class.

 Contains several device fields like device type, status, config
 generation, device and driver features, and function pointers
 to operations that reset the device, read/write its configuration, and
 perform buffer processing.

 It also contains a pointer to the virtqueues that correspond to the device,
 and a private device pointer, to use at the coder's discretion.
*/
typedef struct vdev_s {
	u32 id;	/**< Internal device ID */
	const u16 dev_id;	/**< Device class */
	u16 status;	/**< Current device status */
	u32 cfg;	/**< Device configuration generation */

	union {
		u64 dword;
		struct { u32 lo, hi; };
	} device_feat, driver_feat;	/**< Device and driver feature bits. */

	/**< Perform any needed hardware/software reset */
	void (*hard_reset)(vdev_s*);

	/**< Read a byte from the config space */
	u8 (*read_cfg)(vdev_s*, uint);

	/**< Write a byte to the config space */
	void (*write_cfg)(vdev_s*, uint, u8);

	/**< Start processing any pending jobs in this virtqueue */
	void (*process_vqueue)(vdev_s*, vqueue_s*);

	vqueue_s *const vqs;	/**< Base of virtqueue array */
	const uint vqn;	/**< Number of virtqueues associated to this device */

	void *priv;	/**< Private device data */
} vdev_s;

/**
	First-time VirtIO initialization. Only called by the manager.
*/
void vdev_init(vdev_s *vdev, uint i);

/**
	VirtIO device reset function. Resets all internal
	registers and calls the hardware reset operation.
*/
void vdev_reset(vdev_s *vdev);

/** Gets the internal ID of a virtual device */
#define vdev_id(v)	((v)->id)

/**
	Returns a specific VirtQueue associated to the device
	if it exists, otherwise returns a NULL pointer.
*/
vqueue_s *vdev_get_queue(vdev_s *vdev, uint i);

u32 vdev_reg_read(vdev_s *vdev, uint reg);
void vdev_reg_write(vdev_s *vdev, uint reg, u32 val);

u32 vdev_queue_reg_read(vdev_s *v, uint vqn, u32 reg);
void vdev_queue_reg_write(vdev_s *v, uint vqn, u32 reg, u32 val);

/** VirtDev operation wrappers */
#define vdev_hard_reset(v)	((v)->hard_reset)(v)
#define vdev_cfg_read(v, off)	((v)->read_cfg)(v, off)
#define vdev_cfg_write(v, off, data)	((v)->write_cfg)(v, off, data)
#define vdev_process_vqueue(v, vq)	((v)->process_vqueue)(v, vq)

/**
	VirtIO device stub functions
	These are "compatible" with the interface but dont really
	do anything and barely use any code space.
	Use as stubs when a function isn't really necessary.
*/
void vdev_reset_stub(vdev_s *vdev);
u8 vdev_cfg_read_stub(vdev_s *vdev, uint off);
void vdev_cfg_write_stub(vdev_s *vdev, uint off, u8 val);
void vdev_process_vqueue_stub(vdev_s *vdev, vqueue_s *vq);
