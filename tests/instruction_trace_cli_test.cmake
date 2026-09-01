if(NOT DEFINED EMULATOR OR NOT DEFINED GUEST_ELF)
	message(FATAL_ERROR "EMULATOR and GUEST_ELF must be provided")
endif()

execute_process(
	COMMAND "${EMULATOR}" "${GUEST_ELF}"
	RESULT_VARIABLE untraced_result
	OUTPUT_VARIABLE untraced_output
	ERROR_VARIABLE untraced_error
)

execute_process(
	COMMAND "${EMULATOR}" --trace "${GUEST_ELF}"
	RESULT_VARIABLE traced_result
	OUTPUT_VARIABLE traced_output
	ERROR_VARIABLE traced_error
)

if(NOT untraced_result EQUAL 0 OR NOT traced_result EQUAL 0)
	message(FATAL_ERROR
		"emulator failed\n"
		"untraced result: ${untraced_result}\n${untraced_error}\n"
		"traced result: ${traced_result}\n${traced_error}"
	)
endif()

set(expected_uart_output "timer x3\n")
if(NOT untraced_output STREQUAL expected_uart_output OR
	NOT traced_output STREQUAL expected_uart_output)
	message(FATAL_ERROR
		"tracing changed UART output\n"
		"untraced: [${untraced_output}]\n"
		"traced:   [${traced_output}]"
	)
endif()

if(untraced_error MATCHES "instruction=0x")
	message(FATAL_ERROR "instruction trace was emitted without --trace:\n${untraced_error}")
endif()

string(FIND "${traced_error}" "pc=0x" traced_prefix_position)
if(NOT traced_prefix_position EQUAL 0)
	message(FATAL_ERROR "trace does not begin with an instruction event:\n${traced_error}")
endif()

if(NOT traced_error MATCHES "instruction=0x00100073")
	message(FATAL_ERROR "trap-causing EBREAK was not traced:\n${traced_error}")
endif()

string(REGEX MATCH "instructions retired: [0-9A-Fa-f]+" untraced_retired "${untraced_error}")
string(REGEX MATCH "instructions retired: [0-9A-Fa-f]+" traced_retired "${traced_error}")
if(untraced_retired STREQUAL "" OR traced_retired STREQUAL "")
	message(FATAL_ERROR "missing retired-instruction diagnostic")
endif()

if(NOT traced_retired STREQUAL untraced_retired)
	message(FATAL_ERROR
		"tracing changed the retired-instruction diagnostic\n"
		"untraced: ${untraced_retired}\n"
		"traced:   ${traced_retired}"
	)
endif()
