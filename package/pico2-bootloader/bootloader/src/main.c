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
#ifndef PICO_SDK
static int launch_core1(void);
#endif

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

	if (kernel_size) {
#ifndef PICO_SDK
		/* Bring up core1 as a bare-metal coprocessor before entering Linux. */
		if (launch_core1() == 0)
			puts("[boot] core1 launched as bare-metal coprocessor");
		else
			puts("[boot] core1 launch skipped (no response)");
#endif
		jump_to_kernel(ram_addr, data_addr);
	}

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


#ifndef PICO_SDK
/*
 * RP2350 second core (Hazard3 core1) as a bare-metal coprocessor.
 *
 * Linux is UP (CONFIG_SMP=n) and uses core0 only. The bootloader launches
 * core1 via the bootrom wait_for_vector protocol (RP2350 datasheet 5.3) and
 * then hands core0 to Linux. core1_main is linked into the bootloader .text,
 * which runs from SRAM, so core1 keeps executing after core0 enters Linux
 * (Linux lives in PSRAM and leaves this SRAM region alone). core1 stack and
 * counter sit in the upper SRAM (>256 KiB), unused by bootloader or Linux.
 */

#define C1_UART0_DR   0x40070000u
#define C1_UART0_FR   0x40070018u
#define C1_UART_TXFF  (1u << 5)
#define C1_STACK_TOP  0x20070000u
#define C1_CTR_ADDR   0x20061000u   /* heartbeat counter */

/* h3.unblock = slt x0,x0,x1: Hazard3 equivalent of __sev (RP2350 DS 3.8.6.3.2) */
static inline void h3_unblock(void) { __asm__ volatile ("slt x0, x0, x1"); }

static inline void core1_putc(char c) {
	volatile uint32_t *fr = (volatile uint32_t *)C1_UART0_FR;
	volatile uint32_t *dr = (volatile uint32_t *)C1_UART0_DR;
	while (*fr & C1_UART_TXFF)
		;
	*dr = (uint32_t)(uint8_t)c;
}

static void core1_puts(const char *s) {
	while (*s)
		core1_putc(*s++);
}

/*
 * PIO offload: load and run a PIO program passed from Linux as a '.piojob'
 * blob in SRAM (C1_PIO_JOB). core1 brings up the PIO block and starts the
 * state machine; the PIO then runs autonomously in hardware.
 */
#define C1_PIO_JOB   0x20061100u
#define RESETS_ADDR  0x40020000u
#define IOBANK0_ADDR 0x40028000u
#define PADS0_ADDR   0x40038000u

static volatile uint32_t g_ws2812;   /* nonzero: core1 feeds the PIO FIFO (WS2812) */

