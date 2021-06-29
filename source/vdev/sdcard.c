#include "common.h"

#include "hw/sdmmc.h"
#include "virt/manager.h"

#define VIRTIO_BLK_F_RO	BIT(5)

typedef struct {
	u64 capacity;
	u32 size_max;
	u32 seg_max;
	struct virtio_blk_geometry {
			u16 cylinders;
			u8 heads;
			u8 sectors;
	} geometry;
	u32 blk_size;
	struct virtio_blk_topology {
			// # of logical blocks per physical block (log2)
			u8 physical_block_exp;
			// offset of first aligned logical block
			u8 alignment_offset;
			// suggested minimum I/O size in blocks
			u16 min_io_size;
			// optimal (suggested maximum) I/O size in blocks
			u32 opt_io_size;
	} topology;
	u8 writeback;
	u8 unused0[3];
	u32 max_discard_sectors;
	u32 max_discard_seg;
	u32 discard_sector_alignment;
	u32 max_write_zeroes_sectors;
	u32 max_write_zeroes_seg;
	u8 write_zeroes_may_unmap;
	u8 unused1[3];
} PACKED blk_config;

typedef struct {
	u32 resv;
	u32 type;
	u64 sector_offset;
} PACKED vblk_t;

static blk_config sdmc_blk_config;

static void sdmc_hard_reset(vdev_s *vdev) {
	u8 *data = (u8*)&sdmc_blk_config;
	for (uint i = 0; i < sizeof(sdmc_blk_config); i++)
		data[i] = 0;
	sdmmc_sdcard_init();
	sdmc_blk_config.capacity = sdmmc_sdcard_size();
	sdmc_blk_config.blk_size = 512;
}

static u8 sdmc_cfg_read(vdev_s *vdev, uint offset) {
	if (offset < sizeof(sdmc_blk_config))
		return ((u8*)(&sdmc_blk_config))[offset];
	return 0xFF;
}

static void sdmc_process_vqueue(vdev_s *vdev, vqueue_s *vq) {
	vjob_s vjob;

	while(vqueue_fetch_job_new(vq, &vjob) >= 0) {
		do {
			u32 *data;
			vdesc_s desc;
			vqueue_get_job_desc(vq, &vjob, &desc);

			if (desc.dir == HOST_TO_VDEV) {
				const vblk_t *blk = (const vblk_t*)desc.data;
				OBJ_SETPRIV(vq, (u32)blk->sector_offset);
			} else {
				u8 *data = desc.data;
				if (desc.length < 512) {
					*data = 0;
				} else {
					u32 sectors = desc.length >> 9;
					sdmmc_sdcard_readsectors(OBJ_GETPRIV(vq, u32), sectors, data);
					OBJ_SETPRIV(vq, OBJ_GETPRIV(vq, u32) + sectors);
					vjob_add_written(&vjob, desc.length);
				}
			}
		} while(vqueue_fetch_job_next(vq, &vjob) >= 0);
		vqueue_push_job(vq, &vjob);
	}

	vman_notify_host(vdev, VIRQ_VQUEUE);
}

DECLARE_VIRTDEV(
	vdev_sdcard, NULL,
	VDEV_T_BLOCK, VIRTIO_BLK_F_RO, 1,
	sdmc_hard_reset,
	sdmc_cfg_read, vdev_cfg_write_stub,
	sdmc_process_vqueue
);
