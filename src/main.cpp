#include <cstddef>
#include <exception>
#include <ios>
#include <iostream>
#include <cstdlib>
#include <iomanip>
#include <filesystem>
#include "cpu.hpp"
#include "memory.hpp"
#include "bus.hpp"
#include "program_loader.hpp"
#include "elf_loader.hpp"
#include "runner.hpp"
#include "uart_device.hpp"

int main(int argc, char** argv) {
	if (argc != 2) {
		std::cerr << "usage: risc-v-emulator <program.elf>\n";
		return EXIT_FAILURE;
	}

	try {	
		auto bytes{ read_binary_file(std::filesystem::path{ argv[1] }) };
		constexpr std::size_t memory_size{ 64 * 1024 };

		Memory memory{ memory_size };
		auto entry{ load_elf32(memory, bytes) };

		UartDevice uart{};

		Bus bus{ memory, 0 };
		bus.map_device(uart, 0x1000'0000u);

		Cpu cpu{};
		cpu.set_pc(entry);

		auto result{ run_until_trap(cpu, bus, 1'000'000) };
		auto rc{ EXIT_SUCCESS };

		std::cerr << "stopped by ";
		if (result.trap.has_value()) {
			switch (*result.trap) {
				case TrapCause::BreakPoint: {
					std::cerr << "breakpoint\n";
					break;
				}

				case TrapCause::EnvironmentCall: {
					std::cerr << "environment call\n";
					break;
				}

				case TrapCause::IllegalInstruction: {
					std::cerr << "illegal instruction\n";
					rc = EXIT_FAILURE;
					break;
				}

				case TrapCause::InstructionAddressMisaligned: {
					std::cerr << "instruction address misaligned\n";
					rc = EXIT_FAILURE;
					break;
				}

				case TrapCause::LoadAddressMisaligned: {
					std::cerr << "load address misaligned\n";
					rc = EXIT_FAILURE;
					break;
				}

				case TrapCause::StoreAddressMisaligned: {
					std::cerr << "store address misaligned\n";
					rc = EXIT_FAILURE;
					break;
				}

				case TrapCause::InstructionAccessFault: {
					std::cerr << "instruction access fault\n";
					rc = EXIT_FAILURE;
					break;
				}

				case TrapCause::LoadAccessFault: {
					std::cerr << "load access fault\n";
					rc = EXIT_FAILURE;
					break;
				}

				case TrapCause::StoreAccessFault: {
					std::cerr << "store access fault\n";
					rc = EXIT_FAILURE;
					break;
				}
			}
		}
		else {
			std::cerr << "instruction limit\n";
			rc = EXIT_FAILURE;
		}

		std::cerr << "instructions retired: " << result.instructions_retired << '\n';
		std::cerr << "final pc value: 0x" << std::hex << std::setw(8) << std::setfill('0') << cpu.read_pc() << '\n';

		std::cerr << "register states:\n";
		for (std::size_t i{}; i < 32; i++) {
			std::cerr << "x" << std::dec << i << ": 0x" << std::hex 
				<< std::setw(8) << std::setfill('0') << cpu.read_register(i) << '\n';
		}
		
		for (const auto& byte: uart.transmitted_bytes()) {
			std::cout.put(static_cast<char>(byte));
		}
		std::cout.flush();

		return rc;
	} catch (const std::exception& e) {
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}
}
