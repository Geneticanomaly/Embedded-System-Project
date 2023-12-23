
#ifndef SRC_TIMER_SETUP_H_
#define SRC_TIMER_SETUP_H_

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "zynq_registers.h"
#include <xscugic.h>
#include <xttcps.h>
#include <xuartps_hw.h>

#define TTC_TICK_INTR_ID     42U

extern XScuGic xInterruptController;	// Interrupt controller instance

void SetupTicker();
void SetupTimer();
void TickHandler();


#endif /* SRC_TIMER_SETUP_H_ */
