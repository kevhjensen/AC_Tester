


#include "AC_T_globals.h"

/*
 * Functions to init adc1 triggered by timer3 at 20kHz, data fed thru dma to buffer (no cpu needed)
 * Fills circular buffer, interrupt/callback triggers process buffer when first half then second half filled
 *
 * processEVSEcp / cp values updated at 100Hz (20kHz/ 400(buffer len))
 * now only updated at 50Hz, doesn't process 2nd half of buffer
 */


#define NUM_CH   2

#define ADC_SAMPLE_RATE_x10 3977900
#define CP_ADC_BUF_LEN 7956 // every 10.0001 ms // 39779 50ms

static uint16_t cp_adc_buf[CP_ADC_BUF_LEN * NUM_CH];

uint8_t cp_adc_ready = 0; //1 first half ready, 2 second half


void AdcInit(void)
{
	HAL_ADCEx_Calibration_Start(&hadc1);
	HAL_TIM_Base_Start(&htim3);  // Start TIM2

	HAL_ADC_Start_DMA(&hadc1, (uint32_t*)cp_adc_buf, CP_ADC_BUF_LEN * NUM_CH);  // Start ADC DMA
	HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 2, 0);
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc) {
    if (hadc->Instance == ADC1) {
        // Process first half
    	cp_adc_ready = 1;
    }
}

//
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    if (hadc->Instance == ADC1) {
        // Process second half
    	cp_adc_ready = 2;
    }
}


volatile int32_t evse_cp_high_Vx100 = 0;
volatile int32_t evse_cp_low_Vx100 = 0;
volatile uint32_t evse_cp_duty_x100 = 0;
volatile uint32_t evse_cp_freq_x10 = 0;

volatile int32_t ev_cp_high_Vx100 = 0;
volatile int32_t ev_cp_low_Vx100 = 0;
volatile uint32_t ev_cp_duty_x100 = 0;
volatile uint32_t ev_cp_freq_x10 = 0;




#define EVSE_CP_RISING_TH 1475 // -3v
#define EVSE_CP_FALLING_TH 1475

#define EV_CP_RISING_TH 616 // 2V
#define EV_CP_FALLING_TH 616


void ProcessCpADC(uint16_t *samples, uint16_t len)
{
    uint16_t evse_cp_high = 0;
    uint16_t evse_cp_low = 0xFFFF;
    uint32_t high_count = 0;
    uint16_t first_rise = 0;
    uint16_t last_rise = 0;
    uint16_t rising_edges = 0;

    uint16_t ev_cp_high = 0;
	uint16_t ev_cp_low = 0xFFFF;
	uint32_t ev_high_count = 0;
	uint16_t ev_first_rise = 0;
	uint16_t ev_last_rise = 0;
	uint16_t ev_rising_edges = 0;

    uint8_t prev_low = (samples[0] < EVSE_CP_FALLING_TH);
    uint8_t ev_prev_low = (samples[0] < EV_CP_FALLING_TH);

    for (uint16_t i = 0; i < len; i++) {

        uint16_t ch1 = samples[i * NUM_CH + 0];
        uint16_t ch2 = samples[i * NUM_CH + 1];

        if (ch1 > evse_cp_high) evse_cp_high = ch1;
        if (ch1 < evse_cp_low) evse_cp_low = ch1;
        if (ch1 >= EVSE_CP_RISING_TH) {
            if (prev_low) {
                if (rising_edges == 0) first_rise = i;
                last_rise = i;
                rising_edges++;
            }
            high_count++;
            prev_low = 0;
        }
        else if (ch1 < EVSE_CP_FALLING_TH) {
            prev_low = 1;
        }

        if (ch2 > ev_cp_high) ev_cp_high = ch2;
        if (ch2 < ev_cp_low) ev_cp_low = ch2;
        if (ch2 >= EV_CP_RISING_TH) {
            if (ev_prev_low) {
                if (ev_rising_edges == 0) ev_first_rise = i;
                ev_last_rise = i;
                ev_rising_edges++;
            }
            ev_high_count++;
            ev_prev_low = 0;
        }
        else if (ch2 < EV_CP_FALLING_TH) {
        	ev_prev_low = 1;
        }
    }
    evse_cp_high_Vx100 = evse_cp_V_x100_from_raw(evse_cp_high);
    evse_cp_low_Vx100  = evse_cp_V_x100_from_raw(evse_cp_low);

    if (rising_edges >= 2 && last_rise > first_rise) {
        uint16_t period_samples = last_rise - first_rise;

        evse_cp_duty_x100 = (10000 * high_count) / len;
        evse_cp_freq_x10 = (ADC_SAMPLE_RATE_x10 * (rising_edges - 1)) / period_samples;
    }
    else {
        evse_cp_duty_x100 = 0;
        evse_cp_freq_x10 = 0;
    }


    ev_cp_high_Vx100 = ev_cp_V_x100_from_raw(ev_cp_high);
    ev_cp_low_Vx100 = ev_cp_V_x100_from_raw(ev_cp_low);

    if (ev_rising_edges >= 2 && ev_last_rise > ev_first_rise) {
        uint16_t ev_period_samples = ev_last_rise - ev_first_rise;

         ev_cp_duty_x100 = (10000 * ev_high_count) / len;
         ev_cp_freq_x10 = (ADC_SAMPLE_RATE_x10 * (ev_rising_edges - 1)) / ev_period_samples;
    }
    else {
        ev_cp_duty_x100 = 0;
        ev_cp_freq_x10 = 0;
    }

}



