#ifndef RUNNER_H_
#define RUNNER_H_

#include "executor.hpp"
#include <cstddef>
#include <optional>

class Cpu;
class Memory;

struct RunResult {
	std::optional<TrapCause> trap;
	std::size_t instructions_retired;
};

RunResult run_until_trap(Cpu& cpu, Memory& memory, std::size_t max_instruction_count);

#endif
