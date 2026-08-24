#include <algorithm>
#include <cstddef>
#include <exception>
#include <ios>
#include <iostream>
#include <cstdlib>
#include <iomanip>
#include <filesystem>
#include "cpu.hpp"
#include "memory.hpp"
#include "program_loader.hpp"
#include "runner.hpp"

int main(int argc, char** argv) {
	if (argc != 2) {
		std::cerr << "usage: risc-v-emulator <program.bin>\n";
		return EXIT_FAILURE;
	}

	try {	
		Cpu cpu{};

		auto bytes{ read_binary_file(std::filesystem::path{ argv[1] }) };
		constexpr std::size_t min_memory_size{ 64 * 1024 };
		auto memory_size{ std::max(min_memory_size, bytes.size()) };

		Memory memory{ memory_size };
		memory.load_bytes(0, bytes);

		auto result{ run_until_trap(cpu, memory, 1'000'000) };
		auto rc{ EXIT_SUCCESS };

		std::cout << "stopped by ";
		if (result.trap.has_value()) {
			switch (*result.trap) {
				case TrapCause::BreakPoint: {
					std::cout << "breakpoint\n";
					break;
				}

				case TrapCause::EnvironmentCall: {
					std::cout << "environment call\n";
					break;
				}
			}
		}
		else {
			std::cout << "instruction limit\n";
			rc = EXIT_FAILURE;
		}

		std::cout << "instructions retired: " << result.instructions_retired << '\n';
		std::cout << "final pc value: 0x" << std::hex << std::setw(8) << std::setfill('0') << cpu.read_pc() << '\n';

		std::cout << "register states:\n";
		for (std::size_t i{}; i < 32; i++) {
			std::cout << "x" << std::dec << i << ": 0x" << std::hex 
				<< std::setw(8) << std::setfill('0') << cpu.read_register(i) << '\n';
		}

		return rc;
	} catch (const std::exception& e) {
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}
}
