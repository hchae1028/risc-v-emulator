#include "program_loader.hpp"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <vector>

std::vector<std::uint8_t> read_binary_file(const std::filesystem::path &fpath) {
	std::ifstream file{ fpath, std::ios::binary };

	if (!file.is_open()) {
		throw std::runtime_error("error: file cannot be opened");
	}

	std::vector<std::uint8_t> bytes_read{};
	char byte{};

	while (file.get(byte)) {
		bytes_read.push_back(static_cast<std::uint8_t>(static_cast<unsigned char>(byte)));
	}

	if (file.bad() || !file.eof()) {
		throw std::runtime_error("error: failed to read file");
	}

	return bytes_read;
}
