.section .text
.global _start

.equ UART_TX,					0x10000000
.equ TIMER_BASE,				0x02000000
.equ MTIME_LOW,					0x00
.equ MTIME_HIGH,				0x04
.equ MTIMECMP_LOW,				0x08
.equ MTIMECMP_HIGH,				0x0C
.equ MSTATUS_MIE,				0x08
.equ MIE_MTIE,					0x80
.equ MACHINE_TIMER_INTERRUPT, 	0x80000007
.equ TIMER_INTERVAL,			128

_start:
	la sp, __stack_top

	la t0, trap_handler
	csrw mtvec, t0
	
	call program_next_timer

	li t0, MIE_MTIE
	csrs mie, t0

	li t0, MSTATUS_MIE
	csrs mstatus, t0

	li s0, 0

foreground_loop:
	addi s0, s0, 1

	la t0, interrupts_done
	lw t1, 0(t0)
	beqz t1, foreground_loop

	la t0, message
	li t2, UART_TX

print_loop:
	lbu t1, 0(t0)
	beqz t1, print_loop_done

	sb t1, 0(t2)
	addi t0, t0, 1
	j print_loop

print_loop_done:
	ebreak

program_next_timer:
	li t0, TIMER_BASE

read_mtime:
	lw t1, MTIME_HIGH(t0)
	lw t2, MTIME_LOW(t0)
	lw t3, MTIME_HIGH(t0)
	bne t1, t3, read_mtime

	li t4, TIMER_INTERVAL
	add t5, t2, t4
	sltu t6, t5, t2
    add t1, t1, t6

    li t6, -1
    sw t6, MTIMECMP_LOW(t0)
    sw t1, MTIMECMP_HIGH(t0)
    sw t5, MTIMECMP_LOW(t0)

	ret

trap_handler:
	addi sp, sp, -32
    sw ra, 28(sp)
    sw t0, 24(sp)
    sw t1, 20(sp)
    sw t2, 16(sp)
    sw t3, 12(sp)
    sw t4, 8(sp)
    sw t5, 4(sp)
    sw t6, 0(sp)

	csrr t0, mcause
    li t1, MACHINE_TIMER_INTERRUPT
    bne t0, t1, unexpected_trap

    la t0, interrupt_count
    lw t1, 0(t0)
    addi t1, t1, 1
    sw t1, 0(t0)

    li t2, 3
    beq t1, t2, third_interrupt

    call program_next_timer
    j restore

third_interrupt:
	li t0, MIE_MTIE
    csrc mie, t0

    la t0, interrupts_done
    li t1, 1
    sw t1, 0(t0)

restore:
	lw t6, 0(sp)
    lw t5, 4(sp)
    lw t4, 8(sp)
    lw t3, 12(sp)
    lw t2, 16(sp)
    lw t1, 20(sp)
    lw t0, 24(sp)
    lw ra, 28(sp)
    addi sp, sp, 32

	mret

unexpected_trap:
    lw t6, 0(sp)
    lw t5, 4(sp)
    lw t4, 8(sp)
    lw t3, 12(sp)
    lw t2, 16(sp)
    lw t1, 20(sp)
    lw t0, 24(sp)
    lw ra, 28(sp)
    addi sp, sp, 32

    ebreak

.section .rodata

message:
    .asciz "timer x3\n"

.section .bss
.balign 4

interrupt_count:
    .zero 4

interrupts_done:
    .zero 4
