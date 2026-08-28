#include "bus.hpp"
#include "cpu.hpp"
#include "decoder.hpp"
#include "executor.hpp"
#include "memory.hpp"
#include "trap.hpp"
#include <cassert>
#include <cstdint>

namespace {

constexpr std::uint32_t encode_csr(std::uint16_t csr, std::uint8_t source,
								   std::uint8_t funct3, std::uint8_t rd) {
	return (static_cast<std::uint32_t>(csr) << 20)
		| (static_cast<std::uint32_t>(source) << 15)
		| (static_cast<std::uint32_t>(funct3) << 12)
		| (static_cast<std::uint32_t>(rd) << 7)
		| 0x73u;
}

void assert_illegal_instruction(Cpu& cpu, const Instruction& instruction, Bus& bus) {
	bool trap_thrown{ false };
	try {
		static_cast<void>(execute_instruction(cpu, instruction, bus));
	} catch (const Trap& trap) {
		trap_thrown = true;
		assert(trap.cause == TrapCause::IllegalInstruction);
	}
	assert(trap_thrown);
}

}

int main() {
	constexpr std::uint16_t mstatus{ 0x300u };
	constexpr std::uint16_t mtvec{ 0x305u };
	constexpr std::uint16_t mepc{ 0x341u };
	constexpr std::uint16_t mcause{ 0x342u };
	constexpr std::uint16_t mtval{ 0x343u };

	/* All six Zicsr encodings preserve the raw 12-bit CSR field */
	auto instruction{ decode_instruction(encode_csr(0xFFFu, 1, 0x1u, 2)) };
	assert(instruction.csr == 0xFFFu);
	assert(decode_operation(instruction) == Operation::Csrrw);
	assert(decode_operation(decode_instruction(encode_csr(mtvec, 1, 0x2u, 2))) == Operation::Csrrs);
	assert(decode_operation(decode_instruction(encode_csr(mtvec, 1, 0x3u, 2))) == Operation::Csrrc);
	assert(decode_operation(decode_instruction(encode_csr(mtvec, 1, 0x5u, 2))) == Operation::CsrrwI);
	assert(decode_operation(decode_instruction(encode_csr(mtvec, 1, 0x6u, 2))) == Operation::CsrrsI);
	assert(decode_operation(decode_instruction(encode_csr(mtvec, 1, 0x7u, 2))) == Operation::CsrrcI);
	assert(decode_operation(decode_instruction(encode_csr(mtvec, 1, 0x4u, 2))) == Operation::Unknown);

	Memory memory{ 16 };
	Bus bus{ memory, 0 };

	/* CSRRW returns the old value and writes the register source */
	Cpu swap_cpu{};
	swap_cpu.set_pc(0x80u);
	swap_cpu.write_csr(mtvec, 0x100u);
	swap_cpu.write_register(1, 0x203u);
	instruction = decode_instruction(encode_csr(mtvec, 1, 0x1u, 2));
	static_cast<void>(execute_instruction(swap_cpu, instruction, bus));
	assert(swap_cpu.read_register(2) == 0x100u);
	assert(swap_cpu.read_register(1) == 0x203u);
	assert(swap_cpu.read_csr(mtvec) == 0x200u);
	assert(swap_cpu.read_pc() == 0x80u);

	// rd=x0 suppresses the read but not the write
	swap_cpu.write_register(1, 0x304u);
	instruction = decode_instruction(encode_csr(mtvec, 1, 0x1u, 0));
	static_cast<void>(execute_instruction(swap_cpu, instruction, bus));
	assert(swap_cpu.read_csr(mtvec) == 0x304u);
	assert(swap_cpu.read_register(0) == 0);

	// rs1=x0 is still a write of zero for CSRRW
	instruction = decode_instruction(encode_csr(mtvec, 0, 0x1u, 3));
	static_cast<void>(execute_instruction(swap_cpu, instruction, bus));
	assert(swap_cpu.read_register(3) == 0x304u);
	assert(swap_cpu.read_csr(mtvec) == 0);

	/* CSRRS and CSRRC perform read-modify-write and return the old value */
	Cpu mask_cpu{};
	mask_cpu.write_csr(mtval, 0x00000F00u);
	mask_cpu.write_register(1, 0x000000F0u);
	instruction = decode_instruction(encode_csr(mtval, 1, 0x2u, 2));
	static_cast<void>(execute_instruction(mask_cpu, instruction, bus));
	assert(mask_cpu.read_register(2) == 0x00000F00u);
	assert(mask_cpu.read_csr(mtval) == 0x00000FF0u);

	mask_cpu.write_register(1, 0x000000CCu);
	instruction = decode_instruction(encode_csr(mtval, 1, 0x3u, 3));
	static_cast<void>(execute_instruction(mask_cpu, instruction, bus));
	assert(mask_cpu.read_register(3) == 0x00000FF0u);
	assert(mask_cpu.read_csr(mtval) == 0x00000F30u);

	// rs1=x0 performs a read without writing the CSR
	instruction = decode_instruction(encode_csr(mtval, 0, 0x2u, 4));
	static_cast<void>(execute_instruction(mask_cpu, instruction, bus));
	assert(mask_cpu.read_register(4) == 0x00000F30u);
	assert(mask_cpu.read_csr(mtval) == 0x00000F30u);

	/* Immediate forms use the zero-extended five-bit source field */
	Cpu immediate_cpu{};
	immediate_cpu.write_csr(mcause, 0x80000007u);
	instruction = decode_instruction(encode_csr(mcause, 5, 0x5u, 1));
	static_cast<void>(execute_instruction(immediate_cpu, instruction, bus));
	assert(immediate_cpu.read_register(1) == 0x80000007u);
	assert(immediate_cpu.read_csr(mcause) == 5);

	instruction = decode_instruction(encode_csr(mcause, 2, 0x6u, 2));
	static_cast<void>(execute_instruction(immediate_cpu, instruction, bus));
	assert(immediate_cpu.read_register(2) == 5);
	assert(immediate_cpu.read_csr(mcause) == 7);

	instruction = decode_instruction(encode_csr(mcause, 1, 0x7u, 3));
	static_cast<void>(execute_instruction(immediate_cpu, instruction, bus));
	assert(immediate_cpu.read_register(3) == 7);
	assert(immediate_cpu.read_csr(mcause) == 6);

	// Zero masks read without writing, while CSRRWI still writes zero
	instruction = decode_instruction(encode_csr(mcause, 0, 0x6u, 4));
	static_cast<void>(execute_instruction(immediate_cpu, instruction, bus));
	assert(immediate_cpu.read_register(4) == 6);
	assert(immediate_cpu.read_csr(mcause) == 6);
	instruction = decode_instruction(encode_csr(mcause, 0, 0x5u, 0));
	static_cast<void>(execute_instruction(immediate_cpu, instruction, bus));
	assert(immediate_cpu.read_csr(mcause) == 0);

	/* Zicsr writes to mstatus obey its writable-bit and hardwired-field policy */
	Cpu status_cpu{};
	status_cpu.write_register(1, 0xFFFFFFFFu);
	instruction = decode_instruction(encode_csr(mstatus, 1, 0x1u, 2));
	static_cast<void>(execute_instruction(status_cpu, instruction, bus));
	assert(status_cpu.read_register(2) == 0x00001800u);
	assert(status_cpu.read_csr(mstatus) == 0x00001888u);

	instruction = decode_instruction(encode_csr(mstatus, 0, 0x5u, 3));
	static_cast<void>(execute_instruction(status_cpu, instruction, bus));
	assert(status_cpu.read_register(3) == 0x00001888u);
	assert(status_cpu.read_csr(mstatus) == 0x00001800u);

	/* Unsupported CSR accesses become precise illegal-instruction traps */
	Cpu failed_cpu{};
	failed_cpu.set_pc(0x40u);
	failed_cpu.write_register(1, 0xAAAAAAAAu);
	failed_cpu.write_register(2, 0xDEADBEEFu);
	failed_cpu.write_csr(mtvec, 0x100u);
	failed_cpu.write_csr(mepc, 0x200u);
	failed_cpu.write_csr(mcause, 3);
	failed_cpu.write_csr(mtval, 0x12345678u);
	instruction = decode_instruction(encode_csr(0xFFFu, 1, 0x1u, 2));
	assert_illegal_instruction(failed_cpu, instruction, bus);
	assert(failed_cpu.read_pc() == 0x40u);
	assert(failed_cpu.read_register(1) == 0xAAAAAAAAu);
	assert(failed_cpu.read_register(2) == 0xDEADBEEFu);
	assert(failed_cpu.read_csr(mtvec) == 0x100u);
	assert(failed_cpu.read_csr(mepc) == 0x200u);
	assert(failed_cpu.read_csr(mcause) == 3);
	assert(failed_cpu.read_csr(mtval) == 0x12345678u);

	// rd=x0 suppresses a CSRRW read, but its unsupported write still traps
	instruction = decode_instruction(encode_csr(0xFFFu, 1, 0x1u, 0));
	assert_illegal_instruction(failed_cpu, instruction, bus);
	assert(failed_cpu.read_register(0) == 0);

	/* CSR instructions advance normally through the fetch/decode/execute pipeline */
	Cpu step_cpu{};
	Memory step_memory{ 16 };
	Bus step_bus{ step_memory, 0 };
	step_memory.write32(0, encode_csr(mtval, 5, 0x5u, 1)); // CSRRWI x1, mtval, 5
	step_memory.write32(4, encode_csr(mtval, 2, 0x6u, 2)); // CSRRSI x2, mtval, 2
	step_memory.write32(8, encode_csr(mtval, 1, 0x7u, 3)); // CSRRCI x3, mtval, 1
	step_memory.write32(12, 0x00100073u);                  // EBREAK
	step_cpu.step(step_bus);
	step_cpu.step(step_bus);
	step_cpu.step(step_bus);
	assert(step_cpu.read_pc() == 12);
	assert(step_cpu.read_register(1) == 0);
	assert(step_cpu.read_register(2) == 5);
	assert(step_cpu.read_register(3) == 7);
	assert(step_cpu.read_csr(mtval) == 6);

	bool breakpoint_thrown{ false };
	try {
		step_cpu.step(step_bus);
	} catch (const Trap& trap) {
		breakpoint_thrown = true;
		assert(trap.cause == TrapCause::BreakPoint);
	}
	assert(breakpoint_thrown);
	assert(step_cpu.read_pc() == 0);
	assert(step_cpu.read_csr(mepc) == 12u);

	return 0;
}
