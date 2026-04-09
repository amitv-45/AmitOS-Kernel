/* Multiboot Header Constants */
.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

/* Multiboot Section - 'a' flag ensures it is allocated in the binary */
.section .multiboot, "a"
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

/* Stack Setup */
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

/* Kernel Start */
.section .text
.global _start
.type _start, @function
_start:
	mov $stack_top, %esp
	call kernel_main

	/* Hang if the kernel returns */
	cli
1:	hlt
	jmp 1b

/* Silence the executable stack warning */
.section .note.GNU-stack,"",@progbits