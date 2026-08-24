#include "decoder.hpp"
#include <cstdint>
#include <cassert>

int main() {
	Instruction instr = decode_instruction(0x002081B3);
	assert(instr.opcode == 0x33u);
	assert(instr.rd == 3);
	assert(instr.funct3 == 0);
	assert(instr.rs1 == 1);
	assert(instr.rs2 == 2);
	assert(instr.funct7 == 0);

	instr = decode_instruction(0x41DF5FB3);
	assert(instr.opcode == 0x33u);
	assert(instr.rd == 0x1Fu);
	assert(instr.funct3 == 5);
	assert(instr.rs1 == 0x1Eu);
	assert(instr.rs2 == 0x1Du);
	assert(instr.funct7 == 0x20);

	instr = decode_instruction(0);
	assert(instr.opcode == 0);
	assert(instr.rd == 0);
	assert(instr.funct3 == 0);
	assert(instr.rs1 == 0);
	assert(instr.rs2 == 0);
	assert(instr.funct7 == 0);
	
	std::uint32_t word = 0xFFFFFFFF;
	instr = decode_instruction(word);
	assert(instr.opcode == 0x7Fu);
	assert(instr.rd == 0x1Fu);
	assert(instr.funct3 == 7);
	assert(instr.rs1 == 0x1Fu);
	assert(instr.rs2 == 0x1Fu);
	assert(instr.funct7 == 0x7Fu);

	assert(word == 0xFFFFFFFF);

	/* Decode ADD instruction */
	instr = decode_instruction(0x002081B3);
	assert(decode_operation(instr) == Operation::Add);

	// Change funct7 to SUB
	instr.funct7 = 0x20u;
	assert(decode_operation(instr) == Operation::Sub);
	instr.funct7 = 0;

	instr.funct7 = 1;
	assert(decode_operation(instr) == Operation::Unknown);
	instr.funct7 = 0;

	instr.opcode = 0;
	assert(decode_operation(instr) == Operation::Unknown);
	instr.opcode = 0x33u;

	instr = decode_instruction(0xFFFFFFFF);
	assert(decode_operation(instr) == Operation::Unknown);

	/* Decode ADDI instructions */
	// ADDI x1, x0, 5
	instr = decode_instruction(0x00500093);
	assert(instr.opcode == 0x13u);
	assert(instr.rd == 1);
	assert(instr.funct3 == 0);
	assert(instr.rs1 == 0);
	assert(instr.imm == 5);
	assert(decode_operation(instr) == Operation::AddI);

	// ADDI x2, x0, 7
	instr = decode_instruction(0x00700113);
	assert(instr.rd == 2);
	assert(instr.rs1 == 0);
	assert(instr.imm == 7);
	assert(decode_operation(instr) == Operation::AddI);

	// ADDI x3, x1, -1
	instr = decode_instruction(0xFFF08193);
	assert(instr.rd == 3);
	assert(instr.rs1 == 1);
	assert(instr.imm == 0xFFFFFFFFu);
	assert(decode_operation(instr) == Operation::AddI);

	// Largest positive 12-bit immediate
	instr = decode_instruction(0x7FF00093);
	assert(instr.imm == 0x000007FFu);

	// Smallest negative 12-bit immediate
	instr = decode_instruction(0x80000093);
	assert(instr.imm == 0xFFFFF800u);

	// Unsupported OP-IMM encoding remains unknown
	instr = decode_instruction(0x02501093);
	assert(decode_operation(instr) == Operation::Unknown);

	/* Decode SUB instruction */
	// SUB x3, x1, x2
	instr = decode_instruction(0x402081B3);
	assert(instr.opcode == 0x33u);
	assert(instr.rd == 3);
	assert(instr.funct3 == 0);
	assert(instr.rs1 == 1);
	assert(instr.rs2 == 2);
	assert(instr.funct7 == 0x20u);
	assert(decode_operation(instr) == Operation::Sub);

	// Unsupported OP funct7 remains unknown
	instr.funct7 = 0x01u;
	assert(decode_operation(instr) == Operation::Unknown);

	// ADD remains distinct from SUB
	instr = decode_instruction(0x002081B3);
	assert(decode_operation(instr) == Operation::Add);

	/* Decode register-register bitwise instructions */
	// XOR x3, x1, x2
	instr = decode_instruction(0x0020C1B3);
	assert(instr.opcode == 0x33u);
	assert(instr.rd == 3);
	assert(instr.funct3 == 4);
	assert(instr.rs1 == 1);
	assert(instr.rs2 == 2);
	assert(instr.funct7 == 0);
	assert(decode_operation(instr) == Operation::Xor);

	// OR x3, x1, x2
	instr = decode_instruction(0x0020E1B3);
	assert(instr.funct3 == 6);
	assert(decode_operation(instr) == Operation::Or);

	// AND x3, x1, x2
	instr = decode_instruction(0x0020F1B3);
	assert(instr.funct3 == 7);
	assert(decode_operation(instr) == Operation::And);

	// Logical operations require funct7 == 0
	instr.funct7 = 0x20u;
	assert(decode_operation(instr) == Operation::Unknown);

	/* Decode immediate bitwise instructions */
	// XORI x3, x1, -1
	instr = decode_instruction(0xFFF0C193);
	assert(instr.opcode == 0x13u);
	assert(instr.rd == 3);
	assert(instr.funct3 == 4);
	assert(instr.rs1 == 1);
	assert(instr.imm == 0xFFFFFFFFu);
	assert(decode_operation(instr) == Operation::XorI);

	// ORI x4, x1, 0xF0
	instr = decode_instruction(0x0F00E213);
	assert(instr.rd == 4);
	assert(instr.funct3 == 6);
	assert(instr.rs1 == 1);
	assert(instr.imm == 0x000000F0u);
	assert(decode_operation(instr) == Operation::OrI);

	// ANDI x5, x1, -256
	instr = decode_instruction(0xF000F293);
	assert(instr.rd == 5);
	assert(instr.funct3 == 7);
	assert(instr.rs1 == 1);
	assert(instr.imm == 0xFFFFFF00u);
	assert(decode_operation(instr) == Operation::AndI);

	/* Decode register-register shift instructions */
	// SLL x3, x1, x2
	instr = decode_instruction(0x002091B3);
	assert(instr.opcode == 0x33u);
	assert(instr.rd == 3);
	assert(instr.funct3 == 1);
	assert(instr.rs1 == 1);
	assert(instr.rs2 == 2);
	assert(instr.funct7 == 0);
	assert(decode_operation(instr) == Operation::Sll);

	// SRL x3, x1, x2
	instr = decode_instruction(0x0020D1B3);
	assert(instr.funct3 == 5);
	assert(instr.funct7 == 0);
	assert(decode_operation(instr) == Operation::Srl);

	// SRA x3, x1, x2
	instr = decode_instruction(0x4020D1B3);
	assert(instr.funct3 == 5);
	assert(instr.funct7 == 0x20u);
	assert(decode_operation(instr) == Operation::Sra);

	// Unsupported shift funct7 remains unknown
	instr.funct7 = 0x01u;
	assert(decode_operation(instr) == Operation::Unknown);

	/* Decode immediate shift instructions */
	// SLLI x3, x1, 4
	instr = decode_instruction(0x00409193);
	assert(instr.opcode == 0x13u);
	assert(instr.rd == 3);
	assert(instr.funct3 == 1);
	assert(instr.rs1 == 1);
	assert(instr.funct7 == 0);
	assert(instr.imm == 4);
	assert(decode_operation(instr) == Operation::SllI);

	// SRLI x4, x1, 4
	instr = decode_instruction(0x0040D213);
	assert(instr.rd == 4);
	assert(instr.funct3 == 5);
	assert(instr.funct7 == 0);
	assert(instr.imm == 4);
	assert(decode_operation(instr) == Operation::SrlI);

	// SRAI x5, x1, 4
	instr = decode_instruction(0x4040D293);
	assert(instr.rd == 5);
	assert(instr.funct3 == 5);
	assert(instr.funct7 == 0x20u);
	assert((instr.imm & 0x1Fu) == 4);
	assert(decode_operation(instr) == Operation::SraI);

	// Largest RV32I shift amount
	instr = decode_instruction(0x01F09313); // SLLI x6, x1, 31
	assert((instr.imm & 0x1Fu) == 31);
	assert(decode_operation(instr) == Operation::SllI);

	// Unsupported upper immediate bits are not legal shift encodings
	instr = decode_instruction(0x02409193);
	assert(decode_operation(instr) == Operation::Unknown);
	instr = decode_instruction(0x0240D193);
	assert(decode_operation(instr) == Operation::Unknown);

	/* Decode register-register comparison instructions */
	// SLT x3, x1, x2
	instr = decode_instruction(0x0020A1B3);
	assert(instr.opcode == 0x33u);
	assert(instr.rd == 3);
	assert(instr.funct3 == 2);
	assert(instr.rs1 == 1);
	assert(instr.rs2 == 2);
	assert(instr.funct7 == 0);
	assert(decode_operation(instr) == Operation::Slt);

	// SLTU x3, x1, x2
	instr = decode_instruction(0x0020B1B3);
	assert(instr.funct3 == 3);
	assert(instr.funct7 == 0);
	assert(decode_operation(instr) == Operation::Sltu);

	// Comparisons require funct7 == 0
	instr.funct7 = 0x20u;
	assert(decode_operation(instr) == Operation::Unknown);

	/* Decode immediate comparison instructions */
	// SLTI x3, x1, -1
	instr = decode_instruction(0xFFF0A193);
	assert(instr.opcode == 0x13u);
	assert(instr.rd == 3);
	assert(instr.funct3 == 2);
	assert(instr.rs1 == 1);
	assert(instr.imm == 0xFFFFFFFFu);
	assert(decode_operation(instr) == Operation::SltI);

	// SLTIU x4, x1, -1
	instr = decode_instruction(0xFFF0B213);
	assert(instr.rd == 4);
	assert(instr.funct3 == 3);
	assert(instr.rs1 == 1);
	assert(instr.imm == 0xFFFFFFFFu);
	assert(decode_operation(instr) == Operation::SltIu);

	// Upper immediate bits do not restrict comparison decoding
	assert(instr.funct7 == 0x7Fu);

	/* Decode LW instructions */
	// LW x3, 8(x1)
	instr = decode_instruction(0x0080A183);
	assert(instr.opcode == 0x03u);
	assert(instr.rd == 3);
	assert(instr.funct3 == 2);
	assert(instr.rs1 == 1);
	assert(instr.imm == 8);
	assert(decode_operation(instr) == Operation::Lw);

	// LW x4, -4(x1)
	instr = decode_instruction(0xFFC0A203);
	assert(instr.rd == 4);
	assert(instr.funct3 == 2);
	assert(instr.rs1 == 1);
	assert(instr.imm == 0xFFFFFFFCu);
	assert(decode_operation(instr) == Operation::Lw);

	// Reserved LOAD funct3 remains unknown
	instr = decode_instruction(0x0080F183);
	assert(decode_operation(instr) == Operation::Unknown);

	/* Decode byte-load instructions */
	// LB x3, 1(x1)
	instr = decode_instruction(0x00108183);
	assert(instr.opcode == 0x03u);
	assert(instr.rd == 3);
	assert(instr.funct3 == 0);
	assert(instr.rs1 == 1);
	assert(instr.imm == 1);
	assert(decode_operation(instr) == Operation::Lb);

	// LBU x4, 1(x1)
	instr = decode_instruction(0x0010C203);
	assert(instr.rd == 4);
	assert(instr.funct3 == 4);
	assert(instr.rs1 == 1);
	assert(instr.imm == 1);
	assert(decode_operation(instr) == Operation::Lbu);

	// LB x3, -1(x1)
	instr = decode_instruction(0xFFF08183);
	assert(instr.imm == 0xFFFFFFFFu);
	assert(decode_operation(instr) == Operation::Lb);

	/* Decode halfword-load instructions */
	// LH x3, 2(x1)
	instr = decode_instruction(0x00209183);
	assert(instr.opcode == 0x03u);
	assert(instr.rd == 3);
	assert(instr.funct3 == 1);
	assert(instr.rs1 == 1);
	assert(instr.imm == 2);
	assert(decode_operation(instr) == Operation::Lh);

	// LHU x4, 2(x1)
	instr = decode_instruction(0x0020D203);
	assert(instr.rd == 4);
	assert(instr.funct3 == 5);
	assert(instr.rs1 == 1);
	assert(instr.imm == 2);
	assert(decode_operation(instr) == Operation::Lhu);

	// LH x3, -2(x1)
	instr = decode_instruction(0xFFE09183);
	assert(instr.imm == 0xFFFFFFFEu);
	assert(decode_operation(instr) == Operation::Lh);

	/* Decode S-type immediates */
	// SW x2, 8(x1)
	instr = decode_instruction(0x0020A423);
	assert(instr.opcode == 0x23u);
	assert(instr.funct3 == 2);
	assert(instr.rs1 == 1);
	assert(instr.rs2 == 2);
	assert(instr.imm == 8);
	assert(instr.rd == 8); // Bits 11-7 hold imm[4:0], not a destination
	assert(decode_operation(instr) == Operation::Sw);

	// SW x2, -4(x1)
	instr = decode_instruction(0xFE20AE23);
	assert(instr.opcode == 0x23u);
	assert(instr.funct3 == 2);
	assert(instr.rs1 == 1);
	assert(instr.rs2 == 2);
	assert(instr.imm == 0xFFFFFFFCu);
	assert(instr.rd == 28); // The low five immediate bits are 0b11100
	assert(decode_operation(instr) == Operation::Sw);

	// Largest positive 12-bit S-type immediate
	instr = decode_instruction(0x7E20AFA3); // SW x2, 2047(x1)
	assert(instr.imm == 0x000007FFu);

	// Smallest negative 12-bit S-type immediate
	instr = decode_instruction(0x8020A023); // SW x2, -2048(x1)
	assert(instr.imm == 0xFFFFF800u);
	assert(decode_operation(instr) == Operation::Sw);

	// SB x2, 1(x1)
	instr = decode_instruction(0x002080A3);
	assert(instr.opcode == 0x23u);
	assert(instr.funct3 == 0);
	assert(instr.rs1 == 1);
	assert(instr.rs2 == 2);
	assert(instr.imm == 1);
	assert(decode_operation(instr) == Operation::Sb);

	// SB x2, -1(x1)
	instr = decode_instruction(0xFE208FA3);
	assert(instr.imm == 0xFFFFFFFFu);
	assert(decode_operation(instr) == Operation::Sb);

	// SH x2, 2(x1)
	instr = decode_instruction(0x00209123);
	assert(instr.opcode == 0x23u);
	assert(instr.funct3 == 1);
	assert(instr.rs1 == 1);
	assert(instr.rs2 == 2);
	assert(instr.imm == 2);
	assert(decode_operation(instr) == Operation::Sh);

	// SH x2, -2(x1)
	instr = decode_instruction(0xFE209F23);
	assert(instr.imm == 0xFFFFFFFEu);
	assert(decode_operation(instr) == Operation::Sh);

	// Reserved STORE funct3 remains unknown
	instr = decode_instruction(0x0020F423);
	assert(decode_operation(instr) == Operation::Unknown);

	/* Decode B-type immediates */
	// BEQ x1, x2, +8
	instr = decode_instruction(0x00208463);
	assert(instr.opcode == 0x63u);
	assert(instr.funct3 == 0);
	assert(instr.rs1 == 1);
	assert(instr.rs2 == 2);
	assert(instr.imm == 8);
	assert((instr.imm & 1u) == 0);
	assert(decode_operation(instr) == Operation::Beq);

	// BNE x1, x2, -4
	instr = decode_instruction(0xFE209EE3);
	assert(instr.opcode == 0x63u);
	assert(instr.funct3 == 1);
	assert(instr.rs1 == 1);
	assert(instr.rs2 == 2);
	assert(instr.imm == 0xFFFFFFFCu);
	assert((instr.imm & 1u) == 0);
	assert(decode_operation(instr) == Operation::Bne);

	// BLT x1, x2, +8
	instr = decode_instruction(0x0020C463);
	assert(instr.opcode == 0x63u);
	assert(instr.funct3 == 4);
	assert(instr.rs1 == 1);
	assert(instr.rs2 == 2);
	assert(instr.imm == 8);
	assert(decode_operation(instr) == Operation::Blt);

	// BGE x1, x2, +8
	instr = decode_instruction(0x0020D463);
	assert(instr.opcode == 0x63u);
	assert(instr.funct3 == 5);
	assert(instr.rs1 == 1);
	assert(instr.rs2 == 2);
	assert(instr.imm == 8);
	assert(decode_operation(instr) == Operation::Bge);

	// Signed branches also decode negative B-type immediates
	instr = decode_instruction(0xFE20CEE3); // BLT x1, x2, -4
	assert(instr.imm == 0xFFFFFFFCu);
	assert(decode_operation(instr) == Operation::Blt);
	instr = decode_instruction(0xFE20DEE3); // BGE x1, x2, -4
	assert(instr.imm == 0xFFFFFFFCu);
	assert(decode_operation(instr) == Operation::Bge);

	// BLTU x1, x2, +8
	instr = decode_instruction(0x0020E463);
	assert(instr.opcode == 0x63u);
	assert(instr.funct3 == 6);
	assert(instr.rs1 == 1);
	assert(instr.rs2 == 2);
	assert(instr.imm == 8);
	assert(decode_operation(instr) == Operation::Bltu);

	// BGEU x1, x2, +8
	instr = decode_instruction(0x0020F463);
	assert(instr.opcode == 0x63u);
	assert(instr.funct3 == 7);
	assert(instr.rs1 == 1);
	assert(instr.rs2 == 2);
	assert(instr.imm == 8);
	assert(decode_operation(instr) == Operation::Bgeu);

	// Unsigned branches also decode negative B-type immediates
	instr = decode_instruction(0xFE20EEE3); // BLTU x1, x2, -4
	assert(instr.imm == 0xFFFFFFFCu);
	assert(decode_operation(instr) == Operation::Bltu);
	instr = decode_instruction(0xFE20FEE3); // BGEU x1, x2, -4
	assert(instr.imm == 0xFFFFFFFCu);
	assert(decode_operation(instr) == Operation::Bgeu);

	// Largest positive 13-bit B-type immediate
	instr = decode_instruction(0x7E208FE3);
	assert(instr.imm == 0x00000FFEu);
	assert((instr.imm & 1u) == 0);

	// Smallest negative 13-bit B-type immediate
	instr = decode_instruction(0x80208063);
	assert(instr.imm == 0xFFFFF000u);
	assert((instr.imm & 1u) == 0);

	// Reserved BRANCH funct3 remains unknown
	instr = decode_instruction(0x0020A463);
	assert(decode_operation(instr) == Operation::Unknown);

	/* Decode LUI and its U-type immediate */
	// LUI x1, 0x12345
	instr = decode_instruction(0x123450B7);
	assert(instr.opcode == 0x37u);
	assert(instr.rd == 1);
	assert(instr.imm == 0x12345000u);
	assert(decode_operation(instr) == Operation::Lui);

	// All upper immediate bits, including bit 31, are preserved
	instr = decode_instruction(0xFFFFF137); // LUI x2, 0xFFFFF
	assert(instr.opcode == 0x37u);
	assert(instr.rd == 2);
	assert(instr.imm == 0xFFFFF000u);
	assert(decode_operation(instr) == Operation::Lui);

	// rd may be x0
	instr = decode_instruction(0xABCDE037); // LUI x0, 0xABCDE
	assert(instr.rd == 0);
	assert(instr.imm == 0xABCDE000u);
	assert(decode_operation(instr) == Operation::Lui);

	/* Decode AUIPC and its U-type immediate */
	// AUIPC x1, 0x12345
	instr = decode_instruction(0x12345097);
	assert(instr.opcode == 0x17u);
	assert(instr.rd == 1);
	assert(instr.imm == 0x12345000u);
	assert(decode_operation(instr) == Operation::Auipc);

	// All upper immediate bits, including bit 31, are preserved
	instr = decode_instruction(0xFFFFF117); // AUIPC x2, 0xFFFFF
	assert(instr.opcode == 0x17u);
	assert(instr.rd == 2);
	assert(instr.imm == 0xFFFFF000u);
	assert(decode_operation(instr) == Operation::Auipc);

	// rd may be x0
	instr = decode_instruction(0xABCDE017); // AUIPC x0, 0xABCDE
	assert(instr.rd == 0);
	assert(instr.imm == 0xABCDE000u);
	assert(decode_operation(instr) == Operation::Auipc);

	/* Decode JAL and its J-type immediate */
	// JAL x1, +8
	instr = decode_instruction(0x008000EF);
	assert(instr.opcode == 0x6Fu);
	assert(instr.rd == 1);
	assert(instr.imm == 8);
	assert((instr.imm & 1u) == 0);
	assert(decode_operation(instr) == Operation::Jal);

	// JAL x1, -4
	instr = decode_instruction(0xFFDFF0EF);
	assert(instr.opcode == 0x6Fu);
	assert(instr.rd == 1);
	assert(instr.imm == 0xFFFFFFFCu);
	assert((instr.imm & 1u) == 0);
	assert(decode_operation(instr) == Operation::Jal);

	// rd may be x0
	instr = decode_instruction(0x0080006F); // JAL x0, +8
	assert(instr.rd == 0);
	assert(instr.imm == 8);
	assert(decode_operation(instr) == Operation::Jal);

	// Largest positive 21-bit J-type immediate
	instr = decode_instruction(0x7FFFF06F);
	assert(instr.imm == 0x000FFFFEu);
	assert((instr.imm & 1u) == 0);

	// Smallest negative 21-bit J-type immediate
	instr = decode_instruction(0x8000006F);
	assert(instr.imm == 0xFFF00000u);
	assert((instr.imm & 1u) == 0);

	/* Decode JALR and its I-type immediate */
	// JALR x1, 8(x2)
	instr = decode_instruction(0x008100E7);
	assert(instr.opcode == 0x67u);
	assert(instr.rd == 1);
	assert(instr.funct3 == 0);
	assert(instr.rs1 == 2);
	assert(instr.imm == 8);
	assert(decode_operation(instr) == Operation::Jalr);

	// JALR x1, -4(x2)
	instr = decode_instruction(0xFFC100E7);
	assert(instr.opcode == 0x67u);
	assert(instr.rd == 1);
	assert(instr.funct3 == 0);
	assert(instr.rs1 == 2);
	assert(instr.imm == 0xFFFFFFFCu);
	assert(decode_operation(instr) == Operation::Jalr);

	// rd may be x0, and rd may equal rs1
	instr = decode_instruction(0x00008067); // JALR x0, 0(x1)
	assert(instr.rd == 0);
	assert(instr.rs1 == 1);
	assert(instr.imm == 0);
	assert(decode_operation(instr) == Operation::Jalr);
	instr = decode_instruction(0x000080E7); // JALR x1, 0(x1)
	assert(instr.rd == 1);
	assert(instr.rs1 == 1);
	assert(instr.imm == 0);
	assert(decode_operation(instr) == Operation::Jalr);

	// Reserved JALR funct3 values remain unknown
	instr = decode_instruction(0x008110E7);
	assert(instr.opcode == 0x67u);
	assert(instr.funct3 == 1);
	assert(decode_operation(instr) == Operation::Unknown);

	/* Decode FENCE instructions */
	instr = decode_instruction(0x0FF0000F); // FENCE IORW, IORW
	assert(instr.opcode == 0x0Fu);
	assert(instr.funct3 == 0);
	assert(decode_operation(instr) == Operation::Fence);

	instr = decode_instruction(0x0330000F); // FENCE RW, RW
	assert(instr.opcode == 0x0Fu);
	assert(instr.funct3 == 0);
	assert(decode_operation(instr) == Operation::Fence);

	instr = decode_instruction(0x8330000F); // FENCE.TSO
	assert(instr.opcode == 0x0Fu);
	assert(instr.funct3 == 0);
	assert(decode_operation(instr) == Operation::Fence);

	// A base implementation ignores the reserved rs1 and rd fields
	instr = decode_instruction(0x0FF1008F);
	assert(instr.opcode == 0x0Fu);
	assert(instr.funct3 == 0);
	assert(instr.rs1 == 2);
	assert(instr.rd == 1);
	assert(decode_operation(instr) == Operation::Fence);

	// FENCE.I belongs to a separate extension and remains unknown
	instr = decode_instruction(0x0000100F);
	assert(instr.opcode == 0x0Fu);
	assert(instr.funct3 == 1);
	assert(decode_operation(instr) == Operation::Unknown);

	/* Decode ECALL and EBREAK */
	instr = decode_instruction(0x00000073); // ECALL
	assert(instr.opcode == 0x73u);
	assert(instr.rd == 0);
	assert(instr.funct3 == 0);
	assert(instr.rs1 == 0);
	assert(instr.imm == 0);
	assert(decode_operation(instr) == Operation::Ecall);

	instr = decode_instruction(0x00100073); // EBREAK
	assert(instr.opcode == 0x73u);
	assert(instr.rd == 0);
	assert(instr.funct3 == 0);
	assert(instr.rs1 == 0);
	assert(instr.imm == 1);
	assert(decode_operation(instr) == Operation::Ebreak);

	// Other SYSTEM immediates remain unknown
	instr = decode_instruction(0x00200073);
	assert(instr.imm == 2);
	assert(decode_operation(instr) == Operation::Unknown);

	// ECALL and EBREAK require zero rd and rs1 fields
	instr = decode_instruction(0x000000F3);
	assert(instr.rd == 1);
	assert(decode_operation(instr) == Operation::Unknown);
	instr = decode_instruction(0x00008073);
	assert(instr.rs1 == 1);
	assert(decode_operation(instr) == Operation::Unknown);

	// CSR encodings are outside the current RV32I scope
	instr = decode_instruction(0x00001073);
	assert(instr.funct3 == 1);
	assert(decode_operation(instr) == Operation::Unknown);

	return 0;
}
