#include "uart_device.hpp"
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

UartDevice::UartDevice() = default;

std::size_t UartDevice::size() const {
	return 8;
}

std::uint8_t UartDevice::read8(std::uint32_t) {
	throw std::out_of_range("error: invalid UART write");
}

std::uint16_t UartDevice::read16(std::uint32_t) {
	throw std::out_of_range("error: invalid UART write");
}

std::uint32_t UartDevice::read32(std::uint32_t address) {
	if (address != STATUS_OFFSET) {
		throw std::out_of_range("error: invalid UART write");
	}

	return TX_READY_MASK;
}

void UartDevice::write8(std::uint32_t address, std::uint8_t value) {
	if (address != TX_DATA_OFFSET) {
		throw std::out_of_range("error: writes only to transmit register at zero offset are allowed");
	}
	m_transmitted_bytes.push_back(value);
}

void UartDevice::write16(std::uint32_t, std::uint16_t) {
	throw std::out_of_range("error: only 1-byte writes are allowed for transmit register");
}

void UartDevice::write32(std::uint32_t, std::uint32_t) {
	throw std::out_of_range("error: only 1-byte writes are allowed for transmit register");
}

[[nodiscard]] const std::vector<std::uint8_t>& UartDevice::transmitted_bytes() const {
	return m_transmitted_bytes;
}
