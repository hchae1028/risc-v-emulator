#ifndef TRAP_H_
#define TRAP_H_

#include <cstdint>

enum class TrapCause: std::uint32_t {
	InstructionAddressMisaligned = 0,
	InstructionAccessFault = 1,
	IllegalInstruction = 2,
	BreakPoint = 3,
	LoadAddressMisaligned = 4,
	LoadAccessFault = 5,
	StoreAddressMisaligned = 6,
	StoreAccessFault = 7,
	EnvironmentCall = 11,
	MachineTimerInterrupt = 0x80000007
};

struct Trap {
	TrapCause cause;
	std::uint32_t tval;
};


#endif
