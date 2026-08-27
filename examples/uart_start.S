.section .text
.global _start

_start:
	la sp, __stack_top
	call uart_main
	ebreak
