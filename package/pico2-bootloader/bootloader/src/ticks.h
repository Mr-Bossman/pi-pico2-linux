#ifndef TICKS_H
#define TICKS_H

#include <stdint.h>

#include "rp2350-map.h"
#include "helpers.h"

#define TICKS_RISCV_CTRL_REG	15
#define TICKS_RISCV_CYCKES_REG	16

#define TICKS_CYCLE_SHIFT	0
#define TICKS_CYCLE_MASK	0x1ff
#define TICKS_CRTL_ENABLE_SHIFT	0

static inline void riscv_ticks_init(uint16_t cycles) {
	TICK_BASE[TICKS_RISCV_CYCKES_REG] = (cycles & TICKS_CYCLE_MASK) << TICKS_CYCLE_SHIFT;
	TICK_BASE[TICKS_RISCV_CTRL_REG] |= BIT(TICKS_CRTL_ENABLE_SHIFT);
}

#endif /* TICKS_H */
