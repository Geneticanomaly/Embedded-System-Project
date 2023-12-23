#include <xscugic.h>
#include "zynq_registers.h"
#include <stdbool.h>

#ifndef SRC_MODULATE_HANDLER_H_
#define SRC_MODULATE_HANDLER_H_

float PID_Controller(float y_ref, float y_act ,float Ki, float Kp);
float converterModel(float u_in);
void handleRGBLed(float led_brightness);
void initializeRGBLed();


#endif /* SRC_MODULATE_HANDLER_H_ */
