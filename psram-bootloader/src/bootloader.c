/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "boot/picobin.h"
#include "hardware/watchdog.h"
#include "hardware/structs/qmi.h"
#include "hardware/structs/xip_ctrl.h"

#if defined(SPARKFUN_PROMICRO_RP2350)

// For the pro micro rp2350
#define SFE_RP2350_XIP_CSI_PIN 19
#endif

#define PSRAM_LOCATION _u(0x11000000)


#define PART_LOC_FIRST(x) ( ((x) & PICOBIN_PARTITION_LOCATION_FIRST_SECTOR_BITS) >> \
			   PICOBIN_PARTITION_LOCATION_FIRST_SECTOR_LSB )
#define PART_LOC_LAST(x) ( ((x) & PICOBIN_PARTITION_LOCATION_LAST_SECTOR_BITS) >> \
			   PICOBIN_PARTITION_LOCATION_LAST_SECTOR_LSB )

static size_t _psram_size = 0;

static size_t __no_inline_not_in_flash_func(setup_psram)(uint psram_cs_pin);
static void hexdump(const void *data, size_t size);
static int rom_test(void **data_addr, size_t* data_size);
static int psram_setup_and_test(void);
static int test_executability(void* addr);
static int wait_for_input(const char *msg);

int main() {
	int ret;
	uint32_t jump_ret, data;
	size_t data_size;
	void *data_addr, *ram_addr = (void *)PSRAM_LOCATION;

	stdio_init_all();
	wait_for_input("Waiting for input\r");


	ret = psram_setup_and_test();
	if (ret) {
		goto exit;
	}

	ret = rom_test(&data_addr, &data_size);
	if (ret) {
		goto exit;
	}

	if (data_size > _psram_size) {
		printf("Data size 0x%04x is larger than PSRAM size 0x%04x\n",
			data_size, _psram_size);
		goto exit;
	}

	printf("\nRom dump:\n");
	hexdump(data_addr, 0x20);

	memcpy(ram_addr, data_addr, data_size);
	printf("\nRam dump:\n");
	hexdump(ram_addr, 0x20);

	ret = wait_for_input("Press y to jump to PSRAM...\r");
	if (ret == 'y' || ret == 'Y') {
		data = (uint32_t)printf;
		jump_ret = ((uint32_t (*)(void *))ram_addr)((void *)data);
		printf("Jump to PSRAM returned 0x%08x\n", jump_ret);
	}

exit:
	wait_for_input("Press any key to reset.\r");
	puts("Resetting...");
	watchdog_reboot(0, 0, 0);
	return 0;
}

static int wait_for_input(const char *msg) {
	int ret;

	while (1) {
		printf(msg);
		ret = getchar_timeout_us(0);
		if (ret != PICO_ERROR_TIMEOUT) {
			puts("");
			return ret;
		}
		sleep_ms(1000);
	}
}

static int test_executability(void* addr) {
	const uint16_t ret_inst = 0x8082;
	size_t addr_aligned = ((size_t)addr) & ~1;
	volatile uint16_t* addr_ptr = (volatile uint16_t*)addr_aligned;

	__mem_fence_acquire();
	*addr_ptr = ret_inst;
	__mem_fence_release();

	printf("Jumping to 0x%08x, aligned from 0x%08x\n", addr_aligned, (size_t)addr);
	printf("Function pointers: 0x%02x 0x%02x\n", ((uint8_t*)addr_ptr)[0], ((uint8_t*)addr_ptr)[1]);

	if (*addr_ptr != ret_inst) {
		printf("ERROR: Expected 0x%04x, got 0x%04x\n", ret_inst, *addr_ptr);
		return -1;
	}

	((void (*)(void))addr_aligned)();
	return 0;
}

static int psram_setup_and_test(void) {
	_psram_size = setup_psram(SFE_RP2350_XIP_CSI_PIN);

	if (!_psram_size) {
		printf("PSRAM setup failed\n");
		return -1;
	}

	printf("PSRAM setup complete. PSRAM size 0x%lX (%d)\n", _psram_size, _psram_size);

	return test_executability((size_t *)(PSRAM_LOCATION + _psram_size - 4));
}

static int rom_test(void **data_addr, size_t* data_size) {
	static __attribute__((aligned(4))) uint32_t workarea[1024];
	uint32_t data_end_addr, *data_start_addr = ((uint32_t *)data_addr);
	int rc;

	rc = rom_load_partition_table((uint8_t *)workarea, sizeof(workarea), false);
	if (rc) {
		printf("Partition Table Load failed %d - resetting\n", rc);
		return -1;
	}

	rc = rom_get_partition_table_info((uint32_t*)workarea, 0x8, PT_INFO_PARTITION_LOCATION_AND_FLAGS | PT_INFO_SINGLE_PARTITION);
	if (rc != 3) {
		printf("No boot partition - assuming bin at start of flash\n");
		return -1;
	}

	*data_start_addr = PART_LOC_FIRST(workarea[1]) * 0x1000;
	data_end_addr = (PART_LOC_LAST(workarea[1]) + 1) * 0x1000;
	*data_size = data_end_addr - *data_start_addr;
	*data_start_addr += XIP_BASE;
	printf("Partition Start 0x%04x, End 0x%04x, Size: 0x%04x\n",
	       *data_start_addr, data_end_addr, *data_size);

	return 0;
}

