#include "executor.hpp"
#include "bus.hpp"
#include "cpu.hpp"
#include "trap.hpp"
#include <bit>
#include <cstdint>
#include <optional>
#include <stdexcept>

namespace {

constexpr std::uint16_t MSTATUS_ADDRESS{ 0x300u };
constexpr std::uint16_t MEPC_ADDRESS{ 0x341u };

constexpr std::uint32_t MSTATUS_MIE_MASK{ 1 << 3 };
constexpr std::uint32_t MSTATUS_MPIE_MASK{ 1 << 7 };

std::uint32_t read_csr_or_trap(Cpu& cpu, std::uint16_t address, std::uint32_t word) {
	try {
		return cpu.read_csr(address);
	} catch (const std::out_of_range&) {
		throw Trap{ 
			.cause = TrapCause::IllegalInstruction,
			.tval = word
		};
	}
}

void write_csr_or_trap(Cpu& cpu, std::uint16_t address, std::uint32_t value, std::uint32_t word) {
	try {
		cpu.write_csr(address, value);
	} catch (const std::out_of_range&) {
		throw Trap{ 
			.cause = TrapCause::IllegalInstruction,
			.tval = word
		};
	}
}

}

std::optional<std::uint32_t> execute_instruction(Cpu& cpu, const Instruction& instruction, Bus& bus) {
	switch (decode_operation(instruction)) {
		case Operation::Add: {
			auto result{ cpu.read_register(instruction.rs1) + cpu.read_register(instruction.rs2) };
			cpu.write_register(instruction.rd, result);
			break;
		}
		
		case Operation::AddI: {
			auto result{ cpu.read_register(instruction.rs1) + instruction.imm };
			cpu.write_register(instruction.rd, result);
			break;
		}

		case Operation::Sub: {
			auto result{ cpu.read_register(instruction.rs1) - cpu.read_register(instruction.rs2) };
			cpu.write_register(instruction.rd, result);
			break;
		}

		case Operation::Xor: {
			auto result{ cpu.read_register(instruction.rs1) ^ cpu.read_register(instruction.rs2) };
			cpu.write_register(instruction.rd, result);
			break;
		}

		case Operation::Or: {
			auto result{ cpu.read_register(instruction.rs1) | cpu.read_register(instruction.rs2) };
			cpu.write_register(instruction.rd, result);
			break;
		}

		case Operation::And: {
			auto result{ cpu.read_register(instruction.rs1) & cpu.read_register(instruction.rs2) };
			cpu.write_register(instruction.rd, result);
			break;
		}

		case Operation::XorI: {
			auto result{ cpu.read_register(instruction.rs1) ^ instruction.imm };
			cpu.write_register(instruction.rd, result);
			break;
		}

		case Operation::OrI: {
			auto result{ cpu.read_register(instruction.rs1) | instruction.imm };
			cpu.write_register(instruction.rd, result);
			break;
		}

		case Operation::AndI: {
			auto result{ cpu.read_register(instruction.rs1) & instruction.imm };
			cpu.write_register(instruction.rd, result);
			break;
		}

		case Operation::Sll: {
			std::uint32_t shift = cpu.read_register(instruction.rs2) & 0x1F;
			auto result{ cpu.read_register(instruction.rs1) << shift };
			cpu.write_register(instruction.rd, result);
			break;
		}

		case Operation::Srl: {
			std::uint32_t shift = cpu.read_register(instruction.rs2) & 0x1F;
			auto result{ cpu.read_register(instruction.rs1) >> shift };
			cpu.write_register(instruction.rd, result);
			break;
		}

		case Operation::Sra: {
			std::uint32_t shift = cpu.read_register(instruction.rs2) & 0x1F;
			auto signed_value{ std::bit_cast<std::int32_t>(cpu.read_register(instruction.rs1)) };
			auto result{ std::bit_cast<std::uint32_t>(signed_value >> shift) };
			cpu.write_register(instruction.rd, result);
			break;
		}

		case Operation::SllI: {
			std::uint32_t shift = instruction.imm & 0x1F;
			auto result{ cpu.read_register(instruction.rs1) << shift };
			cpu.write_register(instruction.rd, result);
			break;
		}

		case Operation::SrlI: {
			std::uint32_t shift = instruction.imm & 0x1F;
			auto result{ cpu.read_register(instruction.rs1) >> shift };
			cpu.write_register(instruction.rd, result);
			break;
		}

		case Operation::SraI: {
			std::uint32_t shift = instruction.imm & 0x1F;
			auto signed_value{ std::bit_cast<std::int32_t>(cpu.read_register(instruction.rs1)) };
			auto result{ std::bit_cast<std::uint32_t>(signed_value >> shift) };
			cpu.write_register(instruction.rd, result);
			break;
		}

		case Operation::Slt: {	
			auto signed_rs1{ std::bit_cast<std::int32_t>(cpu.read_register(instruction.rs1)) };
			auto signed_rs2{ std::bit_cast<std::int32_t>(cpu.read_register(instruction.rs2)) };
			auto result{ static_cast<std::uint32_t>(signed_rs1 < signed_rs2) };
			cpu.write_register(instruction.rd, result);
			break;
		}

		case Operation::Sltu: {
			auto result{ static_cast<std::uint32_t>(cpu.read_register(instruction.rs1) < cpu.read_register(instruction.rs2)) };
			cpu.write_register(instruction.rd, result);
			break;
		}

		case Operation::SltI: {
			auto signed_imm{ std::bit_cast<std::int32_t>(instruction.imm) };
			auto signed_rs1{ std::bit_cast<std::int32_t>(cpu.read_register(instruction.rs1)) };
			auto result{ static_cast<std::uint32_t>(signed_rs1 < signed_imm) };
			cpu.write_register(instruction.rd, result);
			break;
		}

		case Operation::SltIu: {
			auto result{ static_cast<std::uint32_t>(cpu.read_register(instruction.rs1) < instruction.imm) };
			cpu.write_register(instruction.rd, result);
			break;
		}

		case Operation::Lui: {
			cpu.write_register(instruction.rd, instruction.imm);
			break;
		}

		case Operation::Lw: {
			auto address{ cpu.read_register(instruction.rs1) + instruction.imm };
			if (address % 4 != 0) {
				throw Trap{
					.cause = TrapCause::LoadAddressMisaligned,
					.tval = address
				};
			}
			
			std::uint32_t value{};
			try {
				value = bus.read32(address);
			} catch (const std::out_of_range&) {
				throw Trap{
					.cause = TrapCause::LoadAccessFault,
					.tval = address
				};
			}

			cpu.write_register(instruction.rd, value);
			break;
		}

		case Operation::Sw: {
			auto address{ cpu.read_register(instruction.rs1) + instruction.imm };
			if (address % 4 != 0) {
				throw Trap{
					.cause = TrapCause::StoreAddressMisaligned, 
					.tval = address
				};
			}
			
			auto rs2{ cpu.read_register(instruction.rs2) };
			try {
				bus.write32(address, rs2);
			} catch (const std::out_of_range&) {
				throw Trap{
					.cause = TrapCause::StoreAccessFault,
					.tval = address
				};
			}
			break;
		}

		case Operation::Lb: {
			auto address{ cpu.read_register(instruction.rs1) + instruction.imm };
			std::uint32_t result{};

			try {
				result = bus.read8(address);
			} catch (const std::out_of_range&) {
				throw Trap{
					.cause = TrapCause::LoadAccessFault,
					.tval = address
				};
			}

			if ((result & 0x80u) != 0) {
				result |= 0xFFFFFF00u;
			}
		
			cpu.write_register(instruction.rd, result);
			break;
		}

		case Operation::Lbu: {
			auto address{ cpu.read_register(instruction.rs1) + instruction.imm };
			std::uint32_t result{};

			try {
				result = bus.read8(address);
			} catch (const std::out_of_range&) {
				throw Trap{
					.cause = TrapCause::LoadAccessFault,
					.tval = address
				};
			}

			cpu.write_register(instruction.rd, result);
			break;
		}

		case Operation::Sb: {
			auto address{ cpu.read_register(instruction.rs1) + instruction.imm };
			
			std::uint8_t value{};
			auto rs2{ cpu.read_register(instruction.rs2) };
 			try {
				value = static_cast<std::uint8_t>(rs2);
				bus.write8(address, value);
			} catch (const std::out_of_range&) {
				throw Trap{
					.cause = TrapCause::StoreAccessFault,
					.tval = address
				};
			}

			break;
		}

		case Operation::Lh: {
			auto address{ cpu.read_register(instruction.rs1) + instruction.imm };
			if (address % 2 != 0) {
				throw Trap{
					.cause = TrapCause::LoadAddressMisaligned,
					.tval = address
				};
			}

			std::uint32_t result{};
			try {
				result = bus.read16(address);
			} catch (const std::out_of_range&) {
				throw Trap{
					.cause = TrapCause::LoadAccessFault,
					.tval = address
				};
			}

			if ((result & 0x8000u) != 0) {
				result |= 0xFFFF0000u;
			}

			cpu.write_register(instruction.rd, result);
			break;
		}

		case Operation::Lhu: {
			auto address{ cpu.read_register(instruction.rs1) + instruction.imm };
			if (address % 2 != 0) {
				throw Trap{ 
					.cause = TrapCause::LoadAddressMisaligned,
					.tval = address
				};
			}

			std::uint32_t result{};
			try {
				result = bus.read16(address);
			} catch (const std::out_of_range&) {
				throw Trap{
					.cause = TrapCause::LoadAccessFault,
					.tval = address
				};
			}	

			cpu.write_register(instruction.rd, result);
			break;
		}

		case Operation::Sh: {
			auto address{ cpu.read_register(instruction.rs1) + instruction.imm };
			if (address % 2 != 0) {
				throw Trap{
					.cause = TrapCause::StoreAddressMisaligned,
					.tval = address
				};
			}

			std::uint16_t value{};
			auto rs2{ cpu.read_register(instruction.rs2) };
			try {
				value = static_cast<std::uint16_t>(rs2);
				bus.write16(address, value);
			} catch (const std::out_of_range&) {
				throw Trap{
					.cause = TrapCause::StoreAccessFault,
					.tval = address
				};
			}

			break;
		}

		case Operation::Beq: {
			bool branch_taken{ cpu.read_register(instruction.rs1) == cpu.read_register(instruction.rs2) };
			if (branch_taken) {
				auto new_pc{ cpu.read_pc() + instruction.imm };
				if (new_pc % 4 != 0) {
					throw Trap{
						.cause = TrapCause::InstructionAddressMisaligned,
						.tval = new_pc
					};
				}

				return new_pc;
			}

			break;
		}

		case Operation::Bne: {
			bool branch_taken{ cpu.read_register(instruction.rs1) != cpu.read_register(instruction.rs2) };
			if (branch_taken) {
				auto new_pc{ cpu.read_pc() + instruction.imm };
				if (new_pc % 4 != 0) {
					throw Trap{
						.cause = TrapCause::InstructionAddressMisaligned,
						.tval = new_pc
					};
				}

				return new_pc;
			}

			break;
		}

		case Operation::Blt: {
			auto signed_rs1{ std::bit_cast<std::int32_t>(cpu.read_register(instruction.rs1)) };
			auto signed_rs2{ std::bit_cast<std::int32_t>(cpu.read_register(instruction.rs2)) };

			bool branch_taken{ signed_rs1 < signed_rs2 };
			if (branch_taken) {
				auto new_pc{ cpu.read_pc() + instruction.imm };
				if (new_pc % 4 != 0) {
					throw Trap{
						.cause = TrapCause::InstructionAddressMisaligned,
						.tval = new_pc
					};
				}

				return new_pc;
			}

			break;
		}

		case Operation::Bge: {
			auto signed_rs1{ std::bit_cast<std::int32_t>(cpu.read_register(instruction.rs1)) };
			auto signed_rs2{ std::bit_cast<std::int32_t>(cpu.read_register(instruction.rs2)) };

			bool branch_taken{ signed_rs1 >= signed_rs2 };
			if (branch_taken) {
				auto new_pc{ cpu.read_pc() + instruction.imm };
				if (new_pc % 4 != 0) {
					throw Trap{
						.cause = TrapCause::InstructionAddressMisaligned,
						.tval = new_pc
					};
				}

				return new_pc;
			}

			break;
		}

		case Operation::Bltu: {
			bool branch_taken{ cpu.read_register(instruction.rs1) < cpu.read_register(instruction.rs2) };
			if (branch_taken) {
				auto new_pc{ cpu.read_pc() + instruction.imm };
				if (new_pc % 4 != 0) {
					throw Trap{
						.cause = TrapCause::InstructionAddressMisaligned,
						.tval = new_pc
					};
				}

				return new_pc;
			}
			break;
		}

		case Operation::Bgeu: {
			bool branch_taken{ cpu.read_register(instruction.rs1) >= cpu.read_register(instruction.rs2) };
			if (branch_taken) {
				auto new_pc{ cpu.read_pc() + instruction.imm };
				if (new_pc % 4 != 0) {
					throw Trap{
						.cause = TrapCause::InstructionAddressMisaligned,
						.tval = new_pc
					};
				}

				return new_pc;
			}
			break;
		}

		case Operation::Auipc: {
			cpu.write_register(instruction.rd, cpu.read_pc() + instruction.imm);
			break;
		}

		case Operation::Jal: {
			auto target{ cpu.read_pc() + instruction.imm };
			if (target % 4 != 0) {
				throw Trap{
					.cause = TrapCause::InstructionAddressMisaligned,
					.tval = target
				};
			}

			cpu.write_register(instruction.rd, cpu.read_pc() + 4);
			return target;
		}

		case Operation::Jalr: {
			auto target{ cpu.read_register(instruction.rs1) + instruction.imm };
			target &= ~(std::uint32_t{ 1 });
			if (target % 4 != 0) {
				throw Trap{
					.cause = TrapCause::InstructionAddressMisaligned,
					.tval = target
				};
			}

			cpu.write_register(instruction.rd, cpu.read_pc() + 4);
			return target;
		}

		case Operation::Fence: {
			return std::nullopt;
		}

		case Operation::Ecall: {
			throw Trap{
				.cause = TrapCause::EnvironmentCall,
				.tval = 0
			};
		}

		case Operation::Ebreak: {
			throw Trap{ 
				.cause = TrapCause::BreakPoint,
				.tval = cpu.read_pc()
			};
		}
		
		case Operation::Csrrw: {
			auto source{ cpu.read_register(instruction.rs1) };

			if (instruction.rd == 0) {
				write_csr_or_trap(cpu, instruction.csr, source, instruction.word);
				break;
			}

			auto old{ read_csr_or_trap(cpu, instruction.csr, instruction.word) };
			write_csr_or_trap(cpu, instruction.csr, source, instruction.word);
			cpu.write_register(instruction.rd, old);
			break;
		}

		case Operation::Csrrs: {
			auto old{ read_csr_or_trap(cpu, instruction.csr, instruction.word) };

			if (instruction.rs1 != 0) {
				auto source{ cpu.read_register(instruction.rs1) };
				write_csr_or_trap(cpu, instruction.csr, source | old, instruction.word);
			}

			cpu.write_register(instruction.rd, old);
			break;
		}

		case Operation::Csrrc: {
			auto old{ read_csr_or_trap(cpu, instruction.csr, instruction.word) };

			if (instruction.rs1 != 0) {
				auto source{ cpu.read_register(instruction.rs1) };
				write_csr_or_trap(cpu, instruction.csr, old & ~source, instruction.word);
			}

			cpu.write_register(instruction.rd, old);
			break;
		}

		case Operation::CsrrwI: {
			auto source{ static_cast<std::uint32_t>(instruction.rs1) };

			if (instruction.rd == 0) {
				write_csr_or_trap(cpu, instruction.csr, source, instruction.word);
				break;
			}

			auto old{ read_csr_or_trap(cpu, instruction.csr, instruction.word) };
			write_csr_or_trap(cpu, instruction.csr, source, instruction.word);
			cpu.write_register(instruction.rd, old);
			break;
		}

		case Operation::CsrrsI: {
			auto old{ read_csr_or_trap(cpu, instruction.csr, instruction.word) };
			auto source{ static_cast<std::uint32_t>(instruction.rs1) };

			if (source != 0) {
				write_csr_or_trap(cpu, instruction.csr, old | source, instruction.word);
			}

			cpu.write_register(instruction.rd, old);
			break;
		}

		case Operation::CsrrcI: {
			auto old{ read_csr_or_trap(cpu, instruction.csr, instruction.word) };
			auto source{ static_cast<std::uint32_t>(instruction.rs1) };

			if (source != 0) {
				write_csr_or_trap(cpu, instruction.csr, old & ~source, instruction.word);
			}

			cpu.write_register(instruction.rd, old);
			break;
		}

		case Operation::Mret: {
			auto mstatus{ cpu.read_csr(MSTATUS_ADDRESS) };

			if ((mstatus & MSTATUS_MPIE_MASK) != 0) {
				mstatus |= MSTATUS_MIE_MASK;
			}
			else {
				mstatus &= ~MSTATUS_MIE_MASK;
			}

			mstatus |= MSTATUS_MPIE_MASK;
			cpu.write_csr(MSTATUS_ADDRESS, mstatus);
			
			return cpu.read_csr(MEPC_ADDRESS);
		}

		case Operation::Unknown: {
			throw Trap{ 
				.cause = TrapCause::IllegalInstruction,
				.tval = instruction.word
			};
		}

	}
	return std::nullopt;
}