static uint32_t pio_load_and_run(void)
{
	volatile uint8_t  *j8  = (volatile uint8_t  *)C1_PIO_JOB;
	volatile uint32_t *j32 = (volatile uint32_t *)C1_PIO_JOB;
	volatile uint16_t *ins = (volatile uint16_t *)(C1_PIO_JOB + 28);

	if (j32[0] != 0x424F4950u)                 /* magic 'PIOB' */
		return 0xBADF11E0u;

	uint32_t pio_idx = j8[4], sm = j8[5], n = j8[6], flags = j8[7];
	uint32_t clkdiv  = j32[2];
	uint32_t set_base = j8[12], set_count = j8[13];
	uint32_t side_base = j8[16], side_count = j8[17];
	uint32_t wrap_bottom = j8[19], wrap_top = j8[20];
	uint32_t gpio_mask = j32[6];
	uint32_t ws2812 = flags & 1u;              /* WS2812 mode: sideset+autopull+FIFO */
	uint32_t i, p;

	if (pio_idx > 2 || sm > 3 || n == 0 || n > 32)
		return 0xBADF11E1u;

	volatile uint32_t *PIO = (volatile uint32_t *)(0x50200000u + pio_idx * 0x100000u);
	uint32_t funcsel = 6u + pio_idx;           /* PIO0=6, PIO1=7, PIO2=8 */
	uint32_t rbit    = 1u << (11 + pio_idx);   /* RESETS bit: PIO0=11.. */
	uint32_t smoff   = 0xc8u + sm * 0x18u;      /* SMx registers from SM0_CLKDIV */

	/* 1) release PIO from reset, wait RESET_DONE */
	{
		uint32_t g = 2000000u;
		*(volatile uint32_t *)(RESETS_ADDR + 0x3000u) = rbit;
		while (!(*(volatile uint32_t *)(RESETS_ADDR + 0x8u) & rbit) && --g)
			;
		if (!g) return 0xBADF11E2u;
	}

	/* 2) pinmux: route each GPIO in the mask to the PIO function, enable pad */
	for (p = 0; p < 32; p++) {
		if (gpio_mask & (1u << p)) {
			*(volatile uint32_t *)(PADS0_ADDR + 4u + p * 4u) = 0u;
			*(volatile uint32_t *)(IOBANK0_ADDR + p * 8u + 4u) = funcsel;
		}
	}

	/* 3) load opcodes into INSTR_MEM */
	for (i = 0; i < n; i++)
		PIO[(0x048u + i * 4u) / 4] = ins[i];

	/* 4) configure the state machine */
	PIO[(smoff + 0x00u) / 4] = clkdiv;                                  /* CLKDIV */
	PIO[(smoff + 0x04u) / 4] = (wrap_top << 12) | (wrap_bottom << 7);   /* EXECCTRL wrap */
	/* SHIFTCTRL: WS2812 -> autopull, PULL_THRESH=24, OUT shift left (MSB first) */
	PIO[(smoff + 0x08u) / 4] = ws2812 ? ((24u << 25) | (1u << 17)) : 0u;
	if (ws2812) {
		/* drive the sideset pin: point SET group at side_base, run `set pindirs,1` */
		PIO[(smoff + 0x14u) / 4] = (1u << 26) | (side_base << 5);
		PIO[(smoff + 0x10u) / 4] = 0xE081u;
	}
	PIO[(smoff + 0x14u) / 4] = (side_count << 29) | (set_count << 26)
				 | (side_base << 10) | (set_base << 5);    /* PINCTRL */
	PIO[(smoff + 0x10u) / 4] = 0x0000u;                                 /* SMx_INSTR: jmp 0 */

	/* 5) enable the SM */
	PIO[0x000u / 4] |= (1u << sm);

	g_ws2812 = ws2812;

	/* status: 0x501 | pio_idx | current PC */
	return 0x50100000u | (pio_idx << 8) | (PIO[(smoff + 0x0cu) / 4] & 0x1fu);
}

static uint32_t pio0_sm0_pc(void)
{
	volatile uint32_t *PIO = (volatile uint32_t *)0x50200000u;
	return PIO[(0xc8u + 0x0cu) / 4] & 0x1fu;   /* SM0_ADDR */
}

/* Handle one command (op<<24 | arg) and return the reply word. */
static uint32_t core1_handle(uint32_t cmd)
{
	uint32_t op  = cmd >> 24;
	uint32_t arg = cmd & 0x00ffffffu;

	switch (op) {
	case 0x01: return 0xC0DE0001u;                       /* PING */
	case 0x02: return *(volatile uint32_t *)C1_CTR_ADDR; /* GET_COUNT */
	case 0x03: return arg + 1u;                          /* ADD1 */
	case 0x04:                                           /* LED on/off */
		if (arg & 1u) SIO_BASE[0x18 / 4] = (1u << LED_PIN);
		else          SIO_BASE[0x20 / 4] = (1u << LED_PIN);
		return 0xC0DE0004u;
	case 0x05: return 0x52503235u;                       /* 'RP25' */
	case 0x06: return pio_load_and_run();                /* LOAD_PIO */
	case 0x07: return pio0_sm0_pc();                     /* PIO0 SM0 PC */
	default:   return 0xBAD00000u | op;
	}
}

/*
 * core1 entry point (never returns). Serves commands from the SRAM mailbox
 * (REQ/ACK) and, as an alternative, the inter-core SIO FIFO; keeps a
 * heartbeat counter running and optionally feeds the WS2812 PIO FIFO.
 */
