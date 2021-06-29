#pragma once

#include "common.h"

enum {
	VDEV_TO_HOST = 0,
	HOST_TO_VDEV = 1,
};

typedef struct vdesc_s {
	void *data;
	u32 length;
	int dir;
} vdesc_s;
