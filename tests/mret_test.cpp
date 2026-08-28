#include "bus.hpp"
#include "cpu.hpp"
#include "decoder.hpp"
#include "executor.hpp"
#include "memory.hpp"
#include "trap.hpp"
#include <cassert>
#include <cstdint>

namespace {

constexpr std::uint16_t mstatus{ 0x300u };
constexpr std::uint16_t mepc{ 0x341u };
constexpr std::uint32_t mstatus_mie{ 1u << 3 };
constexpr std::uint32_t mstatus_mpie{ 1u << 7 };
constexpr std::uint32_t mstatus_mpp_machine{ 0x00001800u };
constexpr std::uint32_t mret{ 0x30200073u };

}

int main() {
	/* MRET has one exact encoding within the SYSTEM opcode. */
	assert(decode_operation(decode_instruction(mret)) == Operation::Mret);
	assert(decode_operation(decode_instruction(0x302000F3u)) == Operation::Unknown); // rd != x0
	assert(decode_operation(decode_instruction(0x30208073u)) == Operation::Unknown); // rs1 != x0
	assert(decode_operation(decode_instruction(0x30300073u)) == Operation::Unknown); // funct12 differs

	Memory memory{ 8 };
	Bus bus{ memory, 0 };

	/* Old MPIE=0 clears MIE, sets MPIE, and returns the aligned mepc. */
	Cpu disabled_cpu{};
	disabled_cpu.set_pc(0x20u);
	disabled_cpu.write_register(1, 0x12345678u);
	disabled_cpu.write_csr(mstatus, mstatus_mie);
	disabled_cpu.write_csr(mepc, 0x43u);
	auto target{ execute_instruction(disabled_cpu, decode_instruction(mret), bus) };
	assert(target.has_value());
	assert(*target == 0x40u);
	assert(disabled_cpu.read_pc() == 0x20u);
	assert(disabled_cpu.read_csr(mstatus) == (mstatus_mpp_machine | mstatus_mpie));
	assert(disabled_cpu.read_csr(mepc) == 0x40u);
	assert(disabled_cpu.read_register(1) == 0x12345678u);

	/* Old MPIE=1 sets MIE and leaves MPIE set. */
	Cpu enabled_cpu{};
	enabled_cpu.write_csr(mstatus, mstatus_mpie);
	enabled_cpu.write_csr(mepc, 0x80u);
	target = execute_instruction(enabled_cpu, decode_instruction(mret), bus);
	assert(target.has_value());
	assert(*target == 0x80u);
	assert(enabled_cpu.read_csr(mstatus)
		== (mstatus_mpp_machine | mstatus_mie | mstatus_mpie));

	/* Cpu::step installs the returned target; a near-match traps precisely. */
	memory.write32(0, mret);
	memory.write32(4, 0x302000F3u);
	Cpu step_cpu{};
	step_cpu.write_csr(mepc, 4u);
	step_cpu.step(bus);
	assert(step_cpu.read_pc() == 4u);
	assert(step_cpu.read_csr(mstatus) == (mstatus_mpp_machine | mstatus_mpie));

	bool illegal_thrown{ false };
	try {
		step_cpu.step(bus);
	} catch (const Trap& trap) {
		illegal_thrown = true;
		assert(trap.cause == TrapCause::IllegalInstruction);
	}
	assert(illegal_thrown);
	assert(step_cpu.read_pc() == 0u);
	assert(step_cpu.read_csr(mepc) == 4u);

	return 0;
}
