#include "bus.hpp"
#include "memory.hpp"
#include <cstddef>
#include <cstdint>
#include <stdexcept>

Bus::Bus(Memory& ram, std::uint32_t ram_base)
	: m_ram{ ram },
	  m_ram_base{ ram_base },
	  m_ram_end{ static_cast<std::uint64_t>(ram_base) + ram.size() }
{
	if (m_ram_end > 0x1'0000'0000ull) {
		throw std::out_of_range{ "error: RAM mapping exceeds 32-bit address space" };
	}
}

std::uint32_t Bus::translate(std::uint32_t address, std::size_t width) const {
	auto start{ static_cast<std::uint64_t>(address) };
	auto end{ start + width };

	if (start < m_ram_base || end > m_ram_end) {
		throw std::out_of_range("error: unmapped bus access");
	}

	return static_cast<std::uint32_t>(start - m_ram_base);
}

std::uint8_t Bus::read8(std::uint32_t address) const {
	auto offset{ translate(address, 1) };
	return m_ram.read8(offset);
}

std::uint16_t Bus::read16(std::uint32_t address) const {
	auto offset{ translate(address, 2) };
	return m_ram.read16(offset);
}

std::uint32_t Bus::read32(std::uint32_t address) const {
	auto offset{ translate(address, 4) };
	return m_ram.read32(offset);
}

void Bus::write8(std::uint32_t address, std::uint8_t value) {
	auto offset{ translate(address, 1) };
	m_ram.write8(offset, value);
}

void Bus::write16(std::uint32_t address, std::uint16_t value) {
	auto offset{ translate(address, 2) };
	m_ram.write16(offset, value);
}

void Bus::write32(std::uint32_t address, std::uint32_t value) {
	auto offset{ translate(address, 4) };
	m_ram.write32(offset, value);
}
