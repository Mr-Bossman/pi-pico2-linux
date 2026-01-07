#include "qmi.h"

#include <stdint.h>
#include <stddef.h>

#include "rp2350-map.h"
#include "helpers.h"
#ifdef PICO_SDK
#include <stdio.h>
#else
#include "stdio_local.h"
#endif
#include "linux/irqflags.h"

#define XIP_CTRL_CTRL			0
#define XIP_CTRL_WRITABLE_M1_BITS	BIT(11)

#define QMI_DIRECT_CSR			0
#define QMI_DIRECT_CSR_CLKDIV_LSB	22
#define QMI_DIRECT_CSR_EN_BITS		BIT(0)
#define QMI_DIRECT_CSR_BUSY_BITS	BIT(1)
#define QMI_DIRECT_CSR_ASSERT_CS1N_BITS	BIT(3)
#define QMI_DIRECT_CSR_TXEMPTY_BITS	BIT(11)

#define QMI_DIRECT_TX			1
#define QMI_DIRECT_TX_OE_BITS		BIT(19)
#define QMI_DIRECT_TX_IWIDTH_LSB	16
#define QMI_DIRECT_TX_IWIDTH_VALUE_Q	0x2

#define QMI_DIRECT_RX			2

#define QMI_M1_TIMING			(0x20/4)
#define QMI_M1_TIMING_PAGEBREAK_LSB	28
#define QMI_M1_TIMING_SELECT_HOLD_LSB	23
#define QMI_M1_TIMING_COOLDOWN_LSB	30
#define QMI_M1_TIMING_RXDELAY_LSB	8
#define QMI_M1_TIMING_MAX_SELECT_LSB	17
#define QMI_M1_TIMING_MIN_DESELECT_LSB	12
#define QMI_M1_TIMING_CLKDIV_LSB	0

#define QMI_M1_TIMING_PAGEBREAK_VALUE_1024	0x2

#define QMI_M1_RFMT			(0x24/4)
#define QMI_M1_RFMT_PREFIX_WIDTH_LSB	0
#define QMI_M1_RFMT_ADDR_WIDTH_LSB	2
#define QMI_M1_RFMT_SUFFIX_WIDTH_LSB	4
#define QMI_M1_RFMT_DUMMY_WIDTH_LSB	6
#define QMI_M1_RFMT_DATA_WIDTH_LSB	8
#define QMI_M1_RFMT_PREFIX_LEN_LSB	12
#define QMI_M1_RFMT_SUFFIX_LEN_LSB	14
#define QMI_M1_RFMT_DUMMY_LEN_LSB	16

#define QMI_M1_RFMT_PREFIX_WIDTH_VALUE_Q	0x2
#define QMI_M1_RFMT_ADDR_WIDTH_VALUE_Q		0x2
#define QMI_M1_RFMT_SUFFIX_WIDTH_VALUE_Q	0x2
#define QMI_M1_RFMT_DUMMY_WIDTH_VALUE_Q		0x2
#define QMI_M1_RFMT_DATA_WIDTH_VALUE_Q		0x2
#define QMI_M1_RFMT_PREFIX_LEN_VALUE_8		0x1
#define QMI_M1_RFMT_SUFFIX_LEN_VALUE_NONE	0x0
#define QMI_M1_RFMT_DUMMY_LEN_VALUE_24		0x6

#define QMI_M1_RCMD			(0x28/4)
#define QMI_M1_RCMD_PREFIX_LSB		0
#define QMI_M1_RCMD_SUFFIX_LSB		8

#define QMI_M1_WFMT			(0x2C/4)
#define QMI_M1_WFMT_PREFIX_WIDTH_LSB	0
#define QMI_M1_WFMT_ADDR_WIDTH_LSB	2
#define QMI_M1_WFMT_SUFFIX_WIDTH_LSB	4
#define QMI_M1_WFMT_DUMMY_WIDTH_LSB	6
#define QMI_M1_WFMT_DATA_WIDTH_LSB	8
#define QMI_M1_WFMT_PREFIX_LEN_LSB	12
#define QMI_M1_WFMT_SUFFIX_LEN_LSB	14
#define QMI_M1_WFMT_DUMMY_LEN_LSB	16

#define QMI_M1_WFMT_PREFIX_WIDTH_VALUE_Q	0x2
#define QMI_M1_WFMT_ADDR_WIDTH_VALUE_Q		0x2
#define QMI_M1_WFMT_SUFFIX_WIDTH_VALUE_Q	0x2
#define QMI_M1_WFMT_DUMMY_WIDTH_VALUE_Q		0x2
#define QMI_M1_WFMT_DATA_WIDTH_VALUE_Q		0x2
#define QMI_M1_WFMT_PREFIX_LEN_VALUE_8		0x1
#define QMI_M1_WFMT_SUFFIX_LEN_VALUE_NONE	0x0
#define QMI_M1_WFMT_DUMMY_LEN_VALUE_NONE	0x0

