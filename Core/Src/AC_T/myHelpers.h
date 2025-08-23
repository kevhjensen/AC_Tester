/*
 * myHelpers.h
 *
 *  Created on: Jul 18, 2025
 *      Author: kevinj
 */

#ifndef SRC_AC_T_MYHELPERS_H_
#define SRC_AC_T_MYHELPERS_H_


/*
 * microsecond time
 */
extern TIM_HandleTypeDef htim2; //1 Mhz counter, 65.6ms overflow

uint16_t micros();

extern char buf[64]; //usb print buf



void addToMsgQ(const char * s);


int32_t ev_cp_V_x100_from_raw(uint16_t raw);
int32_t evse_cp_V_x100_from_raw(uint16_t raw);


float AC_V_from_raw(float raw);
float AC_I_from_raw(float raw);

uint16_t MCP_DP_DN_Value_from_A_setpoint(float A_setpoint);
float actual_A_from_MCP_DP_DN_Value(uint16_t dac_value);


uint8_t update_AC_I_raw_mV_per_actual_A_x1000(uint32_t new_conv_factor);
uint8_t update_charger_type(uint8_t new_ct);

#endif /* SRC_AC_T_MYHELPERS_H_ */
