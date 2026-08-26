#ifndef BUS_H_
#define BUS_H_

#include <cstddef>
#include <cstdint>

class Memory;

class Bus {
private:
	Memory& m_ram;
	std::uint32_t m_ram_base;
	std::uint64_t m_ram_end;

	std::uint32_t translate(std::uint32_t address, std::size_t width) const;

public:
	Bus(Memory& ram, std::uint32_t ram_base); 

	std::uint8_t read8(std::uint32_t address) const;
	std::uint16_t read16(std::uint32_t address) const;
	std::uint32_t read32(std::uint32_t address) const;
		
	void write8(std::uint32_t address, std::uint8_t value);
	void write16(std::uint32_t address, std::uint16_t value);
	void write32(std::uint32_t address, std::uint32_t value);
};

#endif
