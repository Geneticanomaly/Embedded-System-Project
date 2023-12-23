
#include "uart_setup.h"

#define BUFFER_SIZE 128
char uart_input[BUFFER_SIZE];

void SetupUART(){
        uint32_t r = 0; // Temporary value variable

        r = UART_CTRL;
        r &= ~(XUARTPS_CR_TX_EN | XUARTPS_CR_RX_EN); // Clear Tx & Rx Enable
        r |= XUARTPS_CR_RX_DIS | XUARTPS_CR_TX_DIS; // Tx & Rx Disable
        UART_CTRL = r;

        UART_MODE = 0;
        UART_MODE &= ~XUARTPS_MR_CLKSEL; // Clear "Input clock selection" - 0: clock source is uart_ref_clk
        UART_MODE |= XUARTPS_MR_CHARLEN_8_BIT; // Set "8 bits data"
        UART_MODE |= XUARTPS_MR_PARITY_NONE; // Set "No parity mode"
        UART_MODE |= XUARTPS_MR_STOPMODE_1_BIT; // Set "1 stop bit"
        UART_MODE |= XUARTPS_MR_CHMODE_NORM; // Set "Normal mode"

        // baud_rate = sel_clk / (CD * (BDIV + 1) (ref: UG585 - TRM - Ch. 19 UART)
        UART_BAUD_DIV = 6; // ("BDIV")
        UART_BAUD_GEN = 124; // ("CD")
        // Baud Rate = 100Mhz / (124 * (6 + 1)) = 115200 bps

        UART_CTRL |= (XUARTPS_CR_TXRST | XUARTPS_CR_RXRST); // TX & RX logic reset

        r = UART_CTRL;
        r |= XUARTPS_CR_RX_EN | XUARTPS_CR_TX_EN; // Set TX & RX enabled
        r &= ~(XUARTPS_CR_RX_DIS | XUARTPS_CR_TX_DIS); // Clear TX & RX disabled
        UART_CTRL = r;

 }

void SetupUARTInterrupt(){

	 uint32_t r = 0; // Temporary value variable

	 r = UART_CTRL;
	 r &= ~(XUARTPS_CR_TX_EN | XUARTPS_CR_RX_EN); // Clear Tx & Rx Enable
	 r |= XUARTPS_CR_RX_DIS | XUARTPS_CR_TX_DIS; // Tx & Rx Disable
	 UART_CTRL = r;

	 // Connect to the interrupt controller (pointer to function)
	 xInterruptController.Config->HandlerTable[UARTPS_1_INTR].Handler = (Xil_InterruptHandler) UartIRQHandler;

	 // ICDISER1:  Interrupt Controller Distributor (ICD) - Interrupt Set-enable Register (ISER) 2
	 ICDISER2 = 1 << (UARTPS_1_INTR % 32); // Modulo operator (% 2^n) stripping off all but the n LSB bits (removes offset 32)

	 UART_IER = 0 | XUARTPS_IXR_RXOVR;

	 UART_IMR = 0 | XUARTPS_IXR_RXOVR;
	 UART_IDR = XUARTPS_IXR_MASK & ~(XUARTPS_IXR_RXOVR);

	 UART_RXWM = XUARTPS_RXWM_MASK & 1U;

	 UART_CTRL |= (XUARTPS_CR_TXRST | XUARTPS_CR_RXRST); // TX & RX logic reset

	 r = UART_CTRL;
	 r |= XUARTPS_CR_RX_EN | XUARTPS_CR_TX_EN; // Set TX & RX enabled
	 r &= ~(XUARTPS_CR_RX_DIS | XUARTPS_CR_TX_DIS); // Clear TX & RX disabled
	 UART_CTRL = r;

	 UART_ISR = XUARTPS_IXR_MASK;
}

// Check if UART receive FIFO is not empty and return the new data
char uart_receive() {
	 if ((UART_STATUS & XUARTPS_SR_RXEMPTY) == XUARTPS_SR_RXEMPTY) return 0;
	 return UART_FIFO;
}

void UartIRQHandler() {
	UART_ISR = XUARTPS_IXR_MASK;

	static int index = 0;
	static char rx_buf[BUFFER_SIZE];


	char temp = uart_receive();

	// Check for termination characters
	if (temp == '\r' || temp == '\n') {
		// Null-terminate the buffer
		rx_buf[index] = '\0';

		// Process the complete message
		xil_printf("Received message: %s\r\n", rx_buf);
		strncpy(uart_input, rx_buf, sizeof(uart_input));

		// Reset the buffer index
		index = 0;
	} else {
		// Store the character in the buffer
		if (index < BUFFER_SIZE - 1) {  // Avoid buffer overflow
			rx_buf[index] = temp;
			index++;
		}
	}
}


