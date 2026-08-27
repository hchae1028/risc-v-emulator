#include "bus.hpp"
#include "cpu.hpp"
#include "executor.hpp"
#include "memory.hpp"
#include "runner.hpp"
#include "uart_device.hpp"
#include <cassert>
#include <cstdint>

int main() {
	constexpr std::uint32_t uart_base{ 0x10000000u };
	Memory ram{ 32 };
	UartDevice uart{};
	Bus bus{};
	bus.map_device(ram, 0);
	bus.map_device(uart, uart_base);

	// Build the UART address, transmit "Hi\n", then stop.
	ram.write32(0, 0x100000B7u);  // LUI  x1, 0x10000
	ram.write32(4, 0x04800113u);  // ADDI x2, x0, 'H'
	ram.write32(8, 0x00208023u);  // SB   x2, 0(x1)
	ram.write32(12, 0x06900113u); // ADDI x2, x0, 'i'
	ram.write32(16, 0x00208023u); // SB   x2, 0(x1)
	ram.write32(20, 0x00A00113u); // ADDI x2, x0, '\n'
	ram.write32(24, 0x00208023u); // SB   x2, 0(x1)
	ram.write32(28, 0x00100073u); // EBREAK

	Cpu cpu{};
	const auto result{ run_until_trap(cpu, bus, 16) };
	assert(result.trap.has_value());
	assert(*result.trap == TrapCause::BreakPoint);
	assert(result.instructions_retired == 7);
	assert(cpu.read_pc() == 28);
	assert(cpu.read_register(1) == uart_base);
	assert(cpu.read_register(2) == 10);

	assert(uart.transmitted_bytes().size() == 3);
	assert(uart.transmitted_bytes()[0] == static_cast<std::uint8_t>('H'));
	assert(uart.transmitted_bytes()[1] == static_cast<std::uint8_t>('i'));
	assert(uart.transmitted_bytes()[2] == static_cast<std::uint8_t>('\n'));

	// MMIO writes do not alter program RAM.
	assert(ram.read32(8) == 0x00208023u);
	assert(ram.read32(16) == 0x00208023u);
	assert(ram.read32(24) == 0x00208023u);

	return 0;
}
