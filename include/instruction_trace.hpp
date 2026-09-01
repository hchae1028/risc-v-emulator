#ifndef INSTRUCTION_TRACE_H_
#define INSTRUCTION_TRACE_H_

#include <cstdint>
#include <functional>

struct InstructionTrace {
	std::uint32_t pc;
	std::uint32_t instruction;
};

using InstructionTraceCallBack = std::function<void(const InstructionTrace&)>;

#endif
