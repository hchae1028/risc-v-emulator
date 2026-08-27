#ifndef UART_DEVICE_H_
#define UART_DEVICE_H_

#include "bus_device.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

class UartDevice: public BusDevice {
private:
	static constexpr std::uint32_t TX_DATA_OFFSET{};
	static constexpr std::uint32_t STATUS_OFFSET{ 4 };
	static constexpr std::uint32_t TX_READY_MASK{ 1 };

	std::vector<std::uint8_t> m_transmitted_bytes;

public:
	UartDevice();

	std::size_t size() const override;

	std::uint8_t read8(std::uint32_t address) override;
	std::uint16_t read16(std::uint32_t address) override;
	std::uint32_t read32(std::uint32_t address) override;
		
	void write8(std::uint32_t address, std::uint8_t value) override;
	void write16(std::uint32_t address, std::uint16_t value) override;
	void write32(std::uint32_t address, std::uint32_t value) override;

	[[nodiscard]] const std::vector<std::uint8_t>& transmitted_bytes() const;
};

#endif
