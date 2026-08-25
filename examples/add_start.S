.section .text
.global _start

_start:
	la sp, __stack_top

	addi a0, x0, 5
	addi a1, x0, 7

	call add

	ebreak
