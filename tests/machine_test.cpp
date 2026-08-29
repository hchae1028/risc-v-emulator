#include "bus.hpp"
#include "cpu.hpp"
#include "machine.hpp"
#include "memory.hpp"
#include "timer_device.hpp"
#include "trap.hpp"
#include <cassert>
#include <cstdint>

int main() {
	constexpr std::uint16_t mstatus{ 0x300u };
	constexpr std::uint16_t mie{ 0x304u };
	constexpr std::uint16_t mtvec{ 0x305u };
	constexpr std::uint16_t mepc{ 0x341u };
	constexpr std::uint16_t mcause{ 0x342u };
	constexpr std::uint16_t mip{ 0x344u };
	constexpr std::uint32_t global_mie{ 1u << 3 };
	constexpr std::uint32_t mtie{ 1u << 7 };
	constexpr std::uint32_t mtip{ 1u << 7 };

	Memory memory{ 64 };
	memory.write32(0, 0x00100093u);  // ADDI x1, x0, 1
	memory.write32(4, 0x00200113u);  // ADDI x2, x0, 2
	memory.write32(8, 0x00300193u);  // ADDI x3, x0, 3
	memory.write32(32, 0x00400213u); // ADDI x4, x0, 4 (handler)
	Bus bus{ memory, 0 };
	TimerDevice timer{};
	timer.write32(0x08u, 3u);
	timer.write32(0x0Cu, 0u);
	Cpu cpu{};
	cpu.write_csr(mtvec, 32u);
	cpu.write_csr(mstatus, global_mie);
	cpu.write_csr(mie, mtie);
	Machine machine{ cpu, bus, timer };

	/* One deterministic timer tick precedes each attempted CPU step. */
	machine.step();
	assert(timer.read32(0x00u) == 1u);
	assert(cpu.read_pc() == 4u);
	assert(cpu.read_register(1) == 1u);
	assert(cpu.read_csr(mip) == 0u);

	machine.step();
	assert(timer.read32(0x00u) == 2u);
	assert(cpu.read_pc() == 8u);
	assert(cpu.read_register(2) == 2u);
	assert(cpu.read_csr(mip) == 0u);

	bool interrupt_thrown{ false };
	try {
		machine.step();
	} catch (const Trap& trap) {
		interrupt_thrown = true;
		assert(trap.cause == TrapCause::MachineTimerInterrupt);
		assert(trap.tval == 0u);
	}
	assert(interrupt_thrown);
	assert(timer.read32(0x00u) == 3u);
	assert(cpu.read_pc() == 32u);
	assert(cpu.read_csr(mepc) == 8u);
	assert(cpu.read_csr(mcause) == 0x80000007u);
	assert(cpu.read_csr(mip) == mtip);
	assert(cpu.read_register(3) == 0u);

	/* The timer continues while MIE=0 lets the handler execute. */
	machine.step();
	assert(timer.read32(0x00u) == 4u);
	assert(cpu.read_pc() == 36u);
	assert(cpu.read_register(4) == 4u);
	assert(cpu.read_csr(mip) == mtip);

	/* Raising mtimecmp is sampled on the following machine boundary. */
	timer.write32(0x08u, 10u);
	timer.write32(0x0Cu, 0u);
	memory.write32(36, 0x00500293u); // ADDI x5, x0, 5
	machine.step();
	assert(timer.read32(0x00u) == 5u);
	assert(cpu.read_csr(mip) == 0u);
	assert(cpu.read_pc() == 40u);
	assert(cpu.read_register(5) == 5u);

	/* Pending state is sampled even when interrupt delivery is disabled. */
	Memory disabled_memory{ 8 };
	disabled_memory.write32(0, 0x00700313u); // ADDI x6, x0, 7
	Bus disabled_bus{ disabled_memory, 0 };
	TimerDevice disabled_timer{};
	disabled_timer.write32(0x08u, 1u);
	disabled_timer.write32(0x0Cu, 0u);
	Cpu disabled_cpu{};
	Machine disabled_machine{ disabled_cpu, disabled_bus, disabled_timer };
	disabled_machine.step();
	assert(disabled_timer.read32(0x00u) == 1u);
	assert(disabled_cpu.read_csr(mip) == mtip);
	assert(disabled_cpu.read_pc() == 4u);
	assert(disabled_cpu.read_register(6) == 7u);

	return 0;
}
