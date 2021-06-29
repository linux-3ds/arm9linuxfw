#pragma once

#include "common.h"
#include "sys/list.h"

#include "virt/desc.h"

/** There can only be up to 128 virtqueues per device */
#define VIRTQUEUE_MAX_COUNT	128

/** Each virtqueue can only hold up to 32k items */
#define VIRTQUEUE_MAX_DESC	32768

/** Virtual queue object */
typedef struct vqueue_s {
	list_node_s node;	/**< node within the pending vqueue list in the parent device */

	u8 owner;	/**< queue owner internal device ID */

	struct {
		u8 id : 7;	/**< queue index within the device */
		u8 ready : 1;	/**< ready state */
	};

	u16 size_mask;	/**< size mask for all operations (amount of descriptors - 1) */
	u16 avail_idx;	/**< last available index */
	u16 used_idx;	/**< last used index */
	u32 q_desc;	/**< queue descriptor address */
	u32 q_avail;	/**< queue available ring address */
	u32 q_used;	/**< queue used ring address */
	void *priv;
} vqueue_s;

void vqueue_init(vqueue_s *vq, uint owner, uint i);

void vqueue_reset(vqueue_s *vq);

u32 vqueue_reg_read(vqueue_s *vq, uint reg);
void vqueue_reg_write(vqueue_s *vq, uint reg, u32 val);

int vqueue_fetch_avail_first(vqueue_s *vq);
int vqueue_fetch_avail_next(vqueue_s *vq, u16 prev);
void vqueue_push_used(vqueue_s *vq, u16 first, u32 len);

void vqueue_get_desc(vqueue_s *vq, u16 index, vdesc_s *desc);
