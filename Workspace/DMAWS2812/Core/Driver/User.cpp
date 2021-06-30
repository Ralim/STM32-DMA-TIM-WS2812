/*
 * User.cpp
 *
 *  Created on: 30 Jun 2021
 *      Author: Ralim
 */

#include "User.h"
#include "tim.h"
#include "WS2812Timer.h"

WS2812Timer<&htim1, &hdma_tim1_up, 60, 4> leds;
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
	leds.start();
}

