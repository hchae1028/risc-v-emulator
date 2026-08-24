#include "elf_loader.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace {

void write16(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint16_t value) {
	bytes[offset] = static_cast<std::uint8_t>(value & 0xFFu);
	bytes[offset + 1] = static_cast<std::uint8_t>((value >> 8) & 0xFFu);
}

void write32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value) {
	bytes[offset] = static_cast<std::uint8_t>(value & 0xFFu);
	bytes[offset + 1] = static_cast<std::uint8_t>((value >> 8) & 0xFFu);
	bytes[offset + 2] = static_cast<std::uint8_t>((value >> 16) & 0xFFu);
	bytes[offset + 3] = static_cast<std::uint8_t>((value >> 24) & 0xFFu);
}

std::vector<std::uint8_t> make_valid_elf() {
	std::vector<std::uint8_t> bytes(84);
	bytes[0] = 0x7Fu;
	bytes[1] = 0x45u;
	bytes[2] = 0x4Cu;
	bytes[3] = 0x46u;
	bytes[4] = 1;
	bytes[5] = 1;
	bytes[6] = 1;
	write16(bytes, 16, 2);
	write16(bytes, 18, 243);
	write32(bytes, 20, 1);
	write32(bytes, 24, 0x12345678u);
	write32(bytes, 28, 52);
	write32(bytes, 36, 0x10u);
	write16(bytes, 40, 52);
	write16(bytes, 42, 32);
	write16(bytes, 44, 1);
	return bytes;
}

void write_program_header(std::vector<std::uint8_t>& bytes, std::size_t header_offset, std::uint32_t type, std::uint32_t offset, std::uint32_t vaddr, 
						  std::uint32_t paddr, std::uint32_t filesz, std::uint32_t memsz, std::uint32_t flags, std::uint32_t align) {
	write32(bytes, header_offset, type);
	write32(bytes, header_offset + 4, offset);
	write32(bytes, header_offset + 8, vaddr);
    write32(bytes, header_offset + 12, paddr);
    write32(bytes, header_offset + 16, filesz);
    write32(bytes, header_offset + 20, memsz);
    write32(bytes, header_offset + 24, flags);
    write32(bytes, header_offset + 28, align);
}

void assert_rejected(const std::vector<std::uint8_t>& bytes) {
	bool exception_thrown{ false };
	try {
		auto header{ parse_elf32_header(bytes) };
		static_cast<void>(parse_elf32_load_segments(bytes, header));
	} catch (const std::runtime_error& e) {
		exception_thrown = true;
	}
	assert(exception_thrown);
}

}

