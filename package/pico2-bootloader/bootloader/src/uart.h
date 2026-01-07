#ifndef UART_H
#define UART_H

#include <stdint.h>

void uart_init(void);

int uart_getc(void);
int uart_putc(int c);
int uart_puts(const char *str);

#endif /* UART_H */
