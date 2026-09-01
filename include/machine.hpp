#ifndef MACHINE_H_
#define MACHINE_H_

#include "cpu.hpp"
#include "bus.hpp"
#include "timer_device.hpp"
#include "instruction_trace.hpp"

class Machine {
private:
	Cpu& m_cpu;
	Bus& m_bus;
	TimerDevice& m_timer;

public:
	Machine(Cpu& cpu, Bus& bus, TimerDevice& timer);

	void step(const InstructionTraceCallBack& trace = {});
};

#endif
