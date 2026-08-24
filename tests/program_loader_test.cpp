#include "memory.hpp"
#include "program_loader.hpp"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <stdexcept>
#include <vector>

namespace {

void write_binary_file(const std::filesystem::path& path, std::span<const std::uint8_t> bytes) {
	std::ofstream file{ path, std::ios::binary };
	assert(file.is_open());
	file.write(
		reinterpret_cast<const char*>(bytes.data()),
		static_cast<std::streamsize>(bytes.size())
	);
	assert(file.good());
}

}

int main() {
	const auto test_directory{
		std::filesystem::temp_directory_path() / "riscv_emulator_program_loader_test"
	};
	std::filesystem::remove_all(test_directory);
	std::filesystem::create_directories(test_directory);

	// A valid empty file produces an empty byte vector
	const auto empty_path{ test_directory / "empty.bin" };
	{
		std::ofstream empty_file{ empty_path, std::ios::binary };
		assert(empty_file.is_open());
	}
	const auto empty_bytes{ read_binary_file(empty_path) };
	assert(empty_bytes.empty());

	// Every possible byte value survives char-based file input unchanged
	std::array<std::uint8_t, 256> all_byte_values{};
	for (std::size_t i = 0; i < all_byte_values.size(); i++) {
		all_byte_values[i] = static_cast<std::uint8_t>(i);
	}
	const auto all_bytes_path{ test_directory / "all_bytes.bin" };
	write_binary_file(all_bytes_path, all_byte_values);
	const auto loaded_byte_values{ read_binary_file(all_bytes_path) };
	assert(loaded_byte_values.size() == all_byte_values.size());
	for (std::size_t i = 0; i < all_byte_values.size(); i++) {
		assert(loaded_byte_values[i] == all_byte_values[i]);
	}

	// A hand-encoded program can move directly from a file into Memory
	const std::array<std::uint8_t, 16> program_bytes{
		0x93u, 0x00u, 0x50u, 0x00u, // ADDI x1, x0, 5
		0x13u, 0x01u, 0x70u, 0x00u, // ADDI x2, x0, 7
		0xB3u, 0x81u, 0x20u, 0x00u, // ADD x3, x1, x2
		0x73u, 0x00u, 0x10u, 0x00u  // EBREAK
	};
	const auto program_path{ test_directory / "program.bin" };
	write_binary_file(program_path, program_bytes);
	const auto loaded_program{ read_binary_file(program_path) };
	Memory memory{ loaded_program.size() };
	memory.load_bytes(0, loaded_program);
	assert(memory.read32(0) == 0x00500093u);
	assert(memory.read32(4) == 0x00700113u);
	assert(memory.read32(8) == 0x002081B3u);
	assert(memory.read32(12) == 0x00100073u);

	// A nonexistent path reports an open failure
	bool exception_thrown{ false };
	try {
		static_cast<void>(read_binary_file(test_directory / "missing.bin"));
	} catch (const std::runtime_error& e) {
		exception_thrown = true;
	}
	assert(exception_thrown);

	std::filesystem::remove_all(test_directory);
	return 0;
}
