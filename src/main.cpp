#include <cstddef>
#include <exception>
#include <ios>
#include <iostream>
#include <cstdlib>
#include <iomanip>
#include <filesystem>
#include <string_view>
#include "cpu.hpp"
#include "instruction_trace.hpp"
#include "memory.hpp"
#include "bus.hpp"
#include "program_loader.hpp"
#include "elf_loader.hpp"
#include "runner.hpp"
#include "uart_device.hpp"
#include "timer_device.hpp"
#include "machine.hpp"

int main(int argc, char** argv) {
	bool trace_enabled{};
	std::filesystem::path program_path;

	if (argc == 2) {
		program_path = argv[1];
	}
	else if (argc == 3 && std::string_view{ argv[1] } == "--trace") {
		trace_enabled = true;
		program_path = argv[2];
	}
	else {
		std::cerr << "usage: risc-v-emulator [--trace] <program.elf>\n";
		return EXIT_FAILURE;
	}

	try {	
		auto bytes{ read_binary_file(program_path) };
		constexpr std::size_t memory_size{ 64 * 1024 };

		Memory memory{ memory_size };
		auto entry{ load_elf32(memory, bytes) };

		UartDevice uart{};
		TimerDevice timer{};

		Bus bus{ memory, 0 };
		bus.map_device(timer, 0x0200'0000u);
		bus.map_device(uart, 0x1000'0000u);

		Cpu cpu{};
		cpu.set_pc(entry);

		Machine machine{ cpu, bus, timer }; 

		InstructionTraceCallBack trace{};

		if (trace_enabled) {
			trace = [](const InstructionTrace& event) {
				std::cerr << "pc=0x" << std::hex << std::setw(8) << std::setfill('0') << event.pc
					<< " instruction=0x" << std::setw(8) << event.instruction << "\n";
			};
		}

		auto result{ run_until_breakpoint(machine, 1'000'000, trace) };
		auto rc{ EXIT_SUCCESS };

		std::cerr << "\nstopped by ";
		if (result.trap) {
			switch (result.trap->cause) {
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

				case TrapCause::MachineTimerInterrupt: {
					std::cerr << "machine timer interrupt\n";
					rc = EXIT_FAILURE;
					break;
				}
			}
		}
		else {
			std::cerr << "instruction limit\n";
			rc = EXIT_FAILURE;
		}

		std::cerr << std::dec << "instructions retired: " << result.instructions_retired << '\n';
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
