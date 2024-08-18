#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

int entry(void* data);

void _entry_(void* data) {
	uint32_t *regs = (uint32_t*)data;
	// a0 is in regs[9] which is the first argument and return value
	regs[9] = entry((void *)regs[9]);
}

int entry(void *data) {
	int (*pi_printf)(const char * format, ...) = (int (*)(const char * format, ...))data;
	pi_printf("this is super cursed\n");
	pi_printf("this is super cursed\n");
	return 0;
}
