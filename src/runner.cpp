#include "runner.hpp"
#include "cpu.hpp"
#include "memory.hpp"

RunResult run_until_trap(Cpu& cpu, Memory& memory, std::size_t max_instruction_count) {
	std::size_t instructions_retired{};

	try {
		while (instructions_retired < max_instruction_count) {
			cpu.step(memory);
			instructions_retired++;
		}
	} catch (const Trap& trap) {
		return RunResult{
			.trap = trap.cause,
			.instructions_retired = instructions_retired
		};
	}

	return RunResult{
		.trap = std::nullopt,
		.instructions_retired = instructions_retired
	};
}
