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
)

add_executable(risc-v-emulator)
add_executable(cpu_test)
add_executable(memory_test)
add_executable(fetch_test)
add_executable(decoder_test)
add_executable(execute_test)
add_executable(step_test)
add_executable(program_loader_test)
add_executable(runner_test)
add_executable(elf_loader_test)

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

enable_project_warnings(riscv_core)
enable_project_warnings(risc-v-emulator)
enable_project_warnings(cpu_test)
enable_project_warnings(memory_test)
enable_project_warnings(fetch_test)
enable_project_warnings(decoder_test)
enable_project_warnings(execute_test)
enable_project_warnings(step_test)
enable_project_warnings(program_loader_test)
enable_project_warnings(runner_test)
enable_project_warnings(elf_loader_test)

add_test(
	NAME cpu_test
	COMMAND cpu_test
)

add_test(
	NAME memory_test
	COMMAND memory_test
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
