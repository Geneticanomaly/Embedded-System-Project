
#ifndef SRC_UART_SETUP_H_
#define SRC_UART_SETUP_H_

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include <xscugic.h>
#include "zynq_registers.h"
#include <xuartps_hw.h>

#define UARTPS_1_INTR        82U
#define BUFFER_SIZE 128

void SetupUART();
void SetupUARTInterrupt();
void UartIRQHandler();
char uart_receive();
int int_receive_v2();

extern XScuGic xInterruptController;	// Interrupt controller instance
extern char uart_input[BUFFER_SIZE];

#endif /* SRC_UART_SETUP_H_ */
