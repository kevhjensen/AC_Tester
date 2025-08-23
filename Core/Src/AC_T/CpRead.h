/*
 * myAdc.h
 *
 *  Created on: Jul 22, 2025
 *      Author: kevinj
 */

#ifndef SRC_AC_T_CPREAD_H_
#define SRC_AC_T_CPREAD_H_

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim3;


void AdcInit(void);
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc);
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc);



extern volatile int32_t evse_cp_high_Vx100;
extern volatile int32_t evse_cp_low_Vx100;
extern volatile uint32_t evse_cp_duty_x100;
extern volatile uint32_t evse_cp_freq_x10;


extern volatile int32_t ev_cp_high_Vx100;
extern volatile int32_t ev_cp_low_Vx100;
extern volatile uint32_t ev_cp_duty_x100;
extern volatile uint32_t ev_cp_freq_x10;

/* First 8 states are same Zerova log
 *
 * Updated every cpADCdataProcess(~10ms), debounced_state takes 2 consecutive to update
 *
 * PWM detected indicated by *, otherwise steady DC voltage
 */

typedef enum {           //    EV    |   EVSE    | Plugged In State |  EV State    |  EVSE State
				   	     //-------------------------------------------------------------------------
    CP_STATE_A     = 1,  //    0v    |    12v    | Unplugged        |  Not Ready   | Not Ready
    CP_STATE_B1    = 2,  //    9v    |    9v     | Plugged In       |  Not Ready   | Not Ready
    CP_STATE_B2    = 3,  //    9v*   |    9v*    | Plugged In       |  Not Ready   | Ready
    CP_STATE_C     = 4,  //    6v*   |    6v*    | Plugged In       |  Ready       | Ready      -> Charging
    CP_STATE_D     = 5,  //    3v*   |    3v*    | Plugged In       |  Fault       | Fault
    CP_STATE_E     = 6,  //    0v    |    0v     | Either           |  Not Ready   | Off
    CP_STATE_F     = 7,  //  N/A(0v) |   -12v    | Either           |  N/A(state E)| Fault
    CP_STATE_ERROR = 8,  //   other  |   other   | Either           |  Fault       | Fault
    CP_STATE_C1  = 9,    //    6v    |    6v     | Plugged In       |  Ready       | Not Ready
    CP_STATE_A2  = 10,   //    N/A   |   12v*    | Unplugged        |  N/A         | Not Ready
} cp_state_t;





extern volatile uint8_t evse_cp_cur_read_state; //latest 50ms state
extern volatile uint8_t evse_cp_debounce_read_state; //takes 2 consecutive same state measurements to change
extern volatile uint8_t evse_isPLC; // if powerline comms / ISO15118
extern volatile uint16_t evse_cp_cur_amp_limit_x100;


extern volatile uint8_t ev_cp_cur_read_state; //latest 50ms state
extern volatile uint8_t ev_cp_debounce_read_state; //takes 2 consecutive same state measurements to change
extern volatile uint8_t ev_isPLC; // if powerline comms / ISO15118
extern volatile uint16_t ev_cp_cur_amp_limit_x100;


void processCpADCdata1ms(void); // checks for a flag set every ~10ms

#endif /* SRC_AC_T_CPREAD_H_ */
