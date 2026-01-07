#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

#include "rp2350-map.h"

#define GPIO_FUNC_HSTX			0
#define GPIO_FUNC_SPI			1
#define GPIO_FUNC_UART			2
#define GPIO_FUNC_I2C			3
#define GPIO_FUNC_PWM			4
#define GPIO_FUNC_SIO			5
#define GPIO_FUNC_PIO0			6
#define GPIO_FUNC_PIO1			7
#define GPIO_FUNC_PIO2			8
#define GPIO_FUNC_GPCK			9
#define GPIO_FUNC_CORESIGHT_TRACE	9
#define GPIO_FUNC_XIP_CS1		9
#define GPIO_FUNC_USB			10
#define GPIO_FUNC_UART_AUX		11
#define GPIO_FUNC_NULL			0x1f

static inline void set_pinfunc(uint8_t pin, uint8_t func) {
	IO_BANK0_BASE[1 + (pin * 2)] = func;
}

#endif /* GPIO_H */
