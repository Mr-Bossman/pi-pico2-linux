#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include "helpers.h"
#include "qmi.h"

#include "image.h"

#ifdef PICO_SDK
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "pico/stdlib.h"
#else
#include "rp2350-map.h"

#include "pads.h"
#include "resets.h"
#include "clocks.h"
#include "ticks.h"
#include "uart.h"
#include "gpio.h"

#include "ctype_local.h"
#include "stdio_local.h"
#include "string_local.h"
#endif

#define LED_PIN			2
#define RP2350_XIP_CSI_PIN	19

#define PSRAM_LOCATION (0x11000000U)

/* Linker variable for the end of binary image */
#ifdef PICO_SDK
extern size_t __flash_binary_end[];
#else
extern size_t __payload_load_start[];
#endif

#define is_pow2(x) (((x) != 0) && (((x) & ((x) - 1)) == 0))
#define size_ceil(size, align)							\
	({									\
		static_assert(is_pow2(align), "Alignment not a power of 2");	\
		((size) + (align) - 1) & ~((align) - 1);			\
	})

static inline void set_uart0_pinmux(void);
static inline void set_xip_cs1_pinmux(void);

static int wait_for_input(const char *msg);
static void hexdump(const void *data, size_t size);

static inline const void* get_payload_start_address(void);
static int psram_setup_and_test(size_t *psram_size);
static int test_executability(void* addr);
static void jump_to_kernel(void *ram_addr, const void *data_addr);

int main(void) {
	int ret = 0;
	uint32_t jump_ret, data;
	size_t device_tree_size, psram_size, kernel_size = 0;
	void *ram_addr = (void *)PSRAM_LOCATION;
	const void *data_addr;

#ifdef PICO_SDK
	stdio_init_all();
#else
	xosc_init();

	set_reset(RESETS_PLL_USB, 0);
	set_reset(RESETS_PLL_SYS, 0);
	set_usb_pll();
	set_sys_pll();

	set_ref_clock_xosc();
	set_sys_clock_pll_sys();
	set_usb_clock_pll_usb();
	set_adc_clock_pll_usb();
	set_perif_clock_clk_sys();
	set_hstx_clock_clk_sys();

	riscv_ticks_init(CLK_REF / MHZ);

	set_reset(RESETS_UART0, 0);

	set_uart0_pinmux();

	set_config(LED_PIN, PADS_CLEAR);
	set_pinfunc(LED_PIN, GPIO_FUNC_SIO);
	uart_init();

	SIO_BASE[0x38/4] = BIT(LED_PIN); // OE_SET
	SIO_BASE[0x28/4] = BIT(LED_PIN); // OUT_XOR
#endif

	puts("\n\n\nRP2350 Bootloader starting...\n");
	ret = psram_setup_and_test(&psram_size);
	if (ret)
		goto exit;

	data_addr = get_payload_start_address();
	if (!data_addr) {
		printf("Failed to get ROM address is invalid\n");
		goto exit;
	}

	printf("\nRom dump:\n");
	hexdump(data_addr, 0x20);

	device_tree_size = get_devicetree_size(data_addr);

	if (device_tree_size) {
		kernel_size = get_kernel_size(data_addr + device_tree_size);
		if(!kernel_size)
			printf("Kernel not found.\n");
	} else {
		printf("Device Tree not found.\n");
	}

	if ((device_tree_size + kernel_size) > psram_size) {
		printf("Data size 0x%04x is larger than PSRAM size 0x%04x\n",
		       (device_tree_size + kernel_size), psram_size);
		goto exit;
	}

	printf("\nCoping Kernel to RAM...");
	memcpy(ram_addr, data_addr + device_tree_size, kernel_size);
	printf("\rRam dump:              \n");
	hexdump(ram_addr, 0x20);

	if (kernel_size)
		jump_to_kernel(ram_addr, data_addr);

	ret = wait_for_input("Press y to jump to PSRAM...\r");
	if (ret == 'y' || ret == 'Y') {
		data = (uint32_t)printf;
		jump_ret = ((uint32_t (*)(void *))ram_addr)((void *)data);
		printf("Jump to PSRAM returned 0x%08x\n", jump_ret);
	}

exit:
	wait_for_input("Press any key to reset.\r");
	puts("Resetting...");

	return 0;
}

