#include "cpu.hpp"
#include "executor.hpp"
#include "memory.hpp"
#include "runner.hpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <stdexcept>

int main() {
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

	const auto result{ run_until_trap(cpu, memory, 10) };
	assert(result.trap.has_value());
	assert(*result.trap == TrapCause::BreakPoint);
	assert(result.instructions_retired == 3);
	assert(cpu.read_pc() == 12);
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
	const auto ecall_result{ run_until_trap(ecall_cpu, ecall_memory, 10) };
	assert(ecall_result.trap.has_value());
	assert(*ecall_result.trap == TrapCause::EnvironmentCall);
	assert(ecall_result.instructions_retired == 1);
	assert(ecall_cpu.read_pc() == 4);
	assert(ecall_cpu.read_register(1) == 5);
	assert(ecall_memory.read32(0) == 0x00500093u);
	assert(ecall_memory.read32(4) == 0x00000073u);

	/* The instruction budget can stop an infinite guest loop */
	Cpu loop_cpu{};
	Memory loop_memory{ 4 };
	loop_memory.write32(0, 0x0000006F); // JAL x0, 0
	const auto loop_result{ run_until_trap(loop_cpu, loop_memory, 5) };
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
	const auto zero_budget_result{ run_until_trap(zero_budget_cpu, zero_budget_memory, 0) };
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
	auto exact_budget_result{ run_until_trap(exact_budget_cpu, exact_budget_memory, 1) };
	assert(!exact_budget_result.trap.has_value());
	assert(exact_budget_result.instructions_retired == 1);
	assert(exact_budget_cpu.read_pc() == 4);
	assert(exact_budget_cpu.read_register(1) == 5);

	// A later run resumes at the trap and reports zero newly retired instructions
	exact_budget_result = run_until_trap(exact_budget_cpu, exact_budget_memory, 1);
	assert(exact_budget_result.trap.has_value());
	assert(*exact_budget_result.trap == TrapCause::BreakPoint);
	assert(exact_budget_result.instructions_retired == 0);
	assert(exact_budget_cpu.read_pc() == 4);
	assert(exact_budget_cpu.read_register(1) == 5);

	/* Emulator errors propagate instead of being mistaken for guest traps */
	Cpu unknown_cpu{};
	Memory unknown_memory{ 4 };
	unknown_cpu.write_register(1, 0x12345678u);
	unknown_memory.write32(0, 0xFFFFFFFFu);
	bool runtime_error_thrown{ false };
	try {
		static_cast<void>(run_until_trap(unknown_cpu, unknown_memory, 1));
	} catch (const std::runtime_error& e) {
		runtime_error_thrown = true;
	}
	assert(runtime_error_thrown);
	assert(unknown_cpu.read_pc() == 0);
	assert(unknown_cpu.read_register(1) == 0x12345678u);
	assert(unknown_memory.read32(0) == 0xFFFFFFFFu);

	Cpu out_of_range_cpu{};
	Memory out_of_range_memory{ 4 };
	out_of_range_cpu.set_pc(4);
	out_of_range_cpu.write_register(1, 0x87654321u);
	bool out_of_range_thrown{ false };
	try {
		static_cast<void>(run_until_trap(out_of_range_cpu, out_of_range_memory, 1));
	} catch (const std::out_of_range& e) {
		out_of_range_thrown = true;
	}
	assert(out_of_range_thrown);
	assert(out_of_range_cpu.read_pc() == 4);
	assert(out_of_range_cpu.read_register(1) == 0x87654321u);

	return 0;
}
