cmake_minimum_required(VERSION 3.23)

project(risc-v-emulator LANGUAGES CXX)
enable_testing()

function(enable_project_warnings target)
	target_compile_options(${target}
		PRIVATE
			-Wall
			-Wextra
			-Wpedantic
			-Werror
			-Wconversion
	)
endfunction()

add_library(riscv_core
	STATIC
		src/cpu.cpp
		src/memory.cpp
		src/decoder.cpp
		src/executor.cpp
		src/program_loader.cpp
		src/runner.cpp
		src/elf_loader.cpp
		src/bus.cpp
		src/uart_device.cpp
		src/timer_device.cpp
		src/machine.cpp
)

add_executable(risc-v-emulator)
add_executable(cpu_test)
add_executable(memory_test)
add_executable(bus_test)
add_executable(uart_device_test)
add_executable(timer_device_test)
add_executable(timer_interrupt_csr_test)
add_executable(timer_interrupt_delivery_test)
add_executable(machine_test)
add_executable(machine_runner_test)
add_executable(uart_execution_test)
add_executable(csr_instruction_test)
add_executable(mret_test)
add_executable(trap_metadata_test)
add_executable(trap_entry_test)
add_executable(trap_delivery_test)
add_executable(fetch_test)
add_executable(decoder_test)
add_executable(execute_test)
add_executable(step_test)
add_executable(program_loader_test)
add_executable(runner_test)
add_executable(elf_loader_test)
add_executable(elf_execution_test)

target_sources(risc-v-emulator
	PRIVATE
		src/main.cpp
)

target_sources(cpu_test
	PRIVATE
		tests/cpu_test.cpp
)

target_sources(memory_test
	PRIVATE
		tests/memory_test.cpp
)

target_sources(bus_test
	PRIVATE
		tests/bus_test.cpp
)

target_sources(uart_device_test
	PRIVATE
		tests/uart_device_test.cpp
)

target_sources(timer_device_test
	PRIVATE
		tests/timer_device_test.cpp
)

target_sources(timer_interrupt_csr_test
	PRIVATE
		tests/timer_interrupt_csr_test.cpp
)

target_sources(timer_interrupt_delivery_test
	PRIVATE
		tests/timer_interrupt_delivery_test.cpp
)

target_sources(machine_test
	PRIVATE
		tests/machine_test.cpp
)

target_sources(machine_runner_test
	PRIVATE
		tests/machine_runner_test.cpp
)

target_sources(uart_execution_test
	PRIVATE
		tests/uart_execution_test.cpp
)

target_sources(csr_instruction_test
	PRIVATE
		tests/csr_instruction_test.cpp
)

target_sources(mret_test
	PRIVATE
		tests/mret_test.cpp
)

target_sources(trap_metadata_test
	PRIVATE
		tests/trap_metadata_test.cpp
)

target_sources(trap_entry_test
	PRIVATE
		tests/trap_entry_test.cpp
)

target_sources(trap_delivery_test
	PRIVATE
		tests/trap_delivery_test.cpp
)

target_sources(fetch_test
	PRIVATE
		tests/fetch_test.cpp
)

target_sources(decoder_test
	PRIVATE
		tests/decoder_test.cpp
)

target_sources(execute_test
	PRIVATE
		tests/execute_test.cpp
)

target_sources(step_test
	PRIVATE
		tests/step_test.cpp
)

target_sources(program_loader_test
	PRIVATE
		tests/program_loader_test.cpp
)

target_sources(runner_test
	PRIVATE
		tests/runner_test.cpp
)

target_sources(elf_loader_test
	PRIVATE
		tests/elf_loader_test.cpp
)

target_sources(elf_execution_test
	PRIVATE
		tests/elf_execution_test.cpp
)

