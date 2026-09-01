#include "machine.hpp"
#include "cpu.hpp"
#include "timer_device.hpp"
#include "instruction_trace.hpp"

Machine::Machine(Cpu& cpu, Bus& bus, TimerDevice& timer)
	: m_cpu{ cpu },
	  m_bus{ bus },
	  m_timer{ timer }
{
}

void Machine::step(const InstructionTraceCallBack& trace) {
	m_timer.tick();
	auto pending{ m_timer.interrupt_pending() };

	m_cpu.set_machine_timer_interrupt(pending);
	m_cpu.step(m_bus, trace);
}
