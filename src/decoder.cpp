#include "decoder.hpp"
#include <cstdint>

namespace {

std::uint32_t sign_extend(std::uint32_t value, unsigned bits) {
    auto sign_bit{ std::uint32_t{ 1 } << (bits - 1) };

    if ((value & sign_bit) != 0) {
        auto extension_mask{ ~((std::uint32_t{ 1 } << bits) - 1u) };
        value |= extension_mask;
    }

    return value;
}

std::uint32_t decode_i_imm(std::uint32_t instruction_word) {
    auto imm{ (instruction_word >> 20) & 0xFFFu };
    return sign_extend(imm, 12);
}

std::uint32_t decode_s_imm(std::uint32_t instruction_word) {
    auto upper{ ((instruction_word >> 25) & 0x7Fu) << 5 };
    auto lower{ (instruction_word >> 7) & 0x1Fu };

    return sign_extend(upper | lower, 12);
}

std::uint32_t decode_b_imm(std::uint32_t instruction_word) {
    auto imm12{ ((instruction_word >> 31) & 0x1u) << 12 };
    auto imm11{ ((instruction_word >> 7) & 0x1u) << 11 };
    auto imm10_5{ ((instruction_word >> 25) & 0x3Fu) << 5 };
    auto imm4_1{ ((instruction_word >> 8) & 0xFu) << 1 };

    return sign_extend(imm12 | imm11 | imm10_5 | imm4_1, 13);
}

std::uint32_t decode_u_imm(std::uint32_t instruction_word) {
    return instruction_word & 0xFFFFF000u;
}

std::uint32_t decode_j_imm(std::uint32_t instruction_word) {
	auto imm20{ ((instruction_word >> 31) & 0x1u) << 20 };
	auto imm10_1{ ((instruction_word >> 21) & 0x3FFu) << 1 };
	auto imm11{ ((instruction_word >> 20) & 0x1u) << 11 };
	auto imm19_12{ ((instruction_word >> 12) & 0xFFu) << 12 };

	return sign_extend(imm20 | imm19_12 | imm11 | imm10_1, 21);
}

}

Instruction decode_instruction(std::uint32_t instruction_word) {
	std::uint8_t opcode{ static_cast<std::uint8_t>(instruction_word & 0x7Fu) };
	std::uint8_t rd{ static_cast<std::uint8_t>((instruction_word >> 7) & 0x1Fu) };
	std::uint8_t funct3{ static_cast<std::uint8_t>((instruction_word >> 12) & 0x7u) };
	std::uint8_t rs1{ static_cast<std::uint8_t>((instruction_word >> 15) & 0x1Fu) };
	std::uint8_t rs2{ static_cast<std::uint8_t>((instruction_word >> 20) & 0x1Fu) };
	std::uint8_t funct7{ static_cast<std::uint8_t>((instruction_word >> 25) & 0x7Fu) };
	std::uint32_t imm{};
	std::uint16_t csr{ static_cast<std::uint16_t>((instruction_word >> 20) & 0xFFFu) };
	
	switch (opcode) {
        case 0x03:
        case 0x13:
		case 0x67:
		case 0x73:
            imm = decode_i_imm(instruction_word);
            break;

        case 0x23:
            imm = decode_s_imm(instruction_word);
            break;

        case 0x63:
            imm = decode_b_imm(instruction_word);
            break;

		case 0x6F:
			imm = decode_j_imm(instruction_word);
			break;

		case 0x17:
        case 0x37:
            imm = decode_u_imm(instruction_word);
            break;

        default:
            break;
    }

	return Instruction {
		.opcode = opcode,
		.rd = rd,
		.funct3 = funct3,
		.rs1 = rs1,
		.rs2 = rs2,
		.funct7 = funct7,
		.imm = imm,
		.csr = csr,
		.word = instruction_word
	};
}

