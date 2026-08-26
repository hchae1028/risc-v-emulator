#ifndef BUS_DEVICE_H_
#define BUS_DEVICE_H_

#include <cstddef>
#include <cstdint>

class BusDevice {
public:
	virtual ~BusDevice() = default;

	virtual std::size_t size() const = 0;

	virtual std::uint8_t read8(std::uint32_t address) = 0;
	virtual std::uint16_t read16(std::uint32_t address) = 0;
	virtual std::uint32_t read32(std::uint32_t address) = 0;
	
	virtual void write8(std::uint32_t address, std::uint8_t value) = 0;
	virtual void write16(std::uint32_t address, std::uint16_t value) = 0;
	virtual void write32(std::uint32_t address, std::uint32_t value) = 0;
};

#endif
