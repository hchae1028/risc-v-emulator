#include <stdint.h>

#define UART_BASE_ADDRESS  0x10000000u
#define UART_TX_OFFSET     0
#define UART_STATUS_OFFSET 4
#define UART_TX_READY_MASK 1

static volatile uint8_t* const UART_TX = (volatile uint8_t*)(UART_BASE_ADDRESS + UART_TX_OFFSET);

static volatile uint32_t* const UART_STATUS = (volatile uint32_t*)(UART_BASE_ADDRESS + UART_STATUS_OFFSET);

static void put_char(unsigned char c) {
	while ((*UART_STATUS & UART_TX_READY_MASK) == 0) {
	}

	*UART_TX = c;
}

static void put_string(const char* str) {
	while (*str != '\0') {
		put_char((unsigned char)*str);
		str++;
	}
}

void uart_main(void) {
	put_string("Hello, UART!\n");
}
