#include "bus.hpp"
#include "uart_device.hpp"
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
	/* The device exposes one byte-wide transmit register */
	UartDevice uart{};
	assert(uart.size() == 8);
	assert(uart.transmitted_bytes().empty());

	// Transmit data is buffered as raw bytes in write order
	uart.write8(0, static_cast<std::uint8_t>('H'));
	uart.write8(0, 0x00u);
	uart.write8(0, 0xFFu);
	assert(uart.transmitted_bytes().size() == 3);
	assert(uart.transmitted_bytes()[0] == static_cast<std::uint8_t>('H'));
	assert(uart.transmitted_bytes()[1] == 0x00u);
	assert(uart.transmitted_bytes()[2] == 0xFFu);

	// Invalid writes leave the transmit buffer unchanged
	assert_out_of_range([&uart] { uart.write8(1, 0xA5u); });
	assert_out_of_range([&uart] { uart.write16(0, 0xBEEFu); });
	assert_out_of_range([&uart] { uart.write32(0, 0x89ABCDEFu); });
	assert(uart.transmitted_bytes().size() == 3);

	// Only a word read of the status register is supported
	assert_out_of_range([&uart] { static_cast<void>(uart.read8(0)); });
	assert_out_of_range([&uart] { static_cast<void>(uart.read8(4)); });
	assert_out_of_range([&uart] { static_cast<void>(uart.read16(0)); });
	assert_out_of_range([&uart] { static_cast<void>(uart.read16(4)); });
	assert_out_of_range([&uart] { static_cast<void>(uart.read32(0)); });
	assert(uart.read32(4) == 1);
	assert(uart.transmitted_bytes().size() == 3);

	/* Bus accesses use the UART's physical mapping and local offset zero */
	constexpr std::uint32_t uart_base{ 0x10000000u };
	UartDevice mapped_uart{};
	Bus bus{};
	bus.map_device(mapped_uart, uart_base);
	bus.write8(uart_base, static_cast<std::uint8_t>('O'));
	bus.write8(uart_base, static_cast<std::uint8_t>('K'));
	assert(mapped_uart.transmitted_bytes().size() == 2);
	assert(mapped_uart.transmitted_bytes()[0] == static_cast<std::uint8_t>('O'));
	assert(mapped_uart.transmitted_bytes()[1] == static_cast<std::uint8_t>('K'));

	assert_out_of_range([&bus] { bus.write8(uart_base + 1, 0x11u); });
	assert_out_of_range([&bus] { bus.write8(uart_base + 4, 0x11u); });
	assert_out_of_range([&bus] { bus.write16(uart_base, 0x2233u); });
	assert_out_of_range([&bus] { static_cast<void>(bus.read8(uart_base)); });
	assert_out_of_range([&bus] { static_cast<void>(bus.read32(uart_base)); });
	assert(bus.read32(uart_base + 4) == 1);
	assert_out_of_range([&bus] { static_cast<void>(bus.read32(uart_base + 5)); });
	assert(mapped_uart.transmitted_bytes().size() == 2);

	return 0;
}
