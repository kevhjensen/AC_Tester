/*
 * myHelpers.h
 *
 *  Created on: Jul 18, 2025
 *      Author: kevinj
 */

#ifndef SRC_AC_T_MYHELPERS_H_
#define SRC_AC_T_MYHELPERS_H_


#include <stdint.h>

extern char strTmp[64];
extern char buf[64]; //usb print buf

void addToTrace(char * s);

uint32_t int_sqrt64(uint64_t x);

int32_t cp_V_x100_from_raw(uint16_t raw);

int32_t AC_V_mV_from_raw(int32_t raw);
int32_t AC_I_mA_from_raw(int32_t raw);

int64_t AC_P_uW_from_raw(int64_t raw_power);
int64_t energy_uWh_from_uW_us(int64_t power_uW, int64_t time_us);

#endif /* SRC_AC_T_MYHELPERS_H_ */
