#ifndef DECODER_H_
#define DECODER_H_

#include <cstdint>

struct Instruction {
	std::uint8_t opcode;
	std::uint8_t rd;
	std::uint8_t funct3;
	std::uint8_t rs1;
	std::uint8_t rs2;
	std::uint8_t funct7;
	std::uint32_t imm;
	std::uint16_t csr;
};

enum class Operation {
	Add,
	AddI,
	Sub,
	Xor,
	Or,
	And,
	XorI,
	OrI,
	AndI,
	Sll,
	Srl,
	Sra,
	SllI,
	SrlI,
	SraI,
	Slt,
	Sltu,
	SltI,
	SltIu,
	Lui,
	Lw,
	Sw,
	Lb,
	Lbu,
	Sb,
	Lh,
	Lhu,
	Sh,
	Beq,
	Bne,
	Blt,
	Bge,
	Bltu,
	Bgeu,
	Auipc,
	Jal,
	Jalr,
	Fence,
	Ecall,
	Ebreak,
	Csrrw,
	Csrrs,
	Csrrc,
	CsrrwI,
	CsrrsI,
	CsrrcI,
	Unknown
};

Instruction decode_instruction(std::uint32_t instruction_word);

Operation decode_operation(Instruction instruction);

#endif
