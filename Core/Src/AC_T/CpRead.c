


#include "AC_T_globals.h"

/*
 * Functions to init adc1 triggered by timer3 at 20kHz, data fed thru dma to buffer (no cpu needed)
 * Fills circular buffer, interrupt/callback triggers process buffer when first half then second half filled
 *
 * processEVSEcp / cp values updated at 100Hz (20kHz/ 400(buffer len))
 * now only updated at 50Hz, doesn't process 2nd half of buffer
 */



uint16_t evse_cp_adc_buffer[EVSE_CP_ADC_BUF_LEN];
uint8_t evse_cp_adc_ready = 0; //1 first half ready, 2 second half

void AdcInit(void)
{
	HAL_ADCEx_Calibration_Start(&hadc1);
	HAL_TIM_Base_Start(&htim3);  // Start TIM2
	HAL_ADC_Start_DMA(&hadc1, (uint32_t*)evse_cp_adc_buffer, EVSE_CP_ADC_BUF_LEN);  // Start ADC DMA
	HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 2, 0);
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc) {
    if (hadc->Instance == ADC1) {
        // Process first half
    	evse_cp_adc_ready = 1;
    }
}

//
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    if (hadc->Instance == ADC1) {
        // Process second half
    	evse_cp_adc_ready = 2;
    }
}
volatile uint16_t evse_cp_high = 0; // raw adc 12bit value
volatile uint16_t evse_cp_low = 0;
volatile uint32_t evse_cp_duty_x100 = 0;
volatile uint32_t evse_cp_freq_x10 = 0;

volatile int32_t evse_cp_high_Vx100 = 0;
volatile int32_t evse_cp_low_Vx100 = 0;


void ProcessEvseCpADC(uint16_t *samples, uint16_t len)
{
    uint16_t vmax = 0;
    uint16_t vmin = 0xFFFF;

    uint32_t high_count = 0;
    uint16_t first_rise = 0;
    uint16_t last_rise = 0;
    uint16_t rising_edges = 0;

    uint8_t prev_low = (samples[0] < EVSE_CP_FALLING_TH);

    for (uint16_t i = 0; i < len; i++) {
        uint16_t s = samples[i];

        if (s > vmax) vmax = s;
        if (s < vmin) vmin = s;

        if (s >= EVSE_CP_RISING_TH) {
            if (prev_low) {
                if (rising_edges == 0) first_rise = i;
                last_rise = i;
                rising_edges++;
            }
            high_count++;
            prev_low = 0;
        }
        else if (s < EVSE_CP_FALLING_TH) {
            prev_low = 1;
        }
    }

    evse_cp_high = vmax;
    evse_cp_low  = vmin;

    evse_cp_high_Vx100 = cp_V_x100_from_raw(evse_cp_high);
    evse_cp_low_Vx100  = cp_V_x100_from_raw(evse_cp_low);


    if (rising_edges >= 2 && last_rise > first_rise) {
        uint16_t period_samples = last_rise - first_rise;
        evse_cp_duty_x100 = (10000 * high_count) / len;

        evse_cp_freq_x10 = (ADC_SAMPLE_RATE_x10 * (rising_edges - 1)) / period_samples;
    } else {
        evse_cp_duty_x100 = 0;
        evse_cp_freq_x10 = 0;
    }
}



/*
 * 1:state A, 2:State B1, 3:State B2, 4:State C, 5:State D, 6:State E, 7:State F, 8: Pilot error, // same as Zerova logs
 *
 * Add on states:
 * 9: State C1 (6v no pwm) vehicle should never be in this state
 * 10: State A2 (12v high pwm) Unplug from state C, will be a very brief period of state 10
 *
 *
 * Determines J-1772 state with adc data (+-1v or more inaccuarcy, especailly at low voltage
 * 0.5V extra range on each end compared to J-1772 (+-1 V range)
 *
 * No PWM (frequency <100hz)
 * 	A: low>=1050 high<1350
 * 	B1 low>750 high < 1050
 * 	E > -250 <250
 * 	F > -1600 <-1100
 * 	other = error state
 * 	9: >500 <=750
 * :
 *
 *
 * InValid PWM: (pilot error)
 * 	Frequency <900hz or >1100hz
 * 	Low V <-16V or >-8V
 *		DC < 3% or 7% < DC < 8% or DC > 97%
 *
 *	Valid PWM states:
 * 		B2 high>=750 high<1100
 * 		C high >450 high<750
 * 		D high >100 high < 400
 *
 * -1650 min of adc
 */

