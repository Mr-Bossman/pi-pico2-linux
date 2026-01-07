RAM_SIZE = (256 * 1024);
FLASH_SIZE = (8 * 1024 * 1024);

OUTPUT_FORMAT("elf32-littleriscv", "elf32-littleriscv", "elf32-littleriscv")
OUTPUT_ARCH(riscv)
ENTRY(_start)

MEMORY
{
	FLASH (RX) : ORIGIN = 0x10000000, LENGTH = FLASH_SIZE
	RAM (RWX) : ORIGIN = 0x20000000, LENGTH = RAM_SIZE
}

SECTIONS
{
	.start : {
		*(.start*)
		. = ALIGN(4);
	} > FLASH

	.image_header : {
		KEEP(*(.image_header))
	} > FLASH

	.reloc : {
		*(.reloc*)
	} > FLASH

	.text : {
		. = ALIGN(4);
		PROVIDE_HIDDEN(__text_start = .);
		*(.text*)
		. = ALIGN(4);
		PROVIDE_HIDDEN(__text_end = .);
	} > RAM AT > FLASH
	PROVIDE_HIDDEN(__text_load_start = LOADADDR(.text));

	.init_array (READONLY) : {
		. = ALIGN(4);
		PROVIDE_HIDDEN(__ctors_start = .);
		KEEP(*(SORT(.init_array.*)))
		KEEP(*(.init_array))
		PROVIDE_HIDDEN(__ctors_end = .);
	} > RAM AT > FLASH

	.fini_array (READONLY) : {
		. = ALIGN(4);
		PROVIDE_HIDDEN(__dtors_start = .);
		KEEP(*(SORT(.fini_array.*)))
		KEEP(*(.fini_array))
		PROVIDE_HIDDEN(__dtors_end = .);
	} > RAM AT > FLASH

	.data : {
		. = ALIGN(4);
		PROVIDE_HIDDEN(__data_start = .);
		*(.rodata*)
		*(.data*)
		. = ALIGN(4);
		PROVIDE_HIDDEN(__data_end = .);
		/* Kernel doesn't load on bare metal without
		 * alignment for some reason
		 */
		. = ALIGN(16);
	} > RAM AT > FLASH
	PROVIDE_HIDDEN(__data_load_start = LOADADDR(.data));
	PROVIDE_HIDDEN(__payload_load_start = LOADADDR(.data) + SIZEOF(.data));

	.bss : {
		. = ALIGN(4);
		PROVIDE_HIDDEN(__bss_start = .);
		*(.bss*)
		. = ALIGN(4);
		PROVIDE_HIDDEN(__bss_end = .);
	} > RAM

	PROVIDE_HIDDEN(__ram_end = ORIGIN(RAM) + LENGTH(RAM));
}
