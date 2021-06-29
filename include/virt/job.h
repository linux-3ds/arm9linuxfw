#pragma once

#include "common.h"

typedef struct vjob_s {
	u16 first;
	u16 current;
	u32 total_written;
} vjob_s;

static inline void vjob_add_written(vjob_s *vjob, s32 written) {
	vjob->total_written += written;
}
