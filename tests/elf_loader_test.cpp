#include "elf_loader.hpp"
#include "memory.hpp"
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

void fill_memory(Memory& memory, std::uint8_t value) {
	for (std::size_t i{}; i < memory.size(); i++) {
		memory.write8(static_cast<std::uint32_t>(i), value);
	}
}

void assert_memory_filled(Memory& memory, std::uint8_t value) {
	for (std::size_t i{}; i < memory.size(); i++) {
		assert(memory.read8(static_cast<std::uint32_t>(i)) == value);
	}
}

void assert_load_rejected(Memory& memory, const std::vector<std::uint8_t>& bytes,
						  const std::vector<Elf32LoadSegment>& segments) {
	bool exception_thrown{ false };
	try {
		load_elf_segments(memory, bytes, segments);
	} catch (const std::runtime_error& e) {
		exception_thrown = true;
	}
	assert(exception_thrown);
}

void assert_elf_load_rejected(Memory& memory, const std::vector<std::uint8_t>& bytes) {
	bool exception_thrown{ false };
	try {
		static_cast<void>(load_elf32(memory, bytes));
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
	write_program_header(segment_bytes, 52, 1, 84, 0x1053u, 0, 0, 0, 0, 3);
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

	/* Parsed file bytes are loaded at p_vaddr and the remaining memory is zero-filled */
	segment_bytes = make_valid_elf();
	segment_bytes.resize(88);
	segment_bytes[84] = 0x11u;
	segment_bytes[85] = 0x22u;
	segment_bytes[86] = 0x33u;
	segment_bytes[87] = 0x44u;
	write_program_header(segment_bytes, 52, 1, 84, 4, 4, 4, 8, 0xFFFFFFFFu, 4);

	segment_header = parse_elf32_header(segment_bytes);
	segments = parse_elf32_load_segments(segment_bytes, segment_header);

	Memory loaded_memory{ 12 };
	fill_memory(loaded_memory, 0xA5u);
	load_elf_segments(loaded_memory, segment_bytes, segments);

	for (std::uint32_t address{}; address < 4; address++) {
		assert(loaded_memory.read8(address) == 0xA5u);
	}
	assert(loaded_memory.read8(4) == 0x11u);
	assert(loaded_memory.read8(5) == 0x22u);
	assert(loaded_memory.read8(6) == 0x33u);
	assert(loaded_memory.read8(7) == 0x44u);
	for (std::uint32_t address{ 8 }; address < 12; address++) {
		assert(loaded_memory.read8(address) == 0);
	}

	/* A segment with no file-backed bytes still zero-fills its complete memory range */
	const std::vector<std::uint8_t> no_file_bytes{};
	const std::vector<Elf32LoadSegment> zero_fill_segments{
		Elf32LoadSegment{
			.offset = 0,
			.vaddr = 2,
			.paddr = 0,
			.filesz = 0,
			.memsz = 3,
			.flags = 0,
			.align = 1
		}
	};
	Memory zero_fill_memory{ 8 };
	fill_memory(zero_fill_memory, 0xA5u);
	load_elf_segments(zero_fill_memory, no_file_bytes, zero_fill_segments);
	assert(zero_fill_memory.read8(1) == 0xA5u);
	assert(zero_fill_memory.read8(2) == 0);
	assert(zero_fill_memory.read8(3) == 0);
	assert(zero_fill_memory.read8(4) == 0);
	assert(zero_fill_memory.read8(5) == 0xA5u);

	/* Adjacent segment ranges do not overlap */
	const std::vector<std::uint8_t> adjacent_bytes{ 0x10u, 0x20u, 0x30u, 0x40u };
	const std::vector<Elf32LoadSegment> adjacent_segments{
		Elf32LoadSegment{
			.offset = 0,
			.vaddr = 1,
			.paddr = 1,
			.filesz = 2,
			.memsz = 2,
			.flags = 0x5u,
			.align = 1
		},
		Elf32LoadSegment{
			.offset = 2,
			.vaddr = 3,
			.paddr = 3,
			.filesz = 2,
			.memsz = 2,
			.flags = 0x6u,
			.align = 1
		}
	};
	Memory adjacent_memory{ 6 };
	fill_memory(adjacent_memory, 0xA5u);
	load_elf_segments(adjacent_memory, adjacent_bytes, adjacent_segments);
	assert(adjacent_memory.read8(0) == 0xA5u);
	assert(adjacent_memory.read8(1) == 0x10u);
	assert(adjacent_memory.read8(2) == 0x20u);
	assert(adjacent_memory.read8(3) == 0x30u);
	assert(adjacent_memory.read8(4) == 0x40u);
	assert(adjacent_memory.read8(5) == 0xA5u);

	/* Overlapping memory ranges are rejected before either segment is written */
	const std::vector<Elf32LoadSegment> overlapping_segments{
		Elf32LoadSegment{
			.offset = 0,
			.vaddr = 1,
			.paddr = 0,
			.filesz = 2,
			.memsz = 3,
			.flags = 0,
			.align = 1
		},
		Elf32LoadSegment{
			.offset = 2,
			.vaddr = 3,
			.paddr = 0,
			.filesz = 2,
			.memsz = 2,
			.flags = 0,
			.align = 1
		}
	};
	Memory overlapping_memory{ 8 };
	fill_memory(overlapping_memory, 0xA5u);
	assert_load_rejected(overlapping_memory, adjacent_bytes, overlapping_segments);
	assert_memory_filled(overlapping_memory, 0xA5u);

	/* A later out-of-range segment cannot leave an earlier valid segment loaded */
	const std::vector<Elf32LoadSegment> partially_valid_segments{
		Elf32LoadSegment{
			.offset = 0,
			.vaddr = 0,
			.paddr = 0,
			.filesz = 2,
			.memsz = 2,
			.flags = 0,
			.align = 1
		},
		Elf32LoadSegment{
			.offset = 2,
			.vaddr = 7,
			.paddr = 0,
			.filesz = 2,
			.memsz = 2,
			.flags = 0,
			.align = 1
		}
	};
	Memory all_or_nothing_memory{ 8 };
	fill_memory(all_or_nothing_memory, 0xA5u);
	assert_load_rejected(all_or_nothing_memory, adjacent_bytes, partially_valid_segments);
	assert_memory_filled(all_or_nothing_memory, 0xA5u);

	/* A range that crosses the end of the 32-bit address space is rejected */
	const std::vector<Elf32LoadSegment> wrapping_segments{
		Elf32LoadSegment{
			.offset = 0,
			.vaddr = 0xFFFFFFFEu,
			.paddr = 0,
			.filesz = 0,
			.memsz = 3,
			.flags = 0,
			.align = 1
		}
	};
	Memory wrapping_memory{ 8 };
	fill_memory(wrapping_memory, 0xA5u);
	assert_load_rejected(wrapping_memory, no_file_bytes, wrapping_segments);
	assert_memory_filled(wrapping_memory, 0xA5u);

	/* Distinct nonzero physical and virtual addresses are unsupported */
	const std::vector<Elf32LoadSegment> separate_address_segments{
		Elf32LoadSegment{
			.offset = 0,
			.vaddr = 1,
			.paddr = 2,
			.filesz = 1,
			.memsz = 1,
			.flags = 0,
			.align = 1
		}
	};
	Memory separate_address_memory{ 8 };
	fill_memory(separate_address_memory, 0xA5u);
	assert_load_rejected(separate_address_memory, adjacent_bytes, separate_address_segments);
	assert_memory_filled(separate_address_memory, 0xA5u);

	/* Complete ELF loading accepts an entry at the start of an executable segment */
	auto complete_elf{ make_valid_elf() };
	complete_elf.resize(88);
	write32(complete_elf, 24, 4);
	complete_elf[84] = 0x11u;
	complete_elf[85] = 0x22u;
	complete_elf[86] = 0x33u;
	complete_elf[87] = 0x44u;
	write_program_header(complete_elf, 52, 1, 84, 4, 4, 4, 8, 0x5u, 4);

	Memory complete_memory{ 12 };
	fill_memory(complete_memory, 0xA5u);
	const auto loaded_entry{ load_elf32(complete_memory, complete_elf) };
	assert(loaded_entry == 4);
	assert(complete_memory.read8(4) == 0x11u);
	assert(complete_memory.read8(5) == 0x22u);
	assert(complete_memory.read8(6) == 0x33u);
	assert(complete_memory.read8(7) == 0x44u);
	for (std::uint32_t address{ 8 }; address < 12; address++) {
		assert(complete_memory.read8(address) == 0);
	}

	/* Entry validation uses the memory image, including its zero-filled tail */
	auto zero_tail_entry_elf{ complete_elf };
	write32(zero_tail_entry_elf, 24, 8);
	Memory zero_tail_entry_memory{ 12 };
	fill_memory(zero_tail_entry_memory, 0xA5u);
	assert(load_elf32(zero_tail_entry_memory, zero_tail_entry_elf) == 8);
	assert(zero_tail_entry_memory.read8(8) == 0);

	/* The exclusive end of an executable segment is not a valid entry point */
	auto end_entry_elf{ complete_elf };
	write32(end_entry_elf, 24, 12);
	Memory end_entry_memory{ 12 };
	fill_memory(end_entry_memory, 0xA5u);
	assert_elf_load_rejected(end_entry_memory, end_entry_elf);
	assert_memory_filled(end_entry_memory, 0xA5u);

	/* An entry inside a non-executable segment is rejected without modifying memory */
	auto non_executable_entry_elf{ complete_elf };
	write_program_header(non_executable_entry_elf, 52, 1, 84, 4, 4, 4, 8, 0x6u, 4);
	Memory non_executable_entry_memory{ 12 };
	fill_memory(non_executable_entry_memory, 0xA5u);
	assert_elf_load_rejected(non_executable_entry_memory, non_executable_entry_elf);
	assert_memory_filled(non_executable_entry_memory, 0xA5u);

	/* The entry may select one executable segment while every PT_LOAD is loaded */
	auto multiple_segment_elf{ make_valid_elf() };
	multiple_segment_elf.resize(124);
	write16(multiple_segment_elf, 44, 2);
	write32(multiple_segment_elf, 24, 16);
	multiple_segment_elf[116] = 0x10u;
	multiple_segment_elf[117] = 0x20u;
	multiple_segment_elf[118] = 0x30u;
	multiple_segment_elf[119] = 0x40u;
	multiple_segment_elf[120] = 0x50u;
	multiple_segment_elf[121] = 0x60u;
	multiple_segment_elf[122] = 0x70u;
	multiple_segment_elf[123] = 0x80u;
	write_program_header(multiple_segment_elf, 52, 1, 116, 4, 4, 4, 4, 0x6u, 4);
	write_program_header(multiple_segment_elf, 84, 1, 120, 16, 16, 4, 8, 0x5u, 4);

	Memory multiple_segment_memory{ 24 };
	fill_memory(multiple_segment_memory, 0xA5u);
	assert(load_elf32(multiple_segment_memory, multiple_segment_elf) == 16);
	assert(multiple_segment_memory.read8(4) == 0x10u);
	assert(multiple_segment_memory.read8(7) == 0x40u);
	assert(multiple_segment_memory.read8(16) == 0x50u);
	assert(multiple_segment_memory.read8(19) == 0x80u);
	assert(multiple_segment_memory.read8(20) == 0);
	assert(multiple_segment_memory.read8(23) == 0);

	/* An executable with no PT_LOAD entries is rejected without modifying memory */
	const auto no_load_segments_elf{ make_valid_elf() };
	Memory no_load_segments_memory{ 12 };
	fill_memory(no_load_segments_memory, 0xA5u);
	assert_elf_load_rejected(no_load_segments_memory, no_load_segments_elf);
	assert_memory_filled(no_load_segments_memory, 0xA5u);

	return 0;
}