#define C1_MB_REQ_ADDR  0x20061010u
#define C1_MB_ACK_ADDR  0x20061014u
static void core1_main(void) {
	volatile uint32_t *ctr     = (volatile uint32_t *)C1_CTR_ADDR;
	volatile uint32_t *req     = (volatile uint32_t *)C1_MB_REQ_ADDR;
	volatile uint32_t *ack     = (volatile uint32_t *)C1_MB_ACK_ADDR;
	volatile uint32_t *fifo_st = &SIO_BASE[0x50 / 4];
	volatile uint32_t *fifo_wr = &SIO_BASE[0x54 / 4];
	volatile uint32_t *fifo_rd = &SIO_BASE[0x58 / 4];
	uint32_t spin = 0;

	*ctr = 0;
	*req = 0;
	core1_puts("\r\n[core1] hart=1 up: mailbox(SRAM)+FIFO command server ready\r\n");

	for (;;) {
		/* channel 1: SRAM mailbox */
		uint32_t r = *req;
		if (r) {
			*ack = core1_handle(r);
			__asm__ volatile ("fence" ::: "memory");
			*req = 0;
			__asm__ volatile ("fence" ::: "memory");
			/* ring core0 doorbell: DOORBELL_OUT_SET bit0 -> SIO_IRQ_BELL (26) */
			*(volatile uint32_t *)0xd0000180u = 1u;
		}

		/* channel 2: SIO FIFO */
		if (*fifo_st & 1u) {
			uint32_t resp = core1_handle(*fifo_rd);
			while (!(*fifo_st & 2u))
				;
			*fifo_wr = resp;
			__asm__ volatile ("fence" ::: "memory");
		}

		(*ctr)++;                            /* heartbeat */

		/* WS2812: periodically push a GRB word (color<<8) to PIO0 SM0 TX FIFO */
		if (g_ws2812 && ((++spin & 0x007fffffu) == 0)) {
			static uint32_t on = 1;
			volatile uint32_t *txf0 = (volatile uint32_t *)(0x50200000u + 0x010u);
			*txf0 = on ? 0xFF000000u : 0x00000000u;
			on ^= 1u;
		}
	}
}

/* PSM (Power State Machine): per-core reset. RP2350 DS 7.4. */
#define PSM_FRCE_OFF_ADDR  0x40018004u
#define PSM_DONE_ADDR      0x4001800cu
#define PSM_PROC1_BIT      (1u << 24)

/* Power-cycle core1 so it re-runs the bootrom and waits in wait_for_vector. */
static void reset_core1(void) {
	volatile uint32_t *frce_off = (volatile uint32_t *)PSM_FRCE_OFF_ADDR;
	volatile uint32_t *done     = (volatile uint32_t *)PSM_DONE_ADDR;
	uint32_t g;

	*frce_off |= PSM_PROC1_BIT;
	g = 2000000u; while (!(*frce_off & PSM_PROC1_BIT) && --g) ;
	*frce_off &= ~PSM_PROC1_BIT;
	g = 2000000u; while (!(*done & PSM_PROC1_BIT) && --g) ;
}

/* Launch core1 via the SIO FIFO handshake. Returns 0 on success, -1 on
 * timeout (core0 then just boots Linux, so the board is never bricked). */
static int launch_core1(void) {
	volatile uint32_t *fifo_st = &SIO_BASE[0x50 / 4];
	volatile uint32_t *fifo_wr = &SIO_BASE[0x54 / 4];
	volatile uint32_t *fifo_rd = &SIO_BASE[0x58 / 4];

	const uint32_t cmd_seq[6] = {
		0, 0, 1,
		(uint32_t)&core1_main,   /* vector_table (mtvec): must be nonzero (0 = resync) */
		C1_STACK_TOP,            /* sp */
		(uint32_t)&core1_main    /* entry */
	};

	uint32_t seq = 0;
	int tries = 0;

	printf("[c1] entry=0x%08x ST0=0x%08x\n", (uint32_t)&core1_main, *fifo_st);
	reset_core1();
	printf("[c1] after reset ST=0x%08x\n", *fifo_st);

	while (seq < 6) {
		uint32_t cmd = cmd_seq[seq];
		uint32_t guard, resp;
		int to;

		if (++tries > 40) {
			printf("[c1] give up tries=%d seq=%d\n", tries, seq);
			return -1;
		}

		if (cmd == 0) {
			while (*fifo_st & 1u) /* drain RX FIFO */
				(void)*fifo_rd;
			h3_unblock();
		}

		/* push, blocking on TX-ready, with timeout */
		guard = 3000000u; to = 0;
		while (!(*fifo_st & 2u))
			if (!--guard) { to = 1; break; }
		if (to) { printf("[c1] TX timeout seq=%d\n", seq); return -1; }
		*fifo_wr = cmd;
		__asm__ volatile ("fence" ::: "memory");
		h3_unblock();

		/* pop, blocking on RX-valid, with timeout */
		guard = 3000000u; to = 0;
		while (!(*fifo_st & 1u))
			if (!--guard) { to = 1; break; }
		if (to) { printf("[c1] RX timeout seq=%d push=0x%08x\n", seq, cmd); return -1; }
		resp = *fifo_rd;

		printf("[c1] seq=%d push=0x%08x resp=0x%08x %s\n",
		       seq, cmd, resp, (cmd == resp) ? "OK" : "MISMATCH");
		seq = (cmd == resp) ? seq + 1 : 0;   /* bootrom echoes each word */
	}
	printf("[c1] handshake complete\n");
	return 0;
}
#endif /* !PICO_SDK */
