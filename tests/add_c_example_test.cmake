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

if(NOT emulator_output MATCHES "stopped by breakpoint")
	message(FATAL_ERROR "emulator did not stop at EBREAK:\n${emulator_output}")
endif()

if(NOT emulator_output MATCHES "x10: 0x0000000c")
	message(FATAL_ERROR "compiler-generated add result was not 12:\n${emulator_output}")
endif()

if(NOT emulator_output MATCHES "x2: 0x00010000")
	message(FATAL_ERROR "compiler-generated code did not restore sp:\n${emulator_output}")
endif()

if(NOT emulator_output MATCHES "x8: 0x00000000")
	message(FATAL_ERROR "compiler-generated code did not restore s0:\n${emulator_output}")
endif()
