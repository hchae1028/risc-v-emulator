#include "cpu.hpp"
#include "decoder.hpp"
#include "executor.hpp"
#include "memory.hpp"
#include <optional>
#include <stdexcept>

Cpu::Cpu() = default;

std::uint32_t Cpu::read_register(std::size_t index) const {
		return m_registers.at(index);
}

void Cpu::write_register(std::size_t index, std::uint32_t value) {
	if (index == 0) {
		return;
	}

	m_registers.at(index) = value;
}

std::uint32_t Cpu::fetch_instruction(const Memory& memory) const {
	if (m_pc % 4 != 0) {
		throw Trap{ TrapCause::InstructionAddressMisaligned };
	}
	
	try {
		return memory.read32(m_pc);
	} catch (const std::out_of_range&) {
		throw Trap{ TrapCause::InstructionAccessFault };
	}
}

void Cpu::step(Memory& memory) {
	auto word{ fetch_instruction(memory) };
	auto instr{ decode_instruction(word) };

	auto returned{ execute_instruction(*this, instr, memory) };
	if (returned == std::nullopt) {
		m_pc += 4;
	}
	else {
		m_pc = *returned;
	}
}
