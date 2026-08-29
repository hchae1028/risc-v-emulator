#ifndef TIMER_DEVICE_H_
#define TIMER_DEVICE_H_

#include "bus_device.hpp"
#include <cstdint>
#include <cstddef>
#include <limits>

class TimerDevice: public BusDevice {
private:
	static constexpr std::uint32_t MTIME_LOW_OFFSET{};
	static constexpr std::uint32_t MTIME_HIGH_OFFSET{ 0x4u };
	static constexpr std::uint32_t MTIMECMP_LOW_OFFSET{ 0x8u };
	static constexpr std::uint32_t MTIMECMP_HIGH_OFFSET{ 0xCu };

	std::uint64_t m_mtime{};
	std::uint64_t m_mtimecmp{ std::numeric_limits<std::uint64_t>::max() };

public:
	TimerDevice();

	std::size_t size() const override;

	std::uint8_t read8(std::uint32_t address) override;
	std::uint16_t read16(std::uint32_t address) override;
	std::uint32_t read32(std::uint32_t address) override;

	void write8(std::uint32_t address, std::uint8_t value) override;
	void write16(std::uint32_t address, std::uint16_t value) override;
	void write32(std::uint32_t address, std::uint32_t value) override;

	void tick();
	[[nodiscard]] bool interrupt_pending() const;
};

#endif
