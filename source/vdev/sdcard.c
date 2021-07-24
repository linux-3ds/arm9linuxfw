#include "common.h"

#include "hw/sdmmc.h"
#include "virt/manager.h"

#define VIRTIO_BLK_F_RO	BIT(5)
#define VIRTIO_BLK_F_BLK_SIZE	BIT(6)

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

static void sdmc_rx(vdev_s *vdev, vqueue_s *vq, const vblk_t *blk, vjob_s *vj, vdesc_s *vd) {
	// send data from SD to linux

	// first descriptor is a simple vblk (RO), then
	// a 512*n byte buffer (RW) descriptor, and finally the status byte

	u32 offset = blk->sector_offset;

	// all data needs to be handled in separate descriptors
	while(vqueue_fetch_job_next(vq, vj) >= 0) {
		u8 *data;
		vdesc_s desc;

		vqueue_get_job_desc(vq, vj, &desc);

		data = desc.data;

		if (desc.length % 512) {
			// assume status byte
			*data = 0;
		} else {
			u32 count = desc.length / 512;
			sdmmc_sdcard_readsectors(offset, count, data);

			offset += count * 512;
			vjob_add_written(vj, count * 512);
		}
	}
}

static void sdmc_tx(vdev_s *vdev, vqueue_s *vq, const vblk_t *blk, vjob_s *vj, vdesc_s *vd) {
	// single continuous descriptor that contains everything?

	u32 offset, count;
	u8 *data = vd->data + sizeof(*blk);

	offset = blk->sector_offset;
	count = vd->length - sizeof(*blk) - 1;

	DBG_ASSERT(!(count % 512));

	count /= 512;

	sdmmc_sdcard_writesectors(offset, count, data);
	data[count * 512] = 0; // status byte
}

static void sdmc_process_vqueue(vdev_s *vdev, vqueue_s *vq) {
	vjob_s vjob;

	while(vqueue_fetch_job_new(vq, &vjob) >= 0) {
		vblk_t vblk;
		vdesc_s desc;
		vqueue_get_job_desc(vq, &vjob, &desc);

		vblk = *(volatile vblk_t*)(desc.data);

		switch(vblk.type) {
		case 0: // VIRTIO_BLK_T_IN
			sdmc_rx(vdev, vq, &vblk, &vjob, &desc);
			break;

		case 1: // VIRTIO_BLK_T_OUT
			sdmc_tx(vdev, vq, &vblk, &vjob, &desc);
			break;
		}

		vqueue_push_job(vq, &vjob);
		vman_notify_host(vdev, VIRQ_VQUEUE);
	}

/*
			// categorize data by request size first
			switch(desc.length) {
			case 1: // status byte
			{
				// blindly assume there's no error
				*(u8*)(desc.data) = 0;
				break;
			}

			case sizeof(vblk_t): // block request header
			{
				*vdev_vblk = *(volatile vblk_t*)(desc.data);
				OBJ_SETPRIV(vq, (u32)vdev_vblk->sector_offset);
				break;
			}

			default: // probably data
			{
				u8 *data;
				u32 count, offset;

				data = desc.data;
				count = desc.length;
				offset = OBJ_GETPRIV(vq, u32);

				if (count % 512) // wtf?
					break;

				count /= 512;
				// given in multiples of 512

				if (vdev_vblk->type == 0) {
					// VIRTIO_BLK_T_IN
					sdmmc_sdcard_readsectors(offset, count, data);
					vjob_add_written(&vjob, count << 9);
				} else if (vdev_vblk->type == 1) {
					// VIRTIO_BLK_T_OUT
					sdmmc_sdcard_writesectors(offset, count, data);
				}

				OBJ_SETPRIV(vq, offset + count);
				break;
			}
			}
		} while(vqueue_fetch_job_next(vq, &vjob) >= 0);
		vqueue_push_job(vq, &vjob);
	}*/

	//vman_notify_host(vdev, VIRQ_VQUEUE);
}

DECLARE_VIRTDEV(
	vdev_sdcard, NULL,
	VDEV_T_BLOCK, VIRTIO_BLK_F_BLK_SIZE, 1,
	sdmc_hard_reset,
	sdmc_cfg_read, vdev_cfg_write_stub,
	sdmc_process_vqueue
);
