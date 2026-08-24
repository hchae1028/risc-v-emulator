# RISC-V Emulator

A small RISC-V emulator written in modern C++ as a learning project in systems programming, computer architecture, executable formats, and hardware/software interfaces.

## Project status

The emulator currently provides:

- 32 general-purpose RV32 registers and a program counter
- Fetch, decode, and execute stages
- A substantial subset of the RV32I base instruction set, including:
  - Integer arithmetic, logical, shift, and comparison operations
  - Byte, half-word, and word loads and stores
  - Conditional branches
  - `LUI`, `AUIPC`, `JAL`, and `JALR`
  - `FENCE`, `ECALL`, and `EBREAK`
- Little-endian byte-addressable memory with access-range validation
- Alignment checks for instructions and multi-byte memory accesses
- Execution until a trap or configurable instruction limit
- ELF32 header validation for little-endian RISC-V executables
- Parsing and validation of `PT_LOAD` program-header metadata
- Unit tests for CPU state, memory, decoding, execution, stepping, program input, running, and ELF parsing

ELF segments are not yet copied into emulator memory. The command-line program still accepts a flat binary image and places it at address zero.

## Build

Configure the project with the included CMake preset:

```sh
cmake --preset risc-v-emulator
```

Build all targets:

```sh
cmake --build build
```

## Run

Run a flat little-endian RV32 instruction image:

```sh
./build/risc-v-emulator add.bin
```

`add.bin` is a simple flat-binary executable for the instructions:

```text
93 00 50 00  → 0x00500093 → addi x1, x0, 5
13 01 70 00  → 0x00700113 → addi x2, x0, 7
b3 81 20 00  → 0x002081b3 → add  x3, x1, x2
73 00 10 00  → 0x00100073 → ebreak
```

The image is loaded at address `0x00000000`, where execution begins. The emulator stops when it encounters `ECALL`, `EBREAK`, or the instruction limit, then prints the stop reason, retired-instruction count, final program counter, and register state.

The command-line executable does not load ELF files yet.

## Test

Run the complete test suite with CTest:

```sh
ctest --test-dir build --output-on-failure
```

## Source Files

- `Cpu` owns architectural register and program-counter state.
- `Memory` provides validated little-endian reads and writes.
- The decoder converts instruction words into decoded fields and operations.
- The executor applies instruction semantics to CPU and memory state.
- The runner advances the CPU until a trap or instruction budget is reached.
- The program loader reads binary files without interpreting them.
- The ELF loader parses executable metadata without modifying emulator memory.

## ELF loading work

Current ELF support validates the ELF32 identification and fixed header fields, checks the program-header table bounds, and parses `PT_LOAD` entries into segment metadata. Segment validation includes file-range bounds, file-size versus memory-size checks, and ELF alignment requirements.

## Learning goals

This project started off with the following motivations:

Deeper understanding of
- Modern C++ systems programming
- The RISC-V instruction-set architecture
- CPU and memory behavior
- ELF executable structure and loading
- Cross-compilation and linker toolchains
- MMIO and device models
- Traps, exceptions, and interrupts
- Firmware and low-level debugging
- Testing binary formats and architectural behavior
