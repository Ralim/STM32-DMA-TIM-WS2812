/*
 * User.cpp
 *
 *  Created on: 30 Jun 2021
 *      Author: Ralim
 */

#include "User.h"
#include "tim.h"
#include "WS2812Timer.h"

WS2812Timer<&htim1, &hdma_tim1_up, 30, 4> leds;
void DMAHalfC(DMA_HandleTypeDef *_hdma) {
	leds.DMAHalfCompleteCallback();
}
void DMAComplete(DMA_HandleTypeDef *_hdma) {
	leds.DMACompleteCallback();

}
void userMain() {
	HAL_DMA_RegisterCallback(&hdma_tim1_up, HAL_DMA_XFER_HALFCPLT_CB_ID,
			DMAHalfC);
	HAL_DMA_RegisterCallback(&hdma_tim1_up, HAL_DMA_XFER_CPLT_CB_ID,
			DMAComplete);
//	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
	leds.start();
	for (;;) {
		HAL_Delay(10);
		leds.setLED(0, 0, HAL_GetTick() / 10, 0, 0);
		leds.setLED(1, 0, 0, HAL_GetTick() / 10, 0);
//		leds.setLED(2, 0, 0, 0, HAL_GetTick() / 10);
		leds.setLED(2, 0, 0, HAL_GetTick() / 10, HAL_GetTick() / 10);
		leds.setLED(3, 0, HAL_GetTick() / 10, HAL_GetTick() / 10, 0);
	}
}

