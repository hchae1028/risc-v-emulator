if(NOT DEFINED EMULATOR OR NOT DEFINED GUEST_ELF)
	message(FATAL_ERROR "EMULATOR and GUEST_ELF must be provided")
endif()

execute_process(
	COMMAND "${EMULATOR}" "${GUEST_ELF}"
	RESULT_VARIABLE emulator_result
	OUTPUT_VARIABLE emulator_output
	ERROR_VARIABLE emulator_error
)

if(NOT emulator_result EQUAL 0)
	message(FATAL_ERROR
		"emulator exited with ${emulator_result}\n"
		"stdout:\n${emulator_output}\n"
		"stderr:\n${emulator_error}"
	)
endif()

set(expected_uart_output "Hello, UART!\n")
if(NOT emulator_output STREQUAL expected_uart_output)
	message(FATAL_ERROR
		"unexpected UART output\n"
		"expected: [${expected_uart_output}]\n"
		"actual:   [${emulator_output}]"
	)
endif()

if(NOT emulator_error MATCHES "stopped by breakpoint")
	message(FATAL_ERROR "emulator did not stop at EBREAK:\n${emulator_error}")
endif()

if(NOT emulator_error MATCHES "final pc value: 0x0000010c")
	message(FATAL_ERROR "emulator did not preserve the trapping EBREAK PC:\n${emulator_error}")
endif()

if(NOT emulator_error MATCHES "x2: 0x00010000")
	message(FATAL_ERROR "compiler-generated guest did not restore sp:\n${emulator_error}")
endif()
