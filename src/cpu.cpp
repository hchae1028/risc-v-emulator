#include "cpu.hpp"
#include "decoder.hpp"
#include "executor.hpp"
#include "bus.hpp"
#include "trap.hpp"
#include "instruction_trace.hpp"
#include <cstddef>
#include <cstdint>
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

std::uint32_t Cpu::read_csr(std::uint16_t address) const {
	switch (address) {
		case MTVEC_ADDRESS:
			return m_mtvec;
		case MEPC_ADDRESS:
			return m_mepc;
		case MCAUSE_ADDRESS:
			return m_mcause;
		case MTVAL_ADDRESS:
			return m_mtval;
		case MSTATUS_ADDRESS:
			return m_mstatus;
		case MIP_ADDRESS:
			return m_mip;
		case MIE_ADDRESS:
			return m_mie;
		default:
			throw std::out_of_range("error: unspported CSR address");
	}
}

void Cpu::write_csr(std::uint16_t address, std::uint32_t value) {
	switch (address) {
        case MTVEC_ADDRESS:
            m_mtvec = value & ~0x3u;
            break;
        case MEPC_ADDRESS:
            m_mepc = value & ~0x3u;
            break;
        case MCAUSE_ADDRESS:
            m_mcause = value;
            break;
        case MTVAL_ADDRESS:
            m_mtval = value;
            break;
		case MSTATUS_ADDRESS:
			m_mstatus = (value & MSTATUS_WRITABLE_MASK) | MSTATUS_MPP_MASK;
			break;
		case MIP_ADDRESS:
			break;
		case MIE_ADDRESS:
			m_mie = value & MIE_MTIE_MASK;
			break;
        default:
            throw std::out_of_range{ "error: unsupported CSR address" };
    }
}


std::uint32_t Cpu::fetch_instruction(Bus& bus) const {
	if (m_pc % 4 != 0) {
		throw Trap{
			.cause = TrapCause::InstructionAddressMisaligned,
			.tval = m_pc
		};
	}
	
	try {
		return bus.read32(m_pc);
	} catch (const std::out_of_range&) {
		throw Trap{
			.cause = TrapCause::InstructionAccessFault,
			.tval = m_pc
		};
	}
}

void Cpu::step(Bus& bus, const InstructionTraceCallBack& trace) {
	if ((m_mstatus & MSTATUS_MIE_MASK) != 0 && (m_mie & MIE_MTIE_MASK) != 0 && (m_mip & MIP_MTIP_MASK) != 0) {
		Trap m_interrupt {
			.cause = TrapCause::MachineTimerInterrupt,
			.tval = 0
		};

		take_trap(m_interrupt);
		throw m_interrupt;
	}

	try {
		auto pc{ m_pc };
		auto word{ fetch_instruction(bus) };

		if (trace) {
			trace(InstructionTrace{
				.pc = pc,
				.instruction = word
			});
		}

		auto instr{ decode_instruction(word) };
		auto returned{ execute_instruction(*this, instr, bus) };

		if (returned == std::nullopt) {
			m_pc += 4;
		}
		else {
			m_pc = *returned;
		}
	} catch (const Trap& trap) {
		take_trap(trap);
		throw;
	}
}

void Cpu::take_trap(const Trap& trap) {
	m_mepc = m_pc & ~0x3u;
	m_mcause = static_cast<std::uint32_t>(trap.cause);
	m_mtval = trap.tval;

	auto old_mie{ (m_mstatus & MSTATUS_MIE_MASK) << 4 };
	m_mstatus &= ~MSTATUS_MPIE_MASK;
	m_mstatus |= old_mie;
	m_mstatus &= ~MSTATUS_MIE_MASK;
	
	m_pc = m_mtvec;
}

void Cpu::set_machine_timer_interrupt(bool pending) {
	if (pending) {
		m_mip |= MIP_MTIP_MASK;
	}
	else {
		m_mip &= ~MIP_MTIP_MASK;
	}
}
