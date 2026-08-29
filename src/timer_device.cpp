#include "timer_device.hpp"
#include <cstddef>
#include <cstdint>
#include <stdexcept>

TimerDevice::TimerDevice() = default;

std::size_t TimerDevice::size() const {
	return 16;
}

std::uint8_t TimerDevice::read8(std::uint32_t) {
	throw std::out_of_range("error: invalid timer read");
}

std::uint16_t TimerDevice::read16(std::uint32_t) {
	throw std::out_of_range("error: invalid timer read");
}

std::uint32_t TimerDevice::read32(std::uint32_t address) {
	switch (address) {
		case MTIME_LOW_OFFSET:
			return static_cast<std::uint32_t>(m_mtime);
		case MTIME_HIGH_OFFSET:
			return static_cast<std::uint32_t>(m_mtime >> 32);
		case MTIMECMP_LOW_OFFSET:
			return static_cast<std::uint32_t>(m_mtimecmp);
		case MTIMECMP_HIGH_OFFSET:
			return static_cast<std::uint32_t>(m_mtimecmp >> 32);
		default:
			throw std::out_of_range("error: invalid timer read");
	}
}

void TimerDevice::write8(std::uint32_t, std::uint8_t) {
	throw std::out_of_range("error: invalid timer write");
}

void TimerDevice::write16(std::uint32_t, std::uint16_t) {
	throw std::out_of_range("error: invalid timer write");
}

void TimerDevice::write32(std::uint32_t address, std::uint32_t value) {
	switch (address) {
		case MTIME_LOW_OFFSET: {
			m_mtime = (m_mtime & 0xFFFFFFFF00000000ull) | static_cast<std::uint64_t>(value);
			break;
		}
		case MTIME_HIGH_OFFSET: {
			m_mtime = (m_mtime & 0x00000000FFFFFFFFull) | (static_cast<std::uint64_t>(value) << 32);
			break;
		}
		case MTIMECMP_LOW_OFFSET: {
			m_mtimecmp = (m_mtimecmp & 0xFFFFFFFF00000000ull) | static_cast<std::uint64_t>(value);
			break;
		}
		case MTIMECMP_HIGH_OFFSET: {
			m_mtimecmp = (m_mtimecmp & 0x00000000FFFFFFFFull) | (static_cast<std::uint64_t>(value) << 32);
			break;
		}
		default:
			throw std::out_of_range("error: invalid timer write");

	}
}

void TimerDevice::tick() {
	m_mtime++;
}

[[nodiscard]] bool TimerDevice::interrupt_pending() const {
	return m_mtime >= m_mtimecmp;
}
