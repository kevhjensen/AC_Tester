/*
 * AC_WaveGenerator.h
 *
 *  Created on: Jul 7, 2025
 *      Author: kevinj
 *
 *  Functions for generating a simulating CT sensor output
 *
 *  Input: Instantaneous AC mains voltage (ADS1220 at ~1930 or 1700/2 Samples/s)
 *
 *  Output: +-0.5v differential signal (MCP4725 DAC with OP-amp buffer + invert)
 *  		true bipolar differential around 1.65V, DAC scaled down with window resistor
 *
 *  V1: basic emulation, 1.0PF / 0 phase angle, current proportional to voltage
 *
 *  V2: full emulation, PF/ phase angle control
 */

#ifndef INC_AC_WAVEGENERATOR_H_
#define INC_AC_WAVEGENERATOR_H_

#include "main.h"


#endif /* INC_AC_WAVEGENERATOR_H_ */
