#include "ctype_local.h"
#include "string_local.h"
#include "stdio_local.h"

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#include "uart.h"

int isxdigit(int c) {
	if (c >= '0' && c <= '9')
		return 1;
	if (c >= 'a' && c <= 'f')
		return 1;
	if (c >= 'A' && c <= 'F')
		return 1;
	return 0;
}

int isdigit(int c) {
	if (c >= '0' && c <= '9')
		return 1;
	return 0;
}

int isupper(int c) {
	if (c >= 'A' && c <= 'Z')
		return 1;
	return 0;
}

int islower(int c) {
	if (c >= 'a' && c <= 'z')
		return 1;
	return 0;
}

int isalpha(int c) {
	return islower(c) || isupper(c);
}

int isalnum(int c) {
	return isalpha(c) || isdigit(c);
}

int isprint(int c) {
	if (c >= ' ' && c <= '~')
		return 1;
	return 0;
}

int puts(const char *str) {
	uart_puts(str);
	return uart_puts("\n\r");
}

int putchar(int c) {
	return uart_putc(c);
}

void _putchar(char c) {
	uart_putc(c);
	if (c == '\n')
		uart_putc('\r');
}

/*
 * The rest is shamlessly stolen from libgcc
 */

void *
memcpy (void *dest, const void *src, size_t len)
{
  char *d = dest;
  const char *s = src;
  while (len--)
    *d++ = *s++;
  return dest;
}

void *
memset (void *dest, int val, size_t len)
{
  unsigned char *ptr = dest;
  while (len-- > 0)
    *ptr++ = val;
  return dest;
}

int
memcmp (const void *str1, const void *str2, size_t count)
{
  const unsigned char *s1 = str1;
  const unsigned char *s2 = str2;

  while (count-- > 0)
    {
      if (*s1++ != *s2++)
	  return s1[-1] < s2[-1] ? -1 : 1;
    }
  return 0;
}

void *
memmove (void *dest, const void *src, size_t len)
{
  char *d = dest;
  const char *s = src;
  if (d < s)
    while (len--)
      *d++ = *s++;
  else
    {
      const char *lasts = s + (len-1);
      char *lastd = d + (len-1);
      while (len--)
        *lastd-- = *lasts--;
    }
  return dest;
}
