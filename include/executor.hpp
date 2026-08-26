#ifndef EXECUTOR_H_
#define EXECUTOR_H_

#include "cpu.hpp"
#include "decoder.hpp"
#include <cstdint>
#include <optional>

class Bus;

enum class TrapCause {
	EnvironmentCall,
	BreakPoint,
	IllegalInstruction,
	InstructionAddressMisaligned,
	LoadAddressMisaligned,
	StoreAddressMisaligned,
	InstructionAccessFault,
	LoadAccessFault,
	StoreAccessFault
};

struct Trap {
	TrapCause cause;
};

std::optional<std::uint32_t> execute_instruction(Cpu& cpu, const Instruction& instruction, Bus& bus);

#endif
