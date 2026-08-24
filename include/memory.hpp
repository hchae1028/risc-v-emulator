#ifndef MEMORY_H_
#define MEMORY_H_

#include <cstdint>
#include <cstddef>
#include <vector>
#include <span>

class Memory {
private:
	std::vector<std::uint8_t> m_bytes;

	void validate_range(std::uint32_t address, std::size_t width) const;

public:
	explicit Memory(std::size_t size);

	std::size_t size() const;

	[[nodiscard]] std::uint8_t read8(std::uint32_t address) const;
	void write8(std::uint32_t address, std::uint8_t value);

	[[nodiscard]] std::uint32_t read32(std::uint32_t address) const;
	void write32(std::uint32_t address, std::uint32_t value);

	[[nodiscard]] std::uint16_t read16(std::uint32_t address) const;
	void write16(std::uint32_t address, std::uint16_t value);

	void load_bytes(std::uint32_t address, std::span<const std::uint8_t> bytes);
};

#endif
