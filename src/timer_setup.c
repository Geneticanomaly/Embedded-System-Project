
#include "timer_setup.h"


void SetupTimer() {
  TTC0_CNT_CNTRL |= XTTCPS_CNT_CNTRL_DIS_MASK; //Counter Control Register: "Disable the counter"
  // Reset the count control register to it's default value.
  TTC0_CNT_CNTRL = XTTCPS_CNT_CNTRL_RESET_VALUE; //Counter Control Register:" Reset value"

  // Reset the rest of the registers to the default values.
  TTC0_CLK_CNTRL = 0;
  TTC0_INTERVAL_VAL = 0;
  TTC0_MATCH_1 = 0;
  TTC0_MATCH_2_COUNTER_2 = 0;
  TTC0_MATCH_3_COUNTER_2 = 0;
  TTC0_IER = 0;

  // Reset the counter value
  TTC0_CNT_CNTRL |= XTTCPS_CNT_CNTRL_RST_MASK; // Counter Control Register: "Reset counter"

  // Set the options
  TTC0_CNT_CNTRL = XTTCPS_CNT_CNTRL_DIS_MASK | XTTCPS_CNT_CNTRL_INT_MASK; //Counter Control Register: "Disable the counter" | "Interval mode"

  // Set the interval and prescale. Base clock is 111MHz
  // Prescale value (N): if prescale is enabled, the  count rate is divided by 2^(N+1)
  // 1 / (111MHz) * 9000 * 2^(11+1) = 0.3321... [seconds]
  TTC0_INTERVAL_VAL = 9000;
  TTC0_CLK_CNTRL &= ~(XTTCPS_CLK_CNTRL_PS_VAL_MASK | XTTCPS_CLK_CNTRL_PS_EN_MASK); // Clock Control register - clear: "Prescale value" & "Prescale enable"
  TTC0_CLK_CNTRL |= (11 << XTTCPS_CLK_CNTRL_PS_VAL_SHIFT) | XTTCPS_CLK_CNTRL_PS_EN_MASK; // Clock Control register - set: "Prescale value" & "Prescale enable"
}


void SetupTicker(void) {
	// Connect to the interrupt controller
	xInterruptController.Config->HandlerTable[TTC_TICK_INTR_ID].Handler = (Xil_InterruptHandler)TickHandler;
	// Enable the interrupt for the Timer counter
	ICDISER1 = 1<<(TTC_TICK_INTR_ID % 32);			// XScuGic_Enable(&InterruptController, TTC_TICK_INTR_ID);

	// Enable the interrupts for the tick timer/counter. We only care about the interval timeout.
	TTC0_IER |= XTTCPS_IXR_INTERVAL_MASK;			// XTtcPs_EnableInterrupts(TtcPsTick, XTTCPS_IXR_INTERVAL_MASK);
	// Start the tick timer/counter
	// taken to main
	TTC0_CNT_CNTRL &= ~XTTCPS_CNT_CNTRL_DIS_MASK;	// XTtcPs_Start(TtcPsTick);
}


void TickHandler() {
	// Read the interrupt status to clear the interrupt.
	// TTC0_ISR: Triple Timer Counter (TTC) 0 - Interrupt Status Register (ISR)
	TTC0_ISR; // Cleared on read

	AXI_LED_DATA ^= 0x8;

}
