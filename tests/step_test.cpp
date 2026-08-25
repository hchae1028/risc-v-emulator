#include "cpu.hpp"
#include "decoder.hpp"
#include "executor.hpp"
#include "memory.hpp"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

int main() {
	Cpu cpu{};
	Memory memory{ 64 };

	memory.write32(0, 0x00500093);	// ADDI x1, x0, 5
	memory.write32(4, 0x00700113);	// ADDI x2, x0, 7
	memory.write32(8, 0x002081B3);	// ADD x3, x1, x2
	
	cpu.step(memory);
	assert(cpu.read_pc() == 4);
	assert(cpu.read_register(1) == 5);
	assert(cpu.read_register(2) == 0);
	assert(cpu.read_register(3) == 0);

	cpu.step(memory);
	assert(cpu.read_pc() == 8);
	assert(cpu.read_register(1) == 5);
	assert(cpu.read_register(2) == 7);
	assert(cpu.read_register(3) == 0);

	cpu.step(memory);
	assert(cpu.read_pc() == 12);
	assert(cpu.read_register(3) == 12);

	// Program memory remains unchanged
	assert(memory.read32(0) == 0x00500093);
	assert(memory.read32(4) == 0x00700113);
	assert(memory.read32(8) == 0x002081B3);

	std::array<std::uint32_t, 32> registers_before_error{};
	for (std::size_t i = 0; i < registers_before_error.size(); i++) {
		registers_before_error[i] = cpu.read_register(i);
	}

	// Unknown instruction traps and leaves PC and registers unchanged
	bool illegal_trap_thrown{ false };
	TrapCause illegal_trap_cause{ TrapCause::BreakPoint };
	try {
		cpu.step(memory);
	} catch (const Trap& trap) {
		illegal_trap_thrown = true;
		illegal_trap_cause = trap.cause;
	}
	assert(illegal_trap_thrown);
	assert(illegal_trap_cause == TrapCause::IllegalInstruction);
	assert(cpu.read_pc() == 12);
	for (std::size_t i = 0; i < registers_before_error.size(); i++) {
		assert(cpu.read_register(i) == registers_before_error[i]);
	}

	// Aligned out-of-range PC leaves state unchanged
	cpu.set_pc(64);
	bool exception_thrown{ false };
	try {
		cpu.step(memory);
	} catch (const Trap& trap) {
		exception_thrown = true;
		assert(trap.cause == TrapCause::InstructionAccessFault);
	}
	assert(exception_thrown);
	assert(cpu.read_pc() == 64);
	for (std::size_t i = 0; i < registers_before_error.size(); i++) {
		assert(cpu.read_register(i) == registers_before_error[i]);
	}

	/* Register-register bitwise instructions through the full pipeline */
	Cpu bitwise_cpu{};
	Memory bitwise_memory{ 12 };

	bitwise_cpu.write_register(1, 0xCC33CC33u);
	bitwise_cpu.write_register(2, 0x0F0FF0F0u);
	bitwise_memory.write32(0, 0x0020C1B3); // XOR x3, x1, x2
	bitwise_memory.write32(4, 0x0020E233); // OR x4, x1, x2
	bitwise_memory.write32(8, 0x0020F2B3); // AND x5, x1, x2

	bitwise_cpu.step(bitwise_memory);
	assert(bitwise_cpu.read_pc() == 4);
	assert(bitwise_cpu.read_register(3) == 0xC33C3CC3u);

	bitwise_cpu.step(bitwise_memory);
	assert(bitwise_cpu.read_pc() == 8);
	assert(bitwise_cpu.read_register(4) == 0xCF3FFCF3u);

	bitwise_cpu.step(bitwise_memory);
	assert(bitwise_cpu.read_pc() == 12);
	assert(bitwise_cpu.read_register(5) == 0x0C03C030u);
	assert(bitwise_cpu.read_register(1) == 0xCC33CC33u);
	assert(bitwise_cpu.read_register(2) == 0x0F0FF0F0u);

	/* Immediate bitwise instructions through the full pipeline */
	Cpu immediate_bitwise_cpu{};
	Memory immediate_bitwise_memory{ 12 };

	immediate_bitwise_cpu.write_register(1, 0xCC33CC33u);
	immediate_bitwise_memory.write32(0, 0xFFF0C193); // XORI x3, x1, -1
	immediate_bitwise_memory.write32(4, 0x0F00E213); // ORI x4, x1, 0xF0
	immediate_bitwise_memory.write32(8, 0xF000F293); // ANDI x5, x1, -256

	immediate_bitwise_cpu.step(immediate_bitwise_memory);
	assert(immediate_bitwise_cpu.read_pc() == 4);
	assert(immediate_bitwise_cpu.read_register(3) == 0x33CC33CCu);

	immediate_bitwise_cpu.step(immediate_bitwise_memory);
	assert(immediate_bitwise_cpu.read_pc() == 8);
	assert(immediate_bitwise_cpu.read_register(4) == 0xCC33CCF3u);

	immediate_bitwise_cpu.step(immediate_bitwise_memory);
	assert(immediate_bitwise_cpu.read_pc() == 12);
	assert(immediate_bitwise_cpu.read_register(5) == 0xCC33CC00u);
	assert(immediate_bitwise_cpu.read_register(1) == 0xCC33CC33u);

	/* Register-register shifts through the full pipeline */
	Cpu shift_cpu{};
	Memory shift_memory{ 12 };

	shift_cpu.write_register(1, 0x80000001u);
	shift_cpu.write_register(2, 4);
	shift_memory.write32(0, 0x002091B3); // SLL x3, x1, x2
	shift_memory.write32(4, 0x0020D233); // SRL x4, x1, x2
	shift_memory.write32(8, 0x4020D2B3); // SRA x5, x1, x2

	shift_cpu.step(shift_memory);
	assert(shift_cpu.read_pc() == 4);
	assert(shift_cpu.read_register(3) == 0x00000010u);

	shift_cpu.step(shift_memory);
	assert(shift_cpu.read_pc() == 8);
	assert(shift_cpu.read_register(4) == 0x08000000u);

	shift_cpu.step(shift_memory);
	assert(shift_cpu.read_pc() == 12);
	assert(shift_cpu.read_register(5) == 0xF8000000u);
	assert(shift_cpu.read_register(1) == 0x80000001u);
	assert(shift_cpu.read_register(2) == 4);

	/* Immediate shifts through the full pipeline */
	Cpu immediate_shift_cpu{};
	Memory immediate_shift_memory{ 12 };

	immediate_shift_cpu.write_register(1, 0x80000001u);
	immediate_shift_memory.write32(0, 0x00409193); // SLLI x3, x1, 4
	immediate_shift_memory.write32(4, 0x0040D213); // SRLI x4, x1, 4
	immediate_shift_memory.write32(8, 0x4040D293); // SRAI x5, x1, 4

	immediate_shift_cpu.step(immediate_shift_memory);
	assert(immediate_shift_cpu.read_pc() == 4);
	assert(immediate_shift_cpu.read_register(3) == 0x00000010u);

	immediate_shift_cpu.step(immediate_shift_memory);
	assert(immediate_shift_cpu.read_pc() == 8);
	assert(immediate_shift_cpu.read_register(4) == 0x08000000u);

	immediate_shift_cpu.step(immediate_shift_memory);
	assert(immediate_shift_cpu.read_pc() == 12);
	assert(immediate_shift_cpu.read_register(5) == 0xF8000000u);
	assert(immediate_shift_cpu.read_register(1) == 0x80000001u);

	/* Register-register comparisons through the full pipeline */
	Cpu comparison_cpu{};
	Memory comparison_memory{ 8 };

	comparison_cpu.write_register(1, 0xFFFFFFFFu);
	comparison_cpu.write_register(2, 1);
	comparison_memory.write32(0, 0x0020A1B3); // SLT x3, x1, x2
	comparison_memory.write32(4, 0x0020B233); // SLTU x4, x1, x2

	comparison_cpu.step(comparison_memory);
	assert(comparison_cpu.read_pc() == 4);
	assert(comparison_cpu.read_register(3) == 1);

	comparison_cpu.step(comparison_memory);
	assert(comparison_cpu.read_pc() == 8);
	assert(comparison_cpu.read_register(4) == 0);
	assert(comparison_cpu.read_register(1) == 0xFFFFFFFFu);
	assert(comparison_cpu.read_register(2) == 1);

	/* Immediate comparisons through the full pipeline */
	Cpu immediate_comparison_cpu{};
	Memory immediate_comparison_memory{ 8 };

	immediate_comparison_cpu.write_register(1, 0);
	immediate_comparison_memory.write32(0, 0xFFF0A193); // SLTI x3, x1, -1
	immediate_comparison_memory.write32(4, 0xFFF0B213); // SLTIU x4, x1, -1

	immediate_comparison_cpu.step(immediate_comparison_memory);
	assert(immediate_comparison_cpu.read_pc() == 4);
	assert(immediate_comparison_cpu.read_register(3) == 0);

	immediate_comparison_cpu.step(immediate_comparison_memory);
	assert(immediate_comparison_cpu.read_pc() == 8);
	assert(immediate_comparison_cpu.read_register(4) == 1);
	assert(immediate_comparison_cpu.read_register(1) == 0);

	/* LW through the full pipeline */
	Cpu load_cpu{};
	Memory load_memory{ 64 };

	load_cpu.write_register(1, 24);
	load_memory.write32(0, 0x0080A183); // LW x3, 8(x1)
	load_memory.write32(4, 0xFFC0A203); // LW x4, -4(x1)
	load_memory.write32(20, 0x89ABCDEFu);
	load_memory.write32(32, 0x12345678u);

	load_cpu.step(load_memory);
	assert(load_cpu.read_pc() == 4);
	assert(load_cpu.read_register(3) == 0x12345678u);
	assert(load_cpu.read_register(4) == 0);

	load_cpu.step(load_memory);
	assert(load_cpu.read_pc() == 8);
	assert(load_cpu.read_register(4) == 0x89ABCDEFu);
	assert(load_cpu.read_register(1) == 24);
	assert(load_memory.read32(0) == 0x0080A183u);
	assert(load_memory.read32(4) == 0xFFC0A203u);
	assert(load_memory.read32(20) == 0x89ABCDEFu);
	assert(load_memory.read32(32) == 0x12345678u);

	// A failed load through step leaves PC, registers, and memory unchanged
	Cpu failed_load_cpu{};
	Memory failed_load_memory{ 16 };
	failed_load_memory.write32(0, 0x00202183); // LW x3, 2(x0)
	failed_load_cpu.write_register(3, 0xCAFEBABEu);

	bool load_exception_thrown{ false };
	try {
		failed_load_cpu.step(failed_load_memory);
	} catch (const Trap& trap) {
		load_exception_thrown = true;
		assert(trap.cause == TrapCause::LoadAddressMisaligned);
	}
	assert(load_exception_thrown);
	assert(failed_load_cpu.read_pc() == 0);
	assert(failed_load_cpu.read_register(3) == 0xCAFEBABEu);
	assert(failed_load_memory.read32(0) == 0x00202183u);

	/* SW through the full pipeline */
	Cpu store_cpu{};
	Memory store_memory{ 64 };

	store_cpu.write_register(1, 24);
	store_cpu.write_register(2, 0x12345678u);
	store_cpu.write_register(3, 0x89ABCDEFu);
	store_memory.write32(0, 0x0020A423); // SW x2, 8(x1)
	store_memory.write32(4, 0xFE30AE23); // SW x3, -4(x1)

	store_cpu.step(store_memory);
	assert(store_cpu.read_pc() == 4);
	assert(store_memory.read32(32) == 0x12345678u);
	assert(store_cpu.read_register(1) == 24);
	assert(store_cpu.read_register(2) == 0x12345678u);
	assert(store_cpu.read_register(3) == 0x89ABCDEFu);

	store_cpu.step(store_memory);
	assert(store_cpu.read_pc() == 8);
	assert(store_memory.read32(20) == 0x89ABCDEFu);
	assert(store_memory.read32(0) == 0x0020A423u);
	assert(store_memory.read32(4) == 0xFE30AE23u);
	assert(store_cpu.read_register(1) == 24);
	assert(store_cpu.read_register(2) == 0x12345678u);
	assert(store_cpu.read_register(3) == 0x89ABCDEFu);

	// A failed store through step leaves PC, registers, and memory unchanged
	Cpu failed_store_cpu{};
	Memory failed_store_memory{ 16 };
	failed_store_memory.write32(0, 0x00202123); // SW x2, 2(x0)
	failed_store_cpu.write_register(2, 0xCAFEBABEu);

	bool store_exception_thrown{ false };
	try {
		failed_store_cpu.step(failed_store_memory);
	} catch (const Trap& trap) {
		store_exception_thrown = true;
		assert(trap.cause == TrapCause::StoreAddressMisaligned);
	}
	assert(store_exception_thrown);
	assert(failed_store_cpu.read_pc() == 0);
	assert(failed_store_cpu.read_register(2) == 0xCAFEBABEu);
	assert(failed_store_memory.read32(0) == 0x00202123u);

	/* Byte loads through the full pipeline */
	Cpu byte_load_cpu{};
	Memory byte_load_memory{ 64 };

	byte_load_memory.write32(0, 0x01100183); // LB x3, 17(x0)
	byte_load_memory.write32(4, 0x01104203); // LBU x4, 17(x0)
	byte_load_memory.write8(17, 0x80u);

	byte_load_cpu.step(byte_load_memory);
	assert(byte_load_cpu.read_pc() == 4);
	assert(byte_load_cpu.read_register(3) == 0xFFFFFF80u);

	byte_load_cpu.step(byte_load_memory);
	assert(byte_load_cpu.read_pc() == 8);
	assert(byte_load_cpu.read_register(4) == 0x00000080u);
	assert(byte_load_memory.read8(17) == 0x80u);

	// A failed byte load through step leaves architectural state unchanged
	Cpu failed_byte_load_cpu{};
	Memory failed_byte_load_memory{ 8 };
	failed_byte_load_memory.write32(0, 0x00800183); // LB x3, 8(x0)
	failed_byte_load_cpu.write_register(3, 0xCAFEBABEu);

	bool byte_load_exception_thrown{ false };
	try {
		failed_byte_load_cpu.step(failed_byte_load_memory);
	} catch (const Trap& trap) {
		byte_load_exception_thrown = true;
		assert(trap.cause == TrapCause::LoadAccessFault);
	}
	assert(byte_load_exception_thrown);
	assert(failed_byte_load_cpu.read_pc() == 0);
	assert(failed_byte_load_cpu.read_register(3) == 0xCAFEBABEu);
	assert(failed_byte_load_memory.read32(0) == 0x00800183u);

	/* SB through the full pipeline */
	Cpu byte_store_cpu{};
	Memory byte_store_memory{ 64 };

	byte_store_cpu.write_register(2, 0x12345680u);
	byte_store_cpu.write_register(3, 0xABCDEF7Fu);
	byte_store_memory.write32(0, 0x002008A3); // SB x2, 17(x0)
	byte_store_memory.write32(4, 0x00300923); // SB x3, 18(x0)

	byte_store_cpu.step(byte_store_memory);
	assert(byte_store_cpu.read_pc() == 4);
	assert(byte_store_memory.read8(17) == 0x80u);
	assert(byte_store_cpu.read_register(2) == 0x12345680u);

	byte_store_cpu.step(byte_store_memory);
	assert(byte_store_cpu.read_pc() == 8);
	assert(byte_store_memory.read8(18) == 0x7Fu);
	assert(byte_store_memory.read32(0) == 0x002008A3u);
	assert(byte_store_memory.read32(4) == 0x00300923u);
	assert(byte_store_cpu.read_register(2) == 0x12345680u);
	assert(byte_store_cpu.read_register(3) == 0xABCDEF7Fu);

	// A failed byte store through step leaves state unchanged
	Cpu failed_byte_store_cpu{};
	Memory failed_byte_store_memory{ 8 };
	failed_byte_store_memory.write32(0, 0x00200423); // SB x2, 8(x0)
	failed_byte_store_cpu.write_register(2, 0xCAFEBABEu);

	bool byte_store_exception_thrown{ false };
	try {
		failed_byte_store_cpu.step(failed_byte_store_memory);
	} catch (const Trap& trap) {
		byte_store_exception_thrown = true;
		assert(trap.cause == TrapCause::StoreAccessFault);
	}
	assert(byte_store_exception_thrown);
	assert(failed_byte_store_cpu.read_pc() == 0);
	assert(failed_byte_store_cpu.read_register(2) == 0xCAFEBABEu);
	assert(failed_byte_store_memory.read32(0) == 0x00200423u);

	/* Halfword loads through the full pipeline */
	Cpu halfword_load_cpu{};
	Memory halfword_load_memory{ 64 };

	halfword_load_cpu.write_register(1, 16);
	halfword_load_memory.write32(0, 0x00209183); // LH x3, 2(x1)
	halfword_load_memory.write32(4, 0x0020D203); // LHU x4, 2(x1)
	halfword_load_memory.write16(18, 0x8001u);

	halfword_load_cpu.step(halfword_load_memory);
	assert(halfword_load_cpu.read_pc() == 4);
	assert(halfword_load_cpu.read_register(3) == 0xFFFF8001u);

	halfword_load_cpu.step(halfword_load_memory);
	assert(halfword_load_cpu.read_pc() == 8);
	assert(halfword_load_cpu.read_register(4) == 0x00008001u);
	assert(halfword_load_cpu.read_register(1) == 16);
	assert(halfword_load_memory.read16(18) == 0x8001u);

	// A failed halfword load through step leaves state unchanged
	Cpu failed_halfword_load_cpu{};
	Memory failed_halfword_load_memory{ 8 };
	failed_halfword_load_memory.write32(0, 0x00101183); // LH x3, 1(x0)
	failed_halfword_load_cpu.write_register(3, 0xCAFEBABEu);

	bool halfword_load_exception_thrown{ false };
	try {
		failed_halfword_load_cpu.step(failed_halfword_load_memory);
	} catch (const Trap& trap) {
		halfword_load_exception_thrown = true;
		assert(trap.cause == TrapCause::LoadAddressMisaligned);
	}
	assert(halfword_load_exception_thrown);
	assert(failed_halfword_load_cpu.read_pc() == 0);
	assert(failed_halfword_load_cpu.read_register(3) == 0xCAFEBABEu);
	assert(failed_halfword_load_memory.read32(0) == 0x00101183u);

	/* SH through the full pipeline */
	Cpu halfword_store_cpu{};
	Memory halfword_store_memory{ 64 };

	halfword_store_cpu.write_register(2, 0x12348001u);
	halfword_store_cpu.write_register(3, 0xABCD7FFFu);
	halfword_store_memory.write32(0, 0x00201923); // SH x2, 18(x0)
	halfword_store_memory.write32(4, 0x00301A23); // SH x3, 20(x0)

	halfword_store_cpu.step(halfword_store_memory);
	assert(halfword_store_cpu.read_pc() == 4);
	assert(halfword_store_memory.read16(18) == 0x8001u);
	assert(halfword_store_cpu.read_register(2) == 0x12348001u);

	halfword_store_cpu.step(halfword_store_memory);
	assert(halfword_store_cpu.read_pc() == 8);
	assert(halfword_store_memory.read16(20) == 0x7FFFu);
	assert(halfword_store_memory.read32(0) == 0x00201923u);
	assert(halfword_store_memory.read32(4) == 0x00301A23u);
	assert(halfword_store_cpu.read_register(2) == 0x12348001u);
	assert(halfword_store_cpu.read_register(3) == 0xABCD7FFFu);

	// A failed halfword store through step leaves state unchanged
	Cpu failed_halfword_store_cpu{};
	Memory failed_halfword_store_memory{ 8 };
	failed_halfword_store_memory.write32(0, 0x002010A3); // SH x2, 1(x0)
	failed_halfword_store_cpu.write_register(2, 0xCAFEBABEu);

	bool halfword_store_exception_thrown{ false };
	try {
		failed_halfword_store_cpu.step(failed_halfword_store_memory);
	} catch (const Trap& trap) {
		halfword_store_exception_thrown = true;
		assert(trap.cause == TrapCause::StoreAddressMisaligned);
	}
	assert(halfword_store_exception_thrown);
	assert(failed_halfword_store_cpu.read_pc() == 0);
	assert(failed_halfword_store_cpu.read_register(2) == 0xCAFEBABEu);
	assert(failed_halfword_store_memory.read32(0) == 0x002010A3u);

	/* BEQ and BNE through the full pipeline */
	Cpu forward_branch_cpu{};
	Memory forward_branch_memory{ 16 };
	forward_branch_cpu.write_register(1, 42);
	forward_branch_cpu.write_register(2, 42);
	forward_branch_memory.write32(0, 0x00208463); // BEQ x1, x2, +8
	forward_branch_memory.write32(4, 0x00100193); // ADDI x3, x0, 1 (skipped)
	forward_branch_memory.write32(8, 0x00200193); // ADDI x3, x0, 2

	forward_branch_cpu.step(forward_branch_memory);
	assert(forward_branch_cpu.read_pc() == 8);
	assert(forward_branch_cpu.read_register(3) == 0);
	forward_branch_cpu.step(forward_branch_memory);
	assert(forward_branch_cpu.read_pc() == 12);
	assert(forward_branch_cpu.read_register(3) == 2);
	assert(forward_branch_memory.read32(0) == 0x00208463u);

	// Taken BNE can branch backward
	Cpu backward_branch_cpu{};
	Memory backward_branch_memory{ 12 };
	backward_branch_cpu.write_register(1, 1);
	backward_branch_cpu.write_register(2, 2);
	backward_branch_cpu.set_pc(8);
	backward_branch_memory.write32(4, 0x00700213); // ADDI x4, x0, 7
	backward_branch_memory.write32(8, 0xFE209EE3); // BNE x1, x2, -4

	backward_branch_cpu.step(backward_branch_memory);
	assert(backward_branch_cpu.read_pc() == 4);
	backward_branch_cpu.step(backward_branch_memory);
	assert(backward_branch_cpu.read_pc() == 8);
	assert(backward_branch_cpu.read_register(4) == 7);

	// A not-taken branch ignores a misaligned encoded target
	Cpu not_taken_branch_cpu{};
	Memory not_taken_branch_memory{ 8 };
	not_taken_branch_cpu.write_register(1, 1);
	not_taken_branch_cpu.write_register(2, 2);
	not_taken_branch_memory.write32(0, 0x00208163); // BEQ x1, x2, +2
	not_taken_branch_cpu.step(not_taken_branch_memory);
	assert(not_taken_branch_cpu.read_pc() == 4);

	// A taken misaligned target leaves architectural state unchanged
	Cpu failed_branch_cpu{};
	Memory failed_branch_memory{ 8 };
	failed_branch_cpu.write_register(1, 42);
	failed_branch_cpu.write_register(2, 42);
	failed_branch_cpu.write_register(3, 0xCAFEBABEu);
	failed_branch_memory.write32(0, 0x00208163); // BEQ x1, x2, +2

	bool branch_exception_thrown{ false };
	try {
		failed_branch_cpu.step(failed_branch_memory);
	} catch (const Trap& trap) {
		branch_exception_thrown = true;
		assert(trap.cause == TrapCause::InstructionAddressMisaligned);
	}
	assert(branch_exception_thrown);
	assert(failed_branch_cpu.read_pc() == 0);
	assert(failed_branch_cpu.read_register(1) == 42);
	assert(failed_branch_cpu.read_register(2) == 42);
	assert(failed_branch_cpu.read_register(3) == 0xCAFEBABEu);
	assert(failed_branch_memory.read32(0) == 0x00208163u);

	// A taken offset-zero branch keeps the PC at its current instruction
	Cpu self_branch_cpu{};
	Memory self_branch_memory{ 4 };
	self_branch_cpu.write_register(1, 42);
	self_branch_memory.write32(0, 0x00108063); // BEQ x1, x1, 0
	self_branch_cpu.step(self_branch_memory);
	assert(self_branch_cpu.read_pc() == 0);
	assert(self_branch_cpu.read_register(1) == 42);

	/* BLT and BGE through the full pipeline */
	Cpu signed_forward_branch_cpu{};
	Memory signed_forward_branch_memory{ 16 };
	signed_forward_branch_cpu.write_register(1, 0xFFFFFFFFu); // -1 when signed
	signed_forward_branch_cpu.write_register(2, 1);
	signed_forward_branch_memory.write32(0, 0x0020C463); // BLT x1, x2, +8
	signed_forward_branch_memory.write32(4, 0x00100193); // ADDI x3, x0, 1 (skipped)
	signed_forward_branch_memory.write32(8, 0x00200193); // ADDI x3, x0, 2

	signed_forward_branch_cpu.step(signed_forward_branch_memory);
	assert(signed_forward_branch_cpu.read_pc() == 8);
	assert(signed_forward_branch_cpu.read_register(3) == 0);
	signed_forward_branch_cpu.step(signed_forward_branch_memory);
	assert(signed_forward_branch_cpu.read_pc() == 12);
	assert(signed_forward_branch_cpu.read_register(3) == 2);
	assert(signed_forward_branch_cpu.read_register(1) == 0xFFFFFFFFu);
	assert(signed_forward_branch_cpu.read_register(2) == 1);
	assert(signed_forward_branch_memory.read32(0) == 0x0020C463u);

	// Equality takes BGE through fetch, decode, and execute
	Cpu equal_bge_cpu{};
	Memory equal_bge_memory{ 12 };
	equal_bge_cpu.write_register(1, 0x80000000u);
	equal_bge_cpu.write_register(2, 0x80000000u);
	equal_bge_memory.write32(0, 0x0020D463); // BGE x1, x2, +8
	equal_bge_memory.write32(4, 0x00100213); // ADDI x4, x0, 1 (skipped)

	equal_bge_cpu.step(equal_bge_memory);
	assert(equal_bge_cpu.read_pc() == 8);
	assert(equal_bge_cpu.read_register(4) == 0);
	assert(equal_bge_cpu.read_register(1) == 0x80000000u);
	assert(equal_bge_cpu.read_register(2) == 0x80000000u);
	assert(equal_bge_memory.read32(0) == 0x0020D463u);

	/* BLTU and BGEU through the full pipeline */
	Cpu not_taken_bltu_cpu{};
	Memory not_taken_bltu_memory{ 12 };
	not_taken_bltu_cpu.write_register(1, 0xFFFFFFFFu);
	not_taken_bltu_cpu.write_register(2, 1);
	not_taken_bltu_memory.write32(0, 0x0020E463); // BLTU x1, x2, +8 (not taken)
	not_taken_bltu_memory.write32(4, 0x00100193); // ADDI x3, x0, 1

	not_taken_bltu_cpu.step(not_taken_bltu_memory);
	assert(not_taken_bltu_cpu.read_pc() == 4);
	assert(not_taken_bltu_cpu.read_register(3) == 0);
	not_taken_bltu_cpu.step(not_taken_bltu_memory);
	assert(not_taken_bltu_cpu.read_pc() == 8);
	assert(not_taken_bltu_cpu.read_register(3) == 1);
	assert(not_taken_bltu_cpu.read_register(1) == 0xFFFFFFFFu);
	assert(not_taken_bltu_cpu.read_register(2) == 1);
	assert(not_taken_bltu_memory.read32(0) == 0x0020E463u);

	// The same operands take BGEU and skip the sequential instruction
	Cpu taken_bgeu_cpu{};
	Memory taken_bgeu_memory{ 16 };
	taken_bgeu_cpu.write_register(1, 0xFFFFFFFFu);
	taken_bgeu_cpu.write_register(2, 1);
	taken_bgeu_memory.write32(0, 0x0020F463); // BGEU x1, x2, +8
	taken_bgeu_memory.write32(4, 0x00100213); // ADDI x4, x0, 1 (skipped)
	taken_bgeu_memory.write32(8, 0x00200213); // ADDI x4, x0, 2

	taken_bgeu_cpu.step(taken_bgeu_memory);
	assert(taken_bgeu_cpu.read_pc() == 8);
	assert(taken_bgeu_cpu.read_register(4) == 0);
	taken_bgeu_cpu.step(taken_bgeu_memory);
	assert(taken_bgeu_cpu.read_pc() == 12);
	assert(taken_bgeu_cpu.read_register(4) == 2);
	assert(taken_bgeu_cpu.read_register(1) == 0xFFFFFFFFu);
	assert(taken_bgeu_cpu.read_register(2) == 1);
	assert(taken_bgeu_memory.read32(0) == 0x0020F463u);

	/* LUI through the full pipeline */
	Cpu lui_cpu{};
	Memory lui_memory{ 12 };
	lui_memory.write32(0, 0x123450B7); // LUI x1, 0x12345
	lui_memory.write32(4, 0xFFFFF137); // LUI x2, 0xFFFFF
	lui_memory.write32(8, 0xABCDE037); // LUI x0, 0xABCDE

	lui_cpu.step(lui_memory);
	assert(lui_cpu.read_pc() == 4);
	assert(lui_cpu.read_register(1) == 0x12345000u);
	assert(lui_cpu.read_register(2) == 0);

	lui_cpu.step(lui_memory);
	assert(lui_cpu.read_pc() == 8);
	assert(lui_cpu.read_register(1) == 0x12345000u);
	assert(lui_cpu.read_register(2) == 0xFFFFF000u);

	lui_cpu.step(lui_memory);
	assert(lui_cpu.read_pc() == 12);
	assert(lui_cpu.read_register(0) == 0);
	assert(lui_cpu.read_register(1) == 0x12345000u);
	assert(lui_cpu.read_register(2) == 0xFFFFF000u);
	assert(lui_memory.read32(0) == 0x123450B7u);
	assert(lui_memory.read32(4) == 0xFFFFF137u);
	assert(lui_memory.read32(8) == 0xABCDE037u);

	/* AUIPC through the full pipeline */
	Cpu auipc_cpu{};
	Memory auipc_memory{ 12 };
	auipc_memory.write32(0, 0x12345097); // AUIPC x1, 0x12345
	auipc_memory.write32(4, 0x00000117); // AUIPC x2, 0
	auipc_memory.write32(8, 0xABCDE017); // AUIPC x0, 0xABCDE

	auipc_cpu.step(auipc_memory);
	assert(auipc_cpu.read_pc() == 4);
	assert(auipc_cpu.read_register(1) == 0x12345000u);
	assert(auipc_cpu.read_register(2) == 0);

	// The instruction at address 4 uses 4, not the following PC value 8
	auipc_cpu.step(auipc_memory);
	assert(auipc_cpu.read_pc() == 8);
	assert(auipc_cpu.read_register(1) == 0x12345000u);
	assert(auipc_cpu.read_register(2) == 4);

	auipc_cpu.step(auipc_memory);
	assert(auipc_cpu.read_pc() == 12);
	assert(auipc_cpu.read_register(0) == 0);
	assert(auipc_cpu.read_register(1) == 0x12345000u);
	assert(auipc_cpu.read_register(2) == 4);
	assert(auipc_memory.read32(0) == 0x12345097u);
	assert(auipc_memory.read32(4) == 0x00000117u);
	assert(auipc_memory.read32(8) == 0xABCDE017u);

	/* JAL through the full pipeline */
	Cpu jal_cpu{};
	Memory jal_memory{ 16 };
	jal_memory.write32(0, 0x008000EF); // JAL x1, +8
	jal_memory.write32(4, 0x00100113); // ADDI x2, x0, 1 (skipped)
	jal_memory.write32(8, 0x00200113); // ADDI x2, x0, 2

	jal_cpu.step(jal_memory);
	assert(jal_cpu.read_pc() == 8);
	assert(jal_cpu.read_register(1) == 4);
	assert(jal_cpu.read_register(2) == 0);

	jal_cpu.step(jal_memory);
	assert(jal_cpu.read_pc() == 12);
	assert(jal_cpu.read_register(1) == 4);
	assert(jal_cpu.read_register(2) == 2);
	assert(jal_memory.read32(0) == 0x008000EFu);
	assert(jal_memory.read32(4) == 0x00100113u);
	assert(jal_memory.read32(8) == 0x00200113u);

	// A failed JAL through step leaves all architectural state unchanged
	Cpu failed_jal_cpu{};
	Memory failed_jal_memory{ 8 };
	failed_jal_cpu.write_register(1, 0xDEADBEEFu);
	failed_jal_cpu.write_register(2, 0xCAFEBABEu);
	failed_jal_memory.write32(0, 0x002000EF); // JAL x1, +2
	failed_jal_memory.write32(4, 0xA5A5A5A5u);

	bool jal_exception_thrown{ false };
	try {
		failed_jal_cpu.step(failed_jal_memory);
	} catch (const Trap& trap) {
		jal_exception_thrown = true;
		assert(trap.cause == TrapCause::InstructionAddressMisaligned);
	}
	assert(jal_exception_thrown);
	assert(failed_jal_cpu.read_pc() == 0);
	assert(failed_jal_cpu.read_register(1) == 0xDEADBEEFu);
	assert(failed_jal_cpu.read_register(2) == 0xCAFEBABEu);
	assert(failed_jal_memory.read32(0) == 0x002000EFu);
	assert(failed_jal_memory.read32(4) == 0xA5A5A5A5u);

	/* JALR through the full pipeline */
	Cpu jalr_cpu{};
	Memory jalr_memory{ 16 };
	jalr_cpu.write_register(2, 9);
	jalr_memory.write32(0, 0x000100E7); // JALR x1, 0(x2), target bit zero cleared
	jalr_memory.write32(4, 0x00100193); // ADDI x3, x0, 1 (skipped)
	jalr_memory.write32(8, 0x00200193); // ADDI x3, x0, 2

	jalr_cpu.step(jalr_memory);
	assert(jalr_cpu.read_pc() == 8);
	assert(jalr_cpu.read_register(1) == 4);
	assert(jalr_cpu.read_register(2) == 9);
	assert(jalr_cpu.read_register(3) == 0);

	jalr_cpu.step(jalr_memory);
	assert(jalr_cpu.read_pc() == 12);
	assert(jalr_cpu.read_register(1) == 4);
	assert(jalr_cpu.read_register(2) == 9);
	assert(jalr_cpu.read_register(3) == 2);
	assert(jalr_memory.read32(0) == 0x000100E7u);
	assert(jalr_memory.read32(4) == 0x00100193u);
	assert(jalr_memory.read32(8) == 0x00200193u);

	// A failed JALR through step leaves all architectural state unchanged
	Cpu failed_jalr_cpu{};
	Memory failed_jalr_memory{ 8 };
	failed_jalr_cpu.write_register(1, 0xDEADBEEFu);
	failed_jalr_cpu.write_register(2, 102);
	failed_jalr_cpu.write_register(3, 0xCAFEBABEu);
	failed_jalr_memory.write32(0, 0x000100E7); // JALR x1, 0(x2)
	failed_jalr_memory.write32(4, 0xA5A5A5A5u);

	bool jalr_exception_thrown{ false };
	try {
		failed_jalr_cpu.step(failed_jalr_memory);
	} catch (const Trap& trap) {
		jalr_exception_thrown = true;
		assert(trap.cause == TrapCause::InstructionAddressMisaligned);
	}
	assert(jalr_exception_thrown);
	assert(failed_jalr_cpu.read_pc() == 0);
	assert(failed_jalr_cpu.read_register(1) == 0xDEADBEEFu);
	assert(failed_jalr_cpu.read_register(2) == 102);
	assert(failed_jalr_cpu.read_register(3) == 0xCAFEBABEu);
	assert(failed_jalr_memory.read32(0) == 0x000100E7u);
	assert(failed_jalr_memory.read32(4) == 0xA5A5A5A5u);

	/* FENCE through the full pipeline */
	Cpu fence_cpu{};
	Memory fence_memory{ 16 };
	fence_cpu.write_register(1, 0xDEADBEEFu);
	fence_cpu.write_register(2, 0xCAFEBABEu);
	fence_memory.write32(0, 0x0FF0000F); // FENCE IORW, IORW
	fence_memory.write32(4, 0x8330000F); // FENCE.TSO
	fence_memory.write32(8, 0x0FF1008F); // FENCE with ignored rs1 and rd fields
	fence_memory.write32(12, 0x00700193); // ADDI x3, x0, 7

	fence_cpu.step(fence_memory);
	assert(fence_cpu.read_pc() == 4);
	assert(fence_cpu.read_register(1) == 0xDEADBEEFu);
	assert(fence_cpu.read_register(2) == 0xCAFEBABEu);

	fence_cpu.step(fence_memory);
	assert(fence_cpu.read_pc() == 8);
	assert(fence_cpu.read_register(1) == 0xDEADBEEFu);
	assert(fence_cpu.read_register(2) == 0xCAFEBABEu);

	fence_cpu.step(fence_memory);
	assert(fence_cpu.read_pc() == 12);
	assert(fence_cpu.read_register(1) == 0xDEADBEEFu);
	assert(fence_cpu.read_register(2) == 0xCAFEBABEu);

	fence_cpu.step(fence_memory);
	assert(fence_cpu.read_pc() == 16);
	assert(fence_cpu.read_register(1) == 0xDEADBEEFu);
	assert(fence_cpu.read_register(2) == 0xCAFEBABEu);
	assert(fence_cpu.read_register(3) == 7);
	assert(fence_memory.read32(0) == 0x0FF0000Fu);
	assert(fence_memory.read32(4) == 0x8330000Fu);
	assert(fence_memory.read32(8) == 0x0FF1008Fu);
	assert(fence_memory.read32(12) == 0x00700193u);

	// FENCE.I is unsupported and traps without changing architectural state
	Cpu fence_i_cpu{};
	Memory fence_i_memory{ 4 };
	fence_i_cpu.write_register(1, 0x12345678u);
	fence_i_memory.write32(0, 0x0000100F);
	bool fence_i_trap_thrown{ false };
	TrapCause fence_i_trap_cause{ TrapCause::BreakPoint };
	try {
		fence_i_cpu.step(fence_i_memory);
	} catch (const Trap& trap) {
		fence_i_trap_thrown = true;
		fence_i_trap_cause = trap.cause;
	}
	assert(fence_i_trap_thrown);
	assert(fence_i_trap_cause == TrapCause::IllegalInstruction);
	assert(fence_i_cpu.read_pc() == 0);
	assert(fence_i_cpu.read_register(1) == 0x12345678u);
	assert(fence_i_memory.read32(0) == 0x0000100Fu);

	/* ECALL and EBREAK through the full pipeline */
	Cpu ecall_cpu{};
	Memory ecall_memory{ 8 };
	ecall_cpu.write_register(1, 0xDEADBEEFu);
	ecall_cpu.write_register(2, 0xCAFEBABEu);
	ecall_memory.write32(0, 0x00000073); // ECALL
	ecall_memory.write32(4, 0xA5A5A5A5u);

	bool trap_thrown{ false };
	TrapCause trap_cause{ TrapCause::BreakPoint };
	try {
		ecall_cpu.step(ecall_memory);
	} catch (const Trap& trap) {
		trap_thrown = true;
		trap_cause = trap.cause;
	}
	assert(trap_thrown);
	assert(trap_cause == TrapCause::EnvironmentCall);
	assert(ecall_cpu.read_pc() == 0);
	assert(ecall_cpu.read_register(1) == 0xDEADBEEFu);
	assert(ecall_cpu.read_register(2) == 0xCAFEBABEu);
	assert(ecall_memory.read32(0) == 0x00000073u);
	assert(ecall_memory.read32(4) == 0xA5A5A5A5u);

	Cpu ebreak_cpu{};
	Memory ebreak_memory{ 8 };
	ebreak_cpu.write_register(1, 0x12345678u);
	ebreak_cpu.write_register(2, 0x87654321u);
	ebreak_memory.write32(0, 0x00100073); // EBREAK
	ebreak_memory.write32(4, 0x5A5A5A5Au);

	trap_thrown = false;
	trap_cause = TrapCause::EnvironmentCall;
	try {
		ebreak_cpu.step(ebreak_memory);
	} catch (const Trap& trap) {
		trap_thrown = true;
		trap_cause = trap.cause;
	}
	assert(trap_thrown);
	assert(trap_cause == TrapCause::BreakPoint);
	assert(ebreak_cpu.read_pc() == 0);
	assert(ebreak_cpu.read_register(1) == 0x12345678u);
	assert(ebreak_cpu.read_register(2) == 0x87654321u);
	assert(ebreak_memory.read32(0) == 0x00100073u);
	assert(ebreak_memory.read32(4) == 0x5A5A5A5Au);

	// Misaligned PC leaves state unchanged
	cpu.set_pc(2);
	exception_thrown = false;
	try {
		cpu.step(memory);
	} catch (const Trap& trap) {
		exception_thrown = true;
		assert(trap.cause == TrapCause::InstructionAddressMisaligned);
	}
	assert(exception_thrown);
	assert(cpu.read_pc() == 2);
	for (std::size_t i = 0; i < registers_before_error.size(); i++) {
		assert(cpu.read_register(i) == registers_before_error[i]);
	}

	return 0;
}
