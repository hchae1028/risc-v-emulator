#include "runner.hpp"
#include "cpu.hpp"
#include "bus.hpp"
#include "machine.hpp"
#include "instruction_trace.hpp"
#include <cstddef>
#include <optional>

RunResult run_until_trap(Cpu& cpu, Bus& bus, std::size_t max_instruction_count) {
	std::size_t instructions_retired{};

	try {
		while (instructions_retired < max_instruction_count) {
			cpu.step(bus);
			instructions_retired++;
		}
	} catch (const Trap& trap) {
		return RunResult{
			.trap = trap,
			.instructions_retired = instructions_retired
		};
	}

	return RunResult{
		.trap = std::nullopt,
		.instructions_retired = instructions_retired
	};
}

RunResult run_until_breakpoint(Machine& machine, std::size_t max_step_count, const InstructionTraceCallBack& trace) {
	std::size_t instructions_retired{};

	for (std::size_t i{}; i < max_step_count; i++) {
		try {
			machine.step(trace);
			instructions_retired++;
		} catch (const Trap& trap) {
			if (trap.cause == TrapCause::BreakPoint) {
				return RunResult{
					.trap = trap,
					.instructions_retired = instructions_retired
				};
			}
		}
	}

	return RunResult{
		.trap = std::nullopt,
		.instructions_retired = instructions_retired
	};
}
