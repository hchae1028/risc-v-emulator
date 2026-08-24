#ifndef PROGRAM_LOADER_H_
#define PROGRAM_LOADER_H_

#include <cstdint>
#include <filesystem>
#include <vector>

std::vector<std::uint8_t> read_binary_file(const std::filesystem::path& fpath);

#endif