/* Since state can change in middle of an ADC read, could have weird state during transient. Use debounced_state
 *
 * 1:state A, 2:State B1, 3:State B2, 4:State C, 5:State D, 6:State E, 7:State F, 8: Pilot error, // same as Zerova logs
 *
 * Add on states:
 * 9: State C1 (6v no pwm) EVSE ended charge
 * 10: State A2 (12v high -12V pwm) Unplug from state C, will be a very brief period of state 10
 *
 *
 * Vehicle can't detect state F or negative
 *
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
    if (evse_cp_freq_x10 < 10) {
        if (evse_cp_low_Vx100 >= 1050 && evse_cp_high_Vx100 < 1350)
            return 1; // State A
        else if (evse_cp_low_Vx100 > 750 && evse_cp_high_Vx100 < 1050)
            return 2; // State B1
        else if (evse_cp_low_Vx100 > -250 && evse_cp_high_Vx100 < 250)
            return 6; // State E
        else if (evse_cp_low_Vx100 > -1600 && evse_cp_high_Vx100 < -1100)
            return 7; // State F
        else if (evse_cp_low_Vx100 > 450 && evse_cp_high_Vx100 <= 750)
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

uint8_t Get_EV_CP_State(void)
{
    // No PWM detected (low frequency) or duty cycle at the edge
    if (ev_cp_freq_x10 < 10) {
        if (ev_cp_low_Vx100 > 750 && ev_cp_high_Vx100 < 1050)
            return 2; // State B1
        else if (ev_cp_high_Vx100 < 250)
            return 1; // State A
//        else if (ev_cp_low_Vx100 > -1600 && ev_cp_high_Vx100 < -1100)
//            return 7; // State F
        else if (ev_cp_low_Vx100 > 450 && ev_cp_high_Vx100 <= 750)
            return 9; // State C1 9
        else
            return 8; // Pilot error
    }


    // invalid PWM
    if (ev_cp_freq_x10 < 9000 || ev_cp_freq_x10 > 11000)
    	return 8;

    if (ev_cp_duty_x100 > 700 && ev_cp_duty_x100 < 800)
    	return 8;
    if (ev_cp_duty_x100<300 || ev_cp_duty_x100>9700)
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








volatile uint8_t evse_cp_prev_prev_read_state = 0; //2 previous ago 10ms state
volatile uint8_t evse_cp_prev_read_state = 0; //previous 10ms state
volatile uint8_t evse_cp_cur_read_state = 0; //latest 10ms state
volatile uint8_t evse_cp_debounce_read_state = 0; //takes 2 consecutive same state measurements to change
volatile uint8_t evse_cp_last_debounce_read_state = 0;
volatile uint8_t evse_isPLC = 0; // if powerline comms / ISO15118
volatile uint16_t evse_cp_cur_amp_limit_x100 = 0;

volatile uint8_t evse_numCurNotEqualLastState = 0; // not likely but if state keeps switching (more than 5) then error

void updateEvseCpStateAndAmpLimit(void)
{
	evse_cp_prev_prev_read_state = evse_cp_prev_read_state;
	evse_cp_prev_read_state = evse_cp_cur_read_state;
	evse_cp_cur_read_state = Get_EVSE_CP_State();

	evse_cp_last_debounce_read_state = evse_cp_debounce_read_state;
	if (evse_cp_cur_read_state == evse_cp_prev_read_state && evse_cp_prev_prev_read_state == evse_cp_prev_read_state) {
		evse_cp_debounce_read_state = evse_cp_cur_read_state;
		evse_numCurNotEqualLastState = 0;
	}
	else {
		evse_numCurNotEqualLastState++;
		if (evse_numCurNotEqualLastState > 5) {
			evse_cp_debounce_read_state = 8;
		}
	}


	// only set amp limit if in state C (for 2 cycles, so data is clean now, no half 1 state half another state)
	// DC < 3% or 7% < DC < 8% or DC > 97% already handled
	if (evse_cp_debounce_read_state == 4) {
		if (evse_cp_duty_x100 <= 700)
		{
			evse_cp_cur_amp_limit_x100 = 0;
			evse_isPLC = 1;
		}
		else
		{
			evse_isPLC = 0;
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
		evse_isPLC = 0;
	}
}


volatile uint8_t ev_cp_prev_prev_read_state = 0;   // 2 previous ago 10ms state
volatile uint8_t ev_cp_prev_read_state = 0;        // previous 10ms state
volatile uint8_t ev_cp_cur_read_state = 0;         // latest 10ms state
volatile uint8_t ev_cp_debounce_read_state = 0;    // takes 2 consecutive same state measurements to change
volatile uint8_t ev_cp_last_debounce_read_state = 0;
volatile uint8_t ev_isPLC = 0;                     // if powerline comms / ISO15118
volatile uint16_t ev_cp_cur_amp_limit_x100 = 0;

volatile uint8_t ev_numCurNotEqualLastState = 0;   // if state keeps switching (more than 5) then error
void updateEvCpStateAndAmpLimit(void)
{
    ev_cp_prev_prev_read_state = ev_cp_prev_read_state;
    ev_cp_prev_read_state      = ev_cp_cur_read_state;
    ev_cp_cur_read_state       = Get_EV_CP_State();

    ev_cp_last_debounce_read_state = ev_cp_debounce_read_state;
    if (ev_cp_cur_read_state == ev_cp_prev_read_state &&
        ev_cp_prev_prev_read_state == ev_cp_prev_read_state) {

        ev_cp_debounce_read_state = ev_cp_cur_read_state;
        ev_numCurNotEqualLastState = 0;
    }
    else {
        ev_numCurNotEqualLastState++;
        if (ev_numCurNotEqualLastState > 5) {
            ev_cp_debounce_read_state = 8;
        }
    }

    // only set amp limit if in state C (for 3 cycles, so data is clean now)
    if (ev_cp_debounce_read_state == 4) {
        if (ev_cp_duty_x100 <= 700) {
            ev_cp_cur_amp_limit_x100 = 0;
            ev_isPLC = 1;
        }
        else {
            ev_isPLC = 0;
        }

        if (ev_cp_duty_x100 <= 1000) { // 8–10% is 6A
            ev_cp_cur_amp_limit_x100 = 600;
        }
        else if (ev_cp_duty_x100 <= 8500) {
            ev_cp_cur_amp_limit_x100 = ev_cp_duty_x100* 0.6;
        }
        else if (ev_cp_duty_x100 <= 9600) {
            ev_cp_cur_amp_limit_x100 = (ev_cp_duty_x100 - 6400) * 2.5;
        }
        else if (ev_cp_duty_x100 <= 9700) {
            ev_cp_cur_amp_limit_x100 = 8000;
        }
    }
    else {
        ev_cp_cur_amp_limit_x100 = 0;
        ev_isPLC = 0;
    }
}



// checks for a flag that arises every ~10ms
// updates global CP state variables
void processCpADCdata1ms(void)
{

	if (cp_adc_ready == 1) {
		ProcessCpADC(cp_adc_buf, CP_ADC_BUF_LEN / 2); //10ms of data (10 pwm cycles) 200samples at 20kHz

	}
	else if (cp_adc_ready == 2) {
		ProcessCpADC(&cp_adc_buf[CP_ADC_BUF_LEN / 2], CP_ADC_BUF_LEN / 2);
	}

	if (cp_adc_ready != 0) {

		updateEvseCpStateAndAmpLimit();
		updateEvCpStateAndAmpLimit();

		cp_adc_ready = 0;


		if (data_mode == 8 && ev_cp_last_debounce_read_state != ev_cp_debounce_read_state) {
			snprintf(buf, sizeof(buf),
				 "EV CP %u, %u",
				 ev_cp_debounce_read_state,
				 ev_cp_cur_amp_limit_x100
				 );
			addToMsgQ(buf);

			snprintf(buf, sizeof(buf),
				 "AC %.3f, %.3f, %.3f, %.4f, %u",
				 cur_AC_data.Vrms,           // Vrms in volts
				 cur_AC_data.Irms,           // Irms in amps
				 cur_AC_data.RealPower_W/1000.0f,      // kWs
				 AC_EnergyRegister_Wh/1000.0f,
				 isMoreThan60Vac_onChargerOut
				 );
			addToMsgQ(buf);
		}

		if (data_mode == 9 && evse_cp_last_debounce_read_state != evse_cp_debounce_read_state) {
		    snprintf(buf, sizeof(buf),
		             "EVSE CP %u,%u,%u,%u,%li,%li,%lu,%lu,%u",
		             evse_cp_debounce_read_state,
		             evse_cp_cur_read_state,
		             evse_cp_cur_amp_limit_x100,
		             evse_isPLC,
		             evse_cp_high_Vx100,   // High voltage (x100)
		             evse_cp_low_Vx100,    // Low voltage (x100)
		             evse_cp_duty_x100,
		             evse_cp_freq_x10,
		             isMoreThan60Vac_onChargerOut
		    );
		    addToMsgQ(buf);

		    snprintf(buf, sizeof(buf),
		             "EV CP %u,%u,%u,%u,%li,%li,%lu,%lu",
		             ev_cp_debounce_read_state,
		             ev_cp_cur_read_state,
		             ev_cp_cur_amp_limit_x100,
		             ev_isPLC,
		             ev_cp_high_Vx100,     // High voltage (x100)
		             ev_cp_low_Vx100,      // Low voltage (x100)
		             ev_cp_duty_x100,
		             ev_cp_freq_x10
		    );
		    addToMsgQ(buf);
		}

		if (data_mode == 5 || data_mode == 6) {
		    snprintf(buf, sizeof(buf),
		             "EVSE CP %u, %u, %u, %u, %li, %li, %lu, %lu, %u",
		             evse_cp_debounce_read_state,
		             evse_cp_cur_read_state,
		             evse_cp_cur_amp_limit_x100,
		             evse_isPLC,
		             evse_cp_high_Vx100,
		             evse_cp_low_Vx100,
		             evse_cp_duty_x100,
		             evse_cp_freq_x10,
		             isMoreThan60Vac_onChargerOut
		    );
		    addToMsgQ(buf);

		    snprintf(buf, sizeof(buf),
		             "EV CP %u, %u, %u, %u, %li, %li, %lu, %lu",
		             ev_cp_debounce_read_state,
		             ev_cp_cur_read_state,
		             ev_cp_cur_amp_limit_x100,
		             ev_isPLC,
		             ev_cp_high_Vx100,
		             ev_cp_low_Vx100,
		             ev_cp_duty_x100,
		             ev_cp_freq_x10
		    );
		    addToMsgQ(buf);
		}


	}

}
