#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include <xparameters.h>
#include <xuartps_hw.h>
#include <xscugic.h>
#include <xgpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "zynq_registers.h"


// Include header files
#include "timer_setup.h"
#include "uart_setup.h"
#include "btn_handler.h"
#include "modulate_handler.h"

#define LEDMASKA 0x0001 // LED 	0
#define LEDMASKB 0x0002 // LED  1
#define LEDMASKC 0x0004 // LED  2
#define LEDMASKD 0x0008 // LED  3

#define BUFFER_SIZE 128

// States for main program modes
#define MODE_IDLE       0
#define MODE_CONFIG     1
#define MODE_MODULATE   2

// States for parameter modes
#define PARAM_KP        0
#define PARAM_KI        1

// States for modulate modes
#define MODUL_IDLE		0
#define MODUL_INIT		1
#define MODUL_TRANSMIT	2


volatile uint8_t currentParam = PARAM_KP;
float referenceVoltage = 45.0;  // Initial reference voltage

uint8_t modulateState = 0;
int modulCounter = 0;


void vModeRotationTask();
void vParameterTask();
void vModulateTask();

void SetupInterrupts();
void displayIdleMenu();
void displayConfigMenu();
void displayModulateMenu();
void checkUartSyntax();
void initializeRGBLed();

extern XScuGic xInterruptController;