uint8_t Get_EVSE_CP_State(void)
{
    // No PWM detected (low frequency) or duty cycle at the edge
    if (evse_cp_freq_x10 <= 1000) {
        if (evse_cp_low_Vx100 >= 1050 && evse_cp_high_Vx100 < 1350)
            return 1; // State A
        else if (evse_cp_low_Vx100 > 750 && evse_cp_high_Vx100 < 1050)
            return 2; // State B1
        else if (evse_cp_low_Vx100 > -250 && evse_cp_high_Vx100 < 250)
            return 6; // State E
        else if (evse_cp_low_Vx100 > -1600 && evse_cp_high_Vx100 < -1100)
            return 7; // State F
        else if (evse_cp_low_Vx100 > 500 && evse_cp_high_Vx100 <= 750)
            return 9; // State C1 9
        else
            return 8; // Pilot error
    }


    // invalid PWM
    if (evse_cp_freq_x10 < 9000 || evse_cp_freq_x10 > 11000)
    	return 8;
    if (evse_cp_low_Vx100 < -1600 || evse_cp_low_Vx100 > -800)
    	return 8;

    if (evse_cp_duty_x100 > 700 && evse_cp_duty_x100 < 800)
    	return 8;
    if (evse_cp_duty_x100<300 || evse_cp_duty_x100>9700)
        	return 8;



    // valid PWM detected
    if (evse_cp_high_Vx100 >= 750 && evse_cp_high_Vx100 < 1050)
        return 3; // State B2
    else if (evse_cp_high_Vx100 > 450 && evse_cp_high_Vx100 < 750)
        return 4; // State C
    else if (evse_cp_high_Vx100 > 100 && evse_cp_high_Vx100 < 400)
        return 5; // State D
    else if (evse_cp_high_Vx100 >= 1050)
        return 10; // State 10 A2

    return 8; // Pilot error
}


volatile uint8_t evse_cp_prev_read_state = 0; //previous 100ms state
volatile uint8_t evse_cp_cur_read_state = 0; //latest 100ms state
volatile uint8_t evse_cp_debounce_read_state = 0; //takes 2 consecutive same state measurements to change

volatile uint8_t isPLC = 0; // if powerline comms / ISO15118

volatile uint16_t evse_cp_cur_amp_limit_x100 = 0;


void updateEvseCpStateAndAmpLimit(void)
{
	evse_cp_prev_read_state = evse_cp_cur_read_state;
	evse_cp_cur_read_state = Get_EVSE_CP_State();

	if (evse_cp_cur_read_state == evse_cp_prev_read_state) {
			evse_cp_debounce_read_state = evse_cp_cur_read_state;
		}

	// only set amp limit if in state C (for 2 cycles, so data is clean now, no half 1 state half another state)
	// DC < 3% or 7% < DC < 8% or DC > 97% already handled
	if (evse_cp_debounce_read_state == 4) {
		if (evse_cp_duty_x100 <= 700)
		{
			evse_cp_cur_amp_limit_x100 = 0;
			isPLC = 1;
		}
		else
		{
			isPLC = 0;
		}
		if (evse_cp_duty_x100 <= 1000) //8 to 10% is 6A
		{
			evse_cp_cur_amp_limit_x100 = 600;
		}
		else if (evse_cp_duty_x100 <= 8500)
		{
			evse_cp_cur_amp_limit_x100 = evse_cp_duty_x100 * 0.6;
		}
		else if (evse_cp_duty_x100 <= 9600)
		{
			evse_cp_cur_amp_limit_x100 = (evse_cp_duty_x100 - 6400) * 2.5;
		}
		else if (evse_cp_duty_x100 <= 9700)
		{
			evse_cp_cur_amp_limit_x100 = 8000;
		}

	}
	else {
		evse_cp_cur_amp_limit_x100 = 0;
		isPLC = 0;
	}
}


// checks for a flag that arises every ~100ms (99.996ms)
// updates global CP state variables
// requires 2 consecutive state to update evse_cp_read_state
void processCpADCdata10ms(void)
{

	if (evse_cp_adc_ready == 1) {
		ProcessEvseCpADC(evse_cp_adc_buffer, EVSE_CP_ADC_BUF_LEN / 2); //10ms of data (10 pwm cycles) 200samples at 20kHz

	}
	else if (evse_cp_adc_ready == 2) {
		ProcessEvseCpADC(&evse_cp_adc_buffer[EVSE_CP_ADC_BUF_LEN / 2], EVSE_CP_ADC_BUF_LEN / 2);
	}

	if (evse_cp_adc_ready != 0) {

		updateEvseCpStateAndAmpLimit();
		evse_cp_adc_ready = 0;

		if (data_mode == 6 || data_mode == 7) {
					snprintf(buf, sizeof(buf),
							 "CP %lu, %li, %li, %lu, %lu, %u, %u, %u\r\n",
							 HAL_GetTick(),
							 evse_cp_high_Vx100,   // High voltage (x100)
							 evse_cp_low_Vx100,    // Low voltage (x100)
							 evse_cp_duty_x100,
							 evse_cp_freq_x10,
							 evse_cp_debounce_read_state,
							 evse_cp_cur_amp_limit_x100,
							 isPLC);
					CDC_Transmit_FS((uint8_t*)buf, strlen(buf));

				}
	}

}
