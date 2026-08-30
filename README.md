# RISC-V Emulator

A small, command-line RISC-V emulator written in modern C++ as a learning project in systems programming, computer architecture, and hardware/software interfaces.

## Project status

### CPU

- 32-bit RISC-V CPU
- 32 general-purpose RV32 registers and a program counter
- Fetch, decode, and execute cycle 
- Support for the following RV32I instructions:
    - Arithmetic, logical, shift, and comparison operations
    - Byte, half-word, and word loads and stores
    - Conditional branches
    - `LUI`, `AUIPC`, `JAL`, and `JALR`
    - `FENCE`, `ECALL`, and `EBREAK`
- Supported CSRs:
    - `mstatus`
    - `mie`
    - `mip`
    - `mtvec`
    - `mepc`
    - `mcause`
    - `mtval`
- Zicsr CSR instructions
- `MRET`

Compressed instructions, floating-point instructions, and other RISC-V extensions are not currently supported.

### MMIO

The emulator contains 64KB of emulated RAM along with the following address map:

```text
RAM:    0x00000000 — 0x0000FFFF
Timer:  0x02000000 — 0x0200000F
UART:   0x10000000 — 0x10000007
```

A system bus maps physical addresses to RAM or memory-mapped devices.

A stack is intialized at one byte past the last byte of RAM, 0x00010000, and grows downwards.

### ELF loading

- Validates ELF header metadata
- Traverses and processes the program-header table and load `PT_LOAD` segments into RAM
- Uses the ELF entry point for the initial PC

### UART

Emulated a small UART-like device that provides guest output.
Output data is buffered at `m_transmitted_bytes` vector and printed to stdout in `main`.

The transmit-ready status is always set since the current UART has no transmission delay.

A firmware example file is provided at `examples/uart_start.S` along with `uart.c`.

### Timer

A small 64-bit memory-mapped timer provides machine-timer interrupts.

From the base address,
`mtime` register occupies offsets 0x00 — 0x04
`mtimecmp` register occupies offsets 0x08 — 0x0C

`mtime` increments are handled by machine steps in `machine.cpp`.
A machine-timer interrupt becomes pending when `mtime` >= `mtimecmp`.

A firmware example file is provided at `examples/timer_interrupt.S`.

### Trap handling

Implemented traps can be found at `include/trap.hpp` and each is assigned with an exception code in alignment with the official RISC-V specifications.

The emulator currently supports the following trap behaviours:

- Synchronous exceptions
- Machine-timer interrupts
- Saving the interrupted PC in `mepc`
- Recording the trap cause and relevant information in `mcause` and `mtval`, respectively
- Returning from a trap with `mret`

## Build

Configure the project with the included CMake preset:

```sh
cmake --preset risc-v-emulator
```

A RISC-V cross-compiler `riscv64-elf-gcc` is required to build the firmware examples.

Build all targets:

```sh
cmake --build build
```

## Run

Each ELF executalbe tests different aspects of the emulator.
Run an ELF executable of your choice in `build/examples/`:

```sh
./build/risc-v-emulator build/examples/<ELF_of_your_choice>
```
 
## Test

Run the complete test suite with CTest:

```sh
ctest --test-dir build --output-on-failure
```

## Flow

The general flow of the working is as follows:

```text
C source files -> RISC-V cross toolchain -> ELF executable -> ELF loader -> emulator
```

## Limitations

This is a minimal RISC-V emulator for self-learning and is not meant to replicate a complete, release-ready RISC-V system.

It does not provide:

- Supervisor / user privilige modes
- Virtual memory / page tables
- OS support
- Complete RISC-V extensions
- And many more

## Learning goals

This project started off with the following motivations:

To deepen the understanding of
- Modern C++ systems programming
- The RISC-V instruction-set architecture
- CPU and memory behavior
- ELF executable structure and loading
- Cross-compilation and linker toolchains
- MMIO and device models
- Traps, exceptions, and interrupts
- Firmware and low-level debugging
- Testing binary formats and architectural behavior
