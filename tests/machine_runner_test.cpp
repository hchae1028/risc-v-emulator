#include "bus.hpp"
#include "cpu.hpp"
#include "machine.hpp"
#include "memory.hpp"
#include "runner.hpp"
#include "timer_device.hpp"
#include "trap.hpp"
#include <cassert>
#include <cstdint>

namespace {

void configure_timer(TimerDevice& timer, std::uint32_t compare) {
	timer.write32(0x08u, compare);
	timer.write32(0x0Cu, 0u);
}

void enable_timer_interrupts(Cpu& cpu, std::uint32_t handler) {
	cpu.write_csr(0x305u, handler); // mtvec
	cpu.write_csr(0x300u, 1u << 3); // mstatus.MIE
	cpu.write_csr(0x304u, 1u << 7); // mie.MTIE
}

}

int main() {
	constexpr std::uint16_t mie{ 0x304u };
	constexpr std::uint16_t mepc{ 0x341u };
	constexpr std::uint16_t mcause{ 0x342u };
	constexpr std::uint16_t mip{ 0x344u };
	constexpr std::uint32_t mtip{ 1u << 7 };

	/* The runner crosses a timer trap, executes its handler, resumes, and halts. */
	Memory memory{ 64 };
	memory.write32(0, 0x00100093u);  // ADDI x1, x0, 1
	memory.write32(4, 0x00200113u);  // ADDI x2, x0, 2
	memory.write32(8, 0x00300193u);  // ADDI x3, x0, 3
	memory.write32(12, 0x00100073u); // EBREAK
	memory.write32(32, 0x30405073u); // CSRRWI x0, mie, 0
	memory.write32(36, 0x30200073u); // MRET
	Bus bus{ memory, 0 };
	TimerDevice timer{};
	configure_timer(timer, 3u);
	Cpu cpu{};
	enable_timer_interrupts(cpu, 32u);
	Machine machine{ cpu, bus, timer };

	auto result{ run_until_breakpoint(machine, 16) };
	assert(result.trap.has_value());
	assert(result.trap->cause == TrapCause::BreakPoint);
	assert(result.instructions_retired == 5);
	assert(timer.read32(0x00u) == 7u);
	assert(cpu.read_register(1) == 1u);
	assert(cpu.read_register(2) == 2u);
	assert(cpu.read_register(3) == 3u);
	assert(cpu.read_csr(mie) == 0u);
	assert(cpu.read_csr(mip) == mtip);
	assert(cpu.read_csr(mepc) == 12u);
	assert(cpu.read_csr(mcause) == 3u);
	assert(cpu.read_pc() == 32u);

	/* An exact budget may expire immediately after interrupt delivery. */
	Memory budget_memory{ 64 };
	budget_memory.write32(0, 0x00100093u);
	budget_memory.write32(4, 0x00200113u);
	budget_memory.write32(8, 0x00300193u);
	Bus budget_bus{ budget_memory, 0 };
	TimerDevice budget_timer{};
	configure_timer(budget_timer, 3u);
	Cpu budget_cpu{};
	enable_timer_interrupts(budget_cpu, 32u);
	Machine budget_machine{ budget_cpu, budget_bus, budget_timer };

	result = run_until_breakpoint(budget_machine, 3);
	assert(!result.trap.has_value());
	assert(result.instructions_retired == 2);
	assert(budget_timer.read32(0x00u) == 3u);
	assert(budget_cpu.read_pc() == 32u);
	assert(budget_cpu.read_csr(mepc) == 8u);
	assert(budget_cpu.read_csr(mcause) == 0x80000007u);

	/* An uncleared level repeatedly retriggers, but attempted-step budgeting terminates. */
	Memory storm_memory{ 64 };
	storm_memory.write32(0, 0x00100093u);  // never reached
	storm_memory.write32(32, 0x30200073u); // MRET without clearing MTIP
	Bus storm_bus{ storm_memory, 0 };
	TimerDevice storm_timer{};
	configure_timer(storm_timer, 1u);
	Cpu storm_cpu{};
	enable_timer_interrupts(storm_cpu, 32u);
	Machine storm_machine{ storm_cpu, storm_bus, storm_timer };

	result = run_until_breakpoint(storm_machine, 5);
	assert(!result.trap.has_value());
	assert(result.instructions_retired == 2);
	assert(storm_timer.read32(0x00u) == 5u);
	assert(storm_cpu.read_pc() == 32u);
	assert(storm_cpu.read_csr(mepc) == 0u);
	assert(storm_cpu.read_csr(mcause) == 0x80000007u);
	assert(storm_cpu.read_csr(mip) == mtip);
	assert(storm_cpu.read_register(1) == 0u);

	/* Without a trap, every attempted step retires normally. */
	Memory normal_memory{ 8 };
	normal_memory.write32(0, 0x00100093u);
	normal_memory.write32(4, 0x00200113u);
	Bus normal_bus{ normal_memory, 0 };
	TimerDevice normal_timer{};
	Cpu normal_cpu{};
	Machine normal_machine{ normal_cpu, normal_bus, normal_timer };
	result = run_until_breakpoint(normal_machine, 2);
	assert(!result.trap.has_value());
	assert(result.instructions_retired == 2);
	assert(normal_timer.read32(0x00u) == 2u);
	assert(normal_cpu.read_pc() == 8u);
	assert(normal_cpu.read_register(1) == 1u);
	assert(normal_cpu.read_register(2) == 2u);

	return 0;
}
