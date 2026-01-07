#ifndef RESETS_H
#define RESETS_H

#include <stdint.h>

#include "rp2350-map.h"
#include "helpers.h"

#define	RESETS_RESET_REG		0
#define	RESETS_RESET_DONE_REG		2

#define	RESETS_USBCTRL		28
#define	RESETS_UART1		27
#define	RESETS_UART0		26
#define	RESETS_TRNG		25
#define	RESETS_TIMER1		24
#define	RESETS_TIMER0		23
#define	RESETS_TBMAN		22
#define	RESETS_SYSINFO		21
#define	RESETS_SYSCFG		20
#define	RESETS_SPI1		19
#define	RESETS_SPI0		18
#define	RESETS_SHA256		17
#define	RESETS_PWM		16
#define	RESETS_PLL_USB		15
#define	RESETS_PLL_SYS		14
#define	RESETS_PIO2		13
#define	RESETS_PIO1		12
#define	RESETS_PIO0		11
#define	RESETS_PADS_QSPI	10
#define	RESETS_PADS_BANK0	9
#define	RESETS_JTAG		8
#define	RESETS_IO_QSPI		7
#define	RESETS_IO_BANK0		6
#define	RESETS_I2C1		5
#define	RESETS_I2C0		4
#define	RESETS_HSTX		3
#define	RESETS_DMA		2
#define	RESETS_BUSCTRL		1
#define	RESETS_ADC		0

/*
 * A reset_val of 1 means the device is in reset.
 * A reset_val of 0 means the device is not in reset.
 */
static inline void set_reset(uint8_t reset_num, uint8_t reset_val) {
	if (reset_val)
		RESETS_BASE[RESETS_RESET_REG] |= BIT(reset_num);
	else
		RESETS_BASE[RESETS_RESET_REG] &= NBIT(reset_num);

	loop_until_bit_is_set(RESETS_BASE[RESETS_RESET_DONE_REG], reset_num);
}

#endif /* RESETS_H */
