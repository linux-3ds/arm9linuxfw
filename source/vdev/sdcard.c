#include "common.h"

#include "hw/sdmmc.h"
#include "virt/manager.h"

#define VIRTIO_BLK_F_SIZE_MAX	BIT(1)
#define VIRTIO_BLK_F_SEG_MAX	BIT(2)
#define VIRTIO_BLK_F_READONLY	BIT(5)
#define VIRTIO_BLK_F_BLK_SIZE	BIT(6)

#define VIRTIO_BLK_TYPE_IN	0
#define VIRTIO_BLK_TYPE_OUT	1

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
	u32 type;
	u32 resv;
	u64 sector_offset;
} PACKED vblk_t;

static blk_config sdmc_blk_config;

static void sdmc_hard_reset(vdev_s *vdev) {
	u8 *data = (u8*)&sdmc_blk_config;
	for (uint i = 0; i < sizeof(sdmc_blk_config); i++)
		data[i] = 0;
	sdmmc_sdcard_init();
	sdmc_blk_config.capacity = sdmmc_sdcard_size();
	sdmc_blk_config.blk_size = 512u;
	sdmc_blk_config.size_max = 512u * ((1u << 16) - 1); // limited by hardware
	sdmc_blk_config.seg_max = 16u; // picked by a fair dice roll
}

static u8 sdmc_cfg_read(vdev_s *vdev, uint offset) {
	if (offset < sizeof(sdmc_blk_config))
		return ((u8*)(&sdmc_blk_config))[offset];
	return 0xFF;
}

static void sdmc_txrx(vqueue_s *vq, const vblk_t *blk, vjob_s *vj) {
	int err = 0;
	u32 type = blk->type, offset = blk->sector_offset;

	while(vqueue_fetch_job_next(vq, vj) >= 0) {
		vdesc_s desc;
		vqueue_get_job_desc(vq, vj, &desc);

		u8 *data = desc.data;
		u32 len = desc.length;

		if ((len % 512) == 0) {
			u32 nsect = len / 512;

			if (type == VIRTIO_BLK_TYPE_IN) {
				// SD -> memory
				err |= sdmmc_sdcard_readsectors(offset, nsect, data) ? 1 : 0;
				vjob_add_written(vj, len);
			} else if (type == VIRTIO_BLK_TYPE_OUT) {
				// memory -> SD
				err |= sdmmc_sdcard_writesectors(offset, nsect, data) ? 1 : 0;
			} else {
				__builtin_trap();
				__builtin_unreachable();
			}

			data += len;
			offset += nsect;
		} else if ((len % 512) == 1) {
			// assume single status byte
			*data = err;
		} else {
			__builtin_trap();
			__builtin_unreachable();
		}
	}
}

static void sdmc_process_vqueue(vdev_s *vdev, vqueue_s *vq) {
	vjob_s vjob;

	while(vqueue_fetch_job_new(vq, &vjob) >= 0) {
		vblk_t vblk;
		vdesc_s desc;
		vqueue_get_job_desc(vq, &vjob, &desc);

		vblk = *(volatile vblk_t*)(desc.data);

		sdmc_txrx(vq, &vblk, &vjob);

		vqueue_push_job(vq, &vjob);
		vman_notify_host(vdev, VIRQ_VQUEUE);
	}
}

DECLARE_VIRTDEV(
	vdev_sdcard, NULL,
	VDEV_T_BLOCK, VIRTIO_BLK_F_SIZE_MAX | VIRTIO_BLK_F_SEG_MAX | VIRTIO_BLK_F_BLK_SIZE, 1,
	sdmc_hard_reset,
	sdmc_cfg_read, vdev_cfg_write_stub,
	sdmc_process_vqueue
);
