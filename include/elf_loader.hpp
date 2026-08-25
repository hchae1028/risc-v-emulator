#ifndef ELF_LOADER_H_
#define ELF_LOADER_H_

#include <cstdint>
#include <span>
#include <vector>

class Memory;

struct Elf32Header {
	std::uint32_t entry;
	std::uint32_t program_header_offset;
	std::uint16_t program_header_entry_size;
	std::uint16_t program_header_count;
	std::uint32_t flags;
};

struct Elf32LoadSegment {
	std::uint32_t offset;
	std::uint32_t vaddr;
	std::uint32_t paddr;
	std::uint32_t filesz;
	std::uint32_t memsz;
	std::uint32_t flags;
	std::uint32_t align;
};

Elf32Header parse_elf32_header(std::span<const std::uint8_t> bytes);

std::vector<Elf32LoadSegment> parse_elf32_load_segments(std::span<const std::uint8_t> bytes, const Elf32Header& header);

void load_elf_segments(Memory& memory, std::span<const std::uint8_t> bytes, std::span<const Elf32LoadSegment> segments);

#endif
