/*
 * myAdc.h
 *
 *  Created on: Jul 22, 2025
 *      Author: kevinj
 */

#ifndef SRC_AC_T_CPREAD_H_
#define SRC_AC_T_CPREAD_H_

extern ADC_HandleTypeDef hadc1;
//DMA_HandleTypeDef hdma_adc1;
extern TIM_HandleTypeDef htim3; // originally 20kHz, then 40khz, but got aliasing in duty cycle so now non round number

#define ADC_SAMPLE_RATE_x10 384615 //71Pre25 //38919 9Pre184 // 40k 71Pre24 ADC conv started on a 40kHz timer, fed to buf via DMA
#define EVSE_CP_ADC_BUF_LEN  3846 // 50ms per half //7692 // SampleRate/5, a little less than 100ms per half (99.996ms)
//7780//8000  // process each half (4000 = 100ms)
extern uint16_t evse_cp_adc_buffer[EVSE_CP_ADC_BUF_LEN];

// 1475 is -3V
#define EVSE_CP_RISING_TH 1475 //1875 // = 0.25V actual, value needed for a logic high
#define EVSE_CP_FALLING_TH 1475 //1875 // 1812 = -0.25V actual, value needed for a logic low


void AdcInit(void);
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc);
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc);



extern volatile uint16_t evse_cp_high; // raw adc 12bit value
extern volatile uint16_t evse_cp_low;
extern volatile uint32_t evse_cp_duty_x100;
extern volatile uint32_t evse_cp_freq_x10;

extern volatile int32_t evse_cp_high_Vx100;
extern volatile int32_t evse_cp_low_Vx100;




/*
 *
 *  1:state A, 2:State B1, 3:State B2, 4:State C, 5:State D, 6:State E, 7:State F, 8: Pilot error
 *   same as Zerova logs
 *
 *  Updated every cpADCdataProcess(~50ms)
 *
 */
extern volatile uint8_t evse_cp_prev_read_state; //previous 50ms state
extern volatile uint8_t evse_cp_cur_read_state; //latest 50ms state
extern volatile uint8_t evse_cp_debounce_read_state; //takes 2 consecutive same state measurements to change

extern volatile uint8_t isPLC; // if powerline comms / ISO15118
extern volatile uint16_t evse_cp_cur_amp_limit_x100;


void processCpADCdata10ms(void); // checks for a flag set every ~100ms

#endif /* SRC_AC_T_CPREAD_H_ */
