#include "bus.hpp"
#include "memory.hpp"
#include "timer_device.hpp"
#include <cassert>
#include <cstdint>
#include <stdexcept>

namespace {

template <typename Function>
void assert_out_of_range(Function function) {
	bool thrown{ false };
	try {
		function();
	} catch (const std::out_of_range&) {
		thrown = true;
	}
	assert(thrown);
}

}

int main() {
	constexpr std::uint32_t mtime_low{ 0x00u };
	constexpr std::uint32_t mtime_high{ 0x04u };
	constexpr std::uint32_t mtimecmp_low{ 0x08u };
	constexpr std::uint32_t mtimecmp_high{ 0x0Cu };

	TimerDevice timer{};
	assert(timer.size() == 16);
	assert(timer.read32(mtime_low) == 0u);
	assert(timer.read32(mtime_high) == 0u);
	assert(timer.read32(mtimecmp_low) == 0xFFFFFFFFu);
	assert(timer.read32(mtimecmp_high) == 0xFFFFFFFFu);
	assert(!timer.interrupt_pending());

	/* Each RV32 write replaces one half and preserves the other half. */
	timer.write32(mtime_low, 0x89ABCDEFu);
	assert(timer.read32(mtime_low) == 0x89ABCDEFu);
	assert(timer.read32(mtime_high) == 0u);
	timer.write32(mtime_high, 0x01234567u);
	assert(timer.read32(mtime_low) == 0x89ABCDEFu);
	assert(timer.read32(mtime_high) == 0x01234567u);

	timer.write32(mtimecmp_low, 0x76543210u);
	assert(timer.read32(mtimecmp_low) == 0x76543210u);
	assert(timer.read32(mtimecmp_high) == 0xFFFFFFFFu);
	timer.write32(mtimecmp_high, 0xFEDCBA98u);
	assert(timer.read32(mtimecmp_low) == 0x76543210u);
	assert(timer.read32(mtimecmp_high) == 0xFEDCBA98u);

	/* A tick carries into the high half and uint64_t overflow wraps to zero. */
	timer.write32(mtime_high, 5u);
	timer.write32(mtime_low, 0xFFFFFFFFu);
	timer.tick();
	assert(timer.read32(mtime_low) == 0u);
	assert(timer.read32(mtime_high) == 6u);

	timer.write32(mtime_low, 0xFFFFFFFFu);
	timer.write32(mtime_high, 0xFFFFFFFFu);
	assert(timer.interrupt_pending());
	timer.tick();
	assert(timer.read32(mtime_low) == 0u);
	assert(timer.read32(mtime_high) == 0u);
	assert(!timer.interrupt_pending());

	/* Equality asserts pending; moving the compare point forward clears it. */
	timer.write32(mtime_high, 0u);
	timer.write32(mtime_low, 10u);
	timer.write32(mtimecmp_high, 0u);
	timer.write32(mtimecmp_low, 11u);
	assert(!timer.interrupt_pending());
	timer.tick();
	assert(timer.interrupt_pending());
	timer.write32(mtimecmp_low, 12u);
	assert(!timer.interrupt_pending());
	timer.tick();
	assert(timer.interrupt_pending());

	/* Unsupported widths and offsets fail without modifying register state. */
	auto saved_time_low{ timer.read32(mtime_low) };
	auto saved_time_high{ timer.read32(mtime_high) };
	auto saved_compare_low{ timer.read32(mtimecmp_low) };
	auto saved_compare_high{ timer.read32(mtimecmp_high) };
	assert_out_of_range([&] { static_cast<void>(timer.read8(mtime_low)); });
	assert_out_of_range([&] { static_cast<void>(timer.read16(mtime_low)); });
	assert_out_of_range([&] { timer.write8(mtime_low, 0xAAu); });
	assert_out_of_range([&] { timer.write16(mtime_low, 0xAAAAu); });
	assert_out_of_range([&] { static_cast<void>(timer.read32(2u)); });
	assert_out_of_range([&] { static_cast<void>(timer.read32(16u)); });
	assert_out_of_range([&] { timer.write32(2u, 0xAAAAAAAAu); });
	assert_out_of_range([&] { timer.write32(16u, 0xAAAAAAAAu); });
	assert(timer.read32(mtime_low) == saved_time_low);
	assert(timer.read32(mtime_high) == saved_time_high);
	assert(timer.read32(mtimecmp_low) == saved_compare_low);
	assert(timer.read32(mtimecmp_high) == saved_compare_high);

	/* Bus addresses resolve to device-relative offsets without disturbing RAM. */
	constexpr std::uint32_t timer_base{ 0x02000000u };
	Memory ram{ 16 };
	ram.write32(0, 0x12345678u);
	TimerDevice mapped_timer{};
	Bus bus{};
	bus.map_device(ram, 0u);
	bus.map_device(mapped_timer, timer_base);
	bus.write32(timer_base + mtime_low, 0xFFFFFFFEu);
	bus.write32(timer_base + mtime_high, 1u);
	assert(bus.read32(timer_base + mtime_low) == 0xFFFFFFFEu);
	assert(bus.read32(timer_base + mtime_high) == 1u);
	mapped_timer.tick();
	assert(bus.read32(timer_base + mtime_low) == 0xFFFFFFFFu);
	assert(bus.read32(timer_base + mtime_high) == 1u);
	assert(ram.read32(0) == 0x12345678u);
	assert_out_of_range([&] { static_cast<void>(bus.read8(timer_base)); });
	assert_out_of_range([&] { static_cast<void>(bus.read32(timer_base + 16u)); });

	return 0;
}
