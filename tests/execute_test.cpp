#include "bus.hpp"
#include "decoder.hpp"
#include "executor.hpp"
#include "memory.hpp"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace {

auto execute_with_ram(Cpu& cpu, const Instruction& instruction, Memory& ram) {
	Bus bus{ ram, 0 };
	return execute_instruction(cpu, instruction, bus);
}

}

int main() {
	Memory memory{ 64 };

	/* ADD instruction tests */
	// ADD x3, x1, x2 test
	Cpu cpu{};
	cpu.write_register(1, 5);
	cpu.write_register(2, 7);

	Instruction instr = decode_instruction(0x002081B3);
	execute_with_ram(cpu, instr, memory);

	assert(cpu.read_register(3) == 12);

	// Source registers remain unchanged
	assert(cpu.read_register(1) == 5);
	assert(cpu.read_register(2) == 7);
	
	// Unrelated registers stay unchanged
	for (std::size_t i = 0; i < 32; i++) {
		if (i == 1 || i == 2 || i == 3) {
			continue;
		}
		assert(cpu.read_register(i) == 0);
	}

	// x0 as a source register
	instr = decode_instruction(0x002001B3);
	execute_with_ram(cpu, instr, memory);

	assert(cpu.read_register(3) == cpu.read_register(2));

	// x0 as rd
	instr = decode_instruction(0x00208033);
	execute_with_ram(cpu, instr, memory);

	assert(cpu.read_register(0) == 0);

	// rd == rs1
	instr = decode_instruction(0x002080B3);
	execute_with_ram(cpu, instr, memory);

	assert(cpu.read_register(1) == 12);

	// rd == rs2
	cpu.write_register(1, 5);
	cpu.write_register(2, 7);
	instr = decode_instruction(0x00208133);
	execute_with_ram(cpu, instr, memory);

	assert(cpu.read_register(2) == 12);

	// 0xFFFFFFFF + 1 produces 0
	cpu.write_register(1, 0xFFFFFFFF);
	cpu.write_register(2, 1);
	instr = decode_instruction(0x002081B3);
	execute_with_ram(cpu, instr, memory);

	assert(cpu.read_register(3) == 0);

	// PC remains unchanged
	assert(cpu.read_pc() == 0);

	// Unknown instruction produces an illegal-instruction trap
	bool illegal_trap_thrown{ false };
	TrapCause illegal_trap_cause{ TrapCause::BreakPoint };
	instr = decode_instruction(0xFFFFFFFF);
	try {
		execute_with_ram(cpu, instr, memory);
	} catch (const Trap& trap) {
		illegal_trap_thrown = true;
		illegal_trap_cause = trap.cause;
	}
	assert(illegal_trap_thrown);
	assert(illegal_trap_cause == TrapCause::IllegalInstruction);
	assert(cpu.read_register(1) == 0xFFFFFFFF);
	assert(cpu.read_register(2) == 1);
	assert(cpu.read_register(3) == 0);
	assert(cpu.read_pc() == 0);

	/* ADDI instruction tests */
	Cpu addi_cpu{};

	// ADDI x1, x0, 5
	instr = decode_instruction(0x00500093);
	execute_with_ram(addi_cpu, instr, memory);
	assert(addi_cpu.read_register(1) == 5);

	// ADDI x2, x0, 7
	instr = decode_instruction(0x00700113);
	execute_with_ram(addi_cpu, instr, memory);
	assert(addi_cpu.read_register(2) == 7);

	// ADD x3, x1, x2
	instr = decode_instruction(0x002081B3);
	execute_with_ram(addi_cpu, instr, memory);
	assert(addi_cpu.read_register(1) == 5);
	assert(addi_cpu.read_register(2) == 7);
	assert(addi_cpu.read_register(3) == 12);

	// Negative immediate: ADDI x3, x1, -1
	instr = decode_instruction(0xFFF08193);
	execute_with_ram(addi_cpu, instr, memory);
	assert(addi_cpu.read_register(1) == 5);
	assert(addi_cpu.read_register(3) == 4);

	// Largest positive 12-bit immediate
	instr = decode_instruction(0x7FF00213);
	execute_with_ram(addi_cpu, instr, memory);
	assert(addi_cpu.read_register(4) == 0x000007FFu);

	// Smallest negative 12-bit immediate
	instr = decode_instruction(0x80000293);
	execute_with_ram(addi_cpu, instr, memory);
	assert(addi_cpu.read_register(5) == 0xFFFFF800u);

	// Unsigned wraparound: 0xFFFFFFFF + 1 produces 0
	addi_cpu.write_register(6, 0xFFFFFFFFu);
	instr = decode_instruction(0x00130393);
	execute_with_ram(addi_cpu, instr, memory);
	assert(addi_cpu.read_register(6) == 0xFFFFFFFFu);
	assert(addi_cpu.read_register(7) == 0);

	// rd == rs1
	instr = decode_instruction(0x00108093);
	execute_with_ram(addi_cpu, instr, memory);
	assert(addi_cpu.read_register(1) == 6);

	// x0 as destination remains zero
	instr = decode_instruction(0x00108013);
	execute_with_ram(addi_cpu, instr, memory);
	assert(addi_cpu.read_register(0) == 0);

	// PC remains unchanged
	assert(addi_cpu.read_pc() == 0);

	/* SUB instruction tests */
	Cpu sub_cpu{};

	sub_cpu.write_register(1, 7);
	sub_cpu.write_register(2, 5);

	// SUB x3, x1, x2
	instr = decode_instruction(0x402081B3);
	execute_with_ram(sub_cpu, instr, memory);
	assert(sub_cpu.read_register(3) == 2);
	assert(sub_cpu.read_register(1) == 7);
	assert(sub_cpu.read_register(2) == 5);

	// Unsigned underflow: 5 - 7 produces 0xFFFFFFFE
	sub_cpu.write_register(1, 5);
	sub_cpu.write_register(2, 7);
	execute_with_ram(sub_cpu, instr, memory);
	assert(sub_cpu.read_register(3) == 0xFFFFFFFEu);

	// Unsigned underflow: 0 - 1 produces 0xFFFFFFFF
	sub_cpu.write_register(1, 0);
	sub_cpu.write_register(2, 1);
	execute_with_ram(sub_cpu, instr, memory);
	assert(sub_cpu.read_register(3) == 0xFFFFFFFFu);

	// rs1 == rs2 produces zero
	sub_cpu.write_register(1, 123);
	instr = decode_instruction(0x401081B3);
	execute_with_ram(sub_cpu, instr, memory);
	assert(sub_cpu.read_register(3) == 0);

	// rd == rs1
	sub_cpu.write_register(1, 7);
	sub_cpu.write_register(2, 5);
	instr = decode_instruction(0x402080B3);
	execute_with_ram(sub_cpu, instr, memory);
	assert(sub_cpu.read_register(1) == 2);

	// rd == rs2
	sub_cpu.write_register(1, 7);
	sub_cpu.write_register(2, 5);
	instr = decode_instruction(0x40208133);
	execute_with_ram(sub_cpu, instr, memory);
	assert(sub_cpu.read_register(2) == 2);

	// x0 as destination remains zero
	instr = decode_instruction(0x40208033);
	execute_with_ram(sub_cpu, instr, memory);
	assert(sub_cpu.read_register(0) == 0);

	// PC remains unchanged
	assert(sub_cpu.read_pc() == 0);

	/* Register-register bitwise instruction tests */
	Cpu bitwise_cpu{};

	bitwise_cpu.write_register(1, 0xCC33CC33u);
	bitwise_cpu.write_register(2, 0x0F0FF0F0u);

	// XOR x3, x1, x2
	instr = decode_instruction(0x0020C1B3);
	execute_with_ram(bitwise_cpu, instr, memory);
	assert(bitwise_cpu.read_register(3) == 0xC33C3CC3u);

	// OR x4, x1, x2
	instr = decode_instruction(0x0020E233);
	execute_with_ram(bitwise_cpu, instr, memory);
	assert(bitwise_cpu.read_register(4) == 0xCF3FFCF3u);

	// AND x5, x1, x2
	instr = decode_instruction(0x0020F2B3);
	execute_with_ram(bitwise_cpu, instr, memory);
	assert(bitwise_cpu.read_register(5) == 0x0C03C030u);

	// Source registers remain unchanged
	assert(bitwise_cpu.read_register(1) == 0xCC33CC33u);
	assert(bitwise_cpu.read_register(2) == 0x0F0FF0F0u);

	// rd == rs1
	instr = decode_instruction(0x0020C0B3);
	execute_with_ram(bitwise_cpu, instr, memory);
	assert(bitwise_cpu.read_register(1) == 0xC33C3CC3u);

	// rd == rs2
	bitwise_cpu.write_register(1, 0xCC33CC33u);
	bitwise_cpu.write_register(2, 0x0F0FF0F0u);
	instr = decode_instruction(0x0020E133);
	execute_with_ram(bitwise_cpu, instr, memory);
	assert(bitwise_cpu.read_register(2) == 0xCF3FFCF3u);

	// x0 as destination remains zero
	instr = decode_instruction(0x0020F033);
	execute_with_ram(bitwise_cpu, instr, memory);
	assert(bitwise_cpu.read_register(0) == 0);

	// PC remains unchanged
	assert(bitwise_cpu.read_pc() == 0);

	/* Immediate bitwise instruction tests */
	Cpu immediate_bitwise_cpu{};

	immediate_bitwise_cpu.write_register(1, 0xCC33CC33u);

	// XORI x3, x1, -1
	instr = decode_instruction(0xFFF0C193);
	execute_with_ram(immediate_bitwise_cpu, instr, memory);
	assert(immediate_bitwise_cpu.read_register(3) == 0x33CC33CCu);

	// ORI x4, x1, 0xF0
	instr = decode_instruction(0x0F00E213);
	execute_with_ram(immediate_bitwise_cpu, instr, memory);
	assert(immediate_bitwise_cpu.read_register(4) == 0xCC33CCF3u);

	// ANDI x5, x1, -256
	instr = decode_instruction(0xF000F293);
	execute_with_ram(immediate_bitwise_cpu, instr, memory);
	assert(immediate_bitwise_cpu.read_register(5) == 0xCC33CC00u);

	// Source register remains unchanged
	assert(immediate_bitwise_cpu.read_register(1) == 0xCC33CC33u);

	// rd == rs1
	instr = decode_instruction(0xFFF0C093);
	execute_with_ram(immediate_bitwise_cpu, instr, memory);
	assert(immediate_bitwise_cpu.read_register(1) == 0x33CC33CCu);

	// x0 as a source register
	instr = decode_instruction(0x0F006313);
	execute_with_ram(immediate_bitwise_cpu, instr, memory);
	assert(immediate_bitwise_cpu.read_register(6) == 0x000000F0u);

	// x0 as destination remains zero
	instr = decode_instruction(0xF000F013);
	execute_with_ram(immediate_bitwise_cpu, instr, memory);
	assert(immediate_bitwise_cpu.read_register(0) == 0);

	// PC remains unchanged
	assert(immediate_bitwise_cpu.read_pc() == 0);

	/* Register-register shift instruction tests */
	Cpu shift_cpu{};

	shift_cpu.write_register(1, 0x80000001u);
	shift_cpu.write_register(2, 4);

	// SLL x3, x1, x2
	instr = decode_instruction(0x002091B3);
	execute_with_ram(shift_cpu, instr, memory);
	assert(shift_cpu.read_register(3) == 0x00000010u);

	// SRL x4, x1, x2
	instr = decode_instruction(0x0020D233);
	execute_with_ram(shift_cpu, instr, memory);
	assert(shift_cpu.read_register(4) == 0x08000000u);

	// SRA x5, x1, x2
	instr = decode_instruction(0x4020D2B3);
	execute_with_ram(shift_cpu, instr, memory);
	assert(shift_cpu.read_register(5) == 0xF8000000u);

	// Source registers remain unchanged
	assert(shift_cpu.read_register(1) == 0x80000001u);
	assert(shift_cpu.read_register(2) == 4);

	// Only the low five bits of the shift register are used: 36 becomes 4
	shift_cpu.write_register(2, 36);
	instr = decode_instruction(0x00209333); // SLL x6, x1, x2
	execute_with_ram(shift_cpu, instr, memory);
	assert(shift_cpu.read_register(6) == 0x00000010u);
	instr = decode_instruction(0x0020D3B3); // SRL x7, x1, x2
	execute_with_ram(shift_cpu, instr, memory);
	assert(shift_cpu.read_register(7) == 0x08000000u);
	instr = decode_instruction(0x4020D433); // SRA x8, x1, x2
	execute_with_ram(shift_cpu, instr, memory);
	assert(shift_cpu.read_register(8) == 0xF8000000u);

	// Shift boundary: 31
	shift_cpu.write_register(1, 1);
	shift_cpu.write_register(2, 31);
	instr = decode_instruction(0x002091B3);
	execute_with_ram(shift_cpu, instr, memory);
	assert(shift_cpu.read_register(3) == 0x80000000u);
	shift_cpu.write_register(1, 0x80000000u);
	instr = decode_instruction(0x0020D233);
	execute_with_ram(shift_cpu, instr, memory);
	assert(shift_cpu.read_register(4) == 1);
	instr = decode_instruction(0x4020D2B3);
	execute_with_ram(shift_cpu, instr, memory);
	assert(shift_cpu.read_register(5) == 0xFFFFFFFFu);

	// Shift boundary: 0
	shift_cpu.write_register(2, 0);
	instr = decode_instruction(0x0020D233);
	execute_with_ram(shift_cpu, instr, memory);
	assert(shift_cpu.read_register(4) == 0x80000000u);

	// rd == rs2 still uses the original shift amount
	shift_cpu.write_register(1, 0x80000001u);
	shift_cpu.write_register(2, 4);
	instr = decode_instruction(0x4020D133);
	execute_with_ram(shift_cpu, instr, memory);
	assert(shift_cpu.read_register(2) == 0xF8000000u);

	// x0 as destination remains zero
	instr = decode_instruction(0x00209033);
	execute_with_ram(shift_cpu, instr, memory);
	assert(shift_cpu.read_register(0) == 0);

	// PC remains unchanged
	assert(shift_cpu.read_pc() == 0);

	/* Immediate shift instruction tests */
	Cpu immediate_shift_cpu{};

	immediate_shift_cpu.write_register(1, 0x80000001u);

	// SLLI x3, x1, 4
	instr = decode_instruction(0x00409193);
	execute_with_ram(immediate_shift_cpu, instr, memory);
	assert(immediate_shift_cpu.read_register(3) == 0x00000010u);

	// SRLI x4, x1, 4
	instr = decode_instruction(0x0040D213);
	execute_with_ram(immediate_shift_cpu, instr, memory);
	assert(immediate_shift_cpu.read_register(4) == 0x08000000u);

	// SRAI x5, x1, 4
	instr = decode_instruction(0x4040D293);
	execute_with_ram(immediate_shift_cpu, instr, memory);
	assert(immediate_shift_cpu.read_register(5) == 0xF8000000u);

	// Source register remains unchanged
	assert(immediate_shift_cpu.read_register(1) == 0x80000001u);

	// Shift boundary: 0
	instr = decode_instruction(0x00009313); // SLLI x6, x1, 0
	execute_with_ram(immediate_shift_cpu, instr, memory);
	assert(immediate_shift_cpu.read_register(6) == 0x80000001u);

	// Shift boundary: 31
	immediate_shift_cpu.write_register(1, 1);
	instr = decode_instruction(0x01F09313); // SLLI x6, x1, 31
	execute_with_ram(immediate_shift_cpu, instr, memory);
	assert(immediate_shift_cpu.read_register(6) == 0x80000000u);
	immediate_shift_cpu.write_register(1, 0x80000000u);
	instr = decode_instruction(0x01F0D393); // SRLI x7, x1, 31
	execute_with_ram(immediate_shift_cpu, instr, memory);
	assert(immediate_shift_cpu.read_register(7) == 1);
	instr = decode_instruction(0x41F0D413); // SRAI x8, x1, 31
	execute_with_ram(immediate_shift_cpu, instr, memory);
	assert(immediate_shift_cpu.read_register(8) == 0xFFFFFFFFu);

	// rd == rs1
	immediate_shift_cpu.write_register(1, 0x80000001u);
	instr = decode_instruction(0x4040D093); // SRAI x1, x1, 4
	execute_with_ram(immediate_shift_cpu, instr, memory);
	assert(immediate_shift_cpu.read_register(1) == 0xF8000000u);

	// x0 as a source register
	instr = decode_instruction(0x00405493); // SRLI x9, x0, 4
	execute_with_ram(immediate_shift_cpu, instr, memory);
	assert(immediate_shift_cpu.read_register(9) == 0);

	// x0 as destination remains zero
	instr = decode_instruction(0x00409013);
	execute_with_ram(immediate_shift_cpu, instr, memory);
	assert(immediate_shift_cpu.read_register(0) == 0);

	// PC remains unchanged
	assert(immediate_shift_cpu.read_pc() == 0);

	/* Register-register comparison instruction tests */
	Cpu comparison_cpu{};

	comparison_cpu.write_register(1, 0xFFFFFFFFu);
	comparison_cpu.write_register(2, 1);

	// Signed -1 is less than 1
	instr = decode_instruction(0x0020A1B3); // SLT x3, x1, x2
	execute_with_ram(comparison_cpu, instr, memory);
	assert(comparison_cpu.read_register(3) == 1);

	// Unsigned 0xFFFFFFFF is greater than 1
	instr = decode_instruction(0x0020B233); // SLTU x4, x1, x2
	execute_with_ram(comparison_cpu, instr, memory);
	assert(comparison_cpu.read_register(4) == 0);
	assert(comparison_cpu.read_register(1) == 0xFFFFFFFFu);
	assert(comparison_cpu.read_register(2) == 1);

	// Reverse operands produce the opposite signed/unsigned results
	comparison_cpu.write_register(1, 1);
	comparison_cpu.write_register(2, 0xFFFFFFFFu);
	instr = decode_instruction(0x0020A1B3);
	execute_with_ram(comparison_cpu, instr, memory);
	assert(comparison_cpu.read_register(3) == 0);
	instr = decode_instruction(0x0020B233);
	execute_with_ram(comparison_cpu, instr, memory);
	assert(comparison_cpu.read_register(4) == 1);

	// Equal values are not less than each other
	comparison_cpu.write_register(1, 0x80000000u);
	comparison_cpu.write_register(2, 0x80000000u);
	instr = decode_instruction(0x0020A1B3);
	execute_with_ram(comparison_cpu, instr, memory);
	assert(comparison_cpu.read_register(3) == 0);
	instr = decode_instruction(0x0020B233);
	execute_with_ram(comparison_cpu, instr, memory);
	assert(comparison_cpu.read_register(4) == 0);

	// Opposite ends of the signed range
	comparison_cpu.write_register(1, 0x80000000u);
	comparison_cpu.write_register(2, 0x7FFFFFFFu);
	instr = decode_instruction(0x0020A1B3);
	execute_with_ram(comparison_cpu, instr, memory);
	assert(comparison_cpu.read_register(3) == 1);
	instr = decode_instruction(0x0020B233);
	execute_with_ram(comparison_cpu, instr, memory);
	assert(comparison_cpu.read_register(4) == 0);

	// rd == rs1
	comparison_cpu.write_register(1, 0xFFFFFFFFu);
	comparison_cpu.write_register(2, 1);
	instr = decode_instruction(0x0020A0B3); // SLT x1, x1, x2
	execute_with_ram(comparison_cpu, instr, memory);
	assert(comparison_cpu.read_register(1) == 1);

	// rd == rs2
	comparison_cpu.write_register(1, 1);
	comparison_cpu.write_register(2, 0xFFFFFFFFu);
	instr = decode_instruction(0x0020B133); // SLTU x2, x1, x2
	execute_with_ram(comparison_cpu, instr, memory);
	assert(comparison_cpu.read_register(2) == 1);

	// x0 as a source register
	comparison_cpu.write_register(2, 42);
	instr = decode_instruction(0x002032B3); // SLTU x5, x0, x2
	execute_with_ram(comparison_cpu, instr, memory);
	assert(comparison_cpu.read_register(5) == 1);

	// x0 as destination remains zero
	instr = decode_instruction(0x0020A033);
	execute_with_ram(comparison_cpu, instr, memory);
	assert(comparison_cpu.read_register(0) == 0);

	// PC remains unchanged
	assert(comparison_cpu.read_pc() == 0);

	/* Immediate comparison instruction tests */
	Cpu immediate_comparison_cpu{};

	immediate_comparison_cpu.write_register(1, 0);

	// Signed 0 is not less than -1
	instr = decode_instruction(0xFFF0A193); // SLTI x3, x1, -1
	execute_with_ram(immediate_comparison_cpu, instr, memory);
	assert(immediate_comparison_cpu.read_register(3) == 0);

	// Unsigned 0 is less than sign-extended -1
	instr = decode_instruction(0xFFF0B213); // SLTIU x4, x1, -1
	execute_with_ram(immediate_comparison_cpu, instr, memory);
	assert(immediate_comparison_cpu.read_register(4) == 1);
	assert(immediate_comparison_cpu.read_register(1) == 0);

	// Signed -1 is less than 1, but unsigned 0xFFFFFFFF is not
	immediate_comparison_cpu.write_register(1, 0xFFFFFFFFu);
	instr = decode_instruction(0x0010A193); // SLTI x3, x1, 1
	execute_with_ram(immediate_comparison_cpu, instr, memory);
	assert(immediate_comparison_cpu.read_register(3) == 1);
	instr = decode_instruction(0x0010B213); // SLTIU x4, x1, 1
	execute_with_ram(immediate_comparison_cpu, instr, memory);
	assert(immediate_comparison_cpu.read_register(4) == 0);

	// Equal values are not less than each other
	immediate_comparison_cpu.write_register(1, 7);
	instr = decode_instruction(0x0070A193);
	execute_with_ram(immediate_comparison_cpu, instr, memory);
	assert(immediate_comparison_cpu.read_register(3) == 0);
	instr = decode_instruction(0x0070B213);
	execute_with_ram(immediate_comparison_cpu, instr, memory);
	assert(immediate_comparison_cpu.read_register(4) == 0);

	// SLTIU with immediate 1 implements the SEQZ behavior
	immediate_comparison_cpu.write_register(1, 0);
	instr = decode_instruction(0x0010B293); // SLTIU x5, x1, 1
	execute_with_ram(immediate_comparison_cpu, instr, memory);
	assert(immediate_comparison_cpu.read_register(5) == 1);
	immediate_comparison_cpu.write_register(1, 42);
	execute_with_ram(immediate_comparison_cpu, instr, memory);
	assert(immediate_comparison_cpu.read_register(5) == 0);

	// rd == rs1
	immediate_comparison_cpu.write_register(1, 0xFFFFFFFFu);
	instr = decode_instruction(0x0010A093); // SLTI x1, x1, 1
	execute_with_ram(immediate_comparison_cpu, instr, memory);
	assert(immediate_comparison_cpu.read_register(1) == 1);

	// x0 as a source register
	instr = decode_instruction(0x00103313); // SLTIU x6, x0, 1
	execute_with_ram(immediate_comparison_cpu, instr, memory);
	assert(immediate_comparison_cpu.read_register(6) == 1);

	// x0 as destination remains zero
	instr = decode_instruction(0x0010B013);
	execute_with_ram(immediate_comparison_cpu, instr, memory);
	assert(immediate_comparison_cpu.read_register(0) == 0);

	// PC remains unchanged
	assert(immediate_comparison_cpu.read_pc() == 0);

	/* LW instruction tests */
	Cpu load_cpu{};
	Memory load_memory{ 64 };

	load_memory.write32(24, 0x12345678u);
	load_cpu.write_register(1, 16);

	// LW x3, 8(x1)
	instr = decode_instruction(0x0080A183);
	execute_with_ram(load_cpu, instr, load_memory);
	assert(load_cpu.read_register(3) == 0x12345678u);
	assert(load_cpu.read_register(1) == 16);
	assert(load_memory.read32(24) == 0x12345678u);

	// LW x4, -4(x1)
	load_cpu.write_register(1, 28);
	instr = decode_instruction(0xFFC0A203);
	execute_with_ram(load_cpu, instr, load_memory);
	assert(load_cpu.read_register(4) == 0x12345678u);
	assert(load_cpu.read_register(1) == 28);

	// Effective-address addition wraps to 32 bits
	load_memory.write32(4, 0x89ABCDEFu);
	load_cpu.write_register(1, 0xFFFFFFFCu);
	instr = decode_instruction(0x0080A183);
	execute_with_ram(load_cpu, instr, load_memory);
	assert(load_cpu.read_register(3) == 0x89ABCDEFu);

	// rd == rs1 uses the original base address
	load_cpu.write_register(1, 16);
	instr = decode_instruction(0x0080A083); // LW x1, 8(x1)
	execute_with_ram(load_cpu, instr, load_memory);
	assert(load_cpu.read_register(1) == 0x12345678u);

	// A valid load targeting x0 still performs the memory access
	load_cpu.write_register(1, 16);
	instr = decode_instruction(0x0080A003); // LW x0, 8(x1)
	execute_with_ram(load_cpu, instr, load_memory);
	assert(load_cpu.read_register(0) == 0);

	// A load targeting x0 still reports an invalid access
	load_cpu.write_register(1, 64);
	instr = decode_instruction(0x0000A003); // LW x0, 0(x1)
	bool load_exception_thrown{ false };
	try {
		execute_with_ram(load_cpu, instr, load_memory);
	} catch (const Trap& trap) {
		load_exception_thrown = true;
		assert(trap.cause == TrapCause::LoadAccessFault);
	}
	assert(load_exception_thrown);
	assert(load_cpu.read_register(0) == 0);

	// A misaligned load leaves architectural state unchanged
	load_cpu.write_register(1, 17);
	load_cpu.write_register(3, 0xCAFEBABEu);
	instr = decode_instruction(0x0000A183); // LW x3, 0(x1)
	load_exception_thrown = false;
	try {
		execute_with_ram(load_cpu, instr, load_memory);
	} catch (const Trap& trap) {
		load_exception_thrown = true;
		assert(trap.cause == TrapCause::LoadAddressMisaligned);
	}
	assert(load_exception_thrown);
	assert(load_cpu.read_register(1) == 17);
	assert(load_cpu.read_register(3) == 0xCAFEBABEu);
	assert(load_memory.read32(24) == 0x12345678u);

	// An aligned out-of-range load also leaves its destination unchanged
	load_cpu.write_register(1, 64);
	load_exception_thrown = false;
	try {
		execute_with_ram(load_cpu, instr, load_memory);
	} catch (const Trap& trap) {
		load_exception_thrown = true;
		assert(trap.cause == TrapCause::LoadAccessFault);
	}
	assert(load_exception_thrown);
	assert(load_cpu.read_register(3) == 0xCAFEBABEu);

	// Direct execution does not change the PC
	assert(load_cpu.read_pc() == 0);

	/* SW instruction tests */
	Cpu store_cpu{};
	Memory store_memory{ 64 };

	store_cpu.write_register(1, 16);
	store_cpu.write_register(2, 0x12345678u);

	// SW x2, 8(x1)
	instr = decode_instruction(0x0020A423);
	execute_with_ram(store_cpu, instr, store_memory);
	assert(store_memory.read32(24) == 0x12345678u);
	assert(store_cpu.read_register(1) == 16);
	assert(store_cpu.read_register(2) == 0x12345678u);

	// SW x2, -4(x1)
	store_cpu.write_register(1, 28);
	store_cpu.write_register(2, 0x89ABCDEFu);
	instr = decode_instruction(0xFE20AE23);
	execute_with_ram(store_cpu, instr, store_memory);
	assert(store_memory.read32(24) == 0x89ABCDEFu);
	assert(store_cpu.read_register(1) == 28);
	assert(store_cpu.read_register(2) == 0x89ABCDEFu);

	// Effective-address addition wraps to 32 bits
	store_cpu.write_register(1, 0xFFFFFFFCu);
	store_cpu.write_register(2, 0xA5A5A5A5u);
	instr = decode_instruction(0x0020A423);
	execute_with_ram(store_cpu, instr, store_memory);
	assert(store_memory.read32(4) == 0xA5A5A5A5u);

	// rs1 == rs2 supplies both the original base and stored value
	store_cpu.write_register(1, 16);
	instr = decode_instruction(0x0010A423); // SW x1, 8(x1)
	execute_with_ram(store_cpu, instr, store_memory);
	assert(store_memory.read32(24) == 16);
	assert(store_cpu.read_register(1) == 16);

	// x0 as rs2 stores a complete zero word
	store_memory.write32(24, 0xFFFFFFFFu);
	instr = decode_instruction(0x0000A423); // SW x0, 8(x1)
	execute_with_ram(store_cpu, instr, store_memory);
	assert(store_memory.read32(24) == 0);

	std::array<std::uint8_t, 64> memory_before_store_error{};
	for (std::size_t i = 0; i < memory_before_store_error.size(); i++) {
		memory_before_store_error[i] = store_memory.read8(static_cast<std::uint32_t>(i));
	}
	std::array<std::uint32_t, 32> registers_before_store_error{};
	for (std::size_t i = 0; i < registers_before_store_error.size(); i++) {
		registers_before_store_error[i] = store_cpu.read_register(i);
	}

	// A misaligned store changes neither memory nor registers
	store_cpu.write_register(1, 17);
	store_cpu.write_register(2, 0xDEADBEEFu);
	registers_before_store_error[1] = 17;
	registers_before_store_error[2] = 0xDEADBEEFu;
	instr = decode_instruction(0x0020A023); // SW x2, 0(x1)
	bool store_exception_thrown{ false };
	try {
		execute_with_ram(store_cpu, instr, store_memory);
	} catch (const Trap& trap) {
		store_exception_thrown = true;
		assert(trap.cause == TrapCause::StoreAddressMisaligned);
	}
	assert(store_exception_thrown);
	for (std::size_t i = 0; i < memory_before_store_error.size(); i++) {
		assert(store_memory.read8(static_cast<std::uint32_t>(i)) == memory_before_store_error[i]);
	}
	for (std::size_t i = 0; i < registers_before_store_error.size(); i++) {
		assert(store_cpu.read_register(i) == registers_before_store_error[i]);
	}

	// An aligned out-of-range store is also atomic
	store_cpu.write_register(1, 64);
	registers_before_store_error[1] = 64;
	store_exception_thrown = false;
	try {
		execute_with_ram(store_cpu, instr, store_memory);
	} catch (const Trap& trap) {
		store_exception_thrown = true;
		assert(trap.cause == TrapCause::StoreAccessFault);
	}
	assert(store_exception_thrown);
	for (std::size_t i = 0; i < memory_before_store_error.size(); i++) {
		assert(store_memory.read8(static_cast<std::uint32_t>(i)) == memory_before_store_error[i]);
	}
	for (std::size_t i = 0; i < registers_before_store_error.size(); i++) {
		assert(store_cpu.read_register(i) == registers_before_store_error[i]);
	}

	// Direct execution does not change the PC
	assert(store_cpu.read_pc() == 0);

	/* Byte-load instruction tests */
	Cpu byte_load_cpu{};
	Memory byte_load_memory{ 64 };

	byte_load_cpu.write_register(1, 16);
	byte_load_memory.write8(17, 0x80u);

	// An odd byte address is valid; LB sign-extends bit 7
	instr = decode_instruction(0x00108183); // LB x3, 1(x1)
	execute_with_ram(byte_load_cpu, instr, byte_load_memory);
	assert(byte_load_cpu.read_register(3) == 0xFFFFFF80u);

	// LBU zero-extends the same byte
	instr = decode_instruction(0x0010C203); // LBU x4, 1(x1)
	execute_with_ram(byte_load_cpu, instr, byte_load_memory);
	assert(byte_load_cpu.read_register(4) == 0x00000080u);
	assert(byte_load_cpu.read_register(1) == 16);
	assert(byte_load_memory.read8(17) == 0x80u);

	// A byte with sign bit clear has the same result for both operations
	byte_load_memory.write8(17, 0x7Fu);
	instr = decode_instruction(0x00108183);
	execute_with_ram(byte_load_cpu, instr, byte_load_memory);
	assert(byte_load_cpu.read_register(3) == 0x0000007Fu);
	instr = decode_instruction(0x0010C203);
	execute_with_ram(byte_load_cpu, instr, byte_load_memory);
	assert(byte_load_cpu.read_register(4) == 0x0000007Fu);

	// Negative address offset
	byte_load_memory.write8(17, 0x81u);
	byte_load_cpu.write_register(1, 18);
	instr = decode_instruction(0xFFF08183); // LB x3, -1(x1)
	execute_with_ram(byte_load_cpu, instr, byte_load_memory);
	assert(byte_load_cpu.read_register(3) == 0xFFFFFF81u);
	assert(byte_load_cpu.read_register(1) == 18);

	// Effective-address addition wraps to 32 bits
	byte_load_memory.write8(1, 0xA5u);
	byte_load_cpu.write_register(1, 0xFFFFFFFFu);
	instr = decode_instruction(0x0020C203); // LBU x4, 2(x1)
	execute_with_ram(byte_load_cpu, instr, byte_load_memory);
	assert(byte_load_cpu.read_register(4) == 0x000000A5u);

	// rd == rs1 uses the original base address
	byte_load_cpu.write_register(1, 16);
	instr = decode_instruction(0x00108083); // LB x1, 1(x1)
	execute_with_ram(byte_load_cpu, instr, byte_load_memory);
	assert(byte_load_cpu.read_register(1) == 0xFFFFFF81u);

	// A valid load targeting x0 still performs the access
	byte_load_cpu.write_register(1, 16);
	instr = decode_instruction(0x00108003); // LB x0, 1(x1)
	execute_with_ram(byte_load_cpu, instr, byte_load_memory);
	assert(byte_load_cpu.read_register(0) == 0);

	// An invalid load targeting x0 still reports the access failure
	byte_load_cpu.write_register(1, 64);
	instr = decode_instruction(0x00008003); // LB x0, 0(x1)
	bool byte_load_exception_thrown{ false };
	try {
		execute_with_ram(byte_load_cpu, instr, byte_load_memory);
	} catch (const Trap& trap) {
		byte_load_exception_thrown = true;
		assert(trap.cause == TrapCause::LoadAccessFault);
	}
	assert(byte_load_exception_thrown);
	assert(byte_load_cpu.read_register(0) == 0);
	assert(byte_load_memory.read8(17) == 0x81u);

	// LBU reports the same access fault and preserves its destination
	byte_load_cpu.write_register(4, 0xCAFEBABEu);
	instr = decode_instruction(0x0000C203); // LBU x4, 0(x1)
	byte_load_exception_thrown = false;
	try {
		execute_with_ram(byte_load_cpu, instr, byte_load_memory);
	} catch (const Trap& trap) {
		byte_load_exception_thrown = true;
		assert(trap.cause == TrapCause::LoadAccessFault);
	}
	assert(byte_load_exception_thrown);
	assert(byte_load_cpu.read_register(4) == 0xCAFEBABEu);

	// Direct execution does not change the PC
	assert(byte_load_cpu.read_pc() == 0);

	/* SB instruction tests */
	Cpu byte_store_cpu{};
	Memory byte_store_memory{ 64 };

	byte_store_cpu.write_register(1, 16);
	byte_store_cpu.write_register(2, 0x12345680u);
	byte_store_memory.write8(16, 0x11u);
	byte_store_memory.write8(17, 0x22u);
	byte_store_memory.write8(18, 0x33u);

	// SB x2, 1(x1) stores only the low byte at an odd address
	instr = decode_instruction(0x002080A3);
	execute_with_ram(byte_store_cpu, instr, byte_store_memory);
	assert(byte_store_memory.read8(16) == 0x11u);
	assert(byte_store_memory.read8(17) == 0x80u);
	assert(byte_store_memory.read8(18) == 0x33u);
	assert(byte_store_cpu.read_register(1) == 16);
	assert(byte_store_cpu.read_register(2) == 0x12345680u);

	// Negative address offset and truncation of the upper register bits
	byte_store_cpu.write_register(1, 18);
	byte_store_cpu.write_register(2, 0xABCDEF7Fu);
	instr = decode_instruction(0xFE208FA3); // SB x2, -1(x1)
	execute_with_ram(byte_store_cpu, instr, byte_store_memory);
	assert(byte_store_memory.read8(17) == 0x7Fu);
	assert(byte_store_cpu.read_register(1) == 18);
	assert(byte_store_cpu.read_register(2) == 0xABCDEF7Fu);

	// Effective-address addition wraps to 32 bits
	byte_store_cpu.write_register(1, 0xFFFFFFFFu);
	byte_store_cpu.write_register(2, 0x000000A5u);
	instr = decode_instruction(0x00208123); // SB x2, 2(x1)
	execute_with_ram(byte_store_cpu, instr, byte_store_memory);
	assert(byte_store_memory.read8(1) == 0xA5u);

	// rs1 == rs2 supplies both the original base and stored value
	byte_store_cpu.write_register(1, 16);
	instr = decode_instruction(0x001080A3); // SB x1, 1(x1)
	execute_with_ram(byte_store_cpu, instr, byte_store_memory);
	assert(byte_store_memory.read8(17) == 0x10u);
	assert(byte_store_cpu.read_register(1) == 16);

	// x0 as rs2 writes a zero byte
	byte_store_memory.write8(17, 0xFFu);
	instr = decode_instruction(0x000080A3); // SB x0, 1(x1)
	execute_with_ram(byte_store_cpu, instr, byte_store_memory);
	assert(byte_store_memory.read8(17) == 0);

	std::array<std::uint8_t, 64> memory_before_byte_store_error{};
	for (std::size_t i = 0; i < memory_before_byte_store_error.size(); i++) {
		memory_before_byte_store_error[i] = byte_store_memory.read8(static_cast<std::uint32_t>(i));
	}
	std::array<std::uint32_t, 32> registers_before_byte_store_error{};
	for (std::size_t i = 0; i < registers_before_byte_store_error.size(); i++) {
		registers_before_byte_store_error[i] = byte_store_cpu.read_register(i);
	}

	// An out-of-range byte store changes neither memory nor registers
	byte_store_cpu.write_register(1, 64);
	byte_store_cpu.write_register(2, 0xDEADBEEFu);
	registers_before_byte_store_error[1] = 64;
	registers_before_byte_store_error[2] = 0xDEADBEEFu;
	instr = decode_instruction(0x00208023); // SB x2, 0(x1)
	bool byte_store_exception_thrown{ false };
	try {
		execute_with_ram(byte_store_cpu, instr, byte_store_memory);
	} catch (const Trap& trap) {
		byte_store_exception_thrown = true;
		assert(trap.cause == TrapCause::StoreAccessFault);
	}
	assert(byte_store_exception_thrown);
	for (std::size_t i = 0; i < memory_before_byte_store_error.size(); i++) {
		assert(byte_store_memory.read8(static_cast<std::uint32_t>(i)) == memory_before_byte_store_error[i]);
	}
	for (std::size_t i = 0; i < registers_before_byte_store_error.size(); i++) {
		assert(byte_store_cpu.read_register(i) == registers_before_byte_store_error[i]);
	}

	// Direct execution does not change the PC
	assert(byte_store_cpu.read_pc() == 0);

	/* Halfword-load instruction tests */
	Cpu halfword_load_cpu{};
	Memory halfword_load_memory{ 64 };

	halfword_load_cpu.write_register(1, 16);
	halfword_load_memory.write16(18, 0x8001u);

	// LH sign-extends bit 15
	instr = decode_instruction(0x00209183); // LH x3, 2(x1)
	execute_with_ram(halfword_load_cpu, instr, halfword_load_memory);
	assert(halfword_load_cpu.read_register(3) == 0xFFFF8001u);

	// LHU zero-extends the same halfword
	instr = decode_instruction(0x0020D203); // LHU x4, 2(x1)
	execute_with_ram(halfword_load_cpu, instr, halfword_load_memory);
	assert(halfword_load_cpu.read_register(4) == 0x00008001u);
	assert(halfword_load_cpu.read_register(1) == 16);
	assert(halfword_load_memory.read16(18) == 0x8001u);

	// A halfword with sign bit clear has the same result for both operations
	halfword_load_memory.write16(18, 0x7FFFu);
	instr = decode_instruction(0x00209183);
	execute_with_ram(halfword_load_cpu, instr, halfword_load_memory);
	assert(halfword_load_cpu.read_register(3) == 0x00007FFFu);
	instr = decode_instruction(0x0020D203);
	execute_with_ram(halfword_load_cpu, instr, halfword_load_memory);
	assert(halfword_load_cpu.read_register(4) == 0x00007FFFu);

	// Negative address offset
	halfword_load_memory.write16(18, 0x8002u);
	halfword_load_cpu.write_register(1, 20);
	instr = decode_instruction(0xFFE09183); // LH x3, -2(x1)
	execute_with_ram(halfword_load_cpu, instr, halfword_load_memory);
	assert(halfword_load_cpu.read_register(3) == 0xFFFF8002u);
	assert(halfword_load_cpu.read_register(1) == 20);

	// Effective-address addition wraps to 32 bits
	halfword_load_memory.write16(2, 0xA5A5u);
	halfword_load_cpu.write_register(1, 0xFFFFFFFEu);
	instr = decode_instruction(0x0040D203); // LHU x4, 4(x1)
	execute_with_ram(halfword_load_cpu, instr, halfword_load_memory);
	assert(halfword_load_cpu.read_register(4) == 0x0000A5A5u);

	// rd == rs1 uses the original base address
	halfword_load_memory.write16(18, 0x8001u);
	halfword_load_cpu.write_register(1, 16);
	instr = decode_instruction(0x00209083); // LH x1, 2(x1)
	execute_with_ram(halfword_load_cpu, instr, halfword_load_memory);
	assert(halfword_load_cpu.read_register(1) == 0xFFFF8001u);

	// A valid load targeting x0 still performs the access
	halfword_load_cpu.write_register(1, 16);
	instr = decode_instruction(0x00209003); // LH x0, 2(x1)
	execute_with_ram(halfword_load_cpu, instr, halfword_load_memory);
	assert(halfword_load_cpu.read_register(0) == 0);

	// A misaligned load targeting x0 still reports the access failure
	halfword_load_cpu.write_register(1, 17);
	instr = decode_instruction(0x00009003); // LH x0, 0(x1)
	bool halfword_load_exception_thrown{ false };
	try {
		execute_with_ram(halfword_load_cpu, instr, halfword_load_memory);
	} catch (const Trap& trap) {
		halfword_load_exception_thrown = true;
		assert(trap.cause == TrapCause::LoadAddressMisaligned);
	}
	assert(halfword_load_exception_thrown);
	assert(halfword_load_cpu.read_register(0) == 0);

	// An aligned out-of-range load leaves its destination unchanged
	halfword_load_cpu.write_register(1, 64);
	halfword_load_cpu.write_register(3, 0xCAFEBABEu);
	instr = decode_instruction(0x00009183); // LH x3, 0(x1)
	halfword_load_exception_thrown = false;
	try {
		execute_with_ram(halfword_load_cpu, instr, halfword_load_memory);
	} catch (const Trap& trap) {
		halfword_load_exception_thrown = true;
		assert(trap.cause == TrapCause::LoadAccessFault);
	}
	assert(halfword_load_exception_thrown);
	assert(halfword_load_cpu.read_register(3) == 0xCAFEBABEu);
	assert(halfword_load_memory.read16(18) == 0x8001u);

	// LHU reports the same access fault and preserves its destination
	halfword_load_cpu.write_register(4, 0xDEADBEEFu);
	instr = decode_instruction(0x0000D203); // LHU x4, 0(x1)
	halfword_load_exception_thrown = false;
	try {
		execute_with_ram(halfword_load_cpu, instr, halfword_load_memory);
	} catch (const Trap& trap) {
		halfword_load_exception_thrown = true;
		assert(trap.cause == TrapCause::LoadAccessFault);
	}
	assert(halfword_load_exception_thrown);
	assert(halfword_load_cpu.read_register(4) == 0xDEADBEEFu);

	// Direct execution does not change the PC
	assert(halfword_load_cpu.read_pc() == 0);

	/* SH instruction tests */
	Cpu halfword_store_cpu{};
	Memory halfword_store_memory{ 64 };

	halfword_store_cpu.write_register(1, 16);
	halfword_store_cpu.write_register(2, 0x12348001u);
	halfword_store_memory.write8(17, 0x11u);
	halfword_store_memory.write8(18, 0x22u);
	halfword_store_memory.write8(19, 0x33u);
	halfword_store_memory.write8(20, 0x44u);

	// SH x2, 2(x1) stores the low halfword in little-endian order
	instr = decode_instruction(0x00209123);
	execute_with_ram(halfword_store_cpu, instr, halfword_store_memory);
	assert(halfword_store_memory.read8(17) == 0x11u);
	assert(halfword_store_memory.read8(18) == 0x01u);
	assert(halfword_store_memory.read8(19) == 0x80u);
	assert(halfword_store_memory.read8(20) == 0x44u);
	assert(halfword_store_memory.read16(18) == 0x8001u);
	assert(halfword_store_cpu.read_register(1) == 16);
	assert(halfword_store_cpu.read_register(2) == 0x12348001u);

	// Negative address offset and truncation of the upper register bits
	halfword_store_cpu.write_register(1, 20);
	halfword_store_cpu.write_register(2, 0xABCD7FFFu);
	instr = decode_instruction(0xFE209F23); // SH x2, -2(x1)
	execute_with_ram(halfword_store_cpu, instr, halfword_store_memory);
	assert(halfword_store_memory.read16(18) == 0x7FFFu);
	assert(halfword_store_cpu.read_register(1) == 20);
	assert(halfword_store_cpu.read_register(2) == 0xABCD7FFFu);

	// Effective-address addition wraps to 32 bits
	halfword_store_cpu.write_register(1, 0xFFFFFFFEu);
	halfword_store_cpu.write_register(2, 0x0000A5A5u);
	instr = decode_instruction(0x00209223); // SH x2, 4(x1)
	execute_with_ram(halfword_store_cpu, instr, halfword_store_memory);
	assert(halfword_store_memory.read16(2) == 0xA5A5u);

	// rs1 == rs2 supplies both the original base and stored value
	halfword_store_cpu.write_register(1, 16);
	instr = decode_instruction(0x00109123); // SH x1, 2(x1)
	execute_with_ram(halfword_store_cpu, instr, halfword_store_memory);
	assert(halfword_store_memory.read16(18) == 16);
	assert(halfword_store_cpu.read_register(1) == 16);

	// x0 as rs2 writes a zero halfword
	halfword_store_memory.write16(18, 0xFFFFu);
	instr = decode_instruction(0x00009123); // SH x0, 2(x1)
	execute_with_ram(halfword_store_cpu, instr, halfword_store_memory);
	assert(halfword_store_memory.read16(18) == 0);

	std::array<std::uint8_t, 64> memory_before_halfword_store_error{};
	for (std::size_t i = 0; i < memory_before_halfword_store_error.size(); i++) {
		memory_before_halfword_store_error[i] = halfword_store_memory.read8(static_cast<std::uint32_t>(i));
	}
	std::array<std::uint32_t, 32> registers_before_halfword_store_error{};
	for (std::size_t i = 0; i < registers_before_halfword_store_error.size(); i++) {
		registers_before_halfword_store_error[i] = halfword_store_cpu.read_register(i);
	}

	// A misaligned halfword store changes neither memory nor registers
	halfword_store_cpu.write_register(1, 17);
	halfword_store_cpu.write_register(2, 0xDEADBEEFu);
	registers_before_halfword_store_error[1] = 17;
	registers_before_halfword_store_error[2] = 0xDEADBEEFu;
	instr = decode_instruction(0x00209023); // SH x2, 0(x1)
	bool halfword_store_exception_thrown{ false };
	try {
		execute_with_ram(halfword_store_cpu, instr, halfword_store_memory);
	} catch (const Trap& trap) {
		halfword_store_exception_thrown = true;
		assert(trap.cause == TrapCause::StoreAddressMisaligned);
	}
	assert(halfword_store_exception_thrown);
	for (std::size_t i = 0; i < memory_before_halfword_store_error.size(); i++) {
		assert(halfword_store_memory.read8(static_cast<std::uint32_t>(i)) == memory_before_halfword_store_error[i]);
	}
	for (std::size_t i = 0; i < registers_before_halfword_store_error.size(); i++) {
		assert(halfword_store_cpu.read_register(i) == registers_before_halfword_store_error[i]);
	}

	// An aligned out-of-range halfword store is also atomic
	halfword_store_cpu.write_register(1, 64);
	registers_before_halfword_store_error[1] = 64;
	halfword_store_exception_thrown = false;
	try {
		execute_with_ram(halfword_store_cpu, instr, halfword_store_memory);
	} catch (const Trap& trap) {
		halfword_store_exception_thrown = true;
		assert(trap.cause == TrapCause::StoreAccessFault);
	}
	assert(halfword_store_exception_thrown);
	for (std::size_t i = 0; i < memory_before_halfword_store_error.size(); i++) {
		assert(halfword_store_memory.read8(static_cast<std::uint32_t>(i)) == memory_before_halfword_store_error[i]);
	}
	for (std::size_t i = 0; i < registers_before_halfword_store_error.size(); i++) {
		assert(halfword_store_cpu.read_register(i) == registers_before_halfword_store_error[i]);
	}

	// Direct execution does not change the PC
	assert(halfword_store_cpu.read_pc() == 0);

	/* BEQ and BNE instruction tests */
	Cpu branch_cpu{};
	Memory branch_memory{ 16 };
	branch_memory.write32(0, 0xA5A5A5A5u);
	branch_cpu.set_pc(100);
	branch_cpu.write_register(1, 42);
	branch_cpu.write_register(2, 42);

	// Taken BEQ returns an absolute forward target
	instr = decode_instruction(0x00208463); // BEQ x1, x2, +8
	auto branch_target{ execute_with_ram(branch_cpu, instr, branch_memory) };
	assert(branch_target.has_value());
	assert(*branch_target == 108);
	assert(branch_cpu.read_pc() == 100);

	// Equal operands do not take BNE
	instr = decode_instruction(0xFE209EE3); // BNE x1, x2, -4
	branch_target = execute_with_ram(branch_cpu, instr, branch_memory);
	assert(!branch_target.has_value());
	assert(branch_cpu.read_pc() == 100);

	// Unequal operands do not take BEQ but do take backward BNE
	branch_cpu.write_register(2, 7);
	instr = decode_instruction(0x00208463);
	branch_target = execute_with_ram(branch_cpu, instr, branch_memory);
	assert(!branch_target.has_value());
	instr = decode_instruction(0xFE209EE3);
	branch_target = execute_with_ram(branch_cpu, instr, branch_memory);
	assert(branch_target.has_value());
	assert(*branch_target == 96);
	assert(branch_cpu.read_pc() == 100);

	// A taken offset-zero branch still returns an explicit target
	branch_cpu.write_register(1, 42);
	branch_cpu.write_register(2, 42);
	instr = decode_instruction(0x00108063); // BEQ x1, x1, 0
	branch_target = execute_with_ram(branch_cpu, instr, branch_memory);
	assert(branch_target.has_value());
	assert(*branch_target == 100);

	// Branch-target addition wraps to 32 bits
	branch_cpu.set_pc(0xFFFFFFFCu);
	instr = decode_instruction(0x00208463);
	branch_target = execute_with_ram(branch_cpu, instr, branch_memory);
	assert(branch_target.has_value());
	assert(*branch_target == 4);

	// A taken misaligned BEQ target raises an exception
	branch_cpu.set_pc(0);
	instr = decode_instruction(0x00208163); // BEQ x1, x2, +2
	bool branch_exception_thrown{ false };
	try {
		static_cast<void>(execute_with_ram(branch_cpu, instr, branch_memory));
	} catch (const Trap& trap) {
		branch_exception_thrown = true;
		assert(trap.cause == TrapCause::InstructionAddressMisaligned);
	}
	assert(branch_exception_thrown);
	assert(branch_cpu.read_pc() == 0);

	// The same misaligned target is irrelevant when BEQ is not taken
	branch_cpu.write_register(2, 7);
	branch_target = execute_with_ram(branch_cpu, instr, branch_memory);
	assert(!branch_target.has_value());
	assert(branch_cpu.read_pc() == 0);

	// Taken BNE applies the same target-alignment rule
	instr = decode_instruction(0x00209163); // BNE x1, x2, +2
	branch_exception_thrown = false;
	try {
		static_cast<void>(execute_with_ram(branch_cpu, instr, branch_memory));
	} catch (const Trap& trap) {
		branch_exception_thrown = true;
		assert(trap.cause == TrapCause::InstructionAddressMisaligned);
	}
	assert(branch_exception_thrown);
	assert(branch_cpu.read_pc() == 0);

	// Branches change neither registers nor memory
	assert(branch_cpu.read_register(1) == 42);
	assert(branch_cpu.read_register(2) == 7);
	assert(branch_memory.read32(0) == 0xA5A5A5A5u);

	/* BLT and BGE instruction tests */
	Cpu signed_branch_cpu{};
	Memory signed_branch_memory{ 16 };
	signed_branch_memory.write32(0, 0x5A5A5A5Au);
	signed_branch_cpu.set_pc(100);
	signed_branch_cpu.write_register(1, 0xFFFFFFFFu); // -1 when signed
	signed_branch_cpu.write_register(2, 1);

	// -1 is less than 1, so BLT is taken and BGE is not
	instr = decode_instruction(0x0020C463); // BLT x1, x2, +8
	auto signed_branch_target{ execute_with_ram(signed_branch_cpu, instr, signed_branch_memory) };
	assert(signed_branch_target.has_value());
	assert(*signed_branch_target == 108);
	assert(signed_branch_cpu.read_pc() == 100);
	instr = decode_instruction(0x0020D463); // BGE x1, x2, +8
	signed_branch_target = execute_with_ram(signed_branch_cpu, instr, signed_branch_memory);
	assert(!signed_branch_target.has_value());

	// Reversing the signed operands reverses both branch decisions
	signed_branch_cpu.write_register(1, 1);
	signed_branch_cpu.write_register(2, 0xFFFFFFFFu);
	instr = decode_instruction(0x0020C463);
	signed_branch_target = execute_with_ram(signed_branch_cpu, instr, signed_branch_memory);
	assert(!signed_branch_target.has_value());
	instr = decode_instruction(0x0020D463);
	signed_branch_target = execute_with_ram(signed_branch_cpu, instr, signed_branch_memory);
	assert(signed_branch_target.has_value());
	assert(*signed_branch_target == 108);

	// Equality does not take BLT but does take BGE
	signed_branch_cpu.write_register(1, 0x80000000u);
	signed_branch_cpu.write_register(2, 0x80000000u);
	instr = decode_instruction(0x0020C463);
	signed_branch_target = execute_with_ram(signed_branch_cpu, instr, signed_branch_memory);
	assert(!signed_branch_target.has_value());
	instr = decode_instruction(0x0020D463);
	signed_branch_target = execute_with_ram(signed_branch_cpu, instr, signed_branch_memory);
	assert(signed_branch_target.has_value());
	assert(*signed_branch_target == 108);

	// The most-negative signed value is less than the largest positive value
	signed_branch_cpu.write_register(1, 0x80000000u);
	signed_branch_cpu.write_register(2, 0x7FFFFFFFu);
	instr = decode_instruction(0xFE20CEE3); // BLT x1, x2, -4
	signed_branch_target = execute_with_ram(signed_branch_cpu, instr, signed_branch_memory);
	assert(signed_branch_target.has_value());
	assert(*signed_branch_target == 96);

	// A taken misaligned BLT target raises an exception
	signed_branch_cpu.set_pc(0);
	signed_branch_cpu.write_register(1, 0xFFFFFFFFu);
	signed_branch_cpu.write_register(2, 1);
	instr = decode_instruction(0x0020C163); // BLT x1, x2, +2
	bool signed_branch_exception_thrown{ false };
	try {
		static_cast<void>(execute_with_ram(signed_branch_cpu, instr, signed_branch_memory));
	} catch (const Trap& trap) {
		signed_branch_exception_thrown = true;
		assert(trap.cause == TrapCause::InstructionAddressMisaligned);
	}
	assert(signed_branch_exception_thrown);
	assert(signed_branch_cpu.read_pc() == 0);
	assert(signed_branch_cpu.read_register(1) == 0xFFFFFFFFu);
	assert(signed_branch_cpu.read_register(2) == 1);
	assert(signed_branch_memory.read32(0) == 0x5A5A5A5Au);

	// A not-taken BLT ignores the same misaligned encoded target
	signed_branch_cpu.write_register(1, 1);
	signed_branch_cpu.write_register(2, 0xFFFFFFFFu);
	signed_branch_target = execute_with_ram(signed_branch_cpu, instr, signed_branch_memory);
	assert(!signed_branch_target.has_value());
	assert(signed_branch_cpu.read_pc() == 0);

	// Taken BGE applies the same target-alignment rule
	instr = decode_instruction(0x0020D163); // BGE x1, x2, +2
	signed_branch_exception_thrown = false;
	try {
		static_cast<void>(execute_with_ram(signed_branch_cpu, instr, signed_branch_memory));
	} catch (const Trap& trap) {
		signed_branch_exception_thrown = true;
		assert(trap.cause == TrapCause::InstructionAddressMisaligned);
	}
	assert(signed_branch_exception_thrown);
	assert(signed_branch_cpu.read_pc() == 0);
	assert(signed_branch_cpu.read_register(1) == 1);
	assert(signed_branch_cpu.read_register(2) == 0xFFFFFFFFu);
	assert(signed_branch_memory.read32(0) == 0x5A5A5A5Au);

	/* BLTU and BGEU instruction tests */
	Cpu unsigned_branch_cpu{};
	Memory unsigned_branch_memory{ 16 };
	unsigned_branch_memory.write32(0, 0x3C3C3C3Cu);
	unsigned_branch_cpu.set_pc(100);
	unsigned_branch_cpu.write_register(1, 0xFFFFFFFFu);
	unsigned_branch_cpu.write_register(2, 1);

	// UINT32_MAX is not below 1, but it is greater than or equal to 1
	instr = decode_instruction(0x0020E463); // BLTU x1, x2, +8
	auto unsigned_branch_target{ execute_with_ram(unsigned_branch_cpu, instr, unsigned_branch_memory) };
	assert(!unsigned_branch_target.has_value());
	assert(unsigned_branch_cpu.read_pc() == 100);
	instr = decode_instruction(0x0020F463); // BGEU x1, x2, +8
	unsigned_branch_target = execute_with_ram(unsigned_branch_cpu, instr, unsigned_branch_memory);
	assert(unsigned_branch_target.has_value());
	assert(*unsigned_branch_target == 108);
	assert(unsigned_branch_cpu.read_pc() == 100);

	// Reversing the unsigned operands reverses both branch decisions
	unsigned_branch_cpu.write_register(1, 1);
	unsigned_branch_cpu.write_register(2, 0xFFFFFFFFu);
	instr = decode_instruction(0x0020E463);
	unsigned_branch_target = execute_with_ram(unsigned_branch_cpu, instr, unsigned_branch_memory);
	assert(unsigned_branch_target.has_value());
	assert(*unsigned_branch_target == 108);
	instr = decode_instruction(0x0020F463);
	unsigned_branch_target = execute_with_ram(unsigned_branch_cpu, instr, unsigned_branch_memory);
	assert(!unsigned_branch_target.has_value());

	// Equality does not take BLTU but does take BGEU
	unsigned_branch_cpu.write_register(1, 0xFFFFFFFFu);
	unsigned_branch_cpu.write_register(2, 0xFFFFFFFFu);
	instr = decode_instruction(0x0020E463);
	unsigned_branch_target = execute_with_ram(unsigned_branch_cpu, instr, unsigned_branch_memory);
	assert(!unsigned_branch_target.has_value());
	instr = decode_instruction(0x0020F463);
	unsigned_branch_target = execute_with_ram(unsigned_branch_cpu, instr, unsigned_branch_memory);
	assert(unsigned_branch_target.has_value());
	assert(*unsigned_branch_target == 108);

	// A taken unsigned branch can use a negative target offset
	unsigned_branch_cpu.write_register(1, 1);
	unsigned_branch_cpu.write_register(2, 0xFFFFFFFFu);
	instr = decode_instruction(0xFE20EEE3); // BLTU x1, x2, -4
	unsigned_branch_target = execute_with_ram(unsigned_branch_cpu, instr, unsigned_branch_memory);
	assert(unsigned_branch_target.has_value());
	assert(*unsigned_branch_target == 96);

	// A taken misaligned BLTU target raises an exception
	unsigned_branch_cpu.set_pc(0);
	instr = decode_instruction(0x0020E163); // BLTU x1, x2, +2
	bool unsigned_branch_exception_thrown{ false };
	try {
		static_cast<void>(execute_with_ram(unsigned_branch_cpu, instr, unsigned_branch_memory));
	} catch (const Trap& trap) {
		unsigned_branch_exception_thrown = true;
		assert(trap.cause == TrapCause::InstructionAddressMisaligned);
	}
	assert(unsigned_branch_exception_thrown);
	assert(unsigned_branch_cpu.read_pc() == 0);
	assert(unsigned_branch_cpu.read_register(1) == 1);
	assert(unsigned_branch_cpu.read_register(2) == 0xFFFFFFFFu);
	assert(unsigned_branch_memory.read32(0) == 0x3C3C3C3Cu);

	// A not-taken BLTU ignores the same misaligned encoded target
	unsigned_branch_cpu.write_register(1, 0xFFFFFFFFu);
	unsigned_branch_cpu.write_register(2, 1);
	unsigned_branch_target = execute_with_ram(unsigned_branch_cpu, instr, unsigned_branch_memory);
	assert(!unsigned_branch_target.has_value());
	assert(unsigned_branch_cpu.read_pc() == 0);

	// Taken BGEU applies the same target-alignment rule
	instr = decode_instruction(0x0020F163); // BGEU x1, x2, +2
	unsigned_branch_exception_thrown = false;
	try {
		static_cast<void>(execute_with_ram(unsigned_branch_cpu, instr, unsigned_branch_memory));
	} catch (const Trap& trap) {
		unsigned_branch_exception_thrown = true;
		assert(trap.cause == TrapCause::InstructionAddressMisaligned);
	}
	assert(unsigned_branch_exception_thrown);
	assert(unsigned_branch_cpu.read_pc() == 0);
	assert(unsigned_branch_cpu.read_register(1) == 0xFFFFFFFFu);
	assert(unsigned_branch_cpu.read_register(2) == 1);
	assert(unsigned_branch_memory.read32(0) == 0x3C3C3C3Cu);

	/* LUI instruction tests */
	Cpu lui_cpu{};
	Memory lui_memory{ 4 };
	lui_memory.write32(0, 0xA5A5A5A5u);
	lui_cpu.set_pc(100);
	lui_cpu.write_register(1, 0xDEADBEEFu);
	lui_cpu.write_register(2, 0xCAFEBABEu);
	lui_cpu.write_register(3, 0x13579BDFu);

	// LUI writes the decoded upper immediate directly to rd
	instr = decode_instruction(0x123450B7); // LUI x1, 0x12345
	auto lui_target{ execute_with_ram(lui_cpu, instr, lui_memory) };
	assert(lui_cpu.read_register(1) == 0x12345000u);
	assert(lui_cpu.read_register(2) == 0xCAFEBABEu);
	assert(lui_cpu.read_register(3) == 0x13579BDFu);
	assert(!lui_target.has_value());
	assert(lui_cpu.read_pc() == 100);

	// Bit 31 and all other upper immediate bits are preserved
	instr = decode_instruction(0xFFFFF137); // LUI x2, 0xFFFFF
	lui_target = execute_with_ram(lui_cpu, instr, lui_memory);
	assert(lui_cpu.read_register(2) == 0xFFFFF000u);
	assert(lui_cpu.read_register(1) == 0x12345000u);
	assert(lui_cpu.read_register(3) == 0x13579BDFu);
	assert(!lui_target.has_value());
	assert(lui_cpu.read_pc() == 100);

	// The hardwired zero register ignores a LUI result
	instr = decode_instruction(0xABCDE037); // LUI x0, 0xABCDE
	lui_target = execute_with_ram(lui_cpu, instr, lui_memory);
	assert(lui_cpu.read_register(0) == 0);
	assert(lui_cpu.read_register(1) == 0x12345000u);
	assert(lui_cpu.read_register(2) == 0xFFFFF000u);
	assert(lui_cpu.read_register(3) == 0x13579BDFu);
	assert(!lui_target.has_value());
	assert(lui_cpu.read_pc() == 100);
	assert(lui_memory.read32(0) == 0xA5A5A5A5u);

	/* AUIPC instruction tests */
	Cpu auipc_cpu{};
	Memory auipc_memory{ 4 };
	auipc_memory.write32(0, 0x5A5A5A5Au);
	auipc_cpu.set_pc(0x00001000u);
	auipc_cpu.write_register(1, 0xDEADBEEFu);
	auipc_cpu.write_register(2, 0xCAFEBABEu);
	auipc_cpu.write_register(3, 0x13579BDFu);

	// AUIPC adds the upper immediate to the current instruction PC
	instr = decode_instruction(0x12345097); // AUIPC x1, 0x12345
	auto auipc_target{ execute_with_ram(auipc_cpu, instr, auipc_memory) };
	assert(auipc_cpu.read_register(1) == 0x12346000u);
	assert(auipc_cpu.read_register(2) == 0xCAFEBABEu);
	assert(auipc_cpu.read_register(3) == 0x13579BDFu);
	assert(!auipc_target.has_value());
	assert(auipc_cpu.read_pc() == 0x00001000u);

	// Addition wraps to 32 bits
	auipc_cpu.set_pc(0x00002000u);
	instr = decode_instruction(0xFFFFF117); // AUIPC x2, 0xFFFFF
	auipc_target = execute_with_ram(auipc_cpu, instr, auipc_memory);
	assert(auipc_cpu.read_register(2) == 0x00001000u);
	assert(auipc_cpu.read_register(1) == 0x12346000u);
	assert(auipc_cpu.read_register(3) == 0x13579BDFu);
	assert(!auipc_target.has_value());
	assert(auipc_cpu.read_pc() == 0x00002000u);

	// The hardwired zero register ignores an AUIPC result
	auipc_cpu.set_pc(0x00003000u);
	instr = decode_instruction(0xABCDE017); // AUIPC x0, 0xABCDE
	auipc_target = execute_with_ram(auipc_cpu, instr, auipc_memory);
	assert(auipc_cpu.read_register(0) == 0);
	assert(auipc_cpu.read_register(1) == 0x12346000u);
	assert(auipc_cpu.read_register(2) == 0x00001000u);
	assert(auipc_cpu.read_register(3) == 0x13579BDFu);
	assert(!auipc_target.has_value());
	assert(auipc_cpu.read_pc() == 0x00003000u);
	assert(auipc_memory.read32(0) == 0x5A5A5A5Au);

	/* JAL instruction tests */
	Cpu jal_cpu{};
	Memory jal_memory{ 4 };
	jal_memory.write32(0, 0x96969696u);
	jal_cpu.set_pc(100);
	jal_cpu.write_register(1, 0xDEADBEEFu);
	jal_cpu.write_register(2, 0xCAFEBABEu);
	jal_cpu.write_register(3, 0x13579BDFu);

	// A forward JAL returns its absolute target and writes PC + 4 to rd
	instr = decode_instruction(0x008000EF); // JAL x1, +8
	auto jal_target{ execute_with_ram(jal_cpu, instr, jal_memory) };
	assert(jal_target.has_value());
	assert(*jal_target == 108);
	assert(jal_cpu.read_register(1) == 104);
	assert(jal_cpu.read_register(2) == 0xCAFEBABEu);
	assert(jal_cpu.read_register(3) == 0x13579BDFu);
	assert(jal_cpu.read_pc() == 100);

	// A negative offset branches backward while preserving the same link rule
	instr = decode_instruction(0xFFDFF16F); // JAL x2, -4
	jal_target = execute_with_ram(jal_cpu, instr, jal_memory);
	assert(jal_target.has_value());
	assert(*jal_target == 96);
	assert(jal_cpu.read_register(1) == 104);
	assert(jal_cpu.read_register(2) == 104);
	assert(jal_cpu.read_register(3) == 0x13579BDFu);
	assert(jal_cpu.read_pc() == 100);

	// rd == x0 jumps without saving a link
	instr = decode_instruction(0x0080006F); // JAL x0, +8
	jal_target = execute_with_ram(jal_cpu, instr, jal_memory);
	assert(jal_target.has_value());
	assert(*jal_target == 108);
	assert(jal_cpu.read_register(0) == 0);
	assert(jal_cpu.read_pc() == 100);

	// A zero-offset JAL still returns an explicit target and writes its link
	instr = decode_instruction(0x000002EF); // JAL x5, 0
	jal_target = execute_with_ram(jal_cpu, instr, jal_memory);
	assert(jal_target.has_value());
	assert(*jal_target == 100);
	assert(jal_cpu.read_register(5) == 104);
	assert(jal_cpu.read_pc() == 100);

	// Target and link additions both wrap to 32 bits
	jal_cpu.set_pc(0xFFFFFFFCu);
	instr = decode_instruction(0x008001EF); // JAL x3, +8
	jal_target = execute_with_ram(jal_cpu, instr, jal_memory);
	assert(jal_target.has_value());
	assert(*jal_target == 4);
	assert(jal_cpu.read_register(3) == 0);
	assert(jal_cpu.read_pc() == 0xFFFFFFFCu);

	// A misaligned target is rejected before the link register is changed
	jal_cpu.set_pc(100);
	jal_cpu.write_register(4, 0xA5A5A5A5u);
	instr = decode_instruction(0x0020026F); // JAL x4, +2
	bool jal_exception_thrown{ false };
	try {
		static_cast<void>(execute_with_ram(jal_cpu, instr, jal_memory));
	} catch (const Trap& trap) {
		jal_exception_thrown = true;
		assert(trap.cause == TrapCause::InstructionAddressMisaligned);
	}
	assert(jal_exception_thrown);
	assert(jal_cpu.read_pc() == 100);
	assert(jal_cpu.read_register(1) == 104);
	assert(jal_cpu.read_register(2) == 104);
	assert(jal_cpu.read_register(3) == 0);
	assert(jal_cpu.read_register(4) == 0xA5A5A5A5u);
	assert(jal_cpu.read_register(5) == 104);
	assert(jal_memory.read32(0) == 0x96969696u);

	/* JALR instruction tests */
	Cpu jalr_cpu{};
	Memory jalr_memory{ 4 };
	jalr_memory.write32(0, 0x69696969u);
	jalr_cpu.set_pc(100);
	jalr_cpu.write_register(1, 0xDEADBEEFu);
	jalr_cpu.write_register(2, 200);
	jalr_cpu.write_register(3, 0x13579BDFu);

	// JALR returns rs1 + immediate and writes PC + 4 to rd
	instr = decode_instruction(0x008100E7); // JALR x1, 8(x2)
	auto jalr_target{ execute_with_ram(jalr_cpu, instr, jalr_memory) };
	assert(jalr_target.has_value());
	assert(*jalr_target == 208);
	assert(jalr_cpu.read_register(1) == 104);
	assert(jalr_cpu.read_register(2) == 200);
	assert(jalr_cpu.read_register(3) == 0x13579BDFu);
	assert(jalr_cpu.read_pc() == 100);

	// A sign-extended negative immediate can form a backward target
	instr = decode_instruction(0xFFC101E7); // JALR x3, -4(x2)
	jalr_target = execute_with_ram(jalr_cpu, instr, jalr_memory);
	assert(jalr_target.has_value());
	assert(*jalr_target == 196);
	assert(jalr_cpu.read_register(1) == 104);
	assert(jalr_cpu.read_register(2) == 200);
	assert(jalr_cpu.read_register(3) == 104);
	assert(jalr_cpu.read_pc() == 100);

	// Target bit zero is cleared before alignment is checked
	jalr_cpu.write_register(2, 101);
	instr = decode_instruction(0x000100E7); // JALR x1, 0(x2)
	jalr_target = execute_with_ram(jalr_cpu, instr, jalr_memory);
	assert(jalr_target.has_value());
	assert(*jalr_target == 100);
	assert(jalr_cpu.read_register(1) == 104);
	assert(jalr_cpu.read_register(2) == 101);

	// rd == rs1 must calculate the target from the old register value
	jalr_cpu.write_register(1, 200);
	instr = decode_instruction(0x000080E7); // JALR x1, 0(x1)
	jalr_target = execute_with_ram(jalr_cpu, instr, jalr_memory);
	assert(jalr_target.has_value());
	assert(*jalr_target == 200);
	assert(jalr_cpu.read_register(1) == 104);
	assert(jalr_cpu.read_pc() == 100);

	// rd == x0 jumps without saving a link
	jalr_cpu.write_register(1, 200);
	instr = decode_instruction(0x00008067); // JALR x0, 0(x1)
	jalr_target = execute_with_ram(jalr_cpu, instr, jalr_memory);
	assert(jalr_target.has_value());
	assert(*jalr_target == 200);
	assert(jalr_cpu.read_register(0) == 0);
	assert(jalr_cpu.read_register(1) == 200);

	// Target addition wraps to 32 bits
	jalr_cpu.write_register(2, 0);
	instr = decode_instruction(0xFFC100E7); // JALR x1, -4(x2)
	jalr_target = execute_with_ram(jalr_cpu, instr, jalr_memory);
	assert(jalr_target.has_value());
	assert(*jalr_target == 0xFFFFFFFCu);
	assert(jalr_cpu.read_register(1) == 104);
	assert(jalr_cpu.read_pc() == 100);

	// Link addition also wraps to 32 bits
	jalr_cpu.set_pc(0xFFFFFFFCu);
	jalr_cpu.write_register(2, 100);
	instr = decode_instruction(0x000101E7); // JALR x3, 0(x2)
	jalr_target = execute_with_ram(jalr_cpu, instr, jalr_memory);
	assert(jalr_target.has_value());
	assert(*jalr_target == 100);
	assert(jalr_cpu.read_register(3) == 0);
	assert(jalr_cpu.read_pc() == 0xFFFFFFFCu);

	// Clearing bit zero cannot repair a target with bit one set
	jalr_cpu.set_pc(100);
	jalr_cpu.write_register(1, 0xA5A5A5A5u);
	jalr_cpu.write_register(2, 102);
	instr = decode_instruction(0x000100E7); // JALR x1, 0(x2)
	bool jalr_exception_thrown{ false };
	try {
		static_cast<void>(execute_with_ram(jalr_cpu, instr, jalr_memory));
	} catch (const Trap& trap) {
		jalr_exception_thrown = true;
		assert(trap.cause == TrapCause::InstructionAddressMisaligned);
	}
	assert(jalr_exception_thrown);
	assert(jalr_cpu.read_pc() == 100);
	assert(jalr_cpu.read_register(1) == 0xA5A5A5A5u);
	assert(jalr_cpu.read_register(2) == 102);
	assert(jalr_cpu.read_register(3) == 0);
	assert(jalr_memory.read32(0) == 0x69696969u);

	/* FENCE instruction tests */
	Cpu fence_cpu{};
	Memory fence_memory{ 8 };
	fence_memory.write32(0, 0xA5A5A5A5u);
	fence_memory.write32(4, 0x5A5A5A5Au);
	fence_cpu.set_pc(100);
	for (std::size_t i = 1; i < 32; i++) {
		fence_cpu.write_register(i, 0x10000000u + static_cast<std::uint32_t>(i));
	}

	std::array<std::uint32_t, 32> registers_before_fences{};
	for (std::size_t i = 0; i < registers_before_fences.size(); i++) {
		registers_before_fences[i] = fence_cpu.read_register(i);
	}

	// Normal, restricted, TSO, and ignored-field forms are all no-ops here
	instr = decode_instruction(0x0FF0000F); // FENCE IORW, IORW
	auto fence_target{ execute_with_ram(fence_cpu, instr, fence_memory) };
	assert(!fence_target.has_value());
	instr = decode_instruction(0x0330000F); // FENCE RW, RW
	fence_target = execute_with_ram(fence_cpu, instr, fence_memory);
	assert(!fence_target.has_value());
	instr = decode_instruction(0x8330000F); // FENCE.TSO
	fence_target = execute_with_ram(fence_cpu, instr, fence_memory);
	assert(!fence_target.has_value());
	instr = decode_instruction(0x0FF1008F); // Nonzero reserved rs1 and rd
	fence_target = execute_with_ram(fence_cpu, instr, fence_memory);
	assert(!fence_target.has_value());

	assert(fence_cpu.read_pc() == 100);
	for (std::size_t i = 0; i < registers_before_fences.size(); i++) {
		assert(fence_cpu.read_register(i) == registers_before_fences[i]);
	}
	assert(fence_memory.read32(0) == 0xA5A5A5A5u);
	assert(fence_memory.read32(4) == 0x5A5A5A5Au);

	/* ECALL and EBREAK instruction tests */
	Cpu trap_cpu{};
	Memory trap_memory{ 8 };
	trap_memory.write32(0, 0xA5A5A5A5u);
	trap_memory.write32(4, 0x5A5A5A5Au);
	trap_cpu.set_pc(100);
	trap_cpu.write_register(1, 0xDEADBEEFu);
	trap_cpu.write_register(2, 0xCAFEBABEu);
	trap_cpu.write_register(3, 0x13579BDFu);

	// ECALL produces a distinct environment-call trap
	instr = decode_instruction(0x00000073);
	bool trap_thrown{ false };
	TrapCause trap_cause{ TrapCause::BreakPoint };
	try {
		static_cast<void>(execute_with_ram(trap_cpu, instr, trap_memory));
	} catch (const Trap& trap) {
		trap_thrown = true;
		trap_cause = trap.cause;
	}
	assert(trap_thrown);
	assert(trap_cause == TrapCause::EnvironmentCall);
	assert(trap_cpu.read_pc() == 100);
	assert(trap_cpu.read_register(1) == 0xDEADBEEFu);
	assert(trap_cpu.read_register(2) == 0xCAFEBABEu);
	assert(trap_cpu.read_register(3) == 0x13579BDFu);
	assert(trap_memory.read32(0) == 0xA5A5A5A5u);
	assert(trap_memory.read32(4) == 0x5A5A5A5Au);

	// EBREAK produces a distinct breakpoint trap
	instr = decode_instruction(0x00100073);
	trap_thrown = false;
	trap_cause = TrapCause::EnvironmentCall;
	try {
		static_cast<void>(execute_with_ram(trap_cpu, instr, trap_memory));
	} catch (const Trap& trap) {
		trap_thrown = true;
		trap_cause = trap.cause;
	}
	assert(trap_thrown);
	assert(trap_cause == TrapCause::BreakPoint);
	assert(trap_cpu.read_pc() == 100);
	assert(trap_cpu.read_register(1) == 0xDEADBEEFu);
	assert(trap_cpu.read_register(2) == 0xCAFEBABEu);
	assert(trap_cpu.read_register(3) == 0x13579BDFu);
	assert(trap_memory.read32(0) == 0xA5A5A5A5u);
	assert(trap_memory.read32(4) == 0x5A5A5A5Au);

	// A reserved SYSTEM encoding produces an illegal-instruction trap
	instr = decode_instruction(0x00200073);
	trap_thrown = false;
	trap_cause = TrapCause::EnvironmentCall;
	try {
		static_cast<void>(execute_with_ram(trap_cpu, instr, trap_memory));
	} catch (const Trap& trap) {
		trap_thrown = true;
		trap_cause = trap.cause;
	}
	assert(trap_thrown);
	assert(trap_cause == TrapCause::IllegalInstruction);
	assert(trap_cpu.read_pc() == 100);
	assert(trap_cpu.read_register(1) == 0xDEADBEEFu);
	assert(trap_cpu.read_register(2) == 0xCAFEBABEu);
	assert(trap_cpu.read_register(3) == 0x13579BDFu);
	assert(trap_memory.read32(0) == 0xA5A5A5A5u);
	assert(trap_memory.read32(4) == 0x5A5A5A5Au);

	return 0;
}
