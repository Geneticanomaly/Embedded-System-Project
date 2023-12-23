
#include <xscugic.h>
#include "zynq_registers.h"
#include <stdbool.h>

#ifndef SRC_BTN_HANDLER_H_
#define SRC_BTN_HANDLER_H_

#define BUTTONS_channel		2
#define BUTTONS_AXI_ID		XPAR_AXI_GPIO_SW_BTN_DEVICE_ID

extern XScuGic xInterruptController;	// Interrupt controller instance

extern uint8_t state;
extern uint8_t counter;
extern uint8_t paramState;
extern uint8_t paramCounter;

extern float kp;
extern int kp_int;
extern float ki;
extern int ki_int;

extern volatile bool pauseFlag;

void SetupPushButtons();
void PushButtons_Intr_Handler();


#endif /* SRC_BTN_HANDLER_H_ */
