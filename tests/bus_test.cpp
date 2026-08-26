#include "bus.hpp"
#include "memory.hpp"
#include <cassert>
#include <cstdint>
#include <stdexcept>

template <typename Function>
void assert_out_of_range(Function function) {
	bool exception_thrown{ false };
	try {
		function();
	} catch (const std::out_of_range&) {
		exception_thrown = true;
	}
	assert(exception_thrown);
}

int main() {
	/* Physical addresses are translated to offsets in the mapped RAM */
	Memory ram{ 16 };
	Bus bus{ ram, 0x1000u };

	ram.write32(0, 0x12345678u);
	assert(bus.read32(0x1000u) == 0x12345678u);

	bus.write8(0x1004u, 0xA5u);
	bus.write16(0x1006u, 0xBEEFu);
	bus.write32(0x1008u, 0x89ABCDEFu);
	assert(ram.read8(4) == 0xA5u);
	assert(ram.read16(6) == 0xBEEFu);
	assert(ram.read32(8) == 0x89ABCDEFu);

	/* The bus delegates little-endian byte layout to Memory */
	assert(ram.read8(8) == 0xEFu);
	assert(ram.read8(9) == 0xCDu);
	assert(ram.read8(10) == 0xABu);
	assert(ram.read8(11) == 0x89u);

	/* Alignment is not a bus responsibility */
	ram.write32(1, 0x0BADF00Du);
	assert(bus.read32(0x1001u) == 0x0BADF00Du);
	bus.write16(0x1003u, 0x1357u);
	assert(ram.read16(3) == 0x1357u);

	/* Addresses outside the half-open RAM mapping are rejected */
	assert_out_of_range([&bus] { static_cast<void>(bus.read8(0x0FFFu)); });
	assert_out_of_range([&bus] { static_cast<void>(bus.read8(0x1010u)); });
	assert_out_of_range([&bus] { bus.write8(0x1010u, 0xFFu); });

	/* Multi-byte accesses may not cross the upper mapping boundary */
	ram.write8(15, 0x5Au);
	assert_out_of_range([&bus] { static_cast<void>(bus.read16(0x100Fu)); });
	assert_out_of_range([&bus] { static_cast<void>(bus.read32(0x100Du)); });
	assert_out_of_range([&bus] { bus.write16(0x100Fu, 0xFFFFu); });
	assert_out_of_range([&bus] { bus.write32(0x100Du, 0xFFFFFFFFu); });
	assert(ram.read8(15) == 0x5Au);

	/* The last 32-bit physical address can hold a one-byte mapping */
	Memory top_byte_ram{ 1 };
	Bus top_byte_bus{ top_byte_ram, 0xFFFFFFFFu };
	top_byte_bus.write8(0xFFFFFFFFu, 0xC3u);
	assert(top_byte_bus.read8(0xFFFFFFFFu) == 0xC3u);
	assert(top_byte_ram.read8(0) == 0xC3u);

	/* A RAM mapping may not extend beyond the 32-bit address space */
	Memory overflowing_ram{ 2 };
	assert_out_of_range([&overflowing_ram] {
		Bus overflowing_bus{ overflowing_ram, 0xFFFFFFFFu };
		static_cast<void>(overflowing_bus);
	});

	return 0;
}
