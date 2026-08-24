#include <cstddef>
#include <stdexcept>
#include <cassert>
#include "cpu.hpp"
#include "memory.hpp"

int main() {
	bool exception_thrown{ false };

	/* fetch_instruction() tests */
	Cpu cpu2{};
	Memory memory2{ 64 };

	// Fetch an instruction when PC = 0
	assert(cpu2.fetch_instruction(memory2) == 0);

	// Instruction when PC = 4
	cpu2.set_pc(4);
	memory2.write32(4, 0x11111111);
	assert(cpu2.fetch_instruction(memory2) == 0x11111111);
	assert(cpu2.read_pc() == 4);

	// Register values unchanged
	for (std::size_t i = 0; i < 32; i++) {
		auto register_value = cpu2.read_register(i);
		assert(register_value == 0);
	}
	// Memory bytes unchanged
	for (std::uint32_t i = 0; i < 64; i++) {
		if (i == 4 || i == 5 || i == 6 || i == 7) {
			assert(memory2.read8(i) == 0x11);
			continue;
		}
		assert(memory2.read8(i) == 0);
	}

	// Fetch the instruction at the highest valid PC
	cpu2.set_pc(60);
	memory2.write32(60, 0xFFFFFFFF);
	assert(cpu2.fetch_instruction(memory2) == 0xFFFFFFFF);
	assert(memory2.read32(60) == 0xFFFFFFFF);
	
	// Aligned but out-of-range PC
	cpu2.set_pc(64);
	try {
		cpu2.fetch_instruction(memory2);
	} catch (const std::out_of_range& e) {
		exception_thrown = true;
	}
	assert(exception_thrown);

	// Misaligned in-range PC
	exception_thrown = false;
	cpu2.set_pc(2);
	try {
		cpu2.fetch_instruction(memory2);
	} catch (const std::runtime_error& e) {
		exception_thrown = true;
	}
	assert(exception_thrown);

	return 0;
}
