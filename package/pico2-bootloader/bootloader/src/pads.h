#ifndef PADS_H
#define PADS_H

#include <stdint.h>

#include "rp2350-map.h"
#include "helpers.h"

#define PADS_CLEAR	0
#define PADS_SLEWFAST	BIT(0)
#define PADS_SCHMITT	BIT(1)
#define PADS_PDE	BIT(2)
#define PADS_PUE	BIT(3)
#define PADS_IE		BIT(6)
#define PADS_OD		BIT(7)
#define PADS_ISOLATE	BIT(8)

#define PADS_DRIVE_2MA	(0x0U << 4)
#define PADS_DRIVE_4MA	(0x1U << 4)
#define PADS_DRIVE_8MA	(0x2U << 4)
#define PADS_DRIVE_12MA	(0x3U << 4)

static inline void set_config(uint8_t pin, uint32_t conf) {
	PADS_BANK0_BASE[1 + pin] = conf;
}

#endif /* PADS_H */
