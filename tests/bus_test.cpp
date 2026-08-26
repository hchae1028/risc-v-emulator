#include "bus.hpp"
#include "bus_device.hpp"
#include "memory.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace {

class RecordingDevice final : public BusDevice {
private:
	std::size_t m_size;

	void record_access(std::uint32_t address, std::size_t width) {
		last_address = address;
		last_width = width;
		access_count++;
	}

public:
	explicit RecordingDevice(std::size_t size = 16)
		: m_size{ size }
	{
	}

	std::size_t access_count{};
	std::size_t read8_count{};
	std::uint32_t last_address{};
	std::size_t last_width{};
	std::uint32_t last_write_value{};

	std::size_t size() const override {
		return m_size;
	}

	std::uint8_t read8(std::uint32_t address) override {
		record_access(address, 1);
		read8_count++;
		return read8_count == 1 ? std::uint8_t{ 0xA5u } : std::uint8_t{ 0x5Au };
	}

	std::uint16_t read16(std::uint32_t address) override {
		record_access(address, 2);
		return 0xBEEFu;
	}

	std::uint32_t read32(std::uint32_t address) override {
		record_access(address, 4);
		return 0x89ABCDEFu;
	}

	void write8(std::uint32_t address, std::uint8_t value) override {
		record_access(address, 1);
		last_write_value = value;
	}

	void write16(std::uint32_t address, std::uint16_t value) override {
		record_access(address, 2);
		last_write_value = value;
	}

	void write32(std::uint32_t address, std::uint32_t value) override {
		record_access(address, 4);
		last_write_value = value;
	}
};

}

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

template <typename Function>
void assert_invalid_argument(Function function) {
	bool exception_thrown{ false };
	try {
		function();
	} catch (const std::invalid_argument&) {
		exception_thrown = true;
	}
	assert(exception_thrown);
}

