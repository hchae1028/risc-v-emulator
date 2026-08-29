#include "bus.hpp"
#include "cpu.hpp"
#include "memory.hpp"
#include "runner.hpp"
#include "trap.hpp"
#include <cassert>
#include <cstdint>

int main() {
	constexpr std::uint16_t mstatus{ 0x300u };
	constexpr std::uint16_t mie{ 0x304u };
	constexpr std::uint16_t mtvec{ 0x305u };
	constexpr std::uint16_t mepc{ 0x341u };
	constexpr std::uint16_t mcause{ 0x342u };
	constexpr std::uint16_t mtval{ 0x343u };
	constexpr std::uint16_t mip{ 0x344u };
	constexpr std::uint32_t global_mie{ 1u << 3 };
	constexpr std::uint32_t mpie{ 1u << 7 };
	constexpr std::uint32_t mtie{ 1u << 7 };
	constexpr std::uint32_t mtip{ 1u << 7 };
	constexpr std::uint32_t mpp_machine{ 0x00001800u };
	constexpr std::uint32_t timer_mcause{ 0x80000007u };

	Memory memory{ 64 };
	memory.write32(0, 0x00100093u);  // ADDI x1, x0, 1
	memory.write32(32, 0x00300193u); // ADDI x3, x0, 3
	memory.write32(36, 0x30200073u); // MRET
	Bus bus{ memory, 0 };

	/* Pending alone is insufficient when either enable layer is clear. */
	Cpu global_disabled_cpu{};
	global_disabled_cpu.write_csr(mie, mtie);
	global_disabled_cpu.set_machine_timer_interrupt(true);
	global_disabled_cpu.step(bus);
	assert(global_disabled_cpu.read_pc() == 4u);
	assert(global_disabled_cpu.read_register(1) == 1u);
	assert(global_disabled_cpu.read_csr(mip) == mtip);

	Cpu local_disabled_cpu{};
	local_disabled_cpu.write_csr(mstatus, global_mie);
	local_disabled_cpu.set_machine_timer_interrupt(true);
	local_disabled_cpu.step(bus);
	assert(local_disabled_cpu.read_pc() == 4u);
	assert(local_disabled_cpu.read_register(1) == 1u);
	assert(local_disabled_cpu.read_csr(mip) == mtip);

	/* With all three bits set, delivery occurs before fetch and retirement. */
	Cpu enabled_cpu{};
	enabled_cpu.write_csr(mtvec, 32u);
	enabled_cpu.write_csr(mstatus, global_mie);
	enabled_cpu.write_csr(mie, mtie);
	enabled_cpu.set_machine_timer_interrupt(true);
	auto result{ run_until_trap(enabled_cpu, bus, 1) };
	assert(result.trap.has_value());
	assert(result.trap->cause == TrapCause::MachineTimerInterrupt);
	assert(result.trap->tval == 0u);
	assert(result.instructions_retired == 0);
	assert(enabled_cpu.read_pc() == 32u);
	assert(enabled_cpu.read_csr(mepc) == 0u);
	assert(enabled_cpu.read_csr(mcause) == timer_mcause);
	assert(enabled_cpu.read_csr(mtval) == 0u);
	assert(enabled_cpu.read_csr(mstatus) == (mpp_machine | mpie));
	assert(enabled_cpu.read_csr(mip) == mtip);
	assert(enabled_cpu.read_register(1) == 0u);
	assert(enabled_cpu.read_register(3) == 0u);

	/* MIE=0 lets the handler run even while MTIP remains asserted. */
	enabled_cpu.step(bus);
	assert(enabled_cpu.read_pc() == 36u);
	assert(enabled_cpu.read_register(3) == 3u);
	assert(enabled_cpu.read_csr(mip) == mtip);

	/* MRET restores MIE; an uncleared level retriggers before resumed execution. */
	enabled_cpu.step(bus);
	assert(enabled_cpu.read_pc() == 0u);
	assert(enabled_cpu.read_csr(mstatus) == (mpp_machine | global_mie | mpie));
	result = run_until_trap(enabled_cpu, bus, 1);
	assert(result.trap.has_value());
	assert(result.trap->cause == TrapCause::MachineTimerInterrupt);
	assert(result.instructions_retired == 0);
	assert(enabled_cpu.read_pc() == 32u);
	assert(enabled_cpu.read_csr(mepc) == 0u);
	assert(enabled_cpu.read_register(1) == 0u);

	/* Clearing the external line permits MRET to resume the interrupted code. */
	enabled_cpu.set_machine_timer_interrupt(false);
	enabled_cpu.step(bus); // handler ADDI
	enabled_cpu.step(bus); // MRET
	assert(enabled_cpu.read_pc() == 0u);
	enabled_cpu.step(bus); // interrupted ADDI
	assert(enabled_cpu.read_pc() == 4u);
	assert(enabled_cpu.read_register(1) == 1u);
	assert(enabled_cpu.read_csr(mip) == 0u);

	return 0;
}
