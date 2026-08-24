#include "elf_loader.hpp"
#include <bit>
#include <cstddef>
#include <stdexcept>

namespace {

std::uint16_t read16(std::span<const std::uint8_t> bytes, std::size_t offset) {
	auto value{ static_cast<std::uint32_t>(bytes[offset]) | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8) };
	
	return static_cast<std::uint16_t>(value);
}

std::uint32_t read32(std::span<const std::uint8_t> bytes, std::size_t offset) {
	return static_cast<std::uint32_t>(bytes[offset]) | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8)
		| (static_cast<std::uint32_t>(bytes[offset + 2]) << 16) | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

}

Elf32Header parse_elf32_header(std::span<const std::uint8_t> bytes) {
	if (bytes.size() < 52) {
		throw std::runtime_error("error: ELF header must contain at least 52 bytes");
	}

	if (bytes[0] != 0x7Fu || bytes[1] != 0x45u || bytes[2] != 0x4Cu || bytes[3] != 0x46u) {
		throw std::runtime_error("error: invalid ELF magic number");
	}

	if (bytes[4] != 1) {
		throw std::runtime_error("error: ELF is not 32-bit");
	}

	if (bytes[5] != 1) {
		throw std::runtime_error("error: ELF is not little-endian");
	}

	if (bytes[6] != 1) {
		throw std::runtime_error("error: invalid ELF identification version");
	}

	if (read16(bytes, 16) != 2) {
		throw std::runtime_error("error: ELF is not executable");
	}

	if (read16(bytes, 18) != 243) {
		throw std::runtime_error("error: ELF is not RISC-V");
	}

	if (read32(bytes, 20) != 1) {
		throw std::runtime_error("error: invalid ELF version");
	}

	auto entry{ read32(bytes, 24) };
	auto program_header_offset{ read32(bytes, 28) };
	auto flags{ read32(bytes, 36) };
	auto header_size{ read16(bytes, 40) };
	auto program_header_entry_size{ read16(bytes, 42) };
	auto program_header_count{ read16(bytes, 44) };

	if (header_size != 52) {
        throw std::runtime_error{ "error: invalid ELF header size" };
    }

    if (program_header_entry_size != 32) {
        throw std::runtime_error{ "error: invalid program header entry size" };
    }

    if (program_header_count == 0) {
        throw std::runtime_error{ "error: ELF has no program headers" };
    }

	if (entry % 4 != 0) {
        throw std::runtime_error{ "error: ELF entry address is misaligned" };
    }

    if ((flags & 0x1u) != 0) {
        throw std::runtime_error{ "error: compressed RISC-V instructions are unsupported" };
    }

	auto ph_offset{ static_cast<std::size_t>(program_header_offset) };
    auto ph_size{ static_cast<std::size_t>(program_header_entry_size) * static_cast<std::size_t>(program_header_count) };

    if (ph_offset > bytes.size() || ph_size > bytes.size() - ph_offset) {
        throw std::runtime_error{ "error: program header table is out of range" };
    }

	return Elf32Header {
		.entry = entry,
        .program_header_offset = program_header_offset,
        .program_header_entry_size = program_header_entry_size,
        .program_header_count = program_header_count,
        .flags = flags
	};
}

std::vector<Elf32LoadSegment> parse_elf32_load_segments(std::span<const std::uint8_t> bytes, Elf32Header header) {
	std::vector<Elf32LoadSegment> segments{};

	for (std::size_t i{}; i < header.program_header_count; i++) {
		auto offset{ static_cast<std::size_t>(header.program_header_offset) + i * header.program_header_entry_size };

		auto p_type{ read32(bytes, offset) };
		auto p_offset{ read32(bytes, offset + 4) };
		auto p_vaddr{ read32(bytes, offset + 8) };
		auto p_paddr{ read32(bytes, offset + 12) };
		auto p_filesz{ read32(bytes, offset + 16) };
		auto p_memsz{ read32(bytes, offset + 20) };
		auto p_flags{ read32(bytes, offset + 24) };
		auto p_align{ read32(bytes, offset + 28) };

		if (p_type != 1) {
			continue;	// Not PT_LOAD
		}

		if (p_filesz > p_memsz) {
			throw std::runtime_error("error: PT_LOAD file size exceeds memory size");
		}

		if (p_offset > bytes.size() || static_cast<std::size_t>(p_filesz) > bytes.size() - p_offset) {
			throw std::runtime_error("error: PT_LOAD file range is out of bounds");
		}

		if (p_align > 1 && !std::has_single_bit(p_align)) {
			throw std::runtime_error("error: invalid PT_LOAD alignment");
		}

		if (p_align > 1 && p_vaddr % p_align != p_offset % p_align) {
			throw std::runtime_error("error: PT_LOAD alignment mismatch");
		}

		segments.push_back(Elf32LoadSegment {
			.offset = p_offset,
			.vaddr = p_vaddr,
			.paddr = p_paddr,
			.filesz = p_filesz,
			.memsz = p_memsz,
			.flags = p_flags,
			.align = p_align
		});
	}

	return segments;
}
