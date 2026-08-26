#ifndef CPU_H_
#define CPU_H_

#include <cstdint>
#include <cstddef>
#include <array>

class Bus;

class Cpu {
private:
	std::uint32_t m_pc{};
	std::array<std::uint32_t, 32> m_registers{};

public:
	Cpu();

	std::uint32_t read_register(std::size_t index) const;

	void write_register(std::size_t index, std::uint32_t value);

	std::uint32_t read_pc() const {
		return m_pc;
	}

	void set_pc(std::uint32_t pc) {
		m_pc = pc;
	}

	std::uint32_t fetch_instruction(const Bus& bus) const;

	void step(Bus& bus);
};

#endif
