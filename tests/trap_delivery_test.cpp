#include "bus.hpp"
#include "cpu.hpp"
#include "memory.hpp"
#include "runner.hpp"
#include "trap.hpp"
#include <cassert>
#include <cstdint>

int main() {
	constexpr std::uint16_t mstatus{ 0x300u };
	constexpr std::uint16_t mtvec{ 0x305u };
	constexpr std::uint16_t mepc{ 0x341u };
	constexpr std::uint16_t mcause{ 0x342u };
	constexpr std::uint16_t mtval{ 0x343u };
	constexpr std::uint32_t mie{ 1u << 3 };
	constexpr std::uint32_t mpie{ 1u << 7 };
	constexpr std::uint32_t mpp_machine{ 0x00001800u };

	/* ECALL enters mtvec, reports to the host, and does not execute the handler yet. */
	Memory ecall_memory{ 64 };
	ecall_memory.write32(4, 0x00000073u);  // ECALL
	ecall_memory.write32(32, 0x00700293u); // ADDI x5, x0, 7
	Bus ecall_bus{ ecall_memory, 0 };
	Cpu ecall_cpu{};
	ecall_cpu.set_pc(4u);
	ecall_cpu.write_csr(mtvec, 32u);
	ecall_cpu.write_csr(mstatus, mie);
	auto result{ run_until_trap(ecall_cpu, ecall_bus, 1) };
	assert(result.trap.has_value());
	assert(result.trap->cause == TrapCause::EnvironmentCall);
	assert(result.trap->tval == 0u);
	assert(result.instructions_retired == 0);
	assert(ecall_cpu.read_pc() == 32u);
	assert(ecall_cpu.read_csr(mepc) == 4u);
	assert(ecall_cpu.read_csr(mcause) == 11u);
	assert(ecall_cpu.read_csr(mtval) == 0u);
	assert(ecall_cpu.read_csr(mstatus) == (mpp_machine | mpie));
	assert(ecall_cpu.read_register(5) == 0u);

	ecall_cpu.step(ecall_bus);
	assert(ecall_cpu.read_pc() == 36u);
	assert(ecall_cpu.read_register(5) == 7u);

	/* Illegal-instruction delivery preserves the raw word in mtval. */
	Memory illegal_memory{ 64 };
	illegal_memory.write32(8, 0xFFFFFFFFu);
	Bus illegal_bus{ illegal_memory, 0 };
	Cpu illegal_cpu{};
	illegal_cpu.set_pc(8u);
	illegal_cpu.write_csr(mtvec, 32u);
	result = run_until_trap(illegal_cpu, illegal_bus, 1);
	assert(result.trap.has_value());
	assert(result.trap->cause == TrapCause::IllegalInstruction);
	assert(result.instructions_retired == 0);
	assert(illegal_cpu.read_pc() == 32u);
	assert(illegal_cpu.read_csr(mepc) == 8u);
	assert(illegal_cpu.read_csr(mcause) == 2u);
	assert(illegal_cpu.read_csr(mtval) == 0xFFFFFFFFu);

	/* A data fault records the instruction PC separately from its effective address. */
	Memory load_memory{ 64 };
	load_memory.write32(12, 0x04002183u); // LW x3, 64(x0)
	Bus load_bus{ load_memory, 0 };
	Cpu load_cpu{};
	load_cpu.set_pc(12u);
	load_cpu.write_register(3, 0xCAFEBABEu);
	load_cpu.write_csr(mtvec, 32u);
	result = run_until_trap(load_cpu, load_bus, 1);
	assert(result.trap.has_value());
	assert(result.trap->cause == TrapCause::LoadAccessFault);
	assert(result.trap->tval == 64u);
	assert(result.instructions_retired == 0);
	assert(load_cpu.read_pc() == 32u);
	assert(load_cpu.read_csr(mepc) == 12u);
	assert(load_cpu.read_csr(mcause) == 5u);
	assert(load_cpu.read_csr(mtval) == 64u);
	assert(load_cpu.read_register(3) == 0xCAFEBABEu);

	/* A guest handler can inspect the trap, advance mepc, MRET, and resume. */
	Memory handler_memory{ 64 };
	handler_memory.write32(0, 0x00000073u);   // ECALL
	handler_memory.write32(4, 0x02A00313u);   // ADDI x6, x0, 42
	handler_memory.write32(8, 0x00100073u);   // EBREAK
	handler_memory.write32(32, 0x34202573u);  // CSRRS x10, mcause, x0
	handler_memory.write32(36, 0x343025F3u);  // CSRRS x11, mtval, x0
	handler_memory.write32(40, 0x341022F3u);  // CSRRS x5, mepc, x0
	handler_memory.write32(44, 0x00428293u);  // ADDI x5, x5, 4
	handler_memory.write32(48, 0x34129073u);  // CSRRW x0, mepc, x5
	handler_memory.write32(52, 0x30200073u);  // MRET
	Bus handler_bus{ handler_memory, 0 };
	Cpu handler_cpu{};
	handler_cpu.write_csr(mtvec, 32u);
	handler_cpu.write_csr(mstatus, mie);

	auto first_trap{ run_until_trap(handler_cpu, handler_bus, 1) };
	assert(first_trap.trap.has_value());
	assert(first_trap.trap->cause == TrapCause::EnvironmentCall);
	assert(first_trap.instructions_retired == 0);
	assert(handler_cpu.read_pc() == 32u);

	auto final_trap{ run_until_trap(handler_cpu, handler_bus, 16) };
	assert(final_trap.trap.has_value());
	assert(final_trap.trap->cause == TrapCause::BreakPoint);
	assert(final_trap.instructions_retired == 7);
	assert(handler_cpu.read_pc() == 32u);
	assert(handler_cpu.read_csr(mepc) == 8u);
	assert(handler_cpu.read_csr(mcause) == 3u);
	assert(handler_cpu.read_csr(mtval) == 8u);
	assert(handler_cpu.read_register(5) == 4u);
	assert(handler_cpu.read_register(6) == 42u);
	assert(handler_cpu.read_register(10) == 11u);
	assert(handler_cpu.read_register(11) == 0u);

	return 0;
}
