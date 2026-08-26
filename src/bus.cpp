#include "bus.hpp"
#include "bus_device.hpp"
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

Bus::Bus() = default;

Bus::Bus(BusDevice& device, std::uint32_t device_base)
	: Bus{} 
{
	map_device(device, device_base);
}

Bus::ResolvedAccess Bus::resolve(std::uint32_t address, std::size_t width) const {
	auto start{ static_cast<std::uint64_t>(address) };
	auto end{ start + width };

	for (const auto& mapping: m_mappings) {
		if (start >= mapping.m_device_base && end <= mapping.m_device_end) {
			auto offset{ static_cast<std::uint32_t>(start - mapping.m_device_base) };
			return ResolvedAccess{
				.m_device = mapping.m_device,
				.m_offset = offset
			};
		}
	}

	throw std::out_of_range("error: unmapped bus access");
}

std::uint8_t Bus::read8(std::uint32_t address) {
	auto access{ resolve(address, 1) };
	return access.m_device->read8(access.m_offset);
}

std::uint16_t Bus::read16(std::uint32_t address) {
	auto access{ resolve(address, 2) };
	return access.m_device->read16(access.m_offset);
}

std::uint32_t Bus::read32(std::uint32_t address) {
	auto access{ resolve(address, 4) };
	return access.m_device->read32(access.m_offset);
}

void Bus::write8(std::uint32_t address, std::uint8_t value) {
	auto access{ resolve(address, 1) };
	access.m_device->write8(access.m_offset, value);
}

void Bus::write16(std::uint32_t address, std::uint16_t value) {
	auto access{ resolve(address, 2) };
	access.m_device->write16(access.m_offset, value);
}

void Bus::write32(std::uint32_t address, std::uint32_t value) {
	auto access{ resolve(address, 4) };
	access.m_device->write32(access.m_offset, value);	
}

void Bus::map_device(BusDevice& device, std::uint32_t base) {
	constexpr auto address_space{ 0x1'0000'0000ull };
	auto start{ static_cast<std::uint64_t>(base) };
	auto size{ static_cast<std::uint64_t>(device.size()) };

	if (device.size() == 0) {
		throw std::invalid_argument("error: device size is zero");
	}

	if (size > address_space - start) {
		throw std::invalid_argument("error: device mapping exceeds 32-bit address space");
	}

	auto end{ start + size };

	for (const auto& mapping: m_mappings) {
		auto existing_start{ static_cast<std::uint64_t>(mapping.m_device_base) };

		if (start < mapping.m_device_end && existing_start < end) {
			throw std::invalid_argument("error: device mappings overlap");
		}
	}
	
	Mapping mapping {
		.m_device = &device,
		.m_device_base = base,
		.m_device_end = end
	};

	m_mappings.push_back(mapping);
}

