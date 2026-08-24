#include "memory.hpp"
#include <cstdint>
#include <stdexcept>

Memory::Memory(std::size_t size)
	: m_bytes(size)
{
}

void Memory::validate_range(std::uint32_t address, std::size_t width) const {
	if (width > m_bytes.size()) {
		throw std::out_of_range("Memory access out of range");
	}
	
	if (address > (m_bytes.size() - width)) {
		throw std::out_of_range("Memory access out of range");
	}
}

std::size_t Memory::size() const {
	return m_bytes.size();
}

[[nodiscard]] std::uint8_t Memory::read8(std::uint32_t address) const {
	validate_range(address, 1);
	return m_bytes[address];
}

void Memory::write8(std::uint32_t address, std::uint8_t value) {
	validate_range(address, 1);
	m_bytes[address] = value;
}

[[nodiscard]] std::uint32_t Memory::read32(std::uint32_t address) const {
	validate_range(address, 4);

	std::uint32_t read{};

	for (std::size_t i = 0; i < 4; i++) {
		auto byte{ static_cast<std::uint32_t>(m_bytes[address + i]) << (8 * i) };
		read |= byte;
	}

	return read;
}

void Memory::write32(std::uint32_t address, std::uint32_t value) {
	validate_range(address, 4);

	for (std::size_t i = 0; i < 4; i++) {
		auto byte{ static_cast<std::uint8_t>((value >> (8 * i)) & 0xFFu) };
		m_bytes[address + i] = byte;
	}
}

[[nodiscard]] std::uint16_t Memory::read16(std::uint32_t address) const {
	validate_range(address, 2);

	std::uint16_t read{};

	for (std::size_t i = 0; i < 2; i++) {
		auto byte{ static_cast<std::uint16_t>(m_bytes[address + i]) << (8 * i) };
		read |= byte;
	}

	return read;
}

void Memory::write16(std::uint32_t address, std::uint16_t value) {
	validate_range(address, 2);

	for (std::size_t i = 0; i < 2; i++) {
		auto byte{ static_cast<std::uint8_t>((value >> (8 * i)) & 0xFFu) };
		m_bytes[address + i] = byte;
	}
}

void Memory::load_bytes(std::uint32_t address, std::span<const std::uint8_t> bytes) {
	validate_range(address, bytes.size());

	for (std::size_t i = 0; i < bytes.size(); i++) {
		m_bytes[address + i] = bytes[i];
	}
}
