/*
 * myHelpers.c
 *
 *  Created on: Jul 18, 2025
 *      Author: kevinj
 */


#include "AC_T_globals.h"


char buf[64];
char strTmp[64];

void addToTrace(char * s) {
    snprintf(buf, sizeof(buf), "[%ld] %s\r\n", HAL_GetTick(), s);
    CDC_Transmit_FS((uint8_t*)buf, strlen(buf));
}




// accurate
int32_t cp_V_x100_from_raw(uint16_t raw)
{
    return 165 + (330 * raw - 675675) * 11 / 4095;
}



#define ADS_RATIO_REAL_mV_TO_RAW 16384 // 2^23 / 512 (mV, gain 4), gives mV at ADS1220 input from its raw 24 signed int value
// base 2 so can divide float fast

#define AC_V_actual_mV_to_raw_mV_ratio    991       // actual AC mV = 991 * ADS1220 input mV for AC_T PCB
#define AC_V_RESISTOR_RATIO    991       // actual AC mV = 991 * ADS1220 input mV for AC_T PCB

#define AC_I_raw_mV_per_actual_A_x1000_AX48 4832 // charger type 1
#define AC_I_raw_mV_per_actual_A_x1000_AX80 3120 // 2
#define AC_I_raw_mV_per_actual_A_x1000_AW32 4000 // update this

volatile uint8_t charger_type = 1;
volatile uint32_t AC_I_raw_mV_per_actual_A_x1000 = AC_I_raw_mV_per_actual_A_x1000_AX48;
// CT_burdenResistance_divBy_CT_ratio_x1000000
// actual AC I = ADS1220 input mV / 3120 for AC_T PCB

// = 1000 * burdenResistance / (CT ratio / 1000) AX80 = 1000 * 7.8 / (2500 / 1000)



float AC_V_from_raw(float raw)
{
	return (raw * AC_V_actual_mV_to_raw_mV_ratio / ADS_RATIO_REAL_mV_TO_RAW) / 1000.0f;
}
float AC_I_from_raw(float raw)
{
	return (raw * 1000.0f / AC_I_raw_mV_per_actual_A_x1000) / ADS_RATIO_REAL_mV_TO_RAW;
}


float AC_I_raw_V_from_actual_A(float actual_A)
{
	return actual_A * AC_I_raw_mV_per_actual_A_x1000 / 1000000.0f;
}

uint16_t MCP_DP_DN_Value_from_A_setpoint(float A_setpoint)
{
	float output_diff_V = AC_I_raw_V_from_actual_A(A_setpoint);

	float volts =  1.65f + 5.0f * (output_diff_V / 2.0f); // output is scaled down 5x via resistors, /2 since output doubled to make diff output

	if (volts <= 0.0f)   return 0;
	if (volts >= 3.3f)   return 4095;
	return (uint16_t)((volts * (4095.0f / 3.3f)) + 0.5f); // half-up rounding
}


float actual_A_from_MCP_DP_DN_Value(uint16_t dac_value) // inverse of above function
{
    // Convert DAC code back to volts
    float volts = ((float)dac_value * 3.3f) / 4095.0f;

    float output_diff_V = ((volts - 1.65f) * 2.0f) / 5.0f;

    // Reverse AC_I_raw_V_from_actual_A()
    float A_setpoint = output_diff_V * (1000000.0f / AC_I_raw_mV_per_actual_A_x1000);

    return A_setpoint;
}


uint8_t update_AC_I_raw_mV_per_actual_A_x1000(uint32_t new_conv_factor) // may need to add limits
{
	AC_I_raw_mV_per_actual_A_x1000 = new_conv_factor;
	charger_type = 0;
	return 1;
}

uint8_t update_charger_type(uint8_t new_ct)
{
	switch (new_ct) {
	case 0:
		break;
	case 1:
		AC_I_raw_mV_per_actual_A_x1000 = AC_I_raw_mV_per_actual_A_x1000_AX48;
		break;
	case 2:
		AC_I_raw_mV_per_actual_A_x1000 = AC_I_raw_mV_per_actual_A_x1000_AX80;
		break;
	case 3:
		AC_I_raw_mV_per_actual_A_x1000 = AC_I_raw_mV_per_actual_A_x1000_AW32;
		break;
	default:
		return 0;
	}
	charger_type = new_ct;
	return 1;
}




