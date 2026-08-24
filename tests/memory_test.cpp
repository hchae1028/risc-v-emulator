#include "memory.hpp"
#include <array>
#include <cstddef>
#include <cassert>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

int main() {
	Memory memory{ 64 };

	/* Memory class tests */
	assert(memory.size() == 64);

	for (std::uint32_t i = 0; i < 64; i++) {
		assert(memory.read8(i) == 0);
	}
	
	// Read and write first and last bytes in memory
	memory.write8(0, 0x11);
	assert(memory.read8(0) == 0x11);
	memory.write8(63, 0x11);
	assert(memory.read8(63) == 0x11);
	
	// Invalid address rejection
	std::uint32_t capacity{ 64 };

	bool exception_thrown { false };
	try {
		memory.write8(capacity, 0xFF);
	} catch (const std::out_of_range& e) {
		exception_thrown = true;
	}
	assert(exception_thrown);
	
	// Read and write 32 bits in memory
	memory.write32(0, 0x12345678);
	assert(memory.read8(0) == 0x78);
	assert(memory.read8(1) == 0x56);
	assert(memory.read8(2) == 0x34);
	assert(memory.read8(3) == 0x12);
	
	assert(memory.read32(0) == 0x12345678);

	assert(memory.read32(capacity - 4) == 0x11000000);

	// Write four individual bytes and read32
	memory.write32(0, 0);
	memory.write8(0, 0x78);
	memory.write8(1, 0x56);
	memory.write8(2, 0x34);
	memory.write8(3, 0x12);
	assert(memory.read32(0) == 0x12345678);

	exception_thrown = false;
	try {
		memory.write32(capacity - 3, 0xFFFFFFFF);
	} catch (const std::out_of_range& e) {
		exception_thrown = true;
	}
	assert(exception_thrown);
	assert(memory.read32(capacity - 4) == 0x11000000);

	memory.write32(capacity - 4, 0xFFFFFFFF);
	assert(memory.read32(capacity - 4) == 0xFFFFFFFF);
	
	// Write32 at a misaligned memory address
	memory.write32(1, 0xFFFFFFFF);
	assert(memory.read32(1) == 0xFFFFFFFF);

	/* 16-bit memory operation tests */
	Memory halfword_memory{ 8 };

	// write16 stores bytes in little-endian order
	halfword_memory.write16(0, 0x1234u);
	assert(halfword_memory.read8(0) == 0x34u);
	assert(halfword_memory.read8(1) == 0x12u);
	assert(halfword_memory.read16(0) == 0x1234u);

	// read16 reconstructs a value from individual bytes
	halfword_memory.write8(2, 0xEFu);
	halfword_memory.write8(3, 0xBEu);
	assert(halfword_memory.read16(2) == 0xBEEFu);

	// The highest valid starting address is size - 2
	halfword_memory.write16(6, 0xABCDu);
	assert(halfword_memory.read8(6) == 0xCDu);
	assert(halfword_memory.read8(7) == 0xABu);
	assert(halfword_memory.read16(6) == 0xABCDu);

	// The memory abstraction permits unaligned 16-bit accesses
	halfword_memory.write16(1, 0x5678u);
	assert(halfword_memory.read8(1) == 0x78u);
	assert(halfword_memory.read8(2) == 0x56u);
	assert(halfword_memory.read16(1) == 0x5678u);

	std::array<std::uint8_t, 8> bytes_before_halfword_error{};
	for (std::size_t i = 0; i < bytes_before_halfword_error.size(); i++) {
		bytes_before_halfword_error[i] = halfword_memory.read8(static_cast<std::uint32_t>(i));
	}

	// A failed write16 does not partially modify the final byte
	exception_thrown = false;
	try {
		halfword_memory.write16(7, 0xFFFFu);
	} catch (const std::out_of_range& e) {
		exception_thrown = true;
	}
	assert(exception_thrown);
	for (std::size_t i = 0; i < bytes_before_halfword_error.size(); i++) {
		assert(halfword_memory.read8(static_cast<std::uint32_t>(i)) == bytes_before_halfword_error[i]);
	}

	// read16 rejects the same incomplete range
	exception_thrown = false;
	try {
		static_cast<void>(halfword_memory.read16(7));
	} catch (const std::out_of_range& e) {
		exception_thrown = true;
	}
	assert(exception_thrown);

	/* Bulk byte-loading tests */
	Memory loaded_memory{ 12 };
	for (std::uint32_t i = 0; i < 12; i++) {
		loaded_memory.write8(i, 0xAAu);
	}

	// Instruction bytes are copied exactly and retain their little-endian order
	std::array<std::uint8_t, 4> addi_bytes{ 0x93u, 0x00u, 0x50u, 0x00u };
	loaded_memory.load_bytes(0, addi_bytes);
	assert(loaded_memory.read8(0) == 0x93u);
	assert(loaded_memory.read8(1) == 0x00u);
	assert(loaded_memory.read8(2) == 0x50u);
	assert(loaded_memory.read8(3) == 0x00u);
	assert(loaded_memory.read32(0) == 0x00500093u);
	assert(addi_bytes[0] == 0x93u);
	assert(addi_bytes[1] == 0x00u);
	assert(addi_bytes[2] == 0x50u);
	assert(addi_bytes[3] == 0x00u);

	// A vector can also be viewed through the span-based interface
	std::vector<std::uint8_t> offset_bytes{ 0x11u, 0x22u, 0x33u };
	loaded_memory.load_bytes(5, offset_bytes);
	assert(loaded_memory.read8(4) == 0xAAu);
	assert(loaded_memory.read8(5) == 0x11u);
	assert(loaded_memory.read8(6) == 0x22u);
	assert(loaded_memory.read8(7) == 0x33u);
	assert(loaded_memory.read8(8) == 0xAAu);

	// A load may end exactly at the final memory byte
	std::array<std::uint8_t, 4> final_bytes{ 0x44u, 0x55u, 0x66u, 0x77u };
	loaded_memory.load_bytes(8, final_bytes);
	assert(loaded_memory.read8(8) == 0x44u);
	assert(loaded_memory.read8(9) == 0x55u);
	assert(loaded_memory.read8(10) == 0x66u);
	assert(loaded_memory.read8(11) == 0x77u);

	// An empty load is valid at the one-past-the-end address
	loaded_memory.load_bytes(12, std::span<const std::uint8_t>{});
	assert(loaded_memory.read32(8) == 0x77665544u);

	std::array<std::uint8_t, 12> bytes_before_load_error{};
	for (std::size_t i = 0; i < bytes_before_load_error.size(); i++) {
		bytes_before_load_error[i] = loaded_memory.read8(static_cast<std::uint32_t>(i));
	}

	// A range crossing the end is rejected before any byte is written
	std::array<std::uint8_t, 3> crossing_bytes{ 0xFFu, 0xEEu, 0xDDu };
	exception_thrown = false;
	try {
		loaded_memory.load_bytes(10, crossing_bytes);
	} catch (const std::out_of_range& e) {
		exception_thrown = true;
	}
	assert(exception_thrown);
	for (std::size_t i = 0; i < bytes_before_load_error.size(); i++) {
		assert(loaded_memory.read8(static_cast<std::uint32_t>(i)) == bytes_before_load_error[i]);
	}

	// A span larger than all of memory is also rejected atomically
	std::array<std::uint8_t, 13> oversized_bytes{};
	exception_thrown = false;
	try {
		loaded_memory.load_bytes(0, oversized_bytes);
	} catch (const std::out_of_range& e) {
		exception_thrown = true;
	}
	assert(exception_thrown);
	for (std::size_t i = 0; i < bytes_before_load_error.size(); i++) {
		assert(loaded_memory.read8(static_cast<std::uint32_t>(i)) == bytes_before_load_error[i]);
	}

	// Even an empty load rejects an address beyond one-past-the-end
	exception_thrown = false;
	try {
		loaded_memory.load_bytes(13, std::span<const std::uint8_t>{});
	} catch (const std::out_of_range& e) {
		exception_thrown = true;
	}
	assert(exception_thrown);
	for (std::size_t i = 0; i < bytes_before_load_error.size(); i++) {
		assert(loaded_memory.read8(static_cast<std::uint32_t>(i)) == bytes_before_load_error[i]);
	}

	return 0;
}
