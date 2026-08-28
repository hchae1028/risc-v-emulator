#include "cpu.hpp"
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

	/* Trap entry saves precise metadata and moves an enabled MIE into MPIE. */
	Cpu enabled_cpu{};
	enabled_cpu.set_pc(0x104u);
	enabled_cpu.write_register(1, 0x12345678u);
	enabled_cpu.write_csr(mtvec, 0x203u);
	enabled_cpu.write_csr(mepc, 0xDEADBEEFu);
	enabled_cpu.write_csr(mcause, 0xFFFFFFFFu);
	enabled_cpu.write_csr(mtval, 0xAAAAAAAAu);
	enabled_cpu.write_csr(mstatus, mie);
	enabled_cpu.take_trap(Trap{
		.cause = TrapCause::LoadAccessFault,
		.tval = 0x40000000u
	});
	assert(enabled_cpu.read_pc() == 0x200u);
	assert(enabled_cpu.read_csr(mepc) == 0x104u);
	assert(enabled_cpu.read_csr(mcause) == 5u);
	assert(enabled_cpu.read_csr(mtval) == 0x40000000u);
	assert(enabled_cpu.read_csr(mstatus) == (mpp_machine | mpie));
	assert(enabled_cpu.read_register(1) == 0x12345678u);

	/* Trap entry overwrites an old MPIE with a disabled MIE. */
	Cpu disabled_cpu{};
	disabled_cpu.set_pc(0x80u);
	disabled_cpu.write_csr(mtvec, 0x40u);
	disabled_cpu.write_csr(mstatus, mpie);
	disabled_cpu.take_trap(Trap{
		.cause = TrapCause::EnvironmentCall,
		.tval = 0u
	});
	assert(disabled_cpu.read_pc() == 0x40u);
	assert(disabled_cpu.read_csr(mepc) == 0x80u);
	assert(disabled_cpu.read_csr(mcause) == 11u);
	assert(disabled_cpu.read_csr(mtval) == 0u);
	assert(disabled_cpu.read_csr(mstatus) == mpp_machine);

	/* Trap entry observes the same IALIGN=32 mepc WARL policy as CSR writes. */
	Cpu misaligned_cpu{};
	misaligned_cpu.set_pc(0x102u);
	misaligned_cpu.write_csr(mtvec, 0x44u);
	misaligned_cpu.take_trap(Trap{
		.cause = TrapCause::InstructionAddressMisaligned,
		.tval = 0x102u
	});
	assert(misaligned_cpu.read_pc() == 0x44u);
	assert(misaligned_cpu.read_csr(mepc) == 0x100u);
	assert(misaligned_cpu.read_csr(mcause) == 0u);
	assert(misaligned_cpu.read_csr(mtval) == 0x102u);

	return 0;
}
