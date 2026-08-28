#include <cstddef>
#include <cstdint>
#include <cassert>
#include <stdexcept>
#include "cpu.hpp"

template <typename Function>
void assert_out_of_range(Function function) {
	bool exception_thrown{ false };
	try {
		function();
	} catch (const std::out_of_range&) {
		exception_thrown = true;
	}
	assert(exception_thrown);
}

int main() {
	constexpr std::uint16_t mtvec_address{ 0x305u };
	constexpr std::uint16_t mepc_address{ 0x341u };
	constexpr std::uint16_t mcause_address{ 0x342u };
	constexpr std::uint16_t mtval_address{ 0x343u };

	Cpu cpu{};
	
	auto pc = cpu.read_pc();
	
	// Zero-initialization tests
	assert(pc == 0);
	
	for (std::size_t i = 0; i < 32; i++) {
		auto register_value = cpu.read_register(i);
		assert(register_value == 0);
	}
	assert(cpu.read_csr(mtvec_address) == 0);
	assert(cpu.read_csr(mepc_address) == 0);
	assert(cpu.read_csr(mcause_address) == 0);
	assert(cpu.read_csr(mtval_address) == 0);

	// write_register() tests
	std::uint32_t expected {0x12345678};
	cpu.write_register(1, 0x12345678);
	assert(cpu.read_register(1) == expected);

	expected = 0xFFFFFFFF;
	cpu.write_register(31, 0xFFFFFFFF);
	assert(cpu.read_register(31) == expected);

	cpu.write_register(0, 0x12345678);
	assert(cpu.read_register(0) == 0);
	
	// Other registers' values do not change
	assert(cpu.read_register(31) == expected);

	// pc test
	cpu.set_pc(0x1000);
	assert(cpu.read_pc() == 0x1000);

	// mtvec supports Direct mode only and keeps a four-byte-aligned base
	cpu.write_csr(mtvec_address, 0x1234567Fu);
	assert(cpu.read_csr(mtvec_address) == 0x1234567Cu);
	cpu.write_csr(mtvec_address, 0x00000101u);
	assert(cpu.read_csr(mtvec_address) == 0x00000100u);

	// Fixed IALIGN=32 forces both low mepc bits to zero
	cpu.write_csr(mepc_address, 0xFFFFFFFFu);
	assert(cpu.read_csr(mepc_address) == 0xFFFFFFFCu);

	// mcause and mtval preserve all written bits
	cpu.write_csr(mcause_address, 0x80000007u);
	cpu.write_csr(mtval_address, 0xDEADBEEFu);
	assert(cpu.read_csr(mcause_address) == 0x80000007u);
	assert(cpu.read_csr(mtval_address) == 0xDEADBEEFu);

	// CSR accesses do not disturb general registers or the PC
	assert(cpu.read_pc() == 0x1000u);
	assert(cpu.read_register(0) == 0);
	assert(cpu.read_register(1) == 0x12345678u);
	assert(cpu.read_register(31) == 0xFFFFFFFFu);

	// Unsupported addresses fail without changing supported CSR state
	assert_out_of_range([&cpu] { static_cast<void>(cpu.read_csr(0x000u)); });
	assert_out_of_range([&cpu] { cpu.write_csr(0xFFFu, 0xAAAAAAAAu); });
	assert(cpu.read_csr(mtvec_address) == 0x00000100u);
	assert(cpu.read_csr(mepc_address) == 0xFFFFFFFCu);
	assert(cpu.read_csr(mcause_address) == 0x80000007u);
	assert(cpu.read_csr(mtval_address) == 0xDEADBEEFu);

	return 0;
}