static void hexdump(const void *data, size_t size) {
	const uint8_t *data_ptr = (const uint8_t *)data;
	size_t i, b;

	for (i = 0; i < size; i++) {
		if (i % 16 == 0) {
			printf("%08x  ", (uint32_t)data_ptr + i);
		}
		if (i % 8 == 0) {
			printf(" ");
		}
		printf("%02x ", data_ptr[i]);
		if (i % 16 == 15) {
			printf(" |");
			for (b = 0; b < 16; b++){
				if (isprint(data_ptr[i + b - 15])) {
					printf("%c", data_ptr[i + b - 15]);
				} else {
					printf(".");
				}
			}
			printf("|\n");
		}
	}
	printf("%08x\n", 16 + size - (size%16));
}

// The ID check is from the Circuit Python code that was downloaded from:
// https://github.com/raspberrypi/pico-sdk-rp2350/issues/12#issuecomment-2055274428
//
// in the file supervisor/port.c -- the function setup_psram()

// NOTE: The PSRAM IC used by the Circuit Python example is:
//	  https://www.adafruit.com/product/4677
//
// The PSRAM IC used by the SparkFun Pro Micro is: apmemory APS6404L-3SQR-ZR
// https://www.mouser.com/ProductDetail/AP-Memory/APS6404L-3SQR-ZR?qs=IS%252B4QmGtzzpDOdsCIglviw%3D%3D
//
// The datasheets from both these IC's are almost identical (word for word), with the first being ESP32 branded
//

