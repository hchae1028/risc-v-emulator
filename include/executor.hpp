#ifndef EXECUTOR_H_
#define EXECUTOR_H_

#include "cpu.hpp"
#include "decoder.hpp"
#include <cstdint>
#include <optional>

class Bus;

std::optional<std::uint32_t> execute_instruction(Cpu& cpu, const Instruction& instruction, Bus& bus);

#endif
