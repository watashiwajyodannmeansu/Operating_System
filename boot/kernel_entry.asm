.section .text.start,"ax"
.code32
.globl _start
.extern main
.extern __bss_start
.extern __bss_end

_start:
    cli
    cld
    movl	$0x00090000, %esp

    movl	$__bss_start, %edi
    movl	$__bss_end, %ecx
    subl	%edi, %ecx
    xorl	%eax, %eax
    rep		stosb

    xorl	%ebp, %ebp
    call	main

1:
    hlt
    jmp		1b
