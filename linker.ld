ENTRY(_start)

SECTIONS
{
	/* Start placing sections at 1 MiB */
	. = 1M;

	.text BLOCK(4K) : ALIGN(4K)
	{
		/* This MUST be first for the bootloader to find it */
		KEEP(*(.multiboot))
		*(.text)
	}

	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
	}

	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}

	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
	}
}