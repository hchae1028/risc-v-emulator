#include "bus.hpp"
#include "cpu.hpp"
#include "executor.hpp"
#include "memory.hpp"
#include "runner.hpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <stdexcept>

namespace {

RunResult run_with_ram(Cpu& cpu, Memory& ram, std::size_t max_instruction_count) {
	Bus bus{ ram, 0 };
	return run_until_trap(cpu, bus, max_instruction_count);
}

}

int main() {
	constexpr std::uint16_t mepc{ 0x341u };
	/* Run a complete hand-encoded program until EBREAK */
	const std::array<std::uint8_t, 16> program_bytes{
		0x93u, 0x00u, 0x50u, 0x00u, // ADDI x1, x0, 5
		0x13u, 0x01u, 0x70u, 0x00u, // ADDI x2, x0, 7
		0xB3u, 0x81u, 0x20u, 0x00u, // ADD x3, x1, x2
		0x73u, 0x00u, 0x10u, 0x00u  // EBREAK
	};
	Cpu cpu{};
	Memory memory{ program_bytes.size() };
	memory.load_bytes(0, program_bytes);

	const auto result{ run_with_ram(cpu, memory, 10) };
	assert(result.trap.has_value());
	assert(result.trap->cause == TrapCause::BreakPoint);
	assert(result.instructions_retired == 3);
	assert(cpu.read_pc() == 0);
	assert(cpu.read_csr(mepc) == 12u);
	assert(cpu.read_register(1) == 5);
	assert(cpu.read_register(2) == 7);
	assert(cpu.read_register(3) == 12);
	assert(memory.read32(0) == 0x00500093u);
	assert(memory.read32(4) == 0x00700113u);
	assert(memory.read32(8) == 0x002081B3u);
	assert(memory.read32(12) == 0x00100073u);

	/* ECALL reports its distinct cause without retiring */
	Cpu ecall_cpu{};
	Memory ecall_memory{ 8 };
	ecall_memory.write32(0, 0x00500093); // ADDI x1, x0, 5
	ecall_memory.write32(4, 0x00000073); // ECALL
	const auto ecall_result{ run_with_ram(ecall_cpu, ecall_memory, 10) };
	assert(ecall_result.trap.has_value());
	assert(ecall_result.trap->cause == TrapCause::EnvironmentCall);
	assert(ecall_result.instructions_retired == 1);
	assert(ecall_cpu.read_pc() == 0);
	assert(ecall_cpu.read_csr(mepc) == 4u);
	assert(ecall_cpu.read_register(1) == 5);
	assert(ecall_memory.read32(0) == 0x00500093u);
	assert(ecall_memory.read32(4) == 0x00000073u);

	/* The instruction budget can stop an infinite guest loop */
	Cpu loop_cpu{};
	Memory loop_memory{ 4 };
	loop_memory.write32(0, 0x0000006F); // JAL x0, 0
	const auto loop_result{ run_with_ram(loop_cpu, loop_memory, 5) };
	assert(!loop_result.trap.has_value());
	assert(loop_result.instructions_retired == 5);
	assert(loop_cpu.read_pc() == 0);
	assert(loop_cpu.read_register(0) == 0);
	assert(loop_memory.read32(0) == 0x0000006Fu);

	/* A zero budget executes nothing */
	Cpu zero_budget_cpu{};
	Memory zero_budget_memory{ 4 };
	zero_budget_cpu.write_register(1, 0xDEADBEEFu);
	zero_budget_memory.write32(0, 0x00500093);
	const auto zero_budget_result{ run_with_ram(zero_budget_cpu, zero_budget_memory, 0) };
	assert(!zero_budget_result.trap.has_value());
	assert(zero_budget_result.instructions_retired == 0);
	assert(zero_budget_cpu.read_pc() == 0);
	assert(zero_budget_cpu.read_register(1) == 0xDEADBEEFu);
	assert(zero_budget_memory.read32(0) == 0x00500093u);

	/* Reaching the exact budget does not execute the following trap */
	Cpu exact_budget_cpu{};
	Memory exact_budget_memory{ 8 };
	exact_budget_memory.write32(0, 0x00500093); // ADDI x1, x0, 5
	exact_budget_memory.write32(4, 0x00100073); // EBREAK
	auto exact_budget_result{ run_with_ram(exact_budget_cpu, exact_budget_memory, 1) };
	assert(!exact_budget_result.trap.has_value());
	assert(exact_budget_result.instructions_retired == 1);
	assert(exact_budget_cpu.read_pc() == 4);
	assert(exact_budget_cpu.read_register(1) == 5);

	// A later run takes the trap and reports zero newly retired instructions
	exact_budget_result = run_with_ram(exact_budget_cpu, exact_budget_memory, 1);
	assert(exact_budget_result.trap.has_value());
	assert(exact_budget_result.trap->cause == TrapCause::BreakPoint);
	assert(exact_budget_result.instructions_retired == 0);
	assert(exact_budget_cpu.read_pc() == 0);
	assert(exact_budget_cpu.read_csr(mepc) == 4u);
	assert(exact_budget_cpu.read_register(1) == 5);

	/* An illegal first instruction stops precisely without retiring or changing state */
	Cpu illegal_cpu{};
	Memory illegal_memory{ 4 };
	illegal_cpu.write_register(1, 0x12345678u);
	illegal_memory.write32(0, 0xFFFFFFFFu);
	const auto illegal_result{ run_with_ram(illegal_cpu, illegal_memory, 1) };
	assert(illegal_result.trap.has_value());
	assert(illegal_result.trap->cause == TrapCause::IllegalInstruction);
	assert(illegal_result.instructions_retired == 0);
	assert(illegal_cpu.read_pc() == 0);
	assert(illegal_cpu.read_register(1) == 0x12345678u);
	assert(illegal_memory.read32(0) == 0xFFFFFFFFu);

	/* Earlier instructions remain retired when a malformed RV32I encoding follows */
	Cpu malformed_cpu{};
	Memory malformed_memory{ 8 };
	malformed_cpu.write_register(3, 0xDEADBEEFu);
	malformed_memory.write32(0, 0x00500093u); // ADDI x1, x0, 5
	malformed_memory.write32(4, 0x02409193u); // SLLI with reserved upper immediate bits
	const auto malformed_result{ run_with_ram(malformed_cpu, malformed_memory, 2) };
	assert(malformed_result.trap.has_value());
	assert(malformed_result.trap->cause == TrapCause::IllegalInstruction);
	assert(malformed_result.instructions_retired == 1);
	assert(malformed_cpu.read_pc() == 0);
	assert(malformed_cpu.read_csr(mepc) == 4u);
	assert(malformed_cpu.read_register(1) == 5);
	assert(malformed_cpu.read_register(3) == 0xDEADBEEFu);
	assert(malformed_memory.read32(0) == 0x00500093u);
	assert(malformed_memory.read32(4) == 0x02409193u);

	/* An RV32M instruction is illegal while the emulator implements only RV32I */
	Cpu unsupported_extension_cpu{};
	Memory unsupported_extension_memory{ 4 };
	unsupported_extension_cpu.write_register(1, 6);
	unsupported_extension_cpu.write_register(2, 7);
	unsupported_extension_cpu.write_register(3, 0xCAFEBABEu);
	unsupported_extension_memory.write32(0, 0x022081B3u); // MUL x3, x1, x2
	const auto unsupported_extension_result{
		run_with_ram(unsupported_extension_cpu, unsupported_extension_memory, 1)
	};
	assert(unsupported_extension_result.trap.has_value());
	assert(unsupported_extension_result.trap->cause == TrapCause::IllegalInstruction);
	assert(unsupported_extension_result.instructions_retired == 0);
	assert(unsupported_extension_cpu.read_pc() == 0);
	assert(unsupported_extension_cpu.read_register(1) == 6);
	assert(unsupported_extension_cpu.read_register(2) == 7);
	assert(unsupported_extension_cpu.read_register(3) == 0xCAFEBABEu);
	assert(unsupported_extension_memory.read32(0) == 0x022081B3u);

	/* A misaligned fetch stops before any instruction is fetched or retired */
	Cpu misaligned_fetch_cpu{};
	Memory misaligned_fetch_memory{ 8 };
	misaligned_fetch_cpu.set_pc(2);
	misaligned_fetch_cpu.write_register(1, 0x12345678u);
	misaligned_fetch_memory.write32(0, 0xA5A5A5A5u);
	misaligned_fetch_memory.write32(4, 0x5A5A5A5Au);
	const auto misaligned_fetch_result{
		run_with_ram(misaligned_fetch_cpu, misaligned_fetch_memory, 1)
	};
	assert(misaligned_fetch_result.trap.has_value());
	assert(misaligned_fetch_result.trap->cause == TrapCause::InstructionAddressMisaligned);
	assert(misaligned_fetch_result.instructions_retired == 0);
	assert(misaligned_fetch_cpu.read_pc() == 0);
	assert(misaligned_fetch_cpu.read_csr(mepc) == 0u);
	assert(misaligned_fetch_cpu.read_register(1) == 0x12345678u);
	assert(misaligned_fetch_memory.read32(0) == 0xA5A5A5A5u);
	assert(misaligned_fetch_memory.read32(4) == 0x5A5A5A5Au);

	/* A failed jump preserves its link register and earlier retirement */
	Cpu misaligned_jump_cpu{};
	Memory misaligned_jump_memory{ 8 };
	misaligned_jump_cpu.write_register(5, 0xDEADBEEFu);
	misaligned_jump_memory.write32(0, 0x00500213u); // ADDI x4, x0, 5
	misaligned_jump_memory.write32(4, 0x002002EFu); // JAL x5, +2
	const auto misaligned_jump_result{
		run_with_ram(misaligned_jump_cpu, misaligned_jump_memory, 2)
	};
	assert(misaligned_jump_result.trap.has_value());
	assert(misaligned_jump_result.trap->cause == TrapCause::InstructionAddressMisaligned);
	assert(misaligned_jump_result.instructions_retired == 1);
	assert(misaligned_jump_cpu.read_pc() == 0);
	assert(misaligned_jump_cpu.read_csr(mepc) == 4u);
	assert(misaligned_jump_cpu.read_register(4) == 5);
	assert(misaligned_jump_cpu.read_register(5) == 0xDEADBEEFu);
	assert(misaligned_jump_memory.read32(4) == 0x002002EFu);

	/* A failed load preserves its destination and earlier retirement */
	Cpu misaligned_load_cpu{};
	Memory misaligned_load_memory{ 8 };
	misaligned_load_cpu.write_register(1, 1);
	misaligned_load_cpu.write_register(3, 0xCAFEBABEu);
	misaligned_load_memory.write32(0, 0x00500213u); // ADDI x4, x0, 5
	misaligned_load_memory.write32(4, 0x0000A183u); // LW x3, 0(x1)
	const auto misaligned_load_result{
		run_with_ram(misaligned_load_cpu, misaligned_load_memory, 2)
	};
	assert(misaligned_load_result.trap.has_value());
	assert(misaligned_load_result.trap->cause == TrapCause::LoadAddressMisaligned);
	assert(misaligned_load_result.instructions_retired == 1);
	assert(misaligned_load_cpu.read_pc() == 0);
	assert(misaligned_load_cpu.read_csr(mepc) == 4u);
	assert(misaligned_load_cpu.read_register(3) == 0xCAFEBABEu);
	assert(misaligned_load_cpu.read_register(4) == 5);
	assert(misaligned_load_memory.read32(0) == 0x00500213u);
	assert(misaligned_load_memory.read32(4) == 0x0000A183u);

	/* A failed store preserves memory and earlier retirement */
	Cpu misaligned_store_cpu{};
	Memory misaligned_store_memory{ 8 };
	misaligned_store_cpu.write_register(1, 2);
	misaligned_store_cpu.write_register(2, 0xCAFEBABEu);
	misaligned_store_memory.write32(0, 0x00500213u); // ADDI x4, x0, 5
	misaligned_store_memory.write32(4, 0x0020A023u); // SW x2, 0(x1)
	const auto misaligned_store_result{
		run_with_ram(misaligned_store_cpu, misaligned_store_memory, 2)
	};
	assert(misaligned_store_result.trap.has_value());
	assert(misaligned_store_result.trap->cause == TrapCause::StoreAddressMisaligned);
	assert(misaligned_store_result.instructions_retired == 1);
	assert(misaligned_store_cpu.read_pc() == 0);
	assert(misaligned_store_cpu.read_csr(mepc) == 4u);
	assert(misaligned_store_cpu.read_register(2) == 0xCAFEBABEu);
	assert(misaligned_store_cpu.read_register(4) == 5);
	assert(misaligned_store_memory.read32(0) == 0x00500213u);
	assert(misaligned_store_memory.read32(4) == 0x0020A023u);

	/* An instruction access fault is a guest trap with no retirement */
	Cpu instruction_access_cpu{};
	Memory instruction_access_memory{ 4 };
	instruction_access_cpu.set_pc(4);
	instruction_access_cpu.write_register(1, 0x87654321u);
	const auto instruction_access_result{
		run_with_ram(instruction_access_cpu, instruction_access_memory, 1)
	};
	assert(instruction_access_result.trap.has_value());
	assert(instruction_access_result.trap->cause == TrapCause::InstructionAccessFault);
	assert(instruction_access_result.instructions_retired == 0);
	assert(instruction_access_cpu.read_pc() == 0);
	assert(instruction_access_cpu.read_csr(mepc) == 4u);
	assert(instruction_access_cpu.read_register(1) == 0x87654321u);

	/* A load access fault preserves its destination and earlier retirement */
	Cpu load_access_cpu{};
	Memory load_access_memory{ 8 };
	load_access_cpu.write_register(1, 8);
	load_access_cpu.write_register(3, 0xCAFEBABEu);
	load_access_memory.write32(0, 0x00500213u); // ADDI x4, x0, 5
	load_access_memory.write32(4, 0x0000A183u); // LW x3, 0(x1)
	const auto load_access_result{
		run_with_ram(load_access_cpu, load_access_memory, 2)
	};
	assert(load_access_result.trap.has_value());
	assert(load_access_result.trap->cause == TrapCause::LoadAccessFault);
	assert(load_access_result.instructions_retired == 1);
	assert(load_access_cpu.read_pc() == 0);
	assert(load_access_cpu.read_csr(mepc) == 4u);
	assert(load_access_cpu.read_register(3) == 0xCAFEBABEu);
	assert(load_access_cpu.read_register(4) == 5);
	assert(load_access_memory.read32(0) == 0x00500213u);
	assert(load_access_memory.read32(4) == 0x0000A183u);

	/* A store access fault preserves memory and earlier retirement */
	Cpu store_access_cpu{};
	Memory store_access_memory{ 8 };
	store_access_cpu.write_register(1, 8);
	store_access_cpu.write_register(2, 0xCAFEBABEu);
	store_access_memory.write32(0, 0x00500213u); // ADDI x4, x0, 5
	store_access_memory.write32(4, 0x0020A023u); // SW x2, 0(x1)
	const auto store_access_result{
		run_with_ram(store_access_cpu, store_access_memory, 2)
	};
	assert(store_access_result.trap.has_value());
	assert(store_access_result.trap->cause == TrapCause::StoreAccessFault);
	assert(store_access_result.instructions_retired == 1);
	assert(store_access_cpu.read_pc() == 0);
	assert(store_access_cpu.read_csr(mepc) == 4u);
	assert(store_access_cpu.read_register(2) == 0xCAFEBABEu);
	assert(store_access_cpu.read_register(4) == 5);
	assert(store_access_memory.read32(0) == 0x00500213u);
	assert(store_access_memory.read32(4) == 0x0020A023u);

	/* Fetch, load, and store use physical bus addresses with nonzero-based RAM */
	constexpr std::uint32_t mapped_ram_base{ 0x80000000u };
	Memory mapped_ram{ 24 };
	mapped_ram.write32(0, 0x800000B7u);  // LUI x1, 0x80000
	mapped_ram.write32(4, 0x0100A103u);  // LW x2, 16(x1)
	mapped_ram.write32(8, 0x0020AA23u);  // SW x2, 20(x1)
	mapped_ram.write32(12, 0x00100073u); // EBREAK
	mapped_ram.write32(16, 0x12345678u);
	Bus mapped_bus{ mapped_ram, mapped_ram_base };
	Cpu mapped_cpu{};
	mapped_cpu.set_pc(mapped_ram_base);

	const auto mapped_result{ run_until_trap(mapped_cpu, mapped_bus, 10) };
	assert(mapped_result.trap.has_value());
	assert(mapped_result.trap->cause == TrapCause::BreakPoint);
	assert(mapped_result.instructions_retired == 3);
	assert(mapped_cpu.read_pc() == 0);
	assert(mapped_cpu.read_csr(mepc) == mapped_ram_base + 12);
	assert(mapped_cpu.read_register(1) == mapped_ram_base);
	assert(mapped_cpu.read_register(2) == 0x12345678u);
	assert(mapped_ram.read32(16) == 0x12345678u);
	assert(mapped_ram.read32(20) == 0x12345678u);

	return 0;
}
