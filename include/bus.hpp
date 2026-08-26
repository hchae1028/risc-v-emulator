#ifndef BUS_H_
#define BUS_H_

#include <cstddef>
#include <cstdint>
#include <vector>

class BusDevice;

class Bus {
private:
	struct Mapping {
		BusDevice* m_device;
		std::uint32_t m_device_base;
		std::uint64_t m_device_end;
	};

	struct ResolvedAccess {
		BusDevice* m_device;
		std::uint32_t m_offset;
	};

	std::vector<Mapping> m_mappings;

	ResolvedAccess resolve(std::uint32_t address, std::size_t width) const;
	
public:
	Bus();
	Bus(BusDevice& device, std::uint32_t device_base); 

	std::uint8_t read8(std::uint32_t address);
	std::uint16_t read16(std::uint32_t address);
	std::uint32_t read32(std::uint32_t address);
		
	void write8(std::uint32_t address, std::uint8_t value);
	void write16(std::uint32_t address, std::uint16_t value);
	void write32(std::uint32_t address, std::uint32_t value);

	void map_device(BusDevice& device, std::uint32_t base);
};

#endif
