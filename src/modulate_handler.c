

#include "modulate_handler.h"
#include "btn_handler.h"

#include "zynq_registers.h"
#include <stdlib.h>
#include <stdint.h>
#include <xttcps.h>
#include <math.h>

extern uint8_t modulateState;


float PID_Controller(float y_ref, float y_act ,float Ki, float Kp) {
	static float u1_old=0;
	float error_new, u1_new;
	float u1_max = 50.0;

	error_new = y_ref-y_act;
	u1_new = u1_old + Ki * error_new;

	if (fabs(u1_new ) > u1_max) {
		u1_new = u1_old;
	}

	u1_old = u1_new;

	xil_printf("Input voltage: %d\r\n", (int)(u1_new + Kp * error_new));

	return u1_new + Kp * error_new;
}

float converterModel(float u_in) {
    static float i1_k = 0.0;
    static float u1_k = 0.0;
    static float i2_k = 0.0;
    static float u2_k = 0.0;
    static float i3_k = 0.0;
    static float u3_k = 0.0;

    // Update values of i1(k), u1(k), i2(k), u2(k), i3(k) and u3(k)
    float I1_k_plus_h = 0.9878 * i1_k + (-0.0169) * u1_k + 0.0 * i2_k + 0.0 * u2_k + 0.0 * i3_k + 0.0 * u3_k + 0.0471 * u_in;
    float U1_k_plus_h = 0.7626 * i1_k + 0.4102 * u1_k + (-0.3251) * i2_k + 0.2023 * u2_k + (-0.2118) * i3_k + 0.3782 * u3_k + 0.0377 * u_in;
    float I2_k_plus_h = 0.5998 * i1_k + 1.0565 * u1_k + (-0.4043) * i2_k + (-0.3708) * u2_k + 0.3881 * i3_k + (-0.6930) * u3_k + 0.0404 * u_in;
    float U2_k_plus_h = 0.4355 * i1_k + 0.7672 * u1_k + 0.4326 * i2_k + (-0.5470) * u2_k + (-0.4337) * i3_k + 0.7745 * u3_k + 0.0485 * u_in;
    float I3_k_plus_h = 0.3907 * i1_k + 0.6883 * u1_k + 0.3881 * i2_k + 0.3718 * u2_k + (-0.4037) * i3_k + (-1.0648) * u3_k + 0.0373 * u_in;
    float U3_k_plus_h = 0.2101 * i1_k + 0.37 * u1_k + 0.2087 * i2_k + 0.1999 * u2_k + 0.3206 * i3_k + 0.4275 * u3_k + 0.0539 * u_in;

    // set the static values
    i1_k = I1_k_plus_h;
    u1_k = U1_k_plus_h;
    i2_k = I2_k_plus_h;
    u2_k = U2_k_plus_h;
    i3_k = I3_k_plus_h;
    u3_k = U3_k_plus_h;

    xil_printf("Updated output voltage: %d.%02d\r\n", (int)u3_k, abs((int)((u3_k) * 100) % 100));

    // Return the updated output voltage
    return u3_k;
}

void handleRGBLed(float led_brightness) {
	// Variable initialization
	volatile u32* led_register = NULL;
	led_register = &TTC0_MATCH_0;


	while(1) {
		// Update the LED register with the brightness value
		*led_register = (uint16_t)led_brightness; // Convert to uint16_t

		// If not in MODUL_TRANSMIT - Turn the RGB led off by setting the brightness value to 0.
		if (modulateState != 2) {
			led_brightness = 0.0;
			*led_register = (uint16_t)led_brightness; // Convert to uint16_t
			break;
		}

	}
}

void initializeRGBLed() {

		// Set up the 'Clock Control' -register - TTC0_CLK_CNTRLx :
		// 1. Set prescale to 0 (plus 1) (hint: (value << XTTCPS_CLK_CNTRL_PS_VAL_SHIFT)
		// 2. Enable prescaler (hint: use XTTCPS_CLK_CNTRL_PS_EN_MASK mask)
		TTC0_CLK_CNTRL  = (0 << XTTCPS_CLK_CNTRL_PS_VAL_SHIFT) | XTTCPS_CLK_CNTRL_PS_EN_MASK;
		TTC0_CLK_CNTRL2 = (0 << XTTCPS_CLK_CNTRL_PS_VAL_SHIFT) | XTTCPS_CLK_CNTRL_PS_EN_MASK; // Set identical to TTC0_CLK_CNTRL
		TTC0_CLK_CNTRL3 = (0 << XTTCPS_CLK_CNTRL_PS_VAL_SHIFT) | XTTCPS_CLK_CNTRL_PS_EN_MASK; // Set identical to TTC0_CLK_CNTRL

		// Then let's set correct values to 'Operational mode and reset' -register - TTC0_CNT_CNTRLx :
		//     1. Reset count value (hint: use XTTCPS_CNT_CNTRL_RST_MASK mask)
		//     2. Disable counter (XTTCPS_CNT_CNTRL_DIS_MASK)
		//     3. Set timer to Match mode (XTTCPS_CNT_CNTRL_MATCH_MASK)
		//     4. Set the waveform polarity (XTTCPS_CNT_CNTRL_POL_WAVE_MASK)
		//	   When this bit is high, the waveform output goes from high to low
		//	   when a match interrupt happens. If low, the waveform output goes
		//     from low to high.
		//
		//     (Waveform output is default to EMIO, which is connected in the FPGA to the RGB led (LD6)
		TTC0_CNT_CNTRL  = XTTCPS_CNT_CNTRL_RST_MASK | XTTCPS_CNT_CNTRL_DIS_MASK | XTTCPS_CNT_CNTRL_MATCH_MASK | XTTCPS_CNT_CNTRL_POL_WAVE_MASK;
		TTC0_CNT_CNTRL2 = XTTCPS_CNT_CNTRL_RST_MASK | XTTCPS_CNT_CNTRL_DIS_MASK | XTTCPS_CNT_CNTRL_MATCH_MASK | XTTCPS_CNT_CNTRL_POL_WAVE_MASK; // Set identical to TTC0_CNT_CNTRL
		TTC0_CNT_CNTRL3 = XTTCPS_CNT_CNTRL_RST_MASK | XTTCPS_CNT_CNTRL_DIS_MASK | XTTCPS_CNT_CNTRL_MATCH_MASK | XTTCPS_CNT_CNTRL_POL_WAVE_MASK; // Set identical to TTC0_CNT_CNTRL

		// Notice that we don't touch the XTTCPS_EN_WAVE_MASK bit. This is because, as we see from Appendix B.32 of Zynq TRM,
		// it is of the type "active low". So the waveform output is enabled by default and needs to be turned off by
		// writing one to that bit.

		// Match value register - TTC0_MATCH_x
		//     1. Initialize match value to 0
		TTC0_MATCH_0           = 0;
		TTC0_MATCH_1_COUNTER_2 = 0;
		TTC0_MATCH_1_COUNTER_3 = 0;

		// Operational mode and reset register - TTC0_CNT_CNTRLx
		//     1. Start timer (hint: clear operation using XTTCPS_CNT_CNTRL_DIS_MASK)
		TTC0_CNT_CNTRL  &= ~XTTCPS_CNT_CNTRL_DIS_MASK;
		TTC0_CNT_CNTRL2 &= ~XTTCPS_CNT_CNTRL_DIS_MASK;
		TTC0_CNT_CNTRL3 &= ~XTTCPS_CNT_CNTRL_DIS_MASK;
}











