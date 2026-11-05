.section .boot,"ax"
.code16
.globl _start
_start:
    cli
    ljmp $0x0000, $disk_read    # Normalize CS to 0000
disk_read:
	cli
	xorw	%ax, %ax		# setting ax to 0
	movw	%ax, %ds		# setting ds
	movw	%ax, %es		# setting es
	movw	%ax, %ss		# setting ss
	movw	$0x7C00, %sp	# setting sp
	movb	%dl, boot_drive	# setting dl
	movw	$dap, %si		# DS:SI points to the packet
	movb	boot_drive, %dl
	movb	$0x42, %ah		# Extended read
	int 	$0x13			# interrupt
	jc  	_start			# Carry set means the BIOS reported an error
	jmp		change_modes
change_modes:
	lgdt	gdt_descriptor
    movl	%cr0, %eax
    orl		$1, %eax
    movl	%eax, %cr0
    ljmp	$0x0008, $pm_entry

.code32
pm_entry:
    movw	$0x0010, %ax
    movw	%ax, %ds
    movw	%ax, %es
    movw	%ax, %ss
    movw	%ax, %fs
    movw	%ax, %gs

    movl	$0x00090000, %esp
    movl	$0x00008000, %eax
    jmp		*%eax

.align 4
boot_drive:
	.byte	0
.align 4
dap:
	.byte	0x10, 0			# Packet size, reserved
	.word	0x20			# Number of sectors to read
	.word	0x8000, 0		# Buffer offset, then buffer segment: 0000:8000
	.quad	2				# Starting LBA (sector 2)
.align 8
gdt:
    .quad	0
    .quad	0x00CF9A000000FFFF       # 32-bit code segment
    .quad	0x00CF92000000FFFF       # 32-bit data segment
gdt_end:

gdt_descriptor:
    .word	gdt_end - gdt - 1
    .long	gdt

#end of file
.org	510, 0
.word	0xAA55
