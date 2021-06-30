/*
 * WS2812Timer.h
 *
 *  Created on: 30 Jun 2021
 *      Author: Ralim
 */

#ifndef DRIVER_WS2812TIMER_H_
#define DRIVER_WS2812TIMER_H_
#include "stm32f4xx_hal.h"
#include "main.h"
#include <string.h>

/*
 * Timing
 * 0 bit time high = ~350ns (200-500)
 * 1 bit time high = ~700ns (550+)
 * min time low    = ~600ns (450-5000)
 * Reset time low  = 6000ns +
 */
template<TIM_HandleTypeDef *htim, DMA_HandleTypeDef *hdma, int numLeds,
		int timerChannels> class WS2812Timer {
public:
	void start() {

		// ~> Setup Timer burst mode length & base address
		htim->Instance->DCR = (timerChannels) << 8 | 13; // timerChannels number of transfers, with an offset to CCR1 (13)
		// ~> Prep the dma to point to the dma burst registers
		HAL_DMA_Start_IT(hdma, (uint32_t) DMAPrepBuffer,
				(uint32_t) &(htim->Instance->DMAR), timerChannels * 10);
		memset(rawrgb, 0, sizeof(rawrgb));
		rawrgb[0][0] = 0xAA;
		rawrgb[1][1] = 0xAA;
		rawrgb[2][2] = 0xAA;
		fillSection(DMAPrepBuffer);
		fillSection(DMAPrepBuffer + timerChannels * 3 * 2 * 8);
		// ~> Start the Timer running
		HAL_TIM_Base_Start_IT(htim);
		HAL_TIM_PWM_Start(htim, TIM_CHANNEL_1);
		HAL_TIM_PWM_Start(htim, TIM_CHANNEL_2);
		HAL_TIM_PWM_Start(htim, TIM_CHANNEL_3);
		HAL_TIM_PWM_Start(htim, TIM_CHANNEL_4);
	}
	void DMAHalfCompleteCallback() {
		//When DMA is half Complete
		fillSection(DMAPrepBuffer);
	}
	void DMACompleteCallback() {
		//When DMA Completes
		fillSection(DMAPrepBuffer + timerChannels * 3 * 2 * 8);
	}
private:
	/*
	 * State is used to track where we are up to in generating the data
	 *  Steps 0 -> numLeds track which led we are up to generating
	 *  Then we change over to doing the reset pulse at the end to latch
	 */
	int state = 0;
	const int finalState = (numLeds) + 2;
	uint8_t rawrgb[timerChannels][numLeds * 3];
	uint16_t DMAPrepBuffer[timerChannels * 3 * 4 * 8];// Room for 4 leds, RGB, per channel, 8 bits per channel
	const uint16_t timingNumbers[2] = { 35, 70 };
	void fillSection(uint16_t *buffer) {
		//Fill half a DMA buffer worth
		if (state >= numLeds) {
			HAL_GPIO_WritePin(DEBUG_GPIO_Port, DEBUG_Pin, GPIO_PIN_SET);
			memset(buffer, 0, 2 * 3 * timerChannels * 8);//2 leds worth of bytes
		} else {
			HAL_GPIO_WritePin(DEBUG_GPIO_Port, DEBUG_Pin, GPIO_PIN_RESET);
			//Buffer is 1,2,3,4,1,2,3,4
			//But it is the _bits_ in that order
			int pos = 0;
			for (int led = 0; led < 2; led++) {
				int ledx = led + state; // actual led pos
				for (int colour = 0; colour < 3; colour++) {
					ledx *= 3;
					ledx += colour;
					for (int bitp = 0; bitp < 8; bitp++) {
						for (int chan = 0; chan < timerChannels; chan++) {
							uint8_t c = rawrgb[chan][ledx];
							//Append the bit for this channel
							//MSB first
							uint8_t bit = c >> (7 - bitp) & 0x01;
							buffer[pos] = timingNumbers[bit];
							pos++;
						}
					}
				}
			}
		}
		state += 2;		//2 leds per update
		state %= finalState;
	}
};

#endif /* DRIVER_WS2812TIMER_H_ */
