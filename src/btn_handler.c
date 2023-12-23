
#include "btn_handler.h"
#include "modulate_handler.h"

#include <xparameters.h>
#include <xgpio.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "semphr.h"


#define BUTTON_0_MASK 0x01
#define BUTTON_1_MASK 0x02
#define BUTTON_2_MASK 0x04
#define BUTTON_3_MASK 0x08

#define LED_GPIO XPAR_AXI_GPIO_LED_BASEADDR

XGpio BTNS_SWTS;
XGpio LED_Gpio; // Variable to hold the GPIO instance

// Used global variables
uint8_t state = 0;
uint8_t counter = 0; // Used in main.c to handle prints
uint8_t paramState = 0;
uint8_t paramCounter = 0;
float kp = 0.5;
int kp_int = (int)(0.5*100); // Used for printing the value of kp to SDK terminal
float ki = 0.5;
int ki_int = (int)(0.5*100); // Used for printing the value of ki to SDK terminal

extern float referenceVoltage; // Used for handling referenceVoltage
extern uint8_t modulateState; // Used for handling modulating states

u32 buttonState; // Used for handling button states

xSemaphoreHandle BTNsem;

void displayUartSettings();


void SetupPushButtons()
{

	vSemaphoreCreateBinary(BTNsem);

	XGpio_Initialize(&BTNS_SWTS, BUTTONS_AXI_ID);
	XGpio_InterruptEnable(&BTNS_SWTS, 0xF);
	XGpio_InterruptGlobalEnable(&BTNS_SWTS);

	Xil_ExceptionInit();

	// Enable interrupts.
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT, (Xil_ExceptionHandler) XScuGic_InterruptHandler, &xInterruptController);

	/* Defines the PushButtons_Intr_Handler as the FIQ interrupt handler.*/
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_FIQ_INT,
								 (Xil_ExceptionHandler) PushButtons_Intr_Handler,
								 &xInterruptController);
	Xil_ExceptionEnableMask(XIL_EXCEPTION_FIQ);
}


void PushButtons_Intr_Handler(void *data)
{

	buttonState = XGpio_DiscreteRead(&BTNS_SWTS, BUTTONS_channel);


	// Button 1 handler (BTN0)
	if (buttonState & BUTTON_0_MASK) {

		// If state is greater than 2 return to idle state
		// Idle -> Config -> Modul
		if (state < 2) {
			xil_printf("State changed\r\n");
			counter = 0;
			state += 1;
		} else {
			xil_printf("State changed\r\n");
			modulateState = 0;
			counter = 0;
			state = 0;
		}

	}


	// Button 2 handler (BTN1)
	if (buttonState & BUTTON_1_MASK) {

		if (state == 0) {
			displayUartSettings();
		}

		else if (state == 1) // set paramCounter to 0 and display a message in SDK terminal
		{
			paramCounter = 0;
			if (paramState == 0) // If paramState equals 0 increase it by one
			{
				paramState++;
			} else // If paramState equals 1 reset its value to 0
			{
				paramState = 0;
			}
		}
	}


	// Button 3 handler (BTN2)
	if (buttonState & BUTTON_2_MASK) {

		// Handle CONFIG state logic
		if (state == 1) // If in CONFIG mode - handle this
		{
			if (paramState == 0) // paramState = Kp, change it's value by 0.1
			{
				kp -= 0.1;
				// Check that kp value doesn't go below 0
				if (kp <= 0) {
					kp = 0;
				}
				kp_int = (kp * 100);
				xil_printf("Kp value was changed to %d.%02d\r\n", kp_int / 100, kp_int % 100);
			} else // paramState = Ki, change it's value
			{
				ki -= 0.1;
				// Check that ki value doesn't go below 1
				if (ki <= 0) {
					ki = 0;
				}
				ki_int = (ki * 100);
				xil_printf("Ki value was changed to %d.%02d\r\n", ki_int / 100, ki_int % 100);
			}
		}

		// Handle MODULATE state logic
		if (state == 2) {
			referenceVoltage -= 1;
			if (referenceVoltage <= 0) {
				referenceVoltage = 0;
			}
			xil_printf("\nReference voltage value was changed to %d\r\n\n", (int)referenceVoltage);
			modulateState = 1;
		}

	}

	// Button 4 handler (BTN3)
	if (buttonState & BUTTON_3_MASK) {

		// Handle CONFIG state logic
		if (state == 1) // If in CONFIG mode - handle this
		{
			if (paramState == 0) // paramState = Kp, change it's value
			{
				kp += 0.1;
				// Check that kp value doesn't go above 1
				if (kp >= 1) {
					kp = 1;
				}
				kp_int = (kp * 100);
				xil_printf("Kp value was changed to %d.%02d\r\n", kp_int / 100, kp_int % 100);
			} else // paramState = Ki, change it's value
			{
				ki += 0.1;
				// Check that ki value doesn't go above 1
				if (ki >= 1) {
					ki = 1;
				}
				ki_int = (ki * 100);
				xil_printf("Ki value was changed to %d.%02d\r\n", ki_int / 100, ki_int % 100);
			}
		}

		// Handle MODULATE state logic
		if (state == 2) {
			referenceVoltage += 1;
			xil_printf("\nReference voltage value was changed to %d\r\n\n", (int)referenceVoltage);
			modulateState = 1;
		}
	}


	XGpio_InterruptClear(&BTNS_SWTS,0xF);
}

void displayUartSettings() {
	xil_printf("\r\n");
	xil_printf("************ UART Settings ************\r\n");
	xil_printf("\t\t\t\t Examples for typing\r\n");
	xil_printf("\t\t To change state: change state \r\n");
	xil_printf("\t\t To change kp: kp +0.1 or kp -0.3 \r\n");
	xil_printf("\t\t To change ki: ki +0.4 or ki -0.7 \r\n");
	xil_printf("\t\t To change u_ref: u=10 \r\n");
	xil_printf("****************************************\r\n");
	xil_printf("\r\n");
}
