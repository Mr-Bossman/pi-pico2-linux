#ifndef POWER_H
#define POWER_H

#include <stdint.h>

#include "rp2350-map.h"
#include "helpers.h"

#define PSM_FRCE_ON_REG		0
#define PSM_FRCE_OFF_REG	1
#define PSM_DONE_REG		3

#define PSM_PROC1	24
#define PSM_PROC0	23
#define PSM_ACCESSCTRL	22
#define PSM_SIO		21
#define PSM_XIP		20
#define PSM_SRAM9	19
#define PSM_SRAM8	18
#define PSM_SRAM7	17
#define PSM_SRAM6	16
#define PSM_SRAM5	15
#define PSM_SRAM4	14
#define PSM_SRAM3	13
#define PSM_SRAM2	12
#define PSM_SRAM1	11
#define PSM_SRAM0	10
#define PSM_BOOTRAM	9
#define PSM_ROM		8
#define PSM_BUSFABRIC	7
#define PSM_PSM_READY	6
#define PSM_CLOCKS	5
#define PSM_RESETS	4
#define PSM_XOSC	3
#define PSM_ROSC	2
#define PSM_OTP		1
#define PSM_PROC_COLD	0

static inline void set_power(uint8_t psm_num, uint8_t psm_val) {
	if (psm_val)
		PSM_BASE[PSM_FRCE_ON_REG] |= BIT(psm_num);
	else
		PSM_BASE[PSM_FRCE_OFF_REG] |= BIT(psm_num);

	loop_until_bit_is_set(PSM_BASE[PSM_DONE_REG], psm_num);
}

#endif /* POWER_H */