int main() {
	/* Physical addresses are translated to offsets in the mapped RAM */
	Memory ram{ 16 };
	Bus bus{ ram, 0x1000u };

	ram.write32(0, 0x12345678u);
	assert(bus.read32(0x1000u) == 0x12345678u);

	bus.write8(0x1004u, 0xA5u);
	bus.write16(0x1006u, 0xBEEFu);
	bus.write32(0x1008u, 0x89ABCDEFu);
	assert(ram.read8(4) == 0xA5u);
	assert(ram.read16(6) == 0xBEEFu);
	assert(ram.read32(8) == 0x89ABCDEFu);

	/* The bus delegates little-endian byte layout to Memory */
	assert(ram.read8(8) == 0xEFu);
	assert(ram.read8(9) == 0xCDu);
	assert(ram.read8(10) == 0xABu);
	assert(ram.read8(11) == 0x89u);

	/* Alignment is not a bus responsibility */
	ram.write32(1, 0x0BADF00Du);
	assert(bus.read32(0x1001u) == 0x0BADF00Du);
	bus.write16(0x1003u, 0x1357u);
	assert(ram.read16(3) == 0x1357u);

	/* Addresses outside the half-open RAM mapping are rejected */
	assert_out_of_range([&bus] { static_cast<void>(bus.read8(0x0FFFu)); });
	assert_out_of_range([&bus] { static_cast<void>(bus.read8(0x1010u)); });
	assert_out_of_range([&bus] { bus.write8(0x1010u, 0xFFu); });

	/* Multi-byte accesses may not cross the upper mapping boundary */
	ram.write8(15, 0x5Au);
	assert_out_of_range([&bus] { static_cast<void>(bus.read16(0x100Fu)); });
	assert_out_of_range([&bus] { static_cast<void>(bus.read32(0x100Du)); });
	assert_out_of_range([&bus] { bus.write16(0x100Fu, 0xFFFFu); });
	assert_out_of_range([&bus] { bus.write32(0x100Du, 0xFFFFFFFFu); });
	assert(ram.read8(15) == 0x5Au);

	/* The last 32-bit physical address can hold a one-byte mapping */
	Memory top_byte_ram{ 1 };
	Bus top_byte_bus{ top_byte_ram, 0xFFFFFFFFu };
	top_byte_bus.write8(0xFFFFFFFFu, 0xC3u);
	assert(top_byte_bus.read8(0xFFFFFFFFu) == 0xC3u);
	assert(top_byte_ram.read8(0) == 0xC3u);

	/* A device mapping may not extend beyond the 32-bit address space */
	Memory overflowing_ram{ 2 };
	assert_invalid_argument([&overflowing_ram] {
		Bus overflowing_bus{ overflowing_ram, 0xFFFFFFFFu };
		static_cast<void>(overflowing_bus);
	});

	/* Bus dispatches through BusDevice and supplies device-local offsets */
	RecordingDevice device{};
	Bus device_bus{ device, 0x2000u };

	// Repeated reads can mutate device state
	assert(device_bus.read8(0x2001u) == 0xA5u);
	assert(device.read8_count == 1);
	assert(device.last_address == 1);
	assert(device.last_width == 1);
	assert(device_bus.read8(0x2002u) == 0x5Au);
	assert(device.read8_count == 2);
	assert(device.last_address == 2);

	assert(device_bus.read16(0x2004u) == 0xBEEFu);
	assert(device.last_address == 4);
	assert(device.last_width == 2);
	assert(device_bus.read32(0x2008u) == 0x89ABCDEFu);
	assert(device.last_address == 8);
	assert(device.last_width == 4);

	device_bus.write8(0x2003u, 0x11u);
	assert(device.last_address == 3);
	assert(device.last_width == 1);
	assert(device.last_write_value == 0x11u);
	device_bus.write16(0x2006u, 0x2233u);
	assert(device.last_address == 6);
	assert(device.last_width == 2);
	assert(device.last_write_value == 0x2233u);
	device_bus.write32(0x200Cu, 0x44556677u);
	assert(device.last_address == 12);
	assert(device.last_width == 4);
	assert(device.last_write_value == 0x44556677u);

	/* An empty bus has no mapped addresses */
	Bus empty_bus{};
	assert_out_of_range([&empty_bus] { static_cast<void>(empty_bus.read8(0)); });
	assert_out_of_range([&empty_bus] { empty_bus.write32(0, 0xFFFFFFFFu); });

	/* Multiple devices are selected by physical address */
	RecordingDevice first_device{ 8 };
	RecordingDevice second_device{ 8 };
	Bus multi_bus{};
	multi_bus.map_device(first_device, 0x3000u);
	multi_bus.map_device(second_device, 0x4000u);

	assert(multi_bus.read8(0x3002u) == 0xA5u);
	assert(first_device.last_address == 2);
	assert(first_device.access_count == 1);
	assert(second_device.access_count == 0);
	multi_bus.write16(0x4004u, 0xCAFEu);
	assert(second_device.last_address == 4);
	assert(second_device.last_width == 2);
	assert(second_device.last_write_value == 0xCAFEu);
	assert(first_device.access_count == 1);

	// The addresses between mappings remain unmapped
	assert_out_of_range([&multi_bus] { static_cast<void>(multi_bus.read8(0x3008u)); });
	assert_out_of_range([&multi_bus] { static_cast<void>(multi_bus.read8(0x3FFFu)); });

	/* Adjacent mappings are valid, but one access cannot span both */
	RecordingDevice adjacent_first{ 4 };
	RecordingDevice adjacent_second{ 4 };
	Bus adjacent_bus{};
	adjacent_bus.map_device(adjacent_first, 0x5000u);
	adjacent_bus.map_device(adjacent_second, 0x5004u);
	assert(adjacent_bus.read32(0x5000u) == 0x89ABCDEFu);
	assert(adjacent_first.last_address == 0);
	assert(adjacent_bus.read32(0x5004u) == 0x89ABCDEFu);
	assert(adjacent_second.last_address == 0);
	const auto first_accesses_before_crossing{ adjacent_first.access_count };
	const auto second_accesses_before_crossing{ adjacent_second.access_count };
	assert_out_of_range([&adjacent_bus] { static_cast<void>(adjacent_bus.read16(0x5003u)); });
	assert_out_of_range([&adjacent_bus] { adjacent_bus.write32(0x5002u, 0xFFFFFFFFu); });
	assert(adjacent_first.access_count == first_accesses_before_crossing);
	assert(adjacent_second.access_count == second_accesses_before_crossing);

	/* Invalid mappings are rejected before changing the table */
	RecordingDevice existing_device{ 8 };
	RecordingDevice overlapping_device{ 4 };
	RecordingDevice adjacent_device{ 4 };
	Bus validated_bus{ existing_device, 0x6000u };
	assert_invalid_argument([&validated_bus, &overlapping_device] {
		validated_bus.map_device(overlapping_device, 0x6007u);
	});
	assert(validated_bus.read8(0x6007u) == 0xA5u);
	assert(overlapping_device.access_count == 0);
	validated_bus.map_device(adjacent_device, 0x6008u);
	assert(validated_bus.read8(0x6008u) == 0xA5u);
	assert(adjacent_device.last_address == 0);

	RecordingDevice empty_device{ 0 };
	assert_invalid_argument([&validated_bus, &empty_device] {
		validated_bus.map_device(empty_device, 0x7000u);
	});
	assert_out_of_range([&validated_bus] { static_cast<void>(validated_bus.read8(0x7000u)); });

	/* The same device may appear in two non-overlapping mappings */
	RecordingDevice mirrored_device{ 4 };
	Bus mirrored_bus{};
	mirrored_bus.map_device(mirrored_device, 0x8000u);
	mirrored_bus.map_device(mirrored_device, 0x9000u);
	mirrored_bus.write8(0x8001u, 0x11u);
	assert(mirrored_device.last_address == 1);
	mirrored_bus.write8(0x9002u, 0x22u);
	assert(mirrored_device.last_address == 2);
	assert(mirrored_device.last_write_value == 0x22u);

	return 0;
}
