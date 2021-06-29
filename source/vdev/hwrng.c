#include "common.h"

#include "virt/manager.h"

#define REG_PRNG_BASE	(0x10011000)

typedef struct {
	vu32 cold;
	u32 resv1[3];

	vu32 hot;
	u32 resv2[3];

	vu16 cold_seed;
	vu16 hot_seed;
} PACKED prng_regs;

static prng_regs *get_prng_regs(void) {
	return (prng_regs*)(REG_PRNG_BASE);
}

static void hwrng_process_vqueue(vdev_s *vdev, vqueue_s *vq) {
	vjob_s vjob;
	prng_regs *regs = get_prng_regs();

	while(vqueue_fetch_job_new(vq, &vjob) >= 0) {
		do {
			u32 *data;
			vdesc_s desc;
			vqueue_get_job_desc(vq, &vjob, &desc);

			if (desc.dir == HOST_TO_VDEV)
				continue;

			data = desc.data;
			for (uint i = 0; i < (desc.length/4); i++)
				data[i] = regs->hot;
			vjob_add_written(&vjob, desc.length);
		} while(vqueue_fetch_job_next(vq, &vjob) >= 0);

		vqueue_push_job(vq, &vjob);
	}

	vman_notify_host(vdev, VIRQ_VQUEUE);
}

DECLARE_VIRTDEV(
	vdev_hwrng, NULL,
	VDEV_T_HWRNG, 0, 1,
	vdev_reset_stub,
	vdev_cfg_read_stub, vdev_cfg_write_stub,
	hwrng_process_vqueue
);
