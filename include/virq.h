#pragma once

#include "common.h"

void virq_reset(void);

void virq_unmask(u32 irqn);
void virq_mask(u32 irqn);
u32 virq_get_pending(void);
