#include "bus.hpp"
#include "cpu.hpp"
#include "decoder.hpp"
#include "executor.hpp"
#include "memory.hpp"
#include "runner.hpp"
#include "trap.hpp"
#include <cassert>
#include <cstdint>
#include <exception>

namespace {

template <typename Function>
Trap capture_trap(Function function) {
	try {
		function();
	} catch (const Trap& trap) {
		return trap;
	}

	assert(false);
	std::terminate();
}

}

int main() {
	/* Trap causes use the architectural synchronous-exception codes. */
	static_assert(static_cast<std::uint32_t>(TrapCause::InstructionAddressMisaligned) == 0u);
	static_assert(static_cast<std::uint32_t>(TrapCause::InstructionAccessFault) == 1u);
	static_assert(static_cast<std::uint32_t>(TrapCause::IllegalInstruction) == 2u);
	static_assert(static_cast<std::uint32_t>(TrapCause::BreakPoint) == 3u);
	static_assert(static_cast<std::uint32_t>(TrapCause::LoadAddressMisaligned) == 4u);
	static_assert(static_cast<std::uint32_t>(TrapCause::LoadAccessFault) == 5u);
	static_assert(static_cast<std::uint32_t>(TrapCause::StoreAddressMisaligned) == 6u);
	static_assert(static_cast<std::uint32_t>(TrapCause::StoreAccessFault) == 7u);
	static_assert(static_cast<std::uint32_t>(TrapCause::EnvironmentCall) == 11u);

	constexpr std::uint32_t illegal_word{ 0xFFFFFFFFu };
	assert(decode_instruction(illegal_word).word == illegal_word);

	Memory memory{ 4 };
	Bus bus{ memory, 0 };
	Cpu cpu{};

	/* Instruction exceptions report either the instruction bits or faulting PC. */
	auto trap{ capture_trap([&] {
		static_cast<void>(execute_instruction(cpu, decode_instruction(illegal_word), bus));
	}) };
	assert(trap.cause == TrapCause::IllegalInstruction);
	assert(trap.tval == illegal_word);

	cpu.set_pc(0x100u);
	trap = capture_trap([&] {
		static_cast<void>(execute_instruction(cpu, decode_instruction(0x00100073u), bus));
	});
	assert(trap.cause == TrapCause::BreakPoint);
	assert(trap.tval == 0x100u);

	trap = capture_trap([&] {
		static_cast<void>(execute_instruction(cpu, decode_instruction(0x00000073u), bus));
	});
	assert(trap.cause == TrapCause::EnvironmentCall);
	assert(trap.tval == 0u);

	Cpu misaligned_fetch_cpu{};
	misaligned_fetch_cpu.set_pc(2u);
	trap = capture_trap([&] {
		static_cast<void>(misaligned_fetch_cpu.fetch_instruction(bus));
	});
	assert(trap.cause == TrapCause::InstructionAddressMisaligned);
	assert(trap.tval == 2u);

	Cpu access_fetch_cpu{};
	access_fetch_cpu.set_pc(4u);
	trap = capture_trap([&] {
		static_cast<void>(access_fetch_cpu.fetch_instruction(bus));
	});
	assert(trap.cause == TrapCause::InstructionAccessFault);
	assert(trap.tval == 4u);

	/* Control-flow and data exceptions report the attempted effective address. */
	cpu.set_pc(0u);
	trap = capture_trap([&] {
		static_cast<void>(execute_instruction(cpu, decode_instruction(0x002000EFu), bus));
	});
	assert(trap.cause == TrapCause::InstructionAddressMisaligned);
	assert(trap.tval == 2u);

	trap = capture_trap([&] {
		static_cast<void>(execute_instruction(cpu, decode_instruction(0x00202083u), bus));
	});
	assert(trap.cause == TrapCause::LoadAddressMisaligned);
	assert(trap.tval == 2u);

	trap = capture_trap([&] {
		static_cast<void>(execute_instruction(cpu, decode_instruction(0x00402083u), bus));
	});
	assert(trap.cause == TrapCause::LoadAccessFault);
	assert(trap.tval == 4u);

	trap = capture_trap([&] {
		static_cast<void>(execute_instruction(cpu, decode_instruction(0x00002123u), bus));
	});
	assert(trap.cause == TrapCause::StoreAddressMisaligned);
	assert(trap.tval == 2u);

	trap = capture_trap([&] {
		static_cast<void>(execute_instruction(cpu, decode_instruction(0x00002223u), bus));
	});
	assert(trap.cause == TrapCause::StoreAccessFault);
	assert(trap.tval == 4u);

	/* Unsupported CSR accesses retain the complete offending instruction. */
	constexpr std::uint32_t unsupported_csr{ 0xFFF090F3u }; // CSRRW x1, 0xFFF, x1
	cpu.write_register(1, 0xAAAAAAAAu);
	trap = capture_trap([&] {
		static_cast<void>(execute_instruction(cpu, decode_instruction(unsupported_csr), bus));
	});
	assert(trap.cause == TrapCause::IllegalInstruction);
	assert(trap.tval == unsupported_csr);

	/* The runner preserves tval instead of reducing a trap to its cause. */
	Memory runner_memory{ 8 };
	runner_memory.write32(4, 0x00100073u);
	Bus runner_bus{ runner_memory, 0 };
	Cpu runner_cpu{};
	runner_cpu.set_pc(4u);
	auto result{ run_until_trap(runner_cpu, runner_bus, 1) };
	assert(result.trap.has_value());
	assert(result.trap->cause == TrapCause::BreakPoint);
	assert(result.trap->tval == 4u);
	assert(result.instructions_retired == 0);

	return 0;
}
