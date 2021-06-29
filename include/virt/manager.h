#pragma once

typedef struct vdesc_s vdesc_s;

typedef struct vdev_s vdev_s;
typedef struct vqueue_s vqueue_s;

#include "sys/list.h"

#include "virt/job.h"
#include "virt/irq.h"
#include "virt/device.h"
#include "virt/queue.h"

/** Version implemented */
#define VIRT_MANAGER_VERSION	(1)

/** Set an object's private data (must fit in 4 bytes) */
#define OBJ_SETPRIV(v, p)	do{(v)->priv = (void*)(p);}while(0)

/** Retrieve an object's private data, with typecasting */
#define OBJ_GETPRIV(v, t)	((t)((v)->priv))


/**
	Magic macro that declares a VirtIO device and automatically
	adds it to the global device array at compile time

	@name	- name of the virtual device object
	@prv	- private data pointer
	@dclass	- virtual device type id
	@feat	- extra device features
	@hrst	- hardReset function pointer
	@rcfg	- rdCfg function pointer
	@wcfg	- wrCfg function pointer
	@pbuf	- prBuf function pointer
*/
#define DECLARE_VIRTDEV(name, prv, dclass, feat, vn, hrst, rcfg, wcfg, pbuf) \
	vqueue_s KEEP name##_vqs[vn]; \
	vdev_s KEEP name = { \
		.dev_id = (dclass), \
		.status = 0, \
		.cfg = 0, \
		.device_feat.dword = (feat) | VDEV_F_DEFAULT, \
		.driver_feat.dword = 0, \
		.hard_reset = (hrst), \
		.read_cfg = (rcfg), \
		.write_cfg = (wcfg), \
		.process_vqueue = (pbuf), \
		.vqs = name##_vqs, \
		.vqn = (vn), \
		.priv = (prv) \
	}; \
	const u32 KEEP SECTION(".vdev_list") name##_entry = (u32)(&name);

/**
	Initialize device array. MUST be done only once on boot
*/
void vman_init_all(void);

/**
	Returns the number of devices present in the system
*/
uint vman_dev_count(void);

/**
	Get the pointer to a specific virtual device in the system
	if it exists, or NULL otherwise
*/
vdev_s *vman_get_dev(uint n);

/**
	Reads a device register if valid, otherwise returns 0
*/
u32 vman_reg_read(uint dev, uint reg);

/**
	Writes a device register
*/
void vman_reg_write(uint dev, uint reg, u32 val);

/**
	Get the virtual device that owns the virtqueue
*/
vdev_s *vman_vq_owner(vqueue_s *vq);

/**
	Process pending VirtBuffers in the system
	Returns false if there are no more buffers left to process
*/
bool vman_process_pending(void);

/**
	Adds a virtQueue to the pending queue list
	Returns false if it was already present
*/
bool vman_add_pending(vqueue_s *vq);

/**
	Notify the host about changes made to the device
*/
static inline void vman_notify_host(vdev_s *vdev, uint mode) {
	virtirq_set(vdev_id(vdev), mode);
	virtirq_sync();
}

static inline int vqueue_fetch_job_new(vqueue_s *vq, vjob_s *vjob) {
	int first = vqueue_fetch_avail_first(vq);
	vjob->first = first;
	vjob->current = first;
	vjob->total_written = 0;
	/* checking whether this is valid is left up to the caller */
	return first;
}

static inline int vqueue_fetch_job_next(vqueue_s *vq, vjob_s *vjob) {
	int next = vqueue_fetch_avail_next(vq, vjob->current);
	vjob->current = next;
	/* just like before, the caller needs to check it */
	return next;
}

static inline void vqueue_get_job_desc(vqueue_s *vq, const vjob_s *vj,
					vdesc_s *vd) {
	vqueue_get_desc(vq, vj->current, vd);
}

static inline void vqueue_push_job(vqueue_s *vq, const vjob_s *vjob) {
	vqueue_push_used(vq, vjob->first, vjob->total_written);
}