Operation decode_operation(Instruction instruction) {
	switch (instruction.opcode) {
		case 0x33: {
			if (instruction.funct7 == 0) {
				if (instruction.funct3 == 0) {
					return Operation::Add;
				}
				else if (instruction.funct3 == 4) {
					return Operation::Xor;
				}
				else if (instruction.funct3 == 6) {
					return Operation::Or;
				}
				else if (instruction.funct3 == 7) {
					return Operation::And;
				}
				else if (instruction.funct3 == 1) {
					return Operation::Sll;
				}
				else if (instruction.funct3 == 5) {
					return Operation::Srl;
				}
				else if (instruction.funct3 == 2) {
					return Operation::Slt;
				}
				else if (instruction.funct3 == 3) {
					return Operation::Sltu;
				}
			}
			else if (instruction.funct7 == 0x20) {
				if (instruction.funct3 == 0) {
					return Operation::Sub;
				}
				else if (instruction.funct3 == 5) {
					return Operation::Sra;
				}
			}

			break;
		}

		case 0x13: {
			if (instruction.funct3 == 0) {
				return Operation::AddI;
			}
			else if (instruction.funct3 == 4) {
				return Operation::XorI;
			}
			else if (instruction.funct3 == 6) {
				return Operation::OrI;
			}
			else if (instruction.funct3 == 7) {
				return Operation::AndI;
			}
			else if (instruction.funct3 == 2) {
				return Operation::SltI;
			}
			else if (instruction.funct3 == 3) {
				return Operation::SltIu;
			}
			else if (instruction.funct3 == 1 && instruction.funct7 == 0) {
				return Operation::SllI;
			}
			else if (instruction.funct3 == 5 && instruction.funct7 == 0) {
				return Operation::SrlI;
			}
			else if (instruction.funct3 == 5 && instruction.funct7 == 0x20) {
				return Operation::SraI;
			}

			break;
		}

		case 3: {
			if (instruction.funct3 == 2) {
				return Operation::Lw;
			}
			else if (instruction.funct3 == 0) {
				return Operation::Lb;
			}
			else if (instruction.funct3 == 4) {
				return Operation::Lbu;
			}
			else if (instruction.funct3 == 1) {
				return Operation::Lh;
			}
			else if (instruction.funct3 == 5) {
				return Operation::Lhu;
			}

			break;
		}

		case 0x23: {
			if (instruction.funct3 == 2) {
				return Operation::Sw;
			}
			else if (instruction.funct3 == 0) {
				return Operation::Sb;
			}
			else if (instruction.funct3 == 1) {
				return Operation::Sh;
			}

			break;
		}

		case 0x63: {
			if (instruction.funct3 == 0) {
				return Operation::Beq;
			}
			else if (instruction.funct3 == 1) {
				return Operation::Bne;
			}
			else if (instruction.funct3 == 4) {
				return Operation::Blt;
			}
			else if (instruction.funct3 == 5) {
				return Operation::Bge;
			}
			else if (instruction.funct3 == 6) {
				return Operation::Bltu;
			}
			else if (instruction.funct3 == 7) {
				return Operation::Bgeu;
			}

			break;
		}

		case 0x67: {
			if (instruction.funct3 == 0) {
				return Operation::Jalr;
			}

			break;
		}

		case 0x6F: {
			return Operation::Jal;
			break;
		}

		case 0x37: {
			return Operation::Lui;
			break;
		}

		case 0x17: {
			return Operation::Auipc;
			break;
		}

		case 0x0F: {
			if (instruction.funct3 == 0) {
				return Operation::Fence;
			}
			break;
		}

		case 0x73: {
			switch (instruction.funct3) {
				case 0x0: {
					if (instruction.rd == 0 && instruction.rs1 == 0 && instruction.imm == 0) {
						return Operation::Ecall;
					}
					else if (instruction.rd == 0 && instruction.rs1 == 0 && instruction.imm == 1) {
						return Operation::Ebreak;
					}
					else if (instruction.csr == 0x302u && instruction.rs1 == 0 && instruction.rd == 0) {
						return Operation::Mret;
					}
					break;
				}

				case 0x1:
					return Operation::Csrrw;
				case 0x2: 
					return Operation::Csrrs;
				case 0x3: 
					return Operation::Csrrc;
				case 0x5: 
					return Operation::CsrrwI;
				case 0x6: 
					return Operation::CsrrsI;
				case 0x7: 
					return Operation::CsrrcI;
			}
		}
	}
	return Operation::Unknown;
}