target_include_directories(riscv_core
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(riscv_core
	PUBLIC
		cxx_std_20
)

target_link_libraries(risc-v-emulator
	PRIVATE
		riscv_core
)

target_link_libraries(cpu_test
	PRIVATE
		riscv_core
)

target_link_libraries(fetch_test
	PRIVATE
		riscv_core
)

target_link_libraries(memory_test
	PRIVATE
		riscv_core
)

target_link_libraries(bus_test
	PRIVATE
		riscv_core
)

target_link_libraries(uart_device_test
	PRIVATE
		riscv_core
)

target_link_libraries(timer_device_test
	PRIVATE
		riscv_core
)

target_link_libraries(timer_interrupt_csr_test
	PRIVATE
		riscv_core
)

target_link_libraries(timer_interrupt_delivery_test
	PRIVATE
		riscv_core
)

target_link_libraries(machine_test
	PRIVATE
		riscv_core
)

target_link_libraries(machine_runner_test
	PRIVATE
		riscv_core
)

target_link_libraries(uart_execution_test
	PRIVATE
		riscv_core
)

target_link_libraries(csr_instruction_test
	PRIVATE
		riscv_core
)

target_link_libraries(mret_test
	PRIVATE
		riscv_core
)

target_link_libraries(trap_metadata_test
	PRIVATE
		riscv_core
)

target_link_libraries(trap_entry_test
	PRIVATE
		riscv_core
)

target_link_libraries(trap_delivery_test
	PRIVATE
		riscv_core
)

target_link_libraries(decoder_test
	PRIVATE
		riscv_core
)

target_link_libraries(execute_test
	PRIVATE
		riscv_core
)

target_link_libraries(step_test
	PRIVATE
		riscv_core
)

target_link_libraries(program_loader_test
	PRIVATE
		riscv_core
)

target_link_libraries(runner_test
	PRIVATE
		riscv_core
)

target_link_libraries(elf_loader_test
	PRIVATE
		riscv_core
)

target_link_libraries(elf_execution_test
	PRIVATE
		riscv_core
)

enable_project_warnings(riscv_core)
enable_project_warnings(risc-v-emulator)
enable_project_warnings(cpu_test)
enable_project_warnings(memory_test)
enable_project_warnings(bus_test)
enable_project_warnings(uart_device_test)
enable_project_warnings(timer_device_test)
enable_project_warnings(timer_interrupt_csr_test)
enable_project_warnings(timer_interrupt_delivery_test)
enable_project_warnings(machine_test)
enable_project_warnings(machine_runner_test)
enable_project_warnings(uart_execution_test)
enable_project_warnings(csr_instruction_test)
enable_project_warnings(mret_test)
enable_project_warnings(trap_metadata_test)
enable_project_warnings(trap_entry_test)
enable_project_warnings(trap_delivery_test)
enable_project_warnings(fetch_test)
enable_project_warnings(decoder_test)
enable_project_warnings(execute_test)
enable_project_warnings(step_test)
enable_project_warnings(program_loader_test)
enable_project_warnings(runner_test)
enable_project_warnings(elf_loader_test)
enable_project_warnings(elf_execution_test)

add_test(
	NAME cpu_test
	COMMAND cpu_test
)

add_test(
	NAME memory_test
	COMMAND memory_test
)

add_test(
	NAME bus_test
	COMMAND bus_test
)

add_test(
	NAME uart_device_test
	COMMAND uart_device_test
)

add_test(
	NAME timer_device_test
	COMMAND timer_device_test
)

add_test(
	NAME timer_interrupt_csr_test
	COMMAND timer_interrupt_csr_test
)

add_test(
	NAME timer_interrupt_delivery_test
	COMMAND timer_interrupt_delivery_test
)

add_test(
	NAME machine_test
	COMMAND machine_test
)

add_test(
	NAME machine_runner_test
	COMMAND machine_runner_test
)

add_test(
	NAME uart_execution_test
	COMMAND uart_execution_test
)

add_test(
	NAME csr_instruction_test
	COMMAND csr_instruction_test
)

add_test(
	NAME mret_test
	COMMAND mret_test
)

add_test(
	NAME trap_metadata_test
	COMMAND trap_metadata_test
)

add_test(
	NAME trap_entry_test
	COMMAND trap_entry_test
)

add_test(
	NAME trap_delivery_test
	COMMAND trap_delivery_test
)

add_test(
	NAME fetch_test
	COMMAND fetch_test
)

add_test(
	NAME decoder_test
	COMMAND decoder_test
)

add_test(
	NAME execute_test
	COMMAND execute_test
)

add_test(
	NAME step_test
	COMMAND step_test
)

add_test(
	NAME program_loader_test
	COMMAND program_loader_test
)

add_test(
	NAME runner_test
	COMMAND runner_test
)

add_test(
	NAME elf_loader_test
	COMMAND elf_loader_test
)

add_test(
	NAME elf_execution_test
	COMMAND elf_execution_test
)

find_program(RISCV_GCC riscv64-elf-gcc)

if(RISCV_GCC)
	set(EXAMPLE_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/examples)
	set(ADD_C_ELF ${EXAMPLE_OUTPUT_DIR}/add_c.elf)
	set(UART_C_ELF ${EXAMPLE_OUTPUT_DIR}/uart_c.elf)
	set(TIMER_INTERRUPT_ELF ${EXAMPLE_OUTPUT_DIR}/timer_interrupt.elf)

	add_custom_command(
		OUTPUT ${ADD_C_ELF}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${EXAMPLE_OUTPUT_DIR}
		COMMAND ${RISCV_GCC}
			-march=rv32i
			-mabi=ilp32
			-ffreestanding
			-nostdlib
			-nostartfiles
			-fno-pic
			-fno-pie
			-O0
			-Wl,--build-id=none
			-T ${CMAKE_CURRENT_SOURCE_DIR}/examples/linker.ld
			${CMAKE_CURRENT_SOURCE_DIR}/examples/add_start.S
			${CMAKE_CURRENT_SOURCE_DIR}/examples/add.c
			-o ${ADD_C_ELF}
		DEPENDS
			${CMAKE_CURRENT_SOURCE_DIR}/examples/linker.ld
			${CMAKE_CURRENT_SOURCE_DIR}/examples/add_start.S
			${CMAKE_CURRENT_SOURCE_DIR}/examples/add.c
		VERBATIM
	)

	add_custom_target(add_c_example ALL
		DEPENDS ${ADD_C_ELF}
	)
	
	add_custom_command(
		OUTPUT ${UART_C_ELF}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${EXAMPLE_OUTPUT_DIR}
		COMMAND ${RISCV_GCC}
			-march=rv32i
			-mabi=ilp32
			-ffreestanding
			-nostdlib
			-nostartfiles
			-fno-pic
			-fno-pie
			-O0
			-Wl,--build-id=none
			-T ${CMAKE_CURRENT_SOURCE_DIR}/examples/linker.ld
			${CMAKE_CURRENT_SOURCE_DIR}/examples/uart_start.S
			${CMAKE_CURRENT_SOURCE_DIR}/examples/uart.c
			-o ${UART_C_ELF}
		DEPENDS
			${CMAKE_CURRENT_SOURCE_DIR}/examples/linker.ld
			${CMAKE_CURRENT_SOURCE_DIR}/examples/uart_start.S
			${CMAKE_CURRENT_SOURCE_DIR}/examples/uart.c
		VERBATIM
	)

	add_custom_target(uart_c_example ALL
		DEPENDS ${UART_C_ELF}
	)

	add_custom_command(
		OUTPUT ${TIMER_INTERRUPT_ELF}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${EXAMPLE_OUTPUT_DIR}
		COMMAND ${RISCV_GCC}
			-march=rv32i_zicsr
			-mabi=ilp32
			-ffreestanding
			-nostdlib
			-nostartfiles
			-fno-pic
			-fno-pie
			-O2
			-Wl,--build-id=none
			-T ${CMAKE_CURRENT_SOURCE_DIR}/examples/linker.ld
			${CMAKE_CURRENT_SOURCE_DIR}/examples/timer_interrupt.S
			-o ${TIMER_INTERRUPT_ELF}
		DEPENDS
			${CMAKE_CURRENT_SOURCE_DIR}/examples/linker.ld
			${CMAKE_CURRENT_SOURCE_DIR}/examples/timer_interrupt.S
		VERBATIM
	)

	add_custom_target(timer_interrupt_example ALL
		DEPENDS ${TIMER_INTERRUPT_ELF}
	)

	add_test(
		NAME add_c_example_test
		COMMAND ${CMAKE_COMMAND}
			-DEMULATOR=$<TARGET_FILE:risc-v-emulator>
			-DGUEST_ELF=${ADD_C_ELF}
			-P ${CMAKE_CURRENT_SOURCE_DIR}/tests/add_c_example_test.cmake
	)

	add_test(
		NAME uart_c_example_test
		COMMAND ${CMAKE_COMMAND}
			-DEMULATOR=$<TARGET_FILE:risc-v-emulator>
			-DGUEST_ELF=${UART_C_ELF}
			-P ${CMAKE_CURRENT_SOURCE_DIR}/tests/uart_c_example_test.cmake
	)

	add_test(
		NAME timer_interrupt_example_test
		COMMAND ${CMAKE_COMMAND}
			-DEMULATOR=$<TARGET_FILE:risc-v-emulator>
			-DGUEST_ELF=${TIMER_INTERRUPT_ELF}
			-P ${CMAKE_CURRENT_SOURCE_DIR}/tests/timer_interrupt_example_test.cmake
	)

	message(STATUS "RISC-V cross-compiler found: ${RISCV_GCC}")
else()
	message(STATUS "riscv64-elf-gcc not found; guest ELF build disabled")
endif()
