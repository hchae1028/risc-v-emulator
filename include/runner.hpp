#ifndef RUNNER_H_
#define RUNNER_H_

#include "executor.hpp"
#include "trap.hpp"
#include <cstddef>
#include <optional>

class Cpu;
class Bus;
class Machine;

struct RunResult {
	std::optional<Trap> trap;
	std::size_t instructions_retired;
};

RunResult run_until_trap(Cpu& cpu, Bus& bus, std::size_t max_instruction_count);

RunResult run_until_breakpoint(Machine& machine, std::size_t max_step_count);

#endif