#ifndef PICO_SDK
static inline void set_uart0_pinmux(void) {
	set_config(0, PADS_CLEAR); /* TX, PICO PIN 1 */
	set_config(1, PADS_IE); /* RX, PICO PIN 2 */
	set_pinfunc(0, GPIO_FUNC_UART); /* TX, PICO PIN 1 */
	set_pinfunc(1, GPIO_FUNC_UART); /* RX, PICO PIN 2 */
}

static inline void set_xip_cs1_pinmux(void) {
	set_config(RP2350_XIP_CSI_PIN, PADS_CLEAR);
	set_pinfunc(RP2350_XIP_CSI_PIN, GPIO_FUNC_XIP_CS1);
}
#endif

static int wait_for_input(const char *msg) {
	int ret;

	while (1) {
		printf(msg);
#ifdef PICO_SDK
		ret = getchar_timeout_us(0);
		if (ret != PICO_ERROR_TIMEOUT) {
#else
		ret = uart_getc();
		if (ret != -1) {
#endif
			puts("");
			return ret;
		}
		delay(500000);
	}
}

static inline const void* get_payload_start_address(void) {
#ifdef PICO_SDK
	size_t payload_load = (size_t)&__flash_binary_end;
#else
	size_t payload_load = (size_t)&__payload_load_start;
#endif

	return (const void*)size_ceil(payload_load, 0x10);
}

static int psram_setup_and_test(size_t *psram_size) {
#ifdef PICO_SDK
	gpio_set_function(RP2350_XIP_CSI_PIN, GPIO_FUNC_XIP_CS1);
#else
	set_xip_cs1_pinmux();
#endif
	*psram_size = setup_psram();

	if (!*psram_size) {
		puts("PSRAM setup failed");
		return -1;
	}

	printf("PSRAM setup complete. PSRAM size 0x%lX (%d)", *psram_size, *psram_size);
	puts("");

	return test_executability((size_t *)(PSRAM_LOCATION + (*psram_size) - 4));
}

static int test_executability(void* addr) {
	const uint16_t ret_inst = 0x8082;
	size_t addr_aligned = ((size_t)addr) & ~1;
	volatile uint16_t* addr_ptr = (volatile uint16_t*)addr_aligned;

	mb();
	*addr_ptr = ret_inst;
	mb();

	printf("Jumping to 0x%08x, aligned from 0x%08x\n", addr_aligned, (size_t)addr);
	printf("Function pointers: 0x%02x 0x%02x\n", ((uint8_t*)addr_ptr)[0], ((uint8_t*)addr_ptr)[1]);

	if (*addr_ptr != ret_inst) {
		printf("ERROR: Expected 0x%04x, got 0x%04x\n", ret_inst, *addr_ptr);
		return -1;
	}

	((void (*)(void))addr_aligned)();

	return 0;
}

static void jump_to_kernel(void *ram_addr, const void *data_addr) {
	typedef void (*image_entry_arg_t)(unsigned long hart, const void *dtb);
	image_entry_arg_t image_entry = (image_entry_arg_t)ram_addr;

	printf("\nJumping to kernel at 0x%08x and DT at 0x%08x\n", ram_addr, data_addr);
	printf("If you are using USB serial, please connect over the hardware serial port.\n");
	image_entry(0, data_addr);
}

static void hexdump(const void *data, size_t size) {
	const uint8_t *data_ptr = (const uint8_t *)data;
	size_t i, b;

	for (i = 0; i < size; i++) {
		if (i % 16 == 0)
			printf("%08x  ", (uint32_t)data_ptr + i);
		if (i % 8 == 0)
			printf(" ");

		printf("%02x ", data_ptr[i]);
		if (i % 16 == 15) {
			printf(" |");
			for (b = 0; b < 16; b++){
				if (isprint(data_ptr[i + b - 15]))
					printf("%c", data_ptr[i + b - 15]);
				else
					printf(".");
			}
			printf("|\n");
		}
	}
	printf("%08x\n", 16 + size - (size%16));
}
