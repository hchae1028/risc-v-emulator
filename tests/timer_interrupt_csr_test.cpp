#include "bus.hpp"
#include "cpu.hpp"
#include "decoder.hpp"
#include "executor.hpp"
#include "memory.hpp"
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

}

int main() {
	constexpr std::uint16_t mie{ 0x304u };
	constexpr std::uint16_t mip{ 0x344u };
	constexpr std::uint32_t mtie{ 1u << 7 };
	constexpr std::uint32_t mtip{ 1u << 7 };

	/* Only MTIE is writable in mie; MTIP is controlled by the external line. */
	Cpu cpu{};
	assert(cpu.read_csr(mie) == 0u);
	assert(cpu.read_csr(mip) == 0u);
	cpu.write_csr(mie, 0xFFFFFFFFu);
	assert(cpu.read_csr(mie) == mtie);
	cpu.write_csr(mie, 0u);
	assert(cpu.read_csr(mie) == 0u);

	cpu.set_machine_timer_interrupt(true);
	assert(cpu.read_csr(mip) == mtip);
	cpu.write_csr(mip, 0u);
	assert(cpu.read_csr(mip) == mtip);
	cpu.write_csr(mip, 0xFFFFFFFFu);
	assert(cpu.read_csr(mip) == mtip);
	cpu.set_machine_timer_interrupt(false);
	assert(cpu.read_csr(mip) == 0u);

	Memory memory{ 4 };
	Bus bus{ memory, 0 };

	/* Zicsr operations can set and clear MTIE while returning the old value. */
	Cpu instruction_cpu{};
	instruction_cpu.write_register(1, 0xFFFFFFFFu);
	auto instruction{ decode_instruction(encode_csr(mie, 1, 0x1u, 2)) }; // CSRRW
	static_cast<void>(execute_instruction(instruction_cpu, instruction, bus));
	assert(instruction_cpu.read_register(2) == 0u);
	assert(instruction_cpu.read_csr(mie) == mtie);

	instruction_cpu.write_register(1, mtie);
	instruction = decode_instruction(encode_csr(mie, 1, 0x3u, 3)); // CSRRC
	static_cast<void>(execute_instruction(instruction_cpu, instruction, bus));
	assert(instruction_cpu.read_register(3) == mtie);
	assert(instruction_cpu.read_csr(mie) == 0u);

	instruction = decode_instruction(encode_csr(mie, 1, 0x2u, 4)); // CSRRS
	static_cast<void>(execute_instruction(instruction_cpu, instruction, bus));
	assert(instruction_cpu.read_register(4) == 0u);
	assert(instruction_cpu.read_csr(mie) == mtie);

	/* CSR writes to mip succeed but cannot alter the timer's pending signal. */
	instruction_cpu.set_machine_timer_interrupt(true);
	instruction = decode_instruction(encode_csr(mip, 0, 0x2u, 5)); // read-only CSRRS
	static_cast<void>(execute_instruction(instruction_cpu, instruction, bus));
	assert(instruction_cpu.read_register(5) == mtip);
	assert(instruction_cpu.read_csr(mip) == mtip);

	instruction_cpu.write_register(1, 0u);
	instruction = decode_instruction(encode_csr(mip, 1, 0x1u, 6)); // CSRRW attempts clear
	static_cast<void>(execute_instruction(instruction_cpu, instruction, bus));
	assert(instruction_cpu.read_register(6) == mtip);
	assert(instruction_cpu.read_csr(mip) == mtip);

	instruction_cpu.write_register(1, mtip);
	instruction = decode_instruction(encode_csr(mip, 1, 0x3u, 7)); // CSRRC attempts clear
	static_cast<void>(execute_instruction(instruction_cpu, instruction, bus));
	assert(instruction_cpu.read_register(7) == mtip);
	assert(instruction_cpu.read_csr(mip) == mtip);

	instruction_cpu.set_machine_timer_interrupt(false);
	instruction = decode_instruction(encode_csr(mip, 31, 0x5u, 8)); // CSRRWI attempts set
	static_cast<void>(execute_instruction(instruction_cpu, instruction, bus));
	assert(instruction_cpu.read_register(8) == 0u);
	assert(instruction_cpu.read_csr(mip) == 0u);

	return 0;
}
