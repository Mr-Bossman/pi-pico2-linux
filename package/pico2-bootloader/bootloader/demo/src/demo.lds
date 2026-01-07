__stack_size = 0x200;
ENTRY(_start)

SECTIONS
{
	. = 0x11000000;
	.text : ALIGN(16) {
		*(.asm)
		*(.entry.text)
		*(.init.literal)
		*(.init)
		*(.text)
	}

	.data : ALIGN(16) {
		__DATA_BEGIN__ = .;
		*(.rodata)
		*(.rodata.*)
		*(.gnu.linkonce.r.*)
		*(.rodata1)
		*(.dynsbss)
		*(.gnu.linkonce.sb.*)
		*(.scommon)
		*(.gnu.linkonce.sb2.*)
		*(.sbss)
		*(.sbss.*)
		*(.sbss2)
		*(.sbss2.*)
		*(.dynbss)
		*(.data)
		*(.data.*)
		*(.got)
		*(.got.*)
		__DATA_END__ = .;
	}

	.bss : ALIGN( 16 ) {
		__BSS_BEGIN__ = .;
		*(.bss)
		*(.bss.*)
		__BSS_END__ = .;
	}

	.stack : ALIGN( 16 ) {
		_estack = .;
		. = . + __stack_size;
		_sstack = .;
	}
	/DISCARD/ : { *(.riscv.attributes) }
}
