#include "bus.hpp"
#include "cpu.hpp"
#include "instruction_trace.hpp"
#include "memory.hpp"
#include "trap.hpp"
#include <cassert>
#include <cstdint>
#include <vector>

int main() {
	constexpr std::uint32_t addi_x1_x0_5{ 0x00500093u };
	constexpr std::uint32_t ebreak{ 0x00100073u };
	Memory memory{ 16 };
	memory.write32(0, addi_x1_x0_5);
	memory.write32(4, ebreak);
	Bus bus{ memory, 0 };
	Cpu cpu{};
	std::vector<InstructionTrace> events;
	bool observed_before_execution{};

	InstructionTraceCallBack trace{ [&](const InstructionTrace& event) {
		if (events.empty()) {
			observed_before_execution = cpu.read_pc() == 0u && cpu.read_register(1) == 0u;
		}
		events.push_back(event);
	} };

	cpu.step(bus, trace);
	assert(observed_before_execution);
	assert(events.size() == 1u);
	assert(events[0].pc == 0u);
	assert(events[0].instruction == addi_x1_x0_5);
	assert(cpu.read_pc() == 4u);
	assert(cpu.read_register(1) == 5u);

	bool breakpoint_thrown{};
	try {
		cpu.step(bus, trace);
	} catch (const Trap& trap) {
		breakpoint_thrown = true;
		assert(trap.cause == TrapCause::BreakPoint);
	}
	assert(breakpoint_thrown);
	assert(events.size() == 2u);
	assert(events[1].pc == 4u);
	assert(events[1].instruction == ebreak);

	/* A failed fetch has no instruction word to report. */
	Cpu fetch_fault_cpu{};
	fetch_fault_cpu.set_pc(16u);
	events.clear();
	bool fetch_fault_thrown{};
	try {
		fetch_fault_cpu.step(bus, trace);
	} catch (const Trap& trap) {
		fetch_fault_thrown = true;
		assert(trap.cause == TrapCause::InstructionAccessFault);
	}
	assert(fetch_fault_thrown);
	assert(events.empty());

	/* An interrupt is delivered before fetch, so it produces no instruction event. */
	constexpr std::uint16_t mstatus{ 0x300u };
	constexpr std::uint16_t mie{ 0x304u };
	constexpr std::uint32_t global_mie{ 1u << 3 };
	constexpr std::uint32_t mtie{ 1u << 7 };
	Cpu interrupt_cpu{};
	interrupt_cpu.write_csr(mstatus, global_mie);
	interrupt_cpu.write_csr(mie, mtie);
	interrupt_cpu.set_machine_timer_interrupt(true);
	events.clear();
	bool interrupt_thrown{};
	try {
		interrupt_cpu.step(bus, trace);
	} catch (const Trap& trap) {
		interrupt_thrown = true;
		assert(trap.cause == TrapCause::MachineTimerInterrupt);
	}
	assert(interrupt_thrown);
	assert(events.empty());

	return 0;
}
