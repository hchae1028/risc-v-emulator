#include <cstddef>
#include <cstdint>
#include <cassert>
#include "cpu.hpp"

int main() {
	Cpu cpu{};
	
	auto pc = cpu.read_pc();
	
	// Zero-initialization tests
	assert(pc == 0);
	
	for (std::size_t i = 0; i < 32; i++) {
		auto register_value = cpu.read_register(i);
		assert(register_value == 0);
	}

	// write_register() tests
	std::uint32_t expected {0x12345678};
	cpu.write_register(1, 0x12345678);
	assert(cpu.read_register(1) == expected);

	expected = 0xFFFFFFFF;
	cpu.write_register(31, 0xFFFFFFFF);
	assert(cpu.read_register(31) == expected);

	cpu.write_register(0, 0x12345678);
	assert(cpu.read_register(0) == 0);
	
	// Other registers' values do not change
	assert(cpu.read_register(31) == expected);

	// pc test
	cpu.set_pc(0x1000);
	assert(cpu.read_pc() == 0x1000);

	return 0;
}