int main() {
	/* A valid RV32 little-endian executable header is decoded */
	const auto valid_bytes{ make_valid_elf() };
	const auto header{ parse_elf32_header(valid_bytes) };
	assert(header.entry == 0x12345678u);
	assert(header.program_header_offset == 52);
	assert(header.program_header_entry_size == 32);
	assert(header.program_header_count == 1);
	assert(header.flags == 0x10u);

	// Section-header fields are irrelevant to executable segment loading
	auto no_section_table{ valid_bytes };
	write32(no_section_table, 32, 0xFFFFFFFFu);
	write16(no_section_table, 46, 0xFFFFu);
	write16(no_section_table, 48, 0xFFFFu);
	write16(no_section_table, 50, 0xFFFFu);
	const auto header_without_sections{ parse_elf32_header(no_section_table) };
	assert(header_without_sections.entry == 0x12345678u);
	assert(header_without_sections.program_header_offset == 52);

	/* Truncated and incorrectly identified files are rejected */
	auto invalid_bytes{ valid_bytes };
	invalid_bytes.resize(51);
	assert_rejected(invalid_bytes);

	invalid_bytes = valid_bytes;
	invalid_bytes[0] = 0;
	assert_rejected(invalid_bytes);

	invalid_bytes = valid_bytes;
	invalid_bytes[4] = 2; // ELF64
	assert_rejected(invalid_bytes);

	invalid_bytes = valid_bytes;
	invalid_bytes[5] = 2; // Big-endian
	assert_rejected(invalid_bytes);

	invalid_bytes = valid_bytes;
	invalid_bytes[6] = 0;
	assert_rejected(invalid_bytes);

	/* Fixed ELF header fields must describe an RV32 executable */
	invalid_bytes = valid_bytes;
	write16(invalid_bytes, 16, 3); // Shared object instead of executable
	assert_rejected(invalid_bytes);

	invalid_bytes = valid_bytes;
	write16(invalid_bytes, 18, 62); // x86-64 instead of RISC-V
	assert_rejected(invalid_bytes);

	invalid_bytes = valid_bytes;
	write32(invalid_bytes, 20, 0);
	assert_rejected(invalid_bytes);

	invalid_bytes = valid_bytes;
	write16(invalid_bytes, 40, 51);
	assert_rejected(invalid_bytes);

	invalid_bytes = valid_bytes;
	write16(invalid_bytes, 42, 31);
	assert_rejected(invalid_bytes);

	invalid_bytes = valid_bytes;
	write16(invalid_bytes, 44, 0);
	assert_rejected(invalid_bytes);

	// The entry point must match the emulator's four-byte instruction alignment
	invalid_bytes = valid_bytes;
	write32(invalid_bytes, 24, 0x1234567Au);
	assert_rejected(invalid_bytes);

	// ELF files permitted to contain compressed instructions are unsupported
	invalid_bytes = valid_bytes;
	write32(invalid_bytes, 36, 0x11u);
	assert_rejected(invalid_bytes);

	/* The complete program-header table must fit within the file */
	invalid_bytes = valid_bytes;
	write32(invalid_bytes, 28, 85);
	assert_rejected(invalid_bytes);

	invalid_bytes = valid_bytes;
	write32(invalid_bytes, 28, 0xFFFFFFFFu);
	assert_rejected(invalid_bytes);

	invalid_bytes = valid_bytes;
	write16(invalid_bytes, 44, 2);
	assert_rejected(invalid_bytes);

	/* A valid PT_LOAD segment */
	auto segment_bytes{ make_valid_elf() };
	segment_bytes.resize(100);

	write_program_header(segment_bytes, 52, 1, 84, 0x1000u, 0x2000u, 16, 32, 0x5u, 4);

	auto segment_header{ parse_elf32_header(segment_bytes) };
	auto segments{ parse_elf32_load_segments(segment_bytes, segment_header) };

	assert(segments.size() == 1);
	assert(segments[0].offset == 84);
	assert(segments[0].vaddr == 0x1000u);
	assert(segments[0].paddr == 0x2000u);
	assert(segments[0].filesz == 16);
	assert(segments[0].memsz == 32);
	assert(segments[0].flags == 0x5u);
	assert(segments[0].align == 4);

	// Multiple segments with a non-loadable entry (PT_LOAD != 1)
	segment_bytes = make_valid_elf();
	segment_bytes.resize(156);
	write16(segment_bytes, 44, 3);

	write_program_header(segment_bytes, 52, 1, 148, 0x1000u, 0x2000u, 4, 4, 0x5u, 1);
	write_program_header(segment_bytes, 84, 2, 0, 0, 0, 0, 0, 0, 0);
	write_program_header(segment_bytes, 116, 1, 152, 0x3000u, 0x4000u, 4, 8, 0x6u, 1);

	segment_header = parse_elf32_header(segment_bytes);
	segments = parse_elf32_load_segments(segment_bytes, segment_header);

	assert(segments.size() == 2);
	assert(segments[0].vaddr == 0x1000u);
	assert(segments[0].flags == 0x5u);
	assert(segments[1].vaddr == 0x3000u);
	assert(segments[1].flags == 0x6u);

	// File-backed segment may end exactly at EOF
	segment_bytes = make_valid_elf();
	segment_bytes.resize(100);

	write_program_header(segment_bytes, 52, 1, 96, 0x1000u, 0, 4, 4, 0x5u, 1);

	segment_header = parse_elf32_header(segment_bytes);
	segments = parse_elf32_load_segments(segment_bytes, segment_header);

	assert(segments.size() == 1);
	assert(segments[0].offset == 96);
	assert(segments[0].filesz == 4);

	/* Zero-length file data at EOF is valid */
	segment_bytes = make_valid_elf();
	segment_bytes.resize(100);
	write_program_header(segment_bytes, 52, 1, 100, 0x1000u, 0, 0, 16, 0x6u, 1);

	segment_header = parse_elf32_header(segment_bytes);
	segments = parse_elf32_load_segments(segment_bytes, segment_header);

	assert(segments.size() == 1);
	assert(segments[0].offset == 100);
	assert(segments[0].filesz == 0);
	assert(segments[0].memsz == 16);

	/* p_filesz may not exceed p_memsz */
	segment_bytes = make_valid_elf();
	segment_bytes.resize(100);
	write_program_header(segment_bytes, 52, 1, 84, 0, 0, 16, 8, 0, 1);
	assert_rejected(segment_bytes);

	/* p_offset outside the file is rejected */
	segment_bytes = make_valid_elf();
	segment_bytes.resize(100);
	write_program_header(segment_bytes, 52, 1, 101, 0, 0, 0, 0, 0, 1);
	assert_rejected(segment_bytes);

	/* File-backed range extending past EOF is rejected */
	segment_bytes = make_valid_elf();
	segment_bytes.resize(100);
	write_program_header(segment_bytes, 52, 1, 96, 0, 0, 8, 8, 0, 1);
	assert_rejected(segment_bytes);

	/* p_align == 0 is valid */
	segment_bytes = make_valid_elf();
	write_program_header(segment_bytes, 52, 1, 84, 0x1234u, 0, 0, 0, 0, 0);

	segment_header = parse_elf32_header(segment_bytes);
	segments = parse_elf32_load_segments(segment_bytes, segment_header);

	assert(segments.size() == 1);
	assert(segments[0].align == 0);

	/* p_align == 1 is valid */
	segment_bytes = make_valid_elf();
	write_program_header(segment_bytes, 52, 1, 84, 0x1234u, 0, 0, 0, 0, 1);

	segment_header = parse_elf32_header(segment_bytes);
	segments = parse_elf32_load_segments(segment_bytes, segment_header);

	assert(segments.size() == 1);
	assert(segments[0].align == 1);

	/* Congruent power-of-two alignment is valid */
	segment_bytes = make_valid_elf();
	write_program_header(segment_bytes, 52, 1, 84, 0x1054u, 0, 0, 0, 0, 16);

	segment_header = parse_elf32_header(segment_bytes);
	segments = parse_elf32_load_segments(segment_bytes, segment_header);

	assert(segments.size() == 1);
	assert(segments[0].align == 16);

	/* Non-power-of-two alignment is rejected */
	segment_bytes = make_valid_elf();
	write_program_header(segment_bytes, 52, 1, 84, 0x1000u, 0, 0, 0, 0, 3);
	assert_rejected(segment_bytes);

	/* Incongruent offset and virtual address are rejected */
	segment_bytes = make_valid_elf();
	write_program_header(segment_bytes, 52, 1, 84, 0x1050u, 0, 0, 0, 0, 16);
	assert_rejected(segment_bytes);

	/* No PT_LOAD entries returns an empty list */
	segment_bytes = make_valid_elf();
	segment_header = parse_elf32_header(segment_bytes);
	segments = parse_elf32_load_segments(segment_bytes, segment_header);

	assert(segments.empty());

	return 0;
}
