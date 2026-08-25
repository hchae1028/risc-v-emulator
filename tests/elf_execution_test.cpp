#include "cpu.hpp"
#include "elf_loader.hpp"
#include "executor.hpp"
#include "memory.hpp"
#include "runner.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

void write16(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint16_t value) {
	bytes[offset] = static_cast<std::uint8_t>(value & 0xFFu);
	bytes[offset + 1] = static_cast<std::uint8_t>((value >> 8) & 0xFFu);
}

void write32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value) {
	bytes[offset] = static_cast<std::uint8_t>(value & 0xFFu);
	bytes[offset + 1] = static_cast<std::uint8_t>((value >> 8) & 0xFFu);
	bytes[offset + 2] = static_cast<std::uint8_t>((value >> 16) & 0xFFu);
	bytes[offset + 3] = static_cast<std::uint8_t>((value >> 24) & 0xFFu);
}

std::vector<std::uint8_t> make_executable_elf() {
	std::vector<std::uint8_t> bytes(92);
	bytes[0] = 0x7Fu;
	bytes[1] = 0x45u;
	bytes[2] = 0x4Cu;
	bytes[3] = 0x46u;
	bytes[4] = 1;
	bytes[5] = 1;
	bytes[6] = 1;
	write16(bytes, 16, 2);
	write16(bytes, 18, 243);
	write32(bytes, 20, 1);
	write32(bytes, 24, 0x100u);
	write32(bytes, 28, 52);
	write32(bytes, 36, 0x10u);
	write16(bytes, 40, 52);
	write16(bytes, 42, 32);
	write16(bytes, 44, 1);

	write32(bytes, 52, 1);       // PT_LOAD
	write32(bytes, 56, 84);      // p_offset
	write32(bytes, 60, 0x100u);  // p_vaddr
	write32(bytes, 64, 0x100u);  // p_paddr
	write32(bytes, 68, 8);       // p_filesz
	write32(bytes, 72, 8);       // p_memsz
	write32(bytes, 76, 0x5u);    // PF_R | PF_X
	write32(bytes, 80, 4);       // p_align

	bytes[84] = 0x93u;
	bytes[85] = 0x00u;
	bytes[86] = 0x50u;
	bytes[87] = 0x00u; // ADDI x1, x0, 5
	bytes[88] = 0x73u;
	bytes[89] = 0x00u;
	bytes[90] = 0x10u;
	bytes[91] = 0x00u; // EBREAK

	return bytes;
}

}

int main() {
	const auto elf_bytes{ make_executable_elf() };
	Memory memory{ 512 };
	const auto entry{ load_elf32(memory, elf_bytes) };

	Cpu cpu{};
	cpu.set_pc(entry);
	const auto result{ run_until_trap(cpu, memory, 10) };

	assert(entry == 0x100u);
	assert(memory.read32(0x100u) == 0x00500093u);
	assert(memory.read32(0x104u) == 0x00100073u);
	assert(result.trap.has_value());
	assert(*result.trap == TrapCause::BreakPoint);
	assert(result.instructions_retired == 1);
	assert(cpu.read_pc() == 0x104u);
	assert(cpu.read_register(1) == 5);

	return 0;
}
