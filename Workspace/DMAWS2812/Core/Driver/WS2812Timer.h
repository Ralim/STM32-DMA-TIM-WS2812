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
		htim->Instance->DCR = ((timerChannels - 1) << 8) | 13; // timerChannels number of transfers, with an offset to CCR1 (13)
		// ~> Prep the dma to point to the dma burst registers
		HAL_DMA_Start_IT(hdma, (uint32_t) DMAPrepBuffer,
				(uint32_t) &(htim->Instance->DMAR), timerChannels * 3 * 8 * 2);
		memset(rawrgb, 0, sizeof(rawrgb));
		memset(DMAPrepBuffer, 0, sizeof(DMAPrepBuffer));

		DMACompleteCallback();
		DMAHalfCompleteCallback();
		// ~> Start the Timer running
		/* Enable the TIM Update DMA request */
		__HAL_TIM_ENABLE_DMA(htim, TIM_DMA_UPDATE);
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
		fillSection(&DMAPrepBuffer[timerChannels * 3 * 8]);
	}
	void setLED(const uint8_t channel, const int led, const uint8_t r,
			const uint8_t g, const uint8_t b) {
		/*
		 * To reduce send time complexity
		 * Data is stored in the order of
		 * 1b0,2b0,3b0,4b0,1b1,2b1 etc
		 * Channel_bit_bitNumber
		 */
		int startingBitPosition = (led * /*NumberOfBitsPerLedBlockOfChannels*/
		(timerChannels * 3/*BytesPerLed*/* 8/*BitsPerByte*/));
		startingBitPosition += channel; // Channel offset in for starting bit 0
		//Expand bits out for ease + flip RGB -> GRB
		uint32_t totalBits = ((uint32_t) g) << 16 | ((uint32_t) r) << 8
				| ((uint32_t) b);
		for (int bitNum = 0; bitNum < 24; bitNum++) {
			setBit(startingBitPosition, totalBits & (0x01 << (23 - bitNum)));
			startingBitPosition += timerChannels;
		}

	}
private:

	void setBit(int offset, bool value) {
		rawrgb[offset / 8] &= ~(1 << (offset) % 8);
		rawrgb[offset / 8] |= value << (offset) % 8;
	}

	/*
	 * State is used to track where we are up to in generating the data
	 *  Steps 0 -> numLeds track which led we are up to generating
	 *  Then we change over to doing the reset pulse at the end to latch
	 */
	int state = 0;
	const int finalState = (numLeds) + 10;
	const int bytesPerLEDChunk = 3/*Bytes*/* timerChannels;
	uint8_t rawrgb[(timerChannels * numLeds * 3) / 4];
	uint16_t DMAPrepBuffer[timerChannels * 3/*RGB*/* 8/*bits*/* 2/*LEDs*/];	// Room for 2 leds, RGB, per channel, 8 bits per channel
	const uint16_t timingNumbers[2] = { 4, 8 };
	void fillSection(uint16_t *buffer) {
		//Fill half a DMA buffer worth
		if (state >= numLeds) {
			memset(buffer, 0x00,
					3/*RGB*/* timerChannels * 8/*bits*/* sizeof(uint16_t));	//a leds worth of bytes
		} else {
			int startPos = state * bytesPerLEDChunk;
			int bufferPos = 0;
			for (int bytePos = 0; bytePos < bytesPerLEDChunk; bytePos++) {
				uint8_t b = rawrgb[startPos + bytePos];
				for (int bitPos = 0; bitPos < 8; bitPos++) {
					uint8_t bit = (b >> bitPos) & 0x01;
					buffer[bufferPos] = timingNumbers[bit];
					bufferPos++;
				}
			}

		}
		state++;
		state %= finalState;
	}
};

#endif /* DRIVER_WS2812TIMER_H_ */