int main() {
    // Initialization code here
	SetupInterrupts();
	SetupUART();
	SetupUARTInterrupt();
	SetupTimer();
	SetupTicker();
	SetupPushButtons();
	initializeRGBLed();

    AXI_BTN_TRI |= 0xF; 	// Set direction for buttons 0..3 ,  0 means output, 1 means input
    AXI_LED_TRI = ~0xF;


    // Check if the second led is on - If it is turn it off
    if (AXI_LED_DATA & LEDMASKB) {
    	AXI_LED_DATA &= ~(LEDMASKB); // Turn 1st LED off
	}

    // Create tasks
    xTaskCreate(vModeRotationTask, "ModeRotation", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(vParameterTask, "ParameterSelectingTask", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(vModulateTask, "ModulatingTask", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

    // Start FreeRTOS scheduler
    vTaskStartScheduler();

    // Will never reach here
    return 0;
}


void vModeRotationTask() {

    while (1) {

    	checkUartSyntax();

        switch (state) {
        	case MODE_IDLE:

        		if (counter == 0) // Print to console only on first enter
        		{
        			vTaskDelay(pdMS_TO_TICKS(50));
        			AXI_LED_DATA &= ~(LEDMASKC); // Turn 3rd LED off

        			// Check if the led is off
        			if (!(AXI_LED_DATA & LEDMASKA)) {
        				AXI_LED_DATA ^= LEDMASKA; // Turn 1st LED on
        			}
        			displayIdleMenu();
        			counter++;
        			paramCounter = 0;
        		}
				break;
            case MODE_CONFIG:

            	if (counter == 0) // Print to console only on first enter
            	{
            		vTaskDelay(pdMS_TO_TICKS(50));
            		AXI_LED_DATA &= ~(LEDMASKA); // Turn 1st LED off

            		AXI_LED_DATA ^= LEDMASKB; // Turn 2nd LED on
            		displayConfigMenu();
					counter++;
				}
                break;
            case MODE_MODULATE:

            	if (counter == 0) // Print to console only on first enter
            	{
            		vTaskDelay(pdMS_TO_TICKS(50));
            		AXI_LED_DATA &= ~(LEDMASKB); // Turn 2nd LED off
            		AXI_LED_DATA ^= LEDMASKC; // Turn 3rd LED on
					displayModulateMenu();
					modulateState = MODUL_INIT;
					counter++;
				}
                break;
            default:
                // Handle unexpected mode
                break;
        }
    }
}

void vParameterTask() {
	while(1)
	{
		// Added small delay to sync the parameter changes correctly in the SDK
		vTaskDelay(pdMS_TO_TICKS(50));
		switch(paramState) // paramState defined and handled in btn_handler
		{
		case PARAM_KP:
			if (state == 1) {
				if (paramCounter == 0) // Print to console only on first enter
				{

					xil_printf("\nKp parameter selected, value = %d.%02d\r\n", kp_int / 100, kp_int % 100);
					paramCounter++;
				}
			}
			break;

		case PARAM_KI:
			if (state == 1) {
				if (paramCounter == 0) // Print to console only on first enter
				{

					xil_printf("\nKi parameter selected, value = %d.%02d\r\n", ki_int / 100, ki_int % 100);
					paramCounter++;
				}
			}
			break;

		}
	}
}

void vModulateTask() {

	float u_in = 0.0;
	float u_out = 0.0;

	float prevReferenceVoltage = 0;
	float prevKi = 0;
	float prevKp = 0;

	while (1)
	{
		switch(modulateState)
		{
		case MODUL_IDLE:
			// Do nothing
			vTaskDelay(pdMS_TO_TICKS(100));
			break;
		case MODUL_INIT: // Initialize output voltage

			// Only accuire new output voltage if any parameter is changed
			// Reference voltage, Kp or Ki
			if (prevReferenceVoltage != referenceVoltage || prevKi != ki || prevKp != kp) {
				u_in = PID_Controller(referenceVoltage, u_out, ki, kp);
				u_out = converterModel(u_in);
			}

			// Get previous values for parameters
			prevReferenceVoltage = referenceVoltage;
			prevKi = ki;
			prevKp = kp;

			// Switch to transmitting mode
			modulateState = MODUL_TRANSMIT;
			break;
		case MODUL_TRANSMIT: // Turn on the RGB led
			handleRGBLed(u_out); // Turn the led on based on u_out
			break;
		}
	}
}

void SetupInterrupts()
{
	XScuGic_Config *pxGICConfig;

	/* Ensure no interrupts execute while the scheduler is in an inconsistent
	state.  Interrupts are automatically enabled when the scheduler is
	started. */
	portDISABLE_INTERRUPTS();

	/* Obtain the configuration of the GIC. */
	pxGICConfig = XScuGic_LookupConfig( XPAR_SCUGIC_SINGLE_DEVICE_ID );

	/* Install a default handler for each GIC interrupt. */
	XScuGic_CfgInitialize( &xInterruptController, pxGICConfig, pxGICConfig->CpuBaseAddress );
}


void displayIdleMenu() {
	xil_printf("\r\n");
	xil_printf("*********** Idle mode ***********\r\n");
	xil_printf("\t Button 1 - Change mode \r\n");
	xil_printf("\t Button 2 - UART settings \r\n");
	xil_printf("\t\t\t  - Converter off - \r\n");
	xil_printf("**********************************\r\n");
	xil_printf("\r\n");
}

void displayConfigMenu() {
	xil_printf("\r\n");
	xil_printf("********** Configuration mode **********\r\n");
	xil_printf("\t\t Button 1 - Change mode \r\n");
	xil_printf("\t\t Button 2 - Alternate parameter \r\n");
	xil_printf("\t\t Button 3 - Decrease value -0.1 \r\n");
	xil_printf("\t\t Button 4 - Increase value +0.1 \r\n");
	xil_printf("******************************************\r\n");
	xil_printf("\r\n");
}

void displayModulateMenu() {
	xil_printf("\r\n");
	xil_printf("************ Modulate mode ************\r\n");
	xil_printf("\t\t Button 1 - Change mode \r\n");
	xil_printf("\t\t Button 3 - Decrease value -1.0 \r\n");
	xil_printf("\t\t Button 4 - Increase value +1.0 \r\n");
	xil_printf("\t\t\t\t\t - Converter on - \r\n");
	xil_printf("******************************************\r\n");
	xil_printf("\r\n");
}


// Handles UART communication syntax for parameter and state changes
void checkUartSyntax() {

	char user_input[BUFFER_SIZE];

	strncpy(user_input, uart_input, sizeof(user_input));

	// Handle state change
	// Idle -> Config -> Modulate
	if (strcmp(user_input, "state change") == 0) {
		if (state == MODE_IDLE) {
			state += 1;
			counter = 0;
		} else if (state == MODE_CONFIG) {
			state += 1;
			counter = 0;
		} else {
			state = 0;
			counter = 0;
			modulateState = 0;
		}

	}

	if (strlen(user_input) != 0) {

		// Check if it is in Config state and kp +0.3 or ki -0.1 syntax
		if (state == MODE_CONFIG) {

			// If user input is kp change to the state PARAM_KP
			if (strcmp(user_input, "kp") == 0) {
				paramState = PARAM_KP;
				paramCounter = 0;
			}
			// If user input is ki change to the state PARAM_KI
			else if (strcmp(user_input, "ki") == 0) {
				paramState = PARAM_KI;
				paramCounter = 0;
			}

			xil_printf("IN CONFIG\r\n");

			//Check syntax for Kp
			if (strncmp(user_input, "kp +", 4) == 0 && paramState == PARAM_KP) {
				// do the calculation.
				// Use pointer to get the input after kp +
				const char* restOfInput_1 = user_input + 4;

				if (atof(restOfInput_1)) {

					kp += atof(restOfInput_1);
					// Check that Kp can't go over the value of 1
					if (kp >= 1.0) {
						kp = 1.0;
					}
					kp_int = (kp * 100);
					paramCounter = 0;
				}
			}
			else if (strncmp(user_input, "kp -", 4) == 0 && paramState == PARAM_KP) {
				// do the calculation.
				// Use pointer to get the input after kp -
				const char* restOfInput_2 = user_input + 4;

				if (atof(restOfInput_2)) {

					kp -= atof(restOfInput_2);
					// Check that Kp can't go over the value of 1
					if (kp <= 0) {
						kp = 0;
					}
					kp_int = (kp * 100);
					paramCounter = 0;
				}
			}

			//Check syntax for Ki
			else if (strncmp(user_input, "ki +", 4) == 0 && paramState == PARAM_KI) {
				// do the calculation.
				// Use pointer to get the input after ki +
				const char* restOfInput_3 = user_input + 4;

				if (atof(restOfInput_3)) {

					ki += atof(restOfInput_3);
					// Check that Kp can't go over the value of 1
					if (ki >= 1.0) {
						ki = 1.0;
					}
					ki_int = (ki * 100);
					paramCounter = 0;
				}
			}
			else if (strncmp(user_input, "ki -", 4) == 0 && paramState == PARAM_KI) {
				// do the calculation.
				// Use pointer to get the input after ki -
				const char* restOfInput_4 = user_input + 4;

				if (atof(restOfInput_4)) {

					ki -= atof(restOfInput_4);
					// Check that Kp can't go over the value of 1
					if (ki <= 0) {
						ki = 0;
					}
					ki_int = (ki * 100);
					paramCounter = 0;
				}
			}

		}


		// Handle reference voltage changes through uart - example syntax = "u=10"
		if (state == MODE_MODULATE) {
			xil_printf("IN MODULATE\r\n");
			if (strncmp(user_input, "u=", 2) == 0) {

				// Utilize a pointer to get the input after u=
				const char* restOfInput_5 = user_input + 2;

				// Only do this if a number is given after u=
				if (atof(restOfInput_5)) {
					xil_printf("\nChanging u_ref value: %d\n", (int)referenceVoltage);
					referenceVoltage = atof(restOfInput_5);
					xil_printf("New u_ref value: %d\n\n", (int)referenceVoltage);
					modulateState = 1;
				}
			}
		}
		// Reset uart_input
		strcpy(uart_input, "");
	}
}