#define QMI_M1_WCMD			(0x30/4)
#define QMI_M1_WCMD_PREFIX_LSB		0
#define QMI_M1_WCMD_SUFFIX_LSB		8

/* set interrupt enabled status */
static inline void arch_local_irq_write(unsigned long flags)
{
	barrier();
	if (arch_irqs_disabled_flags(flags))
		arch_local_irq_disable();
	else
		arch_local_irq_enable();
	barrier();
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
size_t setup_psram(void)
{
	size_t psram_size = 0;
	//uint32_t intr_stash  = arch_local_irq_save();

	// Try and read the PSRAM ID via direct_csr.
	XIP_QMI_BASE[QMI_DIRECT_CSR] = 30 << QMI_DIRECT_CSR_CLKDIV_LSB | QMI_DIRECT_CSR_EN_BITS;
	// Need to poll for the cooldown on the last XIP transfer to expire
	// (via direct-mode BUSY flag) before it is safe to perform the first
	// direct-mode operation
	while ((XIP_QMI_BASE[QMI_DIRECT_CSR] & QMI_DIRECT_CSR_BUSY_BITS) != 0);

	// Exit out of QMI in case we've inited already
	XIP_QMI_BASE[QMI_DIRECT_CSR] |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
	// Transmit as quad.
	XIP_QMI_BASE[QMI_DIRECT_TX] = QMI_DIRECT_TX_OE_BITS | QMI_DIRECT_TX_IWIDTH_VALUE_Q << QMI_DIRECT_TX_IWIDTH_LSB | 0xf5;
	while ((XIP_QMI_BASE[QMI_DIRECT_CSR] & QMI_DIRECT_CSR_BUSY_BITS) != 0);

	(void)XIP_QMI_BASE[QMI_DIRECT_RX];
	XIP_QMI_BASE[QMI_DIRECT_CSR] &= ~(QMI_DIRECT_CSR_ASSERT_CS1N_BITS);

	// Read the id
	XIP_QMI_BASE[QMI_DIRECT_CSR] |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
	uint8_t kgd = 0;
	uint8_t eid = 0;
	for (size_t i = 0; i < 7; i++) {
		if (i == 0)
			XIP_QMI_BASE[QMI_DIRECT_TX] = 0x9f;
		else
			XIP_QMI_BASE[QMI_DIRECT_TX] = 0xff;

		while ((XIP_QMI_BASE[QMI_DIRECT_CSR] & QMI_DIRECT_CSR_TXEMPTY_BITS) == 0);
		while ((XIP_QMI_BASE[QMI_DIRECT_CSR] & QMI_DIRECT_CSR_BUSY_BITS) != 0);
		if (i == 5)
			kgd = XIP_QMI_BASE[QMI_DIRECT_RX];
		else if (i == 6)
			eid = XIP_QMI_BASE[QMI_DIRECT_RX];
		else
			(void)XIP_QMI_BASE[QMI_DIRECT_RX];
	}

	// Disable direct csr.
	XIP_QMI_BASE[QMI_DIRECT_CSR] &= ~(QMI_DIRECT_CSR_ASSERT_CS1N_BITS | QMI_DIRECT_CSR_EN_BITS);

	if (kgd != 0x5D) {
		printf("Invalid PSRAM ID: %x\n", kgd);
		//arch_local_irq_write(intr_stash);
		return psram_size;
	}

	// Enable quad mode.
	XIP_QMI_BASE[QMI_DIRECT_CSR] = 30 << QMI_DIRECT_CSR_CLKDIV_LSB | QMI_DIRECT_CSR_EN_BITS;
	// Need to poll for the cooldown on the last XIP transfer to expire
	// (via direct-mode BUSY flag) before it is safe to perform the first
	// direct-mode operation
	while ((XIP_QMI_BASE[QMI_DIRECT_CSR] & QMI_DIRECT_CSR_BUSY_BITS) != 0);

	// RESETEN, RESET and quad enable
	for (uint8_t i = 0; i < 3; i++) {
		XIP_QMI_BASE[QMI_DIRECT_CSR] |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
		if (i == 0)
			XIP_QMI_BASE[QMI_DIRECT_TX] = 0x66;
		else if (i == 1)
			XIP_QMI_BASE[QMI_DIRECT_TX] = 0x99;
		else
			XIP_QMI_BASE[QMI_DIRECT_TX] = 0x35;

		while ((XIP_QMI_BASE[QMI_DIRECT_CSR] & QMI_DIRECT_CSR_BUSY_BITS) != 0);

		XIP_QMI_BASE[QMI_DIRECT_CSR] &= ~(QMI_DIRECT_CSR_ASSERT_CS1N_BITS);
		for (size_t j = 0; j < 20; j++)
			nop();

		(void)XIP_QMI_BASE[QMI_DIRECT_RX];
	}
	// Disable direct csr.
	XIP_QMI_BASE[QMI_DIRECT_CSR] &= ~(QMI_DIRECT_CSR_ASSERT_CS1N_BITS | QMI_DIRECT_CSR_EN_BITS);

	XIP_QMI_BASE[QMI_M1_TIMING] =
		QMI_M1_TIMING_PAGEBREAK_VALUE_1024 << QMI_M1_TIMING_PAGEBREAK_LSB | // Break between pages.
		3 << QMI_M1_TIMING_SELECT_HOLD_LSB | // Delay releasing CS for 3 extra system cycles.
		1 << QMI_M1_TIMING_COOLDOWN_LSB | 1 << QMI_M1_TIMING_RXDELAY_LSB |
		16 << QMI_M1_TIMING_MAX_SELECT_LSB |  // In units of 64 system clock cycles. PSRAM says 8us max. 8 / 0.00752 /64
											  // = 16.62
		7 << QMI_M1_TIMING_MIN_DESELECT_LSB | // In units of system clock cycles. PSRAM says 50ns.50 / 7.52 = 6.64
		2 << QMI_M1_TIMING_CLKDIV_LSB;
	XIP_QMI_BASE[QMI_M1_RFMT] = (QMI_M1_RFMT_PREFIX_WIDTH_VALUE_Q << QMI_M1_RFMT_PREFIX_WIDTH_LSB |
						 QMI_M1_RFMT_ADDR_WIDTH_VALUE_Q << QMI_M1_RFMT_ADDR_WIDTH_LSB |
						 QMI_M1_RFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M1_RFMT_SUFFIX_WIDTH_LSB |
						 QMI_M1_RFMT_DUMMY_WIDTH_VALUE_Q << QMI_M1_RFMT_DUMMY_WIDTH_LSB |
						 QMI_M1_RFMT_DUMMY_LEN_VALUE_24 << QMI_M1_RFMT_DUMMY_LEN_LSB |
						 QMI_M1_RFMT_DATA_WIDTH_VALUE_Q << QMI_M1_RFMT_DATA_WIDTH_LSB |
						 QMI_M1_RFMT_PREFIX_LEN_VALUE_8 << QMI_M1_RFMT_PREFIX_LEN_LSB |
						 QMI_M1_RFMT_SUFFIX_LEN_VALUE_NONE << QMI_M1_RFMT_SUFFIX_LEN_LSB);
	XIP_QMI_BASE[QMI_M1_RCMD] = 0xeb << QMI_M1_RCMD_PREFIX_LSB | 0 << QMI_M1_RCMD_SUFFIX_LSB;
	XIP_QMI_BASE[QMI_M1_WFMT] = (QMI_M1_WFMT_PREFIX_WIDTH_VALUE_Q << QMI_M1_WFMT_PREFIX_WIDTH_LSB |
						 QMI_M1_WFMT_ADDR_WIDTH_VALUE_Q << QMI_M1_WFMT_ADDR_WIDTH_LSB |
						 QMI_M1_WFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M1_WFMT_SUFFIX_WIDTH_LSB |
						 QMI_M1_WFMT_DUMMY_WIDTH_VALUE_Q << QMI_M1_WFMT_DUMMY_WIDTH_LSB |
						 QMI_M1_WFMT_DUMMY_LEN_VALUE_NONE << QMI_M1_WFMT_DUMMY_LEN_LSB |
						 QMI_M1_WFMT_DATA_WIDTH_VALUE_Q << QMI_M1_WFMT_DATA_WIDTH_LSB |
						 QMI_M1_WFMT_PREFIX_LEN_VALUE_8 << QMI_M1_WFMT_PREFIX_LEN_LSB |
						 QMI_M1_WFMT_SUFFIX_LEN_VALUE_NONE << QMI_M1_WFMT_SUFFIX_LEN_LSB);
	XIP_QMI_BASE[QMI_M1_WCMD] = 0x38 << QMI_M1_WCMD_PREFIX_LSB | 0 << QMI_M1_WCMD_SUFFIX_LSB;

	psram_size = 1024 * 1024; // 1 MiB
	uint8_t size_id = eid >> 5;
	if (eid == 0x26 || size_id == 2)
		psram_size *= 8;
	else if (size_id == 0)
		psram_size *= 2;
	else if (size_id == 1)
		psram_size *= 4;

	// Mark that we can write to PSRAM.
	XIP_CTRL_BASE[XIP_CTRL_WRITABLE_M1_BITS] |= XIP_CTRL_WRITABLE_M1_BITS;
	//arch_local_irq_write(intr_stash);
	printf("PSRAM ID: %x %x\n", kgd, eid);

	return psram_size;
}