static size_t __no_inline_not_in_flash_func(setup_psram)(uint psram_cs_pin)
{
	if (!psram_cs_pin)
		return 0;
	gpio_set_function(psram_cs_pin, GPIO_FUNC_XIP_CS1);

	size_t psram_size = 0;
	uint32_t intr_stash = save_and_disable_interrupts();

	// Try and read the PSRAM ID via direct_csr.
	qmi_hw->direct_csr = 30 << QMI_DIRECT_CSR_CLKDIV_LSB | QMI_DIRECT_CSR_EN_BITS;
	// Need to poll for the cooldown on the last XIP transfer to expire
	// (via direct-mode BUSY flag) before it is safe to perform the first
	// direct-mode operation
	while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) != 0)
	{
	}

	// Exit out of QMI in case we've inited already
	qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
	// Transmit as quad.
	qmi_hw->direct_tx = QMI_DIRECT_TX_OE_BITS | QMI_DIRECT_TX_IWIDTH_VALUE_Q << QMI_DIRECT_TX_IWIDTH_LSB | 0xf5;
	while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) != 0)
	{
	}
	(void)qmi_hw->direct_rx;
	qmi_hw->direct_csr &= ~(QMI_DIRECT_CSR_ASSERT_CS1N_BITS);

	// Read the id
	qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
	uint8_t kgd = 0;
	uint8_t eid = 0;
	for (size_t i = 0; i < 7; i++)
	{
		if (i == 0)
		{
			qmi_hw->direct_tx = 0x9f;
		}
		else
		{
			qmi_hw->direct_tx = 0xff;
		}
		while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_TXEMPTY_BITS) == 0)
		{
		}
		while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) != 0)
		{
		}
		if (i == 5)
		{
			kgd = qmi_hw->direct_rx;
		}
		else if (i == 6)
		{
			eid = qmi_hw->direct_rx;
		}
		else
		{
			(void)qmi_hw->direct_rx;
		}
	}
	// Disable direct csr.
	qmi_hw->direct_csr &= ~(QMI_DIRECT_CSR_ASSERT_CS1N_BITS | QMI_DIRECT_CSR_EN_BITS);

	if (kgd != 0x5D)
	{
		printf("Invalid PSRAM ID: %x\n", kgd);
		restore_interrupts(intr_stash);
		return psram_size;
	}

	// Enable quad mode.
	qmi_hw->direct_csr = 30 << QMI_DIRECT_CSR_CLKDIV_LSB | QMI_DIRECT_CSR_EN_BITS;
	// Need to poll for the cooldown on the last XIP transfer to expire
	// (via direct-mode BUSY flag) before it is safe to perform the first
	// direct-mode operation
	while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) != 0)
	{
	}

	// RESETEN, RESET and quad enable
	for (uint8_t i = 0; i < 3; i++)
	{
		qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
		if (i == 0)
		{
			qmi_hw->direct_tx = 0x66;
		}
		else if (i == 1)
		{
			qmi_hw->direct_tx = 0x99;
		}
		else
		{
			qmi_hw->direct_tx = 0x35;
		}
		while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) != 0)
		{
		}
		qmi_hw->direct_csr &= ~(QMI_DIRECT_CSR_ASSERT_CS1N_BITS);
		for (size_t j = 0; j < 20; j++)
		{
			asm("nop");
		}
		(void)qmi_hw->direct_rx;
	}
	// Disable direct csr.
	qmi_hw->direct_csr &= ~(QMI_DIRECT_CSR_ASSERT_CS1N_BITS | QMI_DIRECT_CSR_EN_BITS);

	qmi_hw->m[1].timing =
		QMI_M1_TIMING_PAGEBREAK_VALUE_1024 << QMI_M1_TIMING_PAGEBREAK_LSB | // Break between pages.
		3 << QMI_M1_TIMING_SELECT_HOLD_LSB | // Delay releasing CS for 3 extra system cycles.
		1 << QMI_M1_TIMING_COOLDOWN_LSB | 1 << QMI_M1_TIMING_RXDELAY_LSB |
		16 << QMI_M1_TIMING_MAX_SELECT_LSB |  // In units of 64 system clock cycles. PSRAM says 8us max. 8 / 0.00752 /64
											  // = 16.62
		7 << QMI_M1_TIMING_MIN_DESELECT_LSB | // In units of system clock cycles. PSRAM says 50ns.50 / 7.52 = 6.64
		2 << QMI_M1_TIMING_CLKDIV_LSB;
	qmi_hw->m[1].rfmt = (QMI_M1_RFMT_PREFIX_WIDTH_VALUE_Q << QMI_M1_RFMT_PREFIX_WIDTH_LSB |
						 QMI_M1_RFMT_ADDR_WIDTH_VALUE_Q << QMI_M1_RFMT_ADDR_WIDTH_LSB |
						 QMI_M1_RFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M1_RFMT_SUFFIX_WIDTH_LSB |
						 QMI_M1_RFMT_DUMMY_WIDTH_VALUE_Q << QMI_M1_RFMT_DUMMY_WIDTH_LSB |
						 QMI_M1_RFMT_DUMMY_LEN_VALUE_24 << QMI_M1_RFMT_DUMMY_LEN_LSB |
						 QMI_M1_RFMT_DATA_WIDTH_VALUE_Q << QMI_M1_RFMT_DATA_WIDTH_LSB |
						 QMI_M1_RFMT_PREFIX_LEN_VALUE_8 << QMI_M1_RFMT_PREFIX_LEN_LSB |
						 QMI_M1_RFMT_SUFFIX_LEN_VALUE_NONE << QMI_M1_RFMT_SUFFIX_LEN_LSB);
	qmi_hw->m[1].rcmd = 0xeb << QMI_M1_RCMD_PREFIX_LSB | 0 << QMI_M1_RCMD_SUFFIX_LSB;
	qmi_hw->m[1].wfmt = (QMI_M1_WFMT_PREFIX_WIDTH_VALUE_Q << QMI_M1_WFMT_PREFIX_WIDTH_LSB |
						 QMI_M1_WFMT_ADDR_WIDTH_VALUE_Q << QMI_M1_WFMT_ADDR_WIDTH_LSB |
						 QMI_M1_WFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M1_WFMT_SUFFIX_WIDTH_LSB |
						 QMI_M1_WFMT_DUMMY_WIDTH_VALUE_Q << QMI_M1_WFMT_DUMMY_WIDTH_LSB |
						 QMI_M1_WFMT_DUMMY_LEN_VALUE_NONE << QMI_M1_WFMT_DUMMY_LEN_LSB |
						 QMI_M1_WFMT_DATA_WIDTH_VALUE_Q << QMI_M1_WFMT_DATA_WIDTH_LSB |
						 QMI_M1_WFMT_PREFIX_LEN_VALUE_8 << QMI_M1_WFMT_PREFIX_LEN_LSB |
						 QMI_M1_WFMT_SUFFIX_LEN_VALUE_NONE << QMI_M1_WFMT_SUFFIX_LEN_LSB);
	qmi_hw->m[1].wcmd = 0x38 << QMI_M1_WCMD_PREFIX_LSB | 0 << QMI_M1_WCMD_SUFFIX_LSB;

	psram_size = 1024 * 1024; // 1 MiB
	uint8_t size_id = eid >> 5;
	if (eid == 0x26 || size_id == 2)
	{
		psram_size *= 8;
	}
	else if (size_id == 0)
	{
		psram_size *= 2;
	}
	else if (size_id == 1)
	{
		psram_size *= 4;
	}

	// Mark that we can write to PSRAM.
	xip_ctrl_hw->ctrl |= XIP_CTRL_WRITABLE_M1_BITS;
	restore_interrupts(intr_stash);
	// printf("PSRAM ID: %x %x\n", kgd, eid);
	return psram_size;
}
