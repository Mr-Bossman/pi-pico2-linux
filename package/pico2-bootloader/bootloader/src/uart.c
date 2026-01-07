#include "uart.h"

#include <stdint.h>

#include "rp2350-map.h"
#include "resets.h"
#include "clocks.h"

#define UART_UARTCR_REG		12
#define UART_UARTCR_UARTEN_BITS	BIT(0)
#define UART_UARTCR_TXE_BITS	BIT(8)
#define UART_UARTCR_RXE_BITS	BIT(9)

#define UART_UARTFR_REG		6
#define UART_UARTFR_TXFF_SHIFT	5
#define UART_UARTFR_RXFE_SHIFT	4

#define UART_UARTIBRD_REG	9
#define UART_UARTFBRD_REG	10

#define UART_LCR_H_REG		11
#define UART_LCR_H_FEN_BITS	BIT(4)
#define UART_WLEN_8		0x3
#define UART_LCR_H_WLEN_SHIFT	5

#define BAUD			115200
#define F_PERIPH		CLK_PERIF

#define UARTBAUDRATE_DIV \
	((8U * (F_PERIPH) / (BAUD)) + 1)

#define UARTIBRD_PRE_VALUE ( (UARTBAUDRATE_DIV) >> (7) )

#define UARTIBRD_VALUE ( ( (UARTIBRD_PRE_VALUE) == 0) ? 1 : \
			( (UARTIBRD_PRE_VALUE) >= 65535 ? 65535 : \
			(UARTIBRD_PRE_VALUE) ) )

#define UARTFBRD_VALUE ( ( ((UARTIBRD_PRE_VALUE) == 0 \
			|| (UARTIBRD_PRE_VALUE) >= 65535) \
			? 0 : (((UARTBAUDRATE_DIV) & 0x7f) >> 1) ) )

void uart_init(void) {
	UART0_BASE[UART_UARTIBRD_REG] = UARTIBRD_VALUE;
	UART0_BASE[UART_UARTFBRD_REG] = UARTFBRD_VALUE;

	UART0_BASE[UART_LCR_H_REG] = (UART_WLEN_8 << UART_LCR_H_WLEN_SHIFT) | UART_LCR_H_FEN_BITS;
	UART0_BASE[UART_UARTCR_REG] = UART_UARTCR_UARTEN_BITS | UART_UARTCR_TXE_BITS |
				      UART_UARTCR_RXE_BITS;
}

static inline void uart_send(char c) {
	loop_until_bit_is_clear(UART0_BASE[UART_UARTFR_REG], UART_UARTFR_TXFF_SHIFT);
	delay(1000);
	*((uint8_t *)UART0_BASE) = c;
}

int uart_putc(int c) {
	uart_send(c);

	return 0;
}

int uart_puts(const char *str) {
	while (*str)
		uart_send(*str++);

	return 0;
}

int uart_getc(void) {
	if (bit_is_set(UART0_BASE[UART_UARTFR_REG], UART_UARTFR_RXFE_SHIFT))
		return -1;

	return *((uint8_t *)UART0_BASE);
}
