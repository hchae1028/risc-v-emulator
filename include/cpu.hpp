#ifndef CPU_H_
#define CPU_H_

#include <cstdint>
#include <cstddef>
#include <array>

class Bus;
struct Trap;

class Cpu {
private:
	std::uint32_t m_pc{};
	std::array<std::uint32_t, 32> m_registers{};

	static constexpr std::uint16_t MTVEC_ADDRESS { 0x305u };
	static constexpr std::uint16_t MEPC_ADDRESS { 0x341u };
	static constexpr std::uint16_t MCAUSE_ADDRESS { 0x342u };
	static constexpr std::uint16_t MTVAL_ADDRESS { 0x343u };
	static constexpr std::uint16_t MSTATUS_ADDRESS { 0x300u };

	static constexpr std::uint32_t MSTATUS_MIE_MASK{ 1 << 3 };
	static constexpr std::uint32_t MSTATUS_MPIE_MASK{ 1 << 7 };
	static constexpr std::uint32_t MSTATUS_MPP_MASK{ 0b11 << 11 };
	static constexpr std::uint32_t MSTATUS_WRITABLE_MASK{ MSTATUS_MIE_MASK | MSTATUS_MPIE_MASK };
	
	std::uint32_t m_mtvec{};
	std::uint32_t m_mepc{};
	std::uint32_t m_mcause{};
	std::uint32_t m_mtval{};
	std::uint32_t m_mstatus{ MSTATUS_MPP_MASK };

public:
	Cpu();

	std::uint32_t read_register(std::size_t index) const;
	void write_register(std::size_t index, std::uint32_t value);

	std::uint32_t read_csr(std::uint16_t address) const;
	void write_csr(std::uint16_t address, std::uint32_t value);

	std::uint32_t read_pc() const {
		return m_pc;
	}

	void set_pc(std::uint32_t pc) {
		m_pc = pc;
	}

	std::uint32_t fetch_instruction(Bus& bus) const;

	void step(Bus& bus);

	void take_trap(const Trap& trap);
};

#endif
